#ifndef VIDEOTRANSLATOR_H
#define VIDEOTRANSLATOR_H

#include <QObject>
#include <QHostAddress>
#include "RCameraFlow.h"
//#include "rtpdatasender.h"

class QUdpSocket;
class H264Container;


class EncodedFrame
{
public:
  EncodedFrame(qint32 timestamp);
  ~EncodedFrame();

public:
  bool insertSubframe(const QByteArray& subFrame, qint16 seqNum, qint32 timeStamp);

  inline quint32 frameTS() const { return _frameTimeStamp;}

  QByteArray frame();


private:
  QByteArray _netBuffer2;
  quint32 _frameTimeStamp;
  QList<QByteArray*> _frameHash;
};


class VideoTranslator : public QObject
{
  Q_OBJECT

public:
    VideoTranslator(QObject *parent);
    VideoTranslator(const QHostAddress& remoteHost, int remoteVideoPort, int localVideoPort, QObject *parent);
    ~VideoTranslator();

signals:
  // ���������� ���������� ��������
  void updatePicture(const QImage& img);
  void updatePicture(const QPixmap& pix);

  // ���������� �������� ���������� � ������
  void updateLocalPicture(const QImage& img);
  void updateLocalPicture(const QPixmap& pix);

  void updateSysInfo(const QString&);

protected:
  void timerEvent(QTimerEvent*);

  // �������� ����������� �� ����
  void sendImage(QImage img);

protected slots:
  //void hostFound();
  void dataArrived();
  void dataArrived1();
  void onSocketError(QAbstractSocket::SocketError);
  void sendImage();
  void updText();
  void procceedDataProcessing( const QByteArray& rtpPacket );

private:
  // ����� � ������. ��������� �������� � ������.
  RCameraFlow* _cameraFlow;

  // ������ ��� �������� �� ���� � ��� ������
  QUdpSocket *_udpSocketForSend;
  QHostAddress _remoteHost;
  int _remPort;

  //RtpDataSender* _dataSender;

  QUdpSocket *_udpSocketForRead;

  // �������� �����������
  QImage _incImage;

  // ����� ��� �������� ������ �� ����
  //uchar* _netBuffer;
  QByteArray _netBuffer2;
  QString _testStr;
  QString _testStrIn;

  int _ssrc; // �������� ������������� ��� �����
  quint32 _startTime;
  quint16 _senderSeq;
 
  // �������������� ��������: ����������� �������� / ������� �������� �� ����
  int _showTimer;
  int _sendTimer;

  // ������� �������������� �������� �� (�����/���.)
  int _showTimerFreq;
  int _sendTimerFreq;

  // ������ ������ ������������ ������������/�������������� ������� H.264
  H264Container *_pH264;
};

#endif // VIDEOTRANSLATOR_H
