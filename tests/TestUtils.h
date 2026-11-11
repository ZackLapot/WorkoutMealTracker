#pragma once

// Helpers shared by the QtTest unit tests.

#include <QString>
#include <QtTest>

#include <string>

inline QString qs(const std::string& text)
{
    return QString::fromStdString(text);
}

// Checks that `expr` throws ExType and that the message contains `needle`.
// Works with both Qt 5 and Qt 6 (QVERIFY_THROWS_EXCEPTION only exists in newer Qt 6).
#define EXPECT_THROW_MSG(ExType, expr, needle)                                              \
    do {                                                                                    \
        bool caughtExpected = false;                                                        \
        try {                                                                               \
            expr;                                                                           \
        } catch (const ExType& ex) {                                                        \
            caughtExpected = true;                                                          \
            QVERIFY2(qs(ex.what()).contains(QStringLiteral(needle)),                        \
                     qPrintable(QStringLiteral("message was: ") + qs(ex.what())));          \
        }                                                                                   \
        QVERIFY2(caughtExpected, "expected " #ExType " from " #expr);                       \
    } while (false)
