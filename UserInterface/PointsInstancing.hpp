#pragma once

#include <QQuick3DInstancing>

#include <ApproximationLib/IVolumeApproximation.hpp>

class PointsInstancing : public QQuick3DInstancing
{
	Q_OBJECT
	QML_ELEMENT

public:
	PointsInstancing();
	explicit PointsInstancing(QQuick3DObject* parent);
	~PointsInstancing() noexcept override = default;

	PointsInstancing(const PointsInstancing&)                = delete;
	PointsInstancing(PointsInstancing&&) noexcept            = delete;
	PointsInstancing& operator=(const PointsInstancing&)     = delete;
	PointsInstancing& operator=(PointsInstancing&&) noexcept = delete;

	void SetPoints(std::span<const VolumeApproximation::ApproximationPoint> points);

protected:
	QByteArray getInstanceBuffer(int* instanceCount) override;

private:
	QByteArray m_InstanceBuffer{};
	std::size_t m_InstanceCount{};
};
