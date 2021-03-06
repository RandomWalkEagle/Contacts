#include "pluginmanager.h"

#include <QTimer>
#include <QStack>
#include <QProcess>
#include <QLibrary>
#include <QFileInfo>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QFontDatabase>

#include <definitions/fonts.h>
#include <definitions/resources.h>
#include <interfaces/imainwindow.h>
#include <interfaces/isystemintegration.h>
#include <utils/log.h>
#include <utils/options.h>
#include <utils/statistics.h>
#include <utils/networking.h>
#include <utils/systemmanager.h>
#include <utils/custominputdialog.h>
#include <utils/customborderstorage.h>

#define DELAYED_QUIT_TIMEOUT        5000
#define DELAYED_COMMIT_TIMEOUT      2000

#define FILE_PLUGINS_SETTINGS       "plugins.xml"

#define SVN_DATA_PATH               "DataPath"
#define SVN_LOCALE_NAME             "Locale"
#define SVN_BORDERS_ENABLED         "BordersEnabled"

#ifdef SVNINFO
#  include "svninfo.h"
#  define SVN_DATE                  ""
#else
#  define SVN_DATE                  ""
#  define SVN_REVISION              "0:0"
#endif

#define DIR_LOGS                    "logs"
#define DIR_COOKIES                 "cookies"
#define FILE_RAMBLER_USAGE          "ramusage.cnt"
#if defined(Q_WS_WIN)
#  define ENV_APP_DATA              "APPDATA"
#  define DIR_APP_DATA              CLIENT_NAME
#  define PATH_APP_DATA             CLIENT_ORGANIZATION_NAME"/"DIR_APP_DATA
#elif defined(Q_WS_X11)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              ".ramblercontacts"
#  define PATH_APP_DATA             DIR_APP_DATA
#elif defined(Q_WS_MAC)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              CLIENT_NAME
#  define PATH_APP_DATA             "Library/Application Support/"DIR_APP_DATA
#endif

#if defined(Q_WS_WIN)
#  define LIB_PREFIX_SIZE           0
#else
#  define LIB_PREFIX_SIZE           3
#endif

#define COUNTER_LOADED_OPTION       "statistics/ramblerusageloaded"


PluginManager::PluginManager(QApplication *AParent) : QObject(AParent)
{
	FShutdownKind = SK_START;
	FShutdownDelayCount = 0;

	FQtTranslator = new QTranslator(this);
	FUtilsTranslator = new QTranslator(this);
	FLoaderTranslator = new QTranslator(this);

	FShutdownTimer.setSingleShot(true);
	connect(&FShutdownTimer,SIGNAL(timeout()),SLOT(onShutdownTimerTimeout()));

	connect(AParent,SIGNAL(aboutToQuit()),SLOT(onApplicationAboutToQuit()));
	connect(AParent,SIGNAL(commitDataRequest(QSessionManager &)),SLOT(onApplicationCommitDataRequested(QSessionManager &)));
}

PluginManager::~PluginManager()
{

}

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

bool PluginManager::isShutingDown() const
{
	return FShutdownKind != SK_WORK;
}

QString PluginManager::homePath() const
{
	return FDataPath;
}

void PluginManager::setHomePath(const QString &APath)
{
	Options::setGlobalValue(SVN_DATA_PATH, APath);
}

void PluginManager::setLocale(QLocale::Language ALanguage, QLocale::Country ACountry)
{
	if (ALanguage != QLocale::C)
		Options::setGlobalValue(SVN_LOCALE_NAME, QLocale(ALanguage, ACountry).name());
	else
		Options::removeGlobalValue(SVN_LOCALE_NAME);
}

IPlugin *PluginManager::pluginInstance(const QUuid &AUuid) const
{
	return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).plugin : NULL;
}

