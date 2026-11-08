#pragma once

#include <ApproximationLib/ApproximationHelper.hpp>
#include <ApproximationLib/ICpuVolumeApproximation.hpp>
#include <PointsInstancing.hpp>

#include <QObject>
#include <QRunnable>
#include <QQuick3DGeometry>
#include <QVector3D>

#include <vector>

// Qt functions/signals/slots shared with QML need to have special naming convention
// NOLINTBEGIN(readability-identifier-naming)

class SceneController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QStringList availableComputeEngines READ availableComputeEngines)
	Q_PROPERTY(QQuick3DGeometry* mainModel READ mainModel NOTIFY mainModelChanged)
	Q_PROPERTY(PointsInstancing* pointInstanceModel READ pointInstanceModel)
	Q_PROPERTY(
	    double computedVolume READ computedVolume NOTIFY computedVolumeChanged)
	QML_ELEMENT

public:
	SceneController();
	explicit SceneController(QObject* parent);
	~SceneController() noexcept override = default;

	SceneController(const SceneController&)                = delete;
	SceneController(SceneController&&) noexcept            = delete;
	SceneController& operator=(const SceneController&)     = delete;
	SceneController& operator=(SceneController&&) noexcept = delete;

	// Used only through QML
	[[nodiscard]] QStringList availableComputeEngines() const;

	// Used only through QML
	[[nodiscard]] QQuick3DGeometry* mainModel() const noexcept
	{
		return m_Geometry.get();
	}

	// Used only through QML
	[[nodiscard]] PointsInstancing* pointInstanceModel() const noexcept
	{
		return m_InstancingModel.get();
	}

	[[nodiscard]] double computedVolume() const noexcept
	{
		return m_ComputedVolume;
	}

signals:
	void mainModelChanged();
	void repositionCamera(const QVector3D& position);
	void computedVolumeChanged();

public slots:
	void onLoadMeshButtonPress(const QUrl& filePath);
	void onCalculateButtonPress(const QString& pointsCountInput);
	void onComputeEngineChanged(int index);

private slots:
	void onVolumeComputed(
	    const std::shared_ptr<const VolumeApproximation::ApproximationResult>&
	        result);

private:
	std::vector<VolumeApproximation::ExecutorNamePair> m_AvailableEngines;

	// Geometry and VolumeApproximator can change, even when computation is ongoing
	// shared_ptr is made for exactly such scenarios
	std::shared_ptr<QQuick3DGeometry> m_Geometry;
	std::shared_ptr<VolumeApproximation::IVolumeApproximation>
	    m_VolumeApproximation;
	std::unique_ptr<PointsInstancing> m_InstancingModel;
	double m_ComputedVolume{};
};

class ComputeRunnable final
    : public QObject
    , public QRunnable
{
	Q_OBJECT

public:
	ComputeRunnable(const std::size_t pointsCount,
	                std::shared_ptr<const QQuick3DGeometry> geometry,
	                std::shared_ptr<VolumeApproximation::IVolumeApproximation>
	                    executor)
	    : m_PointsCount{ pointsCount }
	    , m_Geometry{ std::move(geometry) }
	    , m_Executor{ std::move(executor) }
	{
		if (m_Geometry == nullptr)
		{
			throw std::invalid_argument{ "Geometry cannot be null." };
		}
		if (m_Executor == nullptr)
		{
			throw std::invalid_argument{ "Volume approximation executor cannot be null." };
		}
	}

	void run() override;

signals:
	void VolumeComputed(
	    const std::shared_ptr<const VolumeApproximation::ApproximationResult>&
	        result);

private:
	std::size_t m_PointsCount{};
	std::shared_ptr<const QQuick3DGeometry> m_Geometry;
	std::shared_ptr<VolumeApproximation::IVolumeApproximation> m_Executor;
};

// NOLINTEND(readability-identifier-naming)