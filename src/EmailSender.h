
// #endif // EMAILSENDER_H
#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <QString>

class EmailSender
{
public:
    EmailSender(const QString& from, const QString& password);
    bool sendEmail(const QString& subject, const QString& body);

private:
    QString senderEmail;
    QString senderPassword;
    QString receiverEmail;
};

#endif // EMAILSENDER_H
