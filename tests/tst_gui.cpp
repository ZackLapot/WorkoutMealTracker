// GUI tests: drive the real MainWindow (offscreen) the way a user would - type into
// fields, click buttons - and check what the screen shows and what gets saved.

#include "TestUtils.h"
#include "core/DataManager.h"
#include "gui/MainWindow.h"
#include "gui/WorkoutsTab.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>

#include <memory>

namespace {

// A MainWindow working on a throw-away data folder.
struct Fixture {
    QTemporaryDir dir;
    std::unique_ptr<MainWindow> window;

    Fixture() { open(); }

    void open()
    {
        window = std::make_unique<MainWindow>(dir.path().toStdString());
        window->show();
    }
    void close() { window.reset(); }

    template <typename T>
    T* find(const char* objectName) const
    {
        return window->findChild<T*>(objectName);
    }

    QString path(const char* fileName) const { return dir.path() + "/" + fileName; }
};

void setProfileFields(const Fixture& f, const QString& name, const QString& age,
                      const QString& height, const QString& weight, const QString& goal)
{
    f.find<QLineEdit>("nameEdit")->setText(name);
    f.find<QLineEdit>("ageEdit")->setText(age);
    f.find<QLineEdit>("heightEdit")->setText(height);
    f.find<QLineEdit>("weightEdit")->setText(weight);
    f.find<QLineEdit>("goalEdit")->setText(goal);
}

// ---- Workouts tab helpers ----

void createWorkout(const Fixture& f, const QString& name, const QString& isoDate = "2026-11-10")
{
    f.find<QLineEdit>("workoutNameEdit")->setText(name);
    f.find<QDateEdit>("workoutDateEdit")->setDate(QDate::fromString(isoDate, Qt::ISODate));
    f.find<QPushButton>("createWorkoutButton")->click();
}

// Types `search` into the library filter, selects the first match and clicks "Add to workout".
void addFromLibrary(const Fixture& f, const QString& search)
{
    f.find<QLineEdit>("searchEdit")->setText(search);
    f.find<QListWidget>("libraryList")->setCurrentRow(0);
    f.find<QPushButton>("addExerciseButton")->click();
}

QString workoutsError(const Fixture& f)
{
    return f.find<QLabel>("workoutsError")->text();
}

QString cell(const Fixture& f, int row, int column)
{
    return f.find<QTableWidget>("exerciseTable")->item(row, column)->text();
}

void addSet(const Fixture& f, const QString& reps, const QString& weight)
{
    f.find<QLineEdit>("setRepsEdit")->setText(reps);
    f.find<QLineEdit>("setWeightEdit")->setText(weight);
    f.find<QPushButton>("addSetButton")->click();
}

void logCardio(const Fixture& f, const QString& minutes, const QString& km)
{
    f.find<QLineEdit>("cardioDurationEdit")->setText(minutes);
    f.find<QLineEdit>("cardioDistanceEdit")->setText(km);
    f.find<QPushButton>("logCardioButton")->click();
}

} // namespace

class TstGui : public QObject {
    Q_OBJECT

private slots:
    // start-up
    void startsWithEmptyDataFolder();
    void damagedProfileShowsWarningBanner();
    void bannerCanBeDismissed();

    // Profile tab
    void profileFieldsShowSavedProfile();
    void invalidProfileInputShowsMessageAndSavesNothing_data();
    void invalidProfileInputShowsMessageAndSavesNothing();
    void validProfileIsSavedAndShown();

    // Workouts tab: library, workouts, exercises
    void libraryListCanBeFilteredByNameAndCategory();
    void createWorkoutValidatesAndLists();
    void addAndRemoveExercises();
    void addingNeedsAWorkoutAndALibrarySelection();
    void removingNeedsASelection();
    void deleteWorkout();
    void switchingBetweenWorkoutsShowsTheirExercises();
    void workoutsSurviveARestart();

