#include "WorkoutLog.h"

#include "Errors.h"
#include "Validation.h"

#include <algorithm>
#include <utility>

namespace {

bool sameSession(const WorkoutSession& session, const std::string& isoDate, const std::string& name)
{
    return session.date() == isoDate &&
           validation::toLower(session.name()) == validation::toLower(validation::trim(name));
}

} // namespace

void WorkoutLog::add(WorkoutSession session)
{
    // first position that is not "less than" the new session
    const auto position = std::lower_bound(sessions_.begin(), sessions_.end(), session);

    // if the session there is not greater either, the two are equivalent: a duplicate
    if (position != sessions_.end() && !(session < *position)) {
        throw DuplicateEntryError("A workout named '" + session.name() + "' already exists on " +
                                  session.date() + ".");
    }
    sessions_.insert(position, std::move(session));
}

bool WorkoutLog::remove(const std::string& isoDate, const std::string& name)
{
    const auto it = std::find_if(sessions_.begin(), sessions_.end(),
                                 [&](const WorkoutSession& s) { return sameSession(s, isoDate, name); });
    if (it == sessions_.end()) {
        return false;
    }
    sessions_.erase(it);
    return true;
}

WorkoutSession* WorkoutLog::find(const std::string& isoDate, const std::string& name)
{
    const auto it = std::find_if(sessions_.begin(), sessions_.end(),
                                 [&](const WorkoutSession& s) { return sameSession(s, isoDate, name); });
    return it == sessions_.end() ? nullptr : &*it;
}

const WorkoutSession* WorkoutLog::find(const std::string& isoDate, const std::string& name) const
{
    const auto it = std::find_if(sessions_.begin(), sessions_.end(),
                                 [&](const WorkoutSession& s) { return sameSession(s, isoDate, name); });
    return it == sessions_.end() ? nullptr : &*it;
}

std::vector<const WorkoutSession*> WorkoutLog::sessionsOn(const std::string& isoDate) const
{
    std::vector<const WorkoutSession*> matches;
    for (const WorkoutSession& session : sessions_) {
        if (session.date() == isoDate) {
            matches.push_back(&session);
        }
    }
    return matches;
}

double WorkoutLog::caloriesBurnedOn(const std::string& isoDate, double bodyWeightKg) const
{
    validation::requireBodyWeight(bodyWeightKg);
    double total = 0.0;
    for (const WorkoutSession* session : sessionsOn(isoDate)) {
        total += session->totalCaloriesBurned(bodyWeightKg);
    }
    return total;
}
