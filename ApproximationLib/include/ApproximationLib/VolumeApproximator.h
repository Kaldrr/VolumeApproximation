#pragma once

#include <ApproximationLib/ApproximationLib_global.h>

#include <vector>

#include <QtGui/QVector3D>

namespace Qt3DCore {
class QGeometryView;
class QAttribute;
} // namespace Qt3DCore

struct QTAPPROXLIB_EXPORT ApproximationPoint {
  enum class PointStatus : std::uint8_t { Inside, Outside };
  QVector3D m_point{};
  PointStatus m_status{PointStatus::Outside};
};

struct QTAPPROXLIB_EXPORT ApproximationResult {
  std::vector<ApproximationPoint> points;
  double volume{0.0};
};

using Triangle = std::array<QVector3D, 3>;

class QTAPPROXLIB_EXPORT VolumeApproximator {
public:
  VolumeApproximator(const Qt3DCore::QGeometryView &geometry);

  ApproximationResult getVolume(int sampleSize);

private:
  std::vector<Triangle> getGeometryTriangles() const;
  std::vector<QVector3D> getRandomPoints(int count, const QVector3D &minExtent,
                                         const QVector3D &maxExtent) const;

  static std::vector<QVector3D>
  getVertices(const Qt3DCore::QAttribute &vertexAttribute);

  static std::vector<ushort>
  getIndices(const Qt3DCore::QAttribute &indexAttribute);

private:
  const Qt3DCore::QGeometryView *const m_geometryView{};
};
