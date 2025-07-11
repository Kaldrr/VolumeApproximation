#pragma once

#include <ApproximationLib/ICpuVolumeApproximation.hpp>

#include <QObject>
#include <QQuick3DGeometry>

class SceneController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(
	    QList<QString> availableComputeEngines READ availableComputeEngines WRITE
	        setAvailableComputeEngines NOTIFY availableComputeEnginesChanged)
	Q_PROPERTY(QQuick3DGeometry* mainModel READ mainModel WRITE setMainModel NOTIFY
	               mainModelChanged)
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

signals:
	void availableComputeEnginesChanged();
	void mainModelChanged();

public slots:
	void onLoadMeshButtonPress(const QUrl& filePath);
	void onCalculateButtonPress(const QString& pointsCountInput);
	void onComputeEngineChanged(int index);

private:
	QList<QString> m_AvailableComputeEngines;
	QQuick3DGeometry* m_Geometry{ nullptr };
	std::shared_ptr<VolumeApproximation::IVolumeApproximation>
	    m_VolumeApproximation;
};