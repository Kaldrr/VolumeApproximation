#include <ApproximationLib/VolumeApproximator.h>

#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <fstream>
#include <iterator>
#include <random>
#include <ranges>
#include <utility>

VolumeApproximator::VolumeApproximator(const Qt3DCore::QGeometry& geometry)
	: m_geometry{ &geometry }
{}

ApproximationResult VolumeApproximator::getVolume(const int sampleSize) {
	const std::vector<Triangle> geometryTriangles = getGeometryTriangles();
	if (geometryTriangles.empty()) {
		return {};
	}

	constexpr float minValue = std::numeric_limits<float>::min();
	constexpr float maxValue = std::numeric_limits<float>::max();

	QVector3D minExtent{ maxValue, maxValue, maxValue };
	QVector3D maxExtent{ minValue, minValue, minValue };

	for (const Triangle& triangle : geometryTriangles) {
		for (const QVector3D& vertex : triangle) {
			for (int i = 0; i < 3; ++i) {
				minExtent[i] = std::min(vertex[i], minExtent[i]);
				maxExtent[i] = std::max(vertex[i], maxExtent[i]);
			}
		}
	}

	const std::vector<QVector3D> randomPoints =
		getRandomPoints(sampleSize, minExtent, maxExtent);

	ApproximationResult result{
		.points = {},
		.volume = 0.0,
		.minExtent = minExtent,
		.maxExtent = maxExtent
	};

	const QVector3D outsidePoint = minExtent - QVector3D{ 1.f, 2.f, 3.f };

	for (const QVector3D& rayOriginPoint: randomPoints) {
		result.points.push_back(computePoint(rayOriginPoint, outsidePoint, geometryTriangles));
	}

	return result;
}

ApproximationPoint VolumeApproximator::computePoint(
	const QVector3D origin,
	const QVector3D pointPosition,
	const std::vector<Triangle>& meshTriangles) const
{
	constexpr float Epsilon = 0.0000001f;

	const QVector3D rayVector = (origin - pointPosition).normalized();
	std::size_t trianglesPassed = 0;

	for (const Triangle& triangle : meshTriangles) {
		const QVector3D edge1 = triangle[1] - triangle[0];
		const QVector3D edge2 = triangle[2] - triangle[0];
		const QVector3D h = QVector3D::crossProduct(rayVector, edge2);
		const float a = QVector3D::dotProduct(edge1, h);

		if (-Epsilon < a && a < Epsilon) {
			continue;
		}

		const float f = 1.0f / a;
		const QVector3D s = origin - triangle[0];
		const float u = f * QVector3D::dotProduct(s, h);
		if (u < 0.0f || u > 1.0f) {
			continue;
		}

		const QVector3D q = QVector3D::crossProduct(s, edge1);
		const float v = f * QVector3D::dotProduct(rayVector, q);
		if (0.0f > v || (u + v) > 1.0f) {
			continue;
		}

		const float t = f * QVector3D::dotProduct(edge2, q);
		if (t > Epsilon) {
			++trianglesPassed;
		}
	}

	return {
		.m_point = pointPosition,
		.m_status = trianglesPassed % 2 == 0 ? ApproximationPoint::PointStatus::Outside
											 : ApproximationPoint::PointStatus::Inside
	};
}

std::vector<Triangle> VolumeApproximator::getGeometryTriangles() const {
	//  assert(m_geometryView->primitiveType() ==
	//  Qt3DCore::QGeometryView::Triangles);
	assert(m_geometry);
	const QList<Qt3DCore::QAttribute*> attributes = m_geometry->attributes();

	const QString vertexAttributeName =
		Qt3DCore::QAttribute::defaultPositionAttributeName();
	constexpr Qt3DCore::QAttribute::AttributeType indexAttributeType =
		Qt3DCore::QAttribute::AttributeType::IndexAttribute;

	const auto vertexAttributeItr = std::ranges::find_if(attributes,
		[&](const Qt3DCore::QAttribute* const attribute) {
			return attribute && attribute->name() == vertexAttributeName;
		});
	const auto indexAttributeItr = std::ranges::find_if(attributes,
		[=](const Qt3DCore::QAttribute* const attribute) {
			return attribute && attribute->attributeType() == indexAttributeType;
		});

	if (vertexAttributeItr == attributes.end() ||
		indexAttributeItr == attributes.end()) {
		qWarning() << "Failed to find vertex/index attributes";
		return {};
	}

	const std::vector<QVector3D> vertices = getVertices(**vertexAttributeItr);
	const std::vector<ushort> indexes = getIndices(**indexAttributeItr);

	assert(indexes.size() % 3 == 0);

	std::vector<Triangle> triangles{};

	for (std::size_t i = 0; i < indexes.size(); i += 3) {
		triangles.push_back({ vertices[indexes[i]],
							  vertices[indexes[i + 1]],
							  vertices[indexes[i + 2]]
			});
	}

	return triangles;
}

std::vector<QVector3D> VolumeApproximator::getRandomPoints(const int count,
	const QVector3D minExtent,
	const QVector3D maxExtent) const
{
	std::vector<QVector3D> randomPoints{};
	randomPoints.reserve(count);

	std::mt19937_64 dev{ std::random_device{}() };
	std::uniform_real_distribution<float> rngX{ minExtent.x(), maxExtent.x() };
	std::uniform_real_distribution<float> rngY{ minExtent.y(), maxExtent.y() };
	std::uniform_real_distribution<float> rngZ{ minExtent.z(), maxExtent.z() };

	for (int i = 0; i < count; ++i) {
		randomPoints.emplace_back(rngX(dev), rngY(dev), rngZ(dev));
	}

	return randomPoints;
}

std::vector<QVector3D> VolumeApproximator::getVertices(const Qt3DCore::QAttribute& vertexAttribute)
{
	assert(vertexAttribute.vertexBaseType() == Qt3DCore::QAttribute::Float);
	static_assert(sizeof(QVector3D) == sizeof(std::array<float, 3>));

	const QByteArray& verticesData = vertexAttribute.buffer()->data();
	const uint count = vertexAttribute.count();
	const uint offset = vertexAttribute.byteOffset();
	const uint stride = vertexAttribute.byteStride();
	const char* const ptr = verticesData.constData();

	std::vector<QVector3D> vertices{};

	for (uint i = 0; i < count; ++i) {
		const char* const vertexPtr = ptr + (i * stride) + offset;
		QVector3D vertex{ Qt::Initialization::Uninitialized };
		std::memcpy(&vertex, vertexPtr, sizeof(QVector3D));
		vertices.push_back(std::move(vertex));
	}

	return vertices;
}

std::vector<ushort> VolumeApproximator::getIndices(const Qt3DCore::QAttribute& indexAttribute)
{
	assert(indexAttribute.vertexBaseType() ==
		Qt3DCore::QAttribute::UnsignedShort);
	const QByteArray& indexesData = indexAttribute.buffer()->data();
	const uint count = indexAttribute.count();
	const uint offset = indexAttribute.byteOffset();
	const uint stride = indexAttribute.byteStride() != 0 ? indexAttribute.byteStride()
		: sizeof(ushort);
	const char* const ptr = indexesData.constData();

	std::vector<ushort> indices{};
	for (uint i = 0; i < count; ++i) {
		const char* const indexPtr = ptr + (i * stride) + offset;
		ushort index;
		std::memcpy(&index, indexPtr, sizeof(ushort));
		indices.push_back(index);
	}

	return indices;
}
