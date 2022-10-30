#pragma once

#include <ApproximationLib/ApproximationHelper.h>

#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QObject>

namespace Qt3DCore
{
class QGeometry;
class QAttribute;
} // namespace Qt3DCore

class VolumeApproximationManager : public QObject
{
	Q_OBJECT

public:
	VolumeApproximationManager();
	explicit VolumeApproximationManager(QObject* parent);

	constexpr static auto GetAvaliableExecutors()
	{
		return std::span{VolumeApproximation::ExecutorNames};
	}

public slots:
	void geometryVolumeRequested(const Qt3DCore::QGeometry* geometry, int sampleSize);
	void SetStrategy(VolumeApproximation::ApproximationExecutor strategy);

signals:
	void approximationFinished(const VolumeApproximation::ApproximationResult& approximationResult);

private:
	std::unique_ptr<VolumeApproximation::IVolumeApproximationStrategy>
	    m_VolumeApproximationStrategy{};
};
