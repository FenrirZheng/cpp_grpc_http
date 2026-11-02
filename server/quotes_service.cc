#include "quotes_service.h"

#include <cstdio>

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using quotes::v1::CreateQuoteRequest;
using quotes::v1::GetQuoteRequest;
using quotes::v1::ListQuotesRequest;
using quotes::v1::Quote;

QuotesService::QuotesService() {
  std::lock_guard<std::mutex> lock(mu_);
  AddLocked("ACME", 12.5);
  AddLocked("GLOBEX", 99.0);
  AddLocked("INITECH", 42.0);
}

Quote QuotesService::AddLocked(const std::string& symbol, double price) {
  Quote q;
  q.set_id(std::to_string(next_id_++));
  q.set_symbol(symbol);
  q.set_price(price);
  quotes_[q.id()] = q;
  return q;
}

Status QuotesService::GetQuote(ServerContext*, const GetQuoteRequest* req, Quote* resp) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = quotes_.find(req->id());
  if (it == quotes_.end()) {
    return Status(StatusCode::NOT_FOUND, "quote " + req->id() + " not found");
  }
  *resp = it->second;
  return Status::OK;
}

Status QuotesService::CreateQuote(ServerContext*, const CreateQuoteRequest* req, Quote* resp) {
  if (req->symbol().empty()) {
    return Status(StatusCode::INVALID_ARGUMENT, "symbol is required");
  }
  std::lock_guard<std::mutex> lock(mu_);
  *resp = AddLocked(req->symbol(), req->price());
  return Status::OK;
}

Status QuotesService::ListQuotes(ServerContext*, const ListQuotesRequest* req,
                                 grpc::ServerWriter<Quote>* writer) {
  std::lock_guard<std::mutex> lock(mu_);
  for (const auto& [id, q] : quotes_) {
    if (q.price() >= req->min_price()) writer->Write(q);
  }
  return Status::OK;
}

// The exception the note describes: a non-JSON response needs HttpBody, and
// the handler itself chooses content_type and data.
Status QuotesService::GetChart(ServerContext*, const GetQuoteRequest* req,
                               google::api::HttpBody* resp) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = quotes_.find(req->id());
  if (it == quotes_.end()) {
    return Status(StatusCode::NOT_FOUND, "quote " + req->id() + " not found");
  }
  const Quote& q = it->second;
  int bar = static_cast<int>(q.price() / 5);
  char svg[256];
  std::snprintf(svg, sizeof svg,
                "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"20\">"
                "<rect width=\"%d\" height=\"20\" fill=\"steelblue\"/></svg>\n",
                bar, bar);
  resp->set_content_type("image/svg+xml");
  resp->set_data(svg);
  return Status::OK;
}
