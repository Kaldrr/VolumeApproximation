#pragma once

#include <QWidget>

#include <Qt3DRender/QMesh>

#include <Qt3DExtras/QDiffuseSpecularMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/Qt3DWindow>

#include <memory>

namespace Ui
{
class SceneContainer;
} // namespace Ui

namespace Qt3DCore
{
class QEntity;
} // namespace Qt3DCore

namespace Qt3DExtras
{
class QAbstractCameraController;
} // namespace Qt3DExtras

class SceneContainer : public QWidget
{
	Q_OBJECT

public:
	SceneContainer();
	explicit SceneContainer(QWidget* parent);
	~SceneContainer();

	const Qt3DCore::QGeometry* getGeometry() const;

public slots:
	void setMainMeshPath(const QUrl& meshFile);
	void setCameraDistance(const QVector3D& distance);

private slots:
	void onMeshStatusChanged(Qt3DRender::QMesh::Status newStatus);

private:
	void addMainMesh();

private:
	std::unique_ptr<Ui::SceneContainer> m_ui;
	Qt3DExtras::Qt3DWindow m_3dWindow{};
	std::unique_ptr<Qt3DCore::QEntity> m_rootEntity;
	std::unique_ptr<Qt3DExtras::QAbstractCameraController> m_cameraController;
	float m_cameraAngle{0.f};
	QVector3D m_cameraDistance{1.f, 1.f, 1.f};

	// Main mesh
	Qt3DRender::QMesh m_mainMesh;
	Qt3DExtras::QDiffuseSpecularMaterial m_mainMeshMaterial{};
	bool m_mainMeshAddedToScene{false};
};
