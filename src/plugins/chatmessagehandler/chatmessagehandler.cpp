#include "chatmessagehandler.h"

#define HISTORY_TIME_PAST         5
#define HISTORY_MESSAGES_COUNT    5

#define DESTROYWINDOW_TIMEOUT     30*60*1000
#define CONSECUTIVE_TIMEOUT       2*60

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_TAB_PAGE_ID           Action::DR_Parametr2

#define NOTIFICATOR_ID            "ChatMessages"

#define URL_SCHEME_ACTION         "action"
#define URL_PATH_HISTORY          "history"
#define URL_PATH_CONTENT          "content"

QDataStream &operator<<(QDataStream &AStream, const TabPageInfo &AInfo)
{
	AStream << AInfo.streamJid;
	AStream << AInfo.contactJid;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, TabPageInfo &AInfo)
{
	AStream >> AInfo.streamJid;
	AStream >> AInfo.contactJid;
	AInfo.page = NULL;
	return AStream;
}

ChatMessageHandler::ChatMessageHandler()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FPresencePlugin = NULL;
	FRamblerHistory = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FAvatars = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FXmppUriQueries = NULL;
	FNotifications = NULL;
}

ChatMessageHandler::~ChatMessageHandler()
{

}

void ChatMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Chat Messages");
	APluginInfo->description = tr("Allows to exchange chat messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}


bool ChatMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRamblerHistory").value(0,NULL);
	if (plugin)
	{
		FRamblerHistory = qobject_cast<IRamblerHistory *>(plugin->instance());
		if (FRamblerHistory)
		{
			connect(FRamblerHistory->instance(),SIGNAL(serverMessagesLoaded(const QString &, const IRamblerHistoryMessages &)),
				SLOT(onRamblerHistoryMessagesLoaded(const QString &, const IRamblerHistoryMessages &)));
			connect(FRamblerHistory->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
				SLOT(onRamblerHistoryRequestFailed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, uchar)),
				SLOT(onNotificationTest(const QString &, uchar)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
			connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool ChatMessageHandler::initObjects()
{
	if (FMessageWidgets)
	{
		FMessageWidgets->insertTabPageHandler(this);
	}
	if (FRostersView)
	{
		FRostersView->insertClickHooker(RCHO_CHATMESSAGEHANDLER,this);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(this,MHO_CHATMESSAGEHANDLER);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	if (FNotifications)
	{
		uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::ChatWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound|INotification::AutoActivate|INotification::TestNotify;
		uchar kindDefs = INotification::RosterIcon|INotification::PopupWindow|INotification::ChatWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
		FNotifications->insertNotificator(NOTIFICATOR_ID,OWO_NOTIFICATIONS_CHAT_MESSAGES,tr("New messages"),kindMask,kindDefs);
	}
	return true;
}

bool ChatMessageHandler::tabPageAvail(const QString &ATabPageId) const
{
	if (FTabPages.contains(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		return pageInfo.page!=NULL || findPresence(pageInfo.streamJid)!=NULL;
	}
	return false;
}

ITabPage *ChatMessageHandler::tabPageFind(const QString &ATabPageId) const
{
	return FTabPages.contains(ATabPageId) ? FTabPages.value(ATabPageId).page : NULL;
}

ITabPage *ChatMessageHandler::tabPageCreate(const QString &ATabPageId)
{
	ITabPage *page = tabPageFind(ATabPageId);
	if (page==NULL && tabPageAvail(ATabPageId))
	{
		TabPageInfo &pageInfo = FTabPages[ATabPageId];
		IPresence *presence = findPresence(pageInfo.streamJid);
		if (presence)
		{
			IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
			if (pitem.isValid)
				page = getWindow(presence->streamJid(), pitem.itemJid);
			else
				page = getWindow(presence->streamJid(), pageInfo.contactJid);
			pageInfo.page = page;
		}
	}
	return page;
}

Action *ChatMessageHandler::tabPageAction(const QString &ATabPageId, QObject *AParent)
{
	if (tabPageAvail(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IPresence *presence = findPresence(pageInfo.streamJid);
		if (presence && presence->isOpen())
		{
			ITabPage *page = tabPageFind(ATabPageId);
			Action *action = new Action(AParent);
			if (page)
			{
				if (page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
				{
					ITabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
					if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
						action->setIcon(notify.iconStorage, notify.iconKey);
					else
						action->setIcon(notify.icon);
				}
				else
				{
					action->setIcon(page->instance()->windowIcon());
				}
			}
			else
			{
				IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
				if (pitem.isValid)
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pitem.itemJid) : QIcon());
				else
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pageInfo.contactJid) : QIcon());
			}
			action->setData(ADR_TAB_PAGE_ID, ATabPageId);
			action->setText(FNotifications!=NULL ? FNotifications->contactName(presence->streamJid(),pageInfo.contactJid) : pageInfo.contactJid.bare());
			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenTabPageAction(bool)));
			return action;
		}
	}
	return NULL;
}

bool ChatMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type == "chat")
		{
			IChatWindow *window = getWindow(AStreamJid, AContactJid);
			window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
			window->showTabPage();
			return true;
		}
	}
	return false;
}

