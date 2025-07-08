#include <ranges>

#include <SceneController.hpp>
#include <CustomGeometry.hpp>

#include <fmt/core.h>

#include <ApproximationLib/ApproximationHelper.hpp>

SceneController::SceneController()
    : SceneController{ nullptr }
{
}

SceneController::SceneController(QObject* const parent)
    : QObject{ parent }
{
	constexpr auto EnginesCount =
	    static_cast<qsizetype>(VolumeApproximation::ExecutorNames.size());

	m_AvailableComputeEngines.reserve(EnginesCount);
	for (const std::string_view engineName :
	     VolumeApproximation::ExecutorNames | std::views::values)
	{
		m_AvailableComputeEngines.emplace_back(QString::fromUtf8(engineName));
	}
	std::ranges::sort(m_AvailableComputeEngines);
}

void SceneController::setAvailableComputeEngines(QList<QString> engines)
{
	m_AvailableComputeEngines = std::move(engines);
	std::ranges::sort(m_AvailableComputeEngines);
	emit availableComputeEnginesChanged();
}

void SceneController::setMainModel(QQuick3DGeometry* geometry)
{
	if (m_Geometry != nullptr)
	{
		m_Geometry->deleteLater();
	}
	m_Geometry = geometry;
	emit mainModelChanged();
}

void SceneController::onLoadMeshButtonPress(const QUrl& filePath)
{
	fmt::println("LoadMeshButton pressed in SceneView!");
	try
	{
		// TODO: Set camera on the model?
		setMainModel(new CustomGeometry{filePath.path().toStdString()});
	} catch (const std::exception& e)
	{
		fmt::print(stderr, "Failed to load new mesh: {}\n", e.what());
	}
}

void SceneController::onCalculateButtonPress(const int index)
{
	fmt::println("CalculateButton pressed in SceneView: {}!",
	             m_AvailableComputeEngines.at(index).toStdString());
}