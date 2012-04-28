#include "callcontrolwidget.h"

#include <QFile>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/stylesheets.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/imagemanager.h>

CallControlWidget::CallControlWidget(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_CALLCONTROLWIDGET);

	FAvatars = NULL;
	FGateways = NULL;
	FMetaContacts = NULL;

	FCallTimer.setInterval(1000);
	FCallTimer.setSingleShot(false);
	connect(&FCallTimer,SIGNAL(timeout()),SLOT(onCallTimerTimeout()));

	FSipCall = ASipCall;
	connect(FSipCall->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(FSipCall->instance(),SIGNAL(deviceStateChanged(int, int)),SLOT(onCallDeviceStateChanged(int, int)));
	connect(FSipCall->instance(),SIGNAL(devicePropertyChanged(int, int, const QVariant &)),SLOT(onCallDevicePropertyChanged(int, int, const QVariant &)));

	connect(ui.pbtAccept,SIGNAL(clicked()),SLOT(onAcceptButtonClicked()));
	connect(ui.pbtReject,SIGNAL(clicked()),SLOT(onRejectButtonClicked()));
	connect(ui.tlbReject,SIGNAL(clicked()),SLOT(onRejectButtonClicked()));
	connect(ui.tlbSilent,SIGNAL(clicked()),SLOT(onSilentButtonClicked()));

	connect(ui.tlbLocalCamera,SIGNAL(clicked(bool)),SLOT(onLocalCameraStateButtonClicked(bool)));
	connect(ui.tlbLocalMicrophone,SIGNAL(clicked(bool)),SLOT(onLocalMicrophoneStateButtonClicked(bool)));

	ui.vlcRemoteMicrophome->setMaximumValume(4.0);
	connect(ui.vlcRemoteMicrophome,SIGNAL(volumeChanged(qreal)),SLOT(onRemoteMicrophoneVolumeChanged(qreal)));

	initialize(APluginManager);

#ifdef USE_PHONON
	FMediaObject = NULL;
	FAudioOutput = NULL;
#else
	FSound = NULL;
#endif

	FMetaRoster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(streamJid()) : NULL;
	FMetaId = FMetaRoster!=NULL ? FMetaRoster->itemMetaContact(contactJid()) : QString::null;
	if (!FMetaId.isEmpty())
	{
		onMetaAvatarChanged(FMetaId);
		connect(FMetaRoster->instance(),SIGNAL(metaAvatarChanged(const QString &)),SLOT(onMetaAvatarChanged(const QString &)));
	}
	else if (FAvatars)
	{
		FAvatars->insertAutoAvatar(ui.lblAvatar,contactJid(),QSize(38,38));
	}
	else
	{
		QImage avatar = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_AVATAR_EMPTY_FEMALE, 1);
		ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
	}

	if (!FMetaId.isEmpty())
		ui.lblName->setText(FMetaContacts->metaContactName(FMetaRoster->metaContact(FMetaId)));
	else if (FGateways)
		ui.lblName->setText(FGateways->legacyIdFromUserJid(streamJid(),contactJid()));
	else
		ui.lblName->setText(contactJid().bare());

	if (FSipCall->role() == ISipCall::CR_INITIATOR)
		setWindowTitle(tr("Call to %1").arg(contactJid().full()));
	else
		setWindowTitle(tr("Call from %1").arg(contactJid().full()));

	foreach(ISipDevice::Type deviceType, QList<ISipDevice::Type>()<<ISipDevice::DT_LOCAL_CAMERA<<ISipDevice::DT_LOCAL_MICROPHONE<<ISipDevice::DT_REMOTE_MICROPHONE) {
		onCallDeviceStateChanged(deviceType,FSipCall->deviceState(deviceType)); }
	onCallStateChanged(FSipCall->state());
}

CallControlWidget::~CallControlWidget()
{
	FSipCall->instance()->deleteLater();
}

