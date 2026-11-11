#include "ExerciseLibrary.h"

#include "CardioExercise.h"
#include "Errors.h"
#include "StrengthExercise.h"
#include "Validation.h"

#include <utility>

namespace {

struct DefaultExercise {
    const char* name;
    Category category;
    double met; // approximate values, in the range used by published MET tables
};

const DefaultExercise kStrengthDefaults[] = {
    {"Bench Press", Category::Chest, 5.0},
    {"Incline Dumbbell Press", Category::Chest, 5.0},
    {"Push-Up", Category::Chest, 3.8},
    {"Chest Fly", Category::Chest, 3.5},

    {"Deadlift", Category::Back, 6.0},
    {"Barbell Row", Category::Back, 5.0},
    {"Lat Pulldown", Category::Back, 4.0},
    {"Pull-Up", Category::Back, 5.0},
    {"Seated Cable Row", Category::Back, 4.0},

    {"Squat", Category::Legs, 6.0},
    {"Leg Press", Category::Legs, 5.0},
    {"Lunge", Category::Legs, 4.5},
    {"Romanian Deadlift", Category::Legs, 5.5},
    {"Calf Raise", Category::Legs, 3.5},
    {"Leg Curl", Category::Legs, 3.5},

    {"Overhead Press", Category::Shoulders, 5.0},
    {"Lateral Raise", Category::Shoulders, 3.5},
    {"Face Pull", Category::Shoulders, 3.5},

    {"Bicep Curl", Category::Arms, 3.5},
    {"Hammer Curl", Category::Arms, 3.5},
    {"Tricep Pushdown", Category::Arms, 3.5},
    {"Tricep Dip", Category::Arms, 4.5},

    {"Crunch", Category::Core, 3.8},
    {"Sit-Up", Category::Core, 3.8},
    {"Hanging Leg Raise", Category::Core, 4.0},
    {"Russian Twist", Category::Core, 3.8},
};

// Cardio exercises always use Category::Cardio.
const DefaultExercise kCardioDefaults[] = {
    {"Running", Category::Cardio, 9.8},
    {"Jogging", Category::Cardio, 7.0},
    {"Walking", Category::Cardio, 3.5},
    {"Brisk Walking", Category::Cardio, 4.3},
    {"Hiking", Category::Cardio, 6.0},
    {"Cycling", Category::Cardio, 8.0},
    {"Stationary Bike", Category::Cardio, 7.0},
    {"Swimming", Category::Cardio, 5.8},
    {"Rowing Machine", Category::Cardio, 7.0},
    {"Elliptical", Category::Cardio, 5.0},
    {"Jump Rope", Category::Cardio, 11.8},
    {"Stair Climber", Category::Cardio, 9.0},
};

// The map key for a name: trimmed and lower-cased.
std::string keyFor(const std::string& name)
{
    return validation::toLower(validation::trim(name));
}

} // namespace

ExerciseLibrary ExerciseLibrary::builtInDefaults()
{
    ExerciseLibrary library;
    for (const DefaultExercise& row : kStrengthDefaults) {
        library.add(std::make_unique<StrengthExercise>(row.name, row.category, row.met));
    }
    for (const DefaultExercise& row : kCardioDefaults) {
        library.add(std::make_unique<CardioExercise>(row.name, row.met));
    }
    return library;
}

void ExerciseLibrary::add(std::unique_ptr<Exercise> exercise)
{
    if (!exercise) {
        throw ValidationError("Cannot add an empty exercise to the library.");
    }

    const std::string key = keyFor(exercise->name());
    if (byName_.count(key) != 0) {
        throw DuplicateEntryError("The library already contains an exercise named '" +
                                  exercise->name() + "'.");
    }
    byName_.emplace(key, ExercisePtr(std::move(exercise)));
}

bool ExerciseLibrary::remove(const std::string& name)
{
    return byName_.erase(keyFor(name)) > 0;
}

ExerciseLibrary::ExercisePtr ExerciseLibrary::find(const std::string& name) const
{
    const auto it = byName_.find(keyFor(name));
    return it == byName_.end() ? nullptr : it->second;
}

std::vector<ExerciseLibrary::ExercisePtr> ExerciseLibrary::search(
    const std::string& nameQuery, std::optional<Category> category) const
{
    const std::string needle = keyFor(nameQuery);

    std::vector<ExercisePtr> matches;
    for (const auto& [key, exercise] : byName_) {
        if (category && exercise->category() != *category) {
            continue;
        }
        if (!needle.empty() && key.find(needle) == std::string::npos) {
            continue;
        }
        matches.push_back(exercise);
    }
    return matches;
}

std::vector<ExerciseLibrary::ExercisePtr> ExerciseLibrary::all() const
{
    return search("");
}
