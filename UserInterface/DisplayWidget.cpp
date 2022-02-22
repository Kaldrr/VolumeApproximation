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

#include <fmt/core.h>


DisplayWidget::DisplayWidget() : DisplayWidget{ nullptr } {}

DisplayWidget::DisplayWidget(QWidget* parent)
	: QWidget{ parent }
	, m_ui{ std::make_unique<Ui::DisplayWidget>() }
{
	m_ui->setupUi(this);

	m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

	connect(m_ui->startButton, &QPushButton::clicked, this,
		&DisplayWidget::onStartButtonClick);
	connect(m_ui->loadMeshButton, &QPushButton::clicked, this,
		&DisplayWidget::onLoadMeshButtonClick);
	//connect(m_ui->pointRadiusInput, &QDoubleSpinBox::valueChanged,
	//	m_ui->sceneContainer, &SceneContainer::updatePointsRadius);

	m_ui->pointRadiusInput->setValue(0.05);
}

void DisplayWidget::onStartButtonClick() {
	const Qt3DCore::QGeometry* const geometry =
		m_ui->sceneContainer->getGeometry();

	if (geometry == nullptr) {
		qDebug() << "Can't approximate volume of nullptr geometry...";
		return;
	}

	const int sampleCount = m_ui->samplesCountInput->value();

	if (sampleCount <= 0) {
		qDebug() << "Need at least 1 point when calculating volume...";
		return;
	}

	VolumeApproximator volumeApproximator{ *geometry };
	const ApproximationResult points = volumeApproximator.getVolume(sampleCount);

	// Get volume of cube that contains the entire mesh in cm3
	const long double cubeVolume =
		(points.maxExtent.x() - points.minExtent.x()) *
		(points.maxExtent.y() - points.minExtent.y()) *
		(points.maxExtent.z() - points.minExtent.z()) * 1'000'000.0;

	const std::size_t pointsHit = std::ranges::count_if(points.points,
		[](const ApproximationPoint& point) {
			return point.m_status == ApproximationPoint::PointStatus::Inside;
		}
	);

	const double finalVolume = cubeVolume * 
		(static_cast<long double>(pointsHit) / static_cast<long double>(points.points.size()));

	fmt::print("Mesh volume: {} cm3\n", finalVolume);

	//m_ui->sceneContainer->setCameraDistance(maxDistance);

	// TODO: Use VolumeApproxmiation, to make a graph
	// x-axis: points used to approximate
	// y-axis: volume from the points
}

void DisplayWidget::onLoadMeshButtonClick() {
	const QString homeDirectory = QDir::homePath();
	QFileDialog fileDialog{ this, "Mesh file", homeDirectory };
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	fileDialog.setViewMode(QFileDialog::Detail);
	if (fileDialog.exec()) {
		const QList<QUrl> selectedFiles = fileDialog.selectedUrls();
		if (!selectedFiles.empty()) {
			assert(selectedFiles.size() == 1);
			emit m_ui->sceneContainer->setMainMeshPath(selectedFiles.front());
		}
	}
}

DisplayWidget::~DisplayWidget() = default;
