#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <Qt3DCore/QGeometry>

#include <fmt/core.h>

#include <QQuick3DGeometry>
#include <QQuickStyle>

namespace
{
[[maybe_unused]]
void RegisterMetatypes()
{
	qRegisterMetaType<QQuick3DGeometry>();
	qRegisterMetaType<const QQuick3DGeometry>();
	qRegisterMetaType<QQuick3DGeometry*>();
	qRegisterMetaType<const QQuick3DGeometry*>();
	qRegisterMetaType<const QQuick3DGeometry&>();
}
} // namespace

int main(int argc, char** argv) noexcept
{
#ifndef NDEBUG
	fmt::print("Executing in debug mode\n");
#endif
	/*
	 * TODO:
	 * Instanced rendering https://doc.qt.io/qt-6/qquick3dinstancing.html
	 * https://doc.qt.io/qt-6/qml-qtquick3d-model.html
	 * QtQuick3D
	 */
	try
	{
		const QGuiApplication app{ argc, argv };
		QQuickStyle::setStyle(QStringLiteral("Universal"));

		QQmlApplicationEngine engine{};
		QObject::connect(
		    &engine, &QQmlApplicationEngine::objectCreationFailed,
		    &app, [] {
			    QCoreApplication::exit(-1);
		    },
		    Qt::QueuedConnection);
		engine.loadFromModule(QStringLiteral("UserInterface"),
		                      QStringLiteral("MainWindow"));

		return QGuiApplication::exec();
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Exception: {}\n", e.what());
	}
	catch (...)
	{
		fmt::print(stderr, "Unknown exception caught\n");
	}
	return -1;
}
