# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A runnable example for the org-roam note
`~/code/org-roam/codeVault/grpc/REST_to_gRPC_Transcoding_With_Proto_Annotations.org`:
a C++ gRPC service served as REST/JSON through two independent proxies, with
the REST mapping living only in `proto/quotes/v1/quotes.proto`. The point being
demonstrated is that the C++ handlers stay REST-agnostic — keep it that way.
`GetChart` (returns `google.api.HttpBody`) is the single deliberate exception.

## Commands

```bash
bash scripts/gen.sh            # proto -> envoy/proto.pb and gateway/gen/ (rerun after any .proto edit)
cmake -S server -B build -G Ninja && cmake --build build   # host build; needs scripts/install-deps.sh (sudo)
docker build -f server/Dockerfile -t quotes-server .        # container build; no host gRPC needed
docker run --rm --network host quotes-server                # :50051
bash scripts/run-envoy.sh      # Envoy :8080 (pinned envoyproxy/envoy:v1.39.6, host network)
(cd gateway && go run .)       # gRPC-Gateway :8081
bash scripts/smoke.sh          # same curl set against both proxies; there is no unit test suite
cd gateway && go install tool  # installs protoc-gen-go / -go-grpc / -grpc-gateway (tool deps in go.mod)
```

## Architecture

- **Single source of truth**: `proto/quotes/v1/quotes.proto`. Three consumers
  generate from it independently: CMake (C++ stubs into `build/gen/`),
  `scripts/gen.sh` (Envoy descriptor `envoy/proto.pb` and Go code in
  `gateway/gen/`). All generated output is gitignored.
- **googleapis protos are vendored** in `third_party/googleapis/google/api/`
  (`annotations`, `http`, `httpbody`). Every protoc call passes
  `-Iproto -Ithird_party/googleapis`. CMake compiles their `.pb.cc` into the
  server because `quotes.pb.cc` and `HttpBody` depend on them.
- **Envoy** (`envoy/envoy.yaml`): transcoded requests are rewritten to
  `POST /quotes.v1.Quotes/<Method>` before routing, so the route matches that
  prefix, not `/v1/`. `services:` must name `quotes.v1.Quotes` exactly or Envoy
  refuses to start. The upstream cluster must be HTTP/2. Validate a config change
  with `docker run ... envoyproxy/envoy:v1.39.6 --mode validate -c /etc/envoy/envoy.yaml`
  (mount the yaml and `proto.pb` like `scripts/run-envoy.sh` does).
- **Gateway** (`gateway/main.go`): a plain gRPC client of the C++ server;
  the Go module path is `example.com/cpp_grpc_http/gateway`, which must match the
  `go_package` option and the `module=` opts in `scripts/gen.sh`.
- **C++ build** (`server/CMakeLists.txt`) tries `find_package(gRPC CONFIG)` and
  falls back to pkg-config, so it works with distro packages.

Measured behavior differences between the two proxies (streaming format,
unknown query params) are recorded in `README.md`; update that table when a
change alters `scripts/smoke.sh` output.
