{
  description = "GitSardine - Git repository manager";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    qt_static = {
      url = "path:./deps/qt_static";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, qt_static }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      # Get Qt from qt_static (must be built first with ./deps/qt_static/build.sh linux)
      qt6Static = qt_static.packages.${system}.linux;

    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "gitsardine-dev";

        buildInputs = [
          qt6Static
          pkgs.cmake
          pkgs.ninja
          pkgs.pkg-config
          pkgs.git
          pkgs.libgit2
        ];

        shellHook = ''
          echo "GitSardine Development Shell"
          echo "============================="
          echo "Static Qt6: ${qt6Static}"
          echo ""
          export QT_DIR="${qt6Static}"
          export CMAKE_PREFIX_PATH="${qt6Static}"
        '';
      };

      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "gitsardine";
        version = "1.0.0";

        src = ./.;

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
          pkgs.pkg-config
        ];

        buildInputs = [
          qt6Static
          pkgs.libgit2
        ];

        cmakeFlags = [
          "-DCMAKE_PREFIX_PATH=${qt6Static}"
        ];
      };
    };
}
