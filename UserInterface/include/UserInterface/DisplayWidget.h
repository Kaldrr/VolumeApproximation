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

class ApproximationResult;

class DisplayWidget : public QWidget
{
	Q_OBJECT

public:
	DisplayWidget();
	explicit DisplayWidget(QWidget* parent);
	~DisplayWidget();

signals:
	void volumeApproximationRequested(const Qt3DCore::QGeometry* geometry, const int sampleCount);

private slots:
	void onStartButtonClick();
	void onLoadMeshButtonClick();
	void volumeApproximationDone(const ApproximationResult& approximationResult);

private:
	std::unique_ptr<Ui::DisplayWidget> m_ui;
	QThread m_workerThread;
};
