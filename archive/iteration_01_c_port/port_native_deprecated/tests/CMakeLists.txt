# Automated Headless Test Suite for Mighty Bomb Jack

add_executable(test_suite test_suite.c)

target_link_libraries(test_suite PRIVATE
    mbj_core
    mbj_compile_options
)

target_include_directories(test_suite PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(test_suite PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME AllTests COMMAND test_suite)
