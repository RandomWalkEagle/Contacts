#ifndef SIPCALL_H
#define SIPCALL_H

#include <QObject>
#include <interfaces/isipphone.h>

class SipCall :
		public QObject,
		public ISipCall
{
	Q_OBJECT
	Q_INTERFACES(ISipCall)
public:
	SipCall(CallerRole role = CR_INITIATOR);
public:
	// ISipCall
	virtual QObject *instance();
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual QList<Jid> callDestinations() const;
	virtual void call(const Jid &AStreamJid, const QList<Jid> &AContacts) const;
	virtual void acceptCall();
	virtual void rejectCall(RejectionCode ACode = RC_BYUSER);
	virtual CallState state() const;
	virtual ErrorCode errorCode() const;
	virtual QString errorString() const;
	virtual CallerRole role() const;
	virtual quint32 callTime() const;
	virtual QString callTimeString() const;
	virtual bool sendDTMFSignal(QChar ASignal);
	// devices
	virtual ISipDevice activeDevice(ISipDevice::Type AType) const;
	virtual bool setActiveDevice(ISipDevice::Type AType, int ADeviceId);
	virtual DeviceState deviceState(ISipDevice::Type AType) const;
	virtual bool setDeviceState(ISipDevice::Type AType, DeviceState AState) const;
	virtual QVariant deviceProperty(ISipDevice::Type AType, int AProperty);
	virtual bool setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
public:
	// SipCall internal
	void setStreamJid(const Jid & AStreamJid);
	void setContactJid(const Jid & AContactJid);
signals:
	void stateChanged(int AState);
	void DTMFSignalReceived(QChar ASignal);
	void activeDeviceChanged(int ADeviceType);
	void deviceStateChanged(ISipDevice::Type AType, DeviceState AState);
	void devicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
public slots:
private:
	Jid callStreamJid;
	Jid callContactJid;
	QList<Jid> destinations;
	CallState currentState;
	ErrorCode currentError;
	CallerRole myRole;
	quint32 currentCallTime;
	ISipDevice camera;
	ISipDevice microphone;
	ISipDevice videoInput;
	ISipDevice audioOutput;
};

#endif // SIPCALL_H