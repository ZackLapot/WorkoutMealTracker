// Unit tests for Exercise, StrengthExercise, CardioExercise and Category.

#include "TestUtils.h"
#include "core/CardioExercise.h"
#include "core/Errors.h"
#include "core/Exercise.h"
#include "core/Limits.h"
#include "core/StrengthExercise.h"

#include <cmath>
#include <memory>
#include <vector>

class TstExercises : public QObject {
    Q_OBJECT

private slots:
    // categories
    void categoryToStringAndBack();
    void categoryFromStringIsCaseInsensitive();
    void categoryFromStringRejectsUnknown();
    void allCategoriesAreUnique();

    // construction rules
    void exerciseNameIsTrimmedAndChecked();
    void metMustBeInRange();
    void strengthCannotBeCardioCategory();
    void cardioAlwaysHasCardioCategory();

    // strength sets
    void strengthSetMath();
    void strengthSetLimits();
    void strengthMaxSets();
    void strengthRemoveAndClearSets();
    void strengthDescribe();

    // calories
    void strengthCalories();
    void cardioCalories();
    void caloriesScaleWithBodyWeight();
    void noLoggedDataMeansZeroCalories();
    void caloriesRejectInvalidBodyWeight();

    // cardio
    void cardioLimits();
    void cardioSpeedAndDescribe();
    void cardioLogSessionReplacesOldValues();

    // polymorphism / cloning
    void cloneKeepsTypeAndIsIndependent();
    void baseClassPointerUsesOverriddenBehaviour();
};

// ---- categories ---------------------------------------------------------------

void TstExercises::categoryToStringAndBack()
{
    for (Category category : allCategories()) {
        QCOMPARE(categoryFromString(categoryToString(category)), category);
    }
    QCOMPARE(qs(categoryToString(Category::Shoulders)), QStringLiteral("Shoulders"));
}

void TstExercises::categoryFromStringIsCaseInsensitive()
{
    QCOMPARE(categoryFromString("chest"), Category::Chest);
    QCOMPARE(categoryFromString("  CARDIO "), Category::Cardio);
    QCOMPARE(categoryFromString("LeGs"), Category::Legs);
}

void TstExercises::categoryFromStringRejectsUnknown()
{
    EXPECT_THROW_MSG(ValidationError, categoryFromString("Wings"), "Unknown category 'Wings'.");
    EXPECT_THROW_MSG(ValidationError, categoryFromString(""), "Unknown category");
}

void TstExercises::allCategoriesAreUnique()
{
    const std::vector<Category>& categories = allCategories();
    QCOMPARE(static_cast<int>(categories.size()), 7);
    for (std::size_t i = 0; i < categories.size(); ++i) {
        for (std::size_t j = i + 1; j < categories.size(); ++j) {
            QVERIFY(categories[i] != categories[j]);
        }
    }
}

// ---- construction rules -----------------------------------------------------------

void TstExercises::exerciseNameIsTrimmedAndChecked()
{
    StrengthExercise ok("  Bench Press  ", Category::Chest, 5.0);
    QCOMPARE(qs(ok.name()), QStringLiteral("Bench Press"));

    EXPECT_THROW_MSG(ValidationError, StrengthExercise("", Category::Chest, 5.0),
                     "Exercise name is required.");
    EXPECT_THROW_MSG(ValidationError, StrengthExercise("   ", Category::Chest, 5.0),
                     "Exercise name is required.");
    EXPECT_THROW_MSG(ValidationError, StrengthExercise(std::string(61, 'x'), Category::Chest, 5.0),
                     "at most 60 characters");
    EXPECT_THROW_MSG(ValidationError, CardioExercise("", 7.0), "Exercise name is required.");
}

void TstExercises::metMustBeInRange()
{
    StrengthExercise low("Low", Category::Core, limits::kMinMet);   // boundary is allowed
    StrengthExercise high("High", Category::Core, limits::kMaxMet); // boundary is allowed
    EXPECT_THROW_MSG(ValidationError, StrengthExercise("X", Category::Core, 0.4),
                     "MET value must be between 0.5 and 20.");
    EXPECT_THROW_MSG(ValidationError, StrengthExercise("X", Category::Core, 20.1),
                     "MET value must be between 0.5 and 20.");
    EXPECT_THROW_MSG(ValidationError, CardioExercise("X", std::nan("")), "MET value");
}

