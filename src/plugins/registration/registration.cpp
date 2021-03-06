#include "registration.h"

#define REGISTRATION_TIMEOUT    60000

#define ADR_StreamJid           Action::DR_StreamJid
#define ADR_ServiceJid          Action::DR_Parametr1
#define ADR_Operation           Action::DR_Parametr2

Registration::Registration()
{
	FDataForms = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
}

Registration::~Registration()
{

}

void Registration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Registration");
	APluginInfo->description = tr("Allows to register on the Jabber servers and services");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Registration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

	return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool Registration::initObjects()
{
	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_REGISTER);
	}
	return true;
}

bool Registration::initSettings()
{
	return true;
}

void Registration::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FSendRequests.contains(AStanza.id()) || FSubmitRequests.contains(AStanza.id()))
	{
		QDomElement query = AStanza.firstElement("query",NS_JABBER_REGISTER);

		QDomElement formElem = query.firstChildElement("x");
		while (!formElem.isNull() && (formElem.namespaceURI()!=NS_JABBER_DATA || formElem.attribute("type",DATAFORM_TYPE_FORM)!=DATAFORM_TYPE_FORM))
			formElem = formElem.nextSiblingElement("x");

		if (FSubmitRequests.contains(AStanza.id()) && AStanza.type() == "result")
		{
			LogDetail(QString("[Registration] Registration submit accepted by '%1' id='%2'").arg(AStanza.from(),AStanza.id()));
			emit registerSuccess(AStanza.id());
		}
		else if (AStanza.type() == "result" || !formElem.isNull())
		{
			IRegisterFields fields;
			fields.fieldMask = 0;
			fields.registered = !query.firstChildElement("registered").isNull();
			fields.serviceJid = AStanza.from();
			fields.instructions = query.firstChildElement("instructions").text();
			if (!query.firstChildElement("username").isNull())
			{
				fields.fieldMask += IRegisterFields::Username;
				fields.username = query.firstChildElement("username").text();
			}
			else if (!query.firstChildElement("name").isNull())
			{
				fields.fieldMask += IRegisterFields::Username;
				fields.username = query.firstChildElement("name").text();
			}
			if (!query.firstChildElement("password").isNull())
			{
				fields.fieldMask += IRegisterFields::Password;
				fields.password = query.firstChildElement("password").text();
			}
			if (!query.firstChildElement("email").isNull())
			{
				fields.fieldMask += IRegisterFields::Email;
				fields.email = query.firstChildElement("email").text();
			}
			fields.key = query.firstChildElement("key").text();

			if (FDataForms)
				fields.form = FDataForms->dataForm(formElem);

			QDomElement oob = query.firstChildElement("x");
			while (!oob.isNull() && oob.namespaceURI()!=NS_JABBER_OOB)
				oob = oob.nextSiblingElement("x");
			if (!oob.isNull())
			{
				fields.fieldMask += IRegisterFields::URL;
				fields.url = oob.firstChildElement("url").text();
			}

			LogDetail(QString("[Registration] Registration fields received from '%1' id='%2'").arg(AStanza.from(),AStanza.id()));
			emit registerFields(AStanza.id(),fields);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[Registration] Failed to process registration request with '%1', id='%2': %3").arg(AStanza.from(),AStanza.id(),err.message()));
			emit registerError(AStanza.id(),err.condition(),err.message());
		}
		FSendRequests.removeAll(AStanza.id());
		FSubmitRequests.removeAll(AStanza.id());
	}
}

void Registration::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FSendRequests.contains(AStanzaId) || FSubmitRequests.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[Registration] Failed to process registration request id='%1': %2").arg(AStanzaId,err.message()));
		emit registerError(AStanzaId,err.condition(),err.message());
		FSendRequests.removeAll(AStanzaId);
		FSubmitRequests.removeAll(AStanzaId);
	}
}

