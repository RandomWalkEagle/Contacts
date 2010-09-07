#include "pluginmanager.h"

#include <QDir>
#include <QTimer>
#include <QStack>
#include <QProcess>
#include <QLibrary>
#include <QFileInfo>
#include <QSettings>

//#include "windows.h"
//#include "WinCred.h"

#define ORGANIZATION_NAME           "Rambler"
#define APPLICATION_NAME            "Virtus"

#define FILE_PLUGINS_SETTINGS       "plugins.xml"

#define SVN_DATA_PATH               "DataPath"
#define SVN_LOCALE_NAME             "Locale"

#ifdef SVNINFO
#  include "svninfo.h"
#  define SVN_DATE                  ""
#else
#  define SVN_DATE                  ""
#  define SVN_REVISION              "0:0"
#endif

#if defined(Q_WS_WIN)
#  define ENV_APP_DATA              "APPDATA"
#  define DIR_APP_DATA              APPLICATION_NAME
#  define PATH_APP_DATA             ORGANIZATION_NAME"/"DIR_APP_DATA
#elif defined(Q_WS_X11)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              ".virtus"
#  define PATH_APP_DATA             DIR_APP_DATA
#elif defined(Q_WS_MAC)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              APPLICATION_NAME
#  define PATH_APP_DATA             "Library/Application Support/"DIR_APP_DATA
#endif

#if defined(Q_WS_WIN)
#  define LIB_PREFIX_SIZE           0
#else
#  define LIB_PREFIX_SIZE           3
#endif

//void startProcess(QString appName, QString appDir, QStringList args);

PluginManager::PluginManager(QApplication *AParent) : QObject(AParent)
{
	FQtTranslator = new QTranslator(this);
	FUtilsTranslator = new QTranslator(this);
	FLoaderTranslator = new QTranslator(this);
	connect(AParent,SIGNAL(aboutToQuit()),SLOT(onApplicationAboutToQuit()));
	connect(AParent,SIGNAL(commitDataRequest(QSessionManager &)),SLOT(onApplicationCommitDataRequested(QSessionManager &)));

	FUpdater = new Updater(this);
	connect(FUpdater, SIGNAL(forceUpdate()), this, SLOT(forceUpdate()));
	updateme = false;
}

PluginManager::~PluginManager()
{
	if(updateme)
	{
		QString uFileName = FUpdater->getUpdateFilename();
		//uFileName = "virtus.zip";
		// ��������� ��������� ������� �� ���������� ������
		QStringList args;
		args << qApp->applicationDirPath();
		args << "virtus.exe";
		args << uFileName;

		//startProcess("UpdateVirtus.exe", qApp->applicationDirPath() + "/update/", args);

		QString updatePath = QDir::tempPath() + "\\virtus\\update\\";
		QProcess* updateProcess = new QProcess();
		//updateProcess->setWorkingDirectory(qApp->applicationDirPath() + "/update");
		//updateProcess->startDetached(qApp->applicationDirPath() + "/update/UpdateVirtus.exe", args);
		updateProcess->setWorkingDirectory(updatePath);
		updateProcess->startDetached(updatePath + "UpdateVirtus.exe", args);
	}
}

