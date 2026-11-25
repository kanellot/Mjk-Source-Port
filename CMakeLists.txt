cmake_minimum_required(VERSION 3.16)

# Evitar mspdbsrv.exe (/Zi) usando /Z7 (Embedded) en MSVC cuando CMake >= 3.25
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

project(mighty_bomb_jack VERSION 0.1.0 LANGUAGES C)

# Estándar estricto C11 sin extensiones propietarias
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Salidas limpias unificadas en build/bin y build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(MJK_BUILD_SHARED_CORE "Build shared core library (mjk_core.dll / libmjk_core.so)" ON)
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

# Localización multiplataforma de SDL2:
# - Windows: usa third_party/sdl2/SDL2-2.30.8/cmake si está disponible
# - Linux: usa el paquete SDL2 del sistema (libsdl2-dev)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2/SDL2-2.30.8/cmake/sdl2-config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2/SDL2-2.30.8/cmake")
endif()
find_package(SDL2 REQUIRED)

# Fuentes puras del núcleo de simulación (cero dependencias de SDL o SO)
set(MJK_CORE_SOURCES
    src/core/mjk_build_info.c
    src/core/mjk_sha1.c
    src/core/mjk_rom_extractor.c
    src/core/mjk_assets.c
    src/core/mjk_ppu.c
    src/core/mjk_room_decoder.c
    src/core/mjk_title.c
    src/core/mjk_state_machine.c
    src/core/mjk_end_screens.c
    src/core/mjk_player.c
    src/core/mjk_camera.c
    src/core/mjk_objects.c
    src/core/mjk_doors.c
    src/core/mjk_enemy.c
    src/core/mjk_enemy_collectibles.c
    src/core/mjk_hud.c
    src/core/mjk_apu_synth.c
    src/core/mjk_apu.c
    src/core/mjk_verify_api.c
)

