// Unit tests for DataManager: JSON round trips and handling of missing / damaged files.

#include "TestUtils.h"
#include "core/CardioExercise.h"
#include "core/DataManager.h"
#include "core/Errors.h"
#include "core/StrengthExercise.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <memory>

namespace {

void writeFile(const QString& path, const QByteArray& content)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(content);
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

std::string toStd(const QString& text) { return text.toStdString(); }

// A log with a strength workout, a cardio workout and an empty workout.
WorkoutLog makeSampleLog()
{
    WorkoutLog log;

    WorkoutSession push("Push Day", "2026-11-10");
    auto& bench = dynamic_cast<StrengthExercise&>(
        push.addExercise(std::make_unique<StrengthExercise>("Bench Press", Category::Chest, 5.0)));
    bench.addSet(10, 60.0);
    bench.addSet(8, 62.5);
    bench.addSet(12, 0.0);
    push.addExercise(std::make_unique<StrengthExercise>("Overhead Press", Category::Shoulders, 5.0));
    log.add(std::move(push));

    WorkoutSession cardio("Morning Run", "2026-11-09");
    auto& run = dynamic_cast<CardioExercise&>(
        cardio.addExercise(std::make_unique<CardioExercise>("Running", 9.8)));
    run.logSession(32.5, 5.25);
    cardio.addExercise(std::make_unique<CardioExercise>("Cycling", 8.0)); // nothing logged
    log.add(std::move(cardio));

    log.add(WorkoutSession("Rest Day", "2026-11-11"));
    return log;
}

} // namespace

class TstDataManager : public QObject {
    Q_OBJECT

private slots:
    // construction / paths
    void emptyDataFolderIsRejected();
    void pathsLiveInTheDataFolder();

    // profile
    void profileRoundTrip();
    void profileWithUnicodeNameRoundTrip();
    void profileMissingGivesDefaults();
    void profileSavedTwiceOverwrites();
    void profileGarbageIsDamaged();
    void profileEmptyFileIsDamaged();
    void profileWrongRootTypeIsDamaged();
    void profileBadVersionIsDamaged();
    void profileOutOfRangeValueIsDamaged();
    void profileWrongTypeOrMissingFieldIsDamaged_data();
    void profileWrongTypeOrMissingFieldIsDamaged();
    void secondDamagedFileReplacesOldBackup();

    // exercise library
    void libraryRoundTrip();
    void libraryMissingGivesBuiltInDefaults();
    void libraryGarbageGivesBuiltInDefaults();
    void libraryWithSomeBadEntriesKeepsTheGoodOnes();
    void libraryWithNoUsableEntriesIsDamaged();
    void libraryFileShippedInRepoMatchesBuiltIns();

    // workouts
    void workoutsRoundTrip();
    void workoutsMissingGivesEmptyLog();
    void workoutsGarbageGivesEmptyLog();
    void workoutsEmptyListIsFine();
    void workoutsWithSomeBadEntriesKeepsTheGoodOnes();

    // saving
    void saveCreatesMissingFolder();
    void saveLeavesNoTemporaryFiles();
    void saveIntoUnwritableLocationThrows();
};

// ---- construction / paths ------------------------------------------------------------------

void TstDataManager::emptyDataFolderIsRejected()
{
    EXPECT_THROW_MSG(ValidationError, DataManager(""), "Data folder is required.");
    EXPECT_THROW_MSG(ValidationError, DataManager("   "), "Data folder is required.");
}

void TstDataManager::pathsLiveInTheDataFolder()
{
    const DataManager manager("/tmp/some-folder");
    QCOMPARE(qs(manager.profilePath()), QStringLiteral("/tmp/some-folder/profile.json"));
    QCOMPARE(qs(manager.workoutsPath()), QStringLiteral("/tmp/some-folder/workouts.json"));
    QCOMPARE(qs(manager.libraryPath()), QStringLiteral("/tmp/some-folder/exercise_library.json"));
}

// ---- profile -----------------------------------------------------------------------------------

