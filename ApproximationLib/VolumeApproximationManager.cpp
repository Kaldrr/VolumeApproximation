#include <ApproximationLib/ParallelAlgorithmVolumeApproximationStrategy.h>
#include <ApproximationLib/VolumeApproximationManager.h>

#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/QGeometryView>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <execution>
#include <fstream>
#include <iterator>
#include <random>
#include <ranges>
#include <utility>

VolumeApproximationManager::VolumeApproximationManager(const Qt3DCore::QGeometry& geometry)
    : m_Geometry{&geometry}
    , m_VolumeApproximationStrategy{
          std::make_unique<ParallelAlgorithmVolumeApproximationStrategy>()}
{
}

ApproximationResult VolumeApproximationManager::getVolume(const int sampleSize)
{
	const std::vector<Triangle> geometryTriangles = getGeometryTriangles();
	if (geometryTriangles.empty())
	{
		return {};
	}

	constexpr float minValue = std::numeric_limits<float>::min();
	constexpr float maxValue = std::numeric_limits<float>::max();

	QVector3D minExtent{maxValue, maxValue, maxValue};
	QVector3D maxExtent{minValue, minValue, minValue};

	for (const Triangle& triangle : geometryTriangles)
	{
		for (const QVector3D& vertex : triangle)
		{
			for (int i = 0; i < 3; ++i)
			{
				minExtent[i] = std::min(vertex[i], minExtent[i]);
				maxExtent[i] = std::max(vertex[i], maxExtent[i]);
			}
		}
	}

	const std::vector<QVector3D> randomPoints =
	    generateRandomPoints(sampleSize, minExtent, maxExtent);


	return m_VolumeApproximationStrategy->ComputeVolume(geometryTriangles, randomPoints,
	    minExtent, maxExtent);
}

std::vector<Triangle> VolumeApproximationManager::getGeometryTriangles() const
{
	assert(m_Geometry);
	//assert(m_Geometry->primitiveType() == Qt3DCore::QGeometryView::PrimitiveType::Triangles);
	const QList<Qt3DCore::QAttribute*> attributes = m_Geometry->attributes();

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

	const std::vector<QVector3D> vertices = getVertices(**vertexAttributeItr);
	const std::vector<ushort> indexes = getIndices(**indexAttributeItr);

	assert(indexes.size() % 3 == 0);

	std::vector<Triangle> triangles{};

	for (std::size_t i = 0; i < indexes.size(); i += 3)
	{
		triangles.push_back(
		    {vertices[indexes[i]], vertices[indexes[i + 1]], vertices[indexes[i + 2]]});
	}

	return triangles;
}

std::vector<QVector3D> VolumeApproximationManager::generateRandomPoints(const int count,
    const QVector3D& minExtent,
    const QVector3D& maxExtent) const
{
	std::vector<QVector3D> randomPoints{};
	randomPoints.reserve(count);

	std::mt19937_64 dev{std::random_device{}()};
	std::uniform_real_distribution<float> rngX{minExtent.x(), maxExtent.x()};
	std::uniform_real_distribution<float> rngY{minExtent.y(), maxExtent.y()};
	std::uniform_real_distribution<float> rngZ{minExtent.z(), maxExtent.z()};

	for (int i = 0; i < count; ++i)
	{
		randomPoints.emplace_back(rngX(dev), rngY(dev), rngZ(dev));
	}

	return randomPoints;
}

std::vector<QVector3D> VolumeApproximationManager::getVertices(
    const Qt3DCore::QAttribute& vertexAttribute)
{
	assert(vertexAttribute.vertexBaseType() == Qt3DCore::QAttribute::Float);
	static_assert(sizeof(QVector3D) == sizeof(std::array<float, 3>));

	const QByteArray& verticesData = vertexAttribute.buffer()->data();
	const uint vertexCount = vertexAttribute.count();
	const uint offset = vertexAttribute.byteOffset();
	const uint stride = vertexAttribute.byteStride();
	const char* const ptr = verticesData.constData();

	std::vector<QVector3D> vertices{};
	vertices.reserve(vertexCount);
	for (uint i = 0; i < vertexCount; ++i)
	{
		const char* const vertexPtr = ptr + (i * stride) + offset;
		QVector3D& vertex = vertices.emplace_back(Qt::Initialization::Uninitialized);
		std::memcpy(&vertex, vertexPtr, sizeof(QVector3D));
	}

	return vertices;
}

std::vector<ushort> VolumeApproximationManager::getIndices(
    const Qt3DCore::QAttribute& indexAttribute)
{
	assert(indexAttribute.vertexBaseType() == Qt3DCore::QAttribute::UnsignedShort);
	const QByteArray& indexesData = indexAttribute.buffer()->data();
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
