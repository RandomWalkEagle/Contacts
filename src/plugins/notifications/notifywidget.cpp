#include "notifywidget.h"

#include <QTimer>
#include <QPainter>
#include <QTextFrame>
#include <QFontMetrics>
#include <QTextDocumentFragment>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

#define DEFAUTL_TIMEOUT           9
#define ANIMATE_STEPS             20
#define ANIMATE_TIME              1000
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       1.0
#define ANIMATE_OPACITY_STEP      ((ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS)

#define IMAGE_SIZE                QSize(36,36)
#define MAX_MESSAGES              3

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

NotifyWidget::NotifyWidget(const INotification &ANotification) : QWidget(NULL, Qt::ToolTip | Qt::WindowStaysOnTopHint)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_NOTIFICATION_NOTIFYWIDGET);

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_NOTIFICATION);
	if (FBorder)
	{
		FBorder->setMovable(false);
		FBorder->setStaysOnTop(true);
		FBorder->setResizable(false);
		FBorder->setShowInTaskBar(false);
		FBorder->setMinimizeButtonVisible(false);
		FBorder->setMaximizeButtonVisible(false);
		FBorder->setAttribute(Qt::WA_DeleteOnClose, true);
		FBorder->setWindowFlags(FBorder->windowFlags() | Qt::ToolTip);
		connect(FBorder, SIGNAL(closeClicked()),FBorder,SLOT(close()));
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
	}
	window()->installEventFilter(this);

	ui.wdtButtons->setVisible(ANotification.actions.count());

	ui.tbrText->setContentsMargins(0,0,0,0);
	ui.tbrText->setMaxHeight(FDesktop->availableGeometry().height() / 6);

	FYPos = -1;
	FAnimateStep = -1;
	FFirstLayout = true;
	FTimeOut = ANotification.data.value(NDR_POPUP_TIMEOUT,DEFAUTL_TIMEOUT).toInt();

	FCloseTimer = new QTimer(this);
	FCloseTimer->setInterval(1000);
	FCloseTimer->setSingleShot(false);
	connect(FCloseTimer, SIGNAL(timeout()), SLOT(onCloseTimerTimeout()));

	FTitle = ANotification.data.value(NDR_POPUP_TITLE).toString();
	FCaption = ANotification.data.value(NDR_POPUP_CAPTION).toString();
	FNotice = ANotification.data.value(NDR_POPUP_NOTICE).toString();

	QIcon icon = qvariant_cast<QIcon>(ANotification.data.value(NDR_POPUP_ICON));
	if (!icon.isNull())
		ui.lblIcon->setPixmap(icon.pixmap(QSize(16,16)));
	else
		ui.lblIcon->setVisible(false);

	QImage image = qvariant_cast<QImage>(ANotification.data.value(NDR_POPUP_IMAGE));
	if (!image.isNull())
	{
		ui.lblImage->setFixedWidth(IMAGE_SIZE.width());
		ui.lblImage->setPixmap(QPixmap::fromImage(image.scaled(IMAGE_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
	}
	else
	{
		ui.lblImage->setVisible(false);
	}
	ui.lblTitle->setVisible(!FTitle.isEmpty());
	ui.lblCaption->setVisible(!FCaption.isEmpty());
	ui.lblNotice->setVisible(!FNotice.isEmpty());

	QString styleKey = ANotification.data.value(NDR_POPUP_STYLEKEY).toString();
	if (!styleKey.isEmpty())
		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(ui.frmPopup,styleKey);

	FCanActivate = ANotification.data.value(NDR_POPUP_CAN_ACTIVATE, true).toBool();

	setCursor(FCanActivate ? Qt::PointingHandCursor : Qt::ArrowCursor);
	ui.tbrText->setCursor(FCanActivate ? Qt::PointingHandCursor : Qt::ArrowCursor);

	appendNotification(ANotification);
	updateElidedText();
}

NotifyWidget::~NotifyWidget()
{
	FWidgets.removeAll(this);
	layoutWidgets();
	emit windowDestroyed();
}

// returns false if widget can't appear
bool NotifyWidget::appear()
{
	if (!FWidgets.contains(this) && (FWidgets.count() < MAX_MESSAGES))
	{
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(false);
		timer->setInterval(ANIMATE_STEP_TIME);
		timer->start();
		connect(timer,SIGNAL(timeout()),SLOT(onAnimateStep()));

		if (FTimeOut > 0)
			FCloseTimer->start();

		window()->setWindowOpacity(ANIMATE_OPACITY_START);

		FWidgets.prepend(this);
		layoutWidgets();
		return true;
	}
	window()->deleteLater();
	return false;
}

void NotifyWidget::animateTo(int AYPos)
{
	if (FYPos != AYPos)
	{
		FYPos = AYPos;
		FAnimateStep = ANIMATE_STEPS;
	}
}

void NotifyWidget::appendAction(Action *AAction)
{
	ActionButton *button = new ActionButton(AAction,ui.wdtButtons);
	button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	ui.wdtButtons->layout()->addWidget(button);
}

void NotifyWidget::appendNotification(const INotification &ANotification)
{
	QString text = ANotification.data.value(NDR_POPUP_TEXT).toString().trimmed();
	if (!text.isEmpty())
	{
		QTextDocument doc;
		doc.setHtml(text);
		if (doc.rootFrame()->lastPosition() > 140)
		{
			QTextCursor cursor(&doc);
			cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,120);
			cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
			cursor.removeSelectedText();
			cursor.insertText("...");
			text = getHtmlBody(doc.toHtml());
		}

		FTextMessages.append(text.replace("/emoticons/smiles/", "/emoticons/smiles_dark/"));  // complex to prevent unneeded replacement
		while(FTextMessages.count() > MAX_MESSAGES)
			FTextMessages.removeAt(0);
	}
	QString html = FTextMessages.join("<br>");
	ui.tbrText->setHtml(html);
	ui.tbrText->setVisible(!html.isEmpty());
}

void NotifyWidget::updateElidedText()
{
	ui.lblCaption->setText(ui.lblCaption->fontMetrics().elidedText(FCaption,Qt::ElideRight,ui.lblCaption->width() - ui.lblCaption->frameWidth()*2));
	ui.lblCaption->setVisible(!ui.lblCaption->text().isEmpty());
	ui.lblTitle->setText(ui.lblTitle->fontMetrics().elidedText(FTitle,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
	ui.lblTitle->setVisible(!ui.lblTitle->text().isEmpty());
	ui.lblNotice->setText(ui.lblNotice->fontMetrics().elidedText(FNotice,Qt::ElideRight,ui.lblNotice->width() - ui.lblNotice->frameWidth()*2));
	ui.lblNotice->setVisible(!ui.lblNotice->text().isEmpty());
}

void NotifyWidget::enterEvent(QEvent *AEvent)
{
	FTimeOut += 14;
	QWidget::enterEvent(AEvent);
}

void NotifyWidget::leaveEvent(QEvent *AEvent)
{
	FTimeOut -= 14;
	QWidget::leaveEvent(AEvent);
}

void NotifyWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QWidget::mouseReleaseEvent(AEvent);
	if (isVisible())
	{
		if ((AEvent->button() == Qt::LeftButton) && FCanActivate)
			emit notifyActivated();
		else if (AEvent->button() == Qt::RightButton)
			emit notifyRemoved();
	}
}

void NotifyWidget::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	updateElidedText();
	layoutWidgets();
}

void NotifyWidget::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool NotifyWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched==window() && AEvent->type() == QEvent::LayoutRequest)
		QTimer::singleShot(0,this,SLOT(onAdjustHeight()));
	return QWidget::eventFilter(AWatched,AEvent);
}