void TstDataManager::profileRoundTrip()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    manager.saveProfile(UserProfile("Demo User", 28, 175.5, 72.35, 2250));
    const auto loaded = manager.loadProfile();

    QVERIFY(loaded.ok());
    QVERIFY(loaded.message.empty());
    QCOMPARE(qs(loaded.value.name()), QStringLiteral("Demo User"));
    QCOMPARE(loaded.value.age(), 28);
    QCOMPARE(loaded.value.heightCm(), 175.5);
    QCOMPARE(loaded.value.weightKg(), 72.35); // exact: doubles survive the JSON round trip
    QCOMPARE(loaded.value.dailyCalorieGoal(), 2250);
}

void TstDataManager::profileWithUnicodeNameRoundTrip()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const std::string name = "Jos\xC3\xA9 \xC3\x91" "and\xC3\xBA";
    manager.saveProfile(UserProfile(name, 30, 170.0, 70.0, 2000));
    QCOMPARE(qs(manager.loadProfile().value.name()), qs(name));
}

void TstDataManager::profileMissingGivesDefaults()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    const auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Missing);
    QVERIFY(qs(loaded.message).contains("No profile found yet"));
    QCOMPARE(qs(loaded.value.name()), QStringLiteral("User")); // default profile
    QVERIFY(!QFile::exists(qs(manager.profilePath())));       // loading creates nothing
}

void TstDataManager::profileSavedTwiceOverwrites()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    manager.saveProfile(UserProfile("First", 30, 170.0, 70.0, 2000));
    manager.saveProfile(UserProfile("Second", 31, 171.0, 71.0, 2100));
    const auto loaded = manager.loadProfile();
    QVERIFY(loaded.ok());
    QCOMPARE(qs(loaded.value.name()), QStringLiteral("Second"));
}

void TstDataManager::profileGarbageIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const QString path = qs(manager.profilePath());
    writeFile(path, "this is { not json");

    const auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QCOMPARE(qs(loaded.value.name()), QStringLiteral("User")); // default profile in use
    QVERIFY(qs(loaded.message).contains("profile.json"));
    QVERIFY(qs(loaded.message).contains("profile.json.corrupt"));

    // the bad file was moved aside, not deleted, and not left in place
    QVERIFY(!QFile::exists(path));
    QCOMPARE(readFile(path + ".corrupt"), QByteArray("this is { not json"));

    // after the damage the program can save and load normally again
    manager.saveProfile(UserProfile("Fresh", 30, 170.0, 70.0, 2000));
    QVERIFY(manager.loadProfile().ok());
}

void TstDataManager::profileEmptyFileIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.profilePath()), QByteArray());

    QCOMPARE(manager.loadProfile().status, LoadStatus::Damaged);
    QVERIFY(QFile::exists(qs(manager.profilePath()) + ".corrupt"));
}

void TstDataManager::profileWrongRootTypeIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.profilePath()), "[1, 2, 3]");

    const auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY(qs(loaded.message).contains("not a JSON object"));
}

void TstDataManager::profileBadVersionIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const QByteArray body =
        R"("name":"A","age":30,"heightCm":170,"weightKg":70,"dailyCalorieGoal":2000)";

    writeFile(qs(manager.profilePath()), "{\"version\":99," + body + "}");
    auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY(qs(loaded.message).contains("format version"));

    writeFile(qs(manager.profilePath()), "{" + body + "}"); // no version at all
    QCOMPARE(manager.loadProfile().status, LoadStatus::Damaged);
}

void TstDataManager::profileOutOfRangeValueIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.profilePath()),
              R"({"version":1,"name":"A","age":30,"heightCm":170,"weightKg":5,"dailyCalorieGoal":2000})");

    const auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY2(qs(loaded.message).contains("Weight must be between 20 and 400 kg"),
             qPrintable(qs(loaded.message)));
}

