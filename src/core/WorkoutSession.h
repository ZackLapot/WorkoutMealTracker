#pragma once

#include "Exercise.h"

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

// One workout: a name, a date and the exercises done in it.
//
// The session owns its exercises through std::unique_ptr<Exercise>, so they are
// released automatically. Because unique_ptr cannot be copied, the copy
// constructor and copy assignment below make deep copies with Exercise::clone().
class WorkoutSession {
public:
    // `isoDate` must be a real date written as YYYY-MM-DD.
    // Throws ValidationError for a blank/too long name or an invalid date.
    WorkoutSession(std::string name, std::string isoDate);

    WorkoutSession(const WorkoutSession& other);
    WorkoutSession& operator=(const WorkoutSession& other);
    WorkoutSession(WorkoutSession&&) noexcept = default;
    WorkoutSession& operator=(WorkoutSession&&) noexcept = default;
    ~WorkoutSession() = default;

    const std::string& name() const { return name_; }
    const std::string& date() const { return date_; }

    // Takes ownership of the exercise and returns a reference to it.
    // Throws ValidationError for a null pointer and DuplicateEntryError if an
    // exercise with the same name (ignoring case) is already in the workout.
    Exercise& addExercise(std::unique_ptr<Exercise> exercise);

    // Removes by name (ignoring case); returns false if it was not in the workout.
    bool removeExercise(const std::string& name);

    // nullptr if the workout has no exercise with that name.
    Exercise* findExercise(const std::string& name);
    const Exercise* findExercise(const std::string& name) const;

    std::size_t exerciseCount() const { return exercises_.size(); }
    bool empty() const { return exercises_.empty(); }
    const std::vector<std::unique_ptr<Exercise>>& exercises() const { return exercises_; }

    // Sum of caloriesBurned() over all exercises (each exercise decides how it
    // calculates its own calories). Throws ValidationError for an invalid weight.
    double totalCaloriesBurned(double bodyWeightKg) const;

    // Orders sessions by date, then by name (ignoring case). Two sessions with
    // the same date and name are equivalent, which is how duplicates are found.
    bool operator<(const WorkoutSession& rhs) const;

    // Prints a readable multi-line summary. Declared as a friend so it can read
    // the private members directly instead of going through accessors.
    friend std::ostream& operator<<(std::ostream& os, const WorkoutSession& session);

private:
    std::string name_;
    std::string date_;
    std::vector<std::unique_ptr<Exercise>> exercises_;
};
