cmake_minimum_required(VERSION 3.21)

project(MGS4Ultra120 VERSION 0.3.3 LANGUAGES C CXX)

set(MGS4ULTRA120_RELEASE_VERSION "v0.3.3-alpha.1")
set(MGS4ULTRA120_VERSION_NUMERIC "0,3,3,1")

include(CTest)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
set(MINHOOK_BUILD_TESTS OFF CACHE BOOL "Disable MinHook tests" FORCE)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG d94c64d32ea37bc4f5ee47d580709f70c6fb6080
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(minhook)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(MGS4ULTRA120_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/version")
file(MAKE_DIRECTORY "${MGS4ULTRA120_GENERATED_DIR}")
configure_file(cmake/version_asi.rc.in
    "${MGS4ULTRA120_GENERATED_DIR}/version_asi.rc" @ONLY)
configure_file(cmake/version_proxy.rc.in
    "${MGS4ULTRA120_GENERATED_DIR}/version_proxy.rc" @ONLY)
configure_file(cmake/version_launcher.rc.in
    "${MGS4ULTRA120_GENERATED_DIR}/version_launcher.rc" @ONLY)

if(BUILD_TESTING)
    add_test(NAME linux_steam_options_smoke
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/linux_steam_options_smoke.py"
    )
endif()

set(WINMM_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/winmm")
if(MSVC)
    enable_language(ASM_MASM)
    set(WINMM_ASSEMBLY_FORMAT masm)
    set(WINMM_ASSEMBLY "${WINMM_GENERATED_DIR}/winmm_trampolines.asm")
else()
    enable_language(ASM)
    set(WINMM_ASSEMBLY_FORMAT gnu)
    set(WINMM_ASSEMBLY "${WINMM_GENERATED_DIR}/winmm_trampolines.S")
endif()
set(WINMM_DEFINITION "${WINMM_GENERATED_DIR}/winmm.def")
set(WINMM_INCLUDE "${WINMM_GENERATED_DIR}/winmm_exports.inc")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/generate_winmm_proxy.py"
        --exports "${CMAKE_SOURCE_DIR}/cmake/winmm_exports.txt"
        --definition "${WINMM_DEFINITION}"
        --include "${WINMM_INCLUDE}"
        --assembly "${WINMM_ASSEMBLY}"
        --format "${WINMM_ASSEMBLY_FORMAT}"
    COMMAND_ERROR_IS_FATAL ANY
)

add_library(mgs4_ultrawide SHARED
    src/mgs4_ultrawide.cpp
    src/winmm_proxy.cpp
    "${MGS4ULTRA120_GENERATED_DIR}/version_proxy.rc"
    "${WINMM_ASSEMBLY}"
    "${WINMM_DEFINITION}"
)
set_target_properties(mgs4_ultrawide PROPERTIES
    OUTPUT_NAME winmm
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_features(mgs4_ultrawide PRIVATE cxx_std_17)
target_compile_definitions(mgs4_ultrawide PRIVATE
    _WIN32_WINNT=0x0A00
    MGS4ULTRA120_WINMM_PROXY=1
    MGS4ULTRA120_VERSION="${MGS4ULTRA120_RELEASE_VERSION}"
)
target_include_directories(mgs4_ultrawide PRIVATE "${WINMM_GENERATED_DIR}")
target_link_libraries(mgs4_ultrawide PRIVATE minhook)

# Preview Windows distribution: the patch is a conventional ASI plugin and a
# pinned Ultimate ASI Loader supplies the independently maintained WinMM proxy.
# Keep the legacy proxy target above so alpha.3 remains reproducible and its
# full export-table regression test continues to run.
add_library(mgs4_ultrawide_asi SHARED
    src/mgs4_ultrawide.cpp
    "${MGS4ULTRA120_GENERATED_DIR}/version_asi.rc"
)
set_target_properties(mgs4_ultrawide_asi PROPERTIES
    OUTPUT_NAME MGS4Ultra120
    PREFIX ""
    SUFFIX ".asi"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_features(mgs4_ultrawide_asi PRIVATE cxx_std_17)
target_compile_definitions(mgs4_ultrawide_asi PRIVATE
    _WIN32_WINNT=0x0A00
    MGS4ULTRA120_ASI=1
    MGS4ULTRA120_VERSION="${MGS4ULTRA120_RELEASE_VERSION}"
)
target_link_libraries(mgs4_ultrawide_asi PRIVATE minhook)

add_executable(mgs4_direct_launcher WIN32
    launcher_wrapper/mgs4_direct_launcher.cpp
    "${MGS4ULTRA120_GENERATED_DIR}/version_launcher.rc"
)
set_target_properties(mgs4_direct_launcher PROPERTIES
    OUTPUT_NAME launcher
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_features(mgs4_direct_launcher PRIVATE cxx_std_17)
target_compile_definitions(mgs4_direct_launcher PRIVATE
    _WIN32_WINNT=0x0A00
    MGS4ULTRA120_VERSION="${MGS4ULTRA120_RELEASE_VERSION}"
)

add_executable(winmm_proxy_smoke tests/winmm_proxy_smoke.cpp)
target_compile_features(winmm_proxy_smoke PRIVATE cxx_std_17)
target_compile_definitions(winmm_proxy_smoke PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(winmm_proxy_smoke PRIVATE mgs4_ultrawide)
set_target_properties(winmm_proxy_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(BUILD_TESTING AND WIN32 AND NOT CMAKE_CROSSCOMPILING)
    add_executable(projection_math_test tests/projection_math_test.cpp)
    target_compile_features(projection_math_test PRIVATE cxx_std_17)
    target_include_directories(projection_math_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME projection_math COMMAND projection_math_test)

    add_executable(supersampling_math_test tests/supersampling_math_test.cpp)
    target_compile_features(supersampling_math_test PRIVATE cxx_std_17)
    target_include_directories(supersampling_math_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
    add_test(NAME supersampling_math COMMAND supersampling_math_test)

    add_test(NAME winmm_proxy_smoke COMMAND winmm_proxy_smoke)
    add_executable(launcher_args_probe tests/launcher_args_probe.cpp)
    target_compile_features(launcher_args_probe PRIVATE cxx_std_17)
    set_target_properties(launcher_args_probe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    add_test(NAME launcher_wrapper_smoke
        COMMAND powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/tests/launcher_wrapper_smoke.ps1"
            -Wrapper "$<TARGET_FILE:mgs4_direct_launcher>"
            -Probe "$<TARGET_FILE:launcher_args_probe>"
    )
    add_executable(asi_plugin_smoke tests/asi_plugin_smoke.cpp)
    target_compile_features(asi_plugin_smoke PRIVATE cxx_std_17)
    set_target_properties(asi_plugin_smoke PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    add_test(NAME asi_plugin_smoke
        COMMAND asi_plugin_smoke "$<TARGET_FILE:mgs4_ultrawide_asi>"
    )
    add_executable(asi_loader_probe tests/asi_loader_probe.cpp)
    target_compile_features(asi_loader_probe PRIVATE cxx_std_17)
    set_target_properties(asi_loader_probe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

if(MINGW)
    target_link_options(mgs4_ultrawide PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_options(mgs4_ultrawide_asi PRIVATE
        -static -static-libgcc -static-libstdc++
    )
    target_link_options(mgs4_direct_launcher PRIVATE
        -municode -static -static-libgcc -static-libstdc++
    )
endif()
