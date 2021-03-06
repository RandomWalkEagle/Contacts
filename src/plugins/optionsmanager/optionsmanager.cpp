#include "optionsmanager.h"

#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QCryptographicHash>
#include <definitions/customborder.h>

#ifdef Q_WS_WIN
# include <QSettings>
#endif

#define DIR_PROFILES                    "profiles"
#define DIR_BINARY                      "binary"
#define FILE_PROFILE                    "profile.xml"
#define FILE_PROFILEDATA                "login.xml"
#define FILE_OPTIONS                    "options.xml"
#define FILE_OPTIONS_COPY               "options.xml.copy"
#define FILE_OPTIONS_FAIL               "options.xml.fail"

#define FILE_BLOCKER                    "blocked"

#define PROFILE_VERSION                 "1.0"

#define ADR_PROFILE                     Action::DR_Parametr1

#define PST_OPTIONS                     "options"
#define PSN_OPTIONS                     "rambler:options"

OptionsManager::OptionsManager()
{
	FPluginManager = NULL;
	FTrayManager = NULL;
	FMainWindowPlugin = NULL;
	FPrivateStorage = NULL;
	FOptionsDialog = NULL;
	FLoginDialog = NULL;
	FSystemIntegration = NULL;

	FAutoSaveTimer.setInterval(30*1000);
	FAutoSaveTimer.setSingleShot(true);
	connect(&FAutoSaveTimer, SIGNAL(timeout()),SLOT(onAutoSaveOptionsTimerTimeout()));

	FServerOptionsTimer.setInterval(1000);
	FServerOptionsTimer.setSingleShot(true);
	connect(&FServerOptionsTimer, SIGNAL(timeout()),SLOT(onSaveServerOptionsTimerTimeout()));

	qsrand(QDateTime::currentDateTime().toTime_t());
}

OptionsManager::~OptionsManager()
{
	if (FOptionsDialog)
		FOptionsDialog->window()->deleteLater();
}

void OptionsManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Options Manager");
	APluginInfo->description = tr("Allows to save, load and manage user preferences");
	APluginInfo ->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool OptionsManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onAboutToQuit()));

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageAboutToClose(const Jid &)),SLOT(onPrivateStorageAboutToClose(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("ISystemIntegration").value(0,NULL);
	if (plugin)
	{
		FSystemIntegration = qobject_cast<ISystemIntegration *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool OptionsManager::initObjects()
{
	FProfilesDir.setPath(FPluginManager->homePath());
	if (!FProfilesDir.exists(DIR_PROFILES))
		FProfilesDir.mkdir(DIR_PROFILES);
	FProfilesDir.cd(DIR_PROFILES);

	FChangeProfileAction = new Action(this);
	FChangeProfileAction->setText(tr("Change User"));
	FChangeProfileAction->setData(Action::DR_SortString,QString("100"));
	connect(FChangeProfileAction,SIGNAL(triggered(bool)),SLOT(onChangeProfileByAction(bool)));

	FShowOptionsDialogAction = new Action(this);
	FShowOptionsDialogAction->setVisible(false);
	FShowOptionsDialogAction->setText(tr("Options"));
	FShowOptionsDialogAction->setShortcutContext(Qt::ApplicationShortcut);
	FShowOptionsDialogAction->setData(Action::DR_SortString,QString("300"));
	connect(FShowOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(onShowOptionsDialogByAction(bool)));

	if (FSystemIntegration)
	{
		Action * menuBarSettings = new Action;
		menuBarSettings->setText("settings");
		menuBarSettings->setShortcut(tr("Ctrl+,"));
		connect(menuBarSettings,SIGNAL(triggered(bool)),SLOT(onShowOptionsDialogByAction(bool)));
		menuBarSettings->setMenuRole(QAction::PreferencesRole);
		FSystemIntegration->addAction(ISystemIntegration::FileRole, menuBarSettings);

		FChangeProfileAction->setMenuRole(QAction::ApplicationSpecificRole);
		FSystemIntegration->addAction(ISystemIntegration::FileRole, FChangeProfileAction);
	}

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FShowOptionsDialogAction,AG_MMENU_OPTIONS_SHOWDIALOG,true);
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FChangeProfileAction,AG_MMENU_OPTIONS_CHANGEPROFILE,true);
	}

#ifndef Q_WS_MAC
	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FShowOptionsDialogAction,AG_TMTM_OPTIONS_DIALOG,true);
	}
