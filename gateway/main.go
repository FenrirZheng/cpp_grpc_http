// gRPC-Gateway reverse proxy: REST on :8081 -> gRPC on localhost:50051.
// The handlers are generated from the same annotated quotes.proto that Envoy
// reads; this process is a plain gRPC client of the C++ server.
package main

import (
	"context"
	"flag"
	"log"
	"net/http"

	"github.com/grpc-ecosystem/grpc-gateway/v2/runtime"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	quotesv1 "example.com/cpp_grpc_http/gateway/gen/quotes/v1"
)

func main() {
	listen := flag.String("listen", ":8081", "REST listen address")
	backend := flag.String("backend", "localhost:50051", "C++ gRPC server")
	flag.Parse()

	ctx := context.Background()
	mux := runtime.NewServeMux(
		// Unlike Envoy's fixed grpcToHttpStatus, error rendering is
		// replaceable here; this keeps the default mapping explicitly.
		runtime.WithErrorHandler(runtime.DefaultHTTPErrorHandler),
	)
	opts := []grpc.DialOption{grpc.WithTransportCredentials(insecure.NewCredentials())}
	if err := quotesv1.RegisterQuotesHandlerFromEndpoint(ctx, mux, *backend, opts); err != nil {
		log.Fatal(err)
	}
	log.Printf("gateway listening on %s -> %s", *listen, *backend)
	log.Fatal(http.ListenAndServe(*listen, mux))
}
