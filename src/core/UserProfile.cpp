#include "UserProfile.h"

#include "Limits.h"
#include "Validation.h"

#include <utility>

namespace {

// Small wrappers so the constructor and the setters use exactly the same rules.
std::string checkedName(const std::string& name)
{
    return validation::requireText(name, "Name", limits::kMaxNameLength);
}

int checkedAge(int age)
{
    return validation::requireInt(age, limits::kMinAgeYears, limits::kMaxAgeYears, "Age", "years");
}

double checkedHeight(double heightCm)
{
    return validation::requireDouble(heightCm, limits::kMinHeightCm, limits::kMaxHeightCm,
                                     "Height", "cm");
}

double checkedWeight(double weightKg)
{
    return validation::requireDouble(weightKg, limits::kMinWeightKg, limits::kMaxWeightKg,
                                     "Weight", "kg");
}

int checkedGoal(int kcal)
{
    return validation::requireInt(kcal, limits::kMinCalorieGoal, limits::kMaxCalorieGoal,
                                  "Daily calorie goal", "kcal");
}

} // namespace

UserProfile::UserProfile()
    : name_("User"), age_(30), heightCm_(170.0), weightKg_(70.0), dailyCalorieGoal_(2000)
{
}

UserProfile::UserProfile(std::string name, int age, double heightCm, double weightKg,
                         int dailyCalorieGoal)
    : name_(checkedName(name)),
      age_(checkedAge(age)),
      heightCm_(checkedHeight(heightCm)),
      weightKg_(checkedWeight(weightKg)),
      dailyCalorieGoal_(checkedGoal(dailyCalorieGoal))
{
}

void UserProfile::setName(const std::string& name)
{
    name_ = checkedName(name);
}

void UserProfile::setAge(int age)
{
    age_ = checkedAge(age);
}

void UserProfile::setHeightCm(double heightCm)
{
    heightCm_ = checkedHeight(heightCm);
}

void UserProfile::setWeightKg(double weightKg)
{
    weightKg_ = checkedWeight(weightKg);
}

void UserProfile::setDailyCalorieGoal(int kcal)
{
    dailyCalorieGoal_ = checkedGoal(kcal);
}

double UserProfile::bmi() const
{
    const double heightM = heightCm_ / 100.0;
    return weightKg_ / (heightM * heightM);
}
