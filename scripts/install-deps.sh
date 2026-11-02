#!/usr/bin/env bash
# One-time system deps for the C++ server (Debian 13: gRPC 1.51.1).
# Needs sudo; Go plugins and the Envoy image need no root.
set -euo pipefail
sudo apt-get install -y libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev protobuf-compiler