bool ChatMessageHandler::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	Q_UNUSED(AOrder);
	if (AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_MY_RESOURCE)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_JID).toString();
		return createWindow(MHO_CHATMESSAGEHANDLER,streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
	return false;
}

bool ChatMessageHandler::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	if (/*AMessage.type()==Message::Chat && */!AMessage.body().isEmpty())
		return true;
	return false;
}

bool ChatMessageHandler::showMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	return createWindow(MHO_CHATMESSAGEHANDLER,message.to(),message.from(),Message::Chat,IMessageHandler::SM_SHOW);
}

bool ChatMessageHandler::receiveMessage(int AMessageId)
{
	bool notify = false;
	Message message = FMessageProcessor->messageById(AMessageId);
	IChatWindow *window = getWindow(message.to(),message.from());
	if (window)
	{
		StyleExtension extension;
		WindowStatus &wstatus = FWindowStatus[window];
		if (!window->isActive())
		{
			notify = true;
			extension.extensions = IMessageContentOptions::Unread;
			wstatus.notified.append(AMessageId);
			updateWindow(window);
		}

		QUuid contentId = showStyledMessage(window,message,extension);
		if (!contentId.isNull() && notify)
		{
			message.setData(MDR_STYLE_CONTENT_ID,contentId.toString());
			wstatus.unread.append(message);
		}
	}
	return notify;
}

INotification ChatMessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
	IChatWindow *window = getWindow(AMessage.to(),AMessage.from());
	QString name = ANotifications->contactName(AMessage.to(),AMessage.from());
	QString messages = tr("%n message(s)","",FWindowStatus.value(window).notified.count());

	INotification notify;
	notify.kinds = ANotifications->notificatorKinds(NOTIFICATOR_ID);
	notify.data.insert(NDR_STREAM_JID,AMessage.to());
	notify.data.insert(NDR_CONTACT_JID,AMessage.from());
	notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
	notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
	notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
	notify.data.insert(NDR_ROSTER_TOOLTIP,messages);
	notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(messages));
	notify.data.insert(NDR_TABPAGE_PRIORITY, TPNP_NEW_MESSAGE);
	notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
	notify.data.insert(NDR_TABPAGE_TOOLTIP, messages);
	notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_CHAT_MHANDLER_TABBARITEM_NEWMESSAGE);
	notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.from()));
	notify.data.insert(NDR_POPUP_CAPTION,tr("Message received"));
	notify.data.insert(NDR_POPUP_TITLE,name);
	notify.data.insert(NDR_POPUP_TEXT,Qt::escape(AMessage.body()));
	notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);
	notify.data.insert(NDR_TYPE, NT_CHATMESSAGE);

	return notify;
}

