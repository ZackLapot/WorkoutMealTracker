#pragma once

#include "ExerciseLibrary.h"
#include "UserProfile.h"
#include "WorkoutLog.h"

#include <string>

// What happened when a data file was loaded.
enum class LoadStatus {
    Ok,       // file read completely
    Missing,  // no file yet (normal on first start); defaults were used
    Partial,  // file readable, but some entries were invalid and skipped
    Damaged   // file unusable; renamed to <name>.corrupt and defaults were used
};

// The loaded value plus how the load went. A load never throws for a bad file:
// the program always gets a usable value and a message it can show the user.
template <typename T>
struct LoadResult {
    T value{};
    LoadStatus status = LoadStatus::Ok;
    std::string message; // empty when status is Ok

    bool ok() const { return status == LoadStatus::Ok; }
};

// Saves and loads the profile, the exercise library and the workouts as JSON
// files inside one data folder:
//
//   profile.json, workouts.json, exercise_library.json
//
// Files are written atomically (temp file + rename) so a crash while saving
// cannot leave a half-written file behind.
class DataManager {
public:
    static constexpr const char* kProfileFile = "profile.json";
    static constexpr const char* kWorkoutsFile = "workouts.json";
    static constexpr const char* kLibraryFile = "exercise_library.json";

    // The folder is created on the first save if it does not exist yet.
    // Throws ValidationError for an empty path.
    explicit DataManager(std::string dataDir);

    const std::string& dataDir() const { return dataDir_; }
    std::string profilePath() const;
    std::string workoutsPath() const;
    std::string libraryPath() const;

    // Loading: missing file -> defaults (Missing); unreadable/invalid file ->
    // defaults, file renamed to <name>.corrupt (Damaged); a few bad entries ->
    // the good ones are kept (Partial).
    LoadResult<UserProfile> loadProfile() const;
    LoadResult<ExerciseLibrary> loadLibrary() const;
    LoadResult<WorkoutLog> loadWorkouts() const;

    // Saving: throws DataFileError if the folder or file cannot be written.
    void saveProfile(const UserProfile& profile) const;
    void saveLibrary(const ExerciseLibrary& library) const;
    void saveWorkouts(const WorkoutLog& log) const;

private:
    std::string dataDir_;
};
