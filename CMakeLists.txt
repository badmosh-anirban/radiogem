cmake_minimum_required(VERSION 3.18)
project(pyradiolib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add RadioLib submodule/source
add_subdirectory(RadioLib)

# Common include directories
set(PYRADIOLIB_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioLib/src
)

# Find lgpio library
find_library(LGPIO_LIB NAMES lgpio)
if(NOT LGPIO_LIB)
    set(LGPIO_LIB "lgpio")
endif()

# ==============================================================================
# Target 1: Standalone Native C++ Verification Executable (Optional)
# ==============================================================================
option(BUILD_NATIVE_TEST "Build native C++ standalone verification executable" OFF)

if(BUILD_NATIVE_TEST OR (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/native_test.cpp" AND NOT DEFINED SKBUILD))
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/native_test.cpp")
        add_executable(native_test examples/native_test.cpp)
        target_include_directories(native_test PRIVATE ${PYRADIOLIB_INCLUDES})
        target_link_libraries(native_test PRIVATE RadioLib ${LGPIO_LIB})
    endif()
endif()

# ==============================================================================
# Target 2: pyradiolib Python Extension Module (pybind11)
# ==============================================================================
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 1. Try system pybind11 config
find_package(pybind11 CONFIG QUIET)

# 2. Try python3 -m pybind11 --cmake
if(NOT pybind11_FOUND AND Python3_EXECUTABLE)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmake
        OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE PYBIND11_PY_RES
        ERROR_QUIET
    )
    if(PYBIND11_PY_RES EQUAL 0 AND PYBIND11_CMAKE_DIR)
        find_package(pybind11 CONFIG QUIET PATHS "${PYBIND11_CMAKE_DIR}")
    endif()
endif()

# 3. Fallback: Download via FetchContent if not available locally
if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found locally; fetching via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
endif()

pybind11_add_module(pyradiolib
    src/bindings.cpp
    src/sx1262_wrapper.cpp
)

target_include_directories(pyradiolib PRIVATE ${PYRADIOLIB_INCLUDES})
target_link_libraries(pyradiolib PRIVATE RadioLib ${LGPIO_LIB})

# Copy built extension module to root or examples for convenient testing
set_target_properties(pyradiolib PROPERTIES
    OUTPUT_NAME "pyradiolib"
)
