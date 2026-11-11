#include "Validation.h"

#include "Errors.h"

#include <charconv>
#include <climits>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <system_error>

namespace validation {

namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int daysInMonth(int year, int month)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

} // namespace

std::string trim(const std::string& text)
{
    const char* whitespace = " \t\r\n\f\v";
    const std::size_t first = text.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

std::string toLower(std::string text)
{
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return text;
}

std::string formatNumber(double value, int maxDecimals)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(maxDecimals) << value;
    std::string text = out.str();

    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    if (text == "-0") {
        text = "0";
    }
    return text;
}

int parseInt(const std::string& text, const std::string& label)
{
    const std::string t = trim(text);
    if (t.empty()) {
        throw ValidationError(label + " is required.");
    }

    long long value = 0;
    const char* first = t.data();
    const char* last = first + t.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec == std::errc::result_out_of_range || value < INT_MIN || value > INT_MAX) {
        throw ValidationError(label + " is too large or too small.");
    }
    if (ec != std::errc() || ptr != last) {
        throw ValidationError(label + " must be a whole number.");
    }
    return static_cast<int>(value);
}

double parseDouble(const std::string& text, const std::string& label)
{
    const std::string t = trim(text);
    if (t.empty()) {
        throw ValidationError(label + " is required.");
    }

    double value = 0.0;
    const char* first = t.data();
    const char* last = first + t.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec == std::errc::result_out_of_range) {
        throw ValidationError(label + " is too large or too small.");
    }
    if (ec != std::errc() || ptr != last || !std::isfinite(value)) {
        std::string message = label + " must be a number";
        if (t.find(',') != std::string::npos) {
            message += " (use a dot for decimals)";
        }
        throw ValidationError(message + ".");
    }
    return value;
}

std::string requireText(const std::string& raw, const std::string& label, std::size_t maxLength)
{
    const std::string text = trim(raw);
    if (text.empty()) {
        throw ValidationError(label + " is required.");
    }
    if (text.size() > maxLength) {
        throw ValidationError(label + " must be at most " + std::to_string(maxLength) +
                              " characters.");
    }
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7f) {
            throw ValidationError(label + " contains invalid characters.");
        }
    }
    return text;
}

int requireInt(int value, int min, int max, const std::string& label, const std::string& unit)
{
    if (value < min || value > max) {
        std::string message = label + " must be between " + std::to_string(min) + " and " +
                              std::to_string(max);
        if (!unit.empty()) {
            message += " " + unit;
        }
        throw ValidationError(message + ".");
    }
    return value;
}

double requireDouble(double value, double min, double max, const std::string& label,
                     const std::string& unit)
{
    if (!std::isfinite(value) || value < min || value > max) {
        std::string message = label + " must be between " + formatNumber(min) + " and " +
                              formatNumber(max);
        if (!unit.empty()) {
            message += " " + unit;
        }
        throw ValidationError(message + ".");
    }
    return value;
}

bool isValidIsoDate(const std::string& text)
{
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return false;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i != 4 && i != 7 && !isDigit(text[i])) {
            return false;
        }
    }

    const int year = std::stoi(text.substr(0, 4));
    const int month = std::stoi(text.substr(5, 2));
    const int day = std::stoi(text.substr(8, 2));

    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    return day <= daysInMonth(year, month);
}

std::string requireIsoDate(const std::string& text, const std::string& label)
{
    if (!isValidIsoDate(text)) {
        throw ValidationError(label + " must be a real date written as YYYY-MM-DD.");
    }
    return text;
}

} // namespace validation
