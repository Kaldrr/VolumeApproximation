#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <Qt3DCore/QGeometry>

#include <fmt/core.h>

namespace
{
// void RegisterMetatypes()
// {
// 	qRegisterMetaType<Qt3DCore::QGeometry*>();
// 	qRegisterMetaType<const Qt3DCore::QGeometry*>();
// }
} // namespace

int main(int argc, char** argv)
{
#ifndef NDEBUG
	fmt::print("Executing in debug mode\n");
#endif

	/*
	 * TODO: Instanced rendering https://doc.qt.io/qt-6/qquick3dinstancing.html
	 * https://doc.qt.io/qt-6/qml-qtquick3d-model.html
	 */

	const QGuiApplication app{ argc, argv };

	QQmlApplicationEngine engine{};
	QObject::connect(
	    &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
	    []() {
		    QCoreApplication::exit(-1);
	    },
	    Qt::QueuedConnection);
	engine.loadFromModule("UserInterface", "MainWindow");

	return QGuiApplication::exec();
}
