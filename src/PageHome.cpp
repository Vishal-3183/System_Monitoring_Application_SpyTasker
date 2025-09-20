#include "PageHome.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QMap>
#include <thread>
#include <QDebug>
#include <QLineEdit>

// --- Static tracking for CPU ---
static FILETIME prevIdleTime, prevKernelTime, prevUserTime;


PageHome::PageHome(QWidget *parent) : QWidget(parent) {
    // ---------- CPU & RAM Progress Bars ----------
    cpuBar = new QProgressBar(this);
    cpuBar->setRange(0, 100);
    cpuBar->setFormat("CPU Usage: %p%");
    cpuBar->setTextVisible(true);

    ramBar = new QProgressBar(this);
    ramBar->setRange(0, 100);
    ramBar->setFormat("RAM Usage: %p%");
    ramBar->setTextVisible(true);

    QVBoxLayout *usageLayout = new QVBoxLayout();
    usageLayout->addWidget(cpuBar);
    usageLayout->addWidget(ramBar);

    // ---------- Process Table Setup ----------
    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Process Name", "PID", "CPU %", "RAM (MB)"});

    QFont monoFont("Consolas", 12);
    table->setFont(monoFont);

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);          // Process Name
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // PID
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // CPU %
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // RAM

    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setTextElideMode(Qt::ElideRight);

    // ---------- Buttons ----------
    refreshBtn = new QPushButton("Refresh");
    endTaskBtn = new QPushButton("End Task");

    connect(refreshBtn, &QPushButton::clicked, this, &PageHome::refreshProcessList);
    connect(endTaskBtn, &QPushButton::clicked, this, &PageHome::endSelectedTask);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(endTaskBtn);

    // ---------- Search Bar ----------
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search process...");
    connect(searchBar, &QLineEdit::textChanged, this, &PageHome::filterProcessList);

    // ---------- Main Layout ----------
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(usageLayout);
    layout->addLayout(btnLayout);
    layout->addWidget(searchBar);
    layout->addWidget(table);

    // ---------- System Stats Timer ----------
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PageHome::updateSystemStats);
    timer->start(1000); // 1-second interval
}



void PageHome::filterProcessList(const QString &filterText) {
    for (int i = 0; i < table->rowCount(); ++i) {
        bool match = table->item(i, 0)->text().contains(filterText, Qt::CaseInsensitive);
        table->setRowHidden(i, !match);
    }
}

// --- WinAPI utility functions ---
double PageHome::getRAMUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG total = memInfo.ullTotalPhys;
    DWORDLONG avail = memInfo.ullAvailPhys;
    return (double)(total - avail) / total * 100.0;
}

double PageHome::getCPUUsage() {
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1;

    ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    ULONGLONG prevIdle = ((ULONGLONG)prevIdleTime.dwHighDateTime << 32) | prevIdleTime.dwLowDateTime;
    ULONGLONG prevKernel = ((ULONGLONG)prevKernelTime.dwHighDateTime << 32) | prevKernelTime.dwLowDateTime;
    ULONGLONG prevUser = ((ULONGLONG)prevUserTime.dwHighDateTime << 32) | prevUserTime.dwLowDateTime;

    ULONGLONG idleDiff = idle - prevIdle;
    ULONGLONG totalDiff = (kernel + user) - (prevKernel + prevUser);

    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;

    if (totalDiff == 0) return 0;
    
    return (1.0 - (double)idleDiff / totalDiff) * 100.0;
}

void PageHome::updateSystemStats() {
    double cpu = getCPUUsage();
    double ram = getRAMUsage();

    if (cpuBar) {
        cpuBar->setValue(static_cast<int>(cpu));
        cpuBar->setFormat(QString("CPU Usage: %1 %").arg(cpu, 0, 'f', 2));
    }

    if (ramBar) {
        ramBar->setValue(static_cast<int>(ram));
        ramBar->setFormat(QString("RAM Usage: %1 %").arg(ram, 0, 'f', 2));
    }
}


