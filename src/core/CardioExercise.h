#pragma once

#include "Exercise.h"

#include <memory>
#include <string>

// A time-based exercise such as running or cycling: the user logs how long
// they did it and, optionally, how far they went.
class CardioExercise : public Exercise {
public:
    // The category of a cardio exercise is always Category::Cardio.
    CardioExercise(std::string name, double met);

    // Logs (or replaces) the session. Distance 0 means "not recorded".
    // Throws ValidationError if the duration or distance is out of range.
    void logSession(double durationMin, double distanceKm = 0.0);
    void clearSession();

    bool isLogged() const { return durationMin_ > 0.0; }
    double durationMin() const { return durationMin_; }
    double distanceKm() const { return distanceKm_; }
    double averageSpeedKmh() const; // 0 when no distance was recorded

    std::string typeName() const override { return "cardio"; }
    double caloriesBurned(double bodyWeightKg) const override;
    std::string describe() const override;
    std::unique_ptr<Exercise> clone() const override;

private:
    double durationMin_ = 0.0;
    double distanceKm_ = 0.0;
};