void TstExercises::strengthCannotBeCardioCategory()
{
    EXPECT_THROW_MSG(ValidationError, StrengthExercise("Jumping", Category::Cardio, 5.0),
                     "Strength exercises cannot use the Cardio category.");
}

void TstExercises::cardioAlwaysHasCardioCategory()
{
    CardioExercise run("Running", 9.8);
    QCOMPARE(run.category(), Category::Cardio);
    QCOMPARE(qs(run.typeName()), QStringLiteral("cardio"));
}

// ---- strength sets ----------------------------------------------------------------

void TstExercises::strengthSetMath()
{
    StrengthExercise bench("Bench Press", Category::Chest, 5.0);
    bench.addSet(10, 60.0);
    bench.addSet(10, 60.0);
    bench.addSet(8, 62.5);

    QCOMPARE(static_cast<int>(bench.setCount()), 3);
    QCOMPARE(bench.totalReps(), 28);
    QCOMPARE(bench.totalVolumeKg(), 10 * 60.0 + 10 * 60.0 + 8 * 62.5); // 1700
    // 28 reps x 4 s + 3 sets x 60 s = 292 s
    QCOMPARE(bench.activeSeconds(), 292.0);
    QCOMPARE(qs(bench.typeName()), QStringLiteral("strength"));
}

void TstExercises::strengthSetLimits()
{
    StrengthExercise ex("Squat", Category::Legs, 6.0);

    // boundaries that are allowed
    ex.addSet(limits::kMinReps, limits::kMinSetWeightKg);
    ex.addSet(limits::kMaxReps, limits::kMaxSetWeightKg);
    QCOMPARE(static_cast<int>(ex.setCount()), 2);

    // invalid values must not change the exercise
    EXPECT_THROW_MSG(ValidationError, ex.addSet(0, 50.0), "Reps must be between 1 and 100.");
    EXPECT_THROW_MSG(ValidationError, ex.addSet(101, 50.0), "Reps must be between 1 and 100.");
    EXPECT_THROW_MSG(ValidationError, ex.addSet(-3, 50.0), "Reps must be between 1 and 100.");
    EXPECT_THROW_MSG(ValidationError, ex.addSet(5, -0.5), "Weight must be between 0 and 1000 kg.");
    EXPECT_THROW_MSG(ValidationError, ex.addSet(5, 1000.5), "Weight must be between 0 and 1000 kg.");
    EXPECT_THROW_MSG(ValidationError, ex.addSet(5, std::nan("")), "Weight must be between");
    QCOMPARE(static_cast<int>(ex.setCount()), 2);
}

void TstExercises::strengthMaxSets()
{
    StrengthExercise ex("Curl", Category::Arms, 3.5);
    for (std::size_t i = 0; i < limits::kMaxSetsPerExercise; ++i) {
        ex.addSet(10, 10.0);
    }
    EXPECT_THROW_MSG(ValidationError, ex.addSet(10, 10.0), "at most 50 sets");
    QCOMPARE(static_cast<int>(ex.setCount()), 50);
}

void TstExercises::strengthRemoveAndClearSets()
{
    StrengthExercise ex("Row", Category::Back, 5.0);
    QVERIFY(!ex.removeLastSet()); // nothing to remove

    ex.addSet(10, 40.0);
    ex.addSet(8, 45.0);
    QVERIFY(ex.removeLastSet());
    QCOMPARE(static_cast<int>(ex.setCount()), 1);
    QCOMPARE(ex.sets().back().reps, 10);

    ex.clearSets();
    QCOMPARE(static_cast<int>(ex.setCount()), 0);
    QCOMPARE(ex.totalReps(), 0);
}