#endif

	return true;
}

bool OptionsManager::initSettings()
{
	Options::setDefaultValue(OPV_MISC_AUTOSTART, false);
	Options::setDefaultValue(OPV_MISC_OPTIONS_SAVE_ON_SERVER, true);
	Options::setDefaultValue(OPV_MISC_CUSTOMBORDERSENABLED, CustomBorderStorage::isBordersEnabled());

#ifdef Q_WS_MAC
	Options::setDefaultValue(OPV_MISC_OPTIONS_DIALOG_LASTNODE, QString(OPN_GATEWAYS));
#else
	Options::setDefaultValue(OPV_MISC_OPTIONS_DIALOG_LASTNODE, QString(OPN_COMMON));
#endif

#ifdef Q_WS_MAC
	IOptionsDialogNode dnode = { ONO_COMMON, OPN_COMMON, tr("Synchronization"), MNI_OPTIONS_DIALOG };
#else
	IOptionsDialogNode dnode = { ONO_COMMON, OPN_COMMON, tr("Common Settings"), MNI_OPTIONS_DIALOG };
#endif
	insertOptionsDialogNode(dnode);
	insertOptionsHolder(this);

	return true;
}

bool OptionsManager::startPlugin()
{
	LoginDialog *dialog = qobject_cast<LoginDialog *>(showLoginDialog());
	if (dialog)
	{
		dialog->loadLastProfile();
		dialog->connectIfReady();
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> OptionsManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_COMMON)
	{
#ifndef Q_WS_MAC
		widgets.insertMulti(OWO_COMMON, optionsHeaderWidget(QString::null, tr("Common settings"), AParent));
#endif
#ifdef Q_WS_WIN
		widgets.insertMulti(OWO_COMMON_AUTOSTART, optionsNodeWidget(Options::node(OPV_MISC_AUTOSTART), tr("Launch application on system start up"), AParent));
#endif

#if defined(Q_WS_X11) || defined(DEBUG_ENABLED)
		if (CustomBorderStorage::isBordersAvail())
			widgets.insertMulti(OWO_COMMON_BORDERSENABLE, optionsNodeWidget(Options::node(OPV_MISC_CUSTOMBORDERSENABLED), tr("Enable windows customization (restart required)"), AParent));
#endif

#ifndef Q_WS_MAC
		widgets.insertMulti(OWO_COMMON_SINC, optionsHeaderWidget(QString::null, tr("Backing store your chat history and preferences"), AParent));
#endif
		widgets.insertMulti(OWO_COMMON_SINC_OPTIONS, optionsNodeWidget(Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER), tr("Sync preferences on my several computers"), AParent));

		widgets.insertMulti(OWO_COMMON_LOCALE, optionsHeaderWidget(QString::null, tr("Interface language"), AParent));
		widgets.insertMulti(OWO_COMMON_LOCALE_SELECT, new LocaleOptionsWidget(AParent));
	}
	return widgets;
}

bool OptionsManager::isOpened() const
{
	return !FProfile.isEmpty();
}

