{
  description = "wqimage: C++23 image processing library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    wqpng = {
      url = "github:weqeqq/wqpng";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    wqcolor = {
      url = "github:weqeqq/wqcolor";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    wqfile = {
      url = "github:weqeqq/wqfile";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    wqparallel = {
      url = "github:weqeqq/wqparallel";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {
    nixpkgs,
    wqpng,
    wqcolor,
    wqfile,
    wqparallel,
    ...
  }: let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    forAllSystems = f: nixpkgs.lib.genAttrs systems f;

    perSystem = system: let
      pkgs = import nixpkgs {inherit system;};
      lib = pkgs.lib;
      gtestPkg =
        if pkgs ? gtest
        then pkgs.gtest
        else pkgs.googletest;

      src = lib.fileset.toSource {
        root = ./.;
        fileset = lib.fileset.unions [
          ./headers
          ./meson.build
          ./meson.options
          ./sources
          ./subprojects/gtest.wrap
          ./subprojects/highway.wrap
          ./subprojects/wqcolor.wrap
          ./subprojects/wqfile.wrap
          ./subprojects/wqparallel.wrap
          ./subprojects/wqpng.wrap
          ./tests
        ];
      };

      wqpngPkg = wqpng.packages.${system}.default;
      wqcolorPkg = wqcolor.packages.${system}.default;
      wqfilePkg = wqfile.packages.${system}.default;
      wqparallelPkg = wqparallel.packages.${system}.default;

      mkWqimage = {tests ? false}:
        pkgs.stdenv.mkDerivation {
          pname = "wqimage";
          version = "0.2.0";
          inherit src;

          strictDeps = true;

          nativeBuildInputs = with pkgs; [
            meson
            ninja
            pkg-config
          ];

          buildInputs = [
            wqpngPkg
            wqcolorPkg
            wqfilePkg
            wqparallelPkg
            pkgs.libhwy
            gtestPkg
          ];

          doCheck = tests;
          checkPhase = lib.optionalString tests ''
            meson test --print-errorlogs
          '';

          meta = with lib; {
            description = "C++23 image processing library built on top of wqpng and wqcolor";
            platforms = platforms.unix;
          };
        };

      package = mkWqimage {};
      checkPackage = mkWqimage {tests = true;};
    in {
      inherit pkgs package checkPackage;
    };
  in {
    packages = forAllSystems (
      system: let
        inherit (perSystem system) package;
      in {
        default = package;
        wqimage = package;
      }
    );

    checks = forAllSystems (
      system: let
        inherit (perSystem system) checkPackage;
      in {
        default = checkPackage;
        wqimage-tests = checkPackage;
      }
    );

    devShells = forAllSystems (
      system: let
        inherit (perSystem system) checkPackage pkgs;
      in {
        default = pkgs.mkShell {
          inputsFrom = [checkPackage];

          packages = with pkgs; [
            clang-tools
            cmake
            nixpkgs-fmt
          ];
        };
      }
    );

    formatter = forAllSystems (system: (perSystem system).pkgs.nixpkgs-fmt);
  };
}
