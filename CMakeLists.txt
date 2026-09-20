cmake_minimum_required(VERSION 3.28)

project(
    qiven-context-draft
    VERSION 0.1.0
    DESCRIPTION "Executable C++ specification of the qiven-context cognition architecture"
    LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()

function(qiven_context_draft_enable_strong_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endfunction()

set(QIVEN_FOUNDATION_ROOT "" CACHE PATH "Path to the qiven-foundation source tree")

if(NOT TARGET qiven::foundation)
    if(QIVEN_FOUNDATION_ROOT)
        set(_qiven_foundation_source "${QIVEN_FOUNDATION_ROOT}")
    else()
        get_filename_component(_qiven_foundation_source "${CMAKE_CURRENT_SOURCE_DIR}/../qiven-foundation" ABSOLUTE)
    endif()

    if(NOT EXISTS "${_qiven_foundation_source}/CMakeLists.txt" OR
       NOT EXISTS "${_qiven_foundation_source}/include/qiven/types.hpp" OR
       NOT EXISTS "${_qiven_foundation_source}/include/qiven/contracts.hpp")
        message(FATAL_ERROR "QIVEN_FOUNDATION_ROOT must identify a qiven-foundation source tree")
    endif()

    set(QIVEN_BUILD_TESTS OFF)
    add_subdirectory("${_qiven_foundation_source}" "${CMAKE_CURRENT_BINARY_DIR}/qiven-foundation" EXCLUDE_FROM_ALL)
    unset(QIVEN_BUILD_TESTS)
endif()

add_library(qiven-context-draft STATIC)
add_library(qiven::context_draft ALIAS qiven-context-draft)

set(
    QIVEN_CONTEXT_DRAFT_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qiven/context/cognition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qiven/context/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qiven/context/persistence.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qiven/context/runtime.hpp
)

set(
    QIVEN_CONTEXT_DRAFT_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/persistence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.cpp
)

target_sources(
    qiven-context-draft
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${QIVEN_CONTEXT_DRAFT_PUBLIC_HEADERS}
    PRIVATE
        ${QIVEN_CONTEXT_DRAFT_PRIVATE_SOURCES}
)

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PREFIX "Header Files"
        FILES ${QIVEN_CONTEXT_DRAFT_PUBLIC_HEADERS}
    )
    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        PREFIX "Source Files"
        FILES ${QIVEN_CONTEXT_DRAFT_PRIVATE_SOURCES}
    )
endif()

target_compile_features(qiven-context-draft PUBLIC cxx_std_20)
target_include_directories(
    qiven-context-draft
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(qiven-context-draft PUBLIC qiven::foundation)
qiven_context_draft_enable_strong_warnings(qiven-context-draft)

set_target_properties(
    qiven-context-draft
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Context"
)

add_subdirectory(apps)

option(QIVEN_CONTEXT_DRAFT_BUILD_TESTS "Build qiven-context-draft tests" ${PROJECT_IS_TOP_LEVEL})

if(QIVEN_CONTEXT_DRAFT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)

    if(MSVC)
        set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT qiven-context-draft-loop)
    endif()
endif()
