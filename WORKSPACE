workspace( name = "rpcing" )

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

PROTOBUF_VERSION="3.6.1.3"
PROTOBUF_SHA256="73fdad358857e120fd0fa19e071a96e15c0f23bb25f85d3f7009abfd4f264a2a"
http_archive(
    name = "com_google_protobuf",
    urls = [ "https://github.com/protocolbuffers/protobuf/archive/v" + PROTOBUF_VERSION + ".tar.gz" ],
    strip_prefix = "protobuf-" + PROTOBUF_VERSION,
    sha256 = PROTOBUF_SHA256
)

GRPC_VERSION="1.18.0"
GRPC_SHA256="069a52a166382dd7b99bf8e7e805f6af40d797cfcee5f80e530ca3fc75fd06e2"
http_archive(
    name = "com_github_grpc_grpc",
    urls = [ "https://github.com/grpc/grpc/archive/v" + GRPC_VERSION + ".tar.gz" ],
    strip_prefix = "grpc-" + GRPC_VERSION,
    sha256 = GRPC_SHA256,
)
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps", "grpc_test_only_deps")
grpc_deps()
grpc_test_only_deps()

RULES_GO_VERSION="0.17.0"
RULES_GO_SHA256="492c3ac68ed9dcf527a07e6a1b2dcbf199c6bf8b35517951467ac32e421c06c1"
http_archive(
    name = "io_bazel_rules_go",
    urls = [ "https://github.com/bazelbuild/rules_go/releases/download/" + RULES_GO_VERSION + "/rules_go-" + RULES_GO_VERSION + ".tar.gz" ],
    sha256 = RULES_GO_SHA256,
)
load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies", "go_register_toolchains")
go_rules_dependencies()
go_register_toolchains()