QList<QString> OptionsManager::profiles() const
{
	QList<QString> profileList;

	foreach(QString dirName, FProfilesDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
		if (FProfilesDir.exists(dirName + "/" FILE_PROFILE))
			profileList.append(dirName);

	return profileList;
}

QString OptionsManager::profilePath(const QString &AProfile) const
{
	return FProfilesDir.absoluteFilePath(AProfile);
}

QString OptionsManager::lastActiveProfile() const
{
	QDateTime lastModified;
	QString lastProfile;
	foreach(QString profile, profiles())
	{
		QFileInfo info(profilePath(profile) + "/" FILE_OPTIONS);
		if (info.exists() && info.lastModified()>lastModified)
		{
			lastProfile = profile;
			lastModified = info.lastModified();
		}
	}
	return lastProfile;
}

QString OptionsManager::currentProfile() const
{
	return FProfile;
}

QByteArray OptionsManager::currentProfileKey() const
{
	return FProfileKey;
}

bool OptionsManager::setCurrentProfile(const QString &AProfile, const QString &APassword)
{
	if (AProfile.isEmpty())
	{
		closeProfile();
		return true;
	}
	else if (AProfile == currentProfile())
	{
		return true;
	}
	else if (checkProfilePassword(AProfile, APassword))
	{
		LogDetail(QString("[OptionsManager] Changing current profile to '%1'").arg(AProfile));

		closeProfile();
		FProfileLocker = new QtLockedFile(QDir(profilePath(AProfile)).absoluteFilePath(FILE_BLOCKER));
		if (FProfileLocker->open(QFile::WriteOnly) && FProfileLocker->lock(QtLockedFile::WriteLock, false))
		{
			QDir profileDir(profilePath(AProfile));
			if (!profileDir.exists(DIR_BINARY))
				profileDir.mkdir(DIR_BINARY);

			// Loading options from file
			QFile optionsFile(profileDir.filePath(FILE_OPTIONS));
			if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(optionsFile.readAll(),true))
			{
				// Trying to open valid copy of options
				optionsFile.close();
				optionsFile.setFileName(profileDir.filePath(FILE_OPTIONS_COPY));
				if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(optionsFile.readAll(),true))
				{
					FProfileOptions.clear();
					FProfileOptions.appendChild(FProfileOptions.createElement("options")).toElement();
				}
				// Renaming invalid options file
				QFile::remove(profileDir.filePath(FILE_OPTIONS_FAIL));
				QFile::rename(profileDir.filePath(FILE_OPTIONS),profileDir.filePath(FILE_OPTIONS_FAIL));
			}
			else
			{
				// Saving the copy of valid options
				QFile::remove(profileDir.filePath(FILE_OPTIONS_COPY));
				QFile::copy(profileDir.filePath(FILE_OPTIONS),profileDir.filePath(FILE_OPTIONS_COPY));
			}
			optionsFile.close();

			if (profileKey(AProfile,APassword).size() < 16)
				changeProfilePassword(AProfile,APassword,APassword);

			openProfile(AProfile, APassword);
			return true;
		}
		else
		{
			LogError(QString("[OptionsManager] Profile '%1' is locked").arg(AProfile));
		}
		FProfileLocker->close();
		delete FProfileLocker;
	}
	else
	{
		LogError(QString("[OptionsManager] Failed to change current profile to '%1': Invalid profile password").arg(AProfile));
	}
	return false;
}

