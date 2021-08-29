#include "ui_DisplayWidget.h"

#include <UserInterface/DisplayWidget.h>

#include <ApproximationLib/VolumeApproximator.h>

#include <QDebug>
#include <QDoubleSpinBox>

#include <limits>

DisplayWidget::DisplayWidget() : DisplayWidget{nullptr} {}

DisplayWidget::DisplayWidget(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::DisplayWidget>()} {
  m_ui->setupUi(this);

  m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

  connect(m_ui->startButton, &QPushButton::clicked, this,
          &DisplayWidget::onStartButtonClick);
  connect(m_ui->pointRadiusInput, &QDoubleSpinBox::valueChanged,
          m_ui->sceneContainer, &SceneContainer::updatePointsRadius);

  m_ui->pointRadiusInput->setValue(0.05);
}

void DisplayWidget::onStartButtonClick() {
  const Qt3DCore::QGeometryView *const geometryView =
      m_ui->sceneContainer->getGeometryView();

  if (geometryView == nullptr) {
    qDebug() << "Can't approximate volume of nullptr geometry...";
    return;
  }

  VolumeApproximator volumeApproximator{*geometryView};
  const ApproximationResult points =
      volumeApproximator.getVolume(m_ui->samplesCountInput->value());

  for (const ApproximationPoint &point : points.points) {
    m_ui->sceneContainer->addSphere(point.m_point);
  }
}

DisplayWidget::~DisplayWidget() = default;
