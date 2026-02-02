include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  z80_tests
  GIT_REPOSITORY https://github.com/SingleStepTests/z80.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(z80_tests)

add_executable(json-tests
    json-tests.cpp
)
target_link_libraries(json-tests PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(json-tests PRIVATE Z80_TESTS_DIR="${z80_tests_SOURCE_DIR}/v1")

add_executable(zex-tests
    zex-tests.cpp
)
target_link_libraries(zex-tests PRIVATE nlohmann_json::nlohmann_json)

add_executable(CPU_test CPU_test.cpp)
add_test(NAME CPU_test COMMAND CPU_test)

set_source_files_properties(../tools/Z80Asm.cpp PROPERTIES COMPILE_DEFINITIONS Z80ASM_TEST_BUILD)
add_executable(Assembler_test Assembler_test.cpp ../tools/Z80Asm.cpp)
add_test(NAME Assembler_test COMMAND Assembler_test)

add_executable(Decoder_test Decoder_test.cpp)
add_test(NAME Decoder_test COMMAND Decoder_test)