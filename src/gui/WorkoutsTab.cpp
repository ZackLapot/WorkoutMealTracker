#include "WorkoutsTab.h"

#include "core/CardioExercise.h"
#include "core/Errors.h"
#include "core/StrengthExercise.h"
#include "core/Validation.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <optional>
#include <utility>

namespace {

constexpr int kDateRole = Qt::UserRole;     // workout list items: "2026-11-10"
constexpr int kNameRole = Qt::UserRole + 1; // workout list items: "Push Day"
constexpr int kExerciseNameRole = Qt::UserRole; // library list items: exercise name

QString qs(const std::string& text)
{
    return QString::fromStdString(text);
}

QString oneDecimal(double value)
{
    return qs(validation::formatNumber(value, 1));
}

QString typeLabel(const Exercise& exercise)
{
    return exercise.typeName() == "strength" ? "Strength" : "Cardio";
}

} // namespace

WorkoutsTab::WorkoutsTab(const ExerciseLibrary& library, WorkoutLog& log,
                         const UserProfile& profile, const DataManager& dataManager,
                         QWidget* parent)
    : QWidget(parent), library_(library), log_(log), profile_(profile), dataManager_(dataManager)
{
    buildUi();
    populateLibraryList();
    populateWorkoutList();
}

// ---- building the screen ------------------------------------------------------------------

