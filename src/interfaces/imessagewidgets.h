#ifndef IMESSAGEWIDGETS_H
#define IMESSAGEWIDGETS_H

#include <QUuid>
#include <QMultiMap>
#include <QTextBrowser>
#include <QTextDocument>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestyles.h>
#include <utils/jid.h>
#include <utils/menu.h>
#include <utils/action.h>
#include <utils/message.h>
#include <utils/menubarchanger.h>
#include <utils/toolbarchanger.h>
#include <utils/statusbarchanger.h>

#define MESSAGEWIDGETS_UUID "{89de35ee-bd44-49fc-8495-edd2cfebb685}"

class IInfoWidget
{
public:
	enum InfoField {
		AccountName         =1,
		ContactName         =2,
		ContactShow         =4,
		ContactStatus       =8,
		ContactAvatar       =16
	};
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual void autoUpdateFields() =0;
	virtual void autoUpdateField(InfoField AField) =0;
	virtual QVariant field(InfoField AField) const =0;
	virtual void setField(InfoField AField, const QVariant &AValue) =0;
	virtual int autoUpdatedFields() const =0;
	virtual bool isFiledAutoUpdated(IInfoWidget::InfoField AField) const =0;
	virtual void setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto) =0;
	virtual int visibleFields() const =0;
	virtual bool isFieldVisible(IInfoWidget::InfoField AField) const =0;
	virtual void setFieldVisible(IInfoWidget::InfoField AField, bool AVisible) =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue) =0;
};

class IViewWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QWidget *styleWidget() const =0;
	virtual IMessageStyle *messageStyle() const =0;
	virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions) =0;
	virtual void appendHtml(const QString &AHtml, const IMessageContentOptions &AOptions) =0;
	virtual void appendText(const QString &AText, const IMessageContentOptions &AOptions) =0;
	virtual void appendMessage(const Message &AMessage, const IMessageContentOptions &AOptions) =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void messageStyleChanged(IMessageStyle *ABefour, const IMessageStyleOptions &AOptions) =0;
	virtual void contentAppended(const QString &AMessage, const IMessageContentOptions &AOptions) =0;
	virtual void urlClicked(const QUrl &AUrl) const =0;
};

class IEditWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QTextEdit *textEdit() const =0;
	virtual QTextDocument *document() const =0;
	virtual void sendMessage() =0;
	virtual void clearEditor() =0;
	virtual bool autoResize() const =0;
	virtual void setAutoResize(bool AResize) =0;
	virtual int minimumLines() const =0;
	virtual void setMinimumLines(int ALines) =0;
	virtual QKeySequence sendKey() const =0;
	virtual void setSendKey(const QKeySequence &AKey) =0;
	virtual bool sendButtonVisible() const =0;
	virtual void setSendButtonVisible(bool AVisible) =0;
protected:
	virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook) =0;
	virtual void messageAboutToBeSend() =0;
	virtual void messageReady() =0;
	virtual void editorCleared() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void autoResizeChanged(bool AResize) =0;
	virtual void minimumLinesChanged(int ALines) =0;
	virtual void sendKeyChanged(const QKeySequence &AKey) =0;
};

class IReceiversWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual QList<Jid> receivers() const =0;
	virtual QString receiverName(const Jid &AReceiver) const =0;
	virtual void addReceiversGroup(const QString &AGroup) =0;
	virtual void removeReceiversGroup(const QString &AGroup) =0;
	virtual void addReceiver(const Jid &AReceiver) =0;
	virtual void removeReceiver(const Jid &AReceiver) =0;
	virtual void clear() =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void receiverAdded(const Jid &AReceiver) =0;
	virtual void receiverRemoved(const Jid &AReceiver) =0;
};

