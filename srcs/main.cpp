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

// class PongServiceImpl final : public PongService::Service {
// public:
//     grpc::Status Pong(grpc::ServerContext*, const Message* request, Message* response) override {
//         if (request->data() == "ping") {
//             response->set_data("pong");
//             return grpc::Status::OK;
//         }

//         return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "expected \"ping\" as an argument");
//     }
// };

// class PingClient {
// public:
//     explicit PingClient(std::shared_ptr<grpc::Channel> channel)
//         : stub_(std::move(channel))
//     {
//     }

//     std::string Ping(std::string ping) {
//         Message message;
//         message.set_data(std::move(ping));

//         grpc::ClientContext ctx;

//         Message response;
//         grpc::Status status = stub_.Pong(&ctx, message, &response);

//         return response.data();
//     }

// private:
//     PongService::Stub stub_;
// };

// void RunServer(grpc::Server** service_store) {
//   std::string server_address("0.0.0.0:50051");

//   PongServiceImpl pong;
//   grpc::ServerBuilder builder;
//   builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//   builder.RegisterService(&pong);
//   std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//   // std::cout << "Server listening on " << server_address << std::endl;

//   *service_store = server.get();
//   server->Wait();
// }

// PingClient NewClient() {
//     return PingClient{grpc::CreateChannel("localhost:50051",
//                                           grpc::InsecureChannelCredentials())};
// }

// void RunClient(PingClient* client) {
//     std::cout << "Server response for ping is: " << client->Ping("ping") << std::endl;
// }

int main() {
    // grpc::Server* service = nullptr;
    // std::thread server_thread{[&] { RunServer(&service); }};

    // auto client = NewClient();
    // RunClient(&client);

    // service->Shutdown();
    // server_thread.join();

    return 0;
}
