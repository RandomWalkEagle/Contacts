#include "sipphonewidget.h"


#include <QMessageBox>
//#include <QSettings>
#include <QDateTime>

#include "rsipauthentication.h"
#include "sipphoneproxy.h"

#include "callaudio.h"

#include "CrossDefine.h"




SipPhoneWidget::SipPhoneWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(hangupCall()));
}

SipPhoneWidget::SipPhoneWidget(KSipAuthentication *auth, CallAudio *callAudio, SipCall *initCall, SipPhoneProxy *parent, const char *name) : QWidget(NULL), _pSipCall(initCall)
{
	ui.setupUi(this);

	connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(hangupCall()));

	_pSipCallMember = NULL;
	_pSipAuthentication = NULL;
	_pAudioContoller = NULL;

	_ringCount = 0;
	_pRingTimer = NULL;
	_isRingingTone = false;
	_pAcceptCallTimer = NULL;
	_subject = "";


	_pCurrentStatus = ui.lblStatus;
	_pCurrentAudioStatus = ui.lblTime;
	//_pDialButton = ui.btnDial;
	_pHangupButton = ui.btnHangup;

	_pSipAuthentication = auth;

	_currentState = PreDial;

	_isHangupInitiator = false;

	// ������ ������� �� ��������� ������� ��������� ����������
	_pAudioContoller = callAudio;
	connect( _pAudioContoller, SIGNAL( outputDead() ), this, SLOT( audioOutputDead() ) );
	connect( _pAudioContoller, SIGNAL( statusUpdated() ), this, SLOT( updateAudioStatus() ) );
	connect( _pAudioContoller, SIGNAL( proxyPictureShow(const QImage&)), this, SLOT(remotePictureShow(const QImage&)));

	// �������
	//connect( this, SIGNAL(callDeleted(bool)), this, SLOT(callDeletedSlot(bool)) );

	_ringCount = 0;
	_pRingTimer = new QTimer();
	connect( _pRingTimer, SIGNAL( timeout() ), this, SLOT( ringTimeout() ) );

	_pAcceptCallTimer = new QTimer();
	connect( _pAcceptCallTimer, SIGNAL( timeout() ), this, SLOT( acceptCallTimeout() ) );

	switchCall(initCall);
}


SipPhoneWidget::~SipPhoneWidget()
{
	delete _pRingTimer;
	delete _pAcceptCallTimer;
	if( _pSipCall )
	{
		delete _pSipCall;
	}
}

void SipPhoneWidget::callDeletedSlot( bool )
{
	QMessageBox::information(NULL, "debug", "SipPhoneWidget::callDeletedSlot()");
}


void SipPhoneWidget::forceDisconnect( void )
{
	dbgPrintf( "KCallWidget: Starting force disconnect...\n" );
	if( _pAudioContoller->getCurrentCall() == _pSipCall )
	{
		_pAudioContoller->detachFromCall();
	}
	if( _pSipCallMember )
		disconnect( _pSipCallMember, 0, this, 0 );

	_pSipCallMember = 0;

	if( _pSipCall )
	{
		delete _pSipCall;
		_pSipCall = 0;
	}

	emit callDeleted(_isHangupInitiator);
	
	_pRingTimer->stop();
	//_pDialButton->setEnabled(false);
	_pHangupButton->setEnabled(false);
	//holdbutton->setEnabled(false);
	//transferbutton->setEnabled(false);
	//morebutton->setEnabled(false);
}

void SipPhoneWidget::callMemberStatusUpdated( void )
{
	SdpMessage sdpm;
	SdpMessage rsdp;
	if( _pSipCallMember->getState() == SipCallMember::state_Disconnected ) 
	{
		if( _pSipCallMember->getLocalStatusDescription().left( 2 ) == "!!" )
		{
			_pCurrentStatus->setText( tr("Call Failed") );
			QMessageBox::critical( this, "Virtus", _pSipCallMember->getLocalStatusDescription().remove(0,2) );
			//setHide();
		}
		else
		{
			_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() + " at " + QDateTime::currentDateTime().toString("hh:mm:ss"));
		}
		forceDisconnect();
	}
	else if( _pSipCallMember->getState() == SipCallMember::state_Redirected )
	{
		_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
		handleRedirect();
	}
	else if( _pSipCallMember->getState() == SipCallMember::state_Refer )
	{
		_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
		_pSipCallMember->setState( SipCallMember::state_Refer_handling );
		//handleRefer();
	}
	else if( _pSipCallMember->getState() == SipCallMember::state_Connected )
	{
		_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
		// hidebutton->setEnabled( false );
		//_pDialButton->setEnabled( false );
		_pHangupButton->setEnabled( true );
		_currentState = Connected;
		//if (!dtmfsenderTimer->isActive())
		//  dtmfsenderTimer->start(dtmfsenderdelay->value() * 10);
	}
	else
	{
		_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
	}
}

