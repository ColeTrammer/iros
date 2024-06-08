{
  description = "An Operating System focused on asynchronicity, minimalism, and performance.";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-root.url = "github:srid/flake-root";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      imports = [
        inputs.treefmt-nix.flakeModule
        inputs.flake-root.flakeModule
      ];
      systems = ["x86_64-linux"];
      perSystem = {
        config,
        pkgs,
        system,
        ...
      }: {
        treefmt = {
          inherit (config.flake-root) projectRootFile;

          programs = {
            alejandra.enable = true;
            clang-format = {
              enable = true;
              package = pkgs.clang-tools_18;
            };
            prettier.enable = true;
            shfmt = {
              enable = true;
              indent_size = 4;
            };
            just.enable = true;
          };

          settings.formatter.cmake-format = {
            command = "${pkgs.cmake-format}/bin/cmake-format";
            options = ["-i"];
            includes = ["CMakeLists.txt" "CMakeToolchain.*.txt" "*.cmake"];
          };
        };

        devShells.default = let
          gccVersion = "14";
          llvmVersion = "18";
        in
          pkgs.mkShell.override {stdenv = pkgs."gcc${gccVersion}Stdenv";} {
            packages =
              [
                config.treefmt.build.wrapper
                pkgs.cmake-format
              ]
              ++ builtins.attrValues config.treefmt.build.programs
              ++ [
                # Clang
                pkgs."clang-tools_${llvmVersion}"
                pkgs."clang_${llvmVersion}"

                # Debug
                pkgs.gdb
                pkgs.valgrind
                pkgs."lldb_${llvmVersion}"

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
            hardeningDisable = ["format"];
          };
      };
    };
}