    // Workouts tab: logging sets and cardio
    void logPanelFollowsTheSelectedExercise();
    void strengthSetsAreLoggedWithCalories();
    void invalidSetInputShowsMessage_data();
    void invalidSetInputShowsMessage();
    void setLimitIsEnforced();
    void removeLastSetWorks();
    void cardioIsLoggedAndCanBeCorrected();
    void invalidCardioInputShowsMessage_data();
    void invalidCardioInputShowsMessage();
    void loggingForTheWrongKindOfExerciseIsRefused();
    void workoutTotalAddsAllExercises();
    void changingBodyWeightRecalculatesCalories();
    void loggedDataSurvivesARestart();
};

// ---- start-up ----------------------------------------------------------------------------

void TstGui::startsWithEmptyDataFolder()
{
    Fixture f;
    QCOMPARE(f.window->windowTitle(), QStringLiteral("Workout & Meal Tracker"));

    auto* tabs = f.find<QTabWidget>("tabs");
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Profile"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("Workouts"));
    QCOMPARE(tabs->count(), 2);

    // first start: friendly info banner about the missing profile
    auto* banner = f.find<QFrame>("banner");
    QVERIFY(banner != nullptr);
    QVERIFY(!banner->isHidden());
    QVERIFY(f.find<QLabel>("bannerLabel")->text().contains("No profile found yet"));

    // the exercise library file was created from the built-in list
    QVERIFY(QFile::exists(f.path("exercise_library.json")));
    // nothing else is written until the user saves something
    QVERIFY(!QFile::exists(f.path("profile.json")));
}

void TstGui::damagedProfileShowsWarningBanner()
{
    Fixture f;
    f.close();
    QFile file(f.path("profile.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ definitely not valid json");
    file.close();

    f.open();
    QVERIFY(!f.find<QFrame>("banner")->isHidden());
    const QString text = f.find<QLabel>("bannerLabel")->text();
    QVERIFY2(text.contains("profile.json.corrupt"), qPrintable(text));
    QVERIFY(QFile::exists(f.path("profile.json.corrupt")));

    // the window still works with the default profile
    QCOMPARE(f.find<QLineEdit>("nameEdit")->text(), QStringLiteral("User"));
}

void TstGui::bannerCanBeDismissed()
{
    Fixture f;
    QVERIFY(!f.find<QFrame>("banner")->isHidden());
    f.find<QPushButton>("bannerDismissButton")->click();
    QVERIFY(f.find<QFrame>("banner")->isHidden());
}

// ---- Profile tab -----------------------------------------------------------------------------

void TstGui::profileFieldsShowSavedProfile()
{
    Fixture f;
    f.close();
    DataManager(f.dir.path().toStdString()).saveProfile(UserProfile("Demo User", 28, 175.5, 72.5, 2200));

    f.open();
    QCOMPARE(f.find<QLineEdit>("nameEdit")->text(), QStringLiteral("Demo User"));
    QCOMPARE(f.find<QLineEdit>("ageEdit")->text(), QStringLiteral("28"));
    QCOMPARE(f.find<QLineEdit>("heightEdit")->text(), QStringLiteral("175.5"));
    QCOMPARE(f.find<QLineEdit>("weightEdit")->text(), QStringLiteral("72.5"));
    QCOMPARE(f.find<QLineEdit>("goalEdit")->text(), QStringLiteral("2200"));
    QVERIFY(f.find<QFrame>("banner")->isHidden()); // profile loaded fine: no notice
}

void TstGui::invalidProfileInputShowsMessageAndSavesNothing_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("age");
    QTest::addColumn<QString>("height");
    QTest::addColumn<QString>("weight");
    QTest::addColumn<QString>("goal");
    QTest::addColumn<QString>("expectedMessage");

    // all other fields are valid in every row
    QTest::newRow("blank name") << "  " << "28" << "175" << "72.5" << "2200" << "Name is required.";
    QTest::newRow("age empty") << "A" << "" << "175" << "72.5" << "2200" << "Age is required.";
    QTest::newRow("age letters") << "A" << "abc" << "175" << "72.5" << "2200" << "Age must be a whole number.";
    QTest::newRow("age decimal") << "A" << "28.5" << "175" << "72.5" << "2200" << "Age must be a whole number.";
    QTest::newRow("age too low") << "A" << "9" << "175" << "72.5" << "2200" << "Age must be between 10 and 120 years.";
    QTest::newRow("height letters") << "A" << "28" << "tall" << "72.5" << "2200" << "Height must be a number.";
    QTest::newRow("height too low") << "A" << "28" << "10" << "72.5" << "2200" << "Height must be between 100 and 250 cm.";
    QTest::newRow("weight negative") << "A" << "28" << "175" << "-5" << "2200" << "Weight must be between 20 and 400 kg.";
    QTest::newRow("weight comma") << "A" << "28" << "175" << "70,5" << "2200"
                                  << "Weight must be a number (use a dot for decimals).";
    QTest::newRow("goal empty") << "A" << "28" << "175" << "72.5" << "" << "Daily calorie goal is required.";
    QTest::newRow("goal too low") << "A" << "28" << "175" << "72.5" << "100"
                                  << "Daily calorie goal must be between 800 and 10000 kcal.";
}

