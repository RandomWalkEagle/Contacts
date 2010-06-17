#include "notifications.h"

#include <QVBoxLayout>

#define ADR_NOTIFYID                    Action::DR_Parametr1

Notifications::Notifications()
{
	FAvatars = NULL;
	FRosterPlugin = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FTrayManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FOptionsManager = NULL;
	FMainWindowPlugin = NULL;

	FActivateAll = NULL;
	FRemoveAll = NULL;
	FNotifyMenu = NULL;

	FNotifyId = 0;
	FSound = NULL;
}

Notifications::~Notifications()
{
	delete FActivateAll;
	delete FRemoveAll;
	delete FNotifyMenu;
	delete FSound;
}

void Notifications::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Notifications Manager");
	APluginInfo->description = tr("Allows other modules to notify the user of the events");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
}

bool Notifications::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int, QSystemTrayIcon::ActivationReason)));
			connect(FTrayManager->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTrayNotifyRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyActivated(IRosterIndex *, int)),
				SLOT(onRosterNotifyActivated(IRosterIndex *, int)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyRemovedByIndex(IRosterIndex *, int)),
				SLOT(onRosterNotifyRemoved(IRosterIndex *, int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool Notifications::initObjects()
{
	FSoundOnOff = new Action(this);
	FSoundOnOff->setToolTip(tr("Enable/Disable notifications sound"));
	FSoundOnOff->setIcon(RSR_STORAGE_MENUICONS, MNI_NOTIFICATIONS_SOUND_ON);
	connect(FSoundOnOff,SIGNAL(triggered(bool)),SLOT(onSoundOnOffActionTriggered(bool)));

	FActivateAll = new Action(this);
	FActivateAll->setVisible(false);
	FActivateAll->setText(tr("Activate All Notifications"));
	FActivateAll->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS_ACTIVATE_ALL);
	connect(FActivateAll,SIGNAL(triggered(bool)),SLOT(onTrayActionTriggered(bool)));

	FRemoveAll = new Action(this);
	FRemoveAll->setVisible(false);
	FRemoveAll->setText(tr("Remove All Notifications"));
	FRemoveAll->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS_REMOVE_ALL);
	connect(FRemoveAll,SIGNAL(triggered(bool)),SLOT(onTrayActionTriggered(bool)));

	FNotifyMenu = new Menu;
	FNotifyMenu->setTitle(tr("Pending Notifications"));
	FNotifyMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS);
	FNotifyMenu->menuAction()->setVisible(false);

	if (FTrayManager)
	{
		//FTrayManager->contextMenu()->addAction(FActivateAll,AG_TMTM_NOTIFICATIONS,false);
		//FTrayManager->contextMenu()->addAction(FRemoveAll,AG_TMTM_NOTIFICATIONS,false);
		//FTrayManager->contextMenu()->addAction(FNotifyMenu->menuAction(),AG_TMTM_NOTIFICATIONS,false);
	}

	if (FMainWindowPlugin)
	{
		//FMainWindowPlugin->mainWindow()->topToolBarChanger()->insertAction(FSoundOnOff,TBG_MWTTB_NOTIFICATIONS_SOUND);
	}

	return true;
}

bool Notifications::initSettings()
{
	Options::setDefaultValue(OPV_NOTIFICATIONS_SOUND,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_ROSTERICON,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_POPUPWINDOW,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_CHATWINDOW,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TRAYICON,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TRAYACTION,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_AUTOACTIVATE,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_EXPANDGROUP,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_NOSOUNDIFDND,false);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_NOTIFICATIONS, OPN_NOTIFICATIONS, tr("Notifications"),tr("Notification options"), MNI_NOTIFICATIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

IOptionsWidget *Notifications::optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent)
{
	if (FOptionsManager && ANodeId == OPN_NOTIFICATIONS)
	{
		AOrder = OWO_NOTIFICATIONS;
		IOptionsContainer *container = FOptionsManager->optionsContainer(AParent);
		container->instance()->setLayout(new QVBoxLayout);
		container->instance()->layout()->setMargin(0);

		IOptionsWidget *widget = new OptionsWidget(this, container->instance());
		container->instance()->layout()->addWidget(widget->instance());
		container->registerChild(widget);

		foreach(QString id, FNotificators.keys())
		{
			Notificator notificator = FNotificators.value(id);
			widget = new NotifyKindsWidget(this,id,notificator.title,notificator.kindMask,container->instance());
			container->instance()->layout()->addWidget(widget->instance());
			container->registerChild(widget);
		}

		return container;
	}
	return NULL;
}

