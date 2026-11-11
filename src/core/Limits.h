#pragma once

#include <cstddef>

// Every numeric limit used for input validation lives here, so the rules are
// documented in one place and the tests can refer to the same values.
namespace limits {

constexpr std::size_t kMaxNameLength = 60;

// User profile
constexpr int kMinAgeYears = 10;
constexpr int kMaxAgeYears = 120;
constexpr double kMinHeightCm = 100.0;
constexpr double kMaxHeightCm = 250.0;
constexpr double kMinWeightKg = 20.0;
constexpr double kMaxWeightKg = 400.0;
constexpr int kMinCalorieGoal = 800;
constexpr int kMaxCalorieGoal = 10000;

// Exercises
constexpr double kMinMet = 0.5;
constexpr double kMaxMet = 20.0;

// Strength sets
constexpr int kMinReps = 1;
constexpr int kMaxReps = 100;
constexpr double kMinSetWeightKg = 0.0; // 0 = body weight only
constexpr double kMaxSetWeightKg = 1000.0;
constexpr std::size_t kMaxSetsPerExercise = 50;

// Cardio sessions
constexpr double kMinCardioMinutes = 1.0;
constexpr double kMaxCardioMinutes = 600.0;
constexpr double kMinDistanceKm = 0.0; // 0 = distance not recorded
constexpr double kMaxDistanceKm = 500.0;

} // namespace limits
