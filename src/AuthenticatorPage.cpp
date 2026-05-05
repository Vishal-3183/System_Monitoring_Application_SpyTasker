#include "AuthenticatorPage.h"
#include "EmailSender.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QHeaderView>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QRegularExpression>
#include <QDebug>

// Windows Event Log headers
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")

AuthenticatorPage::AuthenticatorPage(QWidget* parent)
    : QWidget(parent)
{
    setupLayout();
    loadSettings();
    loadLogs();
    startMonitoring();
}

void AuthenticatorPage::setupLayout()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    logTable = new QTableWidget();
    logTable->setColumnCount(3);
    logTable->setHorizontalHeaderLabels({ "Timestamp", "Type", "Detail" });
    logTable->horizontalHeader()->setStretchLastSection(true);
    logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(logTable);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    emailField = new QLineEdit();
    emailField->setPlaceholderText("Enter email address");

    limitBox = new QSpinBox();
    limitBox->setRange(1, 100);
    limitBox->setPrefix("Limit: ");

    saveButton = new QPushButton("Save Settings");
    connect(saveButton, &QPushButton::clicked, this, &AuthenticatorPage::onSaveSettingsClicked);

    inputLayout->addWidget(emailField);
    inputLayout->addWidget(limitBox);
    inputLayout->addWidget(saveButton);
    mainLayout->addLayout(inputLayout);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    deleteSelectedButton = new QPushButton("Delete Selected");
    clearAllButton = new QPushButton("Clear All Logs");
    connect(deleteSelectedButton, &QPushButton::clicked, this, &AuthenticatorPage::onDeleteSelectedClicked);
    connect(clearAllButton, &QPushButton::clicked, this, &AuthenticatorPage::onClearAllClicked);

    buttonLayout->addWidget(deleteSelectedButton);
    buttonLayout->addWidget(clearAllButton);
    mainLayout->addLayout(buttonLayout);

    testEmailButton = new QPushButton("Test Email");
    connect(testEmailButton, &QPushButton::clicked, this, &AuthenticatorPage::onTestEmailClicked);
    mainLayout->addWidget(testEmailButton);
}

void AuthenticatorPage::onSaveSettingsClicked()
{
    saveSettings();
}

void AuthenticatorPage::saveSettings()
{
    QSettings settings("Spy_Tasker", "Authenticator");
    settings.setValue("authenticator/email", emailField->text());
    settings.setValue("authenticator/limit", limitBox->value());
}

void AuthenticatorPage::loadSettings()
{
    QSettings settings("Spy_Tasker", "Authenticator");
    emailField->setText(settings.value("authenticator/email", "").toString());
    limitBox->setValue(settings.value("authenticator/limit", 3).toInt());
}

void AuthenticatorPage::addLogEntry(const QString& type, const QString& detail)
{
    int row = logTable->rowCount();
    logTable->insertRow(row);
    logTable->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString()));
    logTable->setItem(row, 1, new QTableWidgetItem(type));
    logTable->setItem(row, 2, new QTableWidgetItem(detail));
    saveLogs();
}

void AuthenticatorPage::onClearAllClicked()
{
    logTable->setRowCount(0);
    QFile::remove("auth_logs.json");
}

void AuthenticatorPage::onDeleteSelectedClicked()
{
    QList<QTableWidgetSelectionRange> ranges = logTable->selectedRanges();
    for (const QTableWidgetSelectionRange& range : ranges) {
        for (int row = range.bottomRow(); row >= range.topRow(); --row) {
            logTable->removeRow(row);
        }
    }
    saveLogs();
}

void AuthenticatorPage::saveLogs()
{
    QJsonArray logArray;
    for (int i = 0; i < logTable->rowCount(); ++i) {
        QJsonObject obj;
        obj["timestamp"] = logTable->item(i, 0)->text();
        obj["type"] = logTable->item(i, 1)->text();
        obj["detail"] = logTable->item(i, 2)->text();
        logArray.append(obj);
    }

    QFile file("auth_logs.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(logArray);
        file.write(doc.toJson());
        file.close();
    }
}

void AuthenticatorPage::loadLogs()
{
    QFile file("auth_logs.json");
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QJsonArray logArray = doc.array();
        for (const QJsonValue& val : logArray) {
            QJsonObject obj = val.toObject();
            int row = logTable->rowCount();
            logTable->insertRow(row);
            logTable->setItem(row, 0, new QTableWidgetItem(obj["timestamp"].toString()));
            logTable->setItem(row, 1, new QTableWidgetItem(obj["type"].toString()));
            logTable->setItem(row, 2, new QTableWidgetItem(obj["detail"].toString()));
        }
    }
}

void AuthenticatorPage::onTestEmailClicked()
{
    //qDebug() << "Test email button clicked";
    QString toEmail = emailField->text().trimmed();
    if (toEmail.isEmpty()) {
        addLogEntry("Error", "No receiver email entered.");
        return;
    }

    saveSettings();
/*Add your email and Application-Specific Password*/
    QString fromEmail = "";
    QString appPassword = "";

    EmailSender sender(fromEmail, appPassword);
    bool success = sender.sendEmail(
        "Spy_Tasker Test Email",
        "This is a test email sent from Spy_Tasker's AuthenticatorPage."
        );

    if (success) {
        addLogEntry("Email", "Test email sent to " + toEmail);
    } else {
        addLogEntry("Error", "Failed to send test email.");
    }
}