void TstDataManager::profileWrongTypeOrMissingFieldIsDamaged_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("expectedInMessage");

    QTest::newRow("age is text")
        << QByteArray(R"({"version":1,"name":"A","age":"thirty","heightCm":170,"weightKg":70,"dailyCalorieGoal":2000})")
        << "field 'age'";
    QTest::newRow("age is fractional")
        << QByteArray(R"({"version":1,"name":"A","age":30.5,"heightCm":170,"weightKg":70,"dailyCalorieGoal":2000})")
        << "not a whole number";
    QTest::newRow("weight missing")
        << QByteArray(R"({"version":1,"name":"A","age":30,"heightCm":170,"dailyCalorieGoal":2000})")
        << "field 'weightKg'";
    QTest::newRow("name is null")
        << QByteArray(R"({"version":1,"name":null,"age":30,"heightCm":170,"weightKg":70,"dailyCalorieGoal":2000})")
        << "field 'name'";
}

void TstDataManager::profileWrongTypeOrMissingFieldIsDamaged()
{
    QFETCH(QByteArray, json);
    QFETCH(QString, expectedInMessage);

    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.profilePath()), json);

    const auto loaded = manager.loadProfile();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY2(qs(loaded.message).contains(expectedInMessage), qPrintable(qs(loaded.message)));
}

void TstDataManager::secondDamagedFileReplacesOldBackup()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const QString path = qs(manager.profilePath());

    writeFile(path, "first bad file");
    QCOMPARE(manager.loadProfile().status, LoadStatus::Damaged);
    writeFile(path, "second bad file");
    QCOMPARE(manager.loadProfile().status, LoadStatus::Damaged);

    QCOMPARE(readFile(path + ".corrupt"), QByteArray("second bad file"));
}

// ---- exercise library ---------------------------------------------------------------------------

void TstDataManager::libraryRoundTrip()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const ExerciseLibrary original = ExerciseLibrary::builtInDefaults();

    manager.saveLibrary(original);
    const auto loaded = manager.loadLibrary();

    QVERIFY(loaded.ok());
    QCOMPARE(static_cast<int>(loaded.value.size()), static_cast<int>(original.size()));
    const auto bench = loaded.value.find("Bench Press");
    QVERIFY(bench != nullptr);
    QCOMPARE(bench->met(), 5.0);
    QCOMPARE(bench->category(), Category::Chest);
    QVERIFY(dynamic_cast<const StrengthExercise*>(bench.get()) != nullptr);
    QVERIFY(dynamic_cast<const CardioExercise*>(loaded.value.find("Running").get()) != nullptr);
}

void TstDataManager::libraryMissingGivesBuiltInDefaults()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    const auto loaded = manager.loadLibrary();
    QCOMPARE(loaded.status, LoadStatus::Missing);
    QCOMPARE(static_cast<int>(loaded.value.size()),
             static_cast<int>(ExerciseLibrary::builtInDefaults().size()));
}

void TstDataManager::libraryGarbageGivesBuiltInDefaults()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.libraryPath()), "}}}}");

    const auto loaded = manager.loadLibrary();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY(qs(loaded.message).contains("exercise_library.json.corrupt"));
    QVERIFY(loaded.value.size() > 0);
    QVERIFY(QFile::exists(qs(manager.libraryPath()) + ".corrupt"));
}

void TstDataManager::libraryWithSomeBadEntriesKeepsTheGoodOnes()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.libraryPath()), R"({
      "version": 1,
      "exercises": [
        {"name":"Bench Press","type":"strength","category":"Chest","met":5.0},
        {"name":"Mystery","type":"yoga","category":"Core","met":2.0},
        {"name":"bench press","type":"strength","category":"Chest","met":4.0},
        {"name":"Bad Cardio","type":"cardio","category":"Chest","met":7.0},
        "just a string",
        {"name":"Running","type":"cardio","category":"Cardio","met":9.8}
      ]
    })");

    const auto loaded = manager.loadLibrary();
    QCOMPARE(loaded.status, LoadStatus::Partial);
    QCOMPARE(static_cast<int>(loaded.value.size()), 2); // Bench Press and Running
    QVERIFY(loaded.value.find("Bench Press") != nullptr);
    QCOMPARE(loaded.value.find("Bench Press")->met(), 5.0); // the first one wins
    QVERIFY(loaded.value.find("Running") != nullptr);
    QVERIFY2(qs(loaded.message).contains("Skipped 4 invalid exercise(s)"),
             qPrintable(qs(loaded.message)));
    QVERIFY(QFile::exists(qs(manager.libraryPath()))); // partial files are not moved aside
}

