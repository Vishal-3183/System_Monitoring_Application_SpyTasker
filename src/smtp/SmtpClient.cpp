// Source for SmtpClient
#include "SmtpClient.h"
#include <QDebug>

using namespace SmtpClientNamespace;

SmtpClient::SmtpClient(const QString &host, int port, ConnectionType connectionType)
    : m_host(host), m_port(port), m_connectionType(connectionType), socket(new QSslSocket)
{
}

SmtpClient::~SmtpClient()
{
    delete socket;
}

void SmtpClient::setUser(const QString &user)
{
    m_user = user;
}

void SmtpClient::setPassword(const QString &password)
{
    m_password = password;
}

bool SmtpClient::connectToHost()
{
    socket->connectToHostEncrypted(m_host, m_port);
    return socket->waitForEncrypted(10000);
}

bool SmtpClient::login()
{
    if (!sendCommand("EHLO localhost")) return false;
    if (!sendCommand("AUTH LOGIN")) return false;
    if (!sendCommand(m_user.toUtf8().toBase64())) return false;
    if (!sendCommand(m_password.toUtf8().toBase64())) return false;
    return true;
}

bool SmtpClient::sendMail(const MimeMessage &email)
{
    if (!sendCommand("MAIL FROM:<" + email.getSender() + ">")) return false;

    for (const EmailAddress* recipient : email.getRecipients()) {
        if (!sendCommand("RCPT TO:<" + recipient->getAddress() + ">")) return false;
    }

    if (!sendCommand("DATA")) return false;

    QString data;
    data += "Subject: " + email.getSubject() + "\r\n";
    data += "From: <" + email.getSender() + ">\r\n";

    QStringList recipientAddresses;
    for (const EmailAddress* recipient : email.getRecipients()) {
        recipientAddresses << recipient->getAddress();
    }
    data += "To: <" + recipientAddresses.join(", ") + ">\r\n";
    data += "\r\n";

    if (email.getTextPart() != nullptr) {
        data += email.getTextPart()->toString() + "\r\n";  // Use toString() here
    }

    data += "\r\n.\r\n";

    sendData(data);
    return sendCommand(".");
}

void SmtpClient::quit()
{
    sendCommand("QUIT");
    socket->disconnectFromHost();
}

bool SmtpClient::sendCommand(const QString &cmd)
{
    sendData(cmd + "\r\n");
    return waitForResponse();
}

void SmtpClient::sendData(const QString &data)
{
    socket->write(data.toUtf8());
    socket->waitForBytesWritten();
}

bool SmtpClient::waitForResponse()
{
    if (!socket->waitForReadyRead(5000)) return false;
    const QStringList response = readResponse();
    if (response.isEmpty()) return false;

    const QString code = response.first().left(3);
    return code.startsWith("2") || code.startsWith("3");
}

QString SmtpClient::readResponseLine()
{
    return QString::fromUtf8(socket->readLine()).trimmed();
}

QStringList SmtpClient::readResponse()
{
    QStringList lines;
    QString line = readResponseLine();
    while (!line.isEmpty()) {
        lines << line;
        if (line.length() < 4 || line[3] != '-') break;
        line = readResponseLine();
    }
    return lines;
}
