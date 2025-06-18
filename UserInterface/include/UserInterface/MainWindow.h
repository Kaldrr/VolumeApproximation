#pragma once

#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	explicit MainWindow(QWidget* parent);
	~MainWindow() override;

private:
	std::unique_ptr<Ui::MainWindow> m_ui;
};
