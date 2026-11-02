#!/usr/bin/env bash
# Run Envoy (pinned tag) with host networking so it can reach 127.0.0.1:50051.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
[[ -f $root/envoy/proto.pb ]] || { echo "envoy/proto.pb missing: run scripts/gen.sh" >&2; exit 1; }
exec docker run --rm --name quotes-envoy --network host \
  -v "$root/envoy/envoy.yaml:/etc/envoy/envoy.yaml:ro" \
  -v "$root/envoy/proto.pb:/etc/envoy/proto.pb:ro" \
  envoyproxy/envoy:v1.39.6 -c /etc/envoy/envoy.yaml --log-level warn
