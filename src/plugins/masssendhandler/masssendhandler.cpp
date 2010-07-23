#include "masssendhandler.h"

#define NORMAL_NOTIFICATOR_ID     "MassSend"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_GROUP                 Action::DR_Parametr3

MassSendHandler::MassSendHandler()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FStatusIcons = NULL;
	FPresencePlugin = NULL;
	FRostersView = NULL;
	FXmppUriQueries = NULL;
}

MassSendHandler::~MassSendHandler()
{

}

void MassSendHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Mass Send");
	APluginInfo->description = tr("Allows to send messagesages to multiple users");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}

bool MassSendHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0, NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0, NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0, NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0, NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0, NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
				SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0, NULL);
	if (plugin)
	{
		INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
		if (notifications)
		{
			uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound|INotification::AutoActivate;
			uchar kindDefs = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
			notifications->insertNotificator(NORMAL_NOTIFICATOR_ID, 0, tr("Single Messages"), kindMask, kindDefs);
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0, NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0, NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0, NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin*>(plugin->instance());
		if (FMainWindowPlugin)
		{
			Action * action = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
			action->setText(tr("Mass send"));
			connect(action, SIGNAL(triggered()), SLOT(onMassSendAction()));
			FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action);
		}
	}
	plugin = APluginManager->pluginInterface("IAccountManager").value(0, NULL);
	if (plugin)
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

	return FMessageProcessor && FMessageWidgets && FMessageStyles;
}

bool MassSendHandler::initObjects()
{
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(this, MHO_NORMALMESSAGEHANDLER);
	}
//	if (FXmppUriQueries)
//	{
//		FXmppUriQueries->insertUriHandler(this,XUHO_DEFAULT);
//	}
	return true;
}

bool MassSendHandler::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	if (!AMessage.body().isEmpty() || !AMessage.subject().isEmpty())
		return true;
	return false;
}

bool MassSendHandler::showMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	Jid streamJid = message.to();
	Jid contactJid = message.from();
	createWindow(MHO_NORMALMESSAGEHANDLER,streamJid,contactJid,message.type(), 0);
	return true;
}

bool MassSendHandler::receiveMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	IMassSendDialog *window = findDialog(message.from());
	if (window)
	{
//		FActiveMessages.insertMulti(window,AMessageId);
		updateDialog(window);
	}
//	else
//		FActiveMessages.insertMulti(NULL,AMessageId);
	return true;
}

INotification MassSendHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
	IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	QIcon icon =  storage->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
	QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

	INotification notify;
	notify.kinds = ANotifications->notificatorKinds(NORMAL_NOTIFICATOR_ID);
//	notify.data.insert(NDR_ICON,icon);
//	notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
//	notify.data.insert(NDR_ROSTER_STREAM_JID,AMessage.to());
//	notify.data.insert(NDR_ROSTER_CONTACT_JID,AMessage.from());
//	notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
//	notify.data.insert(NDR_WINDOW_IMAGE,ANotifications->contactAvatar(AMessage.from()));
//	notify.data.insert(NDR_WINDOW_CAPTION, tr("Message received"));
//	notify.data.insert(NDR_WINDOW_TITLE,name);
//	notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
//	notify.data.insert(NDR_SOUND_FILE,SDF_NORMAL_MHANDLER_MESSAGE);

	return notify;
}

bool MassSendHandler::createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	IMassSendDialog *window = getDialog(AStreamJid);
	if (window)
	{
		showDialog(window);
		return true;
	}
	return false;
}

IMassSendDialog *MassSendHandler::getDialog(const Jid &AStreamJid)
{
	IMassSendDialog *window = NULL;
	if (AStreamJid.isValid())
	{
		window = FMessageWidgets->newMassSendDialog(AStreamJid);
		if (window)
		{
//			window->infoWidget()->autoUpdateFields();
			connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
//			connect(window->instance(),SIGNAL(showNextMessage()),SLOT(onShowNextMessage()));
//			connect(window->instance(),SIGNAL(replyMessage()),SLOT(onReplyMessage()));
//			connect(window->instance(),SIGNAL(forwardMessage()),SLOT(onForwardMessage()));
//			connect(window->instance(),SIGNAL(showChatWindow()),SLOT(onShowChatWindow()));
//			connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onWindowDestroyed()));
			FDialogs.append(window);
//			loadActiveMessages(window);
//			showNextMessage(window);
		}
		else
			window = findDialog(AStreamJid);
	}
	return window;
}

