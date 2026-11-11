// Unit tests for UserProfile.

#include "TestUtils.h"
#include "core/Errors.h"
#include "core/Limits.h"
#include "core/UserProfile.h"

#include <cmath>

class TstProfile : public QObject {
    Q_OBJECT

private slots:
    void defaultProfileIsValid();
    void constructsFromValidValues();
    void nameIsTrimmed();
    void boundariesAreAccepted();
    void invalidValuesAreRejected_data();
    void invalidValuesAreRejected();
    void settersValidateAndKeepOldValue();
    void bmiIsCalculated();
};

void TstProfile::defaultProfileIsValid()
{
    const UserProfile profile;
    QCOMPARE(qs(profile.name()), QStringLiteral("User"));
    // the defaults must satisfy the same rules as user input
    UserProfile copy(profile.name(), profile.age(), profile.heightCm(), profile.weightKg(),
                     profile.dailyCalorieGoal());
    QCOMPARE(copy.weightKg(), 70.0);
}

void TstProfile::constructsFromValidValues()
{
    const UserProfile profile("Demo User", 28, 175.0, 72.5, 2200);
    QCOMPARE(qs(profile.name()), QStringLiteral("Demo User"));
    QCOMPARE(profile.age(), 28);
    QCOMPARE(profile.heightCm(), 175.0);
    QCOMPARE(profile.weightKg(), 72.5);
    QCOMPARE(profile.dailyCalorieGoal(), 2200);
}

void TstProfile::nameIsTrimmed()
{
    const UserProfile profile("  Demo User  ", 28, 175.0, 72.5, 2200);
    QCOMPARE(qs(profile.name()), QStringLiteral("Demo User"));
}

void TstProfile::boundariesAreAccepted()
{
    UserProfile lowest("A", limits::kMinAgeYears, limits::kMinHeightCm, limits::kMinWeightKg,
                       limits::kMinCalorieGoal);
    UserProfile highest(std::string(limits::kMaxNameLength, 'x'), limits::kMaxAgeYears,
                        limits::kMaxHeightCm, limits::kMaxWeightKg, limits::kMaxCalorieGoal);
    QCOMPARE(lowest.age(), 10);
    QCOMPARE(highest.dailyCalorieGoal(), 10000);
}

void TstProfile::invalidValuesAreRejected_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("age");
    QTest::addColumn<double>("heightCm");
    QTest::addColumn<double>("weightKg");
    QTest::addColumn<int>("goal");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("blank name") << "  " << 28 << 175.0 << 72.0 << 2200 << "Name is required.";
    QTest::newRow("age too low") << "A" << 9 << 175.0 << 72.0 << 2200
                                 << "Age must be between 10 and 120 years.";
    QTest::newRow("age too high") << "A" << 121 << 175.0 << 72.0 << 2200
                                  << "Age must be between 10 and 120 years.";
    QTest::newRow("height too low") << "A" << 28 << 99.9 << 72.0 << 2200
                                    << "Height must be between 100 and 250 cm.";
    QTest::newRow("height too high") << "A" << 28 << 250.1 << 72.0 << 2200
                                     << "Height must be between 100 and 250 cm.";
    QTest::newRow("weight too low") << "A" << 28 << 175.0 << 19.9 << 2200
                                    << "Weight must be between 20 and 400 kg.";
    QTest::newRow("negative weight") << "A" << 28 << 175.0 << -72.0 << 2200
                                     << "Weight must be between 20 and 400 kg.";
    QTest::newRow("weight is NaN") << "A" << 28 << 175.0 << std::nan("") << 2200
                                   << "Weight must be between 20 and 400 kg.";
    QTest::newRow("goal too low") << "A" << 28 << 175.0 << 72.0 << 799
                                  << "Daily calorie goal must be between 800 and 10000 kcal.";
    QTest::newRow("goal too high") << "A" << 28 << 175.0 << 72.0 << 10001
                                   << "Daily calorie goal must be between 800 and 10000 kcal.";
}

void TstProfile::invalidValuesAreRejected()
{
    QFETCH(QString, name);
    QFETCH(int, age);
    QFETCH(double, heightCm);
    QFETCH(double, weightKg);
    QFETCH(int, goal);
    QFETCH(QString, expectedMessage);

    try {
        UserProfile profile(name.toStdString(), age, heightCm, weightKg, goal);
        QFAIL("expected ValidationError");
    } catch (const ValidationError& e) {
        QCOMPARE(qs(e.what()), expectedMessage);
    }
}

void TstProfile::settersValidateAndKeepOldValue()
{
    UserProfile profile("Demo User", 28, 175.0, 72.5, 2200);

    EXPECT_THROW_MSG(ValidationError, profile.setWeightKg(5.0), "Weight must be between");
    QCOMPARE(profile.weightKg(), 72.5);

    EXPECT_THROW_MSG(ValidationError, profile.setAge(200), "Age must be between");
    QCOMPARE(profile.age(), 28);

    EXPECT_THROW_MSG(ValidationError, profile.setName(""), "Name is required.");
    QCOMPARE(qs(profile.name()), QStringLiteral("Demo User"));

    EXPECT_THROW_MSG(ValidationError, profile.setHeightCm(10.0), "Height must be between");
    EXPECT_THROW_MSG(ValidationError, profile.setDailyCalorieGoal(100), "Daily calorie goal");
    QCOMPARE(profile.heightCm(), 175.0);
    QCOMPARE(profile.dailyCalorieGoal(), 2200);

    profile.setWeightKg(80.0);
    profile.setAge(30);
    profile.setName("Renamed");
    profile.setHeightCm(180.0);
    profile.setDailyCalorieGoal(2500);
    QCOMPARE(profile.weightKg(), 80.0);
    QCOMPARE(profile.age(), 30);
    QCOMPARE(qs(profile.name()), QStringLiteral("Renamed"));
    QCOMPARE(profile.heightCm(), 180.0);
    QCOMPARE(profile.dailyCalorieGoal(), 2500);
}

void TstProfile::bmiIsCalculated()
{
    // 70 / 1.75^2 = 22.857...
    const UserProfile profile("Demo User", 28, 175.0, 70.0, 2200);
    QVERIFY_CLOSE(profile.bmi(), 22.8571, 0.001);
}

QTEST_APPLESS_MAIN(TstProfile)
#include "tst_profile.moc"
