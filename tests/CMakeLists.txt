# chad-engine tests (Catch2 + CTest)
#
# Two binaries:
#   chad_core_tests — pure C++ tests, no OpenGL context required.
#                     Runs in any environment (CI headless OK).
#   chad_gl_tests   — tests that touch OpenGL resources. Needs a display.
#                     Custom main creates a hidden GLFW window + GL 3.3
#                     context (gl_test_main.cpp). Exits with code 77
#                     (CTest convention for "skipped") on headless systems.

include(Catch)

# ---------------- core tests (headless) ----------------

set(CORE_TEST_SOURCES
    test_types.cpp
    test_math.cpp
    test_log.cpp
    test_assert.cpp
    test_camera.cpp)

add_executable(chad_core_tests ${CORE_TEST_SOURCES})

target_link_libraries(chad_core_tests
    PRIVATE
        chad_engine
        Catch2::Catch2WithMain)

set_target_properties(chad_core_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN")

catch_discover_tests(chad_core_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# ---------------- GL tests (needs display) ----------------

set(GL_TEST_SOURCES
    gl_test_main.cpp
    test_shader.cpp
    test_mesh.cpp
    test_texture.cpp
    test_material.cpp
    test_timer.cpp
    test_window.cpp
    test_input.cpp
    test_model.cpp
    test_renderer.cpp
    test_collision.cpp)

add_executable(chad_gl_tests ${GL_TEST_SOURCES})

target_link_libraries(chad_gl_tests
    PRIVATE
        chad_engine
        Catch2::Catch2
        ${GLFW_LIB}
        glad)

target_include_directories(chad_gl_tests PRIVATE ${CMAKE_SOURCE_DIR}/vendor)

set_target_properties(chad_gl_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN")

catch_discover_tests(chad_gl_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
