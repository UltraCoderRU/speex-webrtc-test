#include "MainWindow.h"

#include <QtWidgets>

#include <iostream>

using namespace SpeexWebRTCTest;

const char* assertionList[] = {
    "QObject::connect: Cannot queue arguments",
    "QObject::connect: invalid null parameter",
    "QObject::disconnect: Unexpected null parameter",
    "Unexpected null receiver",
    "QObject: Cannot create children for a parent that is in a different thread.",
    "Timers cannot be stopped from another thread",
    "Timers cannot be started from another thread",
    "Socket notifiers cannot be enabled or disabled from another thread"};

QString formatType(QtMsgType type)
{
	switch (type)
	{
	case QtDebugMsg:
		return "[DEBUG]";
	case QtInfoMsg:
		return "[INFO ]";
	case QtWarningMsg:
		return "[WARN ]";
	case QtCriticalMsg:
		return "[CRIT ]";
	case QtFatalMsg:
		return "[FATAL]";
	default:
		return "";
	}
}

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QString category = context.category;

	// Some important Qt errors can be caught only from log
	// Consider assert for in such cases
	for (const char* filter : assertionList)
	{
		if (msg.contains(filter))
		{
			std::cerr << "Critical message from Qt!\n" << msg.toStdString() << "\n";
			Q_ASSERT(false);
			break;
		}
	}

	auto time = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");

	QString message;
	message = time;
	message.append(" ").append(formatType(type));
	message.append(" ").append(category).append(": ");
	message.append(msg);

	std::cout << message.toLocal8Bit().data() << std::endl;
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	qInstallMessageHandler(myMessageHandler);

	QFile f(":/qdarkstyle/style.qss");
	if (!f.exists())
		qWarning() << "Unable to set stylesheet, file not found\n";
	else
	{
		f.open(QFile::ReadOnly | QFile::Text);
		QTextStream ts(&f);
		app.setStyleSheet(ts.readAll());
	}

	MainWindow window;
	window.show();

	return app.exec();
}
