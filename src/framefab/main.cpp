#include "mainwindow.h"
#include <GL/glut.h>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	glutInit(&argc, argv);
	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}