bool ChatMessageHandler::createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType == Message::Chat)
	{
		IChatWindow *window = getWindow(AStreamJid,AContactJid);
		if (window)
		{
			if (AShowMode==IMessageHandler::SM_SHOW)
				window->showTabPage();
			else if (!window->instance()->isVisible() && AShowMode==IMessageHandler::SM_ADD_TAB)
				FMessageWidgets->assignTabWindowPage(window);
			return true;
		}
	}
	return false;
}

IChatWindow *ChatMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = NULL;
	if (AStreamJid.isValid() && AContactJid.isValid())
	{
		if (AContactJid.resource().isEmpty())
		{
			foreach(IChatWindow *chatWindow, FWindows)
			{
				if (chatWindow->streamJid()==AStreamJid && chatWindow->contactJid().pBare()==AContactJid.pBare())
				{
					window = chatWindow;
					break;
				}
			}
		}
		if (window == NULL)
		{
			window = findWindow(AStreamJid,AContactJid);
		}
		if (window == NULL)
		{
			window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
			if (window)
			{
				window->infoWidget()->autoUpdateFields();
				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				WindowStatus &wstatus = FWindowStatus[window];
				wstatus.createTime = QDateTime::currentDateTime();

				connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
				connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
					SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
				connect(window->viewWidget()->instance(),SIGNAL(urlClicked(const QUrl	&)),SLOT(onUrlClicked(const QUrl	&)));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));

				FWindows.append(window);
				updateWindow(window);

				if (FRostersView && FRostersModel)
				{
					UserContextMenu *menu = new UserContextMenu(FRostersModel,FRostersView,window);
					if (FAvatars)
						FAvatars->insertAutoAvatar(menu->menuAction(),AContactJid,QSize(48,48));
					else
						menu->menuAction()->setIcon(RSR_STORAGE_MENUICONS, MNI_CHAT_MHANDLER_USER_MENU);
					QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
					button->setPopupMode(QToolButton::InstantPopup);
					button->setFixedSize(QSize(48,48));
				}

				setMessageStyle(window);

				TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
				pageInfo.page = window;
				emit tabPageCreated(window);

				requestHistoryMessages(window, HISTORY_MESSAGES_COUNT);

				window->instance()->installEventFilter(this);
			}
		}
	}
	return window;
}

IChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	foreach(IChatWindow *window,FWindows)
		if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
			return window;
	return NULL;
}

void ChatMessageHandler::updateWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (!FWindowStatus.value(AWindow).notified.isEmpty())
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
	else if (FStatusIcons)
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString name = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
	QString resource = AWindow->contactJid().resource();
	QString show = FStatusChanger!=NULL ? FStatusChanger->nameByShow(AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt()) : QString::null;
	QString title = (!resource.isEmpty() ? name+"/"+resource : name) + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);
	AWindow->updateWindow(icon,name,title);
}

void ChatMessageHandler::removeMessageNotifications(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.notified.isEmpty())
	{
		foreach(int messageId, wstatus.notified)
			FMessageProcessor->removeMessage(messageId);
		wstatus.notified.clear();
		updateWindow(AWindow);
	}
}

void ChatMessageHandler::replaceUnreadMessages(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.unread.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		foreach (Message message, wstatus.unread)
		{
			extension.contentId = message.data(MDR_STYLE_CONTENT_ID).toString();
			showStyledMessage(AWindow, message, extension);
		}
		wstatus.unread.clear();
	}
}

void ChatMessageHandler::sendOfflineMessages(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.offline.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		while (!wstatus.offline.isEmpty())
		{
			Message message = wstatus.offline.at(0);
			message.setTo(AWindow->contactJid().eFull());
			if (FMessageProcessor->sendMessage(AWindow->streamJid(),message))
			{
				extension.contentId = message.data(MDR_STYLE_CONTENT_ID).toString();
				showStyledMessage(AWindow, message, extension);
				wstatus.offline.removeAt(0);
			}
			else
			{
				break;
			}
		}
	}
}

