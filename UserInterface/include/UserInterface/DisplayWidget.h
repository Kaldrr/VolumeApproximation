#pragma once

#include <QWidget>

#include <memory>

namespace Ui {
class DisplayWidget;
}

class DisplayWidget : public QWidget {
  Q_OBJECT

public:
  DisplayWidget();
  explicit DisplayWidget(QWidget *parent);
  ~DisplayWidget();

private slots:
  void onStartButtonClick();

private:
  std::unique_ptr<Ui::DisplayWidget> m_ui;
};
