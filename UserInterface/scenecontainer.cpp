#include "ui_scenecontainer.h"

#include <UserInterface/scenecontainer.h>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include <Qt3DInput/QInputAspect>

#include <Qt3DExtras/QAbstractCameraController>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QRenderAspect>

SceneContainer::SceneContainer() : SceneContainer{nullptr} {};

SceneContainer::SceneContainer(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::SceneContainer>()},
      m_3dWindow{nullptr /*, Qt3DRender::API::Vulkan*/},
      m_rootEntity{std::make_unique<Qt3DCore::QEntity>()},
      m_cameraController{std::make_unique<Qt3DExtras::QOrbitCameraController>(
          m_rootEntity.get())} {
  m_ui->setupUi(this);
  layout()->addWidget(QWidget::createWindowContainer(&m_3dWindow, this));

  Qt3DRender::QCamera *const camera = m_3dWindow.camera();
  camera->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
  camera->setPosition(QVector3D(40.f, 40.f, 40.0f));
  camera->setViewCenter(QVector3D(0, 0, 0));
  camera->setUpVector({0, 1, 0});

  m_cameraController->setLinearSpeed(50.0f);
  m_cameraController->setLookSpeed(180.0f);
  m_cameraController->setCamera(camera);

  m_sphereMesh.setRadius(0.2f);
  m_sphereMesh.setGenerateTangents(true);

  auto material = std::make_unique<Qt3DExtras::QPhongMaterial>();
  material->setAmbient(QColor{0, 0, 255});
  m_sphereMaterial = std::move(material);

  m_3dWindow.setRootEntity(m_rootEntity.get());

  for (int i = -3; i < 3; ++i) {
    for (int j = -3; j < 3; ++j) {
      for (int k = -3; k < 3; ++k) {
        addSphere({float(i), float(j), float(k)});
      }
    }
  }
  qDebug() << "Number of spheres: " << m_spheres.size();
}

void SceneContainer::addSphere(const QVector3D &position) {
  Qt3DCore::QEntity *const newSphere =
      m_spheres.emplace_back(new Qt3DCore::QEntity{m_rootEntity.get()});

  auto *const sphereTransform = new Qt3DCore::QTransform{};
  sphereTransform->setTranslation(position);

  newSphere->addComponent(&m_sphereMesh);
  newSphere->addComponent(m_sphereMaterial.get());
  newSphere->addComponent(sphereTransform);
}

SceneContainer::~SceneContainer() = default;
