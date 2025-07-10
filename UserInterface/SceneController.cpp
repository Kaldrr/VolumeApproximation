#include <random>
#include <ranges>
#include <utility>

#include <ApproximationLib/ApproximationHelper.hpp>

#include <CustomGeometry.hpp>
#include <SceneController.hpp>

#include <fmt/core.h>

#include <QVector3D>

namespace
{
[[nodiscard]]
std::vector<VolumeApproximation::Vector3F> GenerateRandomPoints(
    const std::size_t count,
    const VolumeApproximation::Vector3F& minExtent,
    const VolumeApproximation::Vector3F& maxExtent)
{
	std::vector<VolumeApproximation::Vector3F> randomPoints{};
	randomPoints.reserve(count);

	std::mt19937_64 dev{ std::random_device{}() };
	std::uniform_real_distribution<float> rngX{ minExtent[0], maxExtent[0] };
	std::uniform_real_distribution<float> rngY{ minExtent[1], maxExtent[1] };
	std::uniform_real_distribution<float> rngZ{ minExtent[2], maxExtent[2] };

	for (std::size_t i = 0; i < count; ++i)
	{
		randomPoints.emplace_back(rngX(dev), rngY(dev), rngZ(dev));
	}
	return randomPoints;
}

[[nodiscard]]
std::vector<VolumeApproximation::Triangle> ParseTrianglesFromGeometry(
    const CustomGeometry& geometry)
{
	const std::vector<float>& vertexData        = geometry.GetVertexData();
	const std::vector<std::uint32_t>& indexData = geometry.GetIndexData();

	// TODO: This is better...?
	std::vector<VolumeApproximation::Triangle> triangles{};
	triangles.reserve(indexData.size() / 3);

	for (std::size_t i{ 0 }; i < indexData.size(); i += 3)
	{
		// Vertex data is stored as:
		// [x, y, z, nx, ny, nz, u, v]
		// :(
		const std::uint32_t xIdx{ indexData[i] };
		const std::uint32_t yIdx{ indexData[i + 1] };
		const std::uint32_t zIdx{ indexData[i + 2] };

		const float* const xVertex{ &vertexData[xIdx] };
		const float* const yVertex{ &vertexData[yIdx] };
		const float* const zVertex{ &vertexData[zIdx] };

		const VolumeApproximation::Triangle triangle{
			VolumeApproximation::Vector3F{ xVertex[0], xVertex[1], xVertex[2] },
			VolumeApproximation::Vector3F{ yVertex[0], yVertex[1], yVertex[2] },
			VolumeApproximation::Vector3F{ zVertex[0], zVertex[1], zVertex[2] }
		};
		triangles.emplace_back(triangle);
	}
	assert(triangles.size() == indexData.size() / 3);

	return triangles;
}
} // namespace

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

	m_VolumeApproximation = VolumeApproximation::CreateApproximationStrategy(
	    VolumeApproximation::ExecutorNames.front().first);
}

SceneController::~SceneController() noexcept
{
	if (m_Geometry != nullptr)
	{
		m_Geometry->deleteLater();
	}
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
		auto* const newGeometry{ new CustomGeometry{
			filePath.toLocalFile().toStdString() } };
		setMainModel(newGeometry);
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Failed to load new mesh: {}\n", e.what());
	}
}

void SceneController::onCalculateButtonPress(const QString& pointsCountInput,
                                             const int index)
{
	try
	{
		fmt::println("CalculateButton pressed in SceneView: {}!",
		             m_AvailableComputeEngines.at(index).toStdString());

		if (!m_VolumeApproximation->IsAvailable())
		{
			fmt::print(stderr, "Volume approximation engine is not available.\n");
			return;
		}

		const auto* const geometry{ qobject_cast<CustomGeometry*>(m_Geometry) };
		if (geometry == nullptr)
		{
			fmt::print(stderr,
			           "Can't approximate default model, load a model first.\n");
			return;
		}

		const std::size_t pointsCount{ pointsCountInput.toULongLong() };

		const QVector3D minExtentQt{ m_Geometry->boundsMin() };
		const VolumeApproximation::Vector3F minimumExtent{ minExtentQt.x(),
			                                               minExtentQt.y(),
			                                               minExtentQt.z() };
		const QVector3D maxExtentQt{ m_Geometry->boundsMax() };
		const VolumeApproximation::Vector3F maximumExtent{ maxExtentQt.x(),
			                                               maxExtentQt.y(),
			                                               maxExtentQt.z() };

		const std::vector triangles{ ParseTrianglesFromGeometry(*geometry) };
		const std::vector randomPoints{ GenerateRandomPoints(
			pointsCount, minimumExtent, maximumExtent) };

		[[maybe_unused]]
		const auto result = m_VolumeApproximation->ComputeVolume(
		    triangles, randomPoints, minimumExtent, maximumExtent);
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Failed to compute volume: {}\n", e.what());
	}
	catch (...)
	{
		fmt::print(stderr, "Unknown exception caught during volume computation.\n");
	}
}

void SceneController::onComputeEngineChanged(const int index)
{
	if (index < 0 ||
	    std::cmp_greater_equal(index, VolumeApproximation::ExecutorNames.size()))
	{
		fmt::print(stderr, "Invalid index for compute engine: {}\n", index);
		return;
	}
	const auto idx{ static_cast<std::size_t>(index) };

	const VolumeApproximation::ApproximationExecutor chosenExecutor =
	    VolumeApproximation::ExecutorNames.at(idx).first;
	m_VolumeApproximation =
	    VolumeApproximation::CreateApproximationStrategy(chosenExecutor);
}