void ChatMessageHandler::removeOfflineMessage(IChatWindow *AWindow, const QUuid &AContentId)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	for (int index=0; index<wstatus.offline.count(); index++)
	{
		if (wstatus.offline.at(index).data(MDR_STYLE_CONTENT_ID).toString() == AContentId.toString())
		{
			IMessageContentOptions options;
			options.action = IMessageContentOptions::Remove;
			options.contentId = AContentId;
			AWindow->viewWidget()->changeContentHtml(QString::null,options);
			wstatus.offline.removeAt(index);
			break;
		}
	}
}

void ChatMessageHandler::requestHistoryMessages(IChatWindow *AWindow, int ACount)
{
	if (FRamblerHistory && FRamblerHistory->isSupported(AWindow->streamJid()))
	{
		IRamblerHistoryRetrieve retrieve;
		retrieve.with = AWindow->contactJid();
		retrieve.beforeId = FWindowStatus.value(AWindow).historyId;
		retrieve.beforeTime = FWindowStatus.value(AWindow).historyTime;
		retrieve.count = ACount;
		QString id = FRamblerHistory->loadServerMessages(AWindow->streamJid(),retrieve);
		if (!id.isEmpty())
		{
			FHistoryRequests.insert(id,AWindow);
			showHistoryLinks(AWindow,HLS_WAITING);
		}
	}
	else
	{
		showHistoryLinks(AWindow,HLS_FINISHED);
	}
}

IPresence *ChatMessageHandler::findPresence(const Jid &AStreamJid) const
{
	IPresence *precsence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
	for (int i=0; precsence==NULL && i<FPrecences.count(); i++)
		if (AStreamJid && FPrecences.at(i)->streamJid())
			precsence = FPrecences.at(i);
	return precsence;
}

IPresenceItem ChatMessageHandler::findPresenceItem(IPresence *APresence, const Jid &AContactJid) const
{
	IPresenceItem pitem = APresence!=NULL ? APresence->presenceItem(AContactJid) : IPresenceItem();
	QList<IPresenceItem> pitems = APresence!=NULL ? APresence->presenceItems() : QList<IPresenceItem>();
	for (int i=0; !pitem.isValid && i<pitems.count(); i++)
		if (AContactJid && pitems.at(i).itemJid)
			pitem = pitems.at(i);
	return pitem;
}

void ChatMessageHandler::showHistoryLinks(IChatWindow *AWindow, HisloryLoadState AState)
{
	static QString urlMask = QString("<a href='%1'>%2</a>");
	if (FRamblerHistory)
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.time = QDateTime::fromTime_t(0);
		options.timeFormat = " ";
		options.noScroll = true;
		
		QString message;

		QUrl showWindowUrl;
		showWindowUrl.setScheme(URL_SCHEME_ACTION);
		showWindowUrl.setPath(URL_PATH_HISTORY);
		showWindowUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("window")));
		message += (!message.isEmpty() ? QString("<br>") : QString::null) + urlMask.arg(showWindowUrl.toString()).arg(tr("Chat history"));

		if (AState == HLS_READY)
		{
			QUrl showMesagesUrl;
			showMesagesUrl.setScheme(URL_SCHEME_ACTION);
			showMesagesUrl.setPath(URL_PATH_HISTORY);
			showMesagesUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("messages")));
			message += (!message.isEmpty() ? QString("<br>") : QString::null) + urlMask.arg(showMesagesUrl.toString()).arg(tr("Show previous messages"));
		}
		else if (AState == HLS_WAITING)
		{
			message += (!message.isEmpty() ? QString("<br>") : QString::null) + tr("Loading messages from server...");
		}

		WindowStatus &wstatus = FWindowStatus[AWindow];
		if (!wstatus.historyLinkId.isNull())
		{
			options.action = IMessageContentOptions::Replace;
			options.contentId = wstatus.historyLinkId;
		}
		wstatus.historyLinkId =  AWindow->viewWidget()->changeContentHtml(message,options);
	}
}

