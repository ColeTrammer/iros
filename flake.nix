{
  description = "An Operating System focused on asynchronicity, minimalism, and performance.";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-root.url = "github:srid/flake-root";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.treefmt-nix.flakeModule
        inputs.flake-root.flakeModule
      ];
      systems = [ "x86_64-linux" ];
      perSystem =
        { config, pkgs, ... }:
        let
          gccVersion = "14";
          llvmVersion = "18";

          clang = pkgs."llvmPackages_${llvmVersion}".libcxxClang;
          clangTools = pkgs."clang-tools_${llvmVersion}".override { enableLibcxx = true; };

          lldb = pkgs."lldb_${llvmVersion}";

          stdenv = pkgs."gcc${gccVersion}Stdenv";
        in
        {
          treefmt = {
            inherit (config.flake-root) projectRootFile;

            programs = {
              clang-format = {
                enable = true;
                package = clangTools;
              };
              nixfmt.enable = true;
              prettier.enable = true;
              shfmt = {
                enable = true;
                indent_size = 4;
              };
              just.enable = true;
              cmake-format.enable = true;
            };

            settings.formatter.cmake-format.includes = [
              "**/CMakeLists.txt"
              "**/CMakeToolchain_*.txt"
              "*.cmake"
            ];

            settings.formatter.prettier.includes = [
              "**/.clang-tidy"
              "**/snippets.code-snippets"
              ".clang-format"
              ".clang-tidy"
              ".clangd"
              ".prettierrc"
              "flake.lock"
            ];

            settings.excludes = [
              "**/*.dockerignore"
              "**/Dockerfile"
              "**/limine.cfg"
              "*.ld"
              "*.patch"
              "*.png"
              "*.svg"
              "*.txt"
              "*.xml"
              ".editorconfig"
              ".git-blame-ignore-revs"
              ".github/CODEOWNERS"
              ".gitignore"
              ".prettierignore"
              "LICENSE"
            ];
          };

          devShells.default = pkgs.mkShell.override { inherit stdenv; } {
            packages =
              [
                config.treefmt.build.wrapper
                pkgs.cmake-format
              ]
              ++ builtins.attrValues config.treefmt.build.programs
              ++ [
                # Clang
                clangTools
                clang

                # Debug
                pkgs.gdb
                pkgs.valgrind
                lldb

                # justfile build commands
                pkgs.fzf
                pkgs.jq
                pkgs.just

                # Linux build
                pkgs.cmake
                pkgs.ninja
                pkgs.ccache

                # Docs
                pkgs.doxygen
                pkgs.graphviz

                # Cross compiler deps
                pkgs.bison
                pkgs.flex
                pkgs.mpfr
                pkgs.gmp
                pkgs.libmpc
                pkgs.autoconf269
                pkgs.automake115x

                # Build and run Iros images
                pkgs.qemu
                pkgs.parted

                # Coverage
                pkgs.gcovr

                # Linux deps
                pkgs.pipewire
                pkgs.wayland-scanner
                pkgs.wayland
              ];

            # Needed to build the cross compilr
            hardeningDisable = [ "format" ];
          };
        };
    };
}