void TstExercises::strengthDescribe()
{
    StrengthExercise ex("Bench Press", Category::Chest, 5.0);
    QCOMPARE(qs(ex.describe()), QStringLiteral("No sets logged"));

    ex.addSet(10, 60.0);
    QCOMPARE(qs(ex.describe()), QStringLiteral("1 set: 10 x 60 kg"));

    ex.addSet(8, 62.5);
    ex.addSet(12, 0.0);
    QCOMPARE(qs(ex.describe()),
             QStringLiteral("3 sets: 10 x 60 kg, 8 x 62.5 kg, 12 x bodyweight"));
}

// ---- calories ---------------------------------------------------------------------

void TstExercises::strengthCalories()
{
    // 3 sets of 10 reps: 30 x 4 s + 3 x 60 s = 300 s = 1/12 h.
    // kcal = MET 5.0 x 70 kg x 1/12 h = 29.1667
    StrengthExercise bench("Bench Press", Category::Chest, 5.0);
    for (int i = 0; i < 3; ++i) {
        bench.addSet(10, 60.0);
    }
    QVERIFY_CLOSE(bench.caloriesBurned(70.0), 29.1667, 0.001);
}

void TstExercises::cardioCalories()
{
    // kcal = MET 9.8 x 70 kg x 0.5 h = 343.0
    CardioExercise run("Running", 9.8);
    run.logSession(30.0, 5.0);
    QVERIFY_CLOSE(run.caloriesBurned(70.0), 343.0, 1e-9);

    // 45 minutes of cycling at MET 8.0, 82.5 kg: 8 x 82.5 x 0.75 = 495
    CardioExercise bike("Cycling", 8.0);
    bike.logSession(45.0);
    QVERIFY_CLOSE(bike.caloriesBurned(82.5), 495.0, 1e-9);
}

void TstExercises::caloriesScaleWithBodyWeight()
{
    CardioExercise run("Running", 9.8);
    run.logSession(30.0);
    const double light = run.caloriesBurned(60.0);
    const double heavy = run.caloriesBurned(120.0);
    QVERIFY_CLOSE(heavy, 2.0 * light, 1e-9);
}

void TstExercises::noLoggedDataMeansZeroCalories()
{
    StrengthExercise bench("Bench Press", Category::Chest, 5.0);
    CardioExercise run("Running", 9.8);
    QCOMPARE(bench.caloriesBurned(70.0), 0.0);
    QCOMPARE(run.caloriesBurned(70.0), 0.0);
}

void TstExercises::caloriesRejectInvalidBodyWeight()
{
    CardioExercise run("Running", 9.8);
    run.logSession(30.0);
    EXPECT_THROW_MSG(ValidationError, run.caloriesBurned(0.0), "Body weight must be between 20 and 400 kg.");
    EXPECT_THROW_MSG(ValidationError, run.caloriesBurned(19.9), "Body weight");
    EXPECT_THROW_MSG(ValidationError, run.caloriesBurned(400.1), "Body weight");
    EXPECT_THROW_MSG(ValidationError, run.caloriesBurned(-70.0), "Body weight");
    EXPECT_THROW_MSG(ValidationError, run.caloriesBurned(std::nan("")), "Body weight");

    // even an exercise with nothing logged refuses a nonsense body weight
    StrengthExercise bench("Bench Press", Category::Chest, 5.0);
    EXPECT_THROW_MSG(ValidationError, bench.caloriesBurned(0.0), "Body weight");
}

// ---- cardio -----------------------------------------------------------------------

void TstExercises::cardioLimits()
{
    CardioExercise run("Running", 9.8);

    run.logSession(limits::kMinCardioMinutes, limits::kMinDistanceKm);   // boundaries allowed
    run.logSession(limits::kMaxCardioMinutes, limits::kMaxDistanceKm);

    EXPECT_THROW_MSG(ValidationError, run.logSession(0.0), "Duration must be between 1 and 600 minutes.");
    EXPECT_THROW_MSG(ValidationError, run.logSession(0.5), "Duration must be between 1 and 600 minutes.");
    EXPECT_THROW_MSG(ValidationError, run.logSession(600.1), "Duration must be between 1 and 600 minutes.");
    EXPECT_THROW_MSG(ValidationError, run.logSession(-5.0), "Duration");
    EXPECT_THROW_MSG(ValidationError, run.logSession(30.0, -1.0), "Distance must be between 0 and 500 km.");
    EXPECT_THROW_MSG(ValidationError, run.logSession(30.0, 500.1), "Distance must be between 0 and 500 km.");

    // a rejected call leaves the previous valid values in place
    QCOMPARE(run.durationMin(), limits::kMaxCardioMinutes);
}