void ChatMessageHandler::setMessageStyle(IChatWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
	IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
	AWindow->viewWidget()->setMessageStyle(style,soptions);

	WindowStatus &wstatus = FWindowStatus[AWindow];
	wstatus.separators.clear();
	wstatus.unread.clear();
	wstatus.offline.clear();
	wstatus.historyId = QString::null;
	wstatus.historyTime = QDateTime();
	wstatus.historyLinkId = QUuid();

	showHistoryLinks(AWindow, HLS_READY);
}

void ChatMessageHandler::fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
	if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		AOptions.senderId = AWindow->contactJid().full();
		AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid(),AWindow->contactJid()));
		AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->contactJid());
		AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid(),AWindow->contactJid());
		AOptions.senderColor = "blue";
	}
	else
	{
		AOptions.senderId = AWindow->streamJid().full();
		if (AWindow->streamJid() && AWindow->contactJid())
			AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().node());
		else
			AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid()));
		AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->streamJid());
		AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid());
		AOptions.senderColor = "red";
	}
}

QUuid ChatMessageHandler::showDateSeparator(IChatWindow *AWindow, const QDate &ADate)
{
	static const QList<QString> mnames = QList<QString>() << tr("January") << tr("February") <<  tr("March") <<  tr("April")
		<< tr("May") << tr("June") << tr("July") << tr("August") << tr("September") << tr("October") << tr("November") << tr("December");
	static const QList<QString> dnames = QList<QString>() << tr("Monday") << tr("Tuesday") <<  tr("Wednesday") <<  tr("Thursday")
		<< tr("Friday") << tr("Saturday") << tr("Sunday");

	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.separators.contains(ADate))
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.direction = IMessageContentOptions::DirectionIn;
		options.type = IMessageContentOptions::DateSeparator;
		options.time.setDate(ADate);
		options.time.setTime(QTime(0,0));
		options.timeFormat = " ";
		options.noScroll = true;

		QString message;
		QDate currentDate = QDate::currentDate();
		if (ADate == currentDate)
			message = ADate.toString(tr("%1, %2 dd")).arg(tr("Today")).arg(mnames.value(ADate.month()-1));
		else if (ADate.year() == currentDate.year())
			message = ADate.toString(tr("%1, %2 dd")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		else
			message = ADate.toString(tr("%1, %2 dd, yyyy")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		wstatus.separators.append(ADate);
		return AWindow->viewWidget()->changeContentText(message,options);
	}
	return QUuid();
}

QUuid ChatMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Status;
	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	fillContentOptions(AWindow,options);
	return AWindow->viewWidget()->changeContentText(AMessage,options);
}

QUuid ChatMessageHandler::showStyledMessage(IChatWindow *AWindow, const Message &AMessage, const StyleExtension &AExtension)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Message;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid() != AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionOut;

	if (options.time.secsTo(FWindowStatus.value(AWindow).createTime) > HISTORY_TIME_PAST)
	{
		options.noScroll = true;
		options.type |= IMessageContentOptions::History;
	}

	options.action = AExtension.action;
	options.extensions = AExtension.extensions;
	options.contentId = AExtension.contentId;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,AMessage.dateTime().date());
	return AWindow->viewWidget()->changeContentMessage(AMessage,options);
}

bool ChatMessageHandler::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type()==QEvent::WindowDeactivate || AEvent->type()==QEvent::Hide)
	{
		IChatWindow *window = qobject_cast<IChatWindow *>(AObject);
		if (window)
			replaceUnreadMessages(window);
	}
	return QObject::eventFilter(AObject,AEvent);
}

void ChatMessageHandler::onMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		Message message;
		message.setTo(window->contactJid().eFull()).setType(Message::Chat);
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			StyleExtension extension;
			if (!FMessageProcessor->sendMessage(window->streamJid(),message))
				extension.extensions = IMessageContentOptions::Offline;

			QUuid contentId = showStyledMessage(window, message, extension);
			if (!contentId.isNull() && extension.extensions==IMessageContentOptions::Offline)
			{
				message.setData(MDR_STYLE_CONTENT_ID, contentId.toString());
				FWindowStatus[window].offline.append(message);
			}

			replaceUnreadMessages(window);
			window->editWidget()->clearEditor();
		}
	}
}

