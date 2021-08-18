#include "ui_displaywidget.h"

#include <UserInterface/displaywidget.h>

#include <QDebug>

#include <limits>

#include <ApproximationLib/VolumeApproximator.h>

DisplayWidget::DisplayWidget() : DisplayWidget{nullptr} {}

DisplayWidget::DisplayWidget(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::DisplayWidget>()} {
  m_ui->setupUi(this);

  m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

  connect(m_ui->startButton, &QPushButton::clicked, this,
          &DisplayWidget::onStartButtonClick);
}

void DisplayWidget::onStartButtonClick() {
  const Qt3DCore::QGeometryView *const geometry =
      m_ui->sceneContainer->getGeometry();

  if (geometry != nullptr) {
    VolumeApproximator volumeApproximator{*geometry};
    const std::vector<ApproximationPoint> points =
        volumeApproximator.getVolume(m_ui->samplesCountInput->value());
  } else {
    qDebug() << "Can't approximate volume of nullptr geometry...";
  }
}

DisplayWidget::~DisplayWidget() = default;