IMassSendDialog *MassSendHandler::findDialog(const Jid &AStreamJid)
{
	foreach(IMassSendDialog *window, FDialogs)
		if (window->streamJid() == AStreamJid)
			return window;
	return NULL;
}

void MassSendHandler::showDialog(IMassSendDialog *AWindow)
{
	//AWindow->showWindow();
}

/*void MassSendHandler::showNextMessage(IMassSendDialog *AWindow)
{
	if (FActiveMessages.contains(AWindow))
	{
		int messageId = FActiveMessages.value(AWindow);
		Message message = FMessageProcessor->messageById(messageId);
		showStyledMessage(AWindow,message);
		FLastMessages.insert(AWindow,message);
		FMessageProcessor->removeMessage(messageId);
		FActiveMessages.remove(AWindow,messageId);
	}
	updateWindow(AWindow);
}

void MassSendHandler::loadActiveMessages(IMassSendDialog *AWindow)
{
	QList<int> messagesId = FActiveMessages.values(NULL);
	foreach(int messageId, messagesId)
	{
		Message message = FMessageProcessor->messageById(messageId);
		if (AWindow->streamJid() == message.to())
		{
			FActiveMessages.insertMulti(AWindow,messageId);
			FActiveMessages.remove(NULL,messageId);
		}
	}
}*/

void MassSendHandler::updateDialog(IMassSendDialog *AWindow)
{
/*	QIcon icon;
	if (FActiveMessages.contains(AWindow))
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
	else if (FStatusIcons)
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString title = tr("Composing message");
	if (AWindow->mode() == IMessageWindow::ReadMode)
		title = tr("%1 - Message").arg(AWindow->infoWidget()->field(IInfoWidget::ContactName).toString());
	AWindow->updateWindow(icon,title,title);
	AWindow->setNextCount(FActiveMessages.count(AWindow));
	*/
}

void MassSendHandler::setMessageStyle(IMassSendDialog *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Normal);
	IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
	if (style != AWindow->viewWidget()->messageStyle())
		AWindow->viewWidget()->setMessageStyle(style,soptions);
	else if (AWindow->viewWidget()->messageStyle() != NULL)
		AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions);
}

void MassSendHandler::fillContentOptions(IMassSendDialog *AWindow, IMessageContentOptions &AOptions) const
{
//	AOptions.senderColor = "blue";
//	AOptions.senderId = AWindow->contactJid().full();
//	AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid(),AWindow->contactJid()));
//	AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->contactJid());
//	AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid(),AWindow->contactJid());
}

