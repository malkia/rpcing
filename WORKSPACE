workspace( name = "rpcing" )

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive") 

#PROTOBUF_VERSION="3.6.1"
#http_archive(
#    name = "com_google_protobuf",
#    strip_prefix = "protobuf-" + PROTOBUF_VERSION,
#    urls = ["https://github.com/protocolbuffers/protobuf/archive/v" + PROTOBUF_VERSION + ".tar.gz"],
#)

GRPC_VERSION="1.18.0"
GRPC_SHA256="069a52a166382dd7b99bf8e7e805f6af40d797cfcee5f80e530ca3fc75fd06e2"

http_archive(
    name = "com_github_grpc_grpc",
    urls = [
        "https://github.com/grpc/grpc/archive/v" + GRPC_VERSION + ".tar.gz",
    ],
    strip_prefix = "grpc-" + GRPC_VERSION,
    sha256 = GRPC_SHA256,
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")


grpc_deps()
