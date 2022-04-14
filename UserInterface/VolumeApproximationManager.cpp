#include <UserInterface/VolumeApproximationManager.h>

#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>

#include <algorithm>
#include <cassert>
#include <random>
#include <string_view>

#include <fmt/format.h>

using VolumeApproximation::Vector3F;

namespace
{
constexpr std::string_view EnumToString(
    const VolumeApproximation::ApproximationExecutor approximationStrategy)
{
	switch (approximationStrategy)
	{
		case VolumeApproximation::ApproximationExecutor::Cuda:
			return "CUDA";
		case VolumeApproximation::ApproximationExecutor::ParallelCpu:
			return "ParallelAlgorithm";
	}
	assert(false);
	return "";
}
} // namespace

VolumeApproximationManager::VolumeApproximationManager() : VolumeApproximationManager{nullptr}
{
}

VolumeApproximationManager::VolumeApproximationManager(QObject* const parent)
    : QObject{parent}, m_VolumeApproximationStrategy{VolumeApproximation::CreateDefaultStrategy()}
{
}

void VolumeApproximationManager::geometryVolumeRequested(const Qt3DCore::QGeometry* geometry,
    const int sampleSize)
{
	if (geometry == nullptr)
		return;

	const std::vector<VolumeApproximation::Triangle> geometryTriangles =
	    getGeometryTriangles(*geometry);
	if (geometryTriangles.empty())
	{
		return;
	}

	constexpr float minValue = std::numeric_limits<float>::min();
	constexpr float maxValue = std::numeric_limits<float>::max();

	Vector3F minExtent{maxValue, maxValue, maxValue};
	Vector3F maxExtent{minValue, minValue, minValue};

	for (const VolumeApproximation::Triangle& triangle : geometryTriangles)
	{
		for (const Vector3F& vertex : triangle)
		{
			for (int i = 0; i < 3; ++i)
			{
				minExtent[i] = std::min(vertex[i], minExtent[i]);
				maxExtent[i] = std::max(vertex[i], maxExtent[i]);
			}
		}
	}

	const std::vector<Vector3F> randomPoints =
	    generateRandomPoints(sampleSize, minExtent, maxExtent);

	emit approximationFinished(m_VolumeApproximationStrategy->ComputeVolume(geometryTriangles,
	    randomPoints, minExtent, maxExtent));
}

void VolumeApproximationManager::SetStrategy(VolumeApproximation::ApproximationExecutor strategy)
{
	m_VolumeApproximationStrategy = VolumeApproximation::CreateApproximationStrategy(strategy);
}

std::vector<VolumeApproximation::Triangle> VolumeApproximationManager::getGeometryTriangles(
    const Qt3DCore::QGeometry& geometry)
{
	// assert(m_Geometry->primitiveType() == Qt3DCore::QGeometryView::PrimitiveType::Triangles);
	const QList<Qt3DCore::QAttribute*> attributes = geometry.attributes();

	const QString vertexAttributeName = Qt3DCore::QAttribute::defaultPositionAttributeName();
	constexpr Qt3DCore::QAttribute::AttributeType indexAttributeType =
	    Qt3DCore::QAttribute::AttributeType::IndexAttribute;

	const auto vertexAttributeItr =
	    std::ranges::find_if(attributes, [&](const Qt3DCore::QAttribute* const attribute) {
		    return attribute && attribute->name() == vertexAttributeName;
	    });
	const auto indexAttributeItr =
	    std::ranges::find_if(attributes, [=](const Qt3DCore::QAttribute* const attribute) {
		    return attribute && attribute->attributeType() == indexAttributeType;
	    });

	if (vertexAttributeItr == attributes.end() || indexAttributeItr == attributes.end())
	{
		qWarning() << "Failed to find vertex/index attributes";
		return {};
	}

	const std::vector<Vector3F> vertices = getVertices(**vertexAttributeItr);
	const std::vector<ushort> indexes = getIndices(**indexAttributeItr);

	assert(indexes.size() % 3 == 0);

	std::vector<VolumeApproximation::Triangle> triangles{};

	for (std::size_t i = 0; i < indexes.size(); i += 3)
	{
		triangles.push_back(
		    {vertices[indexes[i]], vertices[indexes[i + 1]], vertices[indexes[i + 2]]});
	}

	return triangles;
}

std::vector<Vector3F> VolumeApproximationManager::generateRandomPoints(const int count,
    const Vector3F& minExtent,
    const Vector3F& maxExtent)
{
	std::vector<Vector3F> randomPoints{};
	randomPoints.reserve(count);

	std::mt19937_64 dev{std::random_device{}()};
	std::uniform_real_distribution<float> rngX{minExtent[0], maxExtent[0]};
	std::uniform_real_distribution<float> rngY{minExtent[1], maxExtent[1]};
	std::uniform_real_distribution<float> rngZ{minExtent[2], maxExtent[2]};

	for (int i = 0; i < count; ++i)
	{
		randomPoints.push_back({rngX(dev), rngY(dev), rngZ(dev)});
	}

	return randomPoints;
}

std::vector<Vector3F> VolumeApproximationManager::getVertices(
    const Qt3DCore::QAttribute& vertexAttribute)
{
	assert(vertexAttribute.vertexBaseType() == Qt3DCore::QAttribute::Float);
	static_assert(sizeof(Vector3F) == sizeof(std::array<float, 3>));

	const QByteArray verticesData = vertexAttribute.buffer()->data();
	const uint vertexCount = vertexAttribute.count();
	const uint offset = vertexAttribute.byteOffset();
	const uint stride = vertexAttribute.byteStride();
	const char* const ptr = verticesData.constData();

	std::vector<Vector3F> vertices{};
	vertices.reserve(vertexCount);
	for (uint i = 0; i < vertexCount; ++i)
	{
		const char* const vertexPtr = ptr + (i * stride) + offset;
		Vector3F& vertex = vertices.emplace_back();
		std::memcpy(&vertex, vertexPtr, sizeof(Vector3F));
	}

	return vertices;
}

std::vector<ushort> VolumeApproximationManager::getIndices(
    const Qt3DCore::QAttribute& indexAttribute)
{
	assert(indexAttribute.vertexBaseType() == Qt3DCore::QAttribute::UnsignedShort);
	const QByteArray indexesData = indexAttribute.buffer()->data();
	const uint indicesCount = indexAttribute.count();
	const uint offset = indexAttribute.byteOffset();
	const uint stride =
	    indexAttribute.byteStride() != 0 ? indexAttribute.byteStride() : sizeof(ushort);
	const char* const ptr = indexesData.constData();

	std::vector<ushort> indices{};
	indices.reserve(indicesCount);
	for (uint i = 0; i < indicesCount; ++i)
	{
		const char* const indexPtr = ptr + (i * stride) + offset;
		ushort index;
		std::memcpy(&index, indexPtr, sizeof(ushort));
		indices.push_back(index);
	}

	return indices;
}
