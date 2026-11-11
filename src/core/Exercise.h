#pragma once

#include <memory>
#include <string>
#include <vector>

// Muscle group (or "Cardio") an exercise belongs to. Used to search the library.
enum class Category { Chest, Back, Legs, Shoulders, Arms, Core, Cardio };

// All categories, in display order.
const std::vector<Category>& allCategories();

// "Chest", "Back", ...
std::string categoryToString(Category category);

// Case-insensitive inverse of categoryToString(); throws ValidationError for unknown text.
Category categoryFromString(const std::string& text);

// Abstract base class for everything you can do in a workout.
//
// An Exercise has a name, a category and a MET value (metabolic equivalent: how
// hard the activity is compared with resting). Subclasses add the data that is
// logged for that kind of exercise and decide how calories are calculated, so
// code holding an Exercise* never needs to know which kind it is (polymorphism).
class Exercise {
public:
    // Throws ValidationError if the name is blank/too long or the MET is out of range.
    Exercise(std::string name, Category category, double met);

    virtual ~Exercise() = default;

    // Exercises are copied through clone() so the copy keeps its real type.
    Exercise& operator=(const Exercise&) = delete;

    const std::string& name() const { return name_; }
    Category category() const { return category_; }
    double met() const { return met_; }

    // "strength" or "cardio" (also the value stored in the JSON files).
    virtual std::string typeName() const = 0;

    // Calories burned by what has been logged so far, for a person of the given weight.
    virtual double caloriesBurned(double bodyWeightKg) const = 0;

    // One-line, human-readable summary of what has been logged.
    virtual std::string describe() const = 0;

    // Polymorphic copy: returns a new object of the same dynamic type.
    virtual std::unique_ptr<Exercise> clone() const = 0;

protected:
    // Only subclasses may copy (used by clone()); this prevents object slicing.
    Exercise(const Exercise&) = default;

    // calories = MET x body weight (kg) x time (hours). Validates the weight.
    static double caloriesFromMet(double met, double bodyWeightKg, double durationHours);

private:
    std::string name_;
    Category category_;
    double met_;
};