QList<int> Notifications::notifications() const
{
	return FNotifyRecords.keys();
}

INotification Notifications::notificationById(int ANotifyId) const
{
	return FNotifyRecords.value(ANotifyId).notification;
}

int Notifications::appendNotification(const INotification &ANotification)
{
	NotifyRecord record;
	int notifyId = ++FNotifyId;
	record.notification = ANotification;
	emit notificationAppend(notifyId, record.notification);

	bool isDND = FStatusChanger ? FStatusChanger->statusItemShow(STATUS_MAIN_ID) == IPresence::DoNotDisturb : false;

	QIcon icon;
	QString iconKey = record.notification.data.value(NDR_ICON_KEY).toString();
	QString iconStorage = record.notification.data.value(NDR_ICON_STORAGE).toString();
	if (!iconKey.isEmpty() && !iconStorage.isEmpty())
		icon = IconStorage::staticStorage(iconStorage)->getIcon(iconKey);
	else
		icon = qvariant_cast<QIcon>(record.notification.data.value(NDR_ICON));


	if (FRostersModel && FRostersViewPlugin && Options::node(OPV_NOTIFICATIONS_ROSTERICON).value().toBool() &&
	    (record.notification.kinds & INotification::RosterIcon)>0)
	{
		Jid streamJid = record.notification.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = record.notification.data.value(NDR_CONTACT_JID).toString();
		QString toolTip = record.notification.data.value(NDR_ROSTER_TOOLTIP).toString();
		int order = record.notification.data.value(NDR_ROSTER_NOTIFY_ORDER).toInt();
		int flags = IRostersView::LabelBlink|IRostersView::LabelVisible;
		flags = flags | (Options::node(OPV_NOTIFICATIONS_EXPANDGROUP).value().toBool() ? IRostersView::LabelExpandParents : 0);
		QList<IRosterIndex *> indexes = FRostersModel->getContactIndexList(streamJid,contactJid,true);
		record.rosterId = FRostersViewPlugin->rostersView()->appendNotify(indexes,order,icon,toolTip,flags);
	}

	if (Options::node(OPV_NOTIFICATIONS_POPUPWINDOW).value().toBool() && (record.notification.kinds & INotification::PopupWindow)>0)
	{
		record.widget = new NotifyWidget(record.notification);
		connect(record.widget,SIGNAL(notifyActivated()),SLOT(onWindowNotifyActivated()));
		connect(record.widget,SIGNAL(notifyRemoved()),SLOT(onWindowNotifyRemoved()));
		connect(record.widget,SIGNAL(windowDestroyed()),SLOT(onWindowNotifyDestroyed()));
		record.widget->appear();
	}

	if (FMessageWidgets && FMessageProcessor && Options::node(OPV_NOTIFICATIONS_CHATWINDOW).value().toBool() && (record.notification.kinds & INotification::ChatWindow)>0)
	{
		Jid streamJid = record.notification.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = record.notification.data.value(NDR_CONTACT_JID).toString();
		if (FMessageProcessor->createWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_ADD_TAB))
		{
			IChatWindow *window = FMessageWidgets->findChatWindow(streamJid,contactJid);
			if (window && window->tabPageNotifier()!=NULL)
			{
				ITabPageNotify notify;
				notify.priority = ANotification.data.value(NDR_TABPAGE_PRIORITY).toInt();
				notify.blink = ANotification.data.value(NDR_TABPAGE_ICONBLINK).toBool();
				notify.icon = icon;
				notify.iconKey = iconKey;
				notify.iconStorage = iconStorage;
				notify.toolTip = ANotification.data.value(NDR_TABPAGE_TOOLTIP).toString();
				notify.styleKey = ANotification.data.value(NDR_TABPAGE_STYLEKEY).toString();
				record.tabPageId = window->tabPageNotifier()->insertNotify(notify);
				record.tabPageNotifier = window->tabPageNotifier()->instance();
			}
		}
	}

	if (FTrayManager)
	{
		QString toolTip = record.notification.data.value(NDR_TRAY_TOOLTIP).toString();

		if (Options::node(OPV_NOTIFICATIONS_TRAYICON).value().toBool() && (record.notification.kinds & INotification::TrayIcon)>0)
		{
			ITrayNotify notify;
			notify.blink = true;
			notify.icon = icon;
			notify.iconKey = iconKey;
			notify.iconStorage = iconStorage;
			notify.toolTip = toolTip;
			record.trayId = FTrayManager->appendNotify(notify);
		}

		if (!toolTip.isEmpty() && Options::node(OPV_NOTIFICATIONS_TRAYACTION).value().toBool() &&
		    (record.notification.kinds & INotification::TrayAction)>0)
		{
			record.action = new Action(FNotifyMenu);
			if (!iconKey.isEmpty())
				record.action->setIcon(RSR_STORAGE_MENUICONS,iconKey);
			else
				record.action->setIcon(icon);
			record.action->setText(toolTip);
			record.action->setData(ADR_NOTIFYID,notifyId);
			connect(record.action,SIGNAL(triggered(bool)),SLOT(onActionNotifyActivated(bool)));
			FNotifyMenu->addAction(record.action);
		}
	}

	if (QSound::isAvailable() && !(isDND && Options::node(OPV_NOTIFICATIONS_NOSOUNDIFDND).value().toBool()) &&
	    Options::node(OPV_NOTIFICATIONS_SOUND).value().toBool() && (record.notification.kinds & INotification::PlaySound)>0)
	{
		QString soundName = record.notification.data.value(NDR_SOUND_FILE).toString();
		QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(soundName);
		if (!soundFile.isEmpty() && (FSound==NULL || FSound->isFinished()))
		{
			delete FSound;
			FSound = new QSound(soundFile);
			FSound->play();
		}
	}

	if (Options::node(OPV_NOTIFICATIONS_AUTOACTIVATE).value().toBool() && (record.notification.kinds & INotification::AutoActivate)>0)
	{
		FDelayedActivations.append(notifyId);
		QTimer::singleShot(0,this,SLOT(onActivateDelayedActivations()));
	}

	if (FNotifyRecords.isEmpty())
	{
		FActivateAll->setVisible(true);
		FRemoveAll->setVisible(true);
	}
	FNotifyMenu->menuAction()->setVisible(!FNotifyMenu->isEmpty());

	FNotifyRecords.insert(notifyId,record);
	emit notificationAppended(notifyId, record.notification);
	return notifyId;
}

