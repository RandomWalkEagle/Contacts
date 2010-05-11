#ifndef IAVATARS_H
#define IAVATARS_H

#include <QImage>
#include "../utils/jid.h"

#define AVATARTS_UUID "{22F84EAF-683E-4a20-B5E5-1FE363FD206C}"

class IAvatars {
public:
	virtual QObject *instance() =0;
	virtual QString avatarFileName(const QString &AHash) const =0;
	virtual bool hasAvatar(const QString &AHash) const =0;
	virtual QImage loadAvatar(const QString &AHash) const =0;
	virtual QString saveAvatar(const QByteArray &AImageData) const =0;
	virtual QString saveAvatar(const QImage &AImage, const char *AFormat = NULL) const =0;
	virtual QString avatarHash(const Jid &AContactJid) const =0;
	virtual QImage avatarImage(const Jid &AContactJid) const =0;
	virtual bool setAvatar(const Jid &AStreamJid, const QImage &AImage, const char *AFormat = NULL) =0;
	virtual QString setCustomPictire(const Jid &AContactJid, const QString &AImageFile) =0;
protected:
	virtual void avatarChanged(const Jid &AContactJid) =0;
};

Q_DECLARE_INTERFACE(IAvatars,"Virtus.Plugin.IAvatars/1.0")

#endif