//void startProcess(QString appName, QString appDir, QStringList args)
//{
//	QString fullName = appDir + appName;
//
//	std::wstring applicationName = fullName.toStdWString();
//
//	QString tmpStr;
//	foreach(QString str, args)
//	{
//		tmpStr += str + " ";
//	}
//	tmpStr = tmpStr.simplified();
//	std::wstring arguments = tmpStr.toStdWString();
//
//
//	CREDUI_INFO cui;
//	cui.cbSize = sizeof(CREDUI_INFO);
//	cui.hwndParent = NULL;
//	cui.pszMessageText = NULL;// QString("Enter login and password").toStdWString().c_str();
//	cui.pszCaptionText = NULL;//QString("Login Credentials").toStdWString().c_str();
//	cui.hbmBanner = NULL;
//
//	//TCHAR pszName[CREDUI_MAX_USERNAME_LENGTH] = _T("");
//	//TCHAR pszPwd[CREDUI_MAX_PASSWORD_LENGTH] = _T("");
//	//BOOL fSave = FALSE;
//
//	TCHAR szName[CREDUI_MAX_USERNAME_LENGTH]; szName[0]=0;
//	TCHAR szPwd[CREDUI_MAX_USERNAME_LENGTH]; szPwd[0]=0;
//	BOOL fSave = FALSE;
//	DWORD dwErr = 0;
//
//
//		//ERROR_INVALID_PARAMETER
//
//
//
//	dwErr = CredUIPromptForCredentials(
//		NULL,//&cui,
//		0,//&applicationName[0],//_T("Tool.exe"),
//		NULL,
//		0,
//		szName,
//		CREDUI_MAX_USERNAME_LENGTH+1,
//		szPwd,
//		CREDUI_MAX_PASSWORD_LENGTH+1,
//		&fSave,
//		CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS |
//		CREDUI_FLAGS_REQUEST_ADMINISTRATOR |
//		CREDUI_FLAGS_EXPECT_CONFIRMATION
//		);
//
//	if (!dwErr)
//	{
//		TCHAR szUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
//		TCHAR szDomainName[CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1];
//
//		DWORD dwError = CredUIParseUserName(szName,
//			szUserName,
//			CREDUI_MAX_USERNAME_LENGTH,
//			szDomainName,
//			CREDUI_MAX_DOMAIN_TARGET_LENGTH);
//
//		std::wstring strCommandLine(L"Tool.exe");
//
//		STARTUPINFO strctStartInfo;
//		ZeroMemory(&strctStartInfo, sizeof(STARTUPINFO));
//		strctStartInfo.cb = sizeof(STARTUPINFO);
//
//		PROCESS_INFORMATION strctProcInfo;
//		ZeroMemory(&strctProcInfo, sizeof(PROCESS_INFORMATION));
//
//		CreateProcessWithLogonW(
//			szUserName,
//			szDomainName,
//			szPwd,
//			LOGON_WITH_PROFILE,
//			&applicationName[0], //NULL,
//			&arguments[0], //&strCommandLine[0],
//			0,
//			NULL,
//			NULL,
//			&strctStartInfo,
//			&strctProcInfo);
//
//		CloseHandle(strctProcInfo.hThread);
//		CloseHandle(strctProcInfo.hProcess);
//
//		//SecureZeroMemory(pszName, sizeof(pszName));
//		//SecureZeroMemory(pszPwd, sizeof(pszPwd));
//	}
//}



QString PluginManager::version() const
{
	return CLIENT_VERSION;
}

QString PluginManager::revision() const
{
	static const QString rev = QString(SVN_REVISION).contains(':') ? QString(SVN_REVISION).split(':').value(1) : QString("0");
	return rev;
}

QDateTime PluginManager::revisionDate() const
{
	return QDateTime::fromString(SVN_DATE,"yyyy/MM/dd hh:mm:ss");
}

QString PluginManager::homePath() const
{
	return FDataPath;
}

void PluginManager::setHomePath(const QString &APath)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);
	settings.setValue(SVN_DATA_PATH, APath);
}

void PluginManager::setLocale(QLocale::Language ALanguage, QLocale::Country ACountry)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);
	if (ALanguage != QLocale::C)
		settings.setValue(SVN_LOCALE_NAME, QLocale(ALanguage, ACountry).name());
	else
		settings.remove(SVN_LOCALE_NAME);
}

IPlugin *PluginManager::pluginInstance(const QUuid &AUuid) const
{
	return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).plugin : NULL;
}

QList<IPlugin *> PluginManager::pluginInterface(const QString &AInterface) const
{
	QList<IPlugin *> plugins;
	if (!FPlugins.contains(AInterface))
	{
		foreach(PluginItem pluginItem, FPluginItems)
			if (AInterface.isEmpty() || pluginItem.plugin->instance()->inherits(AInterface.toLatin1().data()))
				FPlugins.insertMulti(AInterface,pluginItem.plugin);
	}
	return FPlugins.values(AInterface);
}

