#include "StrengthExercise.h"

#include "Errors.h"
#include "Limits.h"
#include "Validation.h"

#include <utility>

StrengthExercise::StrengthExercise(std::string name, Category category, double met)
    : Exercise(std::move(name), category, met)
{
    if (category == Category::Cardio) {
        throw ValidationError("Strength exercises cannot use the Cardio category.");
    }
}

void StrengthExercise::addSet(int reps, double weightKg)
{
    validation::requireInt(reps, limits::kMinReps, limits::kMaxReps, "Reps");
    validation::requireDouble(weightKg, limits::kMinSetWeightKg, limits::kMaxSetWeightKg,
                              "Weight", "kg");
    if (sets_.size() >= limits::kMaxSetsPerExercise) {
        throw ValidationError("An exercise can have at most " +
                              std::to_string(limits::kMaxSetsPerExercise) + " sets.");
    }
    sets_.push_back({reps, weightKg});
}

bool StrengthExercise::removeLastSet()
{
    if (sets_.empty()) {
        return false;
    }
    sets_.pop_back();
    return true;
}

void StrengthExercise::clearSets()
{
    sets_.clear();
}

int StrengthExercise::totalReps() const
{
    int total = 0;
    for (const SetEntry& set : sets_) {
        total += set.reps;
    }
    return total;
}

double StrengthExercise::totalVolumeKg() const
{
    double total = 0.0;
    for (const SetEntry& set : sets_) {
        total += set.reps * set.weightKg;
    }
    return total;
}

double StrengthExercise::activeSeconds() const
{
    if (sets_.empty()) {
        return 0.0;
    }
    return totalReps() * kSecondsPerRep + static_cast<double>(sets_.size()) * kRestSecondsPerSet;
}

double StrengthExercise::caloriesBurned(double bodyWeightKg) const
{
    return caloriesFromMet(met(), bodyWeightKg, activeSeconds() / 3600.0);
}

std::string StrengthExercise::describe() const
{
    if (sets_.empty()) {
        return "No sets logged";
    }

    std::string text = std::to_string(sets_.size()) + (sets_.size() == 1 ? " set: " : " sets: ");
    for (std::size_t i = 0; i < sets_.size(); ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += std::to_string(sets_[i].reps) + " x ";
        if (sets_[i].weightKg == 0.0) {
            text += "bodyweight";
        } else {
            text += validation::formatNumber(sets_[i].weightKg) + " kg";
        }
    }
    return text;
}

std::unique_ptr<Exercise> StrengthExercise::clone() const
{
    return std::make_unique<StrengthExercise>(*this);
}
