# GitSardine build commands

# Install dependencies (libgit2, qt6, etc.)
install:
    chmod +x install.sh
    ./install.sh

# Build the project
build:
    mkdir -p build
    cd build && cmake .. && cmake --build .

# Run the application
run:
    ./build/gitsardine

# Build and run
br: build run

# Run tests
test: build
    cd build && ctest --output-on-failure