void WorkoutsTab::buildUi()
{
    // ----- left column: workouts -----
    workoutNameEdit_ = new QLineEdit;
    workoutNameEdit_->setObjectName("workoutNameEdit");
    workoutNameEdit_->setPlaceholderText("e.g. Push Day");

    workoutDateEdit_ = new QDateEdit(QDate::currentDate());
    workoutDateEdit_->setObjectName("workoutDateEdit");
    workoutDateEdit_->setCalendarPopup(true);
    workoutDateEdit_->setDisplayFormat("yyyy-MM-dd");

    createWorkoutButton_ = new QPushButton("Create workout");
    createWorkoutButton_->setObjectName("createWorkoutButton");

    auto* newBox = new QGroupBox("New workout");
    auto* newForm = new QFormLayout(newBox);
    newForm->addRow("Name", workoutNameEdit_);
    newForm->addRow("Date", workoutDateEdit_);
    newForm->addRow(createWorkoutButton_);

    workoutList_ = new QListWidget;
    workoutList_->setObjectName("workoutList");

    deleteWorkoutButton_ = new QPushButton("Delete selected workout");
    deleteWorkoutButton_->setObjectName("deleteWorkoutButton");

    auto* left = new QVBoxLayout;
    left->addWidget(newBox);
    left->addWidget(new QLabel("Your workouts (newest first)"));
    left->addWidget(workoutList_, 1);
    left->addWidget(deleteWorkoutButton_);

    // ----- right column: library, workout contents -----
    selectedLabel_ = new QLabel;
    selectedLabel_->setObjectName("selectedWorkoutLabel");
    selectedLabel_->setStyleSheet("font-size: 15px; font-weight: bold;");

    searchEdit_ = new QLineEdit;
    searchEdit_->setObjectName("searchEdit");
    searchEdit_->setPlaceholderText("Search exercises by name...");
    searchEdit_->setClearButtonEnabled(true);

    categoryCombo_ = new QComboBox;
    categoryCombo_->setObjectName("categoryCombo");
    categoryCombo_->addItem("All categories", -1);
    for (Category category : allCategories()) {
        categoryCombo_->addItem(qs(categoryToString(category)), static_cast<int>(category));
    }

    libraryList_ = new QListWidget;
    libraryList_->setObjectName("libraryList");
    libraryList_->setMinimumHeight(110);

    addExerciseButton_ = new QPushButton("Add to workout");
    addExerciseButton_->setObjectName("addExerciseButton");

    auto* libraryBox = new QGroupBox("Exercise library");
    auto* libraryLayout = new QVBoxLayout(libraryBox);
    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(searchEdit_, 1);
    filterRow->addWidget(categoryCombo_);
    libraryLayout->addLayout(filterRow);
    libraryLayout->addWidget(libraryList_);
    auto* addRow = new QHBoxLayout;
    addRow->addStretch(1);
    addRow->addWidget(addExerciseButton_);
    libraryLayout->addLayout(addRow);

    exerciseTable_ = new QTableWidget(0, 4);
    exerciseTable_->setObjectName("exerciseTable");
    exerciseTable_->setHorizontalHeaderLabels({"Exercise", "Category", "Logged", "kcal burned"});
    exerciseTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    exerciseTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    exerciseTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    exerciseTable_->verticalHeader()->hide();
    exerciseTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    exerciseTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    exerciseTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    exerciseTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    exerciseTable_->setMinimumHeight(120);

    removeExerciseButton_ = new QPushButton("Remove selected exercise");
    removeExerciseButton_->setObjectName("removeExerciseButton");

    auto* exercisesBox = new QGroupBox("Exercises in this workout");
    auto* exercisesLayout = new QVBoxLayout(exercisesBox);
    exercisesLayout->addWidget(exerciseTable_);
    auto* removeRow = new QHBoxLayout;
    removeRow->addStretch(1);
    removeRow->addWidget(removeExerciseButton_);
    exercisesLayout->addLayout(removeRow);

    totalLabel_ = new QLabel;
    totalLabel_->setObjectName("totalLabel");
    totalLabel_->setStyleSheet("font-weight: bold;");

    errorLabel_ = new QLabel;
    errorLabel_->setObjectName("workoutsError");
    errorLabel_->setWordWrap(true);
    errorLabel_->setStyleSheet("color: #b00020; font-weight: bold;");
    errorLabel_->setMinimumHeight(errorLabel_->fontMetrics().height() * 2);
    errorLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto* right = new QVBoxLayout;
    right->addWidget(selectedLabel_);
    right->addWidget(libraryBox);
    right->addWidget(exercisesBox, 1);
    right->addWidget(totalLabel_);
    right->addWidget(errorLabel_);

    // ----- whole tab -----
    auto* leftWidget = new QWidget;
    leftWidget->setLayout(left);
    leftWidget->setFixedWidth(300);
    left->setContentsMargins(0, 0, 0, 0);

    auto* main = new QHBoxLayout(this);
    main->addWidget(leftWidget);
    main->addLayout(right, 1);

    // ----- connections -----
    connect(createWorkoutButton_, &QPushButton::clicked, this, &WorkoutsTab::onCreateWorkout);
    connect(workoutNameEdit_, &QLineEdit::returnPressed, this, &WorkoutsTab::onCreateWorkout);
    connect(deleteWorkoutButton_, &QPushButton::clicked, this, &WorkoutsTab::onDeleteWorkout);
    connect(workoutList_, &QListWidget::currentItemChanged, this,
            &WorkoutsTab::onWorkoutSelectionChanged);

    connect(searchEdit_, &QLineEdit::textChanged, this, &WorkoutsTab::onFilterChanged);
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WorkoutsTab::onFilterChanged);
    connect(addExerciseButton_, &QPushButton::clicked, this, &WorkoutsTab::onAddExercise);
    connect(libraryList_, &QListWidget::itemDoubleClicked, this, &WorkoutsTab::onAddExercise);

    connect(removeExerciseButton_, &QPushButton::clicked, this, &WorkoutsTab::onRemoveExercise);
    connect(exerciseTable_, &QTableWidget::itemSelectionChanged, this,
            &WorkoutsTab::onExerciseSelectionChanged);
}

// ---- filling the widgets ---------------------------------------------------------------------

void WorkoutsTab::populateLibraryList()
{
    std::optional<Category> category;
    const int categoryValue = categoryCombo_->currentData().toInt();
    if (categoryValue >= 0) {
        category = static_cast<Category>(categoryValue);
    }

    const auto matches = library_.search(searchEdit_->text().toStdString(), category);

    libraryList_->clear();
    for (const auto& exercise : matches) {
        auto* item = new QListWidgetItem(QString("%1  -  %2, %3 (MET %4)")
                                             .arg(qs(exercise->name()),
                                                  qs(categoryToString(exercise->category())),
                                                  typeLabel(*exercise).toLower(),
                                                  qs(validation::formatNumber(exercise->met()))));
        item->setData(kExerciseNameRole, qs(exercise->name()));
        libraryList_->addItem(item);
    }
}

