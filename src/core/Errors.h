#pragma once

#include <stdexcept>
#include <string>

// Exception hierarchy used by the whole program.
//
// Core classes throw these when something is wrong; the GUI catches them and
// shows e.what() to the user, so every message is written for a human reader.

// Base class: catching TrackerError catches every error the program raises itself.
class TrackerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// A value is missing, malformed or outside its allowed range.
class ValidationError : public TrackerError {
public:
    using TrackerError::TrackerError;
};

// An item with the same identity already exists (same exercise, same workout, ...).
class DuplicateEntryError : public TrackerError {
public:
    using TrackerError::TrackerError;
};

// A data file could not be written (read problems are reported through LoadResult).
class DataFileError : public TrackerError {
public:
    using TrackerError::TrackerError;
};
