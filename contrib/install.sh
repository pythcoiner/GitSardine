#!/bin/bash
# GitSardine dependency installer
# Installs nix, pulls qontrol and qt_static dependencies

set -e

# Get project root (parent of contrib/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

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

# Source nix if not already available
if ! command -v nix &> /dev/null; then
    NIX_DAEMON_SCRIPT="${NIX_DAEMON_SCRIPT:-}"
    if [ -n "$NIX_DAEMON_SCRIPT" ] && [ -f "$NIX_DAEMON_SCRIPT" ]; then
        . "$NIX_DAEMON_SCRIPT"
    elif [ -f /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh ]; then
        . /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh
    elif [ -f ~/.nix-profile/etc/profile.d/nix.sh ]; then
        . ~/.nix-profile/etc/profile.d/nix.sh
    elif [ -f /etc/profile.d/nix.sh ]; then
        . /etc/profile.d/nix.sh
    fi
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

mkdir -p "$PROJECT_ROOT/deps"

# Pull qontrol dependency
echo ""
echo "Pulling qontrol dependency..."
if [ -d "$PROJECT_ROOT/deps/qontrol/.git" ]; then
    echo "qontrol already exists, updating..."
    (cd "$PROJECT_ROOT/deps/qontrol" && git pull)
else
    echo "Cloning qontrol..."
    git clone https://github.com/pythcoiner/qontrol.git "$PROJECT_ROOT/deps/qontrol"
fi

# Pull qt_static for static Qt6 build
echo ""
echo "Pulling qt_static for static Qt6..."
if [ -d "$PROJECT_ROOT/deps/qt_static/.git" ]; then
    echo "qt_static already exists, updating..."
    (cd "$PROJECT_ROOT/deps/qt_static" && git pull)
else
    echo "Cloning qt_static..."
    git clone https://github.com/pythcoiner/qt_static.git "$PROJECT_ROOT/deps/qt_static"
fi

# Build static Qt6 for Linux
if [ ! -d "$PROJECT_ROOT/deps/qt_static/dist/linux" ] && [ ! -L "$PROJECT_ROOT/deps/qt_static/dist/linux" ]; then
    echo ""
    echo "Building static Qt6 for Linux (this may take 20-30 minutes)..."
    (cd "$PROJECT_ROOT/deps/qt_static" && ./build.sh linux)
else
    echo ""
    echo "Static Qt6 already built."
fi

echo ""
echo "=== Installation complete! ==="
echo "Run 'just build' to compile GitSardine"