void WorkoutsTab::populateWorkoutList(const QString& selectDate, const QString& selectName,
                                      const QString& keepExercise)
{
    {
        // no selection signals while the list is rebuilt
        const QSignalBlocker blocker(workoutList_);
        workoutList_->clear();

        int rowToSelect = -1;
        const auto& sessions = log_.sessions();
        for (auto it = sessions.rbegin(); it != sessions.rend(); ++it) { // newest first
            const QString date = qs(it->date());
            const QString name = qs(it->name());
            const std::size_t count = it->exerciseCount();
            auto* item = new QListWidgetItem(QString("%1  -  %2  (%3 exercise%4)")
                                                 .arg(date, name)
                                                 .arg(count)
                                                 .arg(count == 1 ? QString() : QString("s")));
            item->setData(kDateRole, date);
            item->setData(kNameRole, name);
            workoutList_->addItem(item);
            if (date == selectDate && name == selectName) {
                rowToSelect = workoutList_->count() - 1;
            }
        }
        if (rowToSelect >= 0) {
            workoutList_->setCurrentRow(rowToSelect);
        }
    }
    refreshWorkoutDetails(keepExercise);
}

void WorkoutsTab::refreshWorkoutDetails(const QString& keepExercise)
{
    const WorkoutSession* session = selectedWorkout();
    if (session) {
        selectedLabel_->setText(QString("Workout: %1  (%2)").arg(qs(session->name()), qs(session->date())));
    } else {
        selectedLabel_->setText("Select or create a workout on the left to start logging.");
    }
    populateExerciseTable(session, keepExercise);
    updateTotals(session);
}

void WorkoutsTab::populateExerciseTable(const WorkoutSession* session, const QString& keepExercise)
{
    const QSignalBlocker blocker(exerciseTable_);
    exerciseTable_->setRowCount(0);
    if (!session) {
        return;
    }

    int rowToSelect = -1;
    for (const auto& exercise : session->exercises()) {
        const int row = exerciseTable_->rowCount();
        exerciseTable_->insertRow(row);
        exerciseTable_->setItem(row, 0, new QTableWidgetItem(qs(exercise->name())));
        exerciseTable_->setItem(row, 1, new QTableWidgetItem(qs(categoryToString(exercise->category()))));
        exerciseTable_->setItem(row, 2, new QTableWidgetItem(qs(exercise->describe())));
        auto* kcalItem =
            new QTableWidgetItem(oneDecimal(exercise->caloriesBurned(profile_.weightKg())));
        kcalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        exerciseTable_->setItem(row, 3, kcalItem);

        if (!keepExercise.isEmpty() && qs(exercise->name()) == keepExercise) {
            rowToSelect = row;
        }
    }
    if (rowToSelect >= 0) {
        exerciseTable_->setCurrentCell(rowToSelect, 0);
    }
}

void WorkoutsTab::updateTotals(const WorkoutSession* session)
{
    if (!session) {
        totalLabel_->clear();
        return;
    }
    const double kcal = session->totalCaloriesBurned(profile_.weightKg());
    totalLabel_->setText(QString("Total calories burned in this workout: %1 kcal "
                                 "(estimated for %2 kg body weight)")
                             .arg(oneDecimal(kcal), qs(validation::formatNumber(profile_.weightKg()))));
}

void WorkoutsTab::refreshCalories()
{
    refreshWorkoutDetails(selectedExerciseName());
}

// ---- what is selected -----------------------------------------------------------------------------

WorkoutSession* WorkoutsTab::selectedWorkout()
{
    const QListWidgetItem* item = workoutList_->currentItem();
    if (!item) {
        return nullptr;
    }
    return log_.find(item->data(kDateRole).toString().toStdString(),
                     item->data(kNameRole).toString().toStdString());
}

QString WorkoutsTab::selectedExerciseName() const
{
    const int row = exerciseTable_->currentRow();
    if (row < 0 || !exerciseTable_->item(row, 0)) {
        return QString();
    }
    return exerciseTable_->item(row, 0)->text();
}