QList<IPlugin *> PluginManager::pluginInterface(const QString &AInterface) const
{
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

void PluginManager::showFeedbackDialog()
{
	onShowCommentsDialog();
}

void PluginManager::quit()
{
	if (FShutdownKind == SK_WORK)
	{
		LogDetail(QString("[PluginManager] Quit started"));
		FShutdownKind = SK_QUIT;
		startShutdown();
	}
}

void PluginManager::restart()
{
	if (FShutdownKind == SK_START)
	{
		finishShutdown();
	}
	else if (FShutdownKind == SK_WORK)
	{
		FShutdownKind = SK_RESTART;
		startShutdown();
	}
}

void PluginManager::delayShutdown()
{
	if (FShutdownKind != SK_WORK)
	{
		FShutdownDelayCount++;
	}
}

void PluginManager::continueShutdown()
{
	if (FShutdownKind != SK_WORK)
	{
		FShutdownDelayCount--;
		if (FShutdownDelayCount <= 0)
			FShutdownTimer.start(0);
	}
}

void PluginManager::shutdownRequested()
{
	// TODO: ask user to confirm quit operation
	static int i = 0;
	if (!i++)
	{
		QMessageBox * mb = new QMessageBox(tr("Rambler.Contacts updates ready"), tr("Updates are ready. Do you want to restart Rambler.Contacts now?"), QMessageBox::Question, QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
		mb->setWindowModality(Qt::ApplicationModal);
		connect(mb, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(onMessageBoxButtonClicked(QAbstractButton*)));
		WidgetManager::showActivateRaiseWindow(mb);
	}
}

void PluginManager::showMainWindow()
{
	IPlugin * plugin = pluginInstance(MAINWINDOW_UUID);
	if (plugin)
	{
		IMainWindowPlugin *mainWindowPlugin = qobject_cast<IMainWindowPlugin*>(plugin->instance());
		if (mainWindowPlugin)
		{
			mainWindowPlugin->showMainWindow();
		}
	}
}

void PluginManager::loadSettings()
{
	QStringList args = qApp->arguments();

	QLocale locale(QLocale::C,  QLocale::AnyCountry);
	if (args.contains(CLO_LOCALE))
	{
		locale = QLocale(args.value(args.indexOf(CLO_LOCALE)+1));
	}
	if (locale.language()==QLocale::C && !Options::globalValue(SVN_LOCALE_NAME).toString().isEmpty())
	{
		locale = QLocale(Options::globalValue(SVN_LOCALE_NAME).toString());
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
	if (FDataPath.isNull() && !Options::globalValue(SVN_DATA_PATH).toString().isEmpty())
	{
		QDir dir(Options::globalValue(SVN_DATA_PATH).toString());
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

	Log::setLogTypes(0);
#ifdef LOG_ENABLED
	QDir logDir(FDataPath);
	if (logDir.exists() && (logDir.exists(DIR_LOGS) || logDir.mkpath(DIR_LOGS)) && logDir.cd(DIR_LOGS))
	{
		int types = args.contains(CLO_LOG_TYPES) ? args.value(args.indexOf(CLO_LOG_TYPES)+1).toInt() : Log::Error|Log::Warning|Log::Detaile;
		Log::setLogTypes(types);
		Log::setLogFormat(Log::Simple);
		Log::setLogPath(logDir.absolutePath());
		LogDetail(QString("[PluginManager] %1 v%2 %3").arg(CLIENT_NAME, CLIENT_VERSION, SystemManager::systemOSVersion()));
	}
#endif

	// Borders
#ifndef Q_WS_MAC
	CustomBorderStorage::setBordersEnabled(Options::globalValue(SVN_BORDERS_ENABLED,true).toBool());
#else
	CustomBorderStorage::setBordersEnabled(Options::globalValue(SVN_BORDERS_ENABLED,false).toBool());
#endif

	QDir cookiesDir(FDataPath);
	if (cookiesDir.exists() && (cookiesDir.exists(DIR_COOKIES) || cookiesDir.mkpath(DIR_COOKIES)) && cookiesDir.cd(DIR_COOKIES))
	{
		Networking::setCookiePath(cookiesDir.absolutePath());
	}

	// TNS Counter
	Statistics::instance()->addCounter("http://www.tns-counter.ru/V13a****rambler_ru/ru/CP1251/tmsec=rambler_contacts-application/", Statistics::Image, -1); // only once
	// Rambler Counter
	if (!Options::globalValue(COUNTER_LOADED_OPTION, false).toBool())
	{
		QFile counterFile(qApp->applicationDirPath() + "/"FILE_RAMBLER_USAGE);
		if (counterFile.exists() && counterFile.open(QFile::ReadOnly))
		{
			QStringList counterData = QString::fromUtf8(counterFile.readAll()).split(';');
			QString id = counterData.value(0, "self");
			if (id.isEmpty())
				id = "self";

			bool ok = true;
			int interval = counterData.value(1, "default").toInt(&ok);
			if (!ok)
				interval = 24 * 60 * 60 * 1000; // 24 hours

			Statistics::instance()->addCounter(QString("http://www.rambler.ru/r/p?event=usage&rpid=%1&appid=contact").arg(id), Statistics::Image, interval);
			Options::setGlobalValue(COUNTER_LOADED_OPTION, true);
			if (!counterFile.remove())
				LogError(QString("[PluginManager::loadSettings]: Failed to remove file %1!").arg(counterFile.fileName()));
		}
	}

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

	FileStorage::setResourcesDirs(FileStorage::resourcesDirs()
		<< (QDir::isAbsolutePath(RESOURCES_DIR) ? RESOURCES_DIR : qApp->applicationDirPath()+"/"+RESOURCES_DIR)
		<< FDataPath+"/resources");

	QPalette pal = QApplication::palette();
	pal.setColor(QPalette::Link, StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_LINK_COLOR));
	QApplication::setPalette(pal);

#ifdef Q_WS_MAC
	//qApp->setWindowIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MAINWINDOW_LOGO512));
#elif defined Q_WS_WIN
	if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
		qApp->setWindowIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MAINWINDOW_LOGO48));
	else
		qApp->setWindowIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MAINWINDOW_LOGO16));