SipCall *SipPhoneWidget::getCall()
{
	return _pSipCall;
}

void SipPhoneWidget::switchCall( SipCall *newCall )
{
	_pRingTimer->stop();
	dbgPrintf( "KCallWidget: Switching calls...\n" );

	_pSipCall = newCall;
	
	_pAudioContoller->setBodyMask( _pSipCall->getSdpMessageMask() );
	//_pDialButton->setText( tr("Dial") );
	_pHangupButton->setText( tr("Hangup") );

	if( _pSipCallMember )
		disconnect( _pSipCallMember, 0, this, 0 );

	SipCallMemberIterator mIt = _pSipCall->getMemberList(); // �����
	mIt.toFront();

	_pSipCallMember = mIt.peekNext();//call->getMemberList().toFirst();
	if( _pSipCallMember )
	{
		if( _pSipCallMember->getState() == SipCallMember::state_Disconnected )
		{
			forceDisconnect();
			return;
		}
		if( _pSipCallMember->getState() == SipCallMember::state_Redirected )
		{
			handleRedirect();
			return;
		}
		_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
		connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), this, SLOT( callMemberStatusUpdated() ) );
		//setCaption( QString( tr("Call: ") ) + call->getCallId() );
		if( _pSipCall->getSubject() == tr("Incoming call") )
		{
			if( _pSipCall->getCallType() == SipCall::videoCall )
			{
				//setCaption( QString( tr("Incoming Video Call: ") ) + call->getCallId() );
				//holdbutton->setEnabled( false );
				//transferbutton->setEnabled( false );
			}
			else
			{
				//setCaption( QString( tr("Incoming Call: ") ) + call->getCallId() );
				//holdbutton->setEnabled( true );
				//transferbutton->setEnabled( true );
			}
			QString ss = _pSipCallMember->getUri().uri();
			QDateTime t = QDateTime::currentDateTime();

			//incomingCall = new IncomingCall( ss, t );
			//missedCalls.append( incomingCall );
			//incomingCall = IncomingCall( ss, t );
			//missedCalls.append( incomingCall );

			//updateCallRegister();

			// Ringing tone
			//QSettings settings;
			//QString p = "/settings/General";
			_isRingingTone = true;// ( settings.value( p + "/ringingtone", "No" ).toString().toUpper() == "YES" );

			_ringCount = 0;
			_pRingTimer->setSingleShot(true);
			_pRingTimer->start( ringTime_1 );
		}
		ui.remote->setText( _pSipCallMember->getUri().uri() );
		_subject = _pSipCall->getSubject();
		_pHangupButton->setEnabled( true );
		_pHangupButton->setFocus();
		if( _pSipCallMember->getState() == SipCallMember::state_InviteRequested )
		{
			//hidebutton->setEnabled( false );
			//_pDialButton->setEnabled( false );
			_pHangupButton->setEnabled( true );
			_currentState = Calling;
		}
		else if( _pSipCallMember->getState() == SipCallMember::state_RequestingInvite )
		{
			//_pDialButton->setText( tr("Accept") );
			_pHangupButton->setText( tr("Reject") );
			//hidebutton->setEnabled( false );
			//_pDialButton->setEnabled( true );
			_pHangupButton->setEnabled( true );
			_currentState = Called;
		}
		else
		{
			//hidebutton->setEnabled( false );
			//_pDialButton->setEnabled( false );
			_pHangupButton->setEnabled( true );
			_currentState = Connected;
			//if (!dtmfsenderTimer->isActive())
			//  dtmfsenderTimer->start(dtmfsenderdelay->value() * 10);
		}
	}
	else
	{
		_pCurrentStatus->setText( QString::null );
		if( _pSipCall->getCallType() == SipCall::videoCall )
		{
			//setCaption( getUserPrefix() + QString( tr("Outgoing Video Call: ") ) + call->getCallId() );
			//holdbutton->setEnabled( false );
			//transferbutton->setEnabled( false );
		}
		else
		{
			//setCaption( getUserPrefix() + QString( tr("Outgoing Call: ") ) + call->getCallId() );
			//holdbutton->setEnabled( true );
			//transferbutton->setEnabled( true );
		}
		_subject = _pSipCall->getSubject();
		ui.remote->setText( QString::null );
		//hidebutton->setEnabled( true );
		//_pDialButton->setEnabled( true );
		//_pDialButton->setFocus();
		_pHangupButton->setEnabled( false );
		_currentState = PreDial;
	}
	updateAudioStatus();
}

