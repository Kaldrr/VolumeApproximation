#include <ui/ui_MainWindow.h>

#include <UserInterface/MainWindow.h>

MainWindow::MainWindow() : MainWindow{nullptr}
{
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow{parent}, m_ui{std::make_unique<Ui::MainWindow>()}
{
	m_ui->setupUi(this);
}

MainWindow::~MainWindow() = default;
