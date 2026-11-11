// Unit tests for ExerciseLibrary (add, find, remove, search by name and category).

#include "TestUtils.h"
#include "core/CardioExercise.h"
#include "core/Errors.h"
#include "core/ExerciseLibrary.h"
#include "core/StrengthExercise.h"

#include <memory>

namespace {

// A small library used by most tests.
ExerciseLibrary makeSmallLibrary()
{
    ExerciseLibrary library;
    library.add(std::make_unique<StrengthExercise>("Bench Press", Category::Chest, 5.0));
    library.add(std::make_unique<StrengthExercise>("Overhead Press", Category::Shoulders, 5.0));
    library.add(std::make_unique<StrengthExercise>("Squat", Category::Legs, 6.0));
    library.add(std::make_unique<CardioExercise>("Running", 9.8));
    return library;
}

QStringList names(const std::vector<ExerciseLibrary::ExercisePtr>& list)
{
    QStringList result;
    for (const auto& exercise : list) {
        result << qs(exercise->name());
    }
    return result;
}

} // namespace

class TstLibrary : public QObject {
    Q_OBJECT

private slots:
    void emptyLibraryBehavesSensibly();
    void addAndFindIgnoreCase();
    void duplicateNameIsRejected();
    void nullExerciseIsRejected();
    void removeWorksOnceOnly();

    void searchByNameSubstring();
    void searchIgnoresCaseAndSpaces();
    void searchByCategory();
    void searchByNameAndCategoryTogether();
    void searchWithNoMatchReturnsEmpty();
    void emptyQueryReturnsEverythingSorted();

    void cloneFromLibraryDoesNotChangeTemplate();

    void builtInDefaultsAreComplete();
    void builtInDefaultSearches();
};

void TstLibrary::emptyLibraryBehavesSensibly()
{
    const ExerciseLibrary library;
    QVERIFY(library.empty());
    QCOMPARE(static_cast<int>(library.size()), 0);
    QVERIFY(library.search("anything").empty());
    QVERIFY(library.all().empty());
    QVERIFY(library.find("Bench Press") == nullptr);
}

void TstLibrary::addAndFindIgnoreCase()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QCOMPARE(static_cast<int>(library.size()), 4);

    const auto bench = library.find("bench PRESS");
    QVERIFY(bench != nullptr);
    QCOMPARE(qs(bench->name()), QStringLiteral("Bench Press"));
    QVERIFY(library.find("  Squat ") != nullptr);
    QVERIFY(library.find("Deadlift") == nullptr);
}

void TstLibrary::duplicateNameIsRejected()
{
    ExerciseLibrary library = makeSmallLibrary();
    EXPECT_THROW_MSG(DuplicateEntryError,
                     library.add(std::make_unique<StrengthExercise>("bench press", Category::Chest, 4.0)),
                     "already contains an exercise named 'bench press'");
    // a cardio exercise with the same name is also a duplicate
    EXPECT_THROW_MSG(DuplicateEntryError, library.add(std::make_unique<CardioExercise>("RUNNING", 7.0)),
                     "already contains");
    QCOMPARE(static_cast<int>(library.size()), 4);
    // the original entry is untouched
    QCOMPARE(library.find("Bench Press")->met(), 5.0);
}

void TstLibrary::nullExerciseIsRejected()
{
    ExerciseLibrary library;
    EXPECT_THROW_MSG(ValidationError, library.add(nullptr), "empty exercise");
    QVERIFY(library.empty());
}

void TstLibrary::removeWorksOnceOnly()
{
    ExerciseLibrary library = makeSmallLibrary();
    QVERIFY(library.remove("squat"));
    QVERIFY(library.find("Squat") == nullptr);
    QCOMPARE(static_cast<int>(library.size()), 3);
    QVERIFY(!library.remove("Squat")); // already gone
    QVERIFY(!library.remove("Never existed"));
}

