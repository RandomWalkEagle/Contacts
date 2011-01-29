#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QAbstractItemDelegate>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/irostersview.h>

typedef QMap<int, IRostersLabel> RostersLabelItems;
Q_DECLARE_METATYPE(RostersLabelItems);

struct LabelItem
{
	int id;
	QSize size;
	QRect rect;
	IRostersLabel item;
	bool operator <(const LabelItem &AItem) const
	{
		return item.order < AItem.item.order;
	}
};

class RosterIndexDelegate :
			public QAbstractItemDelegate
{
	Q_OBJECT
	friend class RostersView;
public:
	RosterIndexDelegate(QObject *AParent);
	~RosterIndexDelegate();
	//QAbstractItemDelegate
	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	//RosterIndexDelegate
	int labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	QRect labelRect(int ALabelId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setShowBlinkLabels(bool AShow);
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
protected:
	QHash<int,QRect> drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void drawLabelItem(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const LabelItem &ALabel) const;
	void drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AOption) const;
	void drawFocus(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const QRect &ARect) const;
	QStyleOptionViewItemV4 indexOptions(const QModelIndex &AIndex, const QStyleOptionViewItem &AOption) const;
	QStyleOptionViewItemV4 indexFooterOptions(const QStyleOptionViewItemV4 &AOption) const;
	QList<LabelItem> itemLabels(const QModelIndex &AIndex) const;
	QList<LabelItem> itemFooters(const QModelIndex &AIndex) const;
	QSize variantSize(const QStyleOptionViewItemV4 &AOption, const QVariant &AValue) const;
	void getLabelsSize(const QStyleOptionViewItemV4 &AOption, QList<LabelItem> &ALabels) const;
	void removeWidth(QRect &ARect, int AWidth, bool AIsLeftToRight) const;
	QString prepareText(const QString &AText) const;
private:
	QIcon::Mode getIconMode(QStyle::State AState) const;
	QIcon::State getIconState(QStyle::State AState) const;
private:
	bool FShowBlinkLabels;
private:
	static const int spacing = 2;
};

#endif // ROSTERINDEXDELEGATE_H