void TstDataManager::libraryWithNoUsableEntriesIsDamaged()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    writeFile(qs(manager.libraryPath()), R"({"version":1,"exercises":[]})");
    auto loaded = manager.loadLibrary();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY(qs(loaded.message).contains("no usable exercises"));
    QVERIFY(loaded.value.size() > 0); // built-in list in use

    writeFile(qs(manager.libraryPath()), R"({"version":1})"); // list missing entirely
    QCOMPARE(manager.loadLibrary().status, LoadStatus::Damaged);
}

void TstDataManager::libraryFileShippedInRepoMatchesBuiltIns()
{
    // data/exercise_library.json is the sample file shipped with the project
    QTemporaryDir dir;
    const QString shipped = QStringLiteral(WMT_SOURCE_DIR) + "/data/exercise_library.json";
    QVERIFY2(QFile::exists(shipped), qPrintable(shipped));
    QVERIFY(QFile::copy(shipped, dir.path() + "/exercise_library.json"));

    const DataManager manager(toStd(dir.path()));
    const auto loaded = manager.loadLibrary();
    QVERIFY2(loaded.ok(), qPrintable(qs(loaded.message)));

    const ExerciseLibrary expected = ExerciseLibrary::builtInDefaults();
    QCOMPARE(static_cast<int>(loaded.value.size()), static_cast<int>(expected.size()));
    for (const auto& exercise : expected.all()) {
        const auto match = loaded.value.find(exercise->name());
        QVERIFY2(match != nullptr, qPrintable(qs(exercise->name())));
        QCOMPARE(match->met(), exercise->met());
        QCOMPARE(match->category(), exercise->category());
        QCOMPARE(qs(match->typeName()), qs(exercise->typeName()));
    }
}

// ---- workouts -----------------------------------------------------------------------------------

void TstDataManager::workoutsRoundTrip()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    const WorkoutLog original = makeSampleLog();

    manager.saveWorkouts(original);
    const auto loaded = manager.loadWorkouts();

    QVERIFY(loaded.ok());
    QCOMPARE(static_cast<int>(loaded.value.size()), 3);
    for (std::size_t i = 0; i < original.size(); ++i) {
        const WorkoutSession& a = original.sessions()[i];
        const WorkoutSession& b = loaded.value.sessions()[i];
        QCOMPARE(qs(b.name()), qs(a.name()));
        QCOMPARE(qs(b.date()), qs(a.date()));
        QCOMPARE(static_cast<int>(b.exerciseCount()), static_cast<int>(a.exerciseCount()));
        for (std::size_t e = 0; e < a.exerciseCount(); ++e) {
            QCOMPARE(qs(b.exercises()[e]->typeName()), qs(a.exercises()[e]->typeName()));
            QCOMPARE(qs(b.exercises()[e]->describe()), qs(a.exercises()[e]->describe()));
        }
        QCOMPARE(b.totalCaloriesBurned(70.0), a.totalCaloriesBurned(70.0));
    }

    // spot-check the actual values that were stored
    const WorkoutSession* push = loaded.value.find("2026-11-10", "Push Day");
    QVERIFY(push != nullptr);
    QCOMPARE(qs(push->findExercise("Bench Press")->describe()),
             QStringLiteral("3 sets: 10 x 60 kg, 8 x 62.5 kg, 12 x bodyweight"));
    const WorkoutSession* run = loaded.value.find("2026-11-09", "Morning Run");
    QVERIFY(run != nullptr);
    QCOMPARE(qs(run->findExercise("Running")->describe()), QStringLiteral("32.5 min, 5.25 km (9.7 km/h)"));
    QCOMPARE(qs(run->findExercise("Cycling")->describe()), QStringLiteral("Not logged"));
}

