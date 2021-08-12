#include "ui_mainwindow.h"

#include <UserInterface/mainwindow.h>

MainWindow::MainWindow() : MainWindow{nullptr} {}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent}, m_ui{std::make_unique<Ui::MainWindow>()} {
  m_ui->setupUi(this);
}

MainWindow::~MainWindow() = default;
