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

function(mjk_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX /utf-8)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
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

    add_test(NAME mjk_platform_smoke COMMAND mjk --smoke-test)
endif()