void TstGui::invalidProfileInputShowsMessageAndSavesNothing()
{
    QFETCH(QString, name);
    QFETCH(QString, age);
    QFETCH(QString, height);
    QFETCH(QString, weight);
    QFETCH(QString, goal);
    QFETCH(QString, expectedMessage);

    Fixture f;
    const QString bmiBefore = f.find<QLabel>("bmiLabel")->text();

    setProfileFields(f, name, age, height, weight, goal);
    f.find<QPushButton>("saveProfileButton")->click();

    QCOMPARE(f.find<QLabel>("profileError")->text(), expectedMessage);
    QVERIFY(!QFile::exists(f.path("profile.json")));          // nothing saved
    QCOMPARE(f.find<QLabel>("bmiLabel")->text(), bmiBefore);   // program's profile unchanged
}

void TstGui::validProfileIsSavedAndShown()
{
    Fixture f;
    setProfileFields(f, "  Demo User ", "28", "175", "70", "2200");
    f.find<QPushButton>("saveProfileButton")->click();

    QCOMPARE(f.find<QLabel>("profileError")->text(), QString());
    QCOMPARE(f.find<QLabel>("bmiLabel")->text(), QStringLiteral("BMI: 22.9"));
    QCOMPARE(f.window->statusBar()->currentMessage(), QStringLiteral("Profile saved."));
    QCOMPARE(f.find<QLineEdit>("nameEdit")->text(), QStringLiteral("Demo User")); // trimmed

    // the file really contains the profile
    const auto loaded = DataManager(f.dir.path().toStdString()).loadProfile();
    QVERIFY(loaded.ok());
    QCOMPARE(qs(loaded.value.name()), QStringLiteral("Demo User"));
    QCOMPARE(loaded.value.weightKg(), 70.0);
    QCOMPARE(loaded.value.dailyCalorieGoal(), 2200);

    // a later invalid attempt shows an error but does not touch the saved profile
    f.find<QLineEdit>("weightEdit")->setText("abc");
    f.find<QPushButton>("saveProfileButton")->click();
    QCOMPARE(f.find<QLabel>("profileError")->text(), QStringLiteral("Weight must be a number."));
    QCOMPARE(DataManager(f.dir.path().toStdString()).loadProfile().value.weightKg(), 70.0);

    // fixing the value clears the error
    f.find<QLineEdit>("weightEdit")->setText("71.5");
    f.find<QPushButton>("saveProfileButton")->click();
    QCOMPARE(f.find<QLabel>("profileError")->text(), QString());
}

// ---- Workouts tab -------------------------------------------------------------------------------

