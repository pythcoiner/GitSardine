#!/bin/bash
# GitSardine dependency installer
# Installs nix, pulls qontrol and qt_static dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== GitSardine Installer ==="

# Setup nix store on /home for Qubes OS (root filesystem is limited)
# This must be done before nix installation
if [ ! -d /nix ] || [ ! -d /nix/store ]; then
    echo ""
    echo "Setting up nix store on /home (for Qubes OS compatibility)..."
    sudo mkdir -p /nix /home/nix
    if ! mountpoint -q /nix 2>/dev/null; then
        sudo mount --bind /home/nix /nix
        echo "Note: Add to /etc/fstab for persistence: /home/nix /nix none bind 0 0"
    fi
fi

# Source nix profile if available
if [ -f /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh ]; then
    . /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh
fi

# Install nix if not present
if ! command -v nix &> /dev/null; then
    echo ""
    echo "Installing Nix..."
    curl -L https://nixos.org/nix/install | sh -s -- --daemon --yes
    echo ""
    echo "Nix installed. Please restart your shell and run this script again."
    exit 0
fi

# Check if flakes are enabled (only add if not already present)
if ! nix flake --help &> /dev/null 2>&1; then
    echo ""
    echo "Enabling Nix flakes..."
    mkdir -p ~/.config/nix
    if ! grep -q "experimental-features.*flakes" ~/.config/nix/nix.conf 2>/dev/null; then
        echo "experimental-features = nix-command flakes" >> ~/.config/nix/nix.conf
        echo "Flakes enabled. You may need to restart the nix-daemon."
    fi
fi

mkdir -p "$SCRIPT_DIR/deps"

# Pull qontrol dependency
echo ""
echo "Pulling qontrol dependency..."
if [ -d "$SCRIPT_DIR/deps/qontrol/.git" ]; then
    echo "qontrol already exists, updating..."
    (cd "$SCRIPT_DIR/deps/qontrol" && git pull)
else
    echo "Cloning qontrol..."
    git clone https://github.com/pythcoiner/qontrol.git "$SCRIPT_DIR/deps/qontrol"
fi

# Pull qt_static for static Qt6 build
echo ""
echo "Pulling qt_static for static Qt6..."
if [ -d "$SCRIPT_DIR/deps/qt_static/.git" ]; then
    echo "qt_static already exists, updating..."
    (cd "$SCRIPT_DIR/deps/qt_static" && git pull)
else
    echo "Cloning qt_static..."
    git clone https://github.com/pythcoiner/qt_static.git "$SCRIPT_DIR/deps/qt_static"
fi

# Build static Qt6 for Linux
if [ ! -d "$SCRIPT_DIR/deps/qt_static/dist/linux" ]; then
    echo ""
    echo "Building static Qt6 for Linux (this may take 20-30 minutes)..."
    (cd "$SCRIPT_DIR/deps/qt_static" && ./build.sh linux)
else
    echo ""
    echo "Static Qt6 already built."
fi

echo ""
echo "=== Installation complete! ==="
echo "Run 'just build' to compile GitSardine"
