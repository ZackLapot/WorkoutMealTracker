#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Workout & Meal Tracker");
    window.resize(900, 600);
    window.show();

    return app.exec();
}
