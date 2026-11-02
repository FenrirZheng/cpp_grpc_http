# 計畫：REST → gRPC 轉碼範例（C++ 後端）

## Context

`/init` 帶的參數要求：替 org-roam 筆記
`~/code/org-roam/codeVault/grpc/REST_to_gRPC_Transcoding_With_Proto_Annotations.org`
寫一個可執行範例。筆記的論點：C++ gRPC 服務只要在 `.proto` 加 `google.api.http`
註記，前面擺 Envoy gRPC-JSON transcoder 或 gRPC-Gateway，就能服務 REST/JSON，
**C++ handler 不必改**（唯一例外是回傳 `google.api.HttpBody`）。

`/home/fenrir/code/cpp_grpc_http` 目前是空目錄。本機有 docker 29、go 1.24、
protoc 3.21、cmake、ninja；**缺** gRPC C++ 函式庫、`grpc_cpp_plugin`、envoy、googleapis protos。
使用者問題逾時未答，採推薦預設：兩條路都做、apt 安裝依賴、`git init` 獨立 repo（不 commit）。

## 目錄結構

```
proto/quotes/v1/quotes.proto        # 單一來源；帶 google.api.http 註記
third_party/googleapis/google/api/  # vendored: annotations.proto, http.proto, httpbody.proto
server/                             # C++ gRPC 服務（CMake）
  CMakeLists.txt  main.cc  quotes_service.{h,cc}
envoy/envoy.yaml                    # grpc_json_transcoder filter
gateway/                            # Go gRPC-Gateway 反向代理
  go.mod  main.go  gen/ (生成)
scripts/
  install-deps.sh   # sudo apt install libgrpc++-dev protobuf-compiler-grpc（使用者用 bash 跑）
  gen.sh            # protoc → envoy/proto.pb 描述檔 + gateway/gen Go stubs
  run-envoy.sh      # docker run envoyproxy/envoy:<固定版本> --network host
  smoke.sh          # curl 兩條路，印出狀態碼與 body
README.md           # 每節對應筆記哪一段
CLAUDE.md           # /init 本來要求的檔案：建置/執行指令 + 架構
```

## proto 設計（`quotes.proto`，package `quotes.v1`，service `Quotes`）

| RPC | 註記 | 示範筆記哪一點 |
|---|---|---|
| `GetQuote` | `get: "/v1/quotes/{id}"` | 路徑參數綁定 |
| `CreateQuote` | `post: "/v1/quotes" body: "*"` | JSON body |
| `ListQuotes`（server streaming） | `get: "/v1/quotes"` | 串流：Envoy 與 Gateway 的輸出差異（Gateway 換行分隔） |
| `GetChart` | `get: "/v1/quotes/{id}/chart"`，回 `google.api.HttpBody` | 唯一需要 C++ 設 `content_type`/`data` 的例外 |

`GetQuote` 對不存在的 id 回 `NOT_FOUND`，用來看兩條路的 HTTP 狀態碼映射
（Envoy 固定 `grpcToHttpStatus`；Gateway 預設映射）。

## 各元件

- **C++ server**：記憶體內 map 存 quotes，監聽 `0.0.0.0:50051`（HTTP/2 明文）。
  CMake 先試 `find_package(gRPC CONFIG)`，Debian 套件若沒附 cmake config 則退回
  `pkg_check_modules(grpc++ protobuf)`；用 `add_custom_command` 呼叫 `protoc` +
  `grpc_cpp_plugin` 生 stubs，include `third_party/googleapis` 並一起編 `google/api/*.pb.cc`。
- **Envoy**：listener `:8080` → cluster `grpc_backend`（`127.0.0.1:50051`，
  `http2_protocol_options`）。filter 設 `proto_descriptor: /etc/envoy/proto.pb`、
  `services: [quotes.v1.Quotes]`、`convert_grpc_status: true`、
  `request_validation_options`（`reject_unknown_method`、`reject_unknown_query_parameters`）。
  映像版本實作時固定一個具體 tag，並對照該版文件核對欄位名（筆記提醒欄位跨版本）。
- **gRPC-Gateway**：`go install` protoc-gen-go / protoc-gen-go-grpc / protoc-gen-grpc-gateway（v2），
  `main.go` 用 `runtime.NewServeMux` + `RegisterQuotesHandlerFromEndpoint` 連 `localhost:50051`，
  監聽 `:8081`。
- **smoke.sh**：對 `:8080`（Envoy）與 `:8081`（Gateway）各跑同一組請求：
  GET 存在/不存在的 id、POST 建立、串流列表、chart（非 JSON）、未知路徑（Envoy 預期 404）、
  未知 query 參數（Envoy 預期 400）。

## 執行順序

1. `git init`；寫 vendored googleapis protos（從 github.com/googleapis/googleapis 取 3 檔）。
2. 寫 proto、C++ server、CMake、scripts、envoy.yaml、gateway、README、CLAUDE.md。
3. 請使用者跑 `bash scripts/install-deps.sh`（需要 sudo 密碼，我無法代跑）。
4. 建置與驗證（見下）。

## 驗證

```bash
bash scripts/gen.sh
cmake -S server -B build -G Ninja && cmake --build build
./build/quotes_server &               # :50051
bash scripts/run-envoy.sh &           # :8080
(cd gateway && go run .) &            # :8081
bash scripts/smoke.sh                 # 比對兩條路的狀態碼與 body
```

成功標準：兩條路的 GET/POST/串流/chart 都回正確內容；`NOT_FOUND` 映成 404；
Envoy 對未知路徑回 404、對未知 query 參數回 400。實際輸出寫進 README 的「實測結果」一節。

## 後續（完成後詢問，不自動做）

筆記「Not tested」列了「gRPC-Gateway with a C++ backend」與串流行為；範例跑通後
可回填筆記——那是改 org-roam vault，會先問。
