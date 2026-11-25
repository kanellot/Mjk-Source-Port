cmake_minimum_required(VERSION 3.16)

# Evitar mspdbsrv.exe (/Zi) usando /Z7 (Embedded) en MSVC cuando CMake >= 3.25
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

project(mighty_bomb_jack VERSION 1.0.0 LANGUAGES C)

# Estandar estricto C11 sin extensiones propietarias
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Salidas limpias unificadas en build/bin y build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(MJK_ENABLE_SANITIZERS "Enable ASan + UBSan on GCC/Clang" OFF)
set(MJK_TEST_ROM_PATH "" CACHE FILEPATH "Optional path to mbj-us.nes for ROM-backed unit tests")

function(mjk_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX /utf-8)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        if(MJK_ENABLE_SANITIZERS)
            target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# Localizacion multiplataforma de SDL2 (Windows third_party/ o Linux sistema)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2/SDL2-2.30.8/cmake/sdl2-config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2/SDL2-2.30.8/cmake")
endif()
find_package(SDL2 REQUIRED)

# =============================================================================
# Target 1: Motor puro C11 (cero dependencias de SDL2 o del sistema operativo)
# =============================================================================
set(MJK_ENGINE_SOURCES
    src/assets/sha1.c
    src/assets/rom_loader.c
    src/assets/asset_bank.c
    src/game/input.c
    src/game/scoring.c
    src/game/world.c
    src/game/player.c
    src/game/objects.c
    src/game/doors.c
    src/game/enemies.c
    src/game/screens.c
    src/game/game.c
    src/render/renderer.c
    src/audio/synth_2a03.c
    src/audio/audio_engine.c
)

add_library(mjk_engine STATIC ${MJK_ENGINE_SOURCES})
target_include_directories(mjk_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(NOT WIN32)
    target_link_libraries(mjk_engine PUBLIC m)
endif()
mjk_apply_warnings(mjk_engine)

# =============================================================================
# Target 2: Ejecutable principal SDL2 (cero codigo de debug o tests)
# =============================================================================
add_executable(mjk
    src/main.c
    src/platform/platform_sdl.c
)
target_link_libraries(mjk PRIVATE mjk_engine SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    target_link_libraries(mjk PRIVATE SDL2::SDL2main)
endif()
if(WIN32)
    add_custom_command(TARGET mjk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:mjk>
        COMMENT "Copiando SDL2.dll al directorio de salida"
    )
endif()
mjk_apply_warnings(mjk)

# =============================================================================
# Target 3: Suite exhaustiva de unit tests C11 (CTest con SKIP_RETURN_CODE 77)
# =============================================================================
include(CTest)
if(BUILD_TESTING)
    function(mjk_add_unit_test target_name source_file)
        add_executable(${target_name} ${source_file})
        target_link_libraries(${target_name} PRIVATE mjk_engine)
        target_compile_definitions(${target_name} PRIVATE
            MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            MJK_TEST_ROM_PATH="${MJK_TEST_ROM_PATH}"
        )
        mjk_apply_warnings(${target_name})
        add_test(NAME ${target_name} COMMAND ${target_name})
        set_tests_properties(${target_name} PROPERTIES SKIP_RETURN_CODE 77)
    endfunction()

    mjk_add_unit_test(test_input     tests/c/test_input.c)
    mjk_add_unit_test(test_assets    tests/c/test_assets.c)
    mjk_add_unit_test(test_scoring   tests/c/test_scoring.c)
    mjk_add_unit_test(test_world     tests/c/test_world.c)
    mjk_add_unit_test(test_player    tests/c/test_player.c)
    mjk_add_unit_test(test_objects   tests/c/test_objects.c)
    mjk_add_unit_test(test_doors     tests/c/test_doors.c)
    mjk_add_unit_test(test_enemies   tests/c/test_enemies.c)
    mjk_add_unit_test(test_renderer  tests/c/test_renderer.c)
    mjk_add_unit_test(test_audio     tests/c/test_audio.c)
    mjk_add_unit_test(test_game_flow tests/c/test_game_flow.c)
endif()
