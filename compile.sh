#!/bin/bash

set -e # Exit early if any commands fail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default build type is debug
BUILD_TYPE="debug"

# Check for argument to set build type
if [ "$1" = "--release" ] || [ "$1" = "-r" ]; then
    BUILD_TYPE="release"
    shift # Remove the argument so it doesn't get passed to the binary
fi
# Ensure compile steps are run within the repository directory

cd "$SCRIPT_DIR"

# Create and build in the appropriate directory
BUILD_DIR="bin/$BUILD_TYPE"
echo "Building in $BUILD_DIR..."

# Configure and build
cmake -B "$BUILD_DIR" -S . -DCMAKE_BUILD_TYPE="${BUILD_TYPE^}"  
cmake --build "$BUILD_DIR"

# Gets the binary name that cmake exports to
source "$BUILD_DIR/build_config.sh"

# Execute the binary (which should have the same name as the project)
EXECUTABLE="$BUILD_DIR/$PROJECT_EXECUTABLE_NAME"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Erro: O executável '$PROJECT_EXECUTABLE_NAME' não foi encontrado em $BUILD_DIR."
    echo "Verifique se a variável PROJECT_NAME no script bate com o add_executable no CMakeLists.txt"
    exit 1
fi

echo "Running: $EXECUTABLE $@"
exec "$EXECUTABLE" "$@"
