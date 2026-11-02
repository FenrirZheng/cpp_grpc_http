#pragma once

#include <map>
#include <mutex>
#include <string>

#include <grpcpp/grpcpp.h>

#include "quotes/v1/quotes.grpc.pb.h"

// Plain gRPC service. Nothing here knows about REST, JSON or HTTP paths,
// except GetChart, which returns google.api.HttpBody.
class QuotesService final : public quotes::v1::Quotes::Service {
 public:
  QuotesService();

  grpc::Status GetQuote(grpc::ServerContext*, const quotes::v1::GetQuoteRequest*,
                        quotes::v1::Quote*) override;
  grpc::Status CreateQuote(grpc::ServerContext*, const quotes::v1::CreateQuoteRequest*,
                           quotes::v1::Quote*) override;
  grpc::Status ListQuotes(grpc::ServerContext*, const quotes::v1::ListQuotesRequest*,
                          grpc::ServerWriter<quotes::v1::Quote>*) override;
  grpc::Status GetChart(grpc::ServerContext*, const quotes::v1::GetQuoteRequest*,
                        google::api::HttpBody*) override;

 private:
  std::mutex mu_;
  std::map<std::string, quotes::v1::Quote> quotes_;
  int next_id_ = 1;

  quotes::v1::Quote AddLocked(const std::string& symbol, double price);
};
