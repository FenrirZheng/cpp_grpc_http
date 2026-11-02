#!/usr/bin/env bash
# Generate everything derived from proto/quotes/v1/quotes.proto except the C++
# stubs (CMake generates those):
#   envoy/proto.pb   descriptor set for the Envoy transcoder
#   gateway/gen/     Go messages, gRPC client and gRPC-Gateway handlers
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
inc=(-Iproto -Ithird_party/googleapis)

protoc "${inc[@]}" --include_imports --include_source_info \
  --descriptor_set_out=envoy/proto.pb quotes/v1/quotes.proto
echo "wrote envoy/proto.pb"

gobin=$(go env GOPATH)/bin
export PATH=$gobin:$PATH
for p in protoc-gen-go protoc-gen-go-grpc protoc-gen-grpc-gateway; do
  command -v "$p" >/dev/null || { echo "missing $p: run 'cd gateway && go install tool' or see README" >&2; exit 1; }
done

rm -rf gateway/gen && mkdir -p gateway/gen
protoc "${inc[@]}" \
  --go_out=gateway/gen --go_opt=module=example.com/cpp_grpc_http/gateway/gen \
  --go-grpc_out=gateway/gen --go-grpc_opt=module=example.com/cpp_grpc_http/gateway/gen \
  --grpc-gateway_out=gateway/gen --grpc-gateway_opt=module=example.com/cpp_grpc_http/gateway/gen \
  quotes/v1/quotes.proto
echo "wrote gateway/gen/"
