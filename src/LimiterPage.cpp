#include "LimiterPage.h"
#include "limiter_backend.h"
#include <windows.h>
#include <tlhelp32.h>
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QDebug>
#include <psapi.h>

LimiterPage::LimiterPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top Controls
    QHBoxLayout *topLayout = new QHBoxLayout();
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search process...");
    refreshButton = new QPushButton("Refresh", this);
    topLayout->addWidget(searchBar);
    topLayout->addWidget(refreshButton);
    mainLayout->addLayout(topLayout);

    // Process Table
    processTable = new QTableWidget(this);
    processTable->setColumnCount(2);
    processTable->setHorizontalHeaderLabels({"PID", "Process Name"});
    processTable->horizontalHeader()->setStretchLastSection(true);
    processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    processTable->setFixedHeight(200);
    mainLayout->addWidget(processTable);

    // Limit Inputs
    QGroupBox *limitBox = new QGroupBox("Apply Limits");
    QHBoxLayout *limitLayout = new QHBoxLayout();
    limitLayout->addWidget(new QLabel("Memory (MB):"));
    memoryLimitInput = new QSpinBox(this);
    memoryLimitInput->setMaximum(100000);
    limitLayout->addWidget(memoryLimitInput);

    limitLayout->addWidget(new QLabel("CPU %:"));
    cpuLimitInput = new QSpinBox(this);
    cpuLimitInput->setRange(0, 100);  // Allow 0 for no CPU limit
    cpuLimitInput->setSuffix("%");
    limitLayout->addWidget(cpuLimitInput);

    limitLayout->addWidget(new QLabel("Time (s):"));
    timeLimitInput = new QSpinBox(this);
    timeLimitInput->setMaximum(3600);
    limitLayout->addWidget(timeLimitInput);

    applyButton = new QPushButton("Apply", this);
    limitLayout->addWidget(applyButton);
    limitBox->setLayout(limitLayout);
    mainLayout->addWidget(limitBox);

    // Limits Display
    mainLayout->addWidget(new QLabel("Active Limits:"));
    limitsDisplayWidget = new QWidget(this);
    limitsDisplayLayout = new QVBoxLayout(limitsDisplayWidget);
    mainLayout->addWidget(limitsDisplayWidget);

    connect(refreshButton, &QPushButton::clicked, this, &LimiterPage::refreshProcessList);
    connect(searchBar, &QLineEdit::textChanged, this, &LimiterPage::filterProcessTable);
    connect(applyButton, &QPushButton::clicked, this, &LimiterPage::applyLimitsToSelected);

    monitorTimer = new QTimer(this);
    connect(monitorTimer, &QTimer::timeout, this, &LimiterPage::updateLimitTracking);
    monitorTimer->start(1000);

    refreshProcessList();

}