const IPluginInfo *PluginManager::pluginInfo(const QUuid &AUuid) const
{
	return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).info : NULL;
}

QList<QUuid> PluginManager::pluginDependencesOn(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QList<QUuid> plugins;
	QHash<QUuid, PluginItem>::const_iterator it = FPluginItems.constBegin();
	while (it != FPluginItems.constEnd())
	{
		if (!deepStack.contains(it.key()) && it.value().info->dependences.contains(AUuid))
		{
			plugins += pluginDependencesOn(it.key());
			plugins.append(it.key());
		}
		it++;
	}

	deepStack.pop();
	return plugins;
}

QList<QUuid> PluginManager::pluginDependencesFor(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QList<QUuid> plugins;
	if (FPluginItems.contains(AUuid))
	{
		foreach(QUuid depend, FPluginItems.value(AUuid).info->dependences)
		{
			if (!deepStack.contains(depend) && FPluginItems.contains(depend))
			{
				plugins.append(depend);
				plugins += pluginDependencesFor(depend);
			}
		}
	}

	deepStack.pop();
	return plugins;
}

QString PluginManager::styleSheet() const
{
	return FStyleSheet;
}

void PluginManager::setStyleSheet(const QString& newStyleSheet)
{
	FStyleSheet = newStyleSheet;
	qApp->setStyleSheet(QString(FStyleSheet).replace("%IMAGES_PATH%", qApp->applicationDirPath()+"/resources/stylesheets/shared/images"));
}

void PluginManager::quit()
{
	QTimer::singleShot(0,qApp,SLOT(quit()));
}

void PluginManager::restart()
{
	onApplicationAboutToQuit();
	loadSettings();
	loadPlugins();
	if (initPlugins())
	{
		saveSettings();
		createMenuActions();
		startPlugins();
		FUpdater->checkUpdate(); // ����� �������� ����������
	}
	else
		QTimer::singleShot(0,this,SLOT(restart()));
}

void PluginManager::forceUpdate()
{

}

void PluginManager::loadSettings()
{
	QStringList args = qApp->arguments();
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);

	QLocale locale(QLocale::C,  QLocale::AnyCountry);
	if (args.contains(CLO_LOCALE))
	{
		locale = QLocale(args.value(args.indexOf(CLO_LOCALE)+1));
	}
	if (locale.language()==QLocale::C && !settings.value(SVN_LOCALE_NAME).toString().isEmpty())
	{
		locale = QLocale(settings.value(SVN_LOCALE_NAME).toString());
	}
	if (locale.language() == QLocale::C)
	{
		locale = QLocale::system();
	}
	QLocale::setDefault(locale);

	FDataPath = QString::null;
	if (args.contains(CLO_APP_DATA_DIR))
	{
		QDir dir(args.value(args.indexOf(CLO_APP_DATA_DIR)+1));
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull())
	{
		QDir dir(qApp->applicationDirPath());
		if (dir.exists(DIR_APP_DATA) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull() && !settings.value(SVN_DATA_PATH).toString().isEmpty())
	{
		QDir dir(settings.value(SVN_DATA_PATH).toString());
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull())
	{
		foreach(QString env, QProcess::systemEnvironment())
		{
			if (env.startsWith(ENV_APP_DATA"="))
			{
				QDir dir(env.split("=").value(1));
				if (dir.exists() && (dir.exists(PATH_APP_DATA) || dir.mkpath(PATH_APP_DATA)) && dir.cd(PATH_APP_DATA))
					FDataPath = dir.absolutePath();
			}
		}
	}
	if (FDataPath.isNull())
	{
		QDir dir(QDir::homePath());
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	FileStorage::setResourcesDirs(QList<QString>() << qApp->applicationDirPath()+"/"RESOURCES_DIR << FDataPath+"/resources");

	FPluginsSetup.clear();
	QDir homeDir(FDataPath);
	QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
	if (file.exists() && file.open(QFile::ReadOnly))
		FPluginsSetup.setContent(&file,true);
	file.close();

	if (FPluginsSetup.documentElement().tagName() != "plugins")
	{
		FPluginsSetup.clear();
		FPluginsSetup.appendChild(FPluginsSetup.createElement("plugins"));
	}

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(qApp, STS_PLUGINMANAGER_APPLICATION);

	QFile sheetFile(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->fileFullName(STS_PLUGINMANAGER_APPLICATION));
	sheetFile.open(QFile::ReadOnly);
	FStyleSheet = QString::fromUtf8(sheetFile.readAll());
}

void PluginManager::saveSettings()
{
	if (!FPluginsSetup.documentElement().isNull())
	{
		QDir homeDir(FDataPath);
		QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			file.write(FPluginsSetup.toString(3).toUtf8());
			file.flush();
			file.close();
		}
	}
}

