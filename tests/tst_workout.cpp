// Unit tests for WorkoutSession (ownership, copying, operator<, operator<<) and WorkoutLog.

#include "TestUtils.h"
#include "core/CardioExercise.h"
#include "core/Errors.h"
#include "core/StrengthExercise.h"
#include "core/WorkoutLog.h"
#include "core/WorkoutSession.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

namespace {

std::unique_ptr<StrengthExercise> makeBench(int sets = 3)
{
    auto bench = std::make_unique<StrengthExercise>("Bench Press", Category::Chest, 5.0);
    for (int i = 0; i < sets; ++i) {
        bench->addSet(10, 60.0);
    }
    return bench;
}

std::unique_ptr<CardioExercise> makeRun()
{
    auto run = std::make_unique<CardioExercise>("Running", 9.8);
    run->logSession(30.0, 5.0);
    return run;
}

// Session with 3x10 bench press (29.1667 kcal at 70 kg) and a 30 min run (343 kcal at 70 kg).
WorkoutSession makeFullSession(const std::string& name = "Push Day",
                               const std::string& date = "2026-11-10")
{
    WorkoutSession session(name, date);
    session.addExercise(makeBench());
    session.addExercise(makeRun());
    return session;
}

} // namespace

class TstWorkout : public QObject {
    Q_OBJECT

private slots:
    // WorkoutSession: construction
    void constructsWithTrimmedName();
    void rejectsBadNameOrDate();

    // WorkoutSession: contents
    void addExerciseReturnsReference();
    void duplicateExerciseIsRejected();
    void nullExerciseIsRejected();
    void removeExercise();
    void findExerciseIgnoresCase();
    void totalCaloriesAddsUpPolymorphically();
    void emptyWorkoutHasZeroCalories();
    void invalidBodyWeightIsRejectedEvenWhenEmpty();

    // WorkoutSession: copying
    void copyIsDeep();
    void copyAssignmentIsDeepAndSafeForSelfAssignment();
    void moveKeepsExercises();

    // operators
    void lessThanOrdersByDateThenName();
    void equivalentSessionsAreNotLessThanEachOther();
    void sortingUsesOperatorLessThan();
    void streamOutputIsReadable();

    // WorkoutLog
    void logKeepsSessionsSorted();
    void logRejectsDuplicateDateAndName();
    void logAllowsSameNameOnDifferentDates();
    void logFindAndRemove();
    void logCaloriesPerDay();
    void emptyLogIsHandled();
};

// ---- WorkoutSession: construction ------------------------------------------------------

void TstWorkout::constructsWithTrimmedName()
{
    const WorkoutSession session("  Push Day ", "2026-11-10");
    QCOMPARE(qs(session.name()), QStringLiteral("Push Day"));
    QCOMPARE(qs(session.date()), QStringLiteral("2026-11-10"));
    QVERIFY(session.empty());
}

void TstWorkout::rejectsBadNameOrDate()
{
    EXPECT_THROW_MSG(ValidationError, WorkoutSession("", "2026-11-10"), "Workout name is required.");
    EXPECT_THROW_MSG(ValidationError, WorkoutSession(std::string(61, 'x'), "2026-11-10"),
                     "at most 60 characters");
    EXPECT_THROW_MSG(ValidationError, WorkoutSession("Push", "2026-02-30"),
                     "Workout date must be a real date written as YYYY-MM-DD.");
    EXPECT_THROW_MSG(ValidationError, WorkoutSession("Push", ""), "Workout date");
    EXPECT_THROW_MSG(ValidationError, WorkoutSession("Push", "10/11/2026"), "Workout date");
}

// ---- WorkoutSession: contents ------------------------------------------------------------

void TstWorkout::addExerciseReturnsReference()
{
    WorkoutSession session("Push Day", "2026-11-10");
    Exercise& added = session.addExercise(makeBench(0));
    QCOMPARE(qs(added.name()), QStringLiteral("Bench Press"));
    QCOMPARE(static_cast<int>(session.exerciseCount()), 1);

    // the reference points at the exercise stored in the session
    dynamic_cast<StrengthExercise&>(added).addSet(5, 80.0);
    QCOMPARE(qs(session.exercises().front()->describe()), QStringLiteral("1 set: 5 x 80 kg"));
}

