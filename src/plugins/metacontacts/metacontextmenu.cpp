#include "metacontextmenu.h"

#include <QApplication>
#include <QClipboard>
#include <QStringBuilder>
#include <definitions/metaitemorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>

MetaContextMenu::MetaContextMenu(IRostersModel *AModel, IMetaContacts *AMetaContacts, IMetaTabWindow *AWindow) : Menu(AWindow->instance())
{
	FRosterIndex = NULL;
	FRostersModel = AModel;
	FMetaTabWindow = AWindow;
	FMetaContacts = AMetaContacts;

	Action *infoAction = new Action(this);
	infoAction->setText(tr("Contact information"));
	connect(infoAction, SIGNAL(triggered()), SLOT(onContactInformationAction()));
	addAction(infoAction,AG_DEFAULT);
	setDefaultAction(infoAction);

	Action *copyAction = new Action(this);
	copyAction->setText(tr("Copy information"));
	connect(copyAction, SIGNAL(triggered()), SLOT(onCopyInfoAction()));
	addAction(copyAction, AG_DEFAULT+1);

	FRenameAction = new Action(this);
	FRenameAction->setText(tr("Rename"));
	connect(FRenameAction, SIGNAL(triggered()), SLOT(onRenameAction()));
	addAction(FRenameAction, AG_DEFAULT+1);

	connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
	connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *,int)),SLOT(onRosterIndexDataChanged(IRosterIndex *,int)));
	connect(FRostersModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRosterIndexRemoved(IRosterIndex *)));

	onRosterIndexRemoved(FRosterIndex);
}

MetaContextMenu::~MetaContextMenu()
{

}

bool MetaContextMenu::isAcceptedIndex(IRosterIndex *AIndex)
{
	if (AIndex && FMetaTabWindow->metaRoster()->roster()->streamJid()==AIndex->data(RDR_STREAM_JID).toString())
	{
		QString metaId = AIndex->data(RDR_META_ID).toString();
		if (FMetaTabWindow->metaId() == metaId)
			return true;
	}
	return false;
}

void MetaContextMenu::updateMenu()
{
	if (FRosterIndex)
	{
		QString name = FRosterIndex->data(Qt::DisplayRole).toString();
		setTitle(name);

		QImage avatar = FRosterIndex->data(RDR_AVATAR_IMAGE_LARGE).value<QImage>();
		setIcon(QIcon(QPixmap::fromImage(avatar)));

		FRenameAction->setEnabled(!(FMetaContacts->editMetaContactRestrictions(FRosterIndex->data(RDR_STREAM_JID).toString(),FRosterIndex->data(RDR_META_ID).toString()) & GSR_RENAME_CONTACT));

		menuAction()->setVisible(true);
	}
	else
	{
		menuAction()->setVisible(false);
	}
	emit updated(this);
}

void MetaContextMenu::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (!FRosterIndex && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void MetaContextMenu::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == FRosterIndex)
	{
		if (ARole == RDR_META_ID)
		{
			if (isAcceptedIndex(AIndex))
				updateMenu();
			else
				onRosterIndexRemoved(FRosterIndex);
		}
		else if (ARole == RDR_NAME)
		{
			updateMenu();
		}
		else if ((ARole == RDR_AVATAR_IMAGE) || (ARole == RDR_AVATAR_IMAGE_LARGE))
		{
			updateMenu();
		}
	}
	else if (!FRosterIndex && ARole==RDR_META_ID && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void MetaContextMenu::onRosterIndexRemoved(IRosterIndex *AIndex)
{
	if (FRosterIndex == AIndex)
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_META_ID,FMetaTabWindow->metaId());
		IRosterIndex *streamIndex = FRostersModel->streamRoot(FMetaTabWindow->metaRoster()->streamJid());
		FRosterIndex = streamIndex!=NULL ? streamIndex->findChilds(findData,true).value(0) : NULL;

		updateMenu();
	}
}

void MetaContextMenu::onContactInformationAction()
{
	if (FRosterIndex)
		FMetaContacts->showMetaProfileDialog(FRosterIndex->data(RDR_STREAM_JID).toString(), FRosterIndex->data(RDR_META_ID).toString());
}

void MetaContextMenu::onCopyInfoAction()
{
	if (FRosterIndex)
	{
		QStringList text;
		IMetaRoster * mroster = FMetaContacts->findMetaRoster(FRosterIndex->data(RDR_STREAM_JID).toString());
		if (mroster)
		{
			IMetaContact contact = mroster->metaContact(FRosterIndex->data(RDR_META_ID).toString());
			text << contact.name;
			foreach (Jid jid, contact.items)
			{
				IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(jid);
				QString itemLabel;
				if (descriptor.metaOrder == MIO_SMS)
					itemLabel = tr("Phone");
				else if (descriptor.metaOrder == MIO_MAIL)
					itemLabel = tr("E-mail");
				else
					itemLabel = descriptor.name;
				QString itemName = FMetaContacts->itemFormattedLogin(jid);
				text << QString("%1: %2").arg(itemLabel, itemName);
			}
		}
		QApplication::clipboard()->setText(text.join("\r\n"));
	}
}

void MetaContextMenu::onRenameAction()
{
	if (FRosterIndex)
		FMetaContacts->showRenameContactDialog(FRosterIndex->data(RDR_STREAM_JID).toString(), FRosterIndex->data(RDR_META_ID).toString());
}
