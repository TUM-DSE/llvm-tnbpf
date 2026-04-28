{
  description = "Flake for LLVM development.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/25.11";
    unstable.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

    utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      unstable,
      ...
    }@inputs:
    inputs.utils.lib.eachSystem
      [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ]
      (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
          unstablePkgs = import unstable {
            inherit system;
          };
        in
        {
          devShell = pkgs.mkShell rec {
            name = "llvm dev env";
            hardeningDisable = [
              "libcxxhardeningfast"
              "strictflexarrays1"
              "strictflexarrays3"
            ];

            packages = with pkgs; [
              # Development Tools
              ccache
              gdb
              cmake
              ninja
              graphviz
              llvmPackages_latest.lldb
              pkg-config
              unstablePkgs.mold
              clang
              zlib
              just
              linuxHeaders
              libbpf
            ];

            NIX_LDFLAGS = "-rpath ${pkgs.zlib}/lib";
            LINUX_HEADERS = "${pkgs.linuxHeaders}";
            LIBBPF = "${pkgs.libbpf}";
          };
        }
      );
}