void TstExercises::cardioSpeedAndDescribe()
{
    CardioExercise run("Running", 9.8);
    QVERIFY(!run.isLogged());
    QCOMPARE(qs(run.describe()), QStringLiteral("Not logged"));
    QCOMPARE(run.averageSpeedKmh(), 0.0);

    run.logSession(30.0, 5.0);
    QVERIFY(run.isLogged());
    QCOMPARE(run.averageSpeedKmh(), 10.0);
    QCOMPARE(qs(run.describe()), QStringLiteral("30 min, 5 km (10 km/h)"));

    run.logSession(45.5, 8.0);
    QCOMPARE(qs(run.describe()), QStringLiteral("45.5 min, 8 km (10.5 km/h)"));

    run.logSession(20.0); // no distance recorded
    QCOMPARE(qs(run.describe()), QStringLiteral("20 min"));
    QCOMPARE(run.averageSpeedKmh(), 0.0);
}

void TstExercises::cardioLogSessionReplacesOldValues()
{
    CardioExercise run("Running", 9.8);
    run.logSession(30.0, 5.0);
    run.logSession(10.0);
    QCOMPARE(run.durationMin(), 10.0);
    QCOMPARE(run.distanceKm(), 0.0);

    run.clearSession();
    QVERIFY(!run.isLogged());
}

// ---- polymorphism and cloning --------------------------------------------------------

void TstExercises::cloneKeepsTypeAndIsIndependent()
{
    StrengthExercise original("Bench Press", Category::Chest, 5.0);
    original.addSet(10, 60.0);

    std::unique_ptr<Exercise> copy = original.clone();
    QVERIFY(dynamic_cast<StrengthExercise*>(copy.get()) != nullptr); // still a StrengthExercise
    QCOMPARE(qs(copy->name()), QStringLiteral("Bench Press"));
    QCOMPARE(qs(copy->describe()), qs(original.describe()));

    // changing the copy must not change the original
    dynamic_cast<StrengthExercise*>(copy.get())->addSet(8, 65.0);
    QCOMPARE(static_cast<int>(original.setCount()), 1);

    CardioExercise run("Running", 9.8);
    run.logSession(30.0, 5.0);
    std::unique_ptr<Exercise> runCopy = run.clone();
    QVERIFY(dynamic_cast<CardioExercise*>(runCopy.get()) != nullptr);
    run.clearSession();
    QCOMPARE(qs(runCopy->describe()), QStringLiteral("30 min, 5 km (10 km/h)"));
}

void TstExercises::baseClassPointerUsesOverriddenBehaviour()
{
    std::vector<std::unique_ptr<Exercise>> workout;

    auto bench = std::make_unique<StrengthExercise>("Bench Press", Category::Chest, 5.0);
    for (int i = 0; i < 3; ++i) {
        bench->addSet(10, 60.0);
    }
    auto run = std::make_unique<CardioExercise>("Running", 9.8);
    run->logSession(30.0, 5.0);

    workout.push_back(std::move(bench));
    workout.push_back(std::move(run));

    // The loop only knows about Exercise; each call runs the subclass version.
    double total = 0.0;
    for (const auto& exercise : workout) {
        total += exercise->caloriesBurned(70.0);
    }
    QVERIFY_CLOSE(total, 29.1667 + 343.0, 0.001);
    QCOMPARE(qs(workout[0]->typeName()), QStringLiteral("strength"));
    QCOMPARE(qs(workout[1]->typeName()), QStringLiteral("cardio"));
}

QTEST_APPLESS_MAIN(TstExercises)
#include "tst_exercises.moc"
