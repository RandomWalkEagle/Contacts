#include <signal.h>

#include <QUrl>
#include <QUuid>
#include <QLibrary>
#include <QSettings>
#include <QApplication>
#include <QScopedPointer>
#include <definitions/commandline.h>
#include <definitions/applicationreportparams.h>
#include <definitions/version.h>
#include <utils/log.h>
#include <utils/networking.h>
#include "pluginmanager.h"
#include "proxystyle.h"
#include "singleapp.h"

#ifdef Q_WS_WIN32
# include <thirdparty/holdemutils/RHoldemModule.h>
#endif

#ifdef Q_WS_MAC
# include <utils/macutils.h>
#endif

void generateSegfaultReport(int ASigNum)
{
	static bool fault = false;
	if (!fault)
	{
		fault = true;
		ReportError("SEGFAULT",QString("Segmentation fault with code %1").arg(ASigNum));
	}
	signal(ASigNum, SIG_DFL);
	exit(ASigNum);
}

int main(int argc, char *argv[])
{
#ifndef DEBUG_ENABLED // Catching these signals for release build
	foreach(int sig, QList<int>() << SIGSEGV << SIGILL << SIGFPE << SIGTERM << SIGABRT)
		signal(sig, generateSegfaultReport);
#endif

	// Generating system UUID
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"Rambler");
	QUuid systemUuid = settings.value("system/uuid").toString();
	if (systemUuid.isNull())
	{
		systemUuid = QUuid::createUuid();
		settings.setValue("system/uuid",systemUuid.toString());
	}
	Log::setStaticReportParam(ARP_SYSTEM_UUID,systemUuid.toString());

	// Checking for "--checkinstall" argument
	for (int i=1; i<argc; i++)
	{
		if (!strcmp(argv[i],CLO_CHECK_INSTALL))
			return 0;
	}

#ifndef Q_WS_MAC
	// WARNING! DIRTY HACK!
	// adding "-style windows" args
	// don't know why only this works...
	bool changeStyle = true;
	char **newArgv = new char*[argc+2];
	for (int i=0; i<argc; i++)
	{
		int argLen = strlen(argv[i])+1;
		newArgv[i] = new char[argLen];
		memcpy(newArgv[i],argv[i],argLen);
		changeStyle = changeStyle && strcmp(argv[i],"-style")!=0;
	}
	char paramName[] = "-style", paramValue[] = "windows", paramEmpty[] = "";
	newArgv[argc] = changeStyle ? paramName : paramEmpty;
	newArgv[argc+1] = changeStyle ? paramValue : paramEmpty;

	argc = argc+2;
	argv = newArgv;
#endif

	SingleApp app(argc, argv, "Rambler.Contacts");
	app.setApplicationName(CLIENT_NAME);
	app.setApplicationVersion(CLIENT_VERSION);

#if !defined(Q_WS_MAC)
# ifndef DEBUG_ENABLED
	if (app.isRunning())
	{
		app.sendMessage("show");
		return 0;
	}
# endif
#endif

	app.setQuitOnLastWindowClosed(false);

	QApplication::setStyle(new ProxyStyle);

	// fixing menu/combo/etc problems - disabling all animate/fade effects
	QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);
	QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
	QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);

	// utils
	app.addLibraryPath(app.applicationDirPath());
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

#ifdef Q_WS_MAC
	app.addLibraryPath(app.applicationDirPath() + "/../PlugIns");
	setAppFullScreenEnabled(true);
#endif

	// plugin manager
	PluginManager pm(&app);

	QObject::connect(&app, SIGNAL(messageAvailable(const QString&)), &pm, SLOT(showMainWindow()));

#ifdef Q_WS_WIN
	GUID guid = (GUID)QUuid(CLIENT_GUID);
	QScopedPointer<holdem_utils::RHoldemModule> holdem_module(new holdem_utils::RHoldemModule(guid));
	QObject::connect(holdem_module.data(), SIGNAL(shutdownRequested()), &pm, SLOT(shutdownRequested()));
#endif

	// Starting plugin manager
	pm.restart();

	int ret = app.exec();

#ifdef Q_WS_WIN
	for (int i = 0; i < argc; i++)
		delete argv[i];
	delete argv;
#endif

	return ret;
}
