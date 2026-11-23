cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# CMake 4.x Policy and Compatibility Configuration
# -----------------------------------------------------------------------------
# CMake 4.0 removed compatibility with policies older than 3.5. Setting
# CMAKE_POLICY_VERSION_MINIMUM ensures dependencies fetched via FetchContent
# do not trigger deprecation halts.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# CMP0135: Use extraction timestamp for FetchContent archives
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(mighty_bomb_jack_native
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "Educational native PC port of Mighty Bomb Jack (NES)"
)

# -----------------------------------------------------------------------------
# Global Language Standards
# -----------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Build Configuration Options
# -----------------------------------------------------------------------------
option(BUILD_DESKTOP "Build native desktop executable" ON)
option(BUILD_TESTING "Build automated headless test suite" ON)
option(ENABLE_CONSOLE "Enable console logging window in release builds" ON)

# -----------------------------------------------------------------------------
# Common Compiler Options Interface Library
# -----------------------------------------------------------------------------
add_library(mbj_compile_options INTERFACE)

if(MSVC)
    target_compile_options(mbj_compile_options INTERFACE
        /W4                         # Warning level 4
        /WX-                        # Do not treat warnings as errors
        /MP                         # Multi-processor compilation
        /utf-8                      # UTF-8 source and execution charsets
        /D_CRT_SECURE_NO_WARNINGS   # Suppress CRT deprecation warnings
        /permissive-                # Standards conformance
    )
    target_compile_definitions(mbj_compile_options INTERFACE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(mbj_compile_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# -----------------------------------------------------------------------------
# Core Game Simulation Static Library (mbj_core)
# Pure C, zero platform/GUI/audio dependencies
# -----------------------------------------------------------------------------
set(MBJ_CORE_SOURCES
    src/core/game_state.c
)
foreach(OPT_SRC
    src/core/physics.c
    src/core/collision.c
    src/room/room_decoder.c
    src/room/doors.c
    src/core/chest.c
    src/core/bomb.c
    src/core/enemy.c
    src/core/timer.c
    src/core/scoring.c
    src/core/hud.c
    src/render/raylib_renderer.c
    src/asset/rom_loader.c
    src/asset/palette_data.c
    src/asset/room_data_tables.c
    src/audio/apu_synth.c
    src/audio/sound_driver.c
    src/audio/sound_data.c
)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_SRC}")
        list(APPEND MBJ_CORE_SOURCES ${OPT_SRC})
    endif()
endforeach()

add_library(mbj_core STATIC ${MBJ_CORE_SOURCES})

target_link_libraries(mbj_core PUBLIC mbj_compile_options)

target_include_directories(mbj_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(mbj_core PUBLIC c_std_11)

# -----------------------------------------------------------------------------
# Dependency: Raylib 5.5 (Static Linking via FetchContent)
# -----------------------------------------------------------------------------
if(BUILD_DESKTOP)
    include(FetchContent)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(raylib)
endif()

# -----------------------------------------------------------------------------
# Native Desktop Executable (mighty_bomb_jack)
# -----------------------------------------------------------------------------
if(BUILD_DESKTOP)
    set(MBJ_APP_SOURCES
        src/platform/main.c
    )
    foreach(OPT_APP_SRC
        src/platform/xinput_controller.c
        src/platform/keyboard_input.c
    )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_APP_SRC}")
            list(APPEND MBJ_APP_SOURCES ${OPT_APP_SRC})
        endif()
    endforeach()

    add_executable(mighty_bomb_jack ${MBJ_APP_SOURCES})

    target_link_libraries(mighty_bomb_jack PRIVATE
        mbj_core
        raylib
        mbj_compile_options
    )

    if(WIN32)
        target_link_libraries(mighty_bomb_jack PRIVATE xinput winmm)
    endif()

    target_include_directories(mighty_bomb_jack PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # Subsystem: Console allows debug/educational logging
    if(NOT ENABLE_CONSOLE AND WIN32)
        set_target_properties(mighty_bomb_jack PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    set_target_properties(mighty_bomb_jack PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    if(MSVC)
        set_target_properties(mighty_bomb_jack PROPERTIES
            VS_DPI_AWARE "PerMonitor"
        )
    endif()

    # Post-build copy of assets directory and reference ROM next to the executable
    add_custom_command(TARGET mighty_bomb_jack POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:mighty_bomb_jack>/assets"
        COMMENT "Copying assets to target executable directory"
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../asm/bin/mbj-us.nes")
        add_custom_command(TARGET mighty_bomb_jack POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/../asm/bin/mbj-us.nes"
                "$<TARGET_FILE_DIR:mighty_bomb_jack>/mbj-us.nes"
            COMMENT "Copying reference ROM next to executable"
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Automated Headless Test Suite (ctest)
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