void TstGui::libraryListCanBeFilteredByNameAndCategory()
{
    Fixture f;
    auto* list = f.find<QListWidget>("libraryList");
    auto* search = f.find<QLineEdit>("searchEdit");
    auto* category = f.find<QComboBox>("categoryCombo");

    QCOMPARE(list->count(), 38); // everything
    QCOMPARE(category->count(), 8); // "All categories" + 7 categories

    search->setText("press");
    QCOMPARE(list->count(), 4);
    QVERIFY(list->item(0)->text().startsWith("Bench Press"));

    search->setText("PrEsS"); // case does not matter
    QCOMPARE(list->count(), 4);

    search->clear();
    category->setCurrentIndex(category->findText("Cardio"));
    QCOMPARE(list->count(), 12);

    search->setText("run"); // name and category together
    QCOMPARE(list->count(), 1);
    QVERIFY(list->item(0)->text().startsWith("Running"));

    search->setText("zzz"); // no match: empty list, no crash
    QCOMPARE(list->count(), 0);

    search->clear();
    category->setCurrentIndex(0);
    QCOMPARE(list->count(), 38);
}

void TstGui::createWorkoutValidatesAndLists()
{
    Fixture f;
    auto* list = f.find<QListWidget>("workoutList");

    createWorkout(f, "   "); // blank name
    QCOMPARE(workoutsError(f), QStringLiteral("Workout name is required."));
    QCOMPARE(list->count(), 0);
    QVERIFY(!QFile::exists(f.path("workouts.json")));

    createWorkout(f, QString(61, 'x'));
    QVERIFY(workoutsError(f).contains("at most 60 characters"));
    QCOMPARE(list->count(), 0);

    createWorkout(f, "Push Day");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(list->count(), 1);
    QVERIFY(list->item(0)->text().contains("2026-11-10"));
    QVERIFY(list->item(0)->text().contains("Push Day"));
    QVERIFY(list->item(0)->text().contains("0 exercises"));
    QCOMPARE(list->currentRow(), 0); // the new workout is selected
    QCOMPARE(f.find<QLabel>("selectedWorkoutLabel")->text(), QStringLiteral("Workout: Push Day  (2026-11-10)"));
    QCOMPARE(f.find<QLineEdit>("workoutNameEdit")->text(), QString()); // field cleared
    QVERIFY(QFile::exists(f.path("workouts.json")));

    // same name + same date is a duplicate, even with different capitals
    createWorkout(f, "push DAY");
    QCOMPARE(workoutsError(f), QStringLiteral("A workout named 'push DAY' already exists on 2026-11-10."));
    QCOMPARE(list->count(), 1);

    // same name on another date, and another name on the same date, are fine
    createWorkout(f, "Push Day", "2026-11-12");
    createWorkout(f, "Pull Day", "2026-11-10");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(list->count(), 3);
    QVERIFY(list->item(0)->text().contains("2026-11-12")); // newest first
}

void TstGui::addAndRemoveExercises()
{
    Fixture f;
    createWorkout(f, "Push Day");
    auto* table = f.find<QTableWidget>("exerciseTable");
    auto* workouts = f.find<QListWidget>("workoutList");

    addFromLibrary(f, "bench");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(cell(f, 0, 0), QStringLiteral("Bench Press"));
    QCOMPARE(cell(f, 0, 1), QStringLiteral("Chest"));
    QCOMPARE(cell(f, 0, 2), QStringLiteral("No sets logged"));
    QCOMPARE(cell(f, 0, 3), QStringLiteral("0")); // nothing logged yet
    QVERIFY(workouts->item(0)->text().contains("(1 exercise)"));

    // the same exercise twice is rejected with a helpful message
    addFromLibrary(f, "bench");
    QCOMPARE(workoutsError(f),
             QStringLiteral("'Bench Press' is already in this workout. Add more sets to it instead."));
    QCOMPARE(table->rowCount(), 1);

    addFromLibrary(f, "running");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(table->rowCount(), 2);
    QVERIFY(workouts->item(0)->text().contains("(2 exercises)"));

    // remove the first row
    table->setCurrentCell(0, 0);
    f.find<QPushButton>("removeExerciseButton")->click();
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(cell(f, 0, 0), QStringLiteral("Running"));

    // what is on screen is what was saved
    const auto loaded = DataManager(f.dir.path().toStdString()).loadWorkouts();
    QVERIFY(loaded.ok());
    QCOMPARE(static_cast<int>(loaded.value.sessions().front().exerciseCount()), 1);

    // the library template itself is never modified by workouts
    f.find<QLineEdit>("searchEdit")->setText("bench");
    QCOMPARE(f.find<QListWidget>("libraryList")->count(), 1);
}

