#ifndef AUTOSIZETEXTEDIT_H
#define AUTOSIZETEXTEDIT_H

#include <QTextBrowser>
#include "utilsexport.h"

class UTILS_EXPORT AutoSizeTextEdit :
	public QTextBrowser
{
	Q_OBJECT
public:
	AutoSizeTextEdit(QWidget *AParent);
	~AutoSizeTextEdit();
public:
	bool autoResize() const;
	void setAutoResize(bool AResize);
	int minimumLines() const;
	void setMinimumLines(int ALines);
	int maximumLines() const;
	void setMaximumLines(int ALines);
public:
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
protected:
	virtual void keyPressEvent(QKeyEvent *ev);
protected:
	int textHeight(int ALines = 0) const;
protected slots:
	void onTextChanged();
	void onScrollBarRangeChanged(int min, int max);
private:
	bool FAutoResize;
	int FMinimumLines;
	int FMaximumLines;
};

#endif // AUTOSIZETEXTEDIT_H