void Notifications::activateNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		emit notificationActivated(ANotifyId);
	}
}

void Notifications::removeNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		NotifyRecord record = FNotifyRecords.take(ANotifyId);
		if (FRostersViewPlugin && record.rosterId!=0)
		{
			FRostersViewPlugin->rostersView()->removeNotify(record.rosterId);
		}
		if (!record.tabPageNotifier.isNull())
		{
			ITabPageNotifier *notifier =  qobject_cast<ITabPageNotifier *>(record.tabPageNotifier);
			if (notifier)
				notifier->removeNotify(record.tabPageId);
		}
		if (FTrayManager && record.trayId!=0)
		{
			FTrayManager->removeNotify(record.trayId);
		}
		if (record.widget != NULL)
		{
			record.widget->deleteLater();
		}
		if (record.action != NULL)
		{
			FNotifyMenu->removeAction(record.action);
			delete record.action;
		}
		if (FNotifyRecords.isEmpty())
		{
			FActivateAll->setVisible(false);
			FRemoveAll->setVisible(false);
		}
		FNotifyMenu->menuAction()->setVisible(!FNotifyMenu->isEmpty());
		emit notificationRemoved(ANotifyId);
	}
}

void Notifications::insertNotificator(const QString &AId, const QString &ATitle, uchar AKindMask, uchar ADefault)
{
	if (!FNotificators.contains(AId))
	{
		Notificator notificator;
		notificator.title = ATitle;
		notificator.kinds = 0xFF;
		notificator.defaults = ADefault;
		notificator.kindMask = AKindMask;
		FNotificators.insert(AId,notificator);
	}
}

