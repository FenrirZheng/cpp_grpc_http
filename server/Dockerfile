# Build and run quotes_server without installing gRPC on the host.
# Same Debian 13 packages as scripts/install-deps.sh. Build from the repo root:
#   docker build -f server/Dockerfile -t quotes-server .
#   docker run --rm --network host quotes-server
FROM debian:trixie AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ cmake ninja-build pkg-config \
      libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY proto proto
COPY third_party third_party
COPY server server
RUN cmake -S server -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build

FROM debian:trixie
RUN apt-get update && apt-get install -y --no-install-recommends libgrpc++1.51t64 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/quotes_server /usr/local/bin/quotes_server
ENTRYPOINT ["quotes_server"]
