#include "DataManager.h"

#include "CardioExercise.h"
#include "Errors.h"
#include "StrengthExercise.h"
#include "Validation.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QString>

#include <climits>
#include <cmath>
#include <memory>

namespace {

constexpr int kFormatVersion = 1;

// Thrown internally when a file simply does not exist (not an error on first start).
struct MissingFile {};

QString q(const std::string& text) { return QString::fromStdString(text); }
std::string s(const QString& text) { return text.toStdString(); }

// ---- reading JSON -------------------------------------------------------------------

QJsonObject readJsonObject(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        throw MissingFile{};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        throw DataFileError("the file cannot be opened: " + s(file.errorString()));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        throw DataFileError("it is not valid JSON: " + s(parseError.errorString()));
    }
    if (!document.isObject()) {
        throw DataFileError("the top level of the file is not a JSON object");
    }

    const QJsonObject root = document.object();
    if (root.value("version").toInt(-1) != kFormatVersion) {
        throw DataFileError("the file has a missing or unsupported format version");
    }
    return root;
}

// Typed access to one field; throws ValidationError naming the field if it is
// missing or has the wrong type.
std::string getString(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        throw ValidationError(std::string("field '") + key + "' is missing or not text");
    }
    return s(value.toString());
}

double getNumber(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        throw ValidationError(std::string("field '") + key + "' is missing or not a number");
    }
    return value.toDouble();
}

int getInt(const QJsonObject& object, const char* key)
{
    const double number = getNumber(object, key);
    if (number != std::floor(number) || number < INT_MIN || number > INT_MAX) {
        throw ValidationError(std::string("field '") + key + "' is not a whole number");
    }
    return static_cast<int>(number);
}

// ---- JSON <-> domain objects ----------------------------------------------------------

QJsonObject profileToJson(const UserProfile& profile)
{
    QJsonObject root;
    root["version"] = kFormatVersion;
    root["name"] = q(profile.name());
    root["age"] = profile.age();
    root["heightCm"] = profile.heightCm();
    root["weightKg"] = profile.weightKg();
    root["dailyCalorieGoal"] = profile.dailyCalorieGoal();
    return root;
}

UserProfile profileFromJson(const QJsonObject& root)
{
    // the UserProfile constructor re-checks every range
    return UserProfile(getString(root, "name"), getInt(root, "age"), getNumber(root, "heightCm"),
                       getNumber(root, "weightKg"), getInt(root, "dailyCalorieGoal"));
}

QJsonObject exerciseToJson(const Exercise& exercise)
{
    QJsonObject object;
    object["name"] = q(exercise.name());
    object["type"] = q(exercise.typeName());
    object["category"] = q(categoryToString(exercise.category()));
    object["met"] = exercise.met();

    if (const auto* strength = dynamic_cast<const StrengthExercise*>(&exercise)) {
        QJsonArray sets;
        for (const StrengthExercise::SetEntry& set : strength->sets()) {
            QJsonObject entry;
            entry["reps"] = set.reps;
            entry["weightKg"] = set.weightKg;
            sets.append(entry);
        }
        object["sets"] = sets;
    } else if (const auto* cardio = dynamic_cast<const CardioExercise*>(&exercise)) {
        if (cardio->isLogged()) {
            object["durationMin"] = cardio->durationMin();
            object["distanceKm"] = cardio->distanceKm();
        }
    }
    return object;
}

