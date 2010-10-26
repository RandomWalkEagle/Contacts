#include "mainwindowplugin.h"

#include <QApplication>
#include <QDesktopWidget>

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;

	FOpenAction = NULL;
	FActivationChanged = QTime::currentTime();
	FMainWindow = new MainWindow(new QWidget, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
	FMainWindow->installEventFilter(this);
	WidgetManager::setWindowSticky(FMainWindow,true);

	FNoticeWidget = new NoticeWidget(NULL);
	connect(FNoticeWidget,SIGNAL(noticeInserted(int)),SLOT(onInternalNoticeInserted(int)));
	connect(FNoticeWidget,SIGNAL(noticeRemoved(int)),SLOT(onInternalNoticeRemoved(int)));
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

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
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
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

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
	FOpenAction->setText(tr("Open Virtus"));
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
	Options::setDefaultValue(OPV_MAINWINDOW_STAYONTOP,false);

	if (FOptionsManager)
	{
		FOptionsManager->insertServerOption(OPV_MAINWINDOW_STAYONTOP);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

bool MainWindowPlugin::startPlugin()
{
	IInternalNotice notice;
	notice.priority = 100;
	notice.caption = "caption";
	notice.message = "message asdfk asldkfj asldkfj asldkfj asldkfj asldfkj asdlfkj asldkfj";
	noticeWidget()->insertNotice(notice);
	updateTitle();
	return true;
}

QMultiMap<int, IOptionsWidget *> MainWindowPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_MAINWINDOW, FOptionsManager->optionsNodeWidget(Options::node(OPV_MAINWINDOW_STAYONTOP),tr("Stay on top of other windows"),AParent));
	}
	return widgets;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

void MainWindowPlugin::showMainWindow() const
{
	if (!Options::isNull())
	{
		FMainWindow->show();
		correctWindowPosition();
		WidgetManager::raiseWidget(FMainWindow);
		FMainWindow->activateWindow();
	}
}

IInternalNoticeWidget *MainWindowPlugin::noticeWidget() const
{
	return FNoticeWidget;
}

void MainWindowPlugin::updateTitle()
{
	FMainWindow->setWindowTitle(CLIENT_NAME" | R" + FPluginManager->revision());
}

void MainWindowPlugin::correctWindowPosition() const
{
	QRect windowRect = FMainWindow->geometry();
	QRect screenRect = qApp->desktop()->availableGeometry(qApp->desktop()->screenNumber(windowRect.topLeft()));
	if (!screenRect.isEmpty() && !screenRect.adjusted(10,10,-10,-10).intersects(windowRect))
	{
		if (windowRect.right() <= screenRect.left())
			windowRect.moveLeft(screenRect.left());
		else if (windowRect.left() >= screenRect.right())
			windowRect.moveRight(screenRect.right());
		if (windowRect.top() >= screenRect.bottom())
			windowRect.moveBottom(screenRect.bottom());
		else if (windowRect.bottom() <= screenRect.top())
			windowRect.moveTop(screenRect.top());
		FMainWindow->move(windowRect.topLeft());
	}
}

bool MainWindowPlugin::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched==FMainWindow && AEvent->type()==QEvent::ActivationChange)
		FActivationChanged = QTime::currentTime();
	return QObject::eventFilter(AWatched,AEvent);
}

void MainWindowPlugin::onOptionsOpened()
{
	FMainWindow->resize(Options::node(OPV_MAINWINDOW_SIZE).value().toSize());
	FMainWindow->move(Options::node(OPV_MAINWINDOW_POSITION).value().toPoint());
	FOpenAction->setVisible(true);
	//if (Options::node(OPV_MAINWINDOW_SHOW).value().toBool())
	//	showMainWindow();
	onOptionsChanged(Options::node(OPV_MAINWINDOW_STAYONTOP));
	updateTitle();
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

void MainWindowPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MAINWINDOW_STAYONTOP)
	{
		bool show = FMainWindow->isVisible();
		if (ANode.value().toBool())
			FMainWindow->setWindowFlags(FMainWindow->windowFlags() | Qt::WindowStaysOnTopHint);
		else
			FMainWindow->setWindowFlags(FMainWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if (show)
			showMainWindow();
	}
}

void MainWindowPlugin::onProfileRenamed(const QString &AProfile, const QString &ANewName)
{
	Q_UNUSED(AProfile);
	Q_UNUSED(ANewName);
	updateTitle();
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId<0 && AReason==QSystemTrayIcon::DoubleClick)
	{
		if (FMainWindow->isActiveWindow() || qAbs(FActivationChanged.msecsTo(QTime::currentTime()))<qApp->doubleClickInterval())
			FMainWindow->close();
		else
			showMainWindow();
	}
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	showMainWindow();
}

void MainWindowPlugin::onInternalNoticeInserted(int ANoticeId)
{
	Q_UNUSED(ANoticeId);
	if (FMainWindow->bottomWidget()->indexOf(FNoticeWidget->instance()) < 0)
	{
		FMainWindow->bottomWidget()->addWidget(FNoticeWidget->instance());
		FMainWindow->bottomWidget()->setVisible(true);
	}
}

void MainWindowPlugin::onInternalNoticeRemoved(int ANoticeId)
{
	Q_UNUSED(ANoticeId);
	if (FNoticeWidget->noticeQueue().isEmpty())
	{
		FMainWindow->bottomWidget()->removeWidget(FNoticeWidget->instance());
	}
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
