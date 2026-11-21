# BattleArena graphical interface. It displays the real game model, compiled from
# ../BattleArena (shared with the console project, not copied) as the BattleArenaCore library.
# Based on the official SFML CMake template: https://github.com/SFML/cmake-sfml-project
cmake_minimum_required(VERSION 3.28)
project(BattleArenaGUI LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# The game model: everything in ../BattleArena except the console's main.cpp. No SFML.
set(MODEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../BattleArena)
add_library(BattleArenaCore STATIC
    ${MODEL_DIR}/arena.cpp
    ${MODEL_DIR}/bat.cpp
    ${MODEL_DIR}/battlefactory.cpp
    ${MODEL_DIR}/combatant.cpp
    ${MODEL_DIR}/goblin.cpp
    ${MODEL_DIR}/hero.cpp
    ${MODEL_DIR}/herotitle.cpp
    ${MODEL_DIR}/monster.cpp
    ${MODEL_DIR}/ogre.cpp
    ${MODEL_DIR}/random.cpp
    ${MODEL_DIR}/skeleton.cpp
    ${MODEL_DIR}/worldstate.cpp)
target_include_directories(BattleArenaCore PUBLIC ${MODEL_DIR})
target_compile_features(BattleArenaCore PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(BattleArenaCore PRIVATE /W4 /permissive-)
endif()

add_executable(BattleArenaGUI
    src/main.cpp
    src/Art.cpp
    src/Art.hpp
    src/PixelFont.cpp
    src/PixelFont.hpp
    src/Renderer.cpp
    src/Renderer.hpp
    src/Layout.hpp
    src/LogScroll.hpp
    src/ViewState.hpp)
target_compile_features(BattleArenaGUI PRIVATE cxx_std_20)
target_link_libraries(BattleArenaGUI PRIVATE BattleArenaCore SFML::Graphics)

if(MSVC)
    target_compile_options(BattleArenaGUI PRIVATE /W4 /permissive- /utf-8)
endif()

# Copy assets next to the executable so "assets/..." resolves when run from the output folder.
add_custom_command(TARGET BattleArenaGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BattleArenaGUI>/assets)

# Unit tests for the combat-log scrollbar math.
enable_testing()
add_executable(LogScrollTests tests/log_scroll_tests.cpp)
target_compile_features(LogScrollTests PRIVATE cxx_std_20)
target_include_directories(LogScrollTests PRIVATE src)
target_link_libraries(LogScrollTests PRIVATE SFML::Graphics)
if(MSVC)
    target_compile_options(LogScrollTests PRIVATE /W4 /permissive-)
endif()
add_test(NAME LogScrollTests COMMAND LogScrollTests)

set_target_properties(BattleArenaGUI PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:BattleArenaGUI>)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BattleArenaGUI)