std::unique_ptr<Exercise> exerciseFromJson(const QJsonObject& object)
{
    const std::string name = getString(object, "name");
    const std::string type = getString(object, "type");
    const Category category = categoryFromString(getString(object, "category"));
    const double met = getNumber(object, "met");

    if (type == "strength") {
        auto exercise = std::make_unique<StrengthExercise>(name, category, met);
        if (object.contains("sets")) {
            if (!object.value("sets").isArray()) {
                throw ValidationError("field 'sets' is not a list");
            }
            for (const QJsonValue& value : object.value("sets").toArray()) {
                if (!value.isObject()) {
                    throw ValidationError("a set in '" + name + "' is not an object");
                }
                exercise->addSet(getInt(value.toObject(), "reps"),
                                 getNumber(value.toObject(), "weightKg"));
            }
        }
        return exercise;
    }

    if (type == "cardio") {
        if (category != Category::Cardio) {
            throw ValidationError("cardio exercise '" + name + "' must use the Cardio category");
        }
        auto exercise = std::make_unique<CardioExercise>(name, met);
        if (object.contains("durationMin")) {
            const double distance = object.contains("distanceKm") ? getNumber(object, "distanceKm")
                                                                  : 0.0;
            exercise->logSession(getNumber(object, "durationMin"), distance);
        }
        return exercise;
    }

    throw ValidationError("unknown exercise type '" + type + "'");
}

QJsonObject workoutToJson(const WorkoutSession& session)
{
    QJsonArray exercises;
    for (const auto& exercise : session.exercises()) {
        exercises.append(exerciseToJson(*exercise));
    }

    QJsonObject object;
    object["name"] = q(session.name());
    object["date"] = q(session.date());
    object["exercises"] = exercises;
    return object;
}

WorkoutSession workoutFromJson(const QJsonObject& object)
{
    WorkoutSession session(getString(object, "name"), getString(object, "date"));

    const QJsonValue exercises = object.value("exercises");
    if (!exercises.isArray()) {
        throw ValidationError("field 'exercises' is missing or not a list");
    }
    for (const QJsonValue& value : exercises.toArray()) {
        if (!value.isObject()) {
            throw ValidationError("an exercise entry is not an object");
        }
        session.addExercise(exerciseFromJson(value.toObject()));
    }
    return session;
}

// ---- damaged files ----------------------------------------------------------------------

// Renames a damaged file to <name>.corrupt (replacing an older backup) so it is
// kept for inspection but never read again. Returns false if the rename failed.
bool quarantine(const QString& path)
{
    const QString backup = path + ".corrupt";
    QFile::remove(backup);
    return QFile::rename(path, backup);
}

std::string damagedMessage(const QString& path, const std::string& reason,
                           const std::string& fallbackDescription)
{
    const QString fileName = QFileInfo(path).fileName();
    const bool renamed = quarantine(path);

    std::string message = "'" + s(fileName) + "' could not be used (" + reason + "). ";
    if (renamed) {
        message += "The damaged file was kept as '" + s(fileName) + ".corrupt'. ";
    }
    return message + fallbackDescription;
}

// ---- writing JSON ----------------------------------------------------------------------

void writeJson(const QString& dir, const QString& path, const QJsonObject& root,
               const std::string& what)
{
    if (!QDir().mkpath(dir)) {
        throw DataFileError("Could not create the data folder '" + s(dir) + "'.");
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        throw DataFileError("Could not save the " + what + " to '" + s(path) + "': " +
                            s(file.errorString()));
    }

    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        throw DataFileError("Could not save the " + what + " to '" + s(path) + "': " +
                            s(file.errorString()));
    }
}

} // namespace

// ---- DataManager --------------------------------------------------------------------------

DataManager::DataManager(std::string dataDir)
    : dataDir_(validation::requireText(dataDir, "Data folder", 4096))
{
}

std::string DataManager::profilePath() const
{
    return s(QDir(q(dataDir_)).filePath(kProfileFile));
}

std::string DataManager::workoutsPath() const
{
    return s(QDir(q(dataDir_)).filePath(kWorkoutsFile));
}

std::string DataManager::libraryPath() const
{
    return s(QDir(q(dataDir_)).filePath(kLibraryFile));
}

LoadResult<UserProfile> DataManager::loadProfile() const
{
    const QString path = q(profilePath());
    LoadResult<UserProfile> result;
    try {
        result.value = profileFromJson(readJsonObject(path));
    } catch (const MissingFile&) {
        result.status = LoadStatus::Missing;
        result.message = "No profile found yet. A default profile is being used; "
                         "enter your details in the Profile tab.";
    } catch (const TrackerError& error) {
        result.value = UserProfile();
        result.status = LoadStatus::Damaged;
        result.message = damagedMessage(path, error.what(), "A default profile is being used.");
    }
    return result;
}

