#include "ProfileTab.h"

#include "core/Errors.h"
#include "core/Validation.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString qs(const std::string& text)
{
    return QString::fromStdString(text);
}

QLineEdit* makeEdit(const char* objectName, const QString& placeholder)
{
    auto* edit = new QLineEdit;
    edit->setObjectName(objectName);
    edit->setPlaceholderText(placeholder);
    return edit;
}

} // namespace

ProfileTab::ProfileTab(UserProfile& profile, const DataManager& dataManager, QWidget* parent)
    : QWidget(parent), profile_(profile), dataManager_(dataManager)
{
    nameEdit_ = makeEdit("nameEdit", "Your name");
    ageEdit_ = makeEdit("ageEdit", "e.g. 28");
    heightEdit_ = makeEdit("heightEdit", "e.g. 175");
    weightEdit_ = makeEdit("weightEdit", "e.g. 72.5");
    goalEdit_ = makeEdit("goalEdit", "e.g. 2200");

    auto* form = new QFormLayout;
    form->addRow("Name", nameEdit_);
    form->addRow("Age (years)", ageEdit_);
    form->addRow("Height (cm)", heightEdit_);
    form->addRow("Weight (kg)", weightEdit_);
    form->addRow("Daily calorie goal (kcal)", goalEdit_);

    auto* group = new QGroupBox("Your profile");
    group->setLayout(form);

    auto* intro = new QLabel("Your weight is used to estimate the calories burned in your "
                             "workouts. The calorie goal is what your daily totals are "
                             "compared against.");
    intro->setWordWrap(true);

    bmiLabel_ = new QLabel;
    bmiLabel_->setObjectName("bmiLabel");

    errorLabel_ = new QLabel;
    errorLabel_->setObjectName("profileError");
    errorLabel_->setWordWrap(true);
    errorLabel_->setStyleSheet("color: #b00020; font-weight: bold;");
    errorLabel_->setMinimumHeight(errorLabel_->fontMetrics().height() * 2);
    errorLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    saveButton_ = new QPushButton("Save profile");
    saveButton_->setObjectName("saveProfileButton");
    saveButton_->setDefault(true);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(saveButton_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(group);
    layout->addWidget(bmiLabel_);
    layout->addWidget(errorLabel_);
    layout->addLayout(buttonRow);
    layout->addStretch(1);

    connect(saveButton_, &QPushButton::clicked, this, &ProfileTab::onSaveClicked);
    for (QLineEdit* edit : {nameEdit_, ageEdit_, heightEdit_, weightEdit_, goalEdit_}) {
        connect(edit, &QLineEdit::returnPressed, this, &ProfileTab::onSaveClicked);
    }

    showProfile();
}

void ProfileTab::showProfile()
{
    nameEdit_->setText(qs(profile_.name()));
    ageEdit_->setText(QString::number(profile_.age()));
    heightEdit_->setText(qs(validation::formatNumber(profile_.heightCm())));
    weightEdit_->setText(qs(validation::formatNumber(profile_.weightKg())));
    goalEdit_->setText(QString::number(profile_.dailyCalorieGoal()));
    bmiLabel_->setText("BMI: " + qs(validation::formatNumber(profile_.bmi(), 1)));
    errorLabel_->clear();
}

void ProfileTab::showError(const QString& text)
{
    errorLabel_->setText(text);
}

void ProfileTab::onSaveClicked()
{
    try {
        // 1. turn the typed text into numbers (reports "must be a number" etc.)
        const std::string name = nameEdit_->text().toStdString();
        const int age = validation::parseInt(ageEdit_->text().toStdString(), "Age");
        const double heightCm = validation::parseDouble(heightEdit_->text().toStdString(), "Height");
        const double weightKg = validation::parseDouble(weightEdit_->text().toStdString(), "Weight");
        const int goal = validation::parseInt(goalEdit_->text().toStdString(), "Daily calorie goal");

        // 2. build a profile (reports "must be between ..."), then save it
        const UserProfile updated(name, age, heightCm, weightKg, goal);
        dataManager_.saveProfile(updated);

        // 3. only now replace the program's profile
        profile_ = updated;
        showProfile();
        emit profileSaved();
        emit statusMessage("Profile saved.");
    } catch (const TrackerError& error) {
        showError(qs(error.what()));
    }
}