void TstGui::addingNeedsAWorkoutAndALibrarySelection()
{
    Fixture f;
    f.find<QPushButton>("addExerciseButton")->click();
    QCOMPARE(workoutsError(f), QStringLiteral("Create or select a workout first."));

    createWorkout(f, "Push Day");
    QCOMPARE(workoutsError(f), QString());
    f.find<QPushButton>("addExerciseButton")->click(); // nothing selected in the library
    QCOMPARE(workoutsError(f), QStringLiteral("Select an exercise from the library first."));
    QCOMPARE(f.find<QTableWidget>("exerciseTable")->rowCount(), 0);
}

void TstGui::removingNeedsASelection()
{
    Fixture f;
    f.find<QPushButton>("removeExerciseButton")->click();
    QCOMPARE(workoutsError(f), QStringLiteral("Create or select a workout first."));

    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");
    f.find<QTableWidget>("exerciseTable")->clearSelection();
    f.find<QTableWidget>("exerciseTable")->setCurrentCell(-1, -1);
    f.find<QPushButton>("removeExerciseButton")->click();
    QCOMPARE(workoutsError(f), QStringLiteral("Select an exercise in the workout to remove."));
    QCOMPARE(f.find<QTableWidget>("exerciseTable")->rowCount(), 1);
}

void TstGui::deleteWorkout()
{
    Fixture f;
    f.window->findChild<WorkoutsTab*>()->setConfirmDeletes(false); // no modal dialog in tests
    auto* list = f.find<QListWidget>("workoutList");

    f.find<QPushButton>("deleteWorkoutButton")->click();
    QCOMPARE(workoutsError(f), QStringLiteral("Select a workout to delete."));

    createWorkout(f, "Push Day");
    createWorkout(f, "Pull Day");
    QCOMPARE(list->count(), 2);

    list->setCurrentRow(0); // "Push Day" sorts after "Pull Day" on the same date, so it is first
    f.find<QPushButton>("deleteWorkoutButton")->click();
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(list->count(), 1);
    QCOMPARE(f.find<QLabel>("selectedWorkoutLabel")->text(),
             QStringLiteral("Select or create a workout on the left to start logging."));

    const auto loaded = DataManager(f.dir.path().toStdString()).loadWorkouts();
    QCOMPARE(static_cast<int>(loaded.value.size()), 1);
    QCOMPARE(qs(loaded.value.sessions().front().name()), QStringLiteral("Pull Day"));
}

void TstGui::switchingBetweenWorkoutsShowsTheirExercises()
{
    Fixture f;
    auto* list = f.find<QListWidget>("workoutList");
    auto* table = f.find<QTableWidget>("exerciseTable");

    createWorkout(f, "Push Day", "2026-11-10");
    addFromLibrary(f, "bench");
    createWorkout(f, "Run", "2026-11-11");
    addFromLibrary(f, "running");
    addFromLibrary(f, "cycling");

    QCOMPARE(table->rowCount(), 2); // "Run" is selected (newest)
    list->setCurrentRow(1);         // "Push Day"
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(cell(f, 0, 0), QStringLiteral("Bench Press"));
    list->setCurrentRow(0);
    QCOMPARE(table->rowCount(), 2);
}