void TstDataManager::workoutsMissingGivesEmptyLog()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    const auto loaded = manager.loadWorkouts();
    QCOMPARE(loaded.status, LoadStatus::Missing);
    QVERIFY(loaded.value.empty());
}

void TstDataManager::workoutsGarbageGivesEmptyLog()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.workoutsPath()), "\xff\xfe garbage \x01\x02");

    const auto loaded = manager.loadWorkouts();
    QCOMPARE(loaded.status, LoadStatus::Damaged);
    QVERIFY(loaded.value.empty());
    QVERIFY(qs(loaded.message).contains("workouts.json.corrupt"));
    QVERIFY(QFile::exists(qs(manager.workoutsPath()) + ".corrupt"));
}

void TstDataManager::workoutsEmptyListIsFine()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));

    manager.saveWorkouts(WorkoutLog());
    const auto loaded = manager.loadWorkouts();
    QVERIFY(loaded.ok());
    QVERIFY(loaded.value.empty());
}

void TstDataManager::workoutsWithSomeBadEntriesKeepsTheGoodOnes()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    writeFile(qs(manager.workoutsPath()), R"({
      "version": 1,
      "workouts": [
        {"name":"Good","date":"2026-11-10","exercises":[
          {"name":"Running","type":"cardio","category":"Cardio","met":9.8,"durationMin":30,"distanceKm":5}]},
        {"name":"Bad date","date":"2026-02-30","exercises":[]},
        {"name":"Bad reps","date":"2026-11-11","exercises":[
          {"name":"Squat","type":"strength","category":"Legs","met":6,"sets":[{"reps":0,"weightKg":50}]}]},
        {"name":"No exercise list","date":"2026-11-12"},
        {"name":"good","date":"2026-11-10","exercises":[]},
        42
      ]
    })");

    const auto loaded = manager.loadWorkouts();
    QCOMPARE(loaded.status, LoadStatus::Partial);
    QCOMPARE(static_cast<int>(loaded.value.size()), 1); // only "Good"; "good" is a duplicate
    QVERIFY(loaded.value.find("2026-11-10", "Good") != nullptr);
    QVERIFY2(qs(loaded.message).contains("Skipped 5 invalid workout(s)"), qPrintable(qs(loaded.message)));
    QVERIFY(QFile::exists(qs(manager.workoutsPath()))); // not moved aside
}

// ---- saving -------------------------------------------------------------------------------------

void TstDataManager::saveCreatesMissingFolder()
{
    QTemporaryDir dir;
    const QString nested = dir.path() + "/a/b/data";
    const DataManager manager(toStd(nested));

    manager.saveProfile(UserProfile());
    QVERIFY(QFile::exists(nested + "/profile.json"));
}

void TstDataManager::saveLeavesNoTemporaryFiles()
{
    QTemporaryDir dir;
    const DataManager manager(toStd(dir.path()));
    manager.saveProfile(UserProfile());
    manager.saveWorkouts(makeSampleLog());
    manager.saveLibrary(ExerciseLibrary::builtInDefaults());

    QStringList files = QDir(dir.path()).entryList(QDir::Files, QDir::Name);
    QCOMPARE(files, (QStringList{"exercise_library.json", "profile.json", "workouts.json"}));
}

void TstDataManager::saveIntoUnwritableLocationThrows()
{
    QTemporaryDir dir;
    // a regular file where the data folder should be makes the folder impossible to create
    const QString blocker = dir.path() + "/not-a-folder";
    writeFile(blocker, "x");
    const DataManager manager(toStd(blocker + "/data"));

    EXPECT_THROW_MSG(DataFileError, manager.saveProfile(UserProfile()), "Could not create the data folder");
    EXPECT_THROW_MSG(DataFileError, manager.saveWorkouts(WorkoutLog()), "Could not create the data folder");
    EXPECT_THROW_MSG(DataFileError, manager.saveLibrary(ExerciseLibrary()), "Could not create the data folder");
}

QTEST_APPLESS_MAIN(TstDataManager)
#include "tst_datamanager.moc"