void AuthenticatorPage::startMonitoring()
{
    //qDebug() << "Starting monitoring timer for failed logins...";
    checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout, this, &AuthenticatorPage::checkFailedLogins);
    checkTimer->start(10000); // every 10 seconds
}

quint64 AuthenticatorPage::getEventRecordId(const QString& rawXml)
{
    QRegularExpression regex("<EventRecordID>(\\d+)</EventRecordID>");
    QRegularExpressionMatch match = regex.match(rawXml);
    return match.hasMatch() ? match.captured(1).toULongLong() : 0;
}

QDateTime AuthenticatorPage::getEventTime(const QString& rawXml)
{
    QRegularExpression regex("<TimeCreated SystemTime=\"([^\"]+)\"/>");
    QRegularExpressionMatch match = regex.match(rawXml);
    if (match.hasMatch()) {
        return QDateTime::fromString(match.captured(1), Qt::ISODate);
    }
    return QDateTime();
}

void AuthenticatorPage::checkFailedLogins()
{
    //qDebug() << "Checking failed logins...";

    // Query all failed login events (4625) in Security log
    QString query = "*[System/EventID=4625]";
    EVT_HANDLE results = EvtQuery(NULL, L"Security", (LPCWSTR)QString(query).utf16(), EvtQueryReverseDirection | EvtQueryTolerateQueryErrors);
    // if (!results) {
    //     qDebug() << "Failed to get event query handle.";
    //     return;
    // }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime windowStart = now.addSecs(-5 * 60); // last 5 minutes

    int failedCountLast5Min = 0;

    EVT_HANDLE event;
    DWORD returned;
    while (EvtNext(results, 1, &event, INFINITE, 0, &returned) && returned == 1)
    {
        DWORD bufSize = 0, used = 0, count = 0;
        EvtRender(NULL, event, EvtRenderEventXml, 0, NULL, &used, &count);
        if (used == 0) {
            EvtClose(event);
            continue;
        }
        std::vector<wchar_t> buffer(used / sizeof(wchar_t));
        if (EvtRender(NULL, event, EvtRenderEventXml, used, buffer.data(), &used, &count))
        {
            QString xml = QString::fromWCharArray(buffer.data());
            quint64 recordId = getEventRecordId(xml);

            // Skip events we've already seen (prevent duplication)
            if (seenEventRecords.contains(recordId)) {
                EvtClose(event);
                continue;
            }

            QDateTime eventTime = getEventTime(xml);
            if (!eventTime.isValid()) {
                EvtClose(event);
                continue;
            }

            if (eventTime >= windowStart && eventTime <= now) {
                failedCountLast5Min++;
                seenEventRecords.insert(recordId);
                addLogEntry("Failed Login", "Detected Windows login failure at " + eventTime.toString());
            } else if (eventTime < windowStart) {
                EvtClose(event);
                break; // older events can be ignored
            }
        }
        EvtClose(event);
    }
    EvtClose(results);

    //qDebug() << "Failed login count in last 5 mins:" << failedCountLast5Min;

    int threshold = limitBox->value();
    //qDebug() << "Threshold limit:" << threshold;

    if (failedCountLast5Min >= threshold)
    {
        qDebug() << "Threshold reached, sending alert email...";
        if (!alertSentRecently)
        {
            sendAlertEmail(failedCountLast5Min);
            alertSentRecently = true;

            // Reset flag after 10 minutes cooldown to avoid spamming
            QTimer::singleShot(10 * 60 * 1000, [this]() { alertSentRecently = false; });
        }
        else {
            qDebug() << "Alert email recently sent, skipping duplicate email.";
        }
    }
}

void AuthenticatorPage::sendAlertEmail(int attempts)
{
    QString toEmail = emailField->text().trimmed();
    if (toEmail.isEmpty()) {
        addLogEntry("Error", "No receiver email set, cannot send alert email.");
        return;
    }

    QString fromEmail = "";
    QString appPassword = "";

    EmailSender sender(fromEmail, appPassword);
    QString subject = QString("Spy_Tasker Alert: Multiple Failed Login Attempts");
    QString body = QString("There have been %1 failed login attempts detected on this machine within the last 5 minutes.").arg(attempts);

    // qDebug() << "=== EmailSender::sendEmail ===";
    // qDebug() << "To:      " << toEmail;
    // qDebug() << "From:    " << fromEmail;
    // qDebug() << "Subject: " << subject;
    // qDebug() << "Body:    " << body;

    bool success = sender.sendEmail(subject, body);

    if (success) {
        addLogEntry("Email", QString("Alert email sent to %1 after %2 failed login attempts").arg(toEmail).arg(attempts));
    } else {
        addLogEntry("Error", "Failed to send alert email.");
    }
}

