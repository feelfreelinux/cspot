{ nixpkgs ? import <nixpkgs> { }, lib ? import <nixpkgs/lib> }:

let pkgs = with nixpkgs; [ cmake portaudio protobuf go openssl avahi-compat ];

in nixpkgs.stdenv.mkDerivation {
  name = "cspotcli";
  src = lib.cleanSourceWith {
    filter = lib.cleanSourceFilter;
    src = lib.cleanSourceWith {
      filter = name: type:
        let baseName = baseNameOf (toString name);
        in !(
          # Filter out version control software files/directories
          baseName == ".github" || baseName == ".vscode" || baseName == "docker"
          || baseName == "README.md" || baseName == ".editorconfig"
          || lib.strings.hasInfix "targets/cli/build" name

        );

      src = ./.;
    };
  };
  buildInputs = pkgs;
  configurePhase = ''
    ls -lah
    mkdir -p targets/cli/build
    cd targets/cli/build
    cmake ..
  '';
  buildPhase = ''
    make -j`nproc` # build with multiple threads, so that all CPU cores are working
  '';
  installPhase = ''
    mkdir -p $out/bin
    mv cspotcli $out/bin
  '';
}
