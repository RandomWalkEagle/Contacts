#include "mainwindowplugin.h"

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;

	FOpenAction = NULL;
	FMainWindow = new MainWindow(new QWidget, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
}

MainWindowPlugin::~MainWindowPlugin()
{
	delete FMainWindow;
}

void MainWindowPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Main Window");
	APluginInfo->description = tr("Allows other modules to place their widgets in the main window");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
}

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = FPluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(), SIGNAL(profileRenamed(const QString &, const QString &)),
				SLOT(onProfileRenamed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool MainWindowPlugin::initObjects()
{
	Action *action = new Action(this);
	action->setText(tr("Quit"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FMainWindow->mainMenu()->addAction(action,AG_MMENU_MAINWINDOW,true);

	FOpenAction = new Action(this);
	FOpenAction->setVisible(false);
	FOpenAction->setText(tr("Show roster"));
	FOpenAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_SHOW_ROSTER);
	connect(FOpenAction,SIGNAL(triggered(bool)),SLOT(onShowMainWindowByAction(bool)));

	if (FTrayManager)
		FTrayManager->contextMenu()->addAction(FOpenAction,AG_TMTM_MAINWINDOW,true);

	return true;
}

bool MainWindowPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MAINWINDOW_SHOW,true);
	Options::setDefaultValue(OPV_MAINWINDOW_SIZE,QSize(200,500));
	Options::setDefaultValue(OPV_MAINWINDOW_POSITION,QPoint(0,0));
	return true;
}

bool MainWindowPlugin::startPlugin()
{
	updateTitle();
	return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

void MainWindowPlugin::updateTitle()
{
	FMainWindow->setWindowTitle(CLIENT_NAME" | R" + FPluginManager->revision());
}

void MainWindowPlugin::showMainWindow()
{
	FMainWindow->show();
	WidgetManager::raiseWidget(FMainWindow);
	FMainWindow->activateWindow();
}

void MainWindowPlugin::onOptionsOpened()
{
	FMainWindow->resize(Options::node(OPV_MAINWINDOW_SIZE).value().toSize());
	FMainWindow->move(Options::node(OPV_MAINWINDOW_POSITION).value().toPoint());
	updateTitle();
	FOpenAction->setVisible(true);
	//if (Options::node(OPV_MAINWINDOW_SHOW).value().toBool())
	//	showMainWindow();
}

void MainWindowPlugin::onOptionsClosed()
{
	Options::node(OPV_MAINWINDOW_SHOW).setValue(FMainWindow->isVisible());
	Options::node(OPV_MAINWINDOW_SIZE).setValue(FMainWindow->size());
	Options::node(OPV_MAINWINDOW_POSITION).setValue(FMainWindow->pos());
	updateTitle();
	FMainWindow->close();
	FOpenAction->setVisible(false);
}

void MainWindowPlugin::onProfileRenamed(const QString &AProfile, const QString &ANewName)
{
	Q_UNUSED(AProfile);
	Q_UNUSED(ANewName);
	updateTitle();
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId<0 && AReason==QSystemTrayIcon::Trigger)
	{
		if (!FMainWindow->isVisible() && !Options::isNull())
			showMainWindow();
		else
			FMainWindow->close();
	}
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	showMainWindow();
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
