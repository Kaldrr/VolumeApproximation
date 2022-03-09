#include <ui/ui_DisplayWidget.h>

#include <UserInterface/DisplayWidget.h>

#include <ApproximationLib/VolumeApproximationManager.h>

#include <QDebug>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>

#include <execution>
#include <limits>

#include <fmt/core.h>

namespace
{
class VolumeApproximationWorker : public QObject
{
	Q_OBJECT

signals:
	void approximationResultReady(const ApproximationResult& approximationResult);

public slots:
	void doWork(const Qt3DCore::QGeometry* geometry, const int sampleCount)
	{
		assert(geometry != nullptr);
		VolumeApproximationManager volumeApproximator{*geometry};
		ApproximationResult points = volumeApproximator.getVolume(sampleCount);
		emit approximationResultReady(std::move(points));
	}
};
} // namespace

DisplayWidget::DisplayWidget() : DisplayWidget{nullptr}
{
}

DisplayWidget::DisplayWidget(QWidget* parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::DisplayWidget>()}
{
	m_ui->setupUi(this);

	m_ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

	connect(m_ui->startButton, &QPushButton::clicked, this, &DisplayWidget::onStartButtonClick);
	connect(m_ui->loadMeshButton, &QPushButton::clicked, this,
	    &DisplayWidget::onLoadMeshButtonClick);
	// connect(m_ui->pointRadiusInput, &QDoubleSpinBox::valueChanged,
	//	m_ui->sceneContainer, &SceneContainer::updatePointsRadius);

	m_ui->pointRadiusInput->setValue(0.05);

	// Move worker to a sperate thread, so we don't block main ui-thread
	auto* const worker = new VolumeApproximationWorker{};
	worker->moveToThread(&m_workerThread);
	connect(&m_workerThread, &QThread::finished, worker, &QObject::deleteLater);
	connect(this, &DisplayWidget::volumeApproximationRequested, worker,
	    &VolumeApproximationWorker::doWork);
	connect(worker, &VolumeApproximationWorker::approximationResultReady, this,
	    &DisplayWidget::volumeApproximationDone);
	m_workerThread.start();
}

void DisplayWidget::onStartButtonClick()
{
	const Qt3DCore::QGeometry* const geometry = m_ui->sceneContainer->getGeometry();
	if (geometry == nullptr)
	{
		qDebug() << "Can't approximate volume of nullptr geometry...";
		return;
	}

	const int sampleCount = m_ui->samplesCountInput->value();
	if (sampleCount <= 0)
	{
		qDebug() << "Need at least 1 point when calculating volume...";
		return;
	}

	emit volumeApproximationRequested(geometry, sampleCount);
}

void DisplayWidget::onLoadMeshButtonClick()
{
	const QString homeDirectory = QDir::homePath();
	QFileDialog fileDialog{this, "Mesh file", homeDirectory};
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	fileDialog.setViewMode(QFileDialog::Detail);
	if (fileDialog.exec())
	{
		const QList<QUrl> selectedFiles = fileDialog.selectedUrls();
		if (!selectedFiles.empty())
		{
			assert(selectedFiles.size() == 1);
			m_ui->sceneContainer->setMainMeshPath(selectedFiles.front());
		}
	}
}

void DisplayWidget::volumeApproximationDone(const ApproximationResult& points)
{
	// Get volume of cube that contains the entire mesh in cm3
	const long double cubeVolume = (points.m_maxExtent.x() - points.m_minExtent.x()) *
	                               (points.m_maxExtent.y() - points.m_minExtent.y()) *
	                               (points.m_maxExtent.z() - points.m_minExtent.z()) * 1'000'000.0;

	const std::size_t pointsHit = std::count_if(std::execution::par_unseq, begin(points.m_points),
	    end(points.m_points), [](const ApproximationPoint& point) {
		    return point.m_status == ApproximationPoint::PointStatus::Inside;
	    });

	const double finalVolume = cubeVolume * (static_cast<long double>(pointsHit) /
	                                            static_cast<long double>(points.m_points.size()));

	fmt::print("Mesh volume: {} cm3\n", finalVolume);

	// m_ui->sceneContainer->setCameraDistance(maxDistance);

	// TODO: Use VolumeApproxmiation, to make a graph
	// x-axis: points used to approximate
	// y-axis: volume from the points
}

DisplayWidget::~DisplayWidget()
{
	m_workerThread.quit();
	m_workerThread.wait();
}


#include "DisplayWidget.moc"