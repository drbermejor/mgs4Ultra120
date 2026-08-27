cmake_minimum_required(VERSION 3.21)

project(MGS4Ultra120 VERSION 0.2.1 LANGUAGES C CXX)

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

add_library(mgs4_ultrawide SHARED src/mgs4_ultrawide.cpp)
set_target_properties(mgs4_ultrawide PROPERTIES
    OUTPUT_NAME winmm
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_features(mgs4_ultrawide PRIVATE cxx_std_17)
target_compile_definitions(mgs4_ultrawide PRIVATE _WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
target_link_libraries(mgs4_ultrawide PRIVATE minhook winmm)

if(MINGW)
    target_link_options(mgs4_ultrawide PRIVATE -static -static-libgcc -static-libstdc++)
endif()
