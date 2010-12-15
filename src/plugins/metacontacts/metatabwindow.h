#ifndef METATABWINDOW_H
#define METATABWINDOW_H

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/toolbarchanger.h>
#include "ui_metatabwindow.h"

class MetaTabWindow :
		public QMainWindow,
		public IMetaTabWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMetaTabWindow ITabPage);
public:
	MetaTabWindow(IMessageWidgets *AMessageWidgets, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent = NULL);
	~MetaTabWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabPage
	virtual void showTabPage();
	virtual void closeTabPage();
	virtual QString tabPageId() const;
	virtual bool isActive() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
	//IMetaTabWindow
	virtual Jid metaId() const;
	virtual IMetaRoster *metaRoster() const;
	virtual ITabPage *itemPage(const Jid &AItemJid) const;
	virtual void setItemPage(const Jid &AItemJid, ITabPage *APage);
	virtual Jid currentItem() const;
	virtual void setCurrentItem(const Jid &AItemJid);
signals:
	//ITabPage
	void tabPageShow();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
	//IMetaTabWindow
	void currentItemChanged(const Jid &AItemJid);
	void itemPageRequested(const Jid &AItemJid);
	void itemPageChanged(const Jid &AItemJid, ITabPage *APage);
protected:
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	void updateWindow();
	void updateItemButtons(const QSet<Jid> &AItems);
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onTabPageShow();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabPageNotifierChanged();
	void onTabPageNotifierActiveNotifyChanged(int ANotifyId);
protected slots:
	void onItemButtonActionTriggered();
	void onCurrentWidgetChanged(int AIndex);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
private:
	Ui::MetaTabWindowClass ui;
private:
	IMetaRoster *FMetaRoster;
	IMessageWidgets *FMessageWidgets;
	ITabPageNotifier *FTabPageNotifier;
private:
	Jid FMetaId;
	bool FShownDetached;
	ToolBarChanger *FToolBarChanger;
	QMap<Jid, ITabPage *> FItemTabPages;
	QMap<Jid, QToolButton *> FItemButtons;
};

#endif // METATABWINDOW_H