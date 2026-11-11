#include "CardioExercise.h"

#include "Limits.h"
#include "Validation.h"

#include <utility>

CardioExercise::CardioExercise(std::string name, double met)
    : Exercise(std::move(name), Category::Cardio, met)
{
}

void CardioExercise::logSession(double durationMin, double distanceKm)
{
    validation::requireDouble(durationMin, limits::kMinCardioMinutes, limits::kMaxCardioMinutes,
                              "Duration", "minutes");
    validation::requireDouble(distanceKm, limits::kMinDistanceKm, limits::kMaxDistanceKm,
                              "Distance", "km");
    durationMin_ = durationMin;
    distanceKm_ = distanceKm;
}

void CardioExercise::clearSession()
{
    durationMin_ = 0.0;
    distanceKm_ = 0.0;
}

double CardioExercise::averageSpeedKmh() const
{
    if (durationMin_ <= 0.0 || distanceKm_ <= 0.0) {
        return 0.0;
    }
    return distanceKm_ / (durationMin_ / 60.0);
}

double CardioExercise::caloriesBurned(double bodyWeightKg) const
{
    return caloriesFromMet(met(), bodyWeightKg, durationMin_ / 60.0);
}

std::string CardioExercise::describe() const
{
    if (!isLogged()) {
        return "Not logged";
    }

    std::string text = validation::formatNumber(durationMin_, 1) + " min";
    if (distanceKm_ > 0.0) {
        text += ", " + validation::formatNumber(distanceKm_) + " km (" +
                validation::formatNumber(averageSpeedKmh(), 1) + " km/h)";
    }
    return text;
}

std::unique_ptr<Exercise> CardioExercise::clone() const
{
    return std::make_unique<CardioExercise>(*this);
}