QByteArray OptionsManager::profileKey(const QString &AProfile, const QString &APassword) const
{
	if (checkProfilePassword(AProfile, APassword))
	{
		QDomNode keyText = profileDocument(AProfile).documentElement().firstChildElement("key").firstChild();
		while (!keyText.isNull() && !keyText.isText())
			keyText = keyText.nextSibling();

		QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
		return Options::decrypt(keyValue, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
	}
	return QByteArray();
}

QMap<QString, QVariant> OptionsManager::profileData(const QString &AProfile) const
{
	QMap<QString,QVariant> data;
	if (profiles().contains(AProfile))
	{
		QDomDocument doc;
		QFile login(QDir(profilePath(AProfile)).absoluteFilePath(FILE_PROFILEDATA));
		if (login.open(QFile::ReadOnly) && doc.setContent(&login))
		{
			QDomElement elem = doc.documentElement().firstChildElement();
			while(!elem.isNull())
			{
				data.insert(elem.tagName(),elem.text());
				elem = elem.nextSiblingElement();
			}
		}
		login.close();
	}
	return data;
}

bool OptionsManager::setProfileData(const QString &AProfile, const QMap<QString, QVariant> &AData)
{
	if (profiles().contains(AProfile))
	{
		QFile login(QDir(profilePath(AProfile)).absoluteFilePath(FILE_PROFILEDATA));
		if (login.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			doc.appendChild(doc.createElement("profile-data"));

			for(QMap<QString, QVariant>::const_iterator it=AData.constBegin(); it!=AData.constEnd(); it++)
				doc.documentElement().appendChild(doc.createElement(it.key())).appendChild(doc.createTextNode(it->toString()));

			login.write(doc.toByteArray());
			login.close();
			return true;
		}
	}
	return false;
}

bool OptionsManager::setProfileData(const QString &AProfile, const QString &AKey, const QVariant &AValue)
{
	QMap<QString, QVariant> data = profileData(AProfile);
	if (AValue.isValid())
		data.insert(AKey,AValue);
	else
		data.remove(AKey);
	return setProfileData(AProfile,data);
}

bool OptionsManager::checkProfilePassword(const QString &AProfile, const QString &APassword) const
{
	QDomDocument profileDoc = profileDocument(AProfile);
	if (!profileDoc.isNull())
	{
		QDomNode passText = profileDoc.documentElement().firstChildElement("password").firstChild();
		while (!passText.isNull() && !passText.isText())
			passText = passText.nextSibling();

		if (passText.isNull() && APassword.isEmpty())
			return true;

		QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
		return passHash.toHex() == passText.toText().data().toLatin1();
	}
	return false;
}

bool OptionsManager::changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword)
{
	if (checkProfilePassword(AProfile, AOldPassword))
	{
		QDomDocument profileDoc = profileDocument(AProfile);

		QDomElement passElem = profileDoc.documentElement().firstChildElement("password");
		if (passElem.isNull())
			passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password")).toElement();

		QDomNode passText = passElem.firstChild();
		while (!passText.isNull() && !passText.isText())
			passText = passText.nextSibling();

		QByteArray newPassHash = QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Sha1);
		if (passText.isNull())
			passElem.appendChild(passElem.ownerDocument().createTextNode(newPassHash.toHex()));
		else
			passText.toText().setData(newPassHash.toHex());

		QDomNode keyText = profileDoc.documentElement().firstChildElement("key").firstChild();
		while (!keyText.isNull() && !keyText.isText())
			keyText = keyText.nextSibling();

		QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
		keyValue = Options::decrypt(keyValue, QCryptographicHash::hash(AOldPassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
		if (keyValue.size() < 16)
		{
			keyValue.resize(16);
			for (int i=0; i<keyValue.size(); i++)
				keyValue[i] = qrand();
		}
		keyValue = Options::encrypt(keyValue, QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Md5));
		keyText.toText().setData(keyValue.toBase64());

		return saveProfile(AProfile, profileDoc);
	}
	return false;
}

bool OptionsManager::addProfile(const QString &AProfile, const QString &APassword)
{
	if (!profiles().contains(AProfile))
	{
		LogDetail(QString("[OptionsManager] Creating new profile '%1'").arg(AProfile));
		if (FProfilesDir.exists(AProfile) || FProfilesDir.mkdir(AProfile))
		{
			QDomDocument profileDoc;
			profileDoc.appendChild(profileDoc.createElement("profile"));
			profileDoc.documentElement().setAttribute("version",PROFILE_VERSION);

			QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
			QDomNode passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password"));
			passElem.appendChild(profileDoc.createTextNode(passHash.toHex()));

			QByteArray keyData(16,0);
			for (int i=0; i<keyData.size(); i++)
				keyData[i] = qrand();
			keyData = Options::encrypt(keyData, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5));

			QDomNode keyElem = profileDoc.documentElement().appendChild(profileDoc.createElement("key"));
			keyElem.appendChild(profileDoc.createTextNode(keyData.toBase64()));

			if (saveProfile(AProfile, profileDoc))
			{
				emit profileAdded(AProfile);
				return true;
			}
		}
		else
		{
			LogError(QString("[OptionsManager] Failed to create profile directory '%1'").arg(FProfilesDir.absoluteFilePath(AProfile)));
			ReportError("FAILED-CREATE-PROFILE-DIR",QString("[OptionsManager] Failed to create profile directory '%1'").arg(FProfilesDir.absoluteFilePath(AProfile)),false);
		}
	}
	return false;
}

