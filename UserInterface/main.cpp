#include <QApplication>

#include <UserInterface/MainWindow.h>

#include <Qt3DCore/QGeometry>

#include <fmt/core.h>

void RegisterMetatypes()
{
	// There has to be a better way to do this...
	// qRegisterMetaType<Qt3DCore::QGeometry>();
	qRegisterMetaType<Qt3DCore::QGeometry*>();
	qRegisterMetaType<const Qt3DCore::QGeometry*>();
}

int main(int argc, char** argv)
{
#ifndef NDEBUG
	fmt::print("We in debug mode :)\n");
#endif
	RegisterMetatypes();

	QApplication app{argc, argv};

	MainWindow mainWindow{};
	mainWindow.show();

	return QApplication::exec();
}