void PluginManager::loadPlugins()
{
	QDir dir(QApplication::applicationDirPath());
	if (dir.cd(PLUGINS_DIR))
	{
		QString tsDir = QDir::cleanPath(QDir(QApplication::applicationDirPath()).absoluteFilePath(TRANSLATIONS_DIR "/" + QLocale().name()));
		loadCoreTranslations(tsDir);

		QStringList files = dir.entryList(QDir::Files);
		removePluginsInfo(files);

		foreach (QString file, files)
		{
			if (QLibrary::isLibrary(file) && isPluginEnabled(file))
			{
				QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(file),this);
				if (loader->load())
				{
					IPlugin *plugin = qobject_cast<IPlugin *>(loader->instance());
					if (plugin)
					{
						plugin->instance()->setParent(loader);
						QUuid uid = plugin->pluginUuid();
						if (!FPluginItems.contains(uid))
						{
							PluginItem pluginItem;
							pluginItem.plugin = plugin;
							pluginItem.loader = loader;
							pluginItem.info = new IPluginInfo;
							pluginItem.translator =  NULL;

							QTranslator *translator = new QTranslator(loader);
							QString tsFile = file.mid(LIB_PREFIX_SIZE,file.lastIndexOf('.')-LIB_PREFIX_SIZE);
							if (translator->load(tsFile,tsDir))
							{
								qApp->installTranslator(translator);
								pluginItem.translator = translator;
							}
							else
								delete translator;

							plugin->pluginInfo(pluginItem.info);
							savePluginInfo(file, pluginItem.info).setAttribute("uuid", uid.toString());

							FPluginItems.insert(uid,pluginItem);
						}
						else
						{
							savePluginError(file, tr("Duplicate plugin uuid"));
							delete loader;
						}
					}
					else
					{
						savePluginError(file, tr("Wrong plugin interface"));
						delete loader;
					}
				}
				else
				{
					savePluginError(file, loader->errorString());
					delete loader;
				}
			}
		}

		QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
		while (it!=FPluginItems.constEnd())
		{
			QUuid puid = it.key();
			if (!checkDependences(puid))
			{
				unloadPlugin(puid, tr("Dependences not found"));
				it = FPluginItems.constBegin();
			}
			else if (!checkConflicts(puid))
			{
				foreach(QUuid uid, getConflicts(puid)) {
					unloadPlugin(uid, tr("Conflict with plugin %1").arg(puid.toString())); }
				it = FPluginItems.constBegin();
			}
			else
			{
				it++;
			}
		}
	}
	else
	{
		quit();
	}
}

bool PluginManager::initPlugins()
{
	bool initOk = true;
	QMultiMap<int,IPlugin *> pluginOrder;
	QHash<QUuid, PluginItem>::const_iterator it = FPluginItems.constBegin();
	while (initOk && it!=FPluginItems.constEnd())
	{
		int initOrder = PIO_DEFAULT;
		IPlugin *plugin = it.value().plugin;
		if (plugin->initConnections(this,initOrder))
		{
			pluginOrder.insertMulti(initOrder,plugin);
			it++;
		}
		else
		{
			initOk = false;
			FBlockedPlugins.append(QFileInfo(it.value().loader->fileName()).fileName());
			unloadPlugin(it.key(), tr("Initialization failed"));
		}
	}

	if (initOk)
	{
		foreach(IPlugin *plugin, pluginOrder)
			plugin->initObjects();

		foreach(IPlugin *plugin, pluginOrder)
			plugin->initSettings();
	}

	return initOk;
}

