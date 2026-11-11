#pragma once

#include "Exercise.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// A rep-based exercise such as a bench press: the user logs sets of reps x weight.
class StrengthExercise : public Exercise {
public:
    struct SetEntry {
        int reps;
        double weightKg; // 0 means body weight only
    };

    // Time model used for the calorie estimate: each rep takes a few seconds and
    // every set is followed by a rest period.
    static constexpr double kSecondsPerRep = 4.0;
    static constexpr double kRestSecondsPerSet = 60.0;

    // Throws ValidationError if the category is Cardio (see CardioExercise).
    StrengthExercise(std::string name, Category category, double met);

    // Logs one set. Throws ValidationError for invalid reps/weight or if the
    // exercise already has the maximum number of sets.
    void addSet(int reps, double weightKg);

    // Removes the most recent set; returns false if there were none.
    bool removeLastSet();
    void clearSets();

    const std::vector<SetEntry>& sets() const { return sets_; }
    std::size_t setCount() const { return sets_.size(); }
    int totalReps() const;
    double totalVolumeKg() const; // sum of reps x weight
    double activeSeconds() const; // time assumed for the calorie estimate

    std::string typeName() const override { return "strength"; }
    double caloriesBurned(double bodyWeightKg) const override;
    std::string describe() const override;
    std::unique_ptr<Exercise> clone() const override;

private:
    std::vector<SetEntry> sets_;
};
