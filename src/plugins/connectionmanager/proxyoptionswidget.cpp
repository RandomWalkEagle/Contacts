#include "proxyoptionswidget.h"

#define MANUAL_PROXY_REF_UUID "{5acc925d-9729-4997-9fbd-0771554bf94d}"

ProxyOptionsWidget::ProxyOptionsWidget(IConnectionManager *AManager, OptionsNode ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;
	FConnectionNode = ANode;

	QList<QUuid> proxies = FManager->proxyList();
	ui.rdbExplorerProxy->setVisible(proxies.contains(IEXPLORER_PROXY_REF_UUID));
	ui.rdbFireFoxProxy->setVisible(proxies.contains(FIREFOX_PROXY_REF_UUID));

	connect(ui.rdbAutoProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbExplorerProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbFireFoxProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbManualProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.spbProxyPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbProxyUserPassword,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyUser,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneProxyPassword,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

	reset();
}

void ProxyOptionsWidget::apply()
{
	if (ui.rdbAutoProxy->isChecked())
		FConnectionNode.setValue(APPLICATION_PROXY_REF_UUID, "proxy");
	else if (ui.rdbExplorerProxy->isChecked())
		FConnectionNode.setValue(IEXPLORER_PROXY_REF_UUID, "proxy");
	else if (ui.rdbFireFoxProxy->isChecked())
		FConnectionNode.setValue(FIREFOX_PROXY_REF_UUID, "proxy");
	else
		FConnectionNode.setValue(MANUAL_PROXY_REF_UUID, "proxy");

	IConnectionProxy proxy;
	proxy.name = tr("Manual Proxy");
	proxy.proxy.setType(QNetworkProxy::HttpProxy);
	proxy.proxy.setHostName(ui.lneProxyHost->text());
	proxy.proxy.setPort(ui.spbProxyPort->value());
	proxy.proxy.setUser(ui.chbProxyUserPassword->isChecked() ? ui.lneProxyUser->text() : QString::null);
	proxy.proxy.setPassword(ui.chbProxyUserPassword->isChecked() ? ui.lneProxyPassword->text() : QString::null);
	FManager->setProxy(MANUAL_PROXY_REF_UUID, proxy);

	emit childApply();
}

void ProxyOptionsWidget::reset()
{
	QString proxyId = FConnectionNode.node("proxy").value().toString();
	if (proxyId == APPLICATION_PROXY_REF_UUID)
		ui.rdbAutoProxy->setChecked(true);
	else if (proxyId==IEXPLORER_PROXY_REF_UUID && ui.rdbExplorerProxy->isVisible())
		ui.rdbExplorerProxy->setChecked(true);
	else if (proxyId==FIREFOX_PROXY_REF_UUID && ui.rdbFireFoxProxy->isVisible())
		ui.rdbFireFoxProxy->setChecked(true);
	else
		ui.rdbManualProxy->setChecked(true);

	IConnectionProxy proxy = FManager->proxyById(MANUAL_PROXY_REF_UUID);
	ui.lneProxyHost->setText(proxy.proxy.hostName());
	ui.spbProxyPort->setValue(proxy.proxy.port());
	ui.lneProxyUser->setText(proxy.proxy.user());
	ui.lneProxyPassword->setText(proxy.proxy.password());
	ui.chbProxyUserPassword->setChecked(!ui.lneProxyUser->text().isEmpty());

	emit childReset();
}