void PluginManager::startPlugins()
{
	foreach(PluginItem pluginItem, FPluginItems)
		pluginItem.plugin->startPlugin();
}







void PluginManager::removePluginItem(const QUuid &AUuid, const QString &AError)
{
	if (FPluginItems.contains(AUuid))
	{
		PluginItem pluginItem = FPluginItems.take(AUuid);
		if (!AError.isEmpty())
		{
			savePluginError(QFileInfo(pluginItem.loader->fileName()).fileName(), AError);
		}
		if (pluginItem.translator)
		{
			qApp->removeTranslator(pluginItem.translator);
		}
		delete pluginItem.translator;
		delete pluginItem.info;
		delete pluginItem.loader;
	}
}

void PluginManager::unloadPlugin(const QUuid &AUuid, const QString &AError)
{
	if (FPluginItems.contains(AUuid))
	{
		foreach(QUuid uid, pluginDependencesOn(AUuid))
			removePluginItem(uid, AError);
		removePluginItem(AUuid, AError);
		FPlugins.clear();
	}
}

bool PluginManager::checkDependences(const QUuid AUuid) const
{
	if (FPluginItems.contains(AUuid))
	{
		foreach(QUuid depend, FPluginItems.value(AUuid).info->dependences)
		{
			if (!FPluginItems.contains(depend))
			{
				bool found = false;
				QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
				while (!found && it!=FPluginItems.constEnd())
				{
					found = it.value().info->implements.contains(depend);
					it++;
				}
				if (!found)
					return false;
			}
		}
	}
	return true;
}

bool PluginManager::checkConflicts(const QUuid AUuid) const
{
	if (FPluginItems.contains(AUuid))
	{
		foreach (QUuid conflict, FPluginItems.value(AUuid).info->conflicts)
		{
			if (!FPluginItems.contains(conflict))
			{
				foreach(PluginItem pluginItem, FPluginItems)
					if (pluginItem.info->implements.contains(conflict))
						return false;
			}
			else
				return false;
		}
	}
	return true;
}

QList<QUuid> PluginManager::getConflicts(const QUuid AUuid) const
{
	QSet<QUuid> plugins;
	if (FPluginItems.contains(AUuid))
	{
		foreach (QUuid conflict, FPluginItems.value(AUuid).info->conflicts)
		{
			QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
			while (it!=FPluginItems.constEnd())
			{
				if (it.key()==conflict || it.value().info->implements.contains(conflict))
					plugins+=conflict;
				it++;
			}
		}
	}
	return plugins.toList();
}

void PluginManager::loadCoreTranslations(const QString &ADir)
{
	if (FLoaderTranslator->load("virtus",ADir))
		qApp->installTranslator(FLoaderTranslator);

	if (FUtilsTranslator->load("virtusutils",ADir))
		qApp->installTranslator(FUtilsTranslator);

	if (FQtTranslator->load("qt_"+QLocale().name(),ADir))
		qApp->installTranslator(FQtTranslator);
}

bool PluginManager::isPluginEnabled(const QString &AFile) const
{
	return !FBlockedPlugins.contains(AFile) && FPluginsSetup.documentElement().firstChildElement(AFile).attribute("enabled","true") == "true";
}

