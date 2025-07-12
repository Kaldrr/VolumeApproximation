#include <random>
#include <ranges>
#include <utility>

#include <ApproximationLib/ApproximationHelper.hpp>

#include <CustomGeometry.hpp>
#include <SceneController.hpp>

#include <fmt/core.h>

#include <QThreadPool>
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
		auto* const newGeometry{ new CustomGeometry{
			filePath.toLocalFile().toStdString() } };
		setMainModel(newGeometry);
		const QVector3D minExtent{ newGeometry->boundsMin() };
		// TODO: Implement some better camera controll...
		emit repositionCamera(minExtent * QVector3D{ 0, 0, 2 });
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Failed to load new mesh: {}\n", e.what());
	}
}

void SceneController::onCalculateButtonPress(const QString& pointsCountInput)
{
	if (!m_VolumeApproximation->IsAvailable())
	{
		fmt::print(stderr, "Volume approximation engine is not available.\n");
		return;
	}
	const auto* const geometry{ qobject_cast<const CustomGeometry*>(m_Geometry) };
	if (geometry == nullptr)
	{
		fmt::print(stderr,
		           "Can't approximate default model, load a model first.\n");
		return;
	}

	QThreadPool::globalInstance()->start([geometry,
	                                      executor = m_VolumeApproximation,
	                                      pointsCount =
	                                          pointsCountInput.toULongLong()] {
		try
		{
			const QVector3D minExtentQt{ geometry->boundsMin() };
			const VolumeApproximation::Vector3F minimumExtent{
				std::bit_cast<VolumeApproximation::Vector3F>(minExtentQt)
			};
			const QVector3D maxExtentQt{ geometry->boundsMax() };
			const VolumeApproximation::Vector3F maximumExtent{
				std::bit_cast<VolumeApproximation::Vector3F>(maxExtentQt)
			};

			const std::vector triangles{ ParseTrianglesFromGeometry(*geometry) };
			const std::vector randomPoints{ GenerateRandomPoints(
				pointsCount, minimumExtent, maximumExtent) };

			const VolumeApproximation::ApproximationResult result{
				executor->ComputeVolume(triangles, randomPoints, minimumExtent,
				                        maximumExtent)
			};

			// Get volume of cube that contains the entire mesh in cm3
			const float cubeVolume =
			    std::abs(maximumExtent.X() - minimumExtent.X()) *
			    std::abs(maximumExtent.Y() - minimumExtent.Y()) *
			    std::abs(maximumExtent.Z() - minimumExtent.Z()) * 1'000'000.0F;

			const std::int64_t pointsHit = std::ranges::count_if(
			    result.Points,
			    [](const VolumeApproximation::ApproximationPoint& point) {
				    return point.Status == VolumeApproximation::PointStatus::Inside;
			    });

			const float finalVolume =
			    cubeVolume * (static_cast<float>(pointsHit) /
			                  static_cast<float>(result.Points.size()));

			fmt::print("Mesh volume: {} cm3\n", finalVolume);
		}
		catch (const std::exception& e)
		{
			fmt::print(stderr, "Failed to compute volume: {}\n", e.what());
		}
		catch (...)
		{
			fmt::print(stderr,
			           "Unknown exception caught during volume computation.\n");
		}
	});
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