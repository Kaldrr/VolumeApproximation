#pragma once

#include <QWidget>

#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/Qt3DWindow>

#include <memory>

namespace Ui {
class SceneContainer;
} // namespace Ui

namespace Qt3DCore {
class QEntity;
} // namespace Qt3DCore

namespace Qt3DRender {
class QMaterial;
} // namespace Qt3DRender

namespace Qt3DExtras {
class QAbstractCameraController;
} // namespace Qt3DExtras

class SceneContainer : public QWidget {
  Q_OBJECT

public:
  SceneContainer();
  explicit SceneContainer(QWidget *parent);
  ~SceneContainer();

  const Qt3DCore::QGeometryView *getGeometryView() const;

private:
  void addSphere(const QVector3D &position);
  void updateCamera();

private:
  std::unique_ptr<Ui::SceneContainer> m_ui;
  Qt3DExtras::Qt3DWindow m_3dWindow{};
  std::unique_ptr<Qt3DCore::QEntity> m_rootEntity;
  std::vector<Qt3DCore::QEntity *> m_spheres;
  std::unique_ptr<Qt3DExtras::QAbstractCameraController> m_cameraController;
  Qt3DExtras::QSphereMesh m_sphereMesh;
  std::unique_ptr<Qt3DRender::QMaterial> m_sphereMaterial{};
  float m_cameraAngle{0.f};
};