QDomElement PluginManager::savePluginInfo(const QString &AFile, const IPluginInfo *AInfo)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement(AFile);
	if (pluginElem.isNull())
		pluginElem = FPluginsSetup.firstChildElement("plugins").appendChild(FPluginsSetup.createElement(AFile)).toElement();

	QDomElement nameElem = pluginElem.firstChildElement("name");
	if (nameElem.isNull())
	{
		nameElem = pluginElem.appendChild(FPluginsSetup.createElement("name")).toElement();
		nameElem.appendChild(FPluginsSetup.createTextNode(AInfo->name));
	}
	else
		nameElem.firstChild().toCharacterData().setData(AInfo->name);

	QDomElement descElem = pluginElem.firstChildElement("desc");
	if (descElem.isNull())
	{
		descElem = pluginElem.appendChild(FPluginsSetup.createElement("desc")).toElement();
		descElem.appendChild(FPluginsSetup.createTextNode(AInfo->description));
	}
	else
		descElem.firstChild().toCharacterData().setData(AInfo->description);

	QDomElement versionElem = pluginElem.firstChildElement("version");
	if (versionElem.isNull())
	{
		versionElem = pluginElem.appendChild(FPluginsSetup.createElement("version")).toElement();
		versionElem.appendChild(FPluginsSetup.createTextNode(AInfo->version));
	}
	else
		versionElem.firstChild().toCharacterData().setData(AInfo->version);

	pluginElem.removeChild(pluginElem.firstChildElement("depends"));
	if (!AInfo->dependences.isEmpty())
	{
		QDomElement dependsElem = pluginElem.appendChild(FPluginsSetup.createElement("depends")).toElement();
		foreach(QUuid uid, AInfo->dependences)
			dependsElem.appendChild(FPluginsSetup.createElement("uuid")).appendChild(FPluginsSetup.createTextNode(uid.toString()));
	}

	pluginElem.removeChild(pluginElem.firstChildElement("error"));

	return pluginElem;
}

void PluginManager::savePluginError(const QString &AFile, const QString &AError)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement(AFile);
	if (pluginElem.isNull())
		pluginElem = FPluginsSetup.firstChildElement("plugins").appendChild(FPluginsSetup.createElement(AFile)).toElement();

	QDomElement errorElem = pluginElem.firstChildElement("error");
	if (AError.isEmpty())
	{
		pluginElem.removeChild(errorElem);
	}
	else if (errorElem.isNull())
	{
		errorElem = pluginElem.appendChild(FPluginsSetup.createElement("error")).toElement();
		errorElem.appendChild(FPluginsSetup.createTextNode(AError));
	}
	else
	{
		errorElem.firstChild().toCharacterData().setData(AError);
	}
}

void PluginManager::removePluginsInfo(const QStringList &ACurFiles)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement();
	while (!pluginElem.isNull())
	{
		if (!ACurFiles.contains(pluginElem.tagName()))
		{
			QDomElement oldElem = pluginElem;
			pluginElem = pluginElem.nextSiblingElement();
			oldElem.parentNode().removeChild(oldElem);
		}
		else
			pluginElem = pluginElem.nextSiblingElement();
	}
}