void TstWorkout::duplicateExerciseIsRejected()
{
    WorkoutSession session("Push Day", "2026-11-10");
    session.addExercise(makeBench());
    EXPECT_THROW_MSG(DuplicateEntryError,
                     session.addExercise(std::make_unique<StrengthExercise>("BENCH press", Category::Chest, 5.0)),
                     "'BENCH press' is already in this workout");
    QCOMPARE(static_cast<int>(session.exerciseCount()), 1);
    // the original keeps its three sets
    QCOMPARE(qs(session.findExercise("Bench Press")->describe()),
             QStringLiteral("3 sets: 10 x 60 kg, 10 x 60 kg, 10 x 60 kg"));
}

void TstWorkout::nullExerciseIsRejected()
{
    WorkoutSession session("Push Day", "2026-11-10");
    EXPECT_THROW_MSG(ValidationError, session.addExercise(nullptr), "empty exercise");
    QVERIFY(session.empty());
}

void TstWorkout::removeExercise()
{
    WorkoutSession session = makeFullSession();
    QVERIFY(session.removeExercise("running"));
    QCOMPARE(static_cast<int>(session.exerciseCount()), 1);
    QVERIFY(!session.removeExercise("Running")); // already removed
    QVERIFY(!session.removeExercise("Never added"));
    QVERIFY(session.findExercise("Running") == nullptr);
}

void TstWorkout::findExerciseIgnoresCase()
{
    const WorkoutSession session = makeFullSession();
    QVERIFY(session.findExercise("  bench press ") != nullptr);
    QVERIFY(session.findExercise("RUNNING") != nullptr);
    QVERIFY(session.findExercise("Deadlift") == nullptr);
}

void TstWorkout::totalCaloriesAddsUpPolymorphically()
{
    const WorkoutSession session = makeFullSession();
    QVERIFY_CLOSE(session.totalCaloriesBurned(70.0), 29.1667 + 343.0, 0.001);
}

void TstWorkout::emptyWorkoutHasZeroCalories()
{
    const WorkoutSession session("Rest Day", "2026-11-10");
    QCOMPARE(session.totalCaloriesBurned(70.0), 0.0);
}

void TstWorkout::invalidBodyWeightIsRejectedEvenWhenEmpty()
{
    const WorkoutSession empty("Rest Day", "2026-11-10");
    EXPECT_THROW_MSG(ValidationError, empty.totalCaloriesBurned(0.0), "Body weight must be between");
    const WorkoutSession full = makeFullSession();
    EXPECT_THROW_MSG(ValidationError, full.totalCaloriesBurned(-1.0), "Body weight must be between");
}

// ---- WorkoutSession: copying ----------------------------------------------------------------

void TstWorkout::copyIsDeep()
{
    WorkoutSession original = makeFullSession();
    WorkoutSession copy(original); // copy constructor

    QCOMPARE(static_cast<int>(copy.exerciseCount()), 2);
    QVERIFY(copy.exercises()[0].get() != original.exercises()[0].get()); // different objects

    // changing the copy leaves the original alone
    dynamic_cast<StrengthExercise*>(copy.findExercise("Bench Press"))->addSet(5, 100.0);
    QCOMPARE(static_cast<int>(dynamic_cast<StrengthExercise*>(original.findExercise("Bench Press"))->setCount()), 3);
    QCOMPARE(static_cast<int>(dynamic_cast<StrengthExercise*>(copy.findExercise("Bench Press"))->setCount()), 4);

    // and removing from the original leaves the copy alone
    original.removeExercise("Running");
    QCOMPARE(static_cast<int>(copy.exerciseCount()), 2);
}