Exercise* WorkoutsTab::selectedExercise()
{
    WorkoutSession* session = selectedWorkout();
    const QString name = selectedExerciseName();
    if (!session || name.isEmpty()) {
        return nullptr;
    }
    return session->findExercise(name.toStdString());
}

// ---- user actions -----------------------------------------------------------------------------------

void WorkoutsTab::onCreateWorkout()
{
    clearError();
    try {
        WorkoutSession session(workoutNameEdit_->text().toStdString(),
                               workoutDateEdit_->date().toString(Qt::ISODate).toStdString());
        const QString date = qs(session.date());
        const QString name = qs(session.name());

        log_.add(std::move(session)); // throws if the same date + name already exists
        workoutNameEdit_->clear();
        populateWorkoutList(date, name);
        if (persist()) {
            emit statusMessage("Created workout '" + name + "'.");
        }
    } catch (const TrackerError& error) {
        showError(qs(error.what()));
    }
}

void WorkoutsTab::onDeleteWorkout()
{
    clearError();
    const QListWidgetItem* item = workoutList_->currentItem();
    if (!item) {
        showError("Select a workout to delete.");
        return;
    }
    const QString date = item->data(kDateRole).toString();
    const QString name = item->data(kNameRole).toString();

    if (confirmDeletes_) {
        const auto answer = QMessageBox::question(
            this, "Delete workout",
            QString("Delete '%1' on %2 and everything logged in it?").arg(name, date));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    log_.remove(date.toStdString(), name.toStdString());
    populateWorkoutList();
    if (persist()) {
        emit statusMessage("Deleted workout '" + name + "'.");
    }
}

void WorkoutsTab::onWorkoutSelectionChanged()
{
    clearError();
    refreshWorkoutDetails(QString());
}

void WorkoutsTab::onFilterChanged()
{
    populateLibraryList();
}

void WorkoutsTab::onAddExercise()
{
    clearError();
    const WorkoutSession* session = selectedWorkout();
    if (!session) {
        showError("Create or select a workout first.");
        return;
    }
    const QListWidgetItem* libraryItem = libraryList_->currentItem();
    if (!libraryItem) {
        showError("Select an exercise from the library first.");
        return;
    }

    const QString exerciseName = libraryItem->data(kExerciseNameRole).toString();
    const auto prototype = library_.find(exerciseName.toStdString());
    if (!prototype) {
        showError("That exercise is no longer in the library.");
        return;
    }

    const QString date = qs(session->date());
    const QString workoutName = qs(session->name());
    try {
        // the workout gets its own copy; the library entry stays untouched
        selectedWorkout()->addExercise(prototype->clone());
    } catch (const TrackerError& error) {
        showError(qs(error.what()));
        return;
    }

    populateWorkoutList(date, workoutName, exerciseName);
    persist();
}

void WorkoutsTab::onRemoveExercise()
{
    clearError();
    WorkoutSession* session = selectedWorkout();
    if (!session) {
        showError("Create or select a workout first.");
        return;
    }
    const Exercise* exercise = selectedExercise();
    if (!exercise) {
        showError("Select an exercise in the workout to remove.");
        return;
    }

    const std::string exerciseName = exercise->name(); // copy: the exercise is about to be destroyed
    const QString date = qs(session->date());
    const QString workoutName = qs(session->name());

    session->removeExercise(exerciseName);
    populateWorkoutList(date, workoutName);
    if (persist()) {
        emit statusMessage("Removed '" + qs(exerciseName) + "'.");
    }
}

void WorkoutsTab::onExerciseSelectionChanged()
{
    clearError();
}

// ---- helpers ------------------------------------------------------------------------------------------

bool WorkoutsTab::persist()
{
    try {
        dataManager_.saveWorkouts(log_);
        return true;
    } catch (const DataFileError& error) {
        showError(qs(error.what()));
        return false;
    }
}

void WorkoutsTab::showError(const QString& text)
{
    errorLabel_->setText(text);
}

void WorkoutsTab::clearError()
{
    errorLabel_->clear();
}
