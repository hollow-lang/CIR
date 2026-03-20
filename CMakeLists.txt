cmake_minimum_required(VERSION 3.16)
project(cir CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)
# add_compile_options(-flto -O3)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CIR_SOURCES core/cir.cpp)
set(CIR_HEADERS core/cir.h)

add_library(cir_shared SHARED ${CIR_SOURCES} ${CIR_HEADERS})
set_target_properties(cir_shared PROPERTIES
        OUTPUT_NAME cir
        POSITION_INDEPENDENT_CODE ON
)

add_library(cir_static STATIC ${CIR_SOURCES} ${CIR_HEADERS})
set_target_properties(cir_static PROPERTIES
        OUTPUT_NAME cir
)

add_executable(cas main.cpp)
target_link_libraries(cas PRIVATE cir_shared)

add_executable(discas tools/disassembly/main.cpp)
target_link_libraries(discas PRIVATE cir_shared)
target_include_directories(discas PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(decbc tools/debugger/main.cpp)
target_link_libraries(decbc PRIVATE cir_shared)
target_include_directories(decbc PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(bfcas tools/bf/main.cpp)
target_include_directories(bfcas PRIVATE ${CMAKE_SOURCE_DIR})

add_subdirectory(examples/dl-imports EXCLUDE_FROM_ALL)

add_custom_target(run
        COMMAND ${CMAKE_BINARY_DIR}/cas ${CMAKE_SOURCE_DIR}/example.cas -d ${CMAKE_BINARY_DIR}/examples/dl-imports/libexample.so
        DEPENDS cas libexample
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cas with example"
)