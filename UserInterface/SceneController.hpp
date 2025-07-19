#pragma once

#include <ApproximationLib/ICpuVolumeApproximation.hpp>
#include <PointsInstancing.hpp>

#include <QObject>
#include <QQuick3DGeometry>
#include <QVector3D>

// Qt functions/signals/slots shared with QML need to have special naming convention
// NOLINTBEGIN(readability-identifier-naming)

class SceneController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(
	    QList<QString> availableComputeEngines READ availableComputeEngines WRITE
	        setAvailableComputeEngines NOTIFY availableComputeEnginesChanged)
	Q_PROPERTY(QQuick3DGeometry* mainModel READ mainModel WRITE setMainModel NOTIFY
	               mainModelChanged)
	Q_PROPERTY(PointsInstancing* pointInstanceModel READ pointInstanceModel WRITE setPointInstanceModel
		NOTIFY pointInstanceModelChanged)
	QML_ELEMENT

public:
	SceneController();
	explicit SceneController(QObject* parent);
	~SceneController() noexcept override;

	SceneController(const SceneController&)                = delete;
	SceneController(SceneController&&) noexcept            = delete;
	SceneController& operator=(const SceneController&)     = delete;
	SceneController& operator=(SceneController&&) noexcept = delete;

	[[nodiscard]] const QList<QString>& availableComputeEngines() const noexcept
	{
		return m_AvailableComputeEngines;
	}
	void setAvailableComputeEngines(QList<QString> engines);

	[[nodiscard]] QQuick3DGeometry* mainModel() const noexcept
	{
		return m_Geometry;
	}
	void setMainModel(QQuick3DGeometry* geometry);

	[[nodiscard]] PointsInstancing* pointInstanceModel() const noexcept
	{
		return m_InstancingModel.get();
	}
	void setPointInstanceModel(PointsInstancing* pointInstanceModel) noexcept;

signals:
	void availableComputeEnginesChanged();
	void mainModelChanged();
	void repositionCamera(const QVector3D& position);
	void pointInstanceModelChanged();

public slots:
	void onLoadMeshButtonPress(const QUrl& filePath);
	void onCalculateButtonPress(const QString& pointsCountInput);
	void onComputeEngineChanged(int index);

private:
	QList<QString> m_AvailableComputeEngines;
	QQuick3DGeometry* m_Geometry{ nullptr };
	std::shared_ptr<VolumeApproximation::IVolumeApproximation>
	    m_VolumeApproximation;
	std::unique_ptr<PointsInstancing> m_InstancingModel;
};

// NOLINTEND(readability-identifier-naming)