quint64 getProcessCPUTime(HANDLE hProcess) {
    FILETIME creation, exit, kernel, user;
    if (GetProcessTimes(hProcess, &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER k, u;
        k.LowPart = kernel.dwLowDateTime;
        k.HighPart = kernel.dwHighDateTime;

        u.LowPart = user.dwLowDateTime;
        u.HighPart = user.dwHighDateTime;

        return (k.QuadPart + u.QuadPart);
    }
    return 0;
}

quint64 getTotalSystemCPUTime() {
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER k, u;
        k.LowPart = kernelTime.dwLowDateTime;
        k.HighPart = kernelTime.dwHighDateTime;

        u.LowPart = userTime.dwLowDateTime;
        u.HighPart = userTime.dwHighDateTime;

        return (k.QuadPart + u.QuadPart);
    }
    return 0;
}



void PageHome::populateRealProcesses() {
    table->setRowCount(0); // Clear existing rows
    lastProcessCPU.clear(); // Clear previous CPU snapshot

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        int row = 0;
        do {
            DWORD pid = pe.th32ProcessID;
            QString name = QString::fromWCharArray(pe.szExeFile);
            double ramMB = getRAMUsageForPID(pid);

            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(name));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(pid)));

            // CPU%
            QTableWidgetItem *cpuItem = new QTableWidgetItem("--");
            cpuItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(row, 2, cpuItem);

            // RAM
            QString ramDisplay = (ramMB < 0) ? "N/A" : QString::number(ramMB, 'f', 2);
            QTableWidgetItem *ramItem = new QTableWidgetItem(ramDisplay);
            ramItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(row, 3, ramItem);

            // Cache initial CPU time if accessible
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
                lastProcessCPU[pid] = getProcessCPUTime(hProcess);
                CloseHandle(hProcess);
            }

            row++;
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);

    // Update system CPU snapshot
    lastSystemCPU = getTotalSystemCPUTime();

    // Start or restart the CPU usage timer
    if (!cpuTimer) {
        cpuTimer = new QTimer(this);
        connect(cpuTimer, &QTimer::timeout, this, &PageHome::updateProcessCPU);
        cpuTimer->start(1500); // 1.5 seconds interval
    }
}


double PageHome::getRAMUsageForPID(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return -1;

    PROCESS_MEMORY_COUNTERS pmc;
    double memMB = -1;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        memMB = pmc.WorkingSetSize / (1024.0 * 1024.0); // Bytes to MB
    }

    CloseHandle(hProcess);
    return memMB;
}
void PageHome::refreshProcessList() {
    populateRealProcesses();
}


void PageHome::updateProcessCPU() {
    //qDebug() << "[DEBUG] updateProcessCPU called!";
    quint64 currentSystemCPU = getTotalSystemCPUTime();
    int cpuCores = std::thread::hardware_concurrency();

    for (int row = 0; row < table->rowCount(); ++row) {
        DWORD pid = table->item(row, 1)->text().toUInt();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) continue;

        quint64 currentProcessCPU = getProcessCPUTime(hProcess);
        CloseHandle(hProcess);

        if (lastProcessCPU.contains(pid)) {
            quint64 procDelta = currentProcessCPU - lastProcessCPU[pid];
            quint64 sysDelta = currentSystemCPU - lastSystemCPU;

            double usage = (sysDelta > 0)
                               ? (100.0 * procDelta / sysDelta) * cpuCores
                               : 0;

            table->item(row, 2)->setText(QString::number(usage, 'f', 1));
        }

        lastProcessCPU[pid] = currentProcessCPU;
    }

    lastSystemCPU = currentSystemCPU;
}


void PageHome::endSelectedTask() {
    int row = table->currentRow();
    if (row < 0) return;

    DWORD pid = table->item(row, 1)->text().toUInt();
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

    if (hProcess) {
        if (TerminateProcess(hProcess, 1)) {
            QMessageBox::information(this, "Success", "Process terminated.");
            table->removeRow(row);
        } else {
            QMessageBox::warning(this, "Error", "Failed to terminate process.");
        }
        CloseHandle(hProcess);
    } else {
        QMessageBox::warning(this, "Error", "Access denied or process already closed.");
    }
}

