#ifndef LIMITERPAGE_H
#define LIMITERPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMap>
#include <QProgressBar>
#include <QTimer>

#include <windows.h>  // For DWORD, SIZE_T

struct LimitData {
    DWORD pid;
    QString processName;

    SIZE_T memoryLimit = 0;       // in bytes (convert MB to bytes when storing)
    int cpuPercentLimit = 0;      // 0 means no limit
    int timeRemaining = 0;        // seconds countdown remaining
    int timeLimit = 0;            // total seconds set for reference

    QLabel* limitLabel = nullptr;
    QProgressBar* cpuBar = nullptr;
    QProgressBar* ramBar = nullptr;
    QProgressBar* timeBar = nullptr;
};

// Struct to store per-process limit data
// struct LimitData {
//     DWORD pid;
//     QString processName;
//     SIZE_T memoryLimit;      // in bytes
//     int cpuPercentLimit;     // 0 if no limit
//     int timeRemaining;       // seconds, 0 if no limit

//     QLabel* limitLabel = nullptr;
//     QProgressBar* cpuBar = nullptr;
//     QProgressBar* ramBar = nullptr;   // <-- Add this line
//     QProgressBar* timeBar = nullptr;
// };

class LimiterPage : public QWidget {
    Q_OBJECT

public:
    explicit LimiterPage(QWidget *parent = nullptr);
    //DWORD getRamUsageForProcess(DWORD pid);
    SIZE_T getRamUsageForProcess(DWORD pid);
    bool terminateProcessByPid(DWORD pid);

    void removeLimit(DWORD pid);
    bool applyLimitsToProcess(DWORD pid, ULONGLONG ramLimitMB, int cpuLimitPercent, int timeLimitSec);

private slots:
    void refreshProcessList();
    void filterProcessTable(const QString &query);
    void applyLimitsToSelected();
    void updateLimitTracking();
    DWORD getCpuUsageForProcess(DWORD pid);


private:
    //bool applyLimitsToProcess(DWORD pid, SIZE_T memLimit, int cpuPercent, int timeLimit);

    // UI Elements
    // Add this to store all processes with active limits
    QList<LimitData> trackedLimits;

    QTableWidget *processTable;
    QLineEdit *searchBar;
    QPushButton *refreshButton;
    QPushButton *applyButton;
    QSpinBox *memoryLimitInput;
    QSpinBox *cpuLimitInput;
    QSpinBox *timeLimitInput;

    QWidget *limitsDisplayWidget;
    QVBoxLayout *limitsDisplayLayout;

    QTimer *monitorTimer;

    QMap<DWORD, LimitData> activeLimits;

    // Helper methods
    QString getSelectedProcessName() const;
    DWORD getSelectedPid() const;
    void addLimitDisplay(const LimitData &data);
};

#endif // LIMITERPAGE_H
