#include "Exercise.h"

#include "Errors.h"
#include "Limits.h"
#include "Validation.h"

const std::vector<Category>& allCategories()
{
    static const std::vector<Category> categories = {
        Category::Chest, Category::Back, Category::Legs, Category::Shoulders,
        Category::Arms,  Category::Core, Category::Cardio};
    return categories;
}

std::string categoryToString(Category category)
{
    switch (category) {
    case Category::Chest:     return "Chest";
    case Category::Back:      return "Back";
    case Category::Legs:      return "Legs";
    case Category::Shoulders: return "Shoulders";
    case Category::Arms:      return "Arms";
    case Category::Core:      return "Core";
    case Category::Cardio:    return "Cardio";
    }
    return "Unknown"; // not reachable; keeps compilers happy
}

Category categoryFromString(const std::string& text)
{
    const std::string wanted = validation::toLower(validation::trim(text));
    for (Category category : allCategories()) {
        if (validation::toLower(categoryToString(category)) == wanted) {
            return category;
        }
    }
    throw ValidationError("Unknown category '" + validation::trim(text) + "'.");
}

Exercise::Exercise(std::string name, Category category, double met)
    : name_(validation::requireText(name, "Exercise name", limits::kMaxNameLength)),
      category_(category),
      met_(validation::requireDouble(met, limits::kMinMet, limits::kMaxMet, "MET value"))
{
}

double Exercise::caloriesFromMet(double met, double bodyWeightKg, double durationHours)
{
    validation::requireBodyWeight(bodyWeightKg);
    return met * bodyWeightKg * durationHours;
}