# Target 1: Núcleo estático C11 puro
add_library(mjk_core STATIC ${MJK_CORE_SOURCES})
target_include_directories(mjk_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(NOT WIN32)
    target_link_libraries(mjk_core PUBLIC m)
endif()
mjk_apply_warnings(mjk_core)

# Target opcional: Núcleo compartido para verificación externa
if(MJK_BUILD_SHARED_CORE)
    add_library(mjk_core_shared SHARED ${MJK_CORE_SOURCES})
    set_target_properties(mjk_core_shared PROPERTIES
        OUTPUT_NAME "mjk_core"
        ARCHIVE_OUTPUT_NAME "mjk_core_dll"
    )
    target_include_directories(mjk_core_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_definitions(mjk_core_shared
        PUBLIC MJK_CORE_SHARED
        PRIVATE MJK_CORE_BUILDING_DLL
    )
    if(NOT WIN32)
        target_link_libraries(mjk_core_shared PRIVATE m)
    endif()
    mjk_apply_warnings(mjk_core_shared)
endif()

# Target 2: Ejecutable principal SDL2
add_executable(mjk
    src/main.c
    src/platform/mjk_platform_sdl.c
    src/platform/mjk_debug_overlay.c
)
target_link_libraries(mjk PRIVATE mjk_core SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    target_link_libraries(mjk PRIVATE SDL2::SDL2main)
endif()
if(WIN32)
    target_link_libraries(mjk PRIVATE shell32)
    add_custom_command(TARGET mjk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:mjk>
        COMMENT "Copiando SDL2.dll al directorio de salida"
    )
endif()
mjk_apply_warnings(mjk)

# Target 3: Suite de tests unitarios C11 (Capa 1 / CTest)
include(CTest)
if(BUILD_TESTING)
    add_executable(mjk_tests tests/c/test_smoke.c)
    target_link_libraries(mjk_tests PRIVATE mjk_core)
    mjk_apply_warnings(mjk_tests)
    add_test(NAME mjk_smoke COMMAND mjk_tests)

    add_executable(test_types tests/c/test_types.c)
    target_link_libraries(test_types PRIVATE mjk_core)
    mjk_apply_warnings(test_types)
    add_test(NAME mjk_types COMMAND test_types)

    add_executable(test_hash tests/c/test_hash.c)
    target_link_libraries(test_hash PRIVATE mjk_core)
    target_compile_definitions(test_hash PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_hash)
    add_test(NAME mjk_hash COMMAND test_hash)

    add_executable(test_assets tests/c/test_assets.c)
    target_link_libraries(test_assets PRIVATE mjk_core)
    target_compile_definitions(test_assets PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_assets)
    add_test(NAME mjk_assets COMMAND test_assets)

    add_executable(test_ppu tests/c/test_ppu.c)
    target_link_libraries(test_ppu PRIVATE mjk_core)
    target_compile_definitions(test_ppu PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_ppu)
    add_test(NAME mjk_ppu COMMAND test_ppu)

    add_executable(test_room_decoder tests/c/test_room_decoder.c)
    target_link_libraries(test_room_decoder PRIVATE mjk_core)
    target_compile_definitions(test_room_decoder PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_room_decoder)
    add_test(NAME mjk_room_decoder COMMAND test_room_decoder)

    add_executable(test_state_machine tests/c/test_state_machine.c)
    target_link_libraries(test_state_machine PRIVATE mjk_core)
    target_compile_definitions(test_state_machine PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_state_machine)
    add_test(NAME mjk_state_machine COMMAND test_state_machine)

    add_executable(test_player tests/c/test_player.c)
    target_link_libraries(test_player PRIVATE mjk_core)
    target_compile_definitions(test_player PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_player)
    add_test(NAME mjk_player COMMAND test_player)

    add_executable(test_camera tests/c/test_camera.c)
    target_link_libraries(test_camera PRIVATE mjk_core)
    target_compile_definitions(test_camera PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_camera)
    add_test(NAME mjk_camera COMMAND test_camera)

    add_executable(test_objects tests/c/test_objects.c)
    target_link_libraries(test_objects PRIVATE mjk_core)
    target_compile_definitions(test_objects PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_objects)
    add_test(NAME mjk_objects COMMAND test_objects)

    add_executable(test_doors tests/c/test_doors.c)
    target_link_libraries(test_doors PRIVATE mjk_core)
    target_compile_definitions(test_doors PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_doors)
    add_test(NAME mjk_doors COMMAND test_doors)

    add_executable(test_enemy tests/c/test_enemy.c)
    target_link_libraries(test_enemy PRIVATE mjk_core)
    target_compile_definitions(test_enemy PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_enemy)
    add_test(NAME mjk_enemy COMMAND test_enemy)

    add_executable(test_hud tests/c/test_hud.c)
    target_link_libraries(test_hud PRIVATE mjk_core)
    target_compile_definitions(test_hud PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_hud)
    add_test(NAME mjk_hud COMMAND test_hud)

    add_executable(test_apu tests/c/test_apu.c)
    target_link_libraries(test_apu PRIVATE mjk_core)
    target_compile_definitions(test_apu PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_apu)
    add_test(NAME mjk_apu COMMAND test_apu)

    add_executable(test_full_game tests/c/test_full_game.c)
    target_link_libraries(test_full_game PRIVATE mjk_core)
    target_compile_definitions(test_full_game PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_full_game)
    add_test(NAME mjk_full_game COMMAND test_full_game)

    add_executable(test_items_mighty tests/c/test_items_mighty.c)
    target_link_libraries(test_items_mighty PRIVATE mjk_core)
    target_compile_definitions(test_items_mighty PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_items_mighty)
    add_test(NAME mjk_items_mighty COMMAND test_items_mighty)

    add_executable(test_game_flow tests/c/test_game_flow.c)
    target_link_libraries(test_game_flow PRIVATE mjk_core)
    target_compile_definitions(test_game_flow PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(test_game_flow)
    add_test(NAME mjk_game_flow COMMAND test_game_flow)

    add_test(NAME mjk_platform_smoke COMMAND mjk --smoke-test)

    # Capturador temporal de Golden Vectors (Ticket R-01; se retira en R-17)
    add_executable(capture_golden tests/c/capture_golden.c)
    target_link_libraries(capture_golden PRIVATE mjk_core)
    target_compile_definitions(capture_golden PRIVATE MJK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    mjk_apply_warnings(capture_golden)

    # =========================================================================
    # Nuevo motor refactorizado C11 moderno (Hitos R1..R5; sustituira mjk_core en R-17)
    # =========================================================================
    set(MJK_ENGINE_SOURCES
        src/assets/sha1.c
        src/assets/rom_loader.c
        src/assets/asset_bank.c
        src/game/input.c
    )
    add_library(mjk_engine STATIC ${MJK_ENGINE_SOURCES})
    target_include_directories(mjk_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    if(NOT WIN32)
        target_link_libraries(mjk_engine PUBLIC m)
    endif()
    mjk_apply_warnings(mjk_engine)

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

    mjk_add_unit_test(test_engine_input tests/c/test_input.c)
    mjk_add_unit_test(test_engine_assets tests/c/test_assets_new.c)
endif()
