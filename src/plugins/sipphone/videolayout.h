#ifndef VIDEOLAYOUT_H
#define VIDEOLAYOUT_H

#include <QRectF>
#include <QLayout>
#include <QWidget>
#include "videoframe.h"

class VideoLayout : 
	public QLayout
{
	Q_OBJECT;
public:
	VideoLayout(VideoFrame *ARemoteVideo, VideoFrame *ALocalVideo, QWidget *AButtons, QWidget *AParent);
	~VideoLayout();
	// QLayout
	int count() const;
	void addItem(QLayoutItem *AItem);
	QLayoutItem *itemAt(int AIndex) const;
	QLayoutItem *takeAt(int AIndex);
	// QLayoutItem
	QSize sizeHint() const;
	void setGeometry(const QRect &ARect);
	// VideoLayout
	bool isVideoVisible() const;
	void setVideoVisible(bool AVisible);
	int locaVideoMargin() const;
	void setLocalVideoMargin(int AMargin);
	int buttonsPadding() const;
	void setButtonsPadding(int APadding);
	void setControllsWidget(QWidget *AControlls);
public slots:
	void saveLocalVideoGeometry();
	void restoreLocalVideoGeometry();
protected:
	void saveLocalVideoGeometryScale();
	Qt::Alignment remoteVideoAlignment() const;
	Qt::Alignment geometryAlignment(const QRect &AGeometry) const;
	QRect adjustRemoteVideoPosition(const QRect &AGeometry) const;
	QRect adjustLocalVideoSize(const QRect &AGeometry) const;
	QRect adjustLocalVideoPosition(const QRect &AGeometry) const;
	QRect correctLocalVideoPosition(const QRect &AGeometry) const;
	QRect correctLocalVideoSize(Qt::Corner ACorner, const QRect &AGeometry) const;
protected slots:
	void onLocalVideoDoubleClicked();
	void onLocalVideoMove(const QPoint &APos);
	void onLocalVideoResize(Qt::Corner ACorner, const QPoint &APos);
private:
	bool FVideoVisible;
	int FButtonsPadding;
	int FLocalMargin;
	int FLocalStickDelta;
	QRectF FLocalScale;
	QWidget *FButtons;
	QWidget *FControlls;
	VideoFrame *FLocalVideo;
	VideoFrame *FRemoteVideo;
};

#endif // VIDEOLAYOUT_H
