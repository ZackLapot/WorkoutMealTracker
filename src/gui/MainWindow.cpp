#include "MainWindow.h"

#include "ProfileTab.h"
#include "WorkoutsTab.h"

#include "core/Errors.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <utility>

MainWindow::MainWindow(const std::string& dataDir, QWidget* parent)
    : QMainWindow(parent), dataManager_(dataDir)
{
    loadData();
    buildUi();

    if (!startupNotice_.isEmpty()) {
        showBanner(startupNotice_, startupNoticeIsError_);
    }
}

void MainWindow::loadData()
{
    QStringList notices;

    // Profile: a missing file is normal on first start, a damaged one is a problem.
    {
        LoadResult<UserProfile> result = dataManager_.loadProfile();
        profile_ = result.value;
        if (result.status != LoadStatus::Ok) {
            notices << QString::fromStdString(result.message);
            startupNoticeIsError_ = startupNoticeIsError_ || result.status != LoadStatus::Missing;
        }
    }

    // Exercise library: fall back to the built-in list, and write the file back
    // so the user has a file to look at (and edit).
    {
        LoadResult<ExerciseLibrary> result = dataManager_.loadLibrary();
        library_ = std::move(result.value);
        if (result.status == LoadStatus::Missing || result.status == LoadStatus::Damaged) {
            try {
                dataManager_.saveLibrary(library_);
            } catch (const DataFileError& error) {
                notices << QString::fromStdString(error.what());
                startupNoticeIsError_ = true;
            }
        }
        if (result.status == LoadStatus::Damaged || result.status == LoadStatus::Partial) {
            notices << QString::fromStdString(result.message);
            startupNoticeIsError_ = true;
        }
    }

    // Workouts: no file simply means no workouts yet.
    {
        LoadResult<WorkoutLog> result = dataManager_.loadWorkouts();
        log_ = std::move(result.value);
        if (result.status == LoadStatus::Damaged || result.status == LoadStatus::Partial) {
            notices << QString::fromStdString(result.message);
            startupNoticeIsError_ = true;
        }
    }

    startupNotice_ = notices.join("\n");
}

void MainWindow::buildUi()
{
    setWindowTitle("Workout & Meal Tracker");
    resize(1000, 760);

    // Menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* quitAction = fileMenu->addAction("E&xit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    // Central widget: notice banner on top, tabs below
    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);

    bannerFrame_ = new QFrame;
    bannerFrame_->setObjectName("banner");
    auto* bannerLayout = new QHBoxLayout(bannerFrame_);
    bannerLabel_ = new QLabel;
    bannerLabel_->setObjectName("bannerLabel");
    bannerLabel_->setWordWrap(true);
    auto* dismissButton = new QPushButton("Dismiss");
    dismissButton->setObjectName("bannerDismissButton");
    bannerLayout->addWidget(bannerLabel_, 1);
    bannerLayout->addWidget(dismissButton, 0, Qt::AlignTop);
    connect(dismissButton, &QPushButton::clicked, bannerFrame_, &QWidget::hide);
    bannerFrame_->hide();
    layout->addWidget(bannerFrame_);

    tabs_ = new QTabWidget;
    tabs_->setObjectName("tabs");
    layout->addWidget(tabs_, 1);

    profileTab_ = new ProfileTab(profile_, dataManager_);
    tabs_->addTab(profileTab_, "Profile");
    connect(profileTab_, &ProfileTab::statusMessage, this, &MainWindow::showStatus);

    workoutsTab_ = new WorkoutsTab(library_, log_, profile_, dataManager_);
    tabs_->addTab(workoutsTab_, "Workouts");
    connect(workoutsTab_, &WorkoutsTab::statusMessage, this, &MainWindow::showStatus);

    // calories depend on body weight, so the Workouts tab must recalculate after a profile change
    connect(profileTab_, &ProfileTab::profileSaved, workoutsTab_, &WorkoutsTab::refreshCalories);

    setCentralWidget(central);

    statusBar()->showMessage("Data folder: " + QString::fromStdString(dataManager_.dataDir()));
}

void MainWindow::showBanner(const QString& text, bool isError)
{
    bannerFrame_->setStyleSheet(
        isError ? "QFrame#banner { background: #fdecea; border: 1px solid #e5a29d; border-radius: 4px; }"
                : "QFrame#banner { background: #e8f1fb; border: 1px solid #a9c8ea; border-radius: 4px; }");
    bannerLabel_->setText(text);
    bannerFrame_->show();
}

void MainWindow::showStatus(const QString& text)
{
    statusBar()->showMessage(text, 6000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Workout & Meal Tracker",
                       "Workout & Meal Tracker\n\n"
                       "CST8219 - C++ Programming, individual project.\n"
                       "Logs workouts and meals and compares calories in and out "
                       "against a daily goal.");
}
