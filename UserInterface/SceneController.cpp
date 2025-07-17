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
		emit repositionCamera(minExtent * QVector3D{ 0, 0, 5 });
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Failed to load new mesh: {}\n", e.what());
	}
}

void SceneController::onCalculateButtonPress(const QString& pointsCountInput)
{
	if (m_Geometry != nullptr)
	{
		const auto& geometry     = *m_Geometry;
		const int attributeCount = geometry.attributeCount();
		for (int i{}; i < attributeCount; ++i)
		{
			const QQuick3DGeometry::Attribute attribute = geometry.attribute(i);
			QString semanticStr;
			switch (attribute.semantic)
			{
				case QQuick3DGeometry::Attribute::IndexSemantic:
					semanticStr = "IndexSemantic";
					break;
				case QQuick3DGeometry::Attribute::PositionSemantic:
					semanticStr = "PositionSemantic";
					break;
				case QQuick3DGeometry::Attribute::NormalSemantic:
					semanticStr = "NormalSemantic";
					break;
				case QQuick3DGeometry::Attribute::TexCoordSemantic:
					semanticStr = "TexCoordSemantic";
					break;
				case QQuick3DGeometry::Attribute::TangentSemantic:
					semanticStr = "TangentSemantic";
					break;
				case QQuick3DGeometry::Attribute::BinormalSemantic:
					semanticStr = "BinormalSemantic";
					break;
				case QQuick3DGeometry::Attribute::JointSemantic:
					semanticStr = "JointSemantic";
					break;
				case QQuick3DGeometry::Attribute::WeightSemantic:
					semanticStr = "WeightSemantic";
					break;
				case QQuick3DGeometry::Attribute::ColorSemantic:
					semanticStr = "ColorSemantic";
					break;
				case QQuick3DGeometry::Attribute::TargetPositionSemantic:
					semanticStr = "TargetPositionSemantic";
					break;
				case QQuick3DGeometry::Attribute::TargetNormalSemantic:
					semanticStr = "TargetNormalSemantic";
					break;
				case QQuick3DGeometry::Attribute::TargetTangentSemantic:
					semanticStr = "TargetTangentSemantic";
					break;
				case QQuick3DGeometry::Attribute::TargetBinormalSemantic:
					semanticStr = "TargetBinormalSemantic";
					break;
				case QQuick3DGeometry::Attribute::TexCoord1Semantic:
					semanticStr = "TexCoord1Semantic";
					break;
				// case QQuick3DGeometry::Attribute::TexCoord0Semantic: semanticStr
				// = "TexCoord0Semantic"; break;
				default:
					semanticStr = "Unknown";
					break;
			}
			QString componentTypeStr;
			switch (attribute.componentType)
			{
				case QQuick3DGeometry::Attribute::U16Type:
					componentTypeStr = "U16Type";
					break;
				case QQuick3DGeometry::Attribute::U32Type:
					componentTypeStr = "U32Type";
					break;
				case QQuick3DGeometry::Attribute::I32Type:
					componentTypeStr = "I32Type";
					break;
				case QQuick3DGeometry::Attribute::F32Type:
					componentTypeStr = "F32Type";
					break;
				default:
					componentTypeStr = "Unknown";
					break;
			}
			qDebug()
			    << "Attribute[" << i << "]"
			    << "semantic=" << semanticStr << ", offset=" << attribute.offset
			    << ", componentType=" << componentTypeStr;
		}

		// TODO: Extract vertex data here, instead of extra field in CustomGeometry
		const qsizetype vertexCount{ geometry.vertexData().size() /
			                         geometry.stride() };
	}
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

			const std::vector triangles{ Helpers::ParseTrianglesFromGeometry(*geometry) };
			const std::vector randomPoints{ Helpers::GenerateRandomPoints(
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