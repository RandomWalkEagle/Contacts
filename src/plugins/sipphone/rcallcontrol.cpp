#include "rcallcontrol.h"

#include <QMessageBox>

RCallControl::RCallControl(CallSide callSide, QWidget *parent)
	: QWidget(parent), _callStatus(Ringing)
{
	ui.setupUi(this);

	_callSide = callSide;
	if(callSide == Receiver)
	{
		ui.btnAccept->show();
		ui.btnHangup->show();
	}
	else
	{
		ui.btnAccept->hide();
		ui.btnHangup->show();
	}

	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), SIGNAL(micVolumeChange(int)));

	connect(ui.btnAccept, SIGNAL(clicked()), this, SLOT(onAccept()));
	connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(onHangup()));
}

RCallControl::~RCallControl()
{

}

void RCallControl::statusTextChanged(const QString& text)
{
	ui.lblStatus->setText(text);
}

void RCallControl::onAccept()
{
	QMessageBox::information(NULL, "Accept", "");
	if(_callSide == Caller)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	break;
		//	case Hangup:
		//	case RingTimeout:
		//	case CallError:
		//		emit redialCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			// �� ������ ������ ����
		}
		else if(_callStatus == Hangup)
		{
			emit redialCall();
		}
		else if(_callStatus == Ringing)
		{
			// �� ����� ����
		}
		else if(_callStatus == RingTimeout)
		{
			emit redialCall();
		}
		else if(_callStatus == CallError)
		{
			emit redialCall();
		}
	}
	if(_callSide == Receiver)
	{
		//switch(_callStatus)
		//{
		//case Ringing:
		//	emit acceptCall();
		//	break;
		//case RingTimeout:
		//	emit callbackCall();
		//	break;
		//default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			// �� ����� ����
		}
		else if(_callStatus == Hangup)
		{
			// �� ����� ����
		}
		else if(_callStatus == Ringing)
		{
			emit acceptCall();
		}
		else if(_callStatus == RingTimeout)
		{
			emit callbackCall();
		}
		else if(_callStatus == CallError)
		{
			// �� ����� ����
		}
	}
}

void RCallControl::onHangup()
{
	//QMessageBox::information(NULL, "Hangup", "");
	if(_callSide == Caller)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	case Ringing:
		//		emit hangupCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			emit hangupCall();
		}
		else if(_callStatus == Hangup)
		{
			// �� ����� ����
		}
		else if(_callStatus == Ringing)
		{
			emit hangupCall();
		}
		else if(_callStatus == RingTimeout)
		{
			// �� ����� ����
		}
		else if(_callStatus == CallError)
		{
			// �� ����� ����
		}
		else
		{
			emit hangupCall();
		}
	}
	if(_callSide == Receiver)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	case Ringing:
		//		emit hangupCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			emit hangupCall();
		}
		else if(_callStatus == Hangup)
		{
			// �� ����� ����
		}
		else if(_callStatus == Ringing)
		{
			emit hangupCall();
		}
		else if(_callStatus == RingTimeout)
		{
			// �� ����� ����
		}
		else if(_callStatus == CallError)
		{
			// �� ����� ����
		}
		else
		{
			emit hangupCall();
		}
	}
}

void RCallControl::callStatusChanged(CallStatus status)
{
	if(_callStatus != status)
	{
		_callStatus = status;
	}
	else
		return;


	if(_callStatus == Accepted)
	{
		if(_callSide == Caller)
		{
			statusTextChanged("...");
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
		else
		{
			statusTextChanged("...");
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
	}
	else if(_callStatus == Hangup)
	{
		if(_callSide == Caller)
		{
			statusTextChanged(tr("Calling failure..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
		}
		else
		{
			// � ����������� �������� ��� ������ ������ ������ ���������
		}
	}
	else if(_callStatus == Ringing)
	{
		if(_callSide == Caller)
		{
			statusTextChanged(tr("Outgoing Call..."));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
		else
		{
			statusTextChanged(tr("Incoming Call..."));
			ui.btnAccept->show();
			ui.btnHangup->show();
			ui.btnAccept->setText(tr("Accept"));
			ui.btnHangup->setText(tr("Hangup"));
		}
	}
	else if(_callStatus == RingTimeout)
	{
		if(_callSide == Caller)
		{
			statusTextChanged(tr("No answer..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->setText(tr(""));
		}
		else
		{
			statusTextChanged(tr("Missed Call..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Callback"));
			ui.btnHangup->setText(tr(""));
		}
	}
	//else if(_callStatus == CallStatus::Trying)
	//{
	//	if(_callSide == Caller)
	//	{

	//	}
	//	else
	//	{

	//	}
	//}
	else // ������ ������ ������
	{
		if(_callSide == Caller)
		{
			statusTextChanged(tr("Call Error..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->setText(tr(""));
		}
		else
		{
			// � ����������� �������� ������ ���������
		}
	}
}

