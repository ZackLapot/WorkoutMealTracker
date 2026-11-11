#include "gui/MainWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QString>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QApplication::setApplicationName("WorkoutMealTracker");
    QApplication::setOrganizationName("CST8219");

    // By default the data files live in the standard per-user data folder.
    // --data-dir lets you point the program at another folder (handy for testing).
    QCommandLineParser parser;
    parser.setApplicationDescription("Workout & Meal Tracker");
    parser.addHelpOption();
    QCommandLineOption dataDirOption("data-dir", "Folder that holds the JSON data files.", "folder");
    parser.addOption(dataDirOption);
    parser.process(app);

    QString dataDir = parser.value(dataDirOption);
    if (dataDir.isEmpty()) {
        dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    MainWindow window(dataDir.toStdString());
    window.show();

    return app.exec();
}
