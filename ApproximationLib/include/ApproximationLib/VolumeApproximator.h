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

class QTAPPROXLIB_EXPORT VolumeApproximator {
public:
  VolumeApproximator(const Qt3DCore::QGeometryView &geometry);

  std::vector<ApproximationPoint> getVolume(int sampleSize);

private:
  static std::vector<QVector3D>
  getVertices(const Qt3DCore::QAttribute &vertexAttribute);

  static std::vector<ushort>
  getIndexes(const Qt3DCore::QAttribute &indexAttribute);

private:
  const Qt3DCore::QGeometryView *const m_geometry{};
};
