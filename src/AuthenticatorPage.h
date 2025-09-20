#ifndef AUTHENTICATORPAGE_H
#define AUTHENTICATORPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTimer>
#include <QSet>
#include <QDateTime>  // Needed for QDateTime return type

class AuthenticatorPage : public QWidget
{
    Q_OBJECT

public:
    explicit AuthenticatorPage(QWidget* parent = nullptr);

private slots:
    void onSaveSettingsClicked();
    void onDeleteSelectedClicked();
    void onClearAllClicked();
    void onTestEmailClicked();
    void checkFailedLogins();

private:
    // UI Elements
    QTableWidget* logTable;
    QLineEdit* emailField;
    QSpinBox* limitBox;
    QPushButton* saveButton;
    QPushButton* deleteSelectedButton;
    QPushButton* clearAllButton;
    QPushButton* testEmailButton;

    // State
    QTimer* checkTimer;
    int failedAttemptCount;
    QSet<quint64> seenEventRecords;
    bool alertSentRecently = false;  // Prevent repeated alert emails in cooldown period

    // Methods
    void setupLayout();
    void saveSettings();
    void loadSettings();
    void addLogEntry(const QString& type, const QString& detail);
    void saveLogs();
    void loadLogs();
    void startMonitoring();
    quint64 getEventRecordId(const QString& rawXml);
    QDateTime getEventTime(const QString& rawXml);         // Get event time from XML event data
    void sendAlertEmail(int failedAttempts);               // Send alert email when threshold exceeded
};

#endif // AUTHENTICATORPAGE_H

