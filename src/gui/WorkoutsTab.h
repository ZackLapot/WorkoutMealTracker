#pragma once

#include "core/DataManager.h"
#include "core/ExerciseLibrary.h"
#include "core/UserProfile.h"
#include "core/WorkoutLog.h"

#include <QString>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;

// Tab for building and logging workouts:
//   left  - create / pick / delete a workout
//   right - search the exercise library, add exercises to the selected workout,
//           and see what has been logged with the calories burned.
//
// All rules (duplicates, limits, calories) live in the core classes; this class
// only collects input, calls them, and shows the result or the error message.
class WorkoutsTab : public QWidget {
    Q_OBJECT

public:
    WorkoutsTab(const ExerciseLibrary& library, WorkoutLog& log, const UserProfile& profile,
                const DataManager& dataManager, QWidget* parent = nullptr);

    // Ask "are you sure?" before deleting a workout. Tests switch this off so no
    // modal dialog blocks them.
    void setConfirmDeletes(bool confirm) { confirmDeletes_ = confirm; }

public slots:
    // Recalculate calories (call after the profile's weight changed).
    void refreshCalories();

signals:
    // Short success text for the status bar.
    void statusMessage(const QString& text);

private slots:
    void onCreateWorkout();
    void onDeleteWorkout();
    void onWorkoutSelectionChanged();
    void onFilterChanged();
    void onAddExercise();
    void onRemoveExercise();
    void onExerciseSelectionChanged();

private:
    void buildUi();

    // Fill the widgets from the data. Selecting by date+name keeps the user's place.
    void populateLibraryList();
    void populateWorkoutList(const QString& selectDate = QString(),
                             const QString& selectName = QString(),
                             const QString& keepExercise = QString());
    void refreshWorkoutDetails(const QString& keepExercise);
    void populateExerciseTable(const WorkoutSession* session, const QString& keepExercise);
    void updateTotals(const WorkoutSession* session);

    // What the user has selected right now (nullptr / empty if nothing).
    WorkoutSession* selectedWorkout();
    Exercise* selectedExercise();
    QString selectedExerciseName() const;

    bool persist();
    void showError(const QString& text);
    void clearError();

    const ExerciseLibrary& library_;
    WorkoutLog& log_;
    const UserProfile& profile_;
    const DataManager& dataManager_;
    bool confirmDeletes_ = true;

    QLineEdit* workoutNameEdit_ = nullptr;
    QDateEdit* workoutDateEdit_ = nullptr;
    QPushButton* createWorkoutButton_ = nullptr;
    QListWidget* workoutList_ = nullptr;
    QPushButton* deleteWorkoutButton_ = nullptr;

    QLabel* selectedLabel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QListWidget* libraryList_ = nullptr;
    QPushButton* addExerciseButton_ = nullptr;
    QTableWidget* exerciseTable_ = nullptr;
    QPushButton* removeExerciseButton_ = nullptr;

    QLabel* totalLabel_ = nullptr;
    QLabel* errorLabel_ = nullptr;
};