void MassSendHandler::showStyledMessage(IMassSendDialog *AWindow, const Message &AMessage)
{
	IMessageContentOptions options;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	options.noScroll = true;
	fillContentOptions(AWindow,options);

//	AWindow->setMode(IMessageWindow::ReadMode);
//	AWindow->setSubject(AMessage.subject());
//	AWindow->setThreadId(AMessage.threadId());

	setMessageStyle(AWindow);

	if (AMessage.type() == Message::Error)
	{
		ErrorHandler err(AMessage.stanza().element());
		QString html = tr("<b>The message with a error code %1 is received</b>").arg(err.code());
		html += "<p style='color:red;'>"+Qt::escape(err.message())+"</p>";
		html += "<hr>";
		options.kind = IMessageContentOptions::Message;
		AWindow->viewWidget()->appendHtml(html,options);
	}

	options.kind = IMessageContentOptions::Topic;
	AWindow->viewWidget()->appendText(tr("Subject: %1").arg(!AMessage.subject().isEmpty() ? AMessage.subject() : tr("<no subject>")),options);
	options.kind = IMessageContentOptions::Message;
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

void MassSendHandler::onMessageReady()
{
	IMassSendDialog *window = qobject_cast<IMassSendDialog*>(sender());
	if (window)
	{
		Message message;
		message.setType(Message::Chat);
//		message.setSubject(window->subject());
//		message.setThreadId(window->threadId());
		FMessageProcessor->textToMessage(message, window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			bool sended = false;
			QList<Jid> receiversList = window->receiversWidget()->receivers();
			foreach(Jid receiver, receiversList)
			{
				message.setTo(receiver.eFull());
				sended = FMessageProcessor->sendMessage(window->streamJid(),message) ? true : sended;
			}
			if (sended)
			{
//				if (FActiveMessages.contains(window))
//					showNextMessage(window);
//				else
//					window->closeWindow();
			}
		}
	}
}

//void MassSendHandler::onShowNextMessage()
//{
//	IMassSendDialog *window = qobject_cast<IMassSendDialog*>(sender());
//	if (window)
//	{
////		showNextMessage(window);
//		updateWindow(window);
//	}
//}

//void MassSendHandler::onReplyMessage()
//{
//	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
//	if (window)
//	{
//		window->setMode(IMessageWindow::WriteMode);
//		window->setSubject(tr("Re: %1").arg(window->subject()));
//		window->editWidget()->clearEditor();
//		window->editWidget()->instance()->setFocus();
//		updateWindow(window);
//	}
//}

//void MassSendHandler::onForwardMessage()
//{
//	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
//	if (FLastMessages.contains(window))
//	{
//		Message message = FLastMessages.value(window);
//		window->setMode(IMessageWindow::WriteMode);
//		window->setSubject(tr("Fw: %1").arg(message.subject()));
//		window->setThreadId(message.threadId());
//		FMessageProcessor->messageToText(window->editWidget()->document(),message);
//		window->receiversWidget()->clear();
//		window->setCurrentTabWidget(window->receiversWidget()->instance());
//		updateWindow(window);
//	}
//}

//void MassSendHandler::onShowChatWindow()
//{
//	//IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
//	//if (FMessageProcessor && window)
//	//	FMessageProcessor->openWindow(window->streamJid(),window->contactJid(),Message::Chat);
//}

void MassSendHandler::onWindowDestroyed()
{
	IMassSendDialog *window = qobject_cast<IMassSendDialog*>(sender());
	if (FDialogs.contains(window))
	{
//		QList<int> messagesId = FActiveMessages.values(window);
//		foreach(int messageId, messagesId)
//			FActiveMessages.insertMulti(NULL,messageId);
//		FActiveMessages.remove(window);
//		FLastMessages.remove(window);
		FDialogs.removeAt(FDialogs.indexOf(window));
	}
}

void MassSendHandler::onStatusIconsChanged()
{
	foreach(IMassSendDialog *window, FDialogs)
		updateDialog(window);
}

void MassSendHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		createWindow(MHO_NORMALMESSAGEHANDLER,streamJid,contactJid,Message::Normal, 0);

		QString group = action->data(ADR_GROUP).toString();
		if (!group.isEmpty())
		{
			IMessageWindow *window = FMessageWidgets->findMessageWindow(streamJid,contactJid);
			if (window)
				window->receiversWidget()->addReceiversGroup(group);
		}
	}
}

void MassSendHandler::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	static QList<int> messageActionTypes = QList<int>() << RIT_STREAM_ROOT << RIT_GROUP << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
	if (presence && presence->isOpen())
	{
		if (messageActionTypes.contains(AIndex->type()))
		{
			Jid contactJid = AIndex->data(RDR_JID).toString();
			Action *action = new Action(AMenu);
			action->setText(tr("Message"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMAL_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,streamJid.full());
			if (AIndex->type() == RIT_GROUP)
				action->setData(ADR_GROUP,AIndex->data(RDR_GROUP));
			else if (AIndex->type() != RIT_STREAM_ROOT)
				action->setData(ADR_CONTACT_JID,contactJid.full());
			AMenu->addAction(action,AG_RVCM_NORMALMESSAGEHANDLER,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void MassSendHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
	IMassSendDialog *messageWindow = findDialog(APresence->streamJid());
	if (messageWindow)
		updateDialog(messageWindow);
}

void MassSendHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AContext.isEmpty())
	{
//		foreach (IMassSendDialog *window, FDialogs)
		{
//			if (FLastMessages.value(window).type()==AMessageType && window->viewWidget() && window->viewWidget()->messageStyle())
//			{
//				window->viewWidget()->messageStyle()->changeOptions(window->viewWidget()->styleWidget(),AOptions,false);
//			}
		}
	}
}

void MassSendHandler::onMassSendAction()
{
	Jid streamJid;
	if (FAccountManager->accounts().first()->xmppStream())
		streamJid = FAccountManager->accounts().first()->xmppStream()->streamJid();
	FMassSendDialog = FMessageWidgets->newMassSendDialog(streamJid);
	FMassSendDialog->instance()->show();
}

Q_EXPORT_PLUGIN2(plg_masssendhandler, MassSendHandler)
