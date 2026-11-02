# cpp_grpc_http — REST in front of a C++ gRPC service

Runnable example for the org-roam note
*REST to gRPC Transcoding With Proto Annotations*
(`~/code/org-roam/codeVault/grpc/REST_to_gRPC_Transcoding_With_Proto_Annotations.org`).

One annotated `.proto`, one C++ gRPC server, two independent REST front ends:

```
curl ──REST──> Envoy :8080  (grpc_json_transcoder) ──gRPC/h2──┐
curl ──REST──> Go gRPC-Gateway :8081 ──────────────gRPC/h2──┼──> quotes_server :50051 (C++)
```

| Note section | Where it lives |
|---|---|
| The annotation | `proto/quotes/v1/quotes.proto` (`option (google.api.http)`) |
| Envoy: gRPC-JSON transcoder | `envoy/envoy.yaml`, descriptor from `scripts/gen.sh` |
| gRPC-Gateway | `gateway/main.go`, handlers generated into `gateway/gen/` |
| Status codes and streaming | `scripts/smoke.sh` output below |
| Gotcha: `request_validation_options` | `envoy/envoy.yaml` |
| Gotcha: `google.api.HttpBody` changes C++ | `GetChart` in `server/quotes_service.cc` |

`server/quotes_service.cc` has no REST, JSON or path logic; only `GetChart`
touches HTTP concepts (content type of an `HttpBody`).

## Run

```bash
bash scripts/gen.sh                  # envoy/proto.pb + gateway/gen/  (needs Go plugins, below)

# C++ server, either on the host (after: bash scripts/install-deps.sh, needs sudo) ...
cmake -S server -B build -G Ninja && cmake --build build && ./build/quotes_server
# ... or in a container (no host gRPC needed)
docker build -f server/Dockerfile -t quotes-server . && docker run --rm --network host quotes-server

bash scripts/run-envoy.sh            # :8080, envoyproxy/envoy:v1.39.6
(cd gateway && go run .)             # :8081
bash scripts/smoke.sh
```

Go protoc plugins are declared as `tool` deps in `gateway/go.mod`; install them with
`cd gateway && go install tool` (lands in `$(go env GOPATH)/bin`).

## Measured results (2026-11-02)

Envoy v1.39.6, grpc-gateway v2 (Go 1.26), gRPC C++ 1.51.1 (Debian 13), output of `scripts/smoke.sh`:

| Request | Envoy :8080 | gRPC-Gateway :8081 |
|---|---|---|
| `GET /v1/quotes/1` | 200 JSON | 200 JSON |
| `GET /v1/quotes/999` (`NOT_FOUND`) | 404 `{"code":5,...}` | 404 `{"code":5,...}` |
| `POST /v1/quotes` body `*` | 200 | 200 |
| `POST` without symbol (`INVALID_ARGUMENT`) | 400 | 400 |
| `GET /v1/quotes?min_price=20` (server stream) | one JSON **array** `[{...},{...}]` | newline-separated `{"result":{...}}` objects |
| `GET /v1/quotes/2/chart` (`HttpBody`) | 200 `image/svg+xml` | 200 `image/svg+xml` |
| `GET /v1/nope` | 404 text/plain (`reject_unknown_method`) | 404 JSON |
| `GET /v1/quotes/1?bogus=1` | 400 text/plain (`reject_unknown_query_parameters`) | **200** — unknown param ignored |

The last row is a real difference: gRPC-Gateway ignores unknown query
parameters, while Envoy with `reject_unknown_query_parameters` rejects them.
