
#include "limiter_backend.h"
#include <psapi.h>
#include <QTimer>
#include <QDebug>
#include <windows.h>
#include <tlhelp32.h>
#include <map>

LimiterBackend* LimiterBackend::instance() {
    static LimiterBackend backend;
    return &backend;
}

static std::map<DWORD, ULONGLONG> lastKernelTime, lastUserTime;

LimiterBackend::LimiterBackend(QObject *parent) : QObject(parent) {
    connect(&monitorTimer, &QTimer::timeout, this, &LimiterBackend::monitorProcesses);
    monitorTimer.start(1000); // Monitor every 1 second
}

DWORD LimiterBackend::getCpuUsageForProcess(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return 0;

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        CloseHandle(hProcess);
        return 0;
    }

    ULONGLONG kernel = ((ULONGLONG)ftKernel.dwHighDateTime << 32) | ftKernel.dwLowDateTime;
    ULONGLONG user = ((ULONGLONG)ftUser.dwHighDateTime << 32) | ftUser.dwLowDateTime;

    ULONGLONG prevKernel = lastKernelTime[pid];
    ULONGLONG prevUser = lastUserTime[pid];

    ULONGLONG delta = (kernel + user) - (prevKernel + prevUser);

    lastKernelTime[pid] = kernel;
    lastUserTime[pid] = user;

    // Scale to percentage - approximation assuming 1 second interval
    DWORD cpuPercent = static_cast<DWORD>(delta / 10000); // convert to ms
    return qMin(cpuPercent, 100u); // clamp to 100
}

SIZE_T LimiterBackend::getProcessMemoryUsage(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess)
        return 0;

    PROCESS_MEMORY_COUNTERS pmc;
    SIZE_T memUsage = 0;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        memUsage = pmc.WorkingSetSize;
    }
    CloseHandle(hProcess);
    return memUsage;
}

int LimiterBackend::getProcessCPUPercent(DWORD pid) {
    return static_cast<int>(getCpuUsageForProcess(pid));
}

void LimiterBackend::applyLimits(DWORD pid, SIZE_T memoryLimit, int cpuPercent, int timeLimitSec,
                                 QProgressBar* ramBar, QProgressBar* cpuBar, QProgressBar* timeBar,
                                 QLabel* infoLabel) {
    if (activeLimits.contains(pid))
        return;

    ProcessLimit limit;
    limit.pid = pid;
    limit.memoryLimit = memoryLimit;
    limit.cpuPercentLimit = cpuPercent;
    limit.timeLimitSeconds = timeLimitSec;

    limit.ramBar = ramBar;
    limit.cpuBar = cpuBar;
    limit.timeBar = timeBar;
    limit.infoLabel = infoLabel;
    limit.secondsElapsed = 0;

    if (timeLimitSec > 0 && timeBar) {
        timeBar->setMaximum(timeLimitSec);
        timeBar->setValue(0);
    }

    if (ramBar)
        ramBar->setValue(0);
    if (cpuBar)
        cpuBar->setValue(0);
    if (infoLabel)
        infoLabel->clear();

    activeLimits.insert(pid, limit);
}

void LimiterBackend::monitorProcesses() {
    QList<DWORD> toRemove;

    for (auto it = activeLimits.begin(); it != activeLimits.end(); ++it) {
        ProcessLimit &limit = it.value();

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, limit.pid);
        if (!hProcess) {
            toRemove.append(limit.pid);
            continue;
        }

        // RAM Monitoring
        SIZE_T currentMemory = getProcessMemoryUsage(limit.pid);
        if (limit.memoryLimit > 0 && limit.ramBar) {
            int percentUsed = static_cast<int>((100.0 * currentMemory) / limit.memoryLimit);
            limit.ramBar->setValue(qMin(percentUsed, 100));

            if (currentMemory > limit.memoryLimit) {
                TerminateProcess(hProcess, 1);
                emit processTerminated("RAM limit exceeded for PID: " + QString::number(limit.pid));
                CloseHandle(hProcess);
                toRemove.append(limit.pid);
                continue;
            }
        }

        // CPU Limit
        if (limit.cpuPercentLimit > 0 && limit.cpuBar) {
            int cpuUsage = getProcessCPUPercent(limit.pid);
            limit.cpuBar->setValue(cpuUsage);

            if (cpuUsage > limit.cpuPercentLimit) {
                TerminateProcess(hProcess, 1);
                emit processTerminated("CPU usage exceeded limit for PID: " + QString::number(limit.pid));
                CloseHandle(hProcess);
                toRemove.append(limit.pid);
                continue;
            }
        }

        // Time Limit
        if (limit.timeLimitSeconds > 0 && limit.timeBar) {
            limit.secondsElapsed++;
            limit.timeBar->setValue(limit.secondsElapsed);

            if (limit.secondsElapsed >= limit.timeLimitSeconds) {
                TerminateProcess(hProcess, 1);
                emit processTerminated("Time limit exceeded for PID: " + QString::number(limit.pid));
                CloseHandle(hProcess);
                toRemove.append(limit.pid);
                continue;
            }
        }

        CloseHandle(hProcess);
    }

    for (DWORD pid : toRemove) {
        activeLimits.remove(pid);
    }
}
