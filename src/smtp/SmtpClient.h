// Header for SmtpClient
#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H
#include <QStringList>

#include <QString>
#include <QTcpSocket>
#include <QSslSocket>
#include "MimeMessage.h"

namespace SmtpClientNamespace {

class SmtpClient
{
public:
    enum ConnectionType {
        TcpConnection,
        SslConnection,
        TlsConnection
    };

    SmtpClient(const QString &host, int port = 25, ConnectionType connectionType = TcpConnection);
    ~SmtpClient();

    void setUser(const QString &user);
    void setPassword(const QString &password);

    bool connectToHost();
    bool login();
    bool sendMail(const MimeMessage &email);
    void quit();

private:
    QString m_host;
    int m_port;
    ConnectionType m_connectionType;

    QString m_user;
    QString m_password;

    QSslSocket *socket;

    bool waitForResponse();
    bool sendCommand(const QString &cmd);
    QString readResponseLine();
    QStringList readResponse();

    void sendData(const QString &data);
};

} // namespace SmtpClientNamespace

#endif // SMTPCLIENT_H
