#ifndef SASLAUTH_H
#define SASLAUTH_H

#include <definitions/namespaces.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/log.h>

class SASLAuth :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	SASLAuth(IXmppStream *AXmppStream);
	~SASLAuth();
	virtual QObject *instance() { return this; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QString featureNS() const { return NS_FEATURE_SASL; }
	virtual IXmppStream *xmppStream() const { return FXmppStream; }
	virtual bool start(const QDomElement &AElem);
signals:
	void finished(bool ARestart);
	void error(const QString &AError);
	void featureDestroyed();
private:
	IXmppStream *FXmppStream;
private:
	int FChallengeStep;
};

#endif // SASLAUTH_H
