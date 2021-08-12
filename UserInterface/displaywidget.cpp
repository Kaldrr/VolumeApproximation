#include "ui_displaywidget.h"

#include <UserInterface/displaywidget.h>

#include <QDebug>

#include <limits>

DisplayWidget::DisplayWidget() : DisplayWidget{nullptr} {}

DisplayWidget::DisplayWidget(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::DisplayWidget>()} {
  m_ui->setupUi(this);

  m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

  connect(m_ui->startButton, &QPushButton::clicked, this,
          &DisplayWidget::onStartButtonClick);
}

void DisplayWidget::onStartButtonClick() {
  qDebug() << "Hello start world!\n"
           << "Current value = " << m_ui->samplesCountInput->value();
}

DisplayWidget::~DisplayWidget() = default;
