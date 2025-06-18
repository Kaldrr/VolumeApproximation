#pragma once

#include <QThread>
#include <QWidget>

#include <memory>

namespace Ui
{
class DisplayWidget;
} // namespace Ui

namespace Qt3DCore
{
class QGeometry;
} // namespace Qt3DCore

namespace VolumeApproximation
{
class ApproximationResult;
enum class ApproximationExecutor : std::uint8_t;
} // namespace VolumeApproximation

class DisplayWidget final : public QWidget
{
	Q_OBJECT

public:
	DisplayWidget();
	explicit DisplayWidget(QWidget* parent);
	~DisplayWidget() override;

	DisplayWidget(const DisplayWidget& other)                = delete;
	DisplayWidget(DisplayWidget&& other) noexcept            = delete;
	DisplayWidget& operator=(const DisplayWidget& other)     = delete;
	DisplayWidget& operator=(DisplayWidget&& other) noexcept = delete;

signals:
	void VolumeApproximationRequested(const Qt3DCore::QGeometry* geometry,
	                                  int sampleCount);
	void ComputeEngineRequested(
	    VolumeApproximation::ApproximationExecutor executor);

private slots:
	void OnStartButtonClick();
	void OnLoadMeshButtonClick();
	static void VolumeApproximationDone(
	    const VolumeApproximation::ApproximationResult& approximationResult);
	void OnComputeEngineChanged(int newIndex);

private:
	std::unique_ptr<Ui::DisplayWidget> m_Ui;
	QThread m_WorkerThread;
};