bool OptionsManager::renameProfile(const QString &AProfile, const QString &ANewName)
{
	if (!FProfilesDir.exists(ANewName) && FProfilesDir.rename(AProfile, ANewName))
	{
		emit profileRenamed(AProfile,ANewName);
		return true;
	}
	return false;
}

bool OptionsManager::removeProfile(const QString &AProfile)
{
	QDir profileDir(profilePath(AProfile));
	if (profileDir.exists())
	{
		LogDetail(QString("[OptionsManager] Removing profile '%1'").arg(AProfile));

		if (AProfile == currentProfile())
			closeProfile();

		if (profileDir.remove(FILE_PROFILE))
		{
			emit profileRemoved(AProfile);
			return true;
		}
		else
		{
			LogError(QString("[OptionsManager] Failed to remove profile file '%1'").arg(profileDir.absoluteFilePath(FILE_PROFILE)));
		}
	}
	return false;
}

QList<QString> OptionsManager::serverOptions() const
{
	return FServerOptions;
}

void OptionsManager::insertServerOption(const QString &APath)
{
	if (!APath.isEmpty() && !FServerOptions.contains(APath))
		FServerOptions.append(APath);
}

void OptionsManager::removeServerOption(const QString &APath)
{
	FServerOptions.removeAll(APath);
}

bool OptionsManager::isLoginDialogVisible() const
{
	return FLoginDialog!=NULL && FLoginDialog->isVisible();
}

QDialog *OptionsManager::showLoginDialog(QWidget *AParent)
{
	if (!FLoginDialog)
	{
		FLoginDialog = new LoginDialog(FPluginManager,AParent);
		connect(FLoginDialog,SIGNAL(accepted()),SLOT(onLoginDialogAccepted()));
		connect(FLoginDialog,SIGNAL(rejected()),SLOT(onLoginDialogRejected()));
		WidgetManager::showActivateRaiseWindow(FLoginDialog->window());
	}
	return FLoginDialog;
}

QList<IOptionsHolder *> OptionsManager::optionsHolders() const
{
	return FOptionsHolders;
}

void OptionsManager::insertOptionsHolder(IOptionsHolder *AHolder)
{
	if (!FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.append(AHolder);
		emit optionsHolderInserted(AHolder);
	}
}

void OptionsManager::removeOptionsHolder(IOptionsHolder *AHolder)
{
	if (FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.removeAll(AHolder);
		emit optionsHolderRemoved(AHolder);
	}
}

QList<IOptionsDialogNode> OptionsManager::optionsDialogNodes() const
{
	return FOptionsDialogNodes.values();
}

IOptionsDialogNode OptionsManager::optionsDialogNode(const QString &ANodeId) const
{
	return FOptionsDialogNodes.value(ANodeId);
}

void OptionsManager::insertOptionsDialogNode(const IOptionsDialogNode &ANode)
{
	if (!ANode.nodeId.isEmpty())
	{
		FOptionsDialogNodes[ANode.nodeId] = ANode;
		emit optionsDialogNodeInserted(ANode);
	}
}

void OptionsManager::removeOptionsDialogNode(const QString &ANodeId)
{
	if (FOptionsDialogNodes.contains(ANodeId))
	{
		emit optionsDialogNodeRemoved(FOptionsDialogNodes.take(ANodeId));
	}
}

QWidget *OptionsManager::showOptionsDialog(const QString &ANodeId, QWidget *AParent)
{
	if (isOpened())
	{
		if (!FOptionsDialog)
		{
			FOptionsDialog = new OptionsDialog(this,AParent);
			connect(FOptionsDialog, SIGNAL(applied()), SLOT(onOptionsDialogApplied()));
			connect(FOptionsDialog, SIGNAL(dialogDestroyed()), SLOT(onOptionsDialogDestroyed()));
		}
		FOptionsDialog->showNode(ANodeId.isNull() ? Options::node(OPV_MISC_OPTIONS_DIALOG_LASTNODE).value().toString() : ANodeId);
		WidgetManager::showActivateRaiseWindow(FOptionsDialog->window());
	}
	return FOptionsDialog->window();
}

IOptionsWidget *OptionsManager::optionsHeaderWidget(const QString &AIconKey, const QString &ACaption, QWidget *AParent) const
{
	return new OptionsHeader(AIconKey,ACaption,AParent);
}

