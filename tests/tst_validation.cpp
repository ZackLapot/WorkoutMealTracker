// Unit tests for the Validation helpers (parsing, ranges, dates, formatting).

#include "TestUtils.h"
#include "core/Errors.h"
#include "core/Validation.h"

#include <cmath>

class TstValidation : public QObject {
    Q_OBJECT

private slots:
    void trimRemovesSurroundingWhitespace();
    void toLowerOnlyTouchesAsciiLetters();

    void parseIntAcceptsWholeNumbers();
    void parseIntRejectsBadText_data();
    void parseIntRejectsBadText();

    void parseDoubleAcceptsDecimals();
    void parseDoubleRejectsBadText_data();
    void parseDoubleRejectsBadText();
    void parseDoubleSuggestsDotForComma();

    void requireIntChecksRange();
    void requireDoubleChecksRangeAndNaN();

    void requireTextTrimsAndChecks();

    void isValidIsoDateTable_data();
    void isValidIsoDateTable();
    void requireIsoDateMessage();

    void formatNumberTrimsZeros();
};

void TstValidation::trimRemovesSurroundingWhitespace()
{
    QCOMPARE(qs(validation::trim("  hello \t\n")), QStringLiteral("hello"));
    QCOMPARE(qs(validation::trim("a b")), QStringLiteral("a b"));
    QCOMPARE(qs(validation::trim("   ")), QStringLiteral(""));
    QCOMPARE(qs(validation::trim("")), QStringLiteral(""));
}

void TstValidation::toLowerOnlyTouchesAsciiLetters()
{
    QCOMPARE(qs(validation::toLower("Bench PRESS 2")), QStringLiteral("bench press 2"));
    // UTF-8 bytes of an accented letter must come back unchanged.
    const std::string accented = "Caf\xC3\x89";
    QCOMPARE(qs(validation::toLower(accented)), qs("caf\xC3\x89"));
}

void TstValidation::parseIntAcceptsWholeNumbers()
{
    QCOMPARE(validation::parseInt("42", "Age"), 42);
    QCOMPARE(validation::parseInt("  -7 ", "Age"), -7);
    QCOMPARE(validation::parseInt("0", "Age"), 0);
}

void TstValidation::parseIntRejectsBadText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("empty") << "" << "Reps is required.";
    QTest::newRow("spaces only") << "   " << "Reps is required.";
    QTest::newRow("letters") << "abc" << "Reps must be a whole number.";
    QTest::newRow("trailing text") << "12abc" << "Reps must be a whole number.";
    QTest::newRow("decimal") << "1.5" << "Reps must be a whole number.";
    QTest::newRow("plus sign") << "+5" << "Reps must be a whole number.";
    QTest::newRow("too big") << "99999999999999999999" << "Reps is too large or too small.";
}

void TstValidation::parseIntRejectsBadText()
{
    QFETCH(QString, text);
    QFETCH(QString, expectedMessage);
    try {
        validation::parseInt(text.toStdString(), "Reps");
        QFAIL("expected ValidationError");
    } catch (const ValidationError& e) {
        QCOMPARE(qs(e.what()), expectedMessage);
    }
}

void TstValidation::parseDoubleAcceptsDecimals()
{
    QCOMPARE(validation::parseDouble("72.5", "Weight"), 72.5);
    QCOMPARE(validation::parseDouble(" 0.5 ", "Weight"), 0.5);
    QCOMPARE(validation::parseDouble(".5", "Weight"), 0.5);
    QCOMPARE(validation::parseDouble("70", "Weight"), 70.0);
    QCOMPARE(validation::parseDouble("-3.25", "Weight"), -3.25);
}

void TstValidation::parseDoubleRejectsBadText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("empty") << "" << "Weight is required.";
    QTest::newRow("letters") << "abc" << "Weight must be a number.";
    QTest::newRow("unit typed") << "12kg" << "Weight must be a number.";
    QTest::newRow("two dots") << "1.2.3" << "Weight must be a number.";
    QTest::newRow("nan") << "nan" << "Weight must be a number.";
    QTest::newRow("inf") << "inf" << "Weight must be a number.";
    QTest::newRow("negative inf") << "-inf" << "Weight must be a number.";
    QTest::newRow("huge") << "1e999" << "Weight is too large or too small.";
}

void TstValidation::parseDoubleRejectsBadText()
{
    QFETCH(QString, text);
    QFETCH(QString, expectedMessage);
    try {
        validation::parseDouble(text.toStdString(), "Weight");
        QFAIL("expected ValidationError");
    } catch (const ValidationError& e) {
        QCOMPARE(qs(e.what()), expectedMessage);
    }
}

void TstValidation::parseDoubleSuggestsDotForComma()
{
    EXPECT_THROW_MSG(ValidationError, validation::parseDouble("72,5", "Weight"),
                     "use a dot for decimals");
}

