cmake_minimum_required(VERSION 3.30)

project(snake-game-opengl VERSION 1.0)

set(CMAKE_C_STANDARD 17) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_UTILS OFF CACHE BOOL "utilities" FORCE)

# set(GLFW_BUILD_WAYLAND ON)
# set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
# set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

# file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
set(SOURCE_FILES 
	src/snakeGame.cpp
)

add_subdirectory(vendor/glfw 
	"${CMAKE_CURRENT_BINARY_DIR}/glfw_build")
add_subdirectory(vendor/glew/build/cmake
	"${CMAKE_CURRENT_BINARY_DIR}/glew_build")

include_directories(src)
include_directories(vendor/glfw/include) 
include_directories(vendor/glew/include)

set_property(TARGET glew_s PROPERTY FOLDER GLEW)
set_property(TARGET glew   PROPERTY FOLDER GLEW)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_executable(Rectangle_Example src/rectangleExample.cpp)

target_link_libraries(${PROJECT_NAME}
	glfw
	glew_s
)

target_link_libraries(Rectangle_Example
	glfw
	glew_s
)
configure_file(
    "scripts/build_config.sh"   
    "${CMAKE_BINARY_DIR}/build_config.sh"  
    @ONLY
)