void ChatMessageHandler::onUrlClicked(const QUrl &AUrl)
{
	if (AUrl.scheme() == URL_SCHEME_ACTION)
	{
		IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			if (AUrl.path() == URL_PATH_HISTORY)
			{
				QString keyValue = AUrl.queryItemValue("show");
				if (keyValue == "messages")
				{
					requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
				}
				else if (keyValue == "window")
				{

				}
			}
			else if (AUrl.path() == URL_PATH_CONTENT)
			{
				QUuid contentId = AUrl.queryItemValue("remove");
				if (!contentId.isNull())
				{
					removeOfflineMessage(window,contentId);
				}
			}
		}
	}
}

void ChatMessageHandler::onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
	if ((AField & (IInfoWidget::ContactStatus|IInfoWidget::ContactName))>0)
	{
		IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			Jid streamJid = window->streamJid();
			Jid contactJid = window->contactJid();
			if (AField == IInfoWidget::ContactStatus && Options::node(OPV_MESSAGES_SHOWSTATUS).value().toBool())
			{
				QString status = AValue.toString();
				QString show = FStatusChanger ? FStatusChanger->nameByShow(widget->field(IInfoWidget::ContactShow).toInt()) : QString::null;
				WindowStatus &wstatus = FWindowStatus[window];
				if (wstatus.lastStatusShow != status+show)
				{
					wstatus.lastStatusShow = status+show;
					QString message = tr("%1 changed status to [%2] %3").arg(widget->field(IInfoWidget::ContactName).toString()).arg(show).arg(status);
					showStyledStatus(window,message);
				}
			}
			updateWindow(window);
		}
	}
}

void ChatMessageHandler::onWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
		pageInfo.streamJid = window->streamJid();
		pageInfo.contactJid = window->contactJid();
		pageInfo.page = window;

		if (FWindowTimers.contains(window))
			delete FWindowTimers.take(window);
		removeMessageNotifications(window);
	}
}

void ChatMessageHandler::onWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		if (!FWindowTimers.contains(window))
		{
			QTimer *timer = new QTimer;
			timer->setSingleShot(true);
			connect(timer,SIGNAL(timeout()),window->instance(),SLOT(deleteLater()));
			FWindowTimers.insert(window,timer);
		}
		FWindowTimers[window]->start(DESTROYWINDOW_TIMEOUT);
	}
}

void ChatMessageHandler::onWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		if (FTabPages.contains(window->tabPageId()))
			FTabPages[window->tabPageId()].page = NULL;
		if (FWindowTimers.contains(window))
			delete FWindowTimers.take(window);
		removeMessageNotifications(window);
		FWindows.removeAll(window);
		FWindowStatus.remove(window);
		emit tabPageDestroyed(window);
	}
}

void ChatMessageHandler::onStatusIconsChanged()
{
	foreach(IChatWindow *window, FWindows)
		updateWindow(window);
}

void ChatMessageHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		createWindow(MHO_CHATMESSAGEHANDLER,streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
}

void ChatMessageHandler::onOpenTabPageAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		ITabPage *page = tabPageCreate(action->data(ADR_TAB_PAGE_ID).toString());
		if (page)
			page->showTabPage();
	}
}