void SipPhoneWidget::updateAudioStatus( void )
{
	if( _pAudioContoller->getCurrentCall() == _pSipCall )
	{
		if( _pAudioContoller->isRemoteHold() )
			_pCurrentAudioStatus->setText( tr("Attached [holding]") );
		else
			_pCurrentAudioStatus->setText( tr("Attached [active]") );
	}
	else
	{
		_pCurrentAudioStatus->setText( tr("Unattached") );
	}
}

void SipPhoneWidget::audioOutputDead( void )
{
	dbgPrintf( "KCallAudio: Broken output pipe, disconnecting unpolitely\n" );
	forceDisconnect();
}

void SipPhoneWidget::setRemote( QString newremote )
{
	ui.remote->setText( newremote );
}

void SipPhoneWidget::remotePictureShow(const QImage& img)
{
	ui.lblRemoteImage->setPixmap(QPixmap::fromImage(img));
}

void SipPhoneWidget::clickDial()
{
	dialClicked();
}

void SipPhoneWidget::clickHangup()
{
	hangupCall();
}

void SipPhoneWidget::pleaseDial( const SipUri &dialuri )
{
	ui.remote->setText( dialuri.reqUri() );
	dialClicked();
}

void SipPhoneWidget::dialClicked( void )
{
	// Multi-purpose buttons hack
	if( _currentState == Called )
	{
#pragma message("**** setAutoDelete �� ����� KCallWidget::dialClicked")
		//missedCalls.setAutoDelete( false );

		//missedCalls.remove( incomingCall );

		//missedCalls.setAutoDelete( true );

		//receivedCalls.append( incomingCall );
		//updateCallRegister();
		acceptCall();
		return;
	}
	if( ui.remote->text().length() == 0 )
	{
		QMessageBox::critical( this, tr("Error: No Destination"), tr("You must specify someone to call.") );
		return;
	}

	QString strRemoteUri;
	QString s = ui.remote->text();
	if( s.contains( '[' ) && s.contains( ']' ) )
	{
		strRemoteUri = s.mid( s.indexOf( '[' ) + 1, s.indexOf( ']' ) - s.indexOf( '[' ) - 1 );
	}
	else
	{
		if( s.left( 4 ).toLower() != "tel:" )
		{
			if( s.left( 4 ).toLower() != "sip:" )
			{
				s = "sip:" + s;
			}
			if( !s.contains( '@' ) )
			{
				s = s + "@" + _pSipCall->getHostname();
			}
		}
		strRemoteUri = s;
	}

	//////QSettings settings;
	//////QString p = "/settings/Call register/";
	//////QString str;
	//////QString label;

	//////label = p + "/Dialled" + str.setNum( 0 );
	//////settings.setValue(label, QString(ui.remote->text().toLatin1()) );

	////////for( int i = 0; i < remote->count(); i++ )
	////////{
	////////  label = p + "/Dialled" + str.setNum( 0 );
	////////  //settings.writeEntry(label, remote->text(i).toLatin1() );
	////////  settings.setValue(label, QString(ui.remote->text().toLatin1()) );
	////////}
	////////label = p + "/Dialled" + str.setNum( remote->count() );

	//////label = p + "/Dialled1";
	//////settings.setValue( label, "" );

	SipUri remoteuri( strRemoteUri );

	//remoteuri.setUsername("Buratini");
	//remoteuri.setPassword("8uratino");

	_pAudioContoller->setRtpCodec( codecUnknown );
	_pAudioContoller->setVideoRtpCodec( codecUnknown );

	_pSipCallMember = new SipCallMember( _pSipCall, remoteuri );
	connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), this, SLOT( callMemberStatusUpdated() ) );
	connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), _pSipAuthentication, SLOT( authRequest( SipCallMember * ) ) );

	_pAudioContoller->setCurrentCall( _pSipCall );
	_pAudioContoller->attachToCallMember( _pSipCallMember ); // �����

	SdpMessage invSdpMsg = _pAudioContoller->audioOut();
	QString sdpMessageStr = invSdpMsg.message( _pAudioContoller->getRtpCodec(), _pAudioContoller->getVideoRtpCodec(), _pAudioContoller->getBodyMask() );

	_pSipCallMember->requestInvite(sdpMessageStr , MimeContentType( "application/sdp" ) );

	//_pDialButton->setEnabled( false );
	_pHangupButton->setEnabled( true );
	_currentState = Calling;
	_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
}



