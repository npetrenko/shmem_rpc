function (add_benchmark TARGET EXECS)
    add_executable(${TARGET} ${EXECS})
    target_link_libraries(${TARGET} benchmark pthread)
endfunction()
