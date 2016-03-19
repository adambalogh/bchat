cmake_minimum_required(VERSION 3.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")

include_directories(${PROJECT_SOURCE_DIR}/src/)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(main src/chat/main.cc src/sender.cc src/chat/user.cc src/proto/message.pb.cc)
set_target_properties(main 
  PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG")
target_link_libraries(main uv protobuf hiredis)

add_executable(parser_test src/parser_test.cc)
target_link_libraries(parser_test gmock gtest)

add_executable(parser_benchmark src/parser_benchmark.cc)
set_target_properties(parser_benchmark 
  PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG")
target_link_libraries(parser_benchmark benchmark)

add_executable(user_test src/chat/user_test.cc src/chat/user.cc src/proto/message.pb.cc)
target_link_libraries(user_test protobuf gmock gtest)

add_executable(message_storage_test src/chat/message_storage_test.cc src/proto/message.pb.cc)
target_link_libraries(message_storage_test protobuf gmock gtest)

add_executable(user_benchmark src/chat/user.cc src/chat/user_benchmark.cc
  src/proto/message.pb.cc)
set_target_properties(user_benchmark 
  PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG")
target_link_libraries(user_benchmark protobuf gmock benchmark)

