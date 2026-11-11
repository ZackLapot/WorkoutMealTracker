cmake_minimum_required(VERSION 3.16)

project(WorkoutMealTracker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# Works with Qt 6 (preferred) or Qt 5.15.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

# Turn on a sensible set of compiler warnings for one target.
function(wmt_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- core library: pure C++ logic (no GUI) ----------------------------------
add_library(wmt_core STATIC
    src/core/Errors.h
    src/core/Limits.h
    src/core/Validation.h
    src/core/Validation.cpp
    src/core/Exercise.h
    src/core/Exercise.cpp
)
wmt_enable_warnings(wmt_core)

# ---- application ------------------------------------------------------------
add_executable(WorkoutMealTracker src/main.cpp)
target_link_libraries(WorkoutMealTracker PRIVATE wmt_core Qt${QT_VERSION_MAJOR}::Widgets)
wmt_enable_warnings(WorkoutMealTracker)

# ---- unit tests (QtTest + CTest) --------------------------------------------
enable_testing()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

function(wmt_add_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE wmt_core Qt${QT_VERSION_MAJOR}::Test)
    wmt_enable_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

wmt_add_test(tst_validation)
