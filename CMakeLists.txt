# BattleArena: unified CMake workspace for both presentations.
#
# Open this folder in Visual Studio (File > Open > Folder) to get both executables:
#   BattleArenaConsole  - the text console game (BattleArena/main.cpp)
#   BattleArenaGUI      - the SFML graphical interface (BattleArenaGUI/)
# Both link the one shared game-model library, BattleArenaCore, which is defined in
# BattleArenaGUI/CMakeLists.txt. BattleArena.sln and BattleArenaGUI/ (on its own)
# still build independently.
cmake_minimum_required(VERSION 3.28)
project(BattleArena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Both executables land in <build>/bin; the GUI copies its assets/ folder next to itself.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enabled here so the GUI's tests are visible to ctest from the root build directory.
enable_testing()

# Defines BattleArenaCore (the model), BattleArenaGUI, and LogScrollTests.
add_subdirectory(BattleArenaGUI)

# The console game: its own main.cpp plus the shared model library.
add_executable(BattleArenaConsole BattleArena/main.cpp)
target_link_libraries(BattleArenaConsole PRIVATE BattleArenaCore)
if(MSVC)
    target_compile_options(BattleArenaConsole PRIVATE /W4 /permissive-)
endif()

# Startup project when CMake generates a Visual Studio solution.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BattleArenaGUI)
