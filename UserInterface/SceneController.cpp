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

	m_AvailableComputeEngines.reserve(EnginesCount);
	for (const std::string_view engineName :
	     VolumeApproximation::ExecutorNames | std::views::values)
	{
		m_AvailableComputeEngines.emplace_back(QString::fromUtf8(engineName));
	}
	std::ranges::sort(m_AvailableComputeEngines);

	m_VolumeApproximation = VolumeApproximation::CreateApproximationStrategy(
	    VolumeApproximation::ExecutorNames.front().first);

	connect(this, &SceneController::mainModelChanged,
		m_InstancingModel.get(), &PointsInstancing::ClearPoints);
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
	std::unique_ptr runnable{ std::make_unique<ComputeRunnable>(
		pointsCountInput.toULongLong(), m_Geometry, m_VolumeApproximation) };
	runnable->setAutoDelete(true);
	connect(runnable.get(), &ComputeRunnable::VolumeComputed,
	        m_InstancingModel.get(), &PointsInstancing::SetResult);

	QThreadPool::globalInstance()->start(runnable.release());
}

void SceneController::onComputeEngineChanged(const int index)
{
	if (index < 0 ||
	    std::cmp_greater_equal(index, VolumeApproximation::ExecutorNames.size()))
	{
		fmt::println(stderr, "Invalid index for compute engine: {}", index);
		return;
	}
	const auto idx{ static_cast<std::size_t>(index) };

	const VolumeApproximation::ApproximationExecutor chosenExecutor =
	    VolumeApproximation::ExecutorNames.at(idx).first;
	m_VolumeApproximation.reset(
	    VolumeApproximation::CreateApproximationStrategy(chosenExecutor).release());
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

		const VolumeApproximation::ApproximationResult result{
			m_Executor->ComputeVolume(triangles, randomPoints, minimumExtent,
			                          maximumExtent)
		};
		// TODO: is this actually thread safe???
		// Sending a local variable through a signal
		emit VolumeComputed(result);

		// Get volume of cube that contains the entire mesh in cm3
		const float cubeVolume = std::abs(maximumExtent.X() - minimumExtent.X()) *
		                         std::abs(maximumExtent.Y() - minimumExtent.Y()) *
		                         std::abs(maximumExtent.Z() - minimumExtent.Z()) *
		                         1'000'000.0F;

		const std::int64_t pointsHit = std::ranges::count_if(
		    result.Points,
		    [](const VolumeApproximation::ApproximationPoint& point) {
			    return point.Status == VolumeApproximation::PointStatus::Inside;
		    });

		const float finalVolume =
		    cubeVolume * (static_cast<float>(pointsHit) /
		                  static_cast<float>(result.Points.size()));

		fmt::println("Mesh volume: {} cm3", finalVolume);
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