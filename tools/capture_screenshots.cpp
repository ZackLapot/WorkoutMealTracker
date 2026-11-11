// Developer tool: starts the real MainWindow with demo data, fills it in the way a
// user would, and saves PNG screenshots of the window.
//
//   capture_screenshots <output-folder>
//
// It uses the "offscreen" Qt platform, so it needs no display. The demo person is
// "Demo User" - the screenshots show example data, not real personal data.

#include "core/DataManager.h"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>

#include <iostream>

namespace {

template <typename T>
T* find(MainWindow& window, const char* objectName)
{
    T* widget = window.findChild<T*>(objectName);
    if (!widget) {
        std::cerr << "widget not found: " << objectName << "\n";
        std::exit(1);
    }
    return widget;
}

void save(MainWindow& window, const QDir& folder, const QString& fileName)
{
    QApplication::processEvents();
    const QString path = folder.filePath(fileName);
    if (!window.grab().save(path)) {
        std::cerr << "could not write " << path.toStdString() << "\n";
        std::exit(1);
    }
    std::cout << "saved " << path.toStdString() << "\n";
}

void createWorkout(MainWindow& window, const QString& name, const QString& isoDate)
{
    find<QLineEdit>(window, "workoutNameEdit")->setText(name);
    find<QDateEdit>(window, "workoutDateEdit")->setDate(QDate::fromString(isoDate, Qt::ISODate));
    find<QPushButton>(window, "createWorkoutButton")->click();
}

void addFromLibrary(MainWindow& window, const QString& search)
{
    find<QLineEdit>(window, "searchEdit")->setText(search);
    find<QListWidget>(window, "libraryList")->setCurrentRow(0);
    find<QPushButton>(window, "addExerciseButton")->click();
}

void addSet(MainWindow& window, const QString& reps, const QString& weight)
{
    find<QLineEdit>(window, "setRepsEdit")->setText(reps);
    find<QLineEdit>(window, "setWeightEdit")->setText(weight);
    find<QPushButton>(window, "addSetButton")->click();
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "usage: capture_screenshots <output-folder>\n";
        return 2;
    }
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");

    const QDir output(QString::fromLocal8Bit(argv[1]));
    QDir().mkpath(output.path());

    // ---- 1. Profile tab, then a validation error ---------------------------------------
    {
        QTemporaryDir dataDir;
        DataManager(dataDir.path().toStdString())
            .saveProfile(UserProfile("Demo User", 28, 175.0, 72.5, 2200));

        MainWindow window(dataDir.path().toStdString());
        window.show();
        save(window, output, "01_profile_tab.png");

        find<QLineEdit>(window, "weightEdit")->setText("seventy");
        find<QPushButton>(window, "saveProfileButton")->click();
        save(window, output, "02_profile_validation_error.png");
    }

    // ---- 2. Workouts tab with logged sets and cardio ----------------------------------------
    {
        QTemporaryDir dataDir;
        DataManager(dataDir.path().toStdString())
            .saveProfile(UserProfile("Demo User", 28, 175.0, 72.5, 2200));

        MainWindow window(dataDir.path().toStdString());
        window.show();
        find<QTabWidget>(window, "tabs")->setCurrentIndex(1);

        createWorkout(window, "Morning Run", "2026-11-09");
        addFromLibrary(window, "running");
        find<QLineEdit>(window, "cardioDurationEdit")->setText("32.5");
        find<QLineEdit>(window, "cardioDistanceEdit")->setText("5.25");
        find<QPushButton>(window, "logCardioButton")->click();

        createWorkout(window, "Push Day", "2026-11-10");
        addFromLibrary(window, "bench");
        addSet(window, "10", "60");
        addSet(window, "10", "60");
        addSet(window, "8", "62.5");
        addFromLibrary(window, "overhead");
        addSet(window, "10", "40");
        addSet(window, "8", "42.5");
        addFromLibrary(window, "running");
        find<QLineEdit>(window, "cardioDurationEdit")->setText("20");
        find<QPushButton>(window, "logCardioButton")->click();

        // show the strength panel for Bench Press and a filtered library
        find<QTableWidget>(window, "exerciseTable")->setCurrentCell(0, 0);
        find<QLineEdit>(window, "searchEdit")->setText("press");
        save(window, output, "03_workouts_tab.png");

        // an invalid set: the message appears under the form and nothing is added
        addSet(window, "0", "60");
        save(window, output, "04_workouts_validation_error.png");

        // a duplicate exercise
        addFromLibrary(window, "bench");
        save(window, output, "05_workouts_duplicate_exercise.png");
    }

    // ---- 3. Start-up with a damaged data file -------------------------------------------------
    {
        QTemporaryDir dataDir;
        QFile damaged(dataDir.filePath("profile.json"));
        if (!damaged.open(QIODevice::WriteOnly)) {
            return 1;
        }
        damaged.write("{ \"version\": 1, \"name\": \"Demo User\", \"age\": ");
        damaged.close();

        MainWindow window(dataDir.path().toStdString());
        window.show();
        save(window, output, "06_damaged_file_banner.png");
    }

    return 0;
}
