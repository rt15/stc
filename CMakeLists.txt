cmake_minimum_required(VERSION 3.13.4)

project(stc)

set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-long-long")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset=cp1252")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-stack-arg-probe -fno-stack-check -fno-stack-protector")

string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

if(WIN32)
        add_compile_definitions(WIN32)
        add_compile_definitions(_CONSOLE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BINARY_SUFFIX "d")
        add_compile_definitions(_DEBUG)
else()
        set(BINARY_SUFFIX "")
        add_compile_definitions(NDEBUG)
        add_link_options("-s")
endif()

include_directories(include)

include_directories(${PARENT_DIR}/rt15_portable_runtime/rpr/include)
include_directories(${PARENT_DIR}/clang+llvm-18.1.8-x86_64-pc-windows-msvc/include)

link_directories(${PARENT_DIR}/rt15_portable_runtime/rpr/lib)
link_directories(${PARENT_DIR}/clang+llvm-18.1.8-x86_64-pc-windows-msvc/lib)

link_libraries(staticrpr${BINARY_SUFFIX})
link_libraries(LLVM-C)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(${PROJECT_NAME}${BINARY_SUFFIX} ${SOURCES})
