#pragma once

#include <string>

// The person using the tracker. Body weight is needed for the calorie
// calculations and the daily calorie goal is what the dashboard compares against.
//
// Every value is validated when it is set, so a UserProfile object is always
// in a valid state (see limits in Limits.h).
class UserProfile {
public:
    // A valid placeholder profile ("User", 30 y, 170 cm, 70 kg, 2000 kcal),
    // used until the person saves their own details.
    UserProfile();

    // Throws ValidationError describing the first invalid field.
    UserProfile(std::string name, int age, double heightCm, double weightKg,
                int dailyCalorieGoal);

    const std::string& name() const { return name_; }
    int age() const { return age_; }
    double heightCm() const { return heightCm_; }
    double weightKg() const { return weightKg_; }
    int dailyCalorieGoal() const { return dailyCalorieGoal_; }

    // Each setter validates first and leaves the profile unchanged on error.
    void setName(const std::string& name);
    void setAge(int age);
    void setHeightCm(double heightCm);
    void setWeightKg(double weightKg);
    void setDailyCalorieGoal(int kcal);

    // Body mass index = weight (kg) / height (m) squared.
    double bmi() const;

private:
    std::string name_;
    int age_;
    double heightCm_;
    double weightKg_;
    int dailyCalorieGoal_;
};
