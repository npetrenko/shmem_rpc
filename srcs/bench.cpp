#include "channel.hpp"

#include <rpc.pb.h>
#include <rpc.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include <iostream>
#include <memory>
#include <thread>
#include <random>

#include <benchmark/benchmark.h>

std::string RandomString(size_t lenght, std::mt19937* rd) {
    std::uniform_int_distribution<char> distr;
    std::string out;
    out.reserve(lenght);
    for (size_t i = 0; i < lenght; ++i) {
        out.push_back(distr(*rd));
    }

    return out;
}

class EchoServiceImpl final : public EchoService::Service {
public:
    grpc::Status Echo(grpc::ServerContext*, const Message* request, Message* response) override {
        response->set_data(request->data());
        return grpc::Status::OK;
    }
};

class EchoClient {
public:
    explicit EchoClient(std::shared_ptr<grpc::Channel> channel) : stub_(std::move(channel)) {
    }

    std::string Echo(std::string ping) {
        Message message;
        message.set_data(std::move(ping));

        grpc::ClientContext ctx;

        Message response;
        grpc::Status status = stub_.Echo(&ctx, message, &response);

        return response.data();
    }

private:
    EchoService::Stub stub_;
};

void RunServer(std::atomic<grpc::Server*>* service_store) {
    std::string server_address("0.0.0.0:50051");

    EchoServiceImpl pong;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&pong);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    // std::cout << "Server listening on " << server_address << std::endl;

    service_store->store(server.get());
    server->Wait();
}

EchoClient NewClient() {
    return EchoClient{grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())};
}

void RunBenchmark(benchmark::State& state) {
    std::mt19937 rd{1234};
    const std::string echo_message = RandomString(state.range(0), &rd);

    std::atomic<grpc::Server*> server{nullptr};
    std::thread server_thread{[&] { RunServer(&server); }};
    auto client = NewClient();

    while (state.KeepRunning()) {
        if (client.Echo(echo_message) != echo_message) {
            std::terminate();
        }
    }

    while (!server.load()) {
    }
    server.load()->Shutdown();
    server_thread.join();
}

void RunEncodeDecode(benchmark::State& state) {
    std::mt19937 rd{1234};
    const std::string inp = RandomString(state.range(0), &rd);

    while (state.KeepRunning()) {
        Message sender;
        sender.set_data(inp);
        std::string encoded = sender.SerializeAsString();

        Message reciever;
        reciever.ParseFromString(encoded);
        if (reciever.data() != inp) {
            std::terminate();
        }
    }
}

class ShmemEchoServerImpl {
public:
    ShmemEchoServerImpl(OneWayChannelRef in, OneWayChannelRef out) : in_(in), out_(out) {
    }

    void Echo(const Message* request, Message* response) {
        response->set_data(request->data());
    }

    void Serve() {
        std::string buffer;
        while (true) {
            buffer.clear();

            in_->Read(&buffer);
            Message request;
            request.ParseFromString(buffer);
            buffer.clear();

            if (request.has_shutdownmessage()) {
                return;
            }

            Message response;
            Echo(&request, &response);

            response.SerializeToString(&buffer);
            out_->Write(buffer);
        }
    }

private:
    OneWayChannelRef in_, out_;
};

class ShmemEchoClientImpl {
public:
    ShmemEchoClientImpl(OneWayChannelRef in, OneWayChannelRef out) : in_(in), out_(out) {
    }

    void SendShutdown() {
        Message message;
        message.set_shutdownmessage(true);
        out_->Write(message.SerializeAsString());
    }

    std::string Echo(std::string inp) {
        std::string buffer = std::move(inp);

        {
            Message message;
            message.set_data(buffer);
            buffer.clear();

            message.SerializeToString(&buffer);
            out_->Write(buffer);
            buffer.clear();
        }

        in_->Read(&buffer);
        Message message;
        message.ParseFromString(buffer);

        return std::move(message).data();
    }

private:
    OneWayChannelRef in_, out_;
};

void RunShmemBenchmark(benchmark::State& state) {
    std::mt19937 rd{1234};
    const std::string echo_message = RandomString(state.range(0), &rd);

    auto serv_to_client = OneWayChannel::New();
    auto client_to_serv = OneWayChannel::New();
    ShmemEchoServerImpl server{client_to_serv, serv_to_client};
    ShmemEchoClientImpl client{serv_to_client, client_to_serv};

    std::thread server_thread{[&] { server.Serve(); }};

    while (state.KeepRunning()) {
        if (client.Echo(echo_message) != echo_message) {
            std::terminate();
        }
    }

    client.SendShutdown();

    server_thread.join();
}

BENCHMARK(RunBenchmark)->Range(4, 2 << 20);
BENCHMARK(RunEncodeDecode)->Range(4, 2 << 20);
BENCHMARK(RunShmemBenchmark)->Range(4, 2 << 20);

BENCHMARK_MAIN();
