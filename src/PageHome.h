#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <windows.h>
#include <QProgressBar>
class PageHome : public QWidget {
    Q_OBJECT

public:
    explicit PageHome(QWidget *parent = nullptr);

private slots:
    void refreshProcessList();
    void endSelectedTask();
    void updateSystemStats();
    void filterProcessList(const QString &filterText);
    void updateProcessCPU();


private:
    void populateDummyData();
    void populateRealProcesses();
    double getRAMUsageForPID(DWORD pid);
    double getCPUUsage();
    double getRAMUsage();

    QProgressBar *cpuBar;
    QProgressBar *ramBar;
    QLineEdit *searchBar;
    QTimer *cpuTimer = nullptr;
    QMap<DWORD, quint64> lastProcessCPU; // Maps PID to last CPU time
    quint64 lastSystemCPU = 0;

    QTableWidget *table;
    QPushButton *refreshBtn;
    QPushButton *endTaskBtn;
    QLabel *cpuLabel;
    QLabel *ramLabel;
};
