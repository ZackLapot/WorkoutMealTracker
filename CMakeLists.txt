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
    src/core/StrengthExercise.h
    src/core/StrengthExercise.cpp
    src/core/CardioExercise.h
    src/core/CardioExercise.cpp
    src/core/UserProfile.h
    src/core/UserProfile.cpp
    src/core/ExerciseLibrary.h
    src/core/ExerciseLibrary.cpp
    src/core/WorkoutSession.h
    src/core/WorkoutSession.cpp
    src/core/WorkoutLog.h
    src/core/WorkoutLog.cpp
    src/core/DataManager.h
    src/core/DataManager.cpp
)
# DataManager reads and writes JSON with Qt (only DataManager.cpp includes Qt headers).
target_link_libraries(wmt_core PRIVATE Qt${QT_VERSION_MAJOR}::Core)
wmt_enable_warnings(wmt_core)

# ---- GUI library: Qt Widgets screens built on the core library -------------
add_library(wmt_gui STATIC
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/ProfileTab.h
    src/gui/ProfileTab.cpp
    src/gui/WorkoutsTab.h
    src/gui/WorkoutsTab.cpp
)
target_link_libraries(wmt_gui PUBLIC wmt_core Qt${QT_VERSION_MAJOR}::Widgets)
wmt_enable_warnings(wmt_gui)

# ---- application ------------------------------------------------------------
add_executable(WorkoutMealTracker src/main.cpp)
target_link_libraries(WorkoutMealTracker PRIVATE wmt_gui)
wmt_enable_warnings(WorkoutMealTracker)

# ---- developer tool: regenerates data/exercise_library.json -----------------
add_executable(export_default_library tools/export_default_library.cpp)
target_link_libraries(export_default_library PRIVATE wmt_core)
wmt_enable_warnings(export_default_library)

# ---- unit tests (QtTest + CTest) --------------------------------------------
enable_testing()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

function(wmt_add_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE wmt_core Qt${QT_VERSION_MAJOR}::Test)
    target_compile_definitions(${name} PRIVATE WMT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    wmt_enable_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

wmt_add_test(tst_validation)
wmt_add_test(tst_exercises)
wmt_add_test(tst_profile)
wmt_add_test(tst_library)
wmt_add_test(tst_workout)
wmt_add_test(tst_datamanager)

# GUI tests need the Widgets library and run without a screen (offscreen platform).
add_executable(tst_gui tests/tst_gui.cpp)
target_link_libraries(tst_gui PRIVATE wmt_gui Qt${QT_VERSION_MAJOR}::Test)
target_compile_definitions(tst_gui PRIVATE WMT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
wmt_enable_warnings(tst_gui)
add_test(NAME tst_gui COMMAND tst_gui)
set_tests_properties(tst_gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