void LimiterPage::refreshProcessList() {
    processTable->setRowCount(0);

    // Clear trackedLimits to resync with current processes
    trackedLimits.clear();

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    int row = 0;

    if (Process32First(hSnap, &pe)) {
        do {
            // Add row to processTable
            processTable->insertRow(row);
            processTable->setItem(row, 0, new QTableWidgetItem(QString::number(pe.th32ProcessID)));
            processTable->setItem(row, 1, new QTableWidgetItem(QString::fromWCharArray(pe.szExeFile)));

            // Add process to trackedLimits
            LimitData limitData;
            limitData.pid = pe.th32ProcessID;
            limitData.processName = QString::fromWCharArray(pe.szExeFile);
            limitData.memoryLimit = 0;      // no limits initially
            limitData.cpuPercentLimit = 0;
            limitData.timeLimit = 0;
            limitData.timeRemaining = 0;
            limitData.limitLabel = nullptr;
            limitData.cpuBar = nullptr;
            limitData.ramBar = nullptr;
            limitData.timeBar = nullptr;

            trackedLimits.push_back(limitData);

            row++;
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

void LimiterPage::filterProcessTable(const QString &query) {
    for (int i = 0; i < processTable->rowCount(); ++i) {
        bool match = processTable->item(i, 1)->text().contains(query, Qt::CaseInsensitive);
        processTable->setRowHidden(i, !match);
    }
}

DWORD LimiterPage::getSelectedPid() const {
    QList<QTableWidgetItem *> items = processTable->selectedItems();
    if (items.isEmpty()) return 0;
    return items.first()->text().toUInt();
}

QString LimiterPage::getSelectedProcessName() const {
    QList<QTableWidgetItem *> items = processTable->selectedItems();
    if (items.isEmpty()) return "";
    int row = items.first()->row();
    return processTable->item(row, 1)->text();
}

void LimiterPage::applyLimitsToSelected() {
    DWORD pid = getSelectedPid();
    if (pid == 0) {
        QMessageBox::warning(this, "No Process Selected", "Select a process first.");
        return;
    }
    QString name = getSelectedProcessName();

    SIZE_T memLimit = static_cast<SIZE_T>(memoryLimitInput->value()) * 1024 * 1024;
    int cpuPercent = cpuLimitInput->value();
    int timeLimit = timeLimitInput->value();

    if (memLimit == 0 && cpuPercent == 0 && timeLimit == 0) {
        QMessageBox::warning(this, "No Limits", "Please set at least one limit.");
        return;
    }

    if (!applyLimitsToProcess(pid, memLimit, cpuPercent, timeLimit)) {
        QMessageBox::critical(this, "Failure", "Could not apply limits.");
        return;
    }

    LimitData data;
    data.pid = pid;
    data.processName = name;
    data.memoryLimit = memLimit;
    data.cpuPercentLimit = cpuPercent;
    data.timeRemaining = timeLimit;

    // Create UI elements for progress bars
    data.cpuBar = new QProgressBar;
    data.cpuBar->setFormat("CPU: %p%");
    data.cpuBar->setRange(0, 100);
    data.cpuBar->setValue(0);

    data.ramBar = new QProgressBar;
    data.ramBar->setFormat("RAM: %p%");
    data.ramBar->setRange(0, 100);
    data.ramBar->setValue(0);

    data.timeBar = new QProgressBar;
    data.timeBar->setFormat("Time Remaining: %v sec");
    data.timeBar->setMaximum(timeLimit);
    data.timeBar->setValue(timeLimit);

    // Label showing limits info
    QString infoText = QString("%1 (PID %2) - ").arg(name).arg(pid);
    QStringList limitsApplied;
    if (memLimit > 0) limitsApplied << QString("Memory ≤ %1 MB").arg(memLimit / (1024 * 1024));
    if (cpuPercent > 0) limitsApplied << QString("CPU ≤ %1%").arg(cpuPercent);
    if (timeLimit > 0) limitsApplied << QString("Time ≤ %1 sec").arg(timeLimit);
    infoText += limitsApplied.join(", ");

    data.limitLabel = new QLabel(infoText);

    // Store data and add UI widgets
    activeLimits[pid] = data;
    addLimitDisplay(data);

    // Ensure monitor timer is running
    if (!monitorTimer->isActive())
        monitorTimer->start(1000);

    QMessageBox::information(this, "Success", "Limits applied.");
}

void LimiterPage::addLimitDisplay(const LimitData &data) {
    limitsDisplayLayout->addWidget(data.limitLabel);
    limitsDisplayLayout->addWidget(data.cpuBar);
    limitsDisplayLayout->addWidget(data.ramBar);
    limitsDisplayLayout->addWidget(data.timeBar);
}

// Improved CPU usage estimator using Windows API, keeps previous sample
DWORD LimiterPage::getCpuUsageForProcess(DWORD pid) {
    static std::map<DWORD, ULONGLONG> lastKernel, lastUser;
    static std::map<DWORD, ULONGLONG> lastCheckTime;

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return 0;

    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        CloseHandle(hProcess);
        return 0;
    }

    ULONGLONG curKernel = ((ULONGLONG)ftKernel.dwHighDateTime << 32) | ftKernel.dwLowDateTime;
    ULONGLONG curUser = ((ULONGLONG)ftUser.dwHighDateTime << 32) | ftUser.dwLowDateTime;
    ULONGLONG now = GetTickCount64();

    DWORD usage = 0;
    if (lastCheckTime.count(pid)) {
        ULONGLONG deltaTime = now - lastCheckTime[pid];
        ULONGLONG deltaKernel = curKernel - lastKernel[pid];
        ULONGLONG deltaUser = curUser - lastUser[pid];

        ULONGLONG totalProcessTime = (deltaKernel + deltaUser) / 10000; // in ms
        if (deltaTime > 0) {
            usage = static_cast<DWORD>((100.0 * totalProcessTime) / deltaTime);
            if (usage > 100) usage = 100; // clamp to 100%
        }
    }

    lastKernel[pid] = curKernel;
    lastUser[pid] = curUser;
    lastCheckTime[pid] = now;

    CloseHandle(hProcess);
    return usage;
}

SIZE_T LimiterPage::getRamUsageForProcess(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return 0;

    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        CloseHandle(hProcess);
        return 0;
    }
    CloseHandle(hProcess);
    return pmc.WorkingSetSize;
}

void LimiterPage::updateLimitTracking() {
    QList<DWORD> toRemove;
    LimiterBackend* backend = LimiterBackend::instance();

    for (auto key : activeLimits.keys()) {
        LimitData &data = activeLimits[key];

        // CPU usage
        DWORD cpuUsage = getCpuUsageForProcess(data.pid);
        data.cpuBar->setValue(cpuUsage);

        // RAM usage
        SIZE_T ramUsage = getRamUsageForProcess(data.pid);
        int ramPercent = 0;
        if (data.memoryLimit > 0)
            ramPercent = static_cast<int>((ramUsage * 100) / data.memoryLimit);
        if (ramPercent > 100) ramPercent = 100;
        data.ramBar->setValue(ramPercent);

        // Time limit countdown
        if (data.timeRemaining > 0) {
            data.timeRemaining--;
            data.timeBar->setValue(data.timeRemaining);
        }

        // Check limits and terminate if exceeded
        bool terminate = false;
        QString reason;
        if (data.memoryLimit > 0 && ramUsage > data.memoryLimit) {
            terminate = true;
            reason = "Memory limit exceeded";
        }
        if (data.cpuPercentLimit > 0 && cpuUsage > (DWORD)data.cpuPercentLimit) {
            terminate = true;
            reason = "CPU usage limit exceeded";
        }
        if (data.timeRemaining == 0 && data.timeBar->maximum() > 0) {
            terminate = true;
            reason = "Time limit expired";
        }

        if (terminate) {
            if (terminateProcessByPid(data.pid)) {
                qDebug() << "Terminated PID" << data.pid << "due to" << reason;
                toRemove.append(key);
                continue;
            }
            else {
                qDebug() << "Failed to terminate PID" << data.pid;
            }
        }
    }

    for (DWORD pid : toRemove) {
        removeLimit(pid);
    }

    if (activeLimits.isEmpty()) {
        monitorTimer->stop();
    }
}

void LimiterPage::removeLimit(DWORD pid) {
    if (!activeLimits.contains(pid))
        return;

    LimitData &data = activeLimits[pid];

    // Remove UI elements
    limitsDisplayLayout->removeWidget(data.limitLabel);
    delete data.limitLabel;
    limitsDisplayLayout->removeWidget(data.cpuBar);
    delete data.cpuBar;
    limitsDisplayLayout->removeWidget(data.timeBar);
    delete data.timeBar;
    limitsDisplayLayout->removeWidget(data.ramBar);
    delete data.ramBar;


    // Remove from activeLimits
    activeLimits.remove(pid);

    // Optional: stop timer if no limits remain
    if (activeLimits.isEmpty()) {
        monitorTimer->stop();
    }
}

bool LimiterPage::terminateProcessByPid(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess)
        return false;
    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return (result != 0);
}

bool LimiterPage::applyLimitsToProcess(DWORD pid, ULONGLONG ramLimitMB, int cpuLimitPercent, int timeLimitSec) {
    for (auto& data : trackedLimits) {
        if (data.pid == pid) {
            //qDebug() << "Applying limits to PID:" << pid;

            data.memoryLimit = ramLimitMB; // Already passed as bytes from caller
            data.cpuPercentLimit = cpuLimitPercent;
            data.timeLimit = timeLimitSec;
            data.timeRemaining = timeLimitSec;
            return true;
        }
    }
    return false;
}
