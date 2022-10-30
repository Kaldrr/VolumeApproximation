#pragma once

#include <QThread>
#include <QWidget>

#include <memory>

namespace Ui
{
class DisplayWidget;
}

namespace Qt3DCore
{
class QGeometry;
}

namespace VolumeApproximation
{
class ApproximationResult;
enum class ApproximationExecutor;
} // namespace VolumeApproximation

class DisplayWidget : public QWidget
{
	Q_OBJECT

public:
	DisplayWidget();
	explicit DisplayWidget(QWidget* parent);
	~DisplayWidget();

signals:
	void volumeApproximationRequested(const Qt3DCore::QGeometry* geometry, const int sampleCount);
	void computeEngineRequestd(VolumeApproximation::ApproximationExecutor executor);

private slots:
	void onStartButtonClick();
	void onLoadMeshButtonClick();
	void volumeApproximationDone(
	    const VolumeApproximation::ApproximationResult& approximationResult);
	void onComputeEngineChanged(int newIndex);

private:
	std::unique_ptr<Ui::DisplayWidget> m_ui;
	QThread m_workerThread;
};
