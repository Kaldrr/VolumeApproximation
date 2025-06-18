#include <ui/ui_DisplayWidget.h>

#include <UserInterface/DisplayWidget.h>
#include <UserInterface/VolumeApproximationManager.h>

#include <QDebug>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>

#include <limits>
#include <version>

#include <fmt/core.h>

DisplayWidget::DisplayWidget()
    : DisplayWidget{ nullptr }
{
}

DisplayWidget::DisplayWidget(QWidget* parent)
    : QWidget{ parent }
    , m_Ui{ std::make_unique<Ui::DisplayWidget>() }
{
	m_Ui->setupUi(this);

	m_Ui->samplesCountInput->setRange(0, std::numeric_limits<int>::max());

	connect(m_Ui->startButton, &QPushButton::clicked, this,
	        &DisplayWidget::OnStartButtonClick);
	connect(m_Ui->loadMeshButton, &QPushButton::clicked, this,
	        &DisplayWidget::OnLoadMeshButtonClick);

	// m_Ui->pointRadiusInput->setValue(0.05);
	for (constexpr std::span Executors =
	         VolumeApproximationManager::GetAvailableExecutors();
	     const auto& [enumValue, name] : Executors)
	{
		m_Ui->engineSelect->addItem(
		    QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())),
		    QVariant::fromValue(enumValue));
	}
	connect(m_Ui->engineSelect, &QComboBox::currentIndexChanged, this,
	        &DisplayWidget::OnComputeEngineChanged);

	// Move worker to a separate thread, so we don't block the main ui-thread
	auto* const volumeManager = new VolumeApproximationManager{};
	volumeManager->moveToThread(&m_WorkerThread);
	connect(&m_WorkerThread, &QThread::finished, volumeManager,
	        &QObject::deleteLater);
	connect(this, &DisplayWidget::VolumeApproximationRequested, volumeManager,
	        &VolumeApproximationManager::geometryVolumeRequested);
	connect(volumeManager, &VolumeApproximationManager::approximationFinished, this,
	        &DisplayWidget::VolumeApproximationDone);
	connect(this, &DisplayWidget::ComputeEngineRequested, volumeManager,
	        &VolumeApproximationManager::SetStrategy);
	m_WorkerThread.start();
}

void DisplayWidget::OnStartButtonClick()
{
	const Qt3DCore::QGeometry* const geometry = m_Ui->sceneContainer->getGeometry();
	if (geometry == nullptr)
	{
		qDebug() << "Can't approximate volume of nullptr geometry...";
		return;
	}

	const int sampleCount = m_Ui->samplesCountInput->value();
	if (sampleCount <= 0)
	{
		qDebug() << "Need at least 1 point when calculating volume...";
		return;
	}

	emit VolumeApproximationRequested(geometry, sampleCount);
}

void DisplayWidget::OnLoadMeshButtonClick()
{
	const QString homeDirectory = QDir::homePath();
	QFileDialog fileDialog{ this, "Mesh file", homeDirectory };
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	fileDialog.setViewMode(QFileDialog::Detail);
	if (fileDialog.exec() == QDialog::Accepted)
	{
		if (const QList<QUrl> selectedFiles = fileDialog.selectedUrls();
		    !selectedFiles.empty())
		{
			assert(selectedFiles.size() == 1);
			m_Ui->sceneContainer->setMainMeshPath(selectedFiles.front());
		}
	}
}

void DisplayWidget::VolumeApproximationDone(
    const VolumeApproximation::ApproximationResult& approximationResult)
{
	const VolumeApproximation::Vector3F minExtent = approximationResult.MinExtent;
	const VolumeApproximation::Vector3F maxExtent = approximationResult.MaxExtent;

	// Get volume of cube that contains the entire mesh in cm3
	const long double cubeVolume = (maxExtent.X() - minExtent.X()) *
	                               (maxExtent.Y() - minExtent.Y()) *
	                               (maxExtent.Z() - minExtent.Z()) * 1'000'000.0;

	const std::size_t pointsHit = std::ranges::count_if(
	    approximationResult.Points,
	    [](const VolumeApproximation::ApproximationPoint& point) {
		    return point.Status == VolumeApproximation::PointStatus::Inside;
	    });

	const long double finalVolume =
	    cubeVolume * (static_cast<long double>(pointsHit) /
	                  static_cast<long double>(approximationResult.Points.size()));

	fmt::print("Mesh volume: {} cm3\n", finalVolume);
}

void DisplayWidget::OnComputeEngineChanged(int newIndex)
{
	if (newIndex == -1)
	{
		return;
	}

	const QVariant enumVariant = m_Ui->engineSelect->itemData(newIndex);
	const auto executor =
	    enumVariant.value<VolumeApproximation::ApproximationExecutor>();

	emit ComputeEngineRequested(executor);
}

DisplayWidget::~DisplayWidget()
{
	m_WorkerThread.quit();
	m_WorkerThread.wait();
}
