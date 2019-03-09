workspace(name = "rpcing")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load(":lizard.bzl", "bask", "grub")

GFLAGS_VERSION = "2.2.2"

GFLAGS_SHA256 = "19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5"

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = GFLAGS_SHA256,
    strip_prefix = "gflags-" + GFLAGS_VERSION,
    urls = ["https://github.com/gflags/gflags/archive/v" + GFLAGS_VERSION + ".zip"],
)

PROTOBUF_VERSION = "3.6.1.3"

PROTOBUF_SHA256 = "73fdad358857e120fd0fa19e071a96e15c0f23bb25f85d3f7009abfd4f264a2a"

http_archive(
    name = "com_google_protobuf",
    sha256 = PROTOBUF_SHA256,
    strip_prefix = "protobuf-" + PROTOBUF_VERSION,
    urls = ["https://github.com/protocolbuffers/protobuf/archive/v" + PROTOBUF_VERSION + ".tar.gz"],
)

PROTOBUF_JAVALITE_VERSION = "384989534b2246d413dbcd750744faab2607b516"

PROTOBUF_JAVALITE_SHA256 = "79d102c61e2a479a0b7e5fc167bcfaa4832a0c6aad4a75fa7da0480564931bcc"

http_archive(
    name = "com_google_protobuf_javalite",
    sha256 = PROTOBUF_JAVALITE_SHA256,
    strip_prefix = "protobuf-" + PROTOBUF_JAVALITE_VERSION,
    urls = ["https://github.com/protocolbuffers/protobuf/archive/" + PROTOBUF_JAVALITE_VERSION + ".zip"],
)

GRPC_VERSION = "1.18.0"

GRPC_SHA256 = "069a52a166382dd7b99bf8e7e805f6af40d797cfcee5f80e530ca3fc75fd06e2"