LoadResult<ExerciseLibrary> DataManager::loadLibrary() const
{
    const QString path = q(libraryPath());
    LoadResult<ExerciseLibrary> result;
    try {
        const QJsonObject root = readJsonObject(path);
        const QJsonValue list = root.value("exercises");
        if (!list.isArray()) {
            throw DataFileError("the 'exercises' list is missing");
        }

        int skipped = 0;
        std::string firstProblem;
        for (const QJsonValue& value : list.toArray()) {
            try {
                if (!value.isObject()) {
                    throw ValidationError("an exercise entry is not an object");
                }
                result.value.add(exerciseFromJson(value.toObject()));
            } catch (const TrackerError& error) {
                ++skipped;
                if (firstProblem.empty()) {
                    firstProblem = error.what();
                }
            }
        }

        if (result.value.empty()) {
            throw DataFileError("it contains no usable exercises");
        }
        if (skipped > 0) {
            result.status = LoadStatus::Partial;
            result.message = "Skipped " + std::to_string(skipped) +
                             " invalid exercise(s) in the exercise library (first problem: " +
                             firstProblem + ").";
        }
    } catch (const MissingFile&) {
        result.value = ExerciseLibrary::builtInDefaults();
        result.status = LoadStatus::Missing;
        result.message = "No exercise library file found. The built-in exercise list is being used.";
    } catch (const TrackerError& error) {
        result.value = ExerciseLibrary::builtInDefaults();
        result.status = LoadStatus::Damaged;
        result.message = damagedMessage(path, error.what(),
                                        "The built-in exercise list is being used.");
    }
    return result;
}

LoadResult<WorkoutLog> DataManager::loadWorkouts() const
{
    const QString path = q(workoutsPath());
    LoadResult<WorkoutLog> result;
    try {
        const QJsonObject root = readJsonObject(path);
        const QJsonValue list = root.value("workouts");
        if (!list.isArray()) {
            throw DataFileError("the 'workouts' list is missing");
        }

        int skipped = 0;
        std::string firstProblem;
        for (const QJsonValue& value : list.toArray()) {
            try {
                if (!value.isObject()) {
                    throw ValidationError("a workout entry is not an object");
                }
                result.value.add(workoutFromJson(value.toObject()));
            } catch (const TrackerError& error) {
                ++skipped;
                if (firstProblem.empty()) {
                    firstProblem = error.what();
                }
            }
        }

        if (skipped > 0) {
            result.status = LoadStatus::Partial;
            result.message = "Skipped " + std::to_string(skipped) +
                             " invalid workout(s) while loading (first problem: " + firstProblem +
                             ").";
        }
    } catch (const MissingFile&) {
        result.value = WorkoutLog();
        result.status = LoadStatus::Missing;
        result.message = "No saved workouts yet.";
    } catch (const TrackerError& error) {
        result.value = WorkoutLog();
        result.status = LoadStatus::Damaged;
        result.message = damagedMessage(path, error.what(), "Starting with an empty workout list.");
    }
    return result;
}

void DataManager::saveProfile(const UserProfile& profile) const
{
    writeJson(q(dataDir_), q(profilePath()), profileToJson(profile), "profile");
}

void DataManager::saveLibrary(const ExerciseLibrary& library) const
{
    QJsonArray exercises;
    for (const auto& exercise : library.all()) {
        exercises.append(exerciseToJson(*exercise));
    }
    QJsonObject root;
    root["version"] = kFormatVersion;
    root["exercises"] = exercises;
    writeJson(q(dataDir_), q(libraryPath()), root, "exercise library");
}

void DataManager::saveWorkouts(const WorkoutLog& log) const
{
    QJsonArray workouts;
    for (const WorkoutSession& session : log.sessions()) {
        workouts.append(workoutToJson(session));
    }
    QJsonObject root;
    root["version"] = kFormatVersion;
    root["workouts"] = workouts;
    writeJson(q(dataDir_), q(workoutsPath()), root, "workouts");
}
