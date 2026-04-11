# chad-game - Executable

set(GAME_SOURCES src/main.cpp)

add_executable(chad_game ${GAME_SOURCES})

# Link engine - gets all public includes/libs transitively
target_link_libraries(chad_game PRIVATE chad_engine)

# Game's own headers
target_include_directories(chad_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Output + rpath so game finds libchad_engine.so next to it
set_target_properties(chad_game PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN")
