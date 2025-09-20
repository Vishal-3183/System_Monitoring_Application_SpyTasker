#ifndef LIMITER_BACKEND_H
#define LIMITER_BACKEND_H

#include <Windows.h>
#include <QProgressBar>
#include <QLabel>
#include <QObject>
#include <QMap>
#include <QTimer>
// Struct to hold per-process limits and UI elements
struct ProcessLimit {
    DWORD pid;
    SIZE_T memoryLimit;
    int cpuPercentLimit;
    int timeLimitSeconds;

    QProgressBar* ramBar;
    QProgressBar* cpuBar;
    QProgressBar* timeBar;
    QLabel* infoLabel;

    int secondsElapsed = 0;
};

class LimiterBackend : public QObject
{
    Q_OBJECT

public:
    static LimiterBackend* instance();

    // Apply limits and associate UI elements for progress display and info label
    void applyLimits(DWORD pid, SIZE_T memoryLimit, int cpuPercent, int timeLimitSeconds,
                     QProgressBar* ramBar, QProgressBar* cpuBar, QProgressBar* timeBar,
                     QLabel* infoLabel);

    // Call this regularly (timer) to monitor and enforce limits
    void monitorProcesses();

    // Helpers to get resource usage (implement in .cpp)
    DWORD getCpuUsageForProcess(DWORD pid);
    SIZE_T getProcessMemoryUsage(DWORD pid);
    int getProcessCPUPercent(DWORD pid);

signals:
    void processTerminated(const QString &message);

private:
    explicit LimiterBackend(QObject *parent = nullptr);

    // Map of active limits keyed by process ID
    QMap<DWORD, ProcessLimit> activeLimits;
    QTimer monitorTimer;
};

#endif // LIMITER_BACKEND_H
