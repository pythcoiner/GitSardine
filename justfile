# GitSardine build commands

# Install dependencies (qontrol, qt_static)
install:
    chmod +x contrib/install.sh
    ./contrib/install.sh

# Build the project with static Qt6 using nix
build:
    chmod +x contrib/build.sh
    ./contrib/build.sh

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