void TstGui::workoutsSurviveARestart()
{
    Fixture f;
    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");
    addFromLibrary(f, "squat");
    f.close();

    f.open(); // new window, same data folder
    auto* list = f.find<QListWidget>("workoutList");
    QCOMPARE(list->count(), 1);
    QVERIFY(list->item(0)->text().contains("(2 exercises)"));
    list->setCurrentRow(0);
    QCOMPARE(f.find<QTableWidget>("exerciseTable")->rowCount(), 2);
}

// ---- Workouts tab: logging ------------------------------------------------------------------------

void TstGui::logPanelFollowsTheSelectedExercise()
{
    Fixture f;
    auto* stack = f.find<QStackedWidget>("logStack");
    auto* table = f.find<QTableWidget>("exerciseTable");

    QCOMPARE(stack->currentIndex(), 0); // hint page: nothing selected
    createWorkout(f, "Mixed");
    QCOMPARE(stack->currentIndex(), 0);

    addFromLibrary(f, "bench");   // the new exercise is selected automatically
    QCOMPARE(stack->currentIndex(), 1);
    addFromLibrary(f, "running");
    QCOMPARE(stack->currentIndex(), 2);

    table->setCurrentCell(0, 0);
    QCOMPARE(stack->currentIndex(), 1);
    table->setCurrentCell(1, 0);
    QCOMPARE(stack->currentIndex(), 2);
}

void TstGui::strengthSetsAreLoggedWithCalories()
{
    Fixture f; // no profile saved -> default profile, 70 kg
    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");

    for (int i = 0; i < 3; ++i) {
        addSet(f, "10", "60");
        QCOMPARE(workoutsError(f), QString());
    }
    QCOMPARE(cell(f, 0, 2), QStringLiteral("3 sets: 10 x 60 kg, 10 x 60 kg, 10 x 60 kg"));
    QCOMPARE(cell(f, 0, 3), QStringLiteral("29.2")); // MET 5 x 70 kg x 300 s
    QCOMPARE(f.find<QLabel>("totalLabel")->text(),
             QStringLiteral("Total calories burned in this workout: 29.2 kcal (estimated for 70 kg body weight)"));
    QCOMPARE(f.find<QStackedWidget>("logStack")->currentIndex(), 1); // exercise stays selected

    addSet(f, "8", "62.5");
    QVERIFY(cell(f, 0, 2).endsWith("8 x 62.5 kg"));

    // bodyweight set
    addSet(f, "15", "0");
    QVERIFY(cell(f, 0, 2).endsWith("15 x bodyweight"));
}

void TstGui::invalidSetInputShowsMessage_data()
{
    QTest::addColumn<QString>("reps");
    QTest::addColumn<QString>("weight");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("reps empty") << "" << "60" << "Reps is required.";
    QTest::newRow("reps zero") << "0" << "60" << "Reps must be between 1 and 100.";
    QTest::newRow("reps too many") << "101" << "60" << "Reps must be between 1 and 100.";
    QTest::newRow("reps negative") << "-4" << "60" << "Reps must be between 1 and 100.";
    QTest::newRow("reps letters") << "ten" << "60" << "Reps must be a whole number.";
    QTest::newRow("reps decimal") << "5.5" << "60" << "Reps must be a whole number.";
    QTest::newRow("weight empty") << "10" << "" << "Weight is required.";
    QTest::newRow("weight negative") << "10" << "-1" << "Weight must be between 0 and 1000 kg.";
    QTest::newRow("weight too heavy") << "10" << "1001" << "Weight must be between 0 and 1000 kg.";
    QTest::newRow("weight letters") << "10" << "heavy" << "Weight must be a number.";
    QTest::newRow("weight comma") << "10" << "62,5" << "Weight must be a number (use a dot for decimals).";
}

