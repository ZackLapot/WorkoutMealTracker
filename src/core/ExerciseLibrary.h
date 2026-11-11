#pragma once

#include "Exercise.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The catalogue of exercises the user can pick from when building a workout.
//
// The library holds "template" exercises (nothing logged). To use one in a
// workout, call clone() on it and log sets/cardio on the copy.
class ExerciseLibrary {
public:
    using ExercisePtr = std::shared_ptr<const Exercise>;

    ExerciseLibrary() = default;

    // The standard set of exercises shipped with the program (also used as the
    // fallback when the library file is missing or damaged).
    static ExerciseLibrary builtInDefaults();

    // Adds an exercise. Throws ValidationError for a null pointer and
    // DuplicateEntryError if an exercise with the same name (ignoring case) exists.
    void add(std::unique_ptr<Exercise> exercise);

    // Removes by name (ignoring case); returns false if there was no such exercise.
    bool remove(const std::string& name);

    // Exact name match (ignoring case); nullptr if not found.
    ExercisePtr find(const std::string& name) const;

    // Exercises whose name contains `nameQuery` (ignoring case) and, if given,
    // that belong to `category`. An empty query matches every name.
    // Results are sorted by name.
    std::vector<ExercisePtr> search(const std::string& nameQuery,
                                    std::optional<Category> category = std::nullopt) const;

    // Every exercise, sorted by name.
    std::vector<ExercisePtr> all() const;

    std::size_t size() const { return byName_.size(); }
    bool empty() const { return byName_.empty(); }

private:
    // key = lower-case, trimmed name; keeps the exercises sorted and unique
    std::map<std::string, ExercisePtr> byName_;
};
