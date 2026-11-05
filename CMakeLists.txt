cmake_minimum_required(VERSION 3.23)

project(nativejson_selected LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NATIVEJSON_ENABLE_NATIVE_ARCH "Compile with -march=native" OFF)
option(NATIVEJSON_BUILD_CODE_SIZE "Build one stripped-down executable per adapter" ON)
set(NATIVEJSON_MACHINE "local" CACHE STRING "Machine label used in result filenames")
set(JESON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/jeson/include" CACHE PATH
    "JESON include directory (override to benchmark a development checkout)")
configure_file(cmake/machine.h.in generated/machine.h @ONLY)

set(REFLECTCPP_ALL_FORMATS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_JSON ON CACHE BOOL "" FORCE)
set(REFLECTCPP_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_CHECK_HEADERS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_INSTALL OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_USE_BUNDLED_DEPENDENCIES ON CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/reflect-cpp EXCLUDE_FROM_ALL)

# The benchmark's allocation tracker works by replacing malloc/free at compile
# time. Build a second JSON-only reflect-cpp archive for the benchmark process
# so yyjson allocations are measured too; code-size binaries use the normal,
# uninstrumented upstream target below.
add_library(reflectcpp_tracked STATIC
    thirdparty/reflect-cpp/src/reflectcpp.cpp
    thirdparty/reflect-cpp/src/reflectcpp_json.cpp
    thirdparty/reflect-cpp/src/yyjson.c
)
set_source_files_properties(thirdparty/reflect-cpp/src/yyjson.c PROPERTIES LANGUAGE CXX)
target_include_directories(reflectcpp_tracked PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/reflect-cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/reflect-cpp/include/rfl/thirdparty
)
if(MSVC)
    target_compile_options(reflectcpp_tracked PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/src/memorystat.h)
else()
    target_compile_options(reflectcpp_tracked PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/src/memorystat.h)
endif()
target_include_directories(reflectcpp_tracked PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(NATIVEJSON_ADAPTERS
    src/tests/arduinojsontest.cpp
    src/tests/cjsontest.cpp
    src/tests/jesontest.cpp
    src/tests/nlohmanntest.cpp
    src/tests/rapidjsontest.cpp
    src/tests/reflectcpptest.cpp
)

set(NATIVEJSON_COMMON_INCLUDES
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ArduinoJson/include
    ${JESON_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rapidjson/include
)

function(nativejson_common target)
    target_include_directories(${target} PRIVATE ${NATIVEJSON_COMMON_INCLUDES})
    target_compile_definitions(${target} PRIVATE __STDC_FORMAT_MACROS=1)
    if(MSVC)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(NATIVEJSON_ENABLE_NATIVE_ARCH)
            target_compile_options(${target} PRIVATE -march=native)
        endif()
    endif()
endfunction()

set_source_files_properties(src/cjsonlibs/cjson_all.c PROPERTIES LANGUAGE CXX)
# The fork intentionally retains nativejson-benchmark's historical nlohmann
# v2.1.1 pin. That release predates C++20's allocator API removals, so compile
# just this adapter as C++17 while the JESON/reflect-cpp adapters use C++20.
if(MSVC)
    set_source_files_properties(src/tests/nlohmanntest.cpp PROPERTIES COMPILE_OPTIONS /std:c++17)
else()
    set_source_files_properties(src/tests/nlohmanntest.cpp PROPERTIES COMPILE_OPTIONS -std=c++17)
endif()

add_executable(nativejson_selected
    src/main.cpp
    src/memorystat.cpp
    src/cjsonlibs/cjson_all.c
    ${NATIVEJSON_ADAPTERS}
)
nativejson_common(nativejson_selected)
target_link_libraries(nativejson_selected PRIVATE reflectcpp_tracked)
set_target_properties(nativejson_selected PROPERTIES
    OUTPUT_NAME nativejson_release_x64_cmake
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(NATIVEJSON_BUILD_CODE_SIZE)
    function(add_jsonstat adapter)
        get_filename_component(adapter_name ${adapter} NAME_WE)
        set(target jsonstat_${adapter_name})
        set(extra_sources "")
        set(extra_libraries "")
        if(adapter_name STREQUAL "cjsontest")
            list(APPEND extra_sources src/cjsonlibs/cjson_all.c)
        elseif(adapter_name STREQUAL "reflectcpptest")
            list(APPEND extra_libraries reflectcpp::reflectcpp)
        endif()

        add_executable(${target}
            src/jsonstat/jsonstatmain.cpp
            src/memorystat.cpp
            ${adapter}
            ${extra_sources}
        )
        nativejson_common(${target})
        target_compile_definitions(${target} PRIVATE
            USE_MEMORYSTAT=0
            TEST_PARSE=1
            TEST_STRINGIFY=0
            TEST_PRETTIFY=0
            TEST_STATISTICS=1
            TEST_SAXROUNDTRIP=0
            TEST_SAXSTATISTICS=0
            TEST_SAXSTATISTICSUTF16=0
            TEST_CONFORMANCE=0
            TEST_INFO=0
        )
        target_link_libraries(${target} PRIVATE ${extra_libraries})
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME jsonstat_${adapter_name}_release_x64_cmake
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/jsonstat
        )
    endfunction()

    foreach(adapter IN LISTS NATIVEJSON_ADAPTERS)
        add_jsonstat(${adapter})
    endforeach()
endif()