void TstWorkout::copyAssignmentIsDeepAndSafeForSelfAssignment()
{
    WorkoutSession source = makeFullSession("Source", "2026-11-09");
    WorkoutSession target("Target", "2026-11-01");
    target.addExercise(std::make_unique<CardioExercise>("Cycling", 8.0));

    target = source;
    QCOMPARE(qs(target.name()), QStringLiteral("Source"));
    QCOMPARE(qs(target.date()), QStringLiteral("2026-11-09"));
    QCOMPARE(static_cast<int>(target.exerciseCount()), 2);
    QVERIFY(target.findExercise("Cycling") == nullptr); // old content replaced
    QVERIFY(target.exercises()[0].get() != source.exercises()[0].get());

    // self-assignment must not destroy the data
    WorkoutSession& alias = target;
    target = alias;
    QCOMPARE(static_cast<int>(target.exerciseCount()), 2);
    QVERIFY_CLOSE(target.totalCaloriesBurned(70.0), 29.1667 + 343.0, 0.001);
}

void TstWorkout::moveKeepsExercises()
{
    WorkoutSession source = makeFullSession();
    const Exercise* firstBefore = source.exercises()[0].get();

    WorkoutSession moved(std::move(source));
    QCOMPARE(static_cast<int>(moved.exerciseCount()), 2);
    QVERIFY(moved.exercises()[0].get() == firstBefore); // ownership moved, nothing copied
}

// ---- operators ------------------------------------------------------------------------------

void TstWorkout::lessThanOrdersByDateThenName()
{
    const WorkoutSession early("Zebra", "2026-11-01");
    const WorkoutSession late("Alpha", "2026-11-02");
    QVERIFY(early < late);  // earlier date wins even though the name sorts later
    QVERIFY(!(late < early));

    const WorkoutSession a("Alpha", "2026-11-02");
    const WorkoutSession b("beta", "2026-11-02");
    QVERIFY(a < b);         // same date: by name, ignoring case
    QVERIFY(!(b < a));
}

void TstWorkout::equivalentSessionsAreNotLessThanEachOther()
{
    const WorkoutSession a("Push Day", "2026-11-10");
    const WorkoutSession b("PUSH day", "2026-11-10");
    QVERIFY(!(a < b));
    QVERIFY(!(b < a));
    QVERIFY(!(a < a)); // irreflexive
}

void TstWorkout::sortingUsesOperatorLessThan()
{
    std::vector<WorkoutSession> sessions;
    sessions.push_back(makeFullSession("C", "2026-11-03"));
    sessions.push_back(makeFullSession("B", "2026-11-01"));
    sessions.push_back(makeFullSession("A", "2026-11-01"));
    sessions.push_back(makeFullSession("D", "2026-10-31"));

    std::sort(sessions.begin(), sessions.end());

    QStringList order;
    for (const WorkoutSession& s : sessions) {
        order << qs(s.name());
        QCOMPARE(static_cast<int>(s.exerciseCount()), 2); // sorting (moves) kept the exercises
    }
    QCOMPARE(order, (QStringList{"D", "A", "B", "C"}));
}

void TstWorkout::streamOutputIsReadable()
{
    std::ostringstream out;
    out << makeFullSession();
    const QString expected =
        "Push Day (2026-11-10) - 2 exercises\n"
        "  Bench Press [strength, Chest]: 3 sets: 10 x 60 kg, 10 x 60 kg, 10 x 60 kg\n"
        "  Running [cardio, Cardio]: 30 min, 5 km (10 km/h)";
    QCOMPARE(qs(out.str()), expected);

    std::ostringstream emptyOut;
    emptyOut << WorkoutSession("Rest", "2026-11-11");
    QCOMPARE(qs(emptyOut.str()), QStringLiteral("Rest (2026-11-11) - 0 exercises"));

    WorkoutSession single("Solo", "2026-11-12");
    single.addExercise(makeRun());
    std::ostringstream singleOut;
    singleOut << single;
    QVERIFY(qs(singleOut.str()).startsWith("Solo (2026-11-12) - 1 exercise\n"));
}

// ---- WorkoutLog ---------------------------------------------------------------------------------