http_archive(
    name = "com_github_grpc_grpc",
    sha256 = GRPC_SHA256,
    strip_prefix = "grpc-" + GRPC_VERSION,
    urls = ["https://github.com/grpc/grpc/archive/v" + GRPC_VERSION + ".tar.gz"],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps", "grpc_test_only_deps")

grpc_deps()

grpc_test_only_deps()

#RULES_GO_VERSION="0.17.0"
#RULES_GO_SHA256="492c3ac68ed9dcf527a07e6a1b2dcbf199c6bf8b35517951467ac32e421c06c1"
RULES_GO_VERSION = "0.15.11"

RULES_GO_SHA256 = "7b7c74740e3a757204ddb93241ce728906af795d6c6aa0950e0e640716dc1e4a"

http_archive(
    name = "io_bazel_rules_go",
    sha256 = RULES_GO_SHA256,
    urls = ["https://github.com/bazelbuild/rules_go/releases/download/" + RULES_GO_VERSION + "/rules_go-" + RULES_GO_VERSION + ".tar.gz"],
)

#load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies", "go_register_toolchains")
load("@io_bazel_rules_go//go:def.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains()

GRPC_JAVA_VERSION = GRPC_VERSION
GRPC_JAVA_SHA256 = "0b86e44f9530fd61eb044b3c64c7579f21857ba96bcd9434046fd22891483a6d"

http_archive(
    name = "io_grpc_grpc_java",
    sha256 = GRPC_JAVA_SHA256,
    strip_prefix = "grpc-java-" + GRPC_JAVA_VERSION,
    urls = ["https://github.com/grpc/grpc-java/archive/v" + GRPC_JAVA_VERSION + ".tar.gz"],
)

load("@io_grpc_grpc_java//:repositories.bzl", "grpc_java_repositories")

grpc_java_repositories(
    omit_com_google_code_findbugs_jsr305 = True,
    omit_com_google_errorprone_error_prone_annotations = True,
)

# From https://github.com/bazelbuild/rules_closure/releases
#RULES_CLOSURE_VERSION="0.8.0"
#RULES_CLOSURE_SHA256="b29a8bc2cb10513c864cb1084d6f38613ef14a143797cea0af0f91cd385f5e8c"
RULES_CLOSURE_VERSION = "87d24b1df8b62405de8dd059cb604fd9d4b1e395"

RULES_CLOSURE_SHA256 = "0e6de40666f2ebb2b30dc0339745a274d9999334a249b05a3b1f46462e489adf"

http_archive(
    name = "io_bazel_rules_closure",
    sha256 = RULES_CLOSURE_SHA256,
    strip_prefix = "rules_closure-" + RULES_CLOSURE_VERSION,
    urls = ["https://github.com/bazelbuild/rules_closure/archive/" + RULES_CLOSURE_VERSION + ".tar.gz"],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_repositories")

closure_repositories(
)

# rules_typescript

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "build_bazel_rules_nodejs",
    sha256 = "1416d03823fed624b49a0abbd9979f7c63bbedfd37890ddecedd2fe25cccebc6",
    urls = ["https://github.com/bazelbuild/rules_nodejs/releases/download/0.18.6/rules_nodejs-0.18.6.tar.gz"],
)

# Setup the NodeJS toolchain
load("@build_bazel_rules_nodejs//:defs.bzl", "node_repositories", "npm_install", "yarn_install")

node_repositories(
    node_version = "10.13.0",
    package_json = ["//:package.json"],
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

RULES_RUST_VERSION="5fa9b101a68ddd4a628462b8d2aae06c6cbbda15"
RULES_RUST_SHA256="2d16f6da563e4d926d3ef0753c8eef14d90e7dd43d28f956d587872a39af244c"

http_archive(
    name = "io_bazel_rules_rust",
    sha256 = RULES_RUST_SHA256,
    strip_prefix = "rules_rust-" + RULES_RUST_VERSION,
    urls = [ "https://github.com/bazelbuild/rules_rust/archive/" + RULES_RUST_VERSION + ".tar.gz" ],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "eb5c57e4c12e68c0c20bc774bfbc60a568e800d025557bc4ea022c6479acc867",
    strip_prefix = "bazel-skylib-0.6.0",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/0.6.0.tar.gz"],
)

load("@io_bazel_rules_rust//rust:repositories.bzl", "rust_repositories")

rust_repositories()

load("@io_bazel_rules_rust//:workspace.bzl", "bazel_version")

bazel_version(name = "bazel_version")

load("@io_bazel_rules_rust//proto:repositories.bzl", "rust_proto_repositories")

rust_proto_repositories()

#############################

# Download the rules_docker repository at release v0.7.0
http_archive(
    name = "io_bazel_rules_docker",
    sha256 = "aed1c249d4ec8f703edddf35cbe9dfaca0b5f5ea6e4cd9e83e99f3b0d1136c3d",
    strip_prefix = "rules_docker-0.7.0",
    urls = ["https://github.com/bazelbuild/rules_docker/archive/v0.7.0.tar.gz"],
)

# OPTIONAL: Call this to override the default docker toolchain configuration.
# This call should be placed BEFORE the call to "container_repositories" below
# to actually override the default toolchain configuration.
# Note this is only required if you actually want to call
# docker_toolchain_configure with a custom attr; please read the toolchains
# docs in /toolchains/docker/ before blindly adding this to your WORKSPACE.

load(
    "@io_bazel_rules_docker//toolchains/docker:toolchain.bzl",
    docker_toolchain_configure = "toolchain_configure",
)

docker_toolchain_configure(
    name = "docker_config",
    # OPTIONAL: Path to a directory which has a custom docker client config.json.
    # See https://docs.docker.com/engine/reference/commandline/cli/#configuration-files
    # for more details.
    client_config = "/path/to/docker/client/config",
)

# This is NOT needed when going through the language lang_image
# "repositories" function(s).
load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)

container_repositories()

load("@io_bazel_rules_docker//container:container.bzl", "container_pull")

container_pull(
    name = "java_base",
    # 'tag' is also supported, but digest is encouraged for reproducibility.
    digest = "sha256:deadbeef",
    registry = "gcr.io",
    repository = "distroless/java",
)

load("@io_bazel_rules_docker//go:image.bzl", _go_image_repos = "repositories")

_go_image_repos()

load("@io_bazel_rules_docker//cc:image.bzl", _cc_image_repos = "repositories")

_cc_image_repos()

#bask()
