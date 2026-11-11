# Workout & Meal Tracker

CST8219 – C++ Programming, individual project (Zack Lapot, section 301, group 6).

A desktop application that logs workouts and meals and shows daily calories
in, calories out and the net total against a daily goal.

**Status: first progress update.** The workout half is working (profile, exercise
library, workouts, saving/loading, Qt screens, tests). Meals, the calorie
dashboard and reports are planned for the next milestones (see the table below).

## Build and run

Requirements: a C++17 compiler, CMake 3.16+ and Qt 6 (Qt 5.15 also works).

```bash
cmake -S . -B build              # add -DWMT_QT_MAJOR=5 to force Qt 5
cmake --build build
./build/WorkoutMealTracker       # optional: --data-dir <folder>
```

In Qt Creator, open `CMakeLists.txt` and press Run.

Data files are stored in the per-user application data folder (for example
`~/.local/share/WorkoutMealTracker/` on Linux). The status bar shows the exact folder.
`--data-dir <folder>` uses another folder.

## Run the tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

| Test program        | What it checks |
|---------------------|----------------|
| `tst_validation`    | number parsing, ranges, ISO dates, number formatting |
| `tst_exercises`     | exercise classes, calorie maths, limits, cloning, polymorphism |
| `tst_profile`       | profile fields and boundaries, BMI |
| `tst_library`       | search by name/category, duplicates, built-in list |
| `tst_workout`       | WorkoutSession (copying, `operator<`, `operator<<`) and WorkoutLog |
| `tst_datamanager`   | JSON round trips; missing, empty, garbage, wrong-type and partly invalid files |
| `tst_gui`           | the real window driven offscreen: validation messages, logging, saving, restart |

`tst_gui` runs with `QT_QPA_PLATFORM=offscreen`, so no display is needed.

## Project layout

```
src/core/     pure C++ logic (no GUI): exercises, profile, library, workouts, JSON, validation
src/gui/      Qt Widgets: MainWindow, ProfileTab, WorkoutsTab
tests/        QtTest unit and GUI tests
tools/        export_default_library (makes data/exercise_library.json),
              capture_screenshots (offscreen screenshots of the real window)
data/         exercise_library.json  – sample exercise library (38 exercises)
docs/         UML class diagram (.dot/.png/.svg) and screenshots
```

## Design in one paragraph

`Exercise` is an abstract class (pure virtual `caloriesBurned`, `describe`, `typeName`,
`clone`) with two subclasses: `StrengthExercise` (sets of reps x weight) and
`CardioExercise` (duration and distance). Calories = MET x body weight (kg) x hours; a
strength set is assumed to take 4 s per rep plus 60 s rest. A `WorkoutSession` owns its
exercises through `std::unique_ptr<Exercise>`, the `ExerciseLibrary` shares template
exercises through `std::shared_ptr`, and a workout gets its own copy of a library
exercise via `clone()`. `DataManager` saves and loads JSON files; a missing file gives
defaults, a damaged file is renamed to `<name>.corrupt` and defaults are used, and
individual bad entries are skipped. The GUI only collects input and shows messages; all
rules live in the core classes, which throw `ValidationError` / `DuplicateEntryError` /
`DataFileError`.

MET values in the library are approximate figures in the range used by published MET
tables; they are estimates, not medical data.

## Course concepts and where they are used

| Concept | Where |
|---------|-------|
| Classes, encapsulation, constructors/destructors | all core classes (validating constructors, virtual destructor in `Exercise`, deep copy constructor in `WorkoutSession`) |
| Inheritance, abstract class, pure virtual functions | `Exercise` → `StrengthExercise`, `CardioExercise`; `TrackerError` → `ValidationError` ... |
| Polymorphism | `WorkoutSession::totalCaloriesBurned` calls `Exercise::caloriesBurned` through base pointers; `clone()` |
| Smart pointers | `unique_ptr` in `WorkoutSession`, `shared_ptr` in `ExerciseLibrary` |
| STL containers and algorithms | `vector`, `map`; `lower_bound`, `find_if`, `accumulate`, `sort` |
| Templates | `LoadResult<T>` (the larger `Stats<T>` class is planned for the reports milestone) |
| Operator overloading | `WorkoutSession::operator<`, `operator<<` (`operator+` / `operator==` planned for meals) |
| Friend function | `operator<<` for `WorkoutSession` |
| File I/O | `DataManager` (JSON through Qt's `QJson*`, atomic writes with `QSaveFile`) |
| Qt GUI | `MainWindow`, `ProfileTab`, `WorkoutsTab` |
| Input validation, exceptions | `Validation`, `Limits.h`, `Errors.h`, red messages in the tabs |

## Progress against the plan

- [x] Oct 15–21: repository, CMake setup, core classes, exercise library file
- [x] Oct 22–28: WorkoutSession (`operator<`, friend function), ExerciseLibrary, DataManager, first unit tests
- [x] Oct 29–Nov 4: Qt main window, Profile tab, Workouts tab
- [x] Nov 5–13: validation, testing, UML update, first update
- [ ] Nov 14–20: Nutrition, Meal, premade meal library and custom meals (`operator+`, `operator==`)
- [ ] Nov 21–27: MealPlan, CalorieTracker, Meals tab and Dashboard
- [ ] Nov 28–Dec 3: `Stats<T>` reports, Reports tab, search and filtering, full edge-case pass
- [ ] Dec 4–10: final testing, bug fixes, final UML, README, project report, demo

## Use of AI assistance

This code, its tests, the UML diagram and this README were produced with the help of
Claude (Anthropic, via Claude Code) from my project plan. Commits made that way carry a
`Co-Authored-By: Claude` line. The commit dates are the dates the work was actually
committed.