class IMenuBarWidget
{
public:
	virtual QMenuBar *instance() = 0;
	virtual MenuBarChanger *menuBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

class IToolBarWidget
{
public:
	virtual QToolBar *instance() = 0;
	virtual ToolBarChanger *toolBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

class IStatusBarWidget
{
public:
	virtual QStatusBar *instance() = 0;
	virtual StatusBarChanger *statusBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

class ITabWindowPage
{
public:
	virtual QWidget *instance() =0;
	virtual QString tabPageId() const =0;
	virtual void showWindow() =0;
	virtual void closeWindow() =0;
protected:
	virtual void windowShow() =0;
	virtual void windowClose() =0;
	virtual void windowChanged() =0;
	virtual void windowDestroyed() =0;
};

class ITabWindow
{
public:
	virtual QMainWindow *instance() = 0;
	virtual void showWindow() =0;
	virtual QUuid windowId() const =0;
	virtual QString windowName() const =0;
	virtual Menu *windowMenu() const =0;
	virtual void addPage(ITabWindowPage *APage) =0;
	virtual bool hasPage(ITabWindowPage *APage) const=0;
	virtual ITabWindowPage *currentPage() const =0;
	virtual void setCurrentPage(ITabWindowPage *APage) =0;
	virtual void detachPage(ITabWindowPage *APage) =0;
	virtual void removePage(ITabWindowPage *APage) =0;
protected:
	virtual void pageAdded(ITabWindowPage *APage) =0;
	virtual void currentPageChanged(ITabWindowPage *APage) =0;
	virtual void pageRemoved(ITabWindowPage *APage) =0;
	virtual void pageDetached(ITabWindowPage *APage) =0;
	virtual void windowChanged() =0;
	virtual void windowDestroyed() =0;
};

class IChatWindow :
			public ITabWindowPage
{
public:
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IMenuBarWidget *menuBarWidget() const =0;
	virtual IToolBarWidget *toolBarWidget() const =0;
	virtual IStatusBarWidget *statusBarWidget() const =0;
	virtual bool isActive() const =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle) =0;
protected:
	virtual void messageReady() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void windowActivated() =0;
	virtual void windowClosed() =0;
};

class IMessageWindow :
			public ITabWindowPage
{
public:
	enum Mode {
		ReadMode    =1,
		WriteMode   =2
	};
public:
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual void addTabWidget(QWidget *AWidget) =0;
	virtual void setCurrentTabWidget(QWidget *AWidget) =0;
	virtual void removeTabWidget(QWidget *AWidget) =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
	virtual IToolBarWidget *viewToolBarWidget() const =0;
	virtual IToolBarWidget *editToolBarWidget() const =0;
	virtual Mode mode() const =0;
	virtual void setMode(Mode AMode) =0;
	virtual QString subject() const =0;
	virtual void setSubject(const QString &ASubject) =0;
	virtual QString threadId() const =0;
	virtual void setThreadId(const QString &AThreadId) =0;
	virtual int nextCount() const =0;
	virtual void setNextCount(int ACount) =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle) =0;
protected:
	virtual void showNextMessage() =0;
	virtual void replyMessage() =0;
	virtual void forwardMessage() =0;
	virtual void showChatWindow() =0;
	virtual void messageReady() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void windowActivated() =0;
	virtual void windowClosed() =0;
};

class IViewDropHandler
{
public:
	virtual bool viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent) =0;
	virtual bool viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent) =0;
	virtual void viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent) =0;
	virtual bool viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu) =0;
};

class IViewUrlHandler
{
public:
	virtual bool viewUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder) =0;
};

