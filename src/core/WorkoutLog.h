#pragma once

#include "WorkoutSession.h"

#include <cstddef>
#include <string>
#include <vector>

// Every workout the user has logged, kept sorted by date (then name).
class WorkoutLog {
public:
    // Adds a session in its sorted position. Throws DuplicateEntryError if a
    // session with the same date and name (ignoring case) already exists.
    void add(WorkoutSession session);

    // Removes the session with this date and name; returns false if not found.
    bool remove(const std::string& isoDate, const std::string& name);

    // nullptr if there is no such session. Pointers are only valid until the
    // log is next modified.
    WorkoutSession* find(const std::string& isoDate, const std::string& name);
    const WorkoutSession* find(const std::string& isoDate, const std::string& name) const;

    const std::vector<WorkoutSession>& sessions() const { return sessions_; }
    std::size_t size() const { return sessions_.size(); }
    bool empty() const { return sessions_.empty(); }

    // All sessions on one date.
    std::vector<const WorkoutSession*> sessionsOn(const std::string& isoDate) const;

    // Calories burned across all sessions on one date (0 if there are none).
    // This is the "calories out" number for that day.
    double caloriesBurnedOn(const std::string& isoDate, double bodyWeightKg) const;

private:
    std::vector<WorkoutSession> sessions_; // always sorted with WorkoutSession::operator<
};
