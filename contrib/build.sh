#!/usr/bin/env bash
set -e

# Get project root (parent of contrib/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

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
    else
        echo "Error: nix not found. Install nix or set NIX_DAEMON_SCRIPT env var."
        exit 1
    fi
fi

QT_STATIC_DIR="$PROJECT_ROOT/deps/qt_static/dist/linux"
if [ ! -d "$QT_STATIC_DIR" ] && [ ! -L "$QT_STATIC_DIR" ]; then
    echo "Error: Static Qt6 not found at $QT_STATIC_DIR"
    echo "Run 'just install' first."
    exit 1
fi

# Resolve symlink to get actual Qt path
if [ -L "$QT_STATIC_DIR" ]; then
    QT_STATIC_DIR="$(readlink -f "$QT_STATIC_DIR")"
fi

# Use project's own flake with pure environment (like bed)
cd "$PROJECT_ROOT"
nix develop --ignore-environment --keep HOME --impure --command bash -c "
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_PREFIX_PATH='$QT_STATIC_DIR' .. && \
    cmake --build .
"