#else
	qApp->setWindowIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MAINWINDOW_LOGO64));
#endif

	FileStorage * fontStorage = FileStorage::staticStorage(RSR_STORAGE_FONTS);
	QString fontFile = fontStorage->fileFullName(FNT_SEGOEUI);
	QFontDatabase::addApplicationFont(fontFile);
	fontFile = fontStorage->fileFullName(FNT_SEGOEUI_ITALIC);
	QFontDatabase::addApplicationFont(fontFile);
	fontFile = fontStorage->fileFullName(FNT_SEGOEUI_BOLD);
	QFontDatabase::addApplicationFont(fontFile);
	fontFile = fontStorage->fileFullName(FNT_SEGOEUI_ITALIC_BOLD);
	QFontDatabase::addApplicationFont(fontFile);
	QFontDatabase fontDB;
	QFont segoe = fontDB.font("Segoe UI", "", 12);
	QApplication::setFont(segoe);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(qApp, STS_PLUGINMANAGER_APPLICATION);
}

void PluginManager::saveSettings()
{
	Options::setGlobalValue(SVN_BORDERS_ENABLED, CustomBorderStorage::isBordersEnabled());

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

	Statistics::release();
}

void PluginManager::loadPlugins()
{
	QDir pluginsDir(QApplication::applicationDirPath());
	if (pluginsDir.cd(PLUGINS_DIR))
	{
		LogDetail(QString("[PluginManager] Loading plugins from '%1'").arg(pluginsDir.absolutePath()));

		QString localeName = QLocale().name();
		QDir tsDir(QApplication::applicationDirPath());
		tsDir.cd(TRANSLATIONS_DIR);
		loadCoreTranslations(tsDir,localeName);

		QStringList files = pluginsDir.entryList(QDir::Files);
		removePluginsInfo(files);

		foreach (QString file, files)
		{
			if (QLibrary::isLibrary(file) && isPluginEnabled(file))
			{
				QPluginLoader *loader = new QPluginLoader(pluginsDir.absoluteFilePath(file),this);
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
							if (translator->load(tsFile,tsDir.absoluteFilePath(localeName)) || translator->load(tsFile,tsDir.absoluteFilePath(localeName.left(2))))
							{
								qApp->installTranslator(translator);
								pluginItem.translator = translator;
							}
							else
							{
								delete translator;
							}

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
		LogError(QString("[PluginManager] Could not find plugins directory: '%1'").arg(PLUGINS_DIR));
		ReportError("NO-PLUGINS-DIR",QString("[PluginManager] Could not find plugins directory: '%1'").arg(PLUGINS_DIR),false);
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

void PluginManager::startShutdown()
{
	FShutdownTimer.start(DELAYED_QUIT_TIMEOUT);
	delayShutdown();
	emit shutdownStarted();
	closeTopLevelWidgets();
	continueShutdown();
}

void PluginManager::finishShutdown()
{
	FShutdownTimer.stop();
	switch (FShutdownKind)
	{
	case SK_RESTART:
		onApplicationAboutToQuit();
	case SK_START:
		FShutdownKind = SK_WORK;
		FShutdownDelayCount = 0;

		loadSettings();
		loadPlugins();
		if (initPlugins())
		{
			createMenuActions();
			startPlugins();
			FBlockedPlugins.clear();
		}
		else
		{
			QTimer::singleShot(0,this,SLOT(restart()));
		}
		break;
	case SK_QUIT:
		QTimer::singleShot(0,qApp,SLOT(quit()));
		break;
	default:
		break;
	}
}

void PluginManager::closeTopLevelWidgets()
{
	foreach(QWidget *widget, QApplication::topLevelWidgets())
		widget->close();
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

void PluginManager::loadCoreTranslations(const QDir &ADir, const QString &ALocaleName)
{
	if (FQtTranslator->load("qt_"+ALocaleName,ADir.absoluteFilePath(ALocaleName)) || FQtTranslator->load("qt_"+ALocaleName,ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FQtTranslator);
	else if (FQtTranslator->load("qt_"+QLocale().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		qApp->installTranslator(FQtTranslator);

	if (FLoaderTranslator->load("ramblercontacts",ADir.absoluteFilePath(ALocaleName)) || FLoaderTranslator->load("ramblercontacts",ADir.absoluteFilePath(ALocaleName.left(2))))
	{
		LogDetail("[PluginManager::loadCoreTranslations] loaded ramblercontacts.qm");
		qApp->installTranslator(FLoaderTranslator);
	}
	else
		LogError("[PluginManager::loadCoreTranslations] failed to load ramblercontacts.qm!");

	if (FUtilsTranslator->load("ramblercontactsutils",ADir.absoluteFilePath(ALocaleName)) || FUtilsTranslator->load("ramblercontactsutils",ADir.absoluteFilePath(ALocaleName.left(2))))
	{
		LogDetail("[PluginManager::loadCoreTranslations] loaded ramblercontactsutils.qm");
		qApp->installTranslator(FUtilsTranslator);
	}
	else
		LogError("[PluginManager::loadCoreTranslations] failed to load ramblercontactsutils.qm!");
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
	LogError(QString("[PluginManager] Failed to load plugin '%1': %2").arg(AFile, AError));
	ReportError("LOAD-PLUGIN-ERROR",QString("[PluginManager] Failed to load plugin '%1': %2").arg(AFile, AError),false);

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
	IMainWindowPlugin *mainWindowPligin = plugin ? qobject_cast<IMainWindowPlugin *>(plugin->instance()) : NULL;

	if (mainWindowPligin)
	{
		Action *comments = new Action(mainWindowPligin->mainWindow()->mainMenu());
		comments->setText(tr("Leave your feedback..."));
		connect(comments,SIGNAL(triggered()),SLOT(onShowCommentsDialog()));
		mainWindowPligin->mainWindow()->mainMenu()->addAction(comments, AG_MMENU_PLUGINMANAGER_COMMENTS);

		Action *about = new Action(mainWindowPligin->mainWindow()->mainMenu());
		about->setText(tr("About the program"));
		connect(about,SIGNAL(triggered()),SLOT(onShowAboutBoxDialog()));
		mainWindowPligin->mainWindow()->mainMenu()->addAction(about,AG_MMENU_PLUGINMANAGER_ABOUT);

#ifdef DEBUG_ENABLED
		Action *pluginsDialog = new Action(mainWindowPligin->mainWindow()->mainMenu());
		pluginsDialog->setText(tr("Setup plugins"));
		connect(pluginsDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
		mainWindowPligin->mainWindow()->mainMenu()->addAction(pluginsDialog, AG_MMENU_PLUGINMANAGER_SETUP, true);
#endif
	}

	plugin = pluginInterface("ISystemIntegration").value(0);
	ISystemIntegration *systemIntegrationPligin = plugin ? qobject_cast<ISystemIntegration *>(plugin->instance()) : NULL;

	if (systemIntegrationPligin)
	{
		Action *about = new Action();
		about->setText("about.*");
		connect(about,SIGNAL(triggered()),SLOT(onShowAboutBoxDialog()));
		systemIntegrationPligin->addAction(ISystemIntegration::FileRole, about);

#ifdef DEBUG_ENABLED
		Action *pluginsDialog = new Action;
		pluginsDialog->setText(tr("Setup plugins"));
		connect(pluginsDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
		systemIntegrationPligin->addAction(ISystemIntegration::WindowRole, pluginsDialog, 510);
#endif
	}

}

void PluginManager::onApplicationAboutToQuit()
{
	LogDetail(QString("[PluginManager] Application about to quit"));
	emit aboutToQuit();

	if (!FPluginsDialog.isNull())
		FPluginsDialog->reject();

	if (!FAboutDialog.isNull())
		FAboutDialog->reject();

	if (!FCommentDialog.isNull())
		FCommentDialog->reject();

	foreach(QUuid uid, FPluginItems.keys())
		unloadPlugin(uid);

	saveSettings();

	QCoreApplication::removeTranslator(FQtTranslator);
	QCoreApplication::removeTranslator(FUtilsTranslator);
	QCoreApplication::removeTranslator(FLoaderTranslator);
}

void PluginManager::onApplicationCommitDataRequested(QSessionManager &AManager)
{
	Q_UNUSED(AManager);
	FShutdownKind = SK_QUIT;
	startShutdown();
	QDateTime stopTime = QDateTime::currentDateTime().addMSecs(DELAYED_COMMIT_TIMEOUT);
	while (stopTime>QDateTime::currentDateTime() && FShutdownDelayCount>0)
		QApplication::processEvents();
	finishShutdown();
	qApp->quit();
}

void PluginManager::onShowSetupPluginsDialog(bool)
{
	if (FPluginsDialog.isNull())
	{
		FPluginsDialog = new SetupPluginsDialog(this,FPluginsSetup,NULL);
		connect(FPluginsDialog, SIGNAL(accepted()),SLOT(onSetupPluginsDialogAccepted()));
	}
	WidgetManager::showActivateRaiseWindow(FPluginsDialog->window());
}

void PluginManager::onSetupPluginsDialogAccepted()
{
	saveSettings();
}

void PluginManager::onShowAboutBoxDialog()
{
	if (FAboutDialog.isNull())
		FAboutDialog = new AboutBox(this);
	WidgetManager::showActivateRaiseWindow(FAboutDialog->window());
	WidgetManager::alignWindow(FAboutDialog->window(),Qt::AlignCenter);
}

void PluginManager::onShowCommentsDialog()
{
	if (FCommentDialog.isNull())
		FCommentDialog = new CommentDialog(this);
	IAccountManager * accManager = qobject_cast<IAccountManager*>(pluginInterface("IAccountManager").value(0)->instance());
	if (accManager)
	{
		if (accManager->accounts().count())
		{
			WidgetManager::showActivateRaiseWindow(FCommentDialog->window());
			WidgetManager::alignWindow(FCommentDialog->window(),Qt::AlignCenter);
		}
		else
		{
			CustomInputDialog * cid = new CustomInputDialog(CustomInputDialog::Info);
			cid->setAttribute(Qt::WA_DeleteOnClose, true);
			cid->setCaptionText(tr("Login first"));
			cid->setInfoText(tr("To send a feedback you should login first!"));
			cid->setAcceptButtonText(tr("Ok"));
			cid->show();
		}
	}
}

void PluginManager::onMessageBoxButtonClicked(QAbstractButton *AButton)
{
	QMessageBox *mb = qobject_cast<QMessageBox*>(sender());
	if (mb && (mb->buttonRole(AButton) == QMessageBox::YesRole))
		quit();
}


void PluginManager::onShutdownTimerTimeout()
{
	finishShutdown();
}