Jid CallControlWidget::streamJid() const
{
	return FSipCall->streamJid();
}

Jid CallControlWidget::contactJid() const
{
	return FSipCall->contactJid();
}

ISipCall *CallControlWidget::sipCall() const
{
	return FSipCall;
}

void CallControlWidget::playSound(const QString &ASoundKey, int ALoops)
{
	QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(ASoundKey);

#ifdef USE_PHONON
	if (!FMediaObject)
	{
		FMediaObject = new Phonon::MediaObject(this);
		FAudioOutput = new Phonon::AudioOutput(Phonon::CommunicationCategory, this);
		Phonon::createPath(FMediaObject, FAudioOutput);
	}

	FMediaObject->clear();
	FMediaObject->stop();

	if (!soundFile.isEmpty() && QFile::exists(soundFile))
	{
		Phonon::MediaSource ms(soundFile);
		for(int i=0; i<ALoops; i++)
			FMediaObject->enqueue(ms);
		FMediaObject->play();
	}
#else
	if(FSound)
	{
		FSound->stop();
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents|QEventLoop::ExcludeSocketNotifiers);
		delete FSound;
		FSound = NULL;
	}
	if (!soundFile.isEmpty() && QFile::exists(soundFile) && QSound::isAvailable())
	{
		FSound = new QSound(soundFile);
		FSound->setLoops(ALoops);
		FSound->play();
	}
#endif
}

void CallControlWidget::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IAvatars").value(0);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
	}
}
void CallControlWidget::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_NONE:
		ui.pbtAccept->setVisible(false);
		ui.pbtReject->setVisible(false);
		ui.tlbReject->setVisible(false);
		ui.tlbSilent->setVisible(false);
		ui.wdtDeviceControls->setVisible(false);
		ui.lblNotice->setText(tr("Initializing..."));
		break;
	case ISipCall::CS_CALLING:
		ui.pbtAccept->setVisible(FSipCall->role()==ISipCall::CR_RESPONDER);
		ui.pbtReject->setVisible(FSipCall->role()==ISipCall::CR_RESPONDER);
		ui.tlbReject->setVisible(FSipCall->role()==ISipCall::CR_INITIATOR);
		ui.tlbSilent->setVisible(FSipCall->role()==ISipCall::CR_RESPONDER);
		ui.wdtDeviceControls->setVisible(false);
		ui.lblNotice->setText(tr("Calling..."));
		break;
	case ISipCall::CS_CONNECTING:
		ui.pbtAccept->setVisible(false);
		ui.pbtReject->setVisible(false);
		ui.tlbReject->setVisible(true);
		ui.tlbSilent->setVisible(false);
		ui.wdtDeviceControls->setVisible(false);
		ui.lblNotice->setText(tr("Connecting..."));
		break;
	case ISipCall::CS_TALKING:
		ui.pbtAccept->setVisible(false);
		ui.pbtReject->setVisible(false);
		ui.tlbReject->setVisible(true);
		ui.tlbSilent->setVisible(false);
		ui.wdtDeviceControls->setVisible(true);
		ui.lblNotice->setText(tr("Talking"));
		break;
	case ISipCall::CS_FINISHED:
		ui.pbtAccept->setVisible(false);
		ui.pbtReject->setVisible(false);
		ui.tlbReject->setVisible(false);
		ui.tlbSilent->setVisible(false);
		ui.wdtDeviceControls->setVisible(false);
		ui.lblNotice->setText(tr("Finished"));
		break;
	case ISipCall::CS_ERROR:
		ui.pbtAccept->setVisible(false);
		ui.pbtReject->setVisible(false);
		ui.tlbReject->setVisible(false);
		ui.tlbSilent->setVisible(false);
		ui.wdtDeviceControls->setVisible(false);
		ui.lblNotice->setText(FSipCall->errorString());
		break;
	}

	if (AState == ISipCall::CS_TALKING)
		FCallTimer.start();
	else
		FCallTimer.stop();

	if (AState == ISipCall::CS_CALLING)
		playSound(FSipCall->role()==ISipCall::CR_INITIATOR ? SDF_SIPPHONE_CALL_WAIT : SDF_SIPPHONE_CALL_RINGING, 30);
	else if (AState == ISipCall::CS_TALKING)
		playSound(SDF_SIPPHONE_CALL_START);
	else if (AState == ISipCall::CS_FINISHED)
		playSound(SDF_SIPPHONE_CALL_STOP);
	else if (AState==ISipCall::CS_ERROR && FSipCall->errorCode()==ISipCall::EC_BUSY)
		playSound(SDF_SIPPHONE_CALL_BUSY,5);
	else if (AState==ISipCall::CS_ERROR && FSipCall->errorCode()==ISipCall::EC_NOANSWER)
		playSound(SDF_SIPPHONE_CALL_BUSY,5);
	else if (AState==ISipCall::CS_ERROR && FSipCall->errorCode()==ISipCall::EC_REJECTED)
		playSound(SDF_SIPPHONE_CALL_BUSY,5);
	else if(AState==ISipCall::CS_ERROR)
		playSound(SDF_SIPPHONE_CALL_STOP);
	else
		playSound(QString::null);
}