void TstWorkout::logKeepsSessionsSorted()
{
    WorkoutLog log;
    log.add(makeFullSession("Legs", "2026-11-08"));
    log.add(makeFullSession("Push", "2026-11-10"));
    log.add(makeFullSession("Pull", "2026-11-09"));
    log.add(makeFullSession("Cardio", "2026-11-09"));

    QStringList order;
    for (const WorkoutSession& s : log.sessions()) {
        order << qs(s.date()) + " " + qs(s.name());
    }
    QCOMPARE(order, (QStringList{"2026-11-08 Legs", "2026-11-09 Cardio", "2026-11-09 Pull",
                                 "2026-11-10 Push"}));
}

void TstWorkout::logRejectsDuplicateDateAndName()
{
    WorkoutLog log;
    log.add(makeFullSession("Push Day", "2026-11-10"));
    EXPECT_THROW_MSG(DuplicateEntryError, log.add(makeFullSession("push DAY", "2026-11-10")),
                     "A workout named 'push DAY' already exists on 2026-11-10.");
    QCOMPARE(static_cast<int>(log.size()), 1);
}

void TstWorkout::logAllowsSameNameOnDifferentDates()
{
    WorkoutLog log;
    log.add(makeFullSession("Push Day", "2026-11-10"));
    log.add(makeFullSession("Push Day", "2026-11-12"));      // different date: fine
    log.add(makeFullSession("Pull Day", "2026-11-10"));      // different name: fine
    QCOMPARE(static_cast<int>(log.size()), 3);
}

void TstWorkout::logFindAndRemove()
{
    WorkoutLog log;
    log.add(makeFullSession("Push Day", "2026-11-10"));
    log.add(makeFullSession("Pull Day", "2026-11-10"));

    WorkoutSession* found = log.find("2026-11-10", "PULL day");
    QVERIFY(found != nullptr);
    QCOMPARE(qs(found->name()), QStringLiteral("Pull Day"));
    QVERIFY(log.find("2026-11-11", "Pull Day") == nullptr);

    // changes made through the pointer are stored in the log
    found->removeExercise("Running");
    QCOMPARE(static_cast<int>(log.find("2026-11-10", "Pull Day")->exerciseCount()), 1);

    QVERIFY(log.remove("2026-11-10", "push day"));
    QVERIFY(!log.remove("2026-11-10", "push day"));
    QCOMPARE(static_cast<int>(log.size()), 1);
}

void TstWorkout::logCaloriesPerDay()
{
    WorkoutLog log;
    log.add(makeFullSession("Push Day", "2026-11-10"));
    log.add(makeFullSession("Pull Day", "2026-11-10"));
    log.add(makeFullSession("Legs", "2026-11-11"));

    QCOMPARE(static_cast<int>(log.sessionsOn("2026-11-10").size()), 2);
    // two sessions that day, each 29.1667 + 343.0 kcal at 70 kg
    QVERIFY_CLOSE(log.caloriesBurnedOn("2026-11-10", 70.0), 2 * (29.1667 + 343.0), 0.002);
    QVERIFY_CLOSE(log.caloriesBurnedOn("2026-11-11", 70.0), 29.1667 + 343.0, 0.001);
    QCOMPARE(log.caloriesBurnedOn("2026-11-12", 70.0), 0.0); // nothing logged that day
    EXPECT_THROW_MSG(ValidationError, log.caloriesBurnedOn("2026-11-10", 5.0), "Body weight");
}

void TstWorkout::emptyLogIsHandled()
{
    WorkoutLog log;
    QVERIFY(log.empty());
    QVERIFY(log.sessionsOn("2026-11-10").empty());
    QCOMPARE(log.caloriesBurnedOn("2026-11-10", 70.0), 0.0);
    QVERIFY(log.find("2026-11-10", "Anything") == nullptr);
    QVERIFY(!log.remove("2026-11-10", "Anything"));
}

QTEST_APPLESS_MAIN(TstWorkout)
#include "tst_workout.moc"
