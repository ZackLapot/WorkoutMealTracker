// GUI tests: drive the real MainWindow (offscreen) the way a user would - type into
// fields, click buttons - and check what the screen shows and what gets saved.

#include "TestUtils.h"
#include "core/DataManager.h"
#include "gui/MainWindow.h"

#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
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
};

// ---- start-up ----------------------------------------------------------------------------

void TstGui::startsWithEmptyDataFolder()
{
    Fixture f;
    QCOMPARE(f.window->windowTitle(), QStringLiteral("Workout & Meal Tracker"));

    auto* tabs = f.find<QTabWidget>("tabs");
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Profile"));

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

QTEST_MAIN(TstGui)
#include "tst_gui.moc"