IDataFormLocale Registration::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_REGISTER)
	{
		locale.title = tr("Registration Form");
		locale.fields["username"].label = tr("Account Name");
		locale.fields["nick"].label = tr("Nickname");
		locale.fields["password"].label = tr("Password");
		locale.fields["name"].label = tr("Full Name");
		locale.fields["first"].label = tr("First Name");
		locale.fields["last"].label = tr("Last Name");
		locale.fields["email"].label = tr("Email Address");
		locale.fields["address"].label = tr("Street");
		locale.fields["city"].label = tr("City");
		locale.fields["state"].label = tr("Region");
		locale.fields["zip"].label = tr("Zip Code");
		locale.fields["phone"].label = tr("Telephone Number");
		locale.fields["url"].label = tr("Your Web Paqe");
	}
	return locale;
}

QString Registration::sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	Stanza reg("iq");
	reg.setTo(AServiceJid.full()).setType("get").setId(FStanzaProcessor->newId());
	reg.addElement("query",NS_JABBER_REGISTER);
	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,reg,REGISTRATION_TIMEOUT))
	{
		LogDetail(QString("[Registration] Registration fields request sent to '%1', id='%2'").arg(AServiceJid.full(),reg.id()));
		FSendRequests.append(reg.id());
		return reg.id();
	}
	else
	{
		LogError(QString("[Registration] Failed to send registration fields request to '%1'").arg(AServiceJid.full()));
	}
	return QString::null;
}

QString Registration::sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	Stanza unreg("iq");
	unreg.setTo(AServiceJid.full()).setType("set").setId(FStanzaProcessor->newId());
	unreg.addElement("query",NS_JABBER_REGISTER).appendChild(unreg.createElement("remove"));
	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,unreg,REGISTRATION_TIMEOUT))
	{
		LogDetail(QString("[Registration] Unregister submit sent to '%1', id='%2'").arg(AServiceJid.full(),unreg.id()));
		FSubmitRequests.append(unreg.id());
		return unreg.id();
	}
	else
	{
		LogError(QString("[Registration] Failed to send unregister submit to '%1'").arg(AServiceJid.full()));
	}
	return QString::null;
}

QString Registration::sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword)
{
	Stanza change("iq");
	change.setTo(AServiceJid.full()).setType("set").setId(FStanzaProcessor->newId());
	QDomElement elem = change.addElement("query",NS_JABBER_REGISTER);
	elem.appendChild(change.createElement("username")).appendChild(change.createTextNode(AUserName));
	elem.appendChild(change.createElement("password")).appendChild(change.createTextNode(APassword));
	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,change,REGISTRATION_TIMEOUT))
	{
		LogDetail(QString("[Registration] Change password submit sent to '%1', id='%2'").arg(AServiceJid.full(),change.id()));
		FSubmitRequests.append(change.id());
		return change.id();
	}
	else
	{
		LogError(QString("[Registration] Failed to send change password submit to '%1'").arg(AServiceJid.full()));
	}
	return QString();
}

QString Registration::sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit)
{
	Stanza submit("iq");
	submit.setTo(ASubmit.serviceJid.full()).setType("set").setId(FStanzaProcessor->newId());
	QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
	if (ASubmit.form.type.isEmpty())
	{
		if (!ASubmit.username.isEmpty())
			query.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(ASubmit.username));
		if (!ASubmit.password.isEmpty())
			query.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(ASubmit.password));
		if (!ASubmit.email.isEmpty())
			query.appendChild(submit.createElement("email")).appendChild(submit.createTextNode(ASubmit.email));
		if (!ASubmit.key.isEmpty())
			query.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(ASubmit.key));
	}
	else if (FDataForms)
	{
		FDataForms->xmlForm(ASubmit.form,query);
	}

	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,submit,REGISTRATION_TIMEOUT))
	{
		LogDetail(QString("[Registration] Registration submit sent to '%1', id='%2'").arg(ASubmit.serviceJid.full(),submit.id()));
		FSubmitRequests.append(submit.id());
		return submit.id();
	}
	else
	{
		LogError(QString("[Registration] Failed to send registration submit to '%1'").arg(ASubmit.serviceJid.full()));
	}
	return QString();
}

void Registration::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.active = false;
	dfeature.var = NS_JABBER_REGISTER;
	dfeature.name = tr("Registration");
	dfeature.description = tr("Supports the registration");
	FDiscovery->insertDiscoFeature(dfeature);
}

Q_EXPORT_PLUGIN2(plg_registration, Registration)
