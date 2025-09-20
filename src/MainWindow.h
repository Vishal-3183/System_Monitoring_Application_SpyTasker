#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include "PageHome.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void handleSidebarChange(int index);
    void toggleTheme();

private:
    QListWidget *sidebar;
    QStackedWidget *stack;
    QPushButton *themeToggleBtn;
    bool darkMode = true;

    void applyTheme(bool dark);
};
