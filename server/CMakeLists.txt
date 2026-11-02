cmake_minimum_required(VERSION 3.16)
project(quotes_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PROTO_ROOT ${REPO_ROOT}/proto)
set(GOOGLEAPIS ${REPO_ROOT}/third_party/googleapis)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN_DIR})

# gRPC: prefer the CMake package config; fall back to pkg-config.
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
  find_package(Protobuf CONFIG QUIET)
  if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
  endif()
  set(GRPC_LIBS gRPC::grpc++ protobuf::libprotobuf)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++ protobuf)
  set(GRPC_LIBS PkgConfig::GRPCPP)
endif()

find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Messages for every proto; services only for quotes.proto.
set(MSG_PROTOS
  google/api/http.proto
  google/api/annotations.proto
  google/api/httpbody.proto
  quotes/v1/quotes.proto)

set(GEN_SRCS)
foreach(p ${MSG_PROTOS})
  string(REGEX REPLACE "\\.proto$" "" base ${p})
  list(APPEND GEN_SRCS ${GEN_DIR}/${base}.pb.cc)
endforeach()
list(APPEND GEN_SRCS ${GEN_DIR}/quotes/v1/quotes.grpc.pb.cc)

add_custom_command(
  OUTPUT ${GEN_SRCS}
  COMMAND ${PROTOC}
    -I${PROTO_ROOT} -I${GOOGLEAPIS}
    --cpp_out=${GEN_DIR}
    ${MSG_PROTOS}
  COMMAND ${PROTOC}
    -I${PROTO_ROOT} -I${GOOGLEAPIS}
    --grpc_out=${GEN_DIR}
    --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
    quotes/v1/quotes.proto
  DEPENDS ${PROTO_ROOT}/quotes/v1/quotes.proto
  WORKING_DIRECTORY ${REPO_ROOT}
  COMMENT "protoc: C++ messages + gRPC stubs")

add_executable(quotes_server main.cc quotes_service.cc ${GEN_SRCS})
target_include_directories(quotes_server PRIVATE ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(quotes_server PRIVATE ${GRPC_LIBS})
