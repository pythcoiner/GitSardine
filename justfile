# GitSardine build commands

# Install dependencies (qontrol, qt_static)
install:
    chmod +x install.sh
    ./install.sh

# Build the project with static Qt6 using nix
build:
    #!/usr/bin/env bash
    set -e
    . /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh

    QT_STATIC_DIR="$(pwd)/deps/qt_static/dist/linux"
    if [ ! -d "$QT_STATIC_DIR" ]; then
        echo "Error: Static Qt6 not found at $QT_STATIC_DIR"
        echo "Run 'just install' first."
        exit 1
    fi

    # Use nix develop from qt_static for build environment
    cd deps/qt_static && QT_SRC_PATH="$(pwd)/qt-src/qtbase" nix develop --impure --command bash -c "\
        cd ../.. && \
        mkdir -p build && \
        cd build && \
        cmake -DCMAKE_PREFIX_PATH=$QT_STATIC_DIR .. && \
        cmake --build ."

# Run the application
run:
    ./build/gitsardine

# Build and run
br: build run

# Run tests
test: build
    cd build && ctest --output-on-failure

# Clean build directory
clean:
    rm -rf build

