#include <ApproximationLib/VolumeApproximator.h>

#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/QGeometryView>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <ranges>
#include <utility>

VolumeApproximator::VolumeApproximator(const Qt3DCore::QGeometryView &geometry)
    : m_geometry{&geometry} {}

std::vector<ApproximationPoint>
VolumeApproximator::getVolume(int /*sampleSize*/) {
  assert(m_geometry->primitiveType() == Qt3DCore::QGeometryView::Triangles);
  const Qt3DCore::QGeometry *const geometry = m_geometry->geometry();
  assert(geometry);
  const QList<Qt3DCore::QAttribute *> attributes = geometry->attributes();

  const QString vertexAttributeName =
      Qt3DCore::QAttribute::defaultPositionAttributeName();
  constexpr auto indexAttributeType =
      Qt3DCore::QAttribute::AttributeType::IndexAttribute;

  const auto vertexAttributeItr = std::ranges::find_if(
      attributes, [&](const Qt3DCore::QAttribute *const attribute) {
        return attribute && attribute->name() == vertexAttributeName;
      });
  const auto indexAttributeItr = std::ranges::find_if(
      attributes, [=](const Qt3DCore::QAttribute *const attribute) {
        return attribute && attribute->attributeType() == indexAttributeType;
      });

  if (vertexAttributeItr == attributes.end() ||
      indexAttributeItr == attributes.end()) {
    qWarning() << "Failed to find vertex/index attributes";
    return {};
  }

  const std::vector<QVector3D> vertices = getVertices(**vertexAttributeItr);
  const std::vector<ushort> indexes = getIndexes(**indexAttributeItr);

  std::ofstream output{"output.obj"};
  for (const QVector3D &vertex : vertices) {
    output << "v " << vertex.x() << ' ' << vertex.y() << ' ' << vertex.z()
           << '\n';
  }
  output << '\n';
  for (std::size_t i = 0; i < indexes.size(); i += 3) {
    output << "f " << indexes.at(i) + 1 << ' ' << indexes.at(i + 1) + 1 << ' '
           << indexes.at(i + 2) + 1 << '\n';
  }

  return {};
}

std::vector<QVector3D>
VolumeApproximator::getVertices(const Qt3DCore::QAttribute &vertexAttribute) {
  assert(vertexAttribute.vertexBaseType() == Qt3DCore::QAttribute::Float);
  const QByteArray &verticesData = vertexAttribute.buffer()->data();
  const uint count = vertexAttribute.count();
  qDebug() << "vertex count: " << count;
  const uint offset = vertexAttribute.byteOffset();
  const uint stride = vertexAttribute.byteStride();
  const char *const ptr = verticesData.constData();

  std::vector<QVector3D> vertices{};

  for (uint i = 0; i < count; ++i) {
    const char *const vertexPtr = ptr + (i * stride) + offset;
    QVector3D vertex{Qt::Initialization::Uninitialized};
    std::memcpy(&vertex, vertexPtr, sizeof(QVector3D));
    vertices.push_back(std::move(vertex));
  }

  return vertices;
}

std::vector<ushort>
VolumeApproximator::getIndexes(const Qt3DCore::QAttribute &indexAttribute) {
  assert(indexAttribute.vertexBaseType() ==
         Qt3DCore::QAttribute::UnsignedShort);
  const QByteArray &indexesData = indexAttribute.buffer()->data();
  const uint count = indexAttribute.count();
  qDebug() << "indexCount:" << count;
  const uint offset = indexAttribute.byteOffset();
  const uint stride = indexAttribute.byteStride();
  const char *const ptr = indexesData.constData();

  std::vector<ushort> indexes{};
  if (stride == 0) {
    for (uint i = 0; i < count; ++i) {
      const char *const indexPtr = ptr + (i * sizeof(ushort)) + offset;
      ushort index;
      std::memcpy(&index, indexPtr, sizeof(index));
      indexes.push_back(index);
    }
  } else {
    for (uint i = 0; i < count; ++i) {
      const char *const indexPtr = ptr + (i * stride) + offset;
      ushort index;
      std::memcpy(&index, indexPtr, sizeof(index));
      indexes.push_back(index);
    }
  }

  return indexes;
}