IOptionsWidget *OptionsManager::optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const
{
	return new OptionsWidget(ANode, ACaption, AParent);
}

void OptionsManager::openProfile(const QString &AProfile, const QString &APassword)
{
	if (!isOpened())
	{
		LogDetail(QString("[OptionsManager] Opening profile '%1'").arg(AProfile));
		FProfile = AProfile;
		FProfileKey = profileKey(AProfile, APassword);
		Options::setOptions(FProfileOptions, profilePath(AProfile) + "/" DIR_BINARY, FProfileKey);
		Options::node(OPV_MISC_CUSTOMBORDERSENABLED).setValue(CustomBorderStorage::isBordersEnabled());
		FShowOptionsDialogAction->setVisible(true);
		FChangeProfileAction->setText(tr("Change User (%1)").arg(Jid(Jid::decode(AProfile)).uNode()));
		emit profileOpened(AProfile);
	}
}

bool OptionsManager::saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const
{
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::WriteOnly|QFile::Truncate))
	{
		file.write(AProfileDoc.toString(2).toUtf8());
		file.close();
		return true;
	}
	else
	{
		LogError(QString("[OptionsManager] Failed to save profile '%1' to file '%2'").arg(AProfile,file.fileName()));
	}
	return false;
}

void OptionsManager::closeProfile()
{
	if (isOpened())
	{
		LogDetail(QString("[OptionsManager] Closing profile '%1'").arg(currentProfile()));
		emit profileClosed(currentProfile());
		FAutoSaveTimer.stop();
		if (FOptionsDialog)
		{
			if (CustomBorderStorage::isBordered(FOptionsDialog))
				CustomBorderStorage::widgetBorder(FOptionsDialog)->closeWidget();
			else
				FOptionsDialog->close();
		}
		FShowOptionsDialogAction->setVisible(false);
		FChangeProfileAction->setText(tr("Change User"));
		Options::setOptions(QDomDocument(), QString::null, QByteArray());
		saveOptions();
		FProfile.clear();
		FProfileKey.clear();
		FProfileOptions.clear();
		FProfileLocker->unlock();
		FProfileLocker->close();
		FProfileLocker->remove();
		delete FProfileLocker;
	}
}

bool OptionsManager::saveOptions() const
{
	if (isOpened())
	{
		QFile file(QDir(profilePath(currentProfile())).filePath(FILE_OPTIONS));
		LogDetail(QString("[OptionsManager] Saving options to file '%1'").arg(file.fileName()));
		if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
		{
			file.write(FProfileOptions.toString(2).toUtf8());
			file.close();
			return true;
		}
		else
		{
			LogError(QString("[OptionsManager] Failed to save options to file '%1'").arg(file.fileName()));
			ReportError("FAILED-SAVE-OPTIONS",QString("[OptionsManager] Failed to save options to file '%1'").arg(file.fileName()),false);
		}
	}
	return false;
}

bool OptionsManager::loadServerOptions(const Jid &AStreamJid)
{
	if (FPrivateStorage && AStreamJid.isValid() && Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER).value().toBool())
	{
		LogDetail(QString("[OptionsManager] Loading options from server"));
		if (FPrivateStorage->loadData(AStreamJid,PST_OPTIONS,PSN_OPTIONS).isEmpty())
			LogError(QString("[OptionsManager] Failed to load options from server"));
		else
			return true;
	}
	return false;
}

bool OptionsManager::saveServerOptions(const Jid &AStreamJid)
{
	FServerOptionsTimer.stop();
	if (FPrivateStorage && AStreamJid.isValid() && Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER).value().toBool())
	{
		QDomDocument doc;
		doc.appendChild(doc.createElement("options"));

		if (FPrivateStorage->isLoaded(AStreamJid,PST_OPTIONS,PSN_OPTIONS))
			doc.documentElement().appendChild(FPrivateStorage->getData(AStreamJid,PST_OPTIONS,PSN_OPTIONS).cloneNode(true));
		else
			doc.documentElement().appendChild(doc.createElementNS(PSN_OPTIONS,PST_OPTIONS)).toElement();

		QDomElement root = doc.documentElement().firstChildElement();
		foreach(QString path, FServerOptions)
			Options::exportNode(path,root);

		LogDetail(QString("[OptionsManager] Saving server options"));
		if (FPrivateStorage->saveData(AStreamJid,root).isEmpty())
			LogError(QString("[OptionsManager] Failed to save server options"));
		else
			return true;
	}
	return false;
}