void CallControlWidget::onCallDeviceStateChanged(int AType, int AState)
{
	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
		//ui.tlbLocalCamera->setEnabled(AState!=ISipDevice::DS_UNAVAIL);
		ui.tlbLocalCamera->setChecked(AState==ISipDevice::DS_ENABLED);
		break;
	case ISipDevice::DT_LOCAL_MICROPHONE:
		//ui.tlbLocalMicrophone->setEnabled(AState!=ISipDevice::DS_UNAVAIL);
		ui.tlbLocalMicrophone->setChecked(AState==ISipDevice::DS_ENABLED);
		break;
	case ISipDevice::DT_REMOTE_MICROPHONE:
		//ui.vlcRemoteMicrophome->setEnabled(AState!=ISipDevice::DS_UNAVAIL);
		break;
	default:
		break;
	}
}

void CallControlWidget::onCallDevicePropertyChanged(int AType, int AProperty, const QVariant &AValue)
{
	if (AType==ISipDevice::DT_REMOTE_MICROPHONE && AProperty==ISipDevice::RMP_VOLUME)
	{
		ui.vlcRemoteMicrophome->setVolume(AValue.toReal());
	}
}

void CallControlWidget::onAcceptButtonClicked()
{
	FSipCall->acceptCall();
}

void CallControlWidget::onRejectButtonClicked()
{
	FSipCall->rejectCall();
}

void CallControlWidget::onSilentButtonClicked()
{
	playSound(QString::null);
}

void CallControlWidget::onLocalCameraStateButtonClicked(bool AChecked)
{
	FSipCall->setDeviceState(ISipDevice::DT_LOCAL_CAMERA, AChecked ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
}

void CallControlWidget::onLocalMicrophoneStateButtonClicked(bool AChecked)
{
	FSipCall->setDeviceState(ISipDevice::DT_LOCAL_MICROPHONE, AChecked ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
}

void CallControlWidget::onRemoteMicrophoneVolumeChanged(qreal AVolume)
{
	FSipCall->setDeviceProperty(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::RMP_VOLUME,AVolume);
}

void CallControlWidget::onCallTimerTimeout()
{
	ui.lblNotice->setText(FSipCall->callTimeString());
}

void CallControlWidget::onMetaAvatarChanged(const QString &AMetaId)
{
	if (FMetaId == AMetaId)
	{
		QImage avatar = ImageManager::roundSquared(FMetaRoster->metaAvatarImage(FMetaId, true, false), 38, 3);
		if (avatar.isNull())
			avatar = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_AVATAR_EMPTY_FEMALE, 1);
		ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
	}
}