void ChatMessageHandler::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	static QList<int> chatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(streamJid) : NULL;
	if (presence && presence->isOpen())
	{
		Jid contactJid = AIndex->data(RDR_JID).toString();
		if (chatActionTypes.contains(AIndex->type()))
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Chat"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,streamJid.full());
			action->setData(ADR_CONTACT_JID,contactJid.full());
			AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void ChatMessageHandler::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolTips);
	static QList<int> chatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(streamJid) : NULL;
	if (presence && presence->isOpen())
	{
		Jid contactJid = AIndex->data(RDR_JID).toString();
		if (chatActionTypes.contains(AIndex->type()) && (ALabelId == RLID_DISPLAY))
		{
			Action *action = new Action(AToolBarChanger->toolBar());
			action->setText(tr("Chat"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,streamJid.full());
			action->setData(ADR_CONTACT_JID,contactJid.full());
			AToolBarChanger->insertAction(action);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void ChatMessageHandler::onPresenceAdded(IPresence *APresence)
{
	FPrecences.append(APresence);
}

void ChatMessageHandler::onPresenceOpened(IPresence *APresence)
{
	foreach(IChatWindow *window, FWindows)
		if (window->streamJid() == APresence->streamJid())
			sendOfflineMessages(window);
}

void ChatMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	Jid streamJid = APresence->streamJid();
	Jid contactJid = AItem.itemJid;
	IChatWindow *window = findWindow(streamJid,contactJid);
	if (!window && !contactJid.resource().isEmpty())
	{
		IChatWindow *bareWindow = findWindow(streamJid,contactJid.bare());
		if (bareWindow)
			bareWindow->setContactJid(contactJid);
	}
	if (window && !contactJid.resource().isEmpty())
	{
		IChatWindow *bareWindow = findWindow(streamJid,contactJid.bare());
		if (bareWindow)
			bareWindow->instance()->deleteLater();
	}
}

void ChatMessageHandler::onPresenceRemoved( IPresence *APresence )
{
	FPrecences.removeAll(APresence);
}

void ChatMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IChatWindow *window, FWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setMessageStyle(window);
				requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
			}
		}
	}
}

void ChatMessageHandler::onNotificationTest(const QString &ANotificatorId, uchar AKinds)
{
	if (ANotificatorId == NOTIFICATOR_ID)
	{
		INotification notify;
		notify.kinds = AKinds;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contsctJid = "vasilisa@rambler/virtus";
			notify.data.insert(NDR_STREAM_JID,contsctJid.full());
			notify.data.insert(NDR_CONTACT_JID,contsctJid.full());
			notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
			notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
			notify.data.insert(NDR_POPUP_CAPTION,tr("Message received"));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_TEXT,tr("Hi! Come on www.rambler.ru :)"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contsctJid.full()));
		}
		if (AKinds & INotification::PlaySound)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);
		}
		if (!notify.data.isEmpty())
		{
			notify.data.insert(NDR_TYPE, NT_CHATMESSAGE);
			FNotifications->appendNotification(notify);
		}
	}
}

void ChatMessageHandler::onRamblerHistoryMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			for (int i=0; i<AMessages.messages.count(); i++)
			{
				Message message = AMessages.messages.at(i);
				showStyledMessage(window,message);
			}

			if (!AMessages.beforeId.isEmpty())
			{
				FWindowStatus[window].historyId = AMessages.beforeId;
				FWindowStatus[window].historyTime = AMessages.beforeTime;
			}

			if (AMessages.messages.count() < HISTORY_MESSAGES_COUNT)
				showHistoryLinks(window,HLS_FINISHED);
			else
				showHistoryLinks(window,HLS_READY);
		}
	}
}

void ChatMessageHandler::onRamblerHistoryRequestFailed(const QString &AId, const QString &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			IMessageContentOptions options;
			options.kind = IMessageContentOptions::Status;
			options.type |= IMessageContentOptions::Event;
			options.direction = IMessageContentOptions::DirectionIn;
			options.time = QDateTime::currentDateTime();
			window->viewWidget()->changeContentText(tr("Failed to load history messages from server: %1").arg(AError),options);
			showHistoryLinks(window,HLS_FINISHED);
		}
	}
}

void ChatMessageHandler::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.last-chat-tab-pages").toByteArray();
	QDataStream stream(data);
	stream >> FTabPages;
}

void ChatMessageHandler::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FTabPages;
	Options::setFileValue(data,"messages.last-chat-tab-pages");
}

Q_EXPORT_PLUGIN2(plg_chatmessagehandler, ChatMessageHandler)