void NotifyWidget::onAdjustHeight()
{
	window()->resize(window()->width(),window()->sizeHint().height());
	if (FFirstLayout)
	{
		FFirstLayout = false;
		QRect display = FDesktop->availableGeometry();
		window()->move(display.right() - window()->geometry().width(), display.bottom());
	}
}

void NotifyWidget::onAnimateStep()
{
	QWidget *widget = window();
	if (FAnimateStep > 0)
	{
		int ypos;
		ypos = widget->y()+(FYPos-widget->y())/(FAnimateStep);
		widget->setWindowOpacity(qMin(widget->windowOpacity()+ANIMATE_OPACITY_STEP, ANIMATE_OPACITY_END));
		widget->move(widget->x(),ypos);
		FAnimateStep--;
	}
	else if (FAnimateStep == 0)
	{
		widget->move(widget->x(),FYPos);
		widget->setWindowOpacity(ANIMATE_OPACITY_END);
		FAnimateStep--;
	}
}

void NotifyWidget::onCloseTimerTimeout()
{
	if (FTimeOut > 0)
		FTimeOut--;
	else
		window()->deleteLater();
}

void NotifyWidget::layoutWidgets()
{
	QRect display = FDesktop->availableGeometry();
	int ypos = display.bottom();
	for (int i = 0; ypos > 0 && i < FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			widget->window()->show();
			widget->window()->move(display.right() - widget->window()->geometry().width(), display.bottom());
		}
		ypos -= widget->window()->geometry().height();
		widget->animateTo(ypos);
		ypos -= 2;
	}
}