void TstLibrary::searchByNameSubstring()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QCOMPARE(names(library.search("press")),
             (QStringList{"Bench Press", "Overhead Press"}));
    QCOMPARE(names(library.search("run")), (QStringList{"Running"}));
    QCOMPARE(names(library.search("s")),
             (QStringList{"Bench Press", "Overhead Press", "Squat"}));
}

void TstLibrary::searchIgnoresCaseAndSpaces()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QCOMPARE(names(library.search("  PRESS  ")),
             (QStringList{"Bench Press", "Overhead Press"}));
    QCOMPARE(names(library.search("bench p")), (QStringList{"Bench Press"}));
}

void TstLibrary::searchByCategory()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QCOMPARE(names(library.search("", Category::Chest)), (QStringList{"Bench Press"}));
    QCOMPARE(names(library.search("", Category::Cardio)), (QStringList{"Running"}));
    QVERIFY(library.search("", Category::Core).empty());
}

void TstLibrary::searchByNameAndCategoryTogether()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QCOMPARE(names(library.search("press", Category::Shoulders)),
             (QStringList{"Overhead Press"}));
    // name matches but the category does not
    QVERIFY(library.search("press", Category::Legs).empty());
}

void TstLibrary::searchWithNoMatchReturnsEmpty()
{
    const ExerciseLibrary library = makeSmallLibrary();
    QVERIFY(library.search("zzz").empty());
    QVERIFY(library.search("squat", Category::Chest).empty());
}

void TstLibrary::emptyQueryReturnsEverythingSorted()
{
    const ExerciseLibrary library = makeSmallLibrary();
    const QStringList expected{"Bench Press", "Overhead Press", "Running", "Squat"};
    QCOMPARE(names(library.search("")), expected);
    QCOMPARE(names(library.search("   ")), expected);
    QCOMPARE(names(library.all()), expected);
}

void TstLibrary::cloneFromLibraryDoesNotChangeTemplate()
{
    const ExerciseLibrary library = makeSmallLibrary();
    const auto bench = library.find("Bench Press");

    std::unique_ptr<Exercise> forWorkout = bench->clone();
    auto* strength = dynamic_cast<StrengthExercise*>(forWorkout.get());
    QVERIFY(strength != nullptr);
    strength->addSet(10, 60.0);

    // the template in the library still has nothing logged
    QCOMPARE(qs(bench->describe()), QStringLiteral("No sets logged"));
    QCOMPARE(qs(forWorkout->describe()), QStringLiteral("1 set: 10 x 60 kg"));
}

void TstLibrary::builtInDefaultsAreComplete()
{
    const ExerciseLibrary library = ExerciseLibrary::builtInDefaults();
    QCOMPARE(static_cast<int>(library.size()), 38);

    // every category has at least one exercise
    for (Category category : allCategories()) {
        QVERIFY2(!library.search("", category).empty(),
                 qPrintable(qs(categoryToString(category)) + " has no exercises"));
    }

    // cardio exercises are CardioExercise objects, the rest are StrengthExercise
    for (const auto& exercise : library.all()) {
        if (exercise->category() == Category::Cardio) {
            QVERIFY(dynamic_cast<const CardioExercise*>(exercise.get()) != nullptr);
        } else {
            QVERIFY(dynamic_cast<const StrengthExercise*>(exercise.get()) != nullptr);
        }
    }
}

void TstLibrary::builtInDefaultSearches()
{
    const ExerciseLibrary library = ExerciseLibrary::builtInDefaults();
    QCOMPARE(names(library.search("press")),
             (QStringList{"Bench Press", "Incline Dumbbell Press", "Leg Press", "Overhead Press"}));
    QCOMPARE(static_cast<int>(library.search("", Category::Cardio).size()), 12);
    QCOMPARE(static_cast<int>(library.search("", Category::Legs).size()), 6);
    QCOMPARE(library.find("Running")->met(), 9.8);
}

QTEST_APPLESS_MAIN(TstLibrary)
#include "tst_library.moc"
