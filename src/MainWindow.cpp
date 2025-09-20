#include "MainWindow.h"
#include "LimiterPage.h"
#include "AuthenticatorPage.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QListWidgetItem>
#include <QIcon>
#include <QApplication>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    sidebar = new QListWidget(this);
    sidebar->setFixedWidth(150);
    //sidebar->addItem(new QListWidgetItem(QIcon(":/icons/home.png"), "Home"));
    sidebar->addItem("Home");

    sidebar->addItem("Limiter");
    //sidebar->addItem("Sandbox");
    sidebar->addItem("Security");

    stack = new QStackedWidget(this);
    stack->addWidget(new PageHome()); // Home page
    stack->addWidget(new LimiterPage());// Limiter placeholder
    //stack->addWidget(new QWidget());  // Sandbox placeholder
    stack->addWidget(new AuthenticatorPage());  // Security placeholder

    // themeToggleBtn = new QPushButton(QIcon(":/icons/theme.png"), "Toggle Theme");
    themeToggleBtn = new QPushButton("Toggle Theme");

    connect(themeToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);

    QVBoxLayout *sidebarLayout = new QVBoxLayout();
    sidebarLayout->addWidget(sidebar);
    sidebarLayout->addWidget(themeToggleBtn);
    sidebarLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addLayout(sidebarLayout);
    mainLayout->addWidget(stack);

    central->setLayout(mainLayout);

    connect(sidebar, &QListWidget::currentRowChanged, this, &MainWindow::handleSidebarChange);

    applyTheme(true); // default to dark
}

void MainWindow::handleSidebarChange(int index) {
    stack->setCurrentIndex(index);
}

void MainWindow::toggleTheme() {
    //qDebug() << "Toggling theme. Dark mode now:" << !darkMode;
    darkMode = !darkMode;
    applyTheme(darkMode);
}

void MainWindow::applyTheme(bool dark) {
    QFile file(dark ? ":/styles/dark.qss" : ":/styles/light.qss");
    if (file.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(file.readAll());
    }
}
