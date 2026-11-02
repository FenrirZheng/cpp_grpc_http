#!/usr/bin/env bash
# Send the same REST requests through Envoy (:8080) and gRPC-Gateway (:8081).
# Both proxies must be running in front of quotes_server (:50051).
set -uo pipefail

req() { # name method url [json-body]
  local name=$1 method=$2 url=$3 body=${4-}
  local args=(-s -o /tmp/smoke.$$ -w '%{http_code} %{content_type}' -X "$method")
  [[ -n $body ]] && args+=(-H 'Content-Type: application/json' -d "$body")
  local meta; meta=$(curl "${args[@]}" "$url")
  printf '  %-26s %s\n' "$name" "$meta"
  sed 's/^/      /' /tmp/smoke.$$; echo
  rm -f /tmp/smoke.$$
}

for target in envoy:8080 gateway:8081; do
  name=${target%%:*} base=http://localhost:${target##*:}
  echo "== $name ($base)"
  req 'GET existing id'         GET  "$base/v1/quotes/1"
  req 'GET missing id'          GET  "$base/v1/quotes/999"
  req 'POST create'             POST "$base/v1/quotes" '{"symbol":"HOOLI","price":7.25}'
  req 'POST missing symbol'     POST "$base/v1/quotes" '{"price":1}'
  req 'GET stream ?min_price=20' GET "$base/v1/quotes?min_price=20"
  req 'GET chart (HttpBody)'    GET  "$base/v1/quotes/2/chart"
  req 'GET unknown path'        GET  "$base/v1/nope"
  req 'GET unknown query param' GET  "$base/v1/quotes/1?bogus=1"
done
