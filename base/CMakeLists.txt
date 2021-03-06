cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/cxx_flag_overrides.cmake)

project(base LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)
find_path(V8_PATH include/v8.h HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/libv8-7.2-${SYSTEM_NAME})
find_path(LIBCXX_PATH lib/libc++.a HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/libc++-${SYSTEM_NAME})

message("-- Covert js code to c header file")
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/js/js2c.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/js)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} BASE_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/flex FLEX_SRCS)

add_library(base INTERFACE)
target_sources(base INTERFACE ${BASE_SRCS} ${FLEX_SRCS})
target_include_directories(base INTERFACE ${V8_PATH}/include)
if(UNIX)
    target_link_libraries(base INTERFACE ${V8_PATH}/lib/libv8_monolith.a pthread)
endif()
if(WIN32)
    target_link_libraries(base INTERFACE ${V8_PATH}/lib/v8_monolith.lib winmm dbghelp shlwapi)
endif()
if(UNIX AND NOT APPLE)
    target_include_directories(base INTERFACE ${LIBCXX_PATH}/include/c++/v1)
    target_compile_options(base INTERFACE -nostdinc++)
    target_link_libraries(base INTERFACE -Wl,--whole-archive ${LIBCXX_PATH}/lib/libc++.a ${LIBCXX_PATH}/lib/libc++abi.a -Wl,--no-whole-archive)
    target_link_libraries(base INTERFACE -nodefaultlibs -lm -lc -lrt -lgcc_s)
endif()

# testing #
option(BUILD_TESTING "Build the testing tree." OFF)
option(BUILD_COVERAGE "Enable coverage reporting" OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(BUILD_TESTING ON)
    find_package(Catch2 REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/Catch2")
    aux_source_directory(tests BASE_TESTS_SRCS)
    add_executable(tests ${BASE_TESTS_SRCS})
    target_link_libraries(tests base Catch2::Catch2)

    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    if(BUILD_COVERAGE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake")
        include(CodeCoverage)
        append_coverage_compiler_flags()
        set(COVERAGE_LCOV_EXCLUDES '*usr*' '*libv8*' '*tests*' '*catch2*' 'v1/*' '*flex*')
        setup_target_for_coverage_lcov(NAME coverage EXECUTABLE tests DEPENDENCIES tests)
        if(LIBCXX_PATH)
            target_link_libraries(tests gcov)
        endif()
    endif()
endif()
