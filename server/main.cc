#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "quotes_service.h"

int main(int argc, char** argv) {
  std::string addr = argc > 1 ? argv[1] : "0.0.0.0:50051";
  QuotesService service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  if (!server) {
    std::cerr << "failed to listen on " << addr << "\n";
    return 1;
  }
  std::cout << "quotes_server listening on " << addr << std::endl;
  server->Wait();
}