// ����� ���������� �-�� KCallWidget::acceptCallTimeout
void SipPhoneWidget::acceptCallTimeout( void )
{
	if( _pAudioContoller->checkCodec( _pSipCallMember ) )
	{
		_pAudioContoller->setCurrentCall( _pSipCall );
		_pAudioContoller->attachToCallMember( _pSipCallMember );

		SdpMessage invitee = _pAudioContoller->audioOut();

		QString sdpMessage = invitee.message( _pAudioContoller->getRtpCodec(), _pAudioContoller->getVideoRtpCodec(), _pAudioContoller->getBodyMask() );

		_pSipCallMember->acceptInvite(sdpMessage , MimeContentType( "application/sdp" ) );
		//_pDialButton->setText( tr("Dial") );
		_pHangupButton->setText( tr("Hangup") );
		//hidebutton->setEnabled( false );
		//_pDialButton->setEnabled( false );
		_pHangupButton->setEnabled( true );
	}
	else
	{
		_pSipCallMember->notAcceptableHere();
		QMessageBox::information( this, tr("Accept Call"), tr("Accepted codec not found.") );
	}
}




void SipPhoneWidget::acceptCall( void )
{
	_pRingTimer->stop();

	_pAcceptCallTimer->setSingleShot(true);
	_pAcceptCallTimer->start( acceptTime );
}


void SipPhoneWidget::hangupCall( void )
{
	if(!_pHangupButton || !_pHangupButton->isEnabled() )
	{
		return;
	}
	_pRingTimer->stop();
	_isHangupInitiator = true;

	// Reject call if that's our current state
	if( _currentState == Called )
	{
		_pSipCallMember->declineInvite();
		setHidden(true);
		//setHide();
		return;
	}
	if( _pSipCall->getCallStatus() != SipCall::callDead )
	{
		_pHangupButton->setEnabled( false );
		if( _pSipCallMember->getState() == SipCallMember::state_Connected )
		{
			_pSipCallMember->requestDisconnect();
			// �������� ��������! ���������� �������� ������ �� ������� ������ hangupCall
			if( _pAudioContoller->getCurrentCall() == _pSipCall )
			{
				_pAudioContoller->detachFromCall();
			}
		}
		else
		{
			_pSipCallMember->cancelTransaction();
			if( _pAudioContoller->getCurrentCall() == _pSipCall )
			{
				_pAudioContoller->detachFromCall();
			}
		}
		setHidden(true);
		return;
	}
}


void SipPhoneWidget::handleRedirect( void )
{
	dbgPrintf( "KCallWidget: Handling redirect...\n" );
	//Q3ValueList<SipUri>::Iterator it;

	SipUriList urilist = _pSipCallMember->getRedirectList();
	while ( urilist.getListLength() > 0 )
	{
		SipUri redirto = urilist.getPriorContact();
		QMessageBox mb( tr("Redirect"),
			tr("Call redirected to: ") + "\n" + redirto.reqUri() + "\n\n" +
			tr("Do you want to proceed ? "),
			QMessageBox::Information,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape );
		switch( mb.exec() )
		{
		case QMessageBox::No:
			continue;
		case QMessageBox::Cancel:
			forceDisconnect();
			return;
		}
		if( _pAudioContoller->getCurrentCall() == _pSipCall )
		{
			_pAudioContoller->detachFromCall();
		}
		if( _pSipCallMember )
			disconnect( _pSipCallMember, 0, this, 0 );

		_pSipCallMember = 0;
		if( _pSipCall )
		{
			delete _pSipCall;
			_pSipCall = 0;
		}

		redirectCall( redirto, _subject );
		//setHide();
		return;
	}
	forceDisconnect();
}