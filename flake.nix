{
  description = "GitSardine - Git repository manager";

  inputs = {
    # Use nixos-22.11 for glibc 2.35 (Ubuntu 22.04+ compatibility)
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in {
      devShells.${system}.default = pkgs.mkShell {
        name = "gitsardine-dev";

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
          pkgs.pkg-config
        ];

        buildInputs = [
          # Qt static dependencies (needed at link time)
          pkgs.zlib
          pkgs.libb2
          pkgs.pcre2
          pkgs.double-conversion
          pkgs.openssl
          pkgs.libpng
          pkgs.libjpeg
          pkgs.sqlite
          pkgs.glib
          pkgs.fontconfig
          pkgs.freetype
          pkgs.harfbuzz
          pkgs.dbus
          pkgs.at-spi2-core
          pkgs.libinput
          pkgs.mtdev
          pkgs.systemd

          # X11
          pkgs.xorg.libX11
          pkgs.xorg.libXext
          pkgs.xorg.libXrender
          pkgs.xorg.libXi
          pkgs.xorg.libXcursor
          pkgs.xorg.libXrandr
          pkgs.xorg.libXinerama
          pkgs.xorg.libXfixes
          pkgs.xorg.libXcomposite
          pkgs.xorg.libXdamage
          pkgs.xorg.libxcb
          pkgs.xorg.xcbutil
          pkgs.xorg.xcbutilwm
          pkgs.xorg.xcbutilimage
          pkgs.xorg.xcbutilkeysyms
          pkgs.xorg.xcbutilrenderutil
          pkgs.xorg.xcbutilcursor
          pkgs.xorg.libSM
          pkgs.xorg.libICE
          pkgs.xorg.libxshmfence
          pkgs.libxkbcommon

          # Wayland
          pkgs.wayland
          pkgs.wayland-protocols

          # Graphics
          pkgs.mesa
          pkgs.libGL
          pkgs.vulkan-headers
          pkgs.vulkan-loader
          pkgs.libdrm

          # Project dependencies
          pkgs.libgit2
        ];

        shellHook = ''
          echo "GitSardine Development Shell (glibc 2.35)"
          echo "=========================================="
        '';
      };
    };
}
