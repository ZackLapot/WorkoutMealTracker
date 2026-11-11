#pragma once

#include "core/DataManager.h"
#include "core/UserProfile.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

// Tab where the user enters their details (name, age, height, weight, calorie goal).
//
// The fields are plain text boxes on purpose: whatever is typed is checked with the
// core validation code and a clear error message is shown under the form.
class ProfileTab : public QWidget {
    Q_OBJECT

public:
    ProfileTab(UserProfile& profile, const DataManager& dataManager, QWidget* parent = nullptr);

signals:
    // Emitted after the profile was validated and saved successfully.
    void profileSaved();
    // Short success text for the status bar.
    void statusMessage(const QString& text);

private slots:
    void onSaveClicked();

private:
    void showProfile();
    void showError(const QString& text);

    UserProfile& profile_;
    const DataManager& dataManager_;

    QLineEdit* nameEdit_;
    QLineEdit* ageEdit_;
    QLineEdit* heightEdit_;
    QLineEdit* weightEdit_;
    QLineEdit* goalEdit_;
    QLabel* bmiLabel_;
    QLabel* errorLabel_;
    QPushButton* saveButton_;
};
