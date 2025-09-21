#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuick3DGeometry>

#include <fmt/core.h>

int main(int argc, char** argv) noexcept
{
	try
	{
		const QGuiApplication app{ argc, argv };
		QQuickStyle::setStyle(QStringLiteral("Universal"));

		QQmlApplicationEngine engine{};
		QObject::connect(
		    &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
		    [] {
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
