#!/bin/bash
# GitSardine dependency installer
# Installs libgit2, qt6, and pulls qontrol dependency

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== GitSardine Installer ==="

# Pull qontrol dependency
echo ""
echo "Pulling qontrol dependency..."
mkdir -p "$SCRIPT_DIR/deps"
if [ -d "$SCRIPT_DIR/deps/qontrol/.git" ]; then
    echo "qontrol already exists, updating..."
    cd "$SCRIPT_DIR/deps/qontrol" && git pull
else
    echo "Cloning qontrol..."
    git clone https://github.com/pythcoiner/qontrol.git "$SCRIPT_DIR/deps/qontrol"
fi

# Detect distribution
echo ""
echo "Detecting distribution..."

if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
elif [ -f /etc/debian_version ]; then
    DISTRO="debian"
elif [ -f /etc/arch-release ]; then
    DISTRO="arch"
else
    echo "Unknown distribution"
    exit 1
fi

echo "Detected: $DISTRO"

# Install system dependencies
case "$DISTRO" in
    ubuntu|debian|linuxmint|pop)
        echo "Installing dependencies for Debian/Ubuntu..."
        sudo apt-get update
        sudo apt-get install -y libgit2-dev cmake build-essential qt6-base-dev git
        ;;
    arch|manjaro|endeavouros)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -Sy --noconfirm libgit2 cmake base-devel qt6-base git
        ;;
    fedora)
        echo "Installing dependencies for Fedora..."
        sudo dnf install -y libgit2-devel cmake gcc-c++ qt6-qtbase-devel git
        ;;
    *)
        echo "Unsupported distribution: $DISTRO"
        echo "Please install manually: libgit2, cmake, qt6-base, git"
        exit 1
        ;;
esac

echo ""
echo "=== Installation complete! ==="
echo "Run 'just build' to compile GitSardine"