void PluginManager::createMenuActions()
{
	IPlugin *plugin = pluginInterface("IMainWindowPlugin").value(0);
	IMainWindowPlugin *mainWindowPligin = plugin!=NULL ? qobject_cast<IMainWindowPlugin *>(plugin->instance()) : NULL;

	plugin = pluginInterface("ITrayManager").value(0);
	ITrayManager *trayManager = plugin!=NULL ? qobject_cast<ITrayManager *>(plugin->instance()) : NULL;

	//plugin = pluginInterface("IAccountManager").value(0);
	//IAccountManager *accountManager = plugin != NULL ? qobject_cast<IAccountManager *>(plugin->instance()) : NULL;
	//IAccount *account = accountManager->accounts().value(0);

	//plugin = pluginInterface("IVCardPlugin").value(0);
	//IVCardPlugin *vCardPlugin = plugin != NULL ? qobject_cast<IVCardPlugin *>(plugin->instance()) : NULL;


	if (mainWindowPligin || trayManager)
	{
		Action *pluginsDialog = new Action(mainWindowPligin!=NULL ? mainWindowPligin->instance() : trayManager->instance());
		pluginsDialog->setIcon(RSR_STORAGE_MENUICONS, MNI_PLUGINMANAGER_SETUP);
		connect(pluginsDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
		pluginsDialog->setText(tr("Setup plugins"));

		if (mainWindowPligin)
		{
			Action *aboutQt = new Action(mainWindowPligin->mainWindow()->mainMenu());
			aboutQt->setText(tr("About Qt"));
			aboutQt->setIcon(RSR_STORAGE_MENUICONS,MNI_PLUGINMANAGER_ABOUT_QT);
			connect(aboutQt,SIGNAL(triggered()),QApplication::instance(),SLOT(aboutQt()));
			mainWindowPligin->mainWindow()->mainMenu()->addAction(aboutQt,AG_MMENU_PLUGINMANAGER_ABOUT);

			Action *about = new Action(mainWindowPligin->mainWindow()->mainMenu());
			about->setText(tr("About the program"));
			about->setIcon(RSR_STORAGE_MENUICONS,MNI_PLUGINMANAGER_ABOUT);
			connect(about,SIGNAL(triggered()),SLOT(onShowAboutBoxDialog()));
			mainWindowPligin->mainWindow()->mainMenu()->addAction(about,AG_MMENU_PLUGINMANAGER_ABOUT);

			// �����
			Action *comments = new Action(mainWindowPligin->mainWindow()->mainMenu());
			comments->setText(tr("User Comments"));
			comments->setIcon(RSR_STORAGE_MENUICONS, MNI_PLUGINMANAGER_ABOUT);
			connect(comments,SIGNAL(triggered()),SLOT(onShowCommentsDialog()));
			mainWindowPligin->mainWindow()->mainMenu()->addAction(comments, AG_MMENU_PLUGINMANAGER_COMMENTS);





			mainWindowPligin->mainWindow()->mainMenu()->addAction(pluginsDialog, AG_MMENU_PLUGINMANAGER_SETUP, true);
		}

		//if (trayManager)
		//	trayManager->addAction(pluginsDialog,AG_TMTM_PLUGINMANAGER,true);
	}
	else
		onShowSetupPluginsDialog(false);
}

void PluginManager::onApplicationAboutToQuit()
{
	if (!FPluginsDialog.isNull())
		FPluginsDialog->reject();

	if (!FAboutDialog.isNull())
		FAboutDialog->reject();

	if (!FCommentDialog.isNull())
		FCommentDialog->reject();

	emit aboutToQuit();

	foreach(QUuid uid, FPluginItems.keys())
		unloadPlugin(uid);

	QCoreApplication::removeTranslator(FQtTranslator);
	QCoreApplication::removeTranslator(FUtilsTranslator);
	QCoreApplication::removeTranslator(FLoaderTranslator);
}

void PluginManager::onApplicationCommitDataRequested(QSessionManager &AManager)
{
	Q_UNUSED(AManager);
	onApplicationAboutToQuit();
}

void PluginManager::onShowSetupPluginsDialog(bool)
{
	if (FPluginsDialog.isNull())
	{
		FPluginsDialog = new SetupPluginsDialog(this,FPluginsSetup,NULL);
		connect(FPluginsDialog, SIGNAL(accepted()),SLOT(onSetupPluginsDialogAccepted()));
	}
	FPluginsDialog->show();
	WidgetManager::raiseWidget(FPluginsDialog);
	FPluginsDialog->activateWindow();
}

void PluginManager::onSetupPluginsDialogAccepted()
{
	saveSettings();
}

void PluginManager::onShowAboutBoxDialog()
{
	if (FAboutDialog.isNull())
		FAboutDialog = new AboutBox(this, FUpdater);

	FAboutDialog->show();
	WidgetManager::raiseWidget(FAboutDialog);
	FAboutDialog->activateWindow();
}

void PluginManager::onShowCommentsDialog()
{
	if (FCommentDialog.isNull())
		FCommentDialog = new CommentDialog(this);
	FCommentDialog->show();
	WidgetManager::raiseWidget(FCommentDialog);
	FCommentDialog->activateWindow();
}

void PluginManager::updateMe(QString message, bool state)
{
	updateme = state;
	if(updateme)
	{
		quit();
	}
}
