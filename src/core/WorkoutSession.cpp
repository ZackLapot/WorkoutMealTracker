#include "WorkoutSession.h"

#include "Errors.h"
#include "Limits.h"
#include "Validation.h"

#include <algorithm>
#include <numeric>
#include <utility>

namespace {

// Position of the exercise called `name` (ignoring case), or end() if absent.
template <typename Iterator>
Iterator findByName(Iterator first, Iterator last, const std::string& name)
{
    const std::string wanted = validation::toLower(validation::trim(name));
    return std::find_if(first, last, [&wanted](const std::unique_ptr<Exercise>& exercise) {
        return validation::toLower(exercise->name()) == wanted;
    });
}

} // namespace

WorkoutSession::WorkoutSession(std::string name, std::string isoDate)
    : name_(validation::requireText(name, "Workout name", limits::kMaxNameLength)),
      date_(validation::requireIsoDate(isoDate, "Workout date"))
{
}

WorkoutSession::WorkoutSession(const WorkoutSession& other)
    : name_(other.name_), date_(other.date_)
{
    exercises_.reserve(other.exercises_.size());
    for (const auto& exercise : other.exercises_) {
        exercises_.push_back(exercise->clone());
    }
}

WorkoutSession& WorkoutSession::operator=(const WorkoutSession& other)
{
    // copy first, then move in: if cloning throws, *this is unchanged
    WorkoutSession copy(other);
    *this = std::move(copy);
    return *this;
}

Exercise& WorkoutSession::addExercise(std::unique_ptr<Exercise> exercise)
{
    if (!exercise) {
        throw ValidationError("Cannot add an empty exercise to a workout.");
    }
    if (findByName(exercises_.begin(), exercises_.end(), exercise->name()) != exercises_.end()) {
        throw DuplicateEntryError("'" + exercise->name() +
                                  "' is already in this workout. Add more sets to it instead.");
    }
    exercises_.push_back(std::move(exercise));
    return *exercises_.back();
}

bool WorkoutSession::removeExercise(const std::string& name)
{
    const auto it = findByName(exercises_.begin(), exercises_.end(), name);
    if (it == exercises_.end()) {
        return false;
    }
    exercises_.erase(it);
    return true;
}

Exercise* WorkoutSession::findExercise(const std::string& name)
{
    const auto it = findByName(exercises_.begin(), exercises_.end(), name);
    return it == exercises_.end() ? nullptr : it->get();
}

const Exercise* WorkoutSession::findExercise(const std::string& name) const
{
    const auto it = findByName(exercises_.cbegin(), exercises_.cend(), name);
    return it == exercises_.cend() ? nullptr : it->get();
}

double WorkoutSession::totalCaloriesBurned(double bodyWeightKg) const
{
    validation::requireBodyWeight(bodyWeightKg); // also checked for an empty workout
    return std::accumulate(exercises_.begin(), exercises_.end(), 0.0,
                           [bodyWeightKg](double sum, const std::unique_ptr<Exercise>& exercise) {
                               return sum + exercise->caloriesBurned(bodyWeightKg);
                           });
}

bool WorkoutSession::operator<(const WorkoutSession& rhs) const
{
    if (date_ != rhs.date_) {
        return date_ < rhs.date_; // YYYY-MM-DD sorts correctly as text
    }
    return validation::toLower(name_) < validation::toLower(rhs.name_);
}

std::ostream& operator<<(std::ostream& os, const WorkoutSession& session)
{
    os << session.name_ << " (" << session.date_ << ") - " << session.exercises_.size()
       << (session.exercises_.size() == 1 ? " exercise" : " exercises");
    for (const auto& exercise : session.exercises_) {
        os << "\n  " << exercise->name() << " [" << exercise->typeName() << ", "
           << categoryToString(exercise->category()) << "]: " << exercise->describe();
    }
    return os;
}
