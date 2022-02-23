#include "ui_SceneContainer.h"

#include <UserInterface/SceneContainer.h>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include <Qt3DExtras/QAbstractCameraController>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QSphereMesh>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QRenderAspect>

#include <QMessageBox>

#include <cassert>
#include <chrono>
#include <limits>
#include <numbers>

using namespace std::chrono_literals;

SceneContainer::SceneContainer() : SceneContainer{nullptr} {};

SceneContainer::SceneContainer(QWidget* parent)
    : QWidget{parent}
    , m_ui{std::make_unique<Ui::SceneContainer>()}
    , m_3dWindow{nullptr, Qt3DRender::API::Vulkan}
    , m_rootEntity{std::make_unique<Qt3DCore::QEntity>()}
    , m_cameraController{std::make_unique<Qt3DExtras::QOrbitCameraController>(m_rootEntity.get())}
{
	m_ui->setupUi(this);
	layout()->addWidget(QWidget::createWindowContainer(&m_3dWindow, this));

	Qt3DRender::QCamera* const camera = m_3dWindow.camera();
	camera->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
	camera->setPosition({5.f, 0.f, 0.0f});
	camera->setViewCenter({0, 0, 0});
	camera->setUpVector({0, 1, 0});

	m_cameraController->setLinearSpeed(50.0f);
	m_cameraController->setLookSpeed(180.0f);
	m_cameraController->setCamera(camera);

	// m_sphereMesh.setRings(20);
	// m_sphereMesh.setSlices(20);
	// m_sphereMesh.setGenerateTangents(true);

	// TODO: Rethink showing spheres in the scene
	// m_sphereMaterial.setDiffuse(QColor{ 0, 0, 255, 0 });
	//  m_sphereMaterial.setAlphaBlendingEnabled(true);

	m_3dWindow.setRootEntity(m_rootEntity.get());

	// m_sphereMesh.view()->setPrimitiveType(
	//	Qt3DCore::QGeometryView::PrimitiveType::LineStrip);

	connect(&m_mainMesh, &Qt3DRender::QMesh::statusChanged, this,
	    &SceneContainer::onMeshStatusChanged);
}

const Qt3DCore::QGeometry* SceneContainer::getGeometry() const
{
	return m_mainMesh.geometry();
}

// void SceneContainer::addSphere(const QVector3D& position) {
//	auto* const newSphere = new Qt3DCore::QEntity{ m_rootEntity.get() };
//	m_spheres.push_back(newSphere);
//
//	auto* const sphereTransform = new Qt3DCore::QTransform{ newSphere };
//	sphereTransform->setTranslation(position);
//
//	newSphere->addComponent(&m_sphereMesh);
//	newSphere->addComponent(&m_sphereMaterial);
//	newSphere->addComponent(sphereTransform);
// }

void SceneContainer::OnMainMeshPath(const QUrl& meshFile)
{
	m_mainMesh.setSource(meshFile);
	if (m_mainMeshAddedToScene == false)
	{
		m_mainMeshAddedToScene = true;
		addMainMesh();
	}
}

void SceneContainer::OnCameraDistance(const QVector3D distance)
{
	m_cameraDistance = distance;
}

// void SceneContainer::updatePointsRadius(const double radius) {
//	m_sphereMesh.setRadius(radius);
// }

void SceneContainer::onMeshStatusChanged(const Qt3DRender::QMesh::Status newStatus)
{
	if (newStatus == Qt3DRender::QMesh::Status::Error)
	{
		const QString messageText =
		    QString{"Failed to load mesh from file '%1'"}.arg(m_mainMesh.source().toString());

		QMessageBox messageBox{QMessageBox::Icon::Critical, "Mesh load error", messageText,
		    QMessageBox::StandardButton::Ok, this};
		messageBox.exec();
	}
}

void SceneContainer::addMainMesh()
{
	auto* const newObject = new Qt3DCore::QEntity{m_rootEntity.get()};

	auto* const objectTransform = new Qt3DCore::QTransform{newObject};
	objectTransform->setTranslation({0, 0, 0});

	newObject->addComponent(&m_mainMesh);
	newObject->addComponent(&m_mainMeshMaterial);
	newObject->addComponent(objectTransform);
}

SceneContainer::~SceneContainer() = default;
