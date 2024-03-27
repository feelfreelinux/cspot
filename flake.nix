{
  description = "cspot - A Spotify Connect player written in CPP targeting, but not limited to embedded devices (ESP32).";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-23.11";
    nixpkgs-unstable.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    nixpkgs-unstable,
    flake-utils,
  }: let
    overlay = final: prev: {
      unstable = nixpkgs-unstable.legacyPackages.${prev.system};
    };
  in
    {
      overlays.default = overlay;
    }
    // flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
        overlays = [overlay];
      };

      llvm = pkgs.llvmPackages_14;

      clang-tools = pkgs.clang-tools.override {llvmPackages = llvm;};

      deps = with pkgs; [
        avahi
        pipewire
        avahi-compat
        cmake
        ninja
        python3
        python3Packages.protobuf
        python3Packages.setuptools
        unstable.mbedtls
        portaudio
        alsa-lib
        protobuf
      ];

      apps = {
      };

      packages = {
        target-cli = llvm.stdenv.mkDerivation {
          name = "cspotcli";
          src = ./.;
          cmakeFlags = ["-DCSPOT_TARGET_CLI=ON"];
          nativeBuildInputs = deps;
          # Patch nanopb shebangs to refer to provided python
          postPatch = ''
            patchShebangs cspot/bell/external/nanopb/generator/*
          '';
          enableParallelBuilding = true;
        };
      };

      devShells = {
        default = pkgs.mkShell {
          packages = deps;
        };
      };
    in {
      inherit apps devShells packages;
      checks = packages;
      devShell = devShells.default;
    });
}
