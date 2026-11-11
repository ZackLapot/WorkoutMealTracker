#pragma once

#include <cstddef>
#include <string>

// Small input-checking helpers shared by the core classes and the GUI.
//
// All functions throw ValidationError with a message that can be shown to the
// user as-is (for example "Age must be between 10 and 120 years.").
namespace validation {

// ---- text helpers ---------------------------------------------------------

// Removes leading and trailing whitespace.
std::string trim(const std::string& text);

// Lower-cases ASCII letters only (A-Z). Other bytes, including UTF-8
// sequences, are left untouched so names with accents are never corrupted.
std::string toLower(std::string text);

// Formats a number with at most maxDecimals decimals and no trailing zeros
// (60 -> "60", 62.5 -> "62.5"). Always uses '.' as the decimal separator.
std::string formatNumber(double value, int maxDecimals = 2);

// ---- parsing text typed by the user ----------------------------------------

// Parses a whole number. `label` names the field in the error message.
int parseInt(const std::string& text, const std::string& label);

// Parses a decimal number (finite values only).
double parseDouble(const std::string& text, const std::string& label);

// ---- range and content checks ----------------------------------------------

// Returns the trimmed text, or throws if it is empty, too long or contains
// control characters.
std::string requireText(const std::string& raw, const std::string& label,
                        std::size_t maxLength = 60);

// Returns value if min <= value <= max, otherwise throws.
// `unit` (for example "kg") is only used in the error message.
int requireInt(int value, int min, int max, const std::string& label,
               const std::string& unit = "");
double requireDouble(double value, double min, double max, const std::string& label,
                     const std::string& unit = "");

// Body weight used in calorie calculations (see limits::kMinWeightKg/kMaxWeightKg).
double requireBodyWeight(double weightKg);

// ---- dates -----------------------------------------------------------------

// True for a real calendar date written exactly as YYYY-MM-DD (years 1900-2100).
bool isValidIsoDate(const std::string& text);

// Returns the date unchanged if valid, otherwise throws.
std::string requireIsoDate(const std::string& text, const std::string& label = "Date");

} // namespace validation
