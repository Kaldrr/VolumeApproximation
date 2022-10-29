#include <ui/ui_DisplayWidget.h>

#include <UserInterface/DisplayWidget.h>
#include <UserInterface/VolumeApproximationManager.h>

#include <QDebug>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>

#include <execution>
#include <limits>

#include <fmt/core.h>

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

	// m_ui->pointRadiusInput->setValue(0.05);
	constexpr std::span executors = VolumeApproximationManager::GetAvaliableExecutors();
	for (const auto [enumValue, name] : executors)
	{
		m_ui->engineSelect->addItem(QString::fromUtf8(name.data(), name.size()),
		    QVariant::fromValue(enumValue));
	}
	connect(m_ui->engineSelect, &QComboBox::currentIndexChanged, this,
	    &DisplayWidget::onComputeEngineChanged);

	// Move worker to a sperate thread, so we don't block main ui-thread
	auto* const volumeManager = new VolumeApproximationManager{};
	volumeManager->moveToThread(&m_workerThread);
	connect(&m_workerThread, &QThread::finished, volumeManager, &QObject::deleteLater);
	connect(this, &DisplayWidget::volumeApproximationRequested, volumeManager,
	    &VolumeApproximationManager::geometryVolumeRequested);
	connect(volumeManager, &VolumeApproximationManager::approximationFinished, this,
	    &DisplayWidget::volumeApproximationDone);
	connect(this, &DisplayWidget::computeEngineRequestd, volumeManager,
	    &VolumeApproximationManager::SetStrategy);
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

void DisplayWidget::volumeApproximationDone(const VolumeApproximation::ApproximationResult& points)
{
	// Get volume of cube that contains the entire mesh in cm3
	const long double cubeVolume = (points.m_maxExtent.x() - points.m_minExtent.x()) *
	                               (points.m_maxExtent.y() - points.m_minExtent.y()) *
	                               (points.m_maxExtent.z() - points.m_minExtent.z()) * 1'000'000.0;

	const std::size_t pointsHit = std::count_if(std::execution::par_unseq, begin(points.m_points),
	    end(points.m_points), [](const VolumeApproximation::ApproximationPoint& point) {
		    return point.m_status == VolumeApproximation::PointStatus::Inside;
	    });

	const double finalVolume = cubeVolume * (static_cast<long double>(pointsHit) /
	                                            static_cast<long double>(points.m_points.size()));

	fmt::print("Mesh volume: {} cm3\n", finalVolume);
}

void DisplayWidget::onComputeEngineChanged(int newIndex)
{
	if (newIndex == -1)
		return;

	const QVariant enumVariant = m_ui->engineSelect->itemData(newIndex);
	const auto executor = enumVariant.value<VolumeApproximation::ApproximationExecutor>();

	emit computeEngineRequestd(executor);
}

DisplayWidget::~DisplayWidget()
{
	m_workerThread.quit();
	m_workerThread.wait();
}