void TstValidation::requireIntChecksRange()
{
    QCOMPARE(validation::requireInt(10, 10, 120, "Age", "years"), 10);   // lower boundary
    QCOMPARE(validation::requireInt(120, 10, 120, "Age", "years"), 120); // upper boundary
    EXPECT_THROW_MSG(ValidationError, validation::requireInt(9, 10, 120, "Age", "years"),
                     "Age must be between 10 and 120 years.");
    EXPECT_THROW_MSG(ValidationError, validation::requireInt(121, 10, 120, "Age", "years"),
                     "Age must be between 10 and 120 years.");
    EXPECT_THROW_MSG(ValidationError, validation::requireInt(-1, 0, 5, "Reps"),
                     "Reps must be between 0 and 5.");
}

void TstValidation::requireDoubleChecksRangeAndNaN()
{
    QCOMPARE(validation::requireDouble(20.0, 20.0, 400.0, "Weight", "kg"), 20.0);
    QCOMPARE(validation::requireDouble(400.0, 20.0, 400.0, "Weight", "kg"), 400.0);
    EXPECT_THROW_MSG(ValidationError, validation::requireDouble(19.99, 20.0, 400.0, "Weight", "kg"),
                     "Weight must be between 20 and 400 kg.");
    EXPECT_THROW_MSG(ValidationError, validation::requireDouble(400.01, 20.0, 400.0, "Weight", "kg"),
                     "Weight must be between 20 and 400 kg.");
    EXPECT_THROW_MSG(ValidationError,
                     validation::requireDouble(std::nan(""), 0.0, 10.0, "Distance", "km"),
                     "Distance must be between 0 and 10 km.");
}

void TstValidation::requireTextTrimsAndChecks()
{
    QCOMPARE(qs(validation::requireText("  Push Day  ", "Name")), QStringLiteral("Push Day"));
    EXPECT_THROW_MSG(ValidationError, validation::requireText("   ", "Name"), "Name is required.");
    EXPECT_THROW_MSG(ValidationError, validation::requireText(std::string(61, 'x'), "Name"),
                     "Name must be at most 60 characters.");
    QCOMPARE(qs(validation::requireText(std::string(60, 'x'), "Name")), qs(std::string(60, 'x')));
    EXPECT_THROW_MSG(ValidationError, validation::requireText("bad\nname", "Name"),
                     "Name contains invalid characters.");
}

void TstValidation::isValidIsoDateTable_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("valid");

    QTest::newRow("normal date") << "2026-11-10" << true;
    QTest::newRow("leap day 2024") << "2024-02-29" << true;
    QTest::newRow("leap day 2000") << "2000-02-29" << true;
    QTest::newRow("last day of month") << "2026-12-31" << true;
    QTest::newRow("not a leap year") << "2023-02-29" << false;
    QTest::newRow("century not leap") << "1900-02-29" << false;
    QTest::newRow("month 13") << "2026-13-01" << false;
    QTest::newRow("month 0") << "2026-00-10" << false;
    QTest::newRow("day 0") << "2026-05-00" << false;
    QTest::newRow("April has 30 days") << "2026-04-31" << false;
    QTest::newRow("missing zero padding") << "2026-4-1" << false;
    QTest::newRow("no dashes") << "20261110" << false;
    QTest::newRow("empty") << "" << false;
    QTest::newRow("letters") << "abcd-ef-gh" << false;
    QTest::newRow("trailing space") << "2026-11-10 " << false;
    QTest::newRow("year too early") << "1850-01-01" << false;
    QTest::newRow("year too late") << "2101-01-01" << false;
}

void TstValidation::isValidIsoDateTable()
{
    QFETCH(QString, text);
    QFETCH(bool, valid);
    QCOMPARE(validation::isValidIsoDate(text.toStdString()), valid);
}

void TstValidation::requireIsoDateMessage()
{
    QCOMPARE(qs(validation::requireIsoDate("2026-11-10")), QStringLiteral("2026-11-10"));
    EXPECT_THROW_MSG(ValidationError, validation::requireIsoDate("2026-02-30", "Workout date"),
                     "Workout date must be a real date written as YYYY-MM-DD.");
}

void TstValidation::formatNumberTrimsZeros()
{
    QCOMPARE(qs(validation::formatNumber(60.0)), QStringLiteral("60"));
    QCOMPARE(qs(validation::formatNumber(62.5)), QStringLiteral("62.5"));
    QCOMPARE(qs(validation::formatNumber(10.256)), QStringLiteral("10.26"));
    QCOMPARE(qs(validation::formatNumber(0.0)), QStringLiteral("0"));
    QCOMPARE(qs(validation::formatNumber(-0.0001)), QStringLiteral("0"));
    QCOMPARE(qs(validation::formatNumber(343.0, 1)), QStringLiteral("343"));
    QCOMPARE(qs(validation::formatNumber(29.1666, 1)), QStringLiteral("29.2"));
}

QTEST_APPLESS_MAIN(TstValidation)
#include "tst_validation.moc"
