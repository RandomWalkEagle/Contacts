#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QUuid>
#include <QEvent>
#include <QTimer>
#include <QDialog>
#include <definations/version.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/imainwindow.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include "ui_logindialog.h"

class LoginDialog :
			public QDialog
{
	Q_OBJECT;
public:
	LoginDialog(IPluginManager *APluginManager, QWidget *AParent = NULL);
	~LoginDialog();
public:
	void connectIfReady();
public slots:
	virtual void reject();
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void initialize(IPluginManager *APluginManager);
	bool isCapsLockOn() const;
	void closeCurrentProfile();
	void setConnectEnabled(bool AEnabled);
	void stopReconnection();
	void hideConnectionError();
	void showConnectionError(const QString &ACaption, const QString &AError);
	void hideXmppStreamError();
	void showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint);
	void saveCurrentProfileSettings();
	void loadCurrentProfileSettings();
protected slots:
	void onConnectClicked();
	void onXmppStreamOpened();
	void onXmppStreamClosed();
	void onReconnectTimerTimeout();
	void onCompleterHighLighted(const QString &AText);
	void onCompleterActivated(const QString &AText);
	void onDomainCurrentIntexChanged(int AIndex);
	void onLabelLinkActivated(const QString &ALink);
	void onShowConnectingAnimation();
private:
	Ui::LoginDialogClass ui;
private:
	IAccountManager *FAccountManager;
	IOptionsManager *FOptionsManager;
	IStatusChanger *FStatusChanger;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	bool FNewProfile;
	QUuid FAccountId;
	QTimer FReconnectTimer;
};

#endif // LOGINDIALOG_H