QDomDocument OptionsManager::profileDocument(const QString &AProfile) const
{
	QDomDocument doc;
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::ReadOnly))
	{
		doc.setContent(file.readAll(),true);
		file.close();
	}
	return doc;
}

void OptionsManager::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MISC_AUTOSTART)
	{
#ifdef Q_WS_WIN
		QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
		if (ANode.value().toBool())
			reg.setValue(CLIENT_NAME, QDir::toNativeSeparators(QApplication::applicationFilePath()));
		else
			reg.remove(CLIENT_NAME);
		setProfileData(currentProfile(),"auto-run",ANode.value().toBool());
#endif
	}
	else if (ANode.path() == OPV_MISC_CUSTOMBORDERSENABLED)
	{
		if (ANode.value().toBool() != CustomBorderStorage::isBordersEnabled())
		{
			CustomBorderStorage::setBordersEnabled(ANode.value().toBool());
			QTimer::singleShot(0, FPluginManager->instance(), SLOT(restart()));
		}
	}
	else if (ANode.path() == OPV_MISC_OPTIONS_SAVE_ON_SERVER)
	{
		if (ANode.value().toBool())
			loadServerOptions(FOptionsStreamJid);
	}

	foreach(QString path, FServerOptions)
	{
		if(Options::node(path).isChildNode(ANode))
		{
			FServerOptionsTimer.start();
			break;
		}
	}

	FAutoSaveTimer.start();
}

void OptionsManager::onOptionsDialogApplied()
{
	saveOptions();
}

void OptionsManager::onOptionsDialogDestroyed()
{
	FOptionsDialog = NULL;
}

void OptionsManager::onChangeProfileByAction(bool)
{
	showLoginDialog();
}

void OptionsManager::onShowOptionsDialogByAction(bool)
{
	showOptionsDialog();
}

void OptionsManager::onLoginDialogAccepted()
{
	FLoginDialog = NULL;
}

void OptionsManager::onLoginDialogRejected()
{
	FLoginDialog = NULL;
	if (!isOpened())
		FPluginManager->quit();
}

void OptionsManager::onAutoSaveOptionsTimerTimeout()
{
	saveOptions();
}

void OptionsManager::onSaveServerOptionsTimerTimeout()
{
	if (FOptionsDialog == NULL)
		saveServerOptions(FOptionsStreamJid);
	else
		FServerOptionsTimer.start();
}

void OptionsManager::onPrivateStorageOpened(const Jid &AStreamJid)
{
	if (FOptionsStreamJid.isEmpty())
	{
		FOptionsStreamJid = AStreamJid;
		loadServerOptions(FOptionsStreamJid);
	}
}

void OptionsManager::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (FOptionsStreamJid==AStreamJid && AElement.tagName()==PST_OPTIONS && AElement.namespaceURI()==PSN_OPTIONS)
	{
		LogDetail(QString("[OptionsManager] Importing options from server"));
		foreach(QString path, FServerOptions)
			Options::importNode(path,AElement);
		FServerOptionsTimer.stop();
	}
}

void OptionsManager::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FOptionsStreamJid==AStreamJid && ATagName==PST_OPTIONS && ANamespace==PSN_OPTIONS)
		loadServerOptions(FOptionsStreamJid);
}

void OptionsManager::onPrivateStorageAboutToClose(const Jid &AStreamJid)
{
	if (FOptionsStreamJid == AStreamJid)
	{
		if (FServerOptionsTimer.isActive())
			saveServerOptions(FOptionsStreamJid);
		FOptionsStreamJid = Jid::null;
	}
}

void OptionsManager::onAboutToQuit()
{
	closeProfile();
}

Q_EXPORT_PLUGIN2(plg_optionsmanager, OptionsManager)
