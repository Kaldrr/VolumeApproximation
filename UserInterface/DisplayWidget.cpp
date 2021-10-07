#include "ui_DisplayWidget.h"

#include <UserInterface/DisplayWidget.h>

#include <ApproximationLib/VolumeApproximator.h>

#include <QDebug>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>

#include <limits>

DisplayWidget::DisplayWidget() : DisplayWidget{nullptr} {}

DisplayWidget::DisplayWidget(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::DisplayWidget>()} {
  m_ui->setupUi(this);

  m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

  connect(m_ui->startButton, &QPushButton::clicked, this,
          &DisplayWidget::onStartButtonClick);
  connect(m_ui->loadMeshButton, &QPushButton::clicked, this,
          &DisplayWidget::onLoadMeshButtonClick);
  connect(m_ui->pointRadiusInput, &QDoubleSpinBox::valueChanged,
          m_ui->sceneContainer, &SceneContainer::updatePointsRadius);

  m_ui->pointRadiusInput->setValue(0.05);
}

void DisplayWidget::onStartButtonClick() {
  const Qt3DCore::QGeometry *const geometry =
      m_ui->sceneContainer->getGeometry();

  if (geometry == nullptr) {
    qDebug() << "Can't approximate volume of nullptr geometry...";
    return;
  }

  VolumeApproximator volumeApproximator{*geometry};
  const ApproximationResult points =
      volumeApproximator.getVolume(m_ui->samplesCountInput->value());

  constexpr float minValue = std::numeric_limits<float>::min();
  QVector3D maxDistance{minValue, minValue, minValue};

  for (const ApproximationPoint &point : points.points) {
    maxDistance.setX(std::max(std::abs(point.m_point.x()), maxDistance.x()));
    maxDistance.setY(std::max(std::abs(point.m_point.y()), maxDistance.y()));
    maxDistance.setZ(std::max(std::abs(point.m_point.z()), maxDistance.z()));

    m_ui->sceneContainer->addSphere(point.m_point);
  }

  m_ui->sceneContainer->setCameraDistance(maxDistance);
}

void DisplayWidget::onLoadMeshButtonClick() {
  const QString homeDirectory = QDir::homePath();
  QFileDialog fileDialog{this, "Mesh file", homeDirectory};
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setViewMode(QFileDialog::Detail);
  if (fileDialog.exec()) {
    const QList<QUrl> selectedFiles = fileDialog.selectedUrls();
    if (!selectedFiles.empty()) {
      assert(selectedFiles.size() == 1);
      m_ui->sceneContainer->setMainMeshPath(selectedFiles.front());
    }
  }
}

DisplayWidget::~DisplayWidget() = default;
