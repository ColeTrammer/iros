{
  description = "An Operating System focused on asynchronicity, minimalism, and performance.";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-root.url = "github:srid/flake-root";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
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
        ...
      }: {
        treefmt = {
          inherit (config.flake-root) projectRootFile;

          programs = {
            alejandra.enable = true;
            clang-format.enable = true;
            prettier.enable = true;
            shfmt = {
              enable = true;
              indent_size = 4;
            };
          };

          settings.formatter.cmake-format = {
            command = "${pkgs.cmake-format}/bin/cmake-format";
            options = ["-i"];
            includes = ["CMakeLists.txt" "CMakeToolchain.*.txt" "*.cmake"];
          };
        };

        devShells.default = pkgs.mkShell {
          packages =
            [
              config.treefmt.build.wrapper
              pkgs.cmake-format
            ]
            ++ builtins.attrValues config.treefmt.build.programs
            ++ [
              pkgs.nil
              pkgs.clang-tools
              pkgs.neocmakelsp
              pkgs.marksman
              pkgs.markdownlint-cli
              pkgs.dockerfile-language-server-nodejs
              pkgs.yaml-language-server
              pkgs.hadolint
              (pkgs.writeShellScriptBin
                "vscode-json-language-server"
                ''${pkgs.nodePackages_latest.vscode-json-languageserver}/bin/vscode-json-languageserver "$@"'')
            ]
            ++ [
              pkgs.clang
              pkgs.cmake
              pkgs.ninja
              pkgs.doxygen
              pkgs.graphviz
              pkgs.ccache
              pkgs.gcovr
              pkgs.openssl
              pkgs.mpfr
              pkgs.gmp
              pkgs.libmpc
              pkgs.qemu
              pkgs.gcc12
              pkgs.nodejs
              pkgs.grub2
              pkgs.xorriso
              pkgs.parted
            ];

            hardeningDisable = [ "format" "fortify" ];
        };
      };
    };
}
