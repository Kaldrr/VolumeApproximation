#include <ranges>
#include <utility>

#include <ApproximationLib/ApproximationHelper.hpp>

#include <SceneController.hpp>
#include <UserInterface/CustomGeometry.hpp>

#include <fmt/core.h>

#include <QThreadPool>
#include <QVector3D>

#include "UserInterface/GeometryHelpers.hpp"

SceneController::SceneController()
    : SceneController{ nullptr }
{
}

SceneController::SceneController(QObject* const parent)
    : QObject{ parent }
    , m_InstancingModel{ std::make_unique<PointsInstancing>() }
{
	constexpr std::size_t EnginesCount = VolumeApproximation::ExecutorNames.size();

	m_AvailableEngines.reserve(EnginesCount);
	// Offer only the engines usable on this machine (e.g. CUDA may be compiled
	// in, but there may be no GPU/driver at runtime). ExecutorNames is ordered by
	// preference, so the first available engine becomes the default.
	for (const auto& [executor, engineName] : VolumeApproximation::ExecutorNames)
	{
		std::unique_ptr<VolumeApproximation::IVolumeApproximation> strategy =
		    VolumeApproximation::CreateApproximationStrategy(executor);
		if (!strategy->IsAvailable())
		{
			fmt::println(stderr, "Compute engine '{}' is not available, skipping",
			             engineName);
			continue;
		}
		if (m_VolumeApproximation == nullptr)
		{
			m_VolumeApproximation = std::move(strategy);
		}
		m_AvailableEngines.emplace_back(executor, engineName);
	}

	connect(this, &SceneController::mainModelChanged, m_InstancingModel.get(),
	        &PointsInstancing::ClearPoints);
}

QStringList SceneController::availableComputeEngines() const
{
	QStringList names;
	names.reserve(static_cast<qsizetype>(m_AvailableEngines.size()));
	for (const std::string_view engineName :
	     m_AvailableEngines | std::views::values)
	{
		names.emplace_back(QString::fromUtf8(engineName));
	}
	return names;
}

void SceneController::onLoadMeshButtonPress(const QUrl& filePath)
{
	try
	{
		m_Geometry =
		    std::make_shared<CustomGeometry>(filePath.toLocalFile().toStdString());
		emit mainModelChanged();
		emit repositionCamera(m_Geometry->boundsMin() * QVector3D{ 0, 0, 5 });
	}
	catch (const std::exception& e)
	{
		fmt::println(stderr, "Failed to load new mesh: {}", e.what());
	}
}

void SceneController::onCalculateButtonPress(const QString& pointsCountInput)
{
	if (m_VolumeApproximation == nullptr || !m_VolumeApproximation->IsAvailable())
	{
		fmt::println(stderr, "Volume approximation engine is not available");
		return;
	}
	auto runnable{ std::make_unique<ComputeRunnable>(
		pointsCountInput.toULongLong(), m_Geometry, m_VolumeApproximation) };
	runnable->setAutoDelete(true);
	connect(runnable.get(), &ComputeRunnable::VolumeComputed,
	        m_InstancingModel.get(), &PointsInstancing::SetResult);
	connect(runnable.get(), &ComputeRunnable::VolumeComputed, this,
	        &SceneController::onVolumeComputed);

	QThreadPool::globalInstance()->start(runnable.release());
}

void SceneController::onComputeEngineChanged(const int index)
{
	if (index < 0 || std::cmp_greater_equal(index, m_AvailableEngines.size()))
	{
		fmt::println(stderr, "Invalid index for compute engine: {}", index);
		return;
	}
	const auto idx{ static_cast<std::size_t>(index) };

	const VolumeApproximation::ApproximationExecutor chosenExecutor =
	    m_AvailableEngines.at(idx).first;
	m_VolumeApproximation =
	    VolumeApproximation::CreateApproximationStrategy(chosenExecutor);
}

void ComputeRunnable::run()
{
	try
	{
		const VolumeApproximation::Vector3F minimumExtent{
			std::bit_cast<VolumeApproximation::Vector3F>(m_Geometry->boundsMin())
		};
		const VolumeApproximation::Vector3F maximumExtent{
			std::bit_cast<VolumeApproximation::Vector3F>(m_Geometry->boundsMax())
		};

		const std::vector triangles{ Helpers::ParseTrianglesFromGeometry(
			*m_Geometry) };
		const std::vector randomPoints{ Helpers::GenerateRandomPoints(
			m_PointsCount, minimumExtent, maximumExtent) };

		auto result{ std::make_shared<VolumeApproximation::ApproximationResult>(
			m_Executor->ComputeVolume(triangles, randomPoints, minimumExtent,
			                          maximumExtent)) };
		emit VolumeComputed(std::move(result));
	}
	catch (const std::exception& e)
	{
		fmt::println(stderr, "Failed to compute volume: {}", e.what());
	}
	catch (...)
	{
		fmt::println(stderr, "Unknown exception caught during volume computation.");
	}
}

void SceneController::onVolumeComputed(
    const std::shared_ptr<const VolumeApproximation::ApproximationResult>& result)
{
	m_ComputedVolume = result->Volume;
	emit computedVolumeChanged();
}