void TstGui::invalidSetInputShowsMessage()
{
    QFETCH(QString, reps);
    QFETCH(QString, weight);
    QFETCH(QString, expectedMessage);

    Fixture f;
    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");

    addSet(f, reps, weight);
    QCOMPARE(workoutsError(f), expectedMessage);
    QCOMPARE(cell(f, 0, 2), QStringLiteral("No sets logged")); // nothing was added

    // after correcting the input the same screen works
    addSet(f, "10", "60");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(cell(f, 0, 2), QStringLiteral("1 set: 10 x 60 kg"));
}

void TstGui::setLimitIsEnforced()
{
    Fixture f;
    createWorkout(f, "Push Day");
    addFromLibrary(f, "curl");
    for (int i = 0; i < 50; ++i) {
        addSet(f, "10", "10");
    }
    QCOMPARE(workoutsError(f), QString());
    addSet(f, "10", "10"); // the 51st
    QCOMPARE(workoutsError(f), QStringLiteral("An exercise can have at most 50 sets."));
    QVERIFY(cell(f, 0, 2).startsWith("50 sets:"));
}

void TstGui::removeLastSetWorks()
{
    Fixture f;
    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");

    f.find<QPushButton>("removeSetButton")->click();
    QCOMPARE(workoutsError(f), QStringLiteral("There are no sets to remove."));

    addSet(f, "10", "60");
    addSet(f, "8", "65");
    f.find<QPushButton>("removeSetButton")->click();
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(cell(f, 0, 2), QStringLiteral("1 set: 10 x 60 kg"));
}

void TstGui::cardioIsLoggedAndCanBeCorrected()
{
    Fixture f;
    createWorkout(f, "Run");
    addFromLibrary(f, "running");
    QCOMPARE(f.find<QStackedWidget>("logStack")->currentIndex(), 2);

    logCardio(f, "30", "5");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(cell(f, 0, 2), QStringLiteral("30 min, 5 km (10 km/h)"));
    QCOMPARE(cell(f, 0, 3), QStringLiteral("343")); // MET 9.8 x 70 kg x 0.5 h
    QVERIFY(f.find<QLabel>("totalLabel")->text().contains("343 kcal"));

    // logging again replaces the earlier values; distance is optional
    logCardio(f, "45", "");
    QCOMPARE(workoutsError(f), QString());
    QCOMPARE(cell(f, 0, 2), QStringLiteral("45 min"));
    QCOMPARE(cell(f, 0, 3), QStringLiteral("514.5")); // 9.8 x 70 x 0.75

    // the log fields show the stored values again when the exercise is re-selected
    f.find<QTableWidget>("exerciseTable")->setCurrentCell(-1, -1);
    f.find<QTableWidget>("exerciseTable")->setCurrentCell(0, 0);
    QCOMPARE(f.find<QLineEdit>("cardioDurationEdit")->text(), QStringLiteral("45"));
    QCOMPARE(f.find<QLineEdit>("cardioDistanceEdit")->text(), QString());
}

void TstGui::invalidCardioInputShowsMessage_data()
{
    QTest::addColumn<QString>("minutes");
    QTest::addColumn<QString>("km");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("duration empty") << "" << "5" << "Duration is required.";
    QTest::newRow("duration letters") << "abc" << "5" << "Duration must be a number.";
    QTest::newRow("duration zero") << "0" << "5" << "Duration must be between 1 and 600 minutes.";
    QTest::newRow("duration negative") << "-10" << "5" << "Duration must be between 1 and 600 minutes.";
    QTest::newRow("duration too long") << "601" << "5" << "Duration must be between 1 and 600 minutes.";
    QTest::newRow("distance letters") << "30" << "far" << "Distance must be a number.";
    QTest::newRow("distance negative") << "30" << "-1" << "Distance must be between 0 and 500 km.";
    QTest::newRow("distance too far") << "30" << "501" << "Distance must be between 0 and 500 km.";
}

