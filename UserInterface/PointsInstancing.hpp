#pragma once

#include <QQuick3DInstancing>

#include <ApproximationLib/IVolumeApproximation.hpp>

class PointsInstancing : public QQuick3DInstancing
{
	Q_OBJECT

public:
	PointsInstancing();
	explicit PointsInstancing(QQuick3DObject* parent);
	~PointsInstancing() noexcept override = default;

	PointsInstancing(const PointsInstancing&)                = delete;
	PointsInstancing(PointsInstancing&&) noexcept            = delete;
	PointsInstancing& operator=(const PointsInstancing&)     = delete;
	PointsInstancing& operator=(PointsInstancing&&) noexcept = delete;

public slots:
	void SetResult(const VolumeApproximation::ApproximationResult& result);
	void SetPoints(std::span<const VolumeApproximation::ApproximationPoint> points);
	void ClearPoints();

protected:
	QByteArray getInstanceBuffer(int* instanceCount) override;

private:
	QByteArray m_InstanceBuffer;
	std::size_t m_InstanceCount{};
};