uchar Notifications::notificatorKinds(const QString &AId) const
{
	if (FNotificators.contains(AId))
	{
		Notificator &notificator = FNotificators[AId];
		if (notificator.kinds == 0xFF)
		{
			if (Options::node(OPV_NOTIFICATIONS_ROOT).hasValue("notificator",AId))
				notificator.kinds = Options::node(OPV_NOTIFICATIONS_NOTIFICATOR_ITEM,AId).value().toInt() & notificator.kindMask;
			else
				notificator.kinds = notificator.defaults;
		}
		return notificator.kinds;
	}
	return 0xFF;
}

void Notifications::setNotificatorKinds(const QString &AId, uchar AKinds)
{
	if (FNotificators.contains(AId))
	{
		Notificator &notificator = FNotificators[AId];
		notificator.kinds = AKinds & notificator.kindMask;
		Options::node(OPV_NOTIFICATIONS_NOTIFICATOR_ITEM,AId).setValue(notificator.kinds);
	}
}

void Notifications::removeNotificator(const QString &AId)
{
	FNotificators.remove(AId);
	Options::node(OPV_NOTIFICATIONS_ROOT).removeChilds("notificator",AId);
}

QImage Notifications::contactAvatar(const Jid &AContactJid) const
{
	return FAvatars!=NULL ? FAvatars->avatarImage(AContactJid) : QImage();
}

QIcon Notifications::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FStatusIcons!=NULL ? FStatusIcons->iconByJid(AStreamJid,AContactJid) : QIcon();
}

QString Notifications::contactName(const Jid &AStreamJId, const Jid &AContactJid) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJId) : NULL;
	QString name = roster!=NULL ? roster->rosterItem(AContactJid).name : AContactJid.node();
	if (name.isEmpty())
		name = AContactJid.bare();
	return name;
}

int Notifications::notifyIdByRosterId(int ARosterId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().rosterId == ARosterId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByTrayId(int ATrayId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().trayId == ATrayId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByWidget(NotifyWidget *AWidget) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().widget == AWidget)
			return it.key();
	return -1;
}

void Notifications::onActivateDelayedActivations()
{
	foreach(int notifyId, FDelayedActivations)
		activateNotification(notifyId);
	FDelayedActivations.clear();
}

void Notifications::onSoundOnOffActionTriggered(bool)
{
	OptionsNode node = Options::node(OPV_NOTIFICATIONS_SOUND);
	node.setValue(!node.value().toBool());
}

void Notifications::onTrayActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		foreach(int notifyId, FNotifyRecords.keys())
		{
			if (action == FActivateAll)
				activateNotification(notifyId);
			else if (action == FRemoveAll)
				removeNotification(notifyId);
		}
	}
}

void Notifications::onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId)
{
	Q_UNUSED(AIndex);
	activateNotification(notifyIdByRosterId(ANotifyId));
}

void Notifications::onRosterNotifyRemoved(IRosterIndex *AIndex, int ANotifyId)
{
	Q_UNUSED(AIndex);
	removeNotification(notifyIdByRosterId(ANotifyId));
}

void Notifications::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (AReason == QSystemTrayIcon::Trigger)
	{
		activateNotification(notifyIdByTrayId(ANotifyId));
	}
}

void Notifications::onTrayNotifyRemoved(int ANotifyId)
{
	removeNotification(notifyIdByTrayId(ANotifyId));
}

void Notifications::onWindowNotifyActivated()
{
	activateNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyRemoved()
{
	removeNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyDestroyed()
{
	int notifyId = notifyIdByWidget(qobject_cast<NotifyWidget*>(sender()));
	if (FNotifyRecords.contains(notifyId))
		FNotifyRecords[notifyId].widget = NULL;
}

void Notifications::onActionNotifyActivated(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int notifyId = action->data(ADR_NOTIFYID).toInt();
		activateNotification(notifyId);
	}
}

void Notifications::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_NOTIFICATIONS_SOUND));
}

void Notifications::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_NOTIFICATIONS_SOUND)
	{
		FSoundOnOff->setIcon(RSR_STORAGE_MENUICONS, ANode.value().toBool() ? MNI_NOTIFICATIONS_SOUND_ON : MNI_NOTIFICATIONS_SOUND_OFF);
	}
}

Q_EXPORT_PLUGIN2(plg_notifications, Notifications)