class IMessageWidgets
{
public:
	virtual QObject *instance() = 0;
	virtual IPluginManager *pluginManager() const =0;
	virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid) =0;
	virtual IMenuBarWidget *newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual IStatusBarWidget *newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual QList<IMessageWindow *> messageWindows() const =0;
	virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode) =0;
	virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<IChatWindow *> chatWindows() const =0;
	virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<QUuid> tabWindowList() const =0;
	virtual QUuid appendTabWindow(const QString &AName) =0;
	virtual void deleteTabWindow(const QUuid &AWindowId) =0;
	virtual QString tabWindowName(const QUuid &AWindowId) const =0;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName) =0;
	virtual QList<ITabWindow *> tabWindows() const =0;
	virtual ITabWindow *openTabWindow(const QUuid &AWindowId) =0;
	virtual ITabWindow *findTabWindow(const QUuid &AWindowId) const =0;
	virtual void assignTabWindowPage(ITabWindowPage *APage) =0;
	virtual QList<IViewDropHandler *> viewDropHandlers() const =0;
	virtual void insertViewDropHandler(IViewDropHandler *AHandler) =0;
	virtual void removeViewDropHandler(IViewDropHandler *AHandler) =0;
	virtual QMultiMap<int, IViewUrlHandler *> viewUrlHandlers() const =0;
	virtual void insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual void removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder) =0;
protected:
	virtual void infoWidgetCreated(IInfoWidget *AInfoWidget) =0;
	virtual void viewWidgetCreated(IViewWidget *AViewWidget) =0;
	virtual void editWidgetCreated(IEditWidget *AEditWidget) =0;
	virtual void receiversWidgetCreated(IReceiversWidget *AReceiversWidget) =0;
	virtual void menuBarWidgetCreated(IMenuBarWidget *AMenuBarWidget) =0;
	virtual void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget) =0;
	virtual void statusBarWidgetCreated(IStatusBarWidget *AStatusBarWidget) =0;
	virtual void messageWindowCreated(IMessageWindow *AWindow) =0;
	virtual void messageWindowDestroyed(IMessageWindow *AWindow) =0;
	virtual void chatWindowCreated(IChatWindow *AWindow) =0;
	virtual void chatWindowDestroyed(IChatWindow *AWindow) =0;
	virtual void tabWindowAppended(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowDeleted(const QUuid &AWindowId) =0;
	virtual void tabWindowCreated(ITabWindow *AWindow) =0;
	virtual void tabWindowDestroyed(ITabWindow *AWindow) =0;
	virtual void viewDropHandlerInserted(IViewDropHandler *AHandler) =0;
	virtual void viewDropHandlerRemoved(IViewDropHandler *AHandler) =0;
	virtual void viewUrlHandlerInserted(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual void viewUrlHandlerRemoved(IViewUrlHandler *AHandler, int AOrder) =0;
};

Q_DECLARE_INTERFACE(IInfoWidget,"Virtus.Plugin.IInfoWidget/1.0")
Q_DECLARE_INTERFACE(IViewWidget,"Virtus.Plugin.IViewWidget/1.0")
Q_DECLARE_INTERFACE(IEditWidget,"Virtus.Plugin.IEditWidget/1.0")
Q_DECLARE_INTERFACE(IReceiversWidget,"Virtus.Plugin.IReceiversWidget/1.0")
Q_DECLARE_INTERFACE(IMenuBarWidget,"Virtus.Plugin.IMenuBarWidget/1.0")
Q_DECLARE_INTERFACE(IToolBarWidget,"Virtus.Plugin.IToolBarWidget/1.0")
Q_DECLARE_INTERFACE(IStatusBarWidget,"Virtus.Plugin.IStatusBarWidget/1.0")
Q_DECLARE_INTERFACE(ITabWindowPage,"Virtus.Plugin.ITabWindowPage/1.0")
Q_DECLARE_INTERFACE(ITabWindow,"Virtus.Plugin.ITabWindow/1.0")
Q_DECLARE_INTERFACE(IChatWindow,"Virtus.Plugin.IChatWindow/1.0")
Q_DECLARE_INTERFACE(IMessageWindow,"Virtus.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IViewDropHandler,"Virtus.Plugin.IViewDropHandler/1.0")
Q_DECLARE_INTERFACE(IViewUrlHandler,"Virtus.Plugin.IViewUrlHandler/1.0")
Q_DECLARE_INTERFACE(IMessageWidgets,"Virtus.Plugin.IMessageWidgets/1.0")

#endif