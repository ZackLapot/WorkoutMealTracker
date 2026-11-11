#pragma once

#include "core/DataManager.h"
#include "core/ExerciseLibrary.h"
#include "core/UserProfile.h"
#include "core/WorkoutLog.h"

#include <QMainWindow>

#include <string>

class QFrame;
class QLabel;
class QTabWidget;

// The application window: a tab widget on top of the core classes.
//
// The window owns the program's data (profile, exercise library, workout log)
// and loads it from the data folder at startup. The tabs only get references to
// that data, so every tab always sees the same objects.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const std::string& dataDir, QWidget* parent = nullptr);

private slots:
    void showStatus(const QString& text);
    void showAbout();

private:
    void loadData();
    void buildUi();
    void showBanner(const QString& text, bool isError);

    // Data first: the tabs created in buildUi() keep references to these members.
    DataManager dataManager_;
    UserProfile profile_;
    ExerciseLibrary library_;
    WorkoutLog log_;

    QTabWidget* tabs_ = nullptr;
    QFrame* bannerFrame_ = nullptr;
    QLabel* bannerLabel_ = nullptr;
    QString startupNotice_;
    bool startupNoticeIsError_ = false;
};
