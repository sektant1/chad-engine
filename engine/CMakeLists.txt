# chad-engine - Shared Library

set(ENGINE_SOURCES
    src/platform/window.cpp
    src/platform/input.cpp
    src/platform/timer.cpp
    src/renderer/shader.cpp
    src/renderer/mesh.cpp
    src/renderer/texture.cpp
    src/renderer/material.cpp
    src/renderer/camera.cpp
    src/renderer/model.cpp
    src/renderer/renderer.cpp
    src/physics/collision.cpp)

add_library(chad_engine SHARED ${ENGINE_SOURCES})

# Public headers
target_include_directories(chad_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Dependencies
target_link_libraries(
  chad_engine
  PUBLIC glad imgui
  PRIVATE ${GLFW_LIB}
          assimp::assimp
          ${CMAKE_DL_LIBS}
          m)

# Internal include paths (stb_image, etc.)
target_include_directories(chad_engine PRIVATE ${CMAKE_SOURCE_DIR}/vendor)

# Output .so next to game binary so $ORIGIN rpath finds it
set_target_properties(chad_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
