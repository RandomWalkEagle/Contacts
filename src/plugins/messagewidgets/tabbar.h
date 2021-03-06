#ifndef TABBAR_H
#define TABBAR_H

#include <QFrame>
#include <QMouseEvent>
#include "tabbarlayout.h"
#include "tabbaritem.h"

class TabBar :
	public QFrame
{
	Q_OBJECT
public:
	TabBar(QWidget *AParent = NULL);
	virtual ~TabBar();
	int count() const;
	int currentIndex() const;
	void setCurrentIndex(int AIndex);
	QIcon tabIcon(int AIndex) const;
	void setTabIcon(int AIndex, const QIcon &AIcon);
	QString tabIconKey(int AIndex) const;
	void setTabIconKey(int AIndex, const QString &AIconKey);
	QString tabText(int AIndex) const;
	void setTabText(int AIndex, const QString &AText);
	QString tabToolTip(int AIndex) const;
	void setTabToolTip(int AIndex, const QString &AToolTip);
	ITabPageNotify tabNotify(int AIndex) const;
	void setTabNotify(int AIndex, const ITabPageNotify &ANotify);
	bool tabsClosable() const;
	void setTabsClosable(bool ACloseable);
	int tabAt(const QPoint &APosition) const;
	int addTab(const QString &AText);
	void removeTab(int AIndex);
public slots:
	void showNextTab();
	void showPrevTab();
signals:
	void currentChanged(int AIndex);
	void tabMenuRequested(int AIndex);
	void tabCloseRequested(int AIndex);
protected slots:
	void onCloseButtonClicked();
protected:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void dragEnterEvent(QDragEnterEvent *AEvent);
	void dragMoveEvent(QDragMoveEvent *AEvent);
	void dragLeaveEvent(QDragLeaveEvent *AEvent);
private:
	int FPressedIndex;
	QPoint FPressedPos;
	QPoint FDragCenterDistance;
private:
	int FCurrentIndex;
	bool FTabsCloseable;
	TabBarLayout *FLayout;
	QList<TabBarItem *> FItems;
};

#endif // TABBAR_H
