workspace( name = "rpcing" )

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load(":lizard.bzl", "bask", "grub")

PROTOBUF_VERSION="3.6.1.3"
PROTOBUF_SHA256="73fdad358857e120fd0fa19e071a96e15c0f23bb25f85d3f7009abfd4f264a2a"
http_archive(
    name = "com_google_protobuf",
    urls = [ "https://github.com/protocolbuffers/protobuf/archive/v" + PROTOBUF_VERSION + ".tar.gz" ],
    strip_prefix = "protobuf-" + PROTOBUF_VERSION,
    sha256 = PROTOBUF_SHA256
)

PROTOBUF_JAVALITE_VERSION="384989534b2246d413dbcd750744faab2607b516"
PROTOBUF_JAVALITE_SHA256="79d102c61e2a479a0b7e5fc167bcfaa4832a0c6aad4a75fa7da0480564931bcc"
http_archive(
    name = "com_google_protobuf_javalite",
    strip_prefix = "protobuf-" + PROTOBUF_JAVALITE_VERSION,
    urls = ["https://github.com/protocolbuffers/protobuf/archive/" + PROTOBUF_JAVALITE_VERSION + ".zip"],
    sha256 = PROTOBUF_JAVALITE_SHA256
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

#RULES_GO_VERSION="0.17.0"
#RULES_GO_SHA256="492c3ac68ed9dcf527a07e6a1b2dcbf199c6bf8b35517951467ac32e421c06c1"
RULES_GO_VERSION="0.15.11"
RULES_GO_SHA256="7b7c74740e3a757204ddb93241ce728906af795d6c6aa0950e0e640716dc1e4a"
http_archive(
    name = "io_bazel_rules_go",
    urls = [ "https://github.com/bazelbuild/rules_go/releases/download/" + RULES_GO_VERSION + "/rules_go-" + RULES_GO_VERSION + ".tar.gz" ],
    sha256 = RULES_GO_SHA256,
)
#load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies", "go_register_toolchains")
load("@io_bazel_rules_go//go:def.bzl", "go_rules_dependencies", "go_register_toolchains")
go_rules_dependencies()
go_register_toolchains()

GRPC_JAVA_VERSION=GRPC_VERSION
GRPC_JAVA_SHA256="0b86e44f9530fd61eb044b3c64c7579f21857ba96bcd9434046fd22891483a6d"
http_archive(
    name = "io_grpc_grpc_java",
    strip_prefix = "grpc-java-" + GRPC_JAVA_VERSION,
    urls = [ "https://github.com/grpc/grpc-java/archive/v" + GRPC_JAVA_VERSION + ".tar.gz" ],
    sha256 = GRPC_JAVA_SHA256,
)
load("@io_grpc_grpc_java//:repositories.bzl", "grpc_java_repositories")
grpc_java_repositories(
    omit_com_google_errorprone_error_prone_annotations = True,
    omit_com_google_code_findbugs_jsr305 = True
)

# From https://github.com/bazelbuild/rules_closure/releases
RULES_CLOSURE_VERSION="0.8.0"
RULES_CLOSURE_SHA256="b29a8bc2cb10513c864cb1084d6f38613ef14a143797cea0af0f91cd385f5e8c"
http_archive(
    name = "io_bazel_rules_closure",
    strip_prefix = "rules_closure-" + RULES_CLOSURE_VERSION,
    urls = [ "https://github.com/bazelbuild/rules_closure/archive/" + RULES_CLOSURE_VERSION + ".tar.gz" ],
    sha256 = RULES_CLOSURE_SHA256,
)
load("@io_bazel_rules_closure//closure:defs.bzl", "closure_repositories")
closure_repositories(
)

# rules_typescript

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
    name = "build_bazel_rules_nodejs",
    urls = ["https://github.com/bazelbuild/rules_nodejs/releases/download/0.18.6/rules_nodejs-0.18.6.tar.gz"],
    sha256 = "1416d03823fed624b49a0abbd9979f7c63bbedfd37890ddecedd2fe25cccebc6",
)

# Setup the NodeJS toolchain
load("@build_bazel_rules_nodejs//:defs.bzl", "node_repositories", "yarn_install", "npm_install")
node_repositories(
    package_json = ["//:package.json"],
    node_version = "10.13.0",
    yarn_version = "1.12.1",
)

# Setup Bazel managed npm dependencies with the `yarn_install` rule.
# The name of this rule should be set to `npm` so that `ts_library`
# can find your npm dependencies by default in the `@npm` workspace. You may
# also use the `npm_install` rule with a `package-lock.json` file if you prefer.
# See https://github.com/bazelbuild/rules_nodejs#dependencies for more info.
yarn_install(
  name = "npm",
  package_json = "//:package.json",
  yarn_lock = "//:yarn.lock",
)

#npm_install(
#    name = "npm",
#    package_json = "//:package.json",
#    package_lock_json = "//:package-lock.json",
#)

# Install all Bazel dependencies needed for npm packages that supply Bazel rules
#load("@npm//:install_bazel_dependencies.bzl", "install_bazel_dependencies")
#install_bazel_dependencies()

# Setup TypeScript toolchain
#load("@npm_bazel_typescript//:defs.bzl", "ts_setup_workspace")
#ts_setup_workspace()

http_archive(
    name = "io_bazel_rules_rust",
    sha256 = "c82118824b2448b77146f1dae97b6eaa717babedad0822aca4879f3cbbf2b7b5",
    strip_prefix = "rules_rust-3228ccd3814c2ad0d7307d2f87fb8ff9616149d7",
    urls = [
        # Master branch as of 2018-12-11
        "https://github.com/bazelbuild/rules_rust/archive/3228ccd3814c2ad0d7307d2f87fb8ff9616149d7.tar.gz",
    ],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "eb5c57e4c12e68c0c20bc774bfbc60a568e800d025557bc4ea022c6479acc867",
    strip_prefix = "bazel-skylib-0.6.0",
    url = "https://github.com/bazelbuild/bazel-skylib/archive/0.6.0.tar.gz",
)

load("@io_bazel_rules_rust//rust:repositories.bzl", "rust_repositories")
rust_repositories()

load("@io_bazel_rules_rust//:workspace.bzl", "bazel_version")
bazel_version(name = "bazel_version")

load("@io_bazel_rules_rust//proto:repositories.bzl", "rust_proto_repositories")
rust_proto_repositories()
