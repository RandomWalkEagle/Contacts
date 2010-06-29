#ifndef ROSTERSEARCH_H
#define ROSTERSEARCH_H

#include <QTimer>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterproxyorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersearch.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/action.h>
#include <utils/toolbarchanger.h>
#include "searchedit.h"

class RosterSearch :
			public QSortFilterProxyModel,
			public IPlugin,
			public IRosterSearch
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterSearch);
public:
	RosterSearch();
	~RosterSearch();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return ROSTERSEARCH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRosterSearch
	virtual void startSearch();
	virtual QString searchPattern() const;
	virtual void setSearchPattern(const QString &APattern);
	virtual bool isSearchEnabled() const;
	virtual void setSearchEnabled(bool AEnabled);
	virtual void insertSearchField(int ADataRole, const QString &AName, bool AEnabled);
	virtual Menu *searchFieldsMenu() const;
	virtual QList<int> searchFields() const;
	virtual bool isSearchFieldEnabled(int ADataRole) const;
	virtual void setSearchFieldEnabled(int ADataRole, bool AEnabled);
	virtual void removeSearchField(int ADataRole);
signals:
	void searchResultUpdated();
	void searchStateChanged(bool AEnabled);
	void searchPatternChanged(const QString &APattern);
	void searchFieldInserted(int ADataRole, const QString &AName);
	void searchFieldChanged(int ADataRole);
	void searchFieldRemoved(int ADataRole);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
	bool eventFilter(QObject *, QEvent *);
protected slots:
	void onFieldActionTriggered(bool);
	void onSearchActionTriggered(bool AChecked);
	void onEditTimedOut();
	void onSearchTextChanged(const QString &text);
	void createSearchLinks();
	void destroySearchLinks();
	void createNotFoundItem();
	void destroyNotFoundItem();
	void indexClicked(IRosterIndex *AIndex, int ALabelId);
private:
	IMainWindow *FMainWindow;
	IRostersViewPlugin *FRostersViewPlugin;
	IRostersModel * rostersModel;
	IRosterIndex *searchInHistory;
	IRosterIndex *searchInRambler;
	IRosterIndex *searchNotFound;
private:
	Menu *FFieldsMenu;
	QTimer FEditTimeout;
	SearchEdit *FSearchEdit;
	QHash<int,Action *> FFieldActions;
	bool searchEnabled;
	int searchInRamblerLabel, searchInHistoryLabel, searchNotFoundLabel;
	mutable int foundItems;
};

#endif // ROSTERSEARCH_H
