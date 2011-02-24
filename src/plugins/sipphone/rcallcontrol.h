#ifndef RCALLCONTROL_H
#define RCALLCONTROL_H

#include <QWidget>
#include "ui_rcallcontrol.h"

class RCallControl : public QWidget
{
	Q_OBJECT

public:
	enum CallSide
	{
		Receiver = 0,
		Caller = 1
	};
	enum CallStatus
	{
		Accepted,		// ������ ������
		Hangup,			// �������� ������
		Ringing,		// ������
		RingTimeout,// ����� ����� ������� ������ (��� ������)
		CallError		// ���� ������
		//Trying,			// ������� ����������� �����
	};

public:
	RCallControl(CallSide, QWidget *parent = 0);
	~RCallControl();

	CallSide side() {return _callSide;}

signals:
	void camStateChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);

signals: // ������� ����������� �������
	void acceptCall();		// ������� ������
	void hangupCall();		// ����� ������
	void redialCall();		// ������ �������� ���������� ������� � ������ ���������� ������
	void callbackCall();	// ������ �������� ���������� ������� � ������ ��������� ������ �� ����������� �����

public slots:
	void statusTextChanged(const QString&);
	void callStatusChanged(CallStatus);

protected slots:
	void onAccept();
	void onHangup();

private:
	CallSide _callSide;			// �������������� ������� ������ (��������/�����������)
	CallStatus _callStatus;	// ������ ������
	Ui::RCallControl ui;
};

#endif // RCALLCONTROL_H