void TstGui::invalidCardioInputShowsMessage()
{
    QFETCH(QString, minutes);
    QFETCH(QString, km);
    QFETCH(QString, expectedMessage);

    Fixture f;
    createWorkout(f, "Run");
    addFromLibrary(f, "running");

    logCardio(f, minutes, km);
    QCOMPARE(workoutsError(f), expectedMessage);
    QCOMPARE(cell(f, 0, 2), QStringLiteral("Not logged"));
    QCOMPARE(cell(f, 0, 3), QStringLiteral("0"));
}

void TstGui::loggingForTheWrongKindOfExerciseIsRefused()
{
    Fixture f;
    createWorkout(f, "Mixed");
    addFromLibrary(f, "bench");
    addFromLibrary(f, "running");

    auto* table = f.find<QTableWidget>("exerciseTable");
    table->setCurrentCell(1, 0); // Running selected, but "Add set" is for strength
    addSet(f, "10", "60");
    QCOMPARE(workoutsError(f), QStringLiteral("Select a strength exercise in the workout first."));
    table->setCurrentCell(0, 0); // Bench Press selected, but "Log cardio" is for cardio
    logCardio(f, "30", "5");
    QCOMPARE(workoutsError(f), QStringLiteral("Select a cardio exercise in the workout first."));

    // nothing selected at all
    table->setCurrentCell(-1, -1);
    addSet(f, "10", "60");
    QCOMPARE(workoutsError(f), QStringLiteral("Select a strength exercise in the workout first."));
    QCOMPARE(cell(f, 0, 2), QStringLiteral("No sets logged"));
    QCOMPARE(cell(f, 1, 2), QStringLiteral("Not logged"));
}

void TstGui::workoutTotalAddsAllExercises()
{
    Fixture f;
    createWorkout(f, "Mixed");
    addFromLibrary(f, "bench");
    for (int i = 0; i < 3; ++i) {
        addSet(f, "10", "60");
    }
    addFromLibrary(f, "running");
    logCardio(f, "30", "5");

    // 29.1667 + 343.0 = 372.1667
    QVERIFY2(f.find<QLabel>("totalLabel")->text().contains("372.2 kcal"),
             qPrintable(f.find<QLabel>("totalLabel")->text()));
}

void TstGui::changingBodyWeightRecalculatesCalories()
{
    Fixture f;
    createWorkout(f, "Push Day");
    addFromLibrary(f, "bench");
    for (int i = 0; i < 3; ++i) {
        addSet(f, "10", "60");
    }
    QCOMPARE(cell(f, 0, 3), QStringLiteral("29.2")); // default 70 kg

    // save a 140 kg profile in the Profile tab: the Workouts tab must follow
    setProfileFields(f, "Demo User", "28", "175", "140", "2200");
    f.find<QPushButton>("saveProfileButton")->click();
    QCOMPARE(f.find<QLabel>("profileError")->text(), QString());

    QCOMPARE(cell(f, 0, 3), QStringLiteral("58.3"));
    QVERIFY(f.find<QLabel>("totalLabel")->text().contains("58.3 kcal"));
    QVERIFY(f.find<QLabel>("totalLabel")->text().contains("140 kg"));
}

void TstGui::loggedDataSurvivesARestart()
{
    Fixture f;
    createWorkout(f, "Mixed");
    addFromLibrary(f, "bench");
    addSet(f, "10", "60");
    addSet(f, "8", "62.5");
    addFromLibrary(f, "running");
    logCardio(f, "32.5", "5.25");
    f.close();

    f.open();
    f.find<QListWidget>("workoutList")->setCurrentRow(0);
    QCOMPARE(cell(f, 0, 0), QStringLiteral("Bench Press"));
    QCOMPARE(cell(f, 0, 2), QStringLiteral("2 sets: 10 x 60 kg, 8 x 62.5 kg"));
    QCOMPARE(cell(f, 1, 0), QStringLiteral("Running"));
    QCOMPARE(cell(f, 1, 2), QStringLiteral("32.5 min, 5.25 km (9.7 km/h)"));
}

QTEST_MAIN(TstGui)
#include "tst_gui.moc"
