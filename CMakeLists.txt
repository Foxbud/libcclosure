cmake_minimum_required(VERSION 3.19)
# Copyright 2021 Garrett Fairburn <breadboardfox@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


project(CClosure
    VERSION 1.0.0
    DESCRIPTION "Thread-safe closures as first-class functions for C"
    LANGUAGES C)

# Define options.
set(BUILD_ARCH_OPTS "x86" "x86_64")
list(JOIN BUILD_ARCH_OPTS " " BUILD_ARCH_OPTS_STR)
set(BUILD_ARCH "x86_64" CACHE STRING "Target architecure: ${BUILD_ARCH_OPTS_STR}")
if(NOT "${BUILD_ARCH}" IN_LIST BUILD_ARCH_OPTS)
    message(FATAL_ERROR
        "\"${BUILD_ARCH}\" is not a supported architecture!\n"
        "Supported architectures: ${BUILD_ARCH_OPTS_STR}"
    )
endif()

set(BUILD_THREADING TRUE CACHE BOOL "Whether or not to build with multi-threading support")

# Handle pre-release version.
set(PRE_RELEASE TRUE)
if(PRE_RELEASE)
    set(EXTENDED_PROJECT_VERSION "${CMAKE_PROJECT_VERSION}-pre")
else()
    set(EXTENDED_PROJECT_VERSION "${CMAKE_PROJECT_VERSION}")
endif()

# Set cmake defaults.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(CTest)
include(GenerateExportHeader)

if(BUILD_THREADING)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads)
    if(CMAKE_USE_PTHREADS_INIT)
        set(THREAD_PTHREADS TRUE)
    else()
        message(FATAL_ERROR
            "Build with multi-threading support selected, but could not find pthreads!"
        )
    endif()
endif()

find_package(Doxygen)

# Add cclosure object library target.
add_library(cclosure OBJECT
    "src/cclosure.c"
)
generate_export_header(cclosure
    BASE_NAME CCLOSURE
    EXPORT_FILE_NAME include/export.h)
set_target_properties(cclosure PROPERTIES
    C_VISIBILITY_PRESET hidden
    CMAKE_VISIBILITY_INLINES_HIDDEN TRUE
    POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(cclosure
    PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_compile_options(cclosure 
    PRIVATE
        -Wall -Wextra -Werror
        $<$<STREQUAL:${BUILD_ARCH},x86>:-m32>
)
if(THREAD_PTHREADS)
    target_compile_definitions(cclosure
        PRIVATE THREAD_PTHREADS=1)
endif()

# Add cclosure concrete library targets.
add_library(cclosure_static STATIC "$<TARGET_OBJECTS:cclosure>")
add_library(cclosure_shared SHARED "$<TARGET_OBJECTS:cclosure>")
get_target_property(CCLOSURE_INTERFACE_INCLUDE_DIRECTORIES cclosure INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(cclosure_static PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CCLOSURE_INTERFACE_INCLUDE_DIRECTORIES}"
    OUTPUT_NAME cclosure)
set_target_properties(cclosure_shared PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CCLOSURE_INTERFACE_INCLUDE_DIRECTORIES}"
    OUTPUT_NAME cclosure)
target_link_options(cclosure_shared
    PRIVATE
        -rdynamic
        $<$<STREQUAL:${BUILD_ARCH},x86>:-m32>
)
if(THREAD_PTHREADS)
    target_link_libraries(cclosure_static
        PRIVATE Threads::Threads)
    target_link_libraries(cclosure_shared
        PRIVATE Threads::Threads)
endif()

# Doxygen.
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NUMBER "${EXTENDED_PROJECT_VERSION}")
    set(DOXYGEN_HAVE_DOT FALSE)
    set(DOXYGEN_SHOW_INCLUDE_FILES FALSE)
    set(DOXYGEN_TYPEDEF_HIDES_STRUCT TRUE)
    set(DOXYGEN_STRIP_FROM_PATH "include")
    doxygen_add_docs(docs
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()



# Installation.
install(TARGETS cclosure_static cclosure_shared
    EXPORT CClosureTargets
    LIBRARY DESTINATION "lib$<$<STREQUAL:${BUILD_ARCH},x86>:32>"
    ARCHIVE DESTINATION "lib$<$<STREQUAL:${BUILD_ARCH},x86>:32>"
    INCLUDES DESTINATION include
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/cclosure.h"
    DESTINATION include
)
if("${BUILD_ARCH}" STREQUAL "x86")
    install(EXPORT CClosureTargets
        FILE CClosureTargets.cmake
        NAMESPACE CClosure::
        DESTINATION lib32/cmake/CClosure
    )
else()
    install(EXPORT CClosureTargets
        FILE CClosureTargets.cmake
        NAMESPACE CClosure::
        DESTINATION lib/cmake/CClosure
    )
endif()


# Testing.
if (BUILD_TESTING)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TestRunners")
    macro(make_test TEST_NAME)
        set(TEST_TARGET "test_${TEST_NAME}_runner")
        add_executable("${TEST_TARGET}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/src/${TEST_NAME}.c")
        set_target_properties("${TEST_TARGET}" PROPERTIES
            OUTPUT_NAME "TestRunners/${TEST_TARGET}")
        target_include_directories("${TEST_TARGET}"
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/include")
        target_compile_options("${TEST_TARGET}"
            PRIVATE
                -Og
                -g
                -Wall
                -Wextra
                -Werror
                $<$<STREQUAL:${BUILD_ARCH},x86>:-m32>
        )
        target_link_options("${TEST_TARGET}"
            PRIVATE
                -rdynamic
                $<$<STREQUAL:${BUILD_ARCH},x86>:-m32>
        )
        target_link_libraries("${TEST_TARGET}" PRIVATE cclosure_shared)
        add_test(NAME "test_${TEST_NAME}" COMMAND "${TEST_TARGET}")
    endmacro()

    make_test(thread_type)
    if(THREAD_PTHREADS)
        target_compile_definitions("${TEST_TARGET}"
            PRIVATE THREAD_TYPE=CCLOSURE_THREAD_PTHREADS)
    else()
        target_compile_definitions("${TEST_TARGET}"
            PRIVATE THREAD_TYPE=CCLOSURE_THREAD_NONE)
    endif()

    make_test(basic_alloc)
    make_test(basic_usage)
    make_test(agg_return)
    make_test(agg_pass)
    make_test(vargs_pass)
    make_test(get_fcn)
    make_test(excessive_alloc)

    if(THREAD_PTHREADS)
        make_test(basic_threading)
        target_link_libraries("${TEST_TARGET}"
            PRIVATE Threads::Threads)
        make_test(excessive_threading)
        target_link_libraries("${TEST_TARGET}"
            PRIVATE Threads::Threads)
    endif()
endif()