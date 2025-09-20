// Header for MimeMessage
#ifndef MIMEMESSAGE_H
#define MIMEMESSAGE_H

#include <QString>
#include <QList>
#include "EmailAddress.h"
#include "MimeText.h"

class MimeMessage
{
public:
    MimeMessage();

    void setSender(EmailAddress* address);
    void addRecipient(EmailAddress* address);
    void setSubject(const QString &subject);
    void addPart(MimeText* part);

    QString getSender() const;
    QList<EmailAddress*> getRecipients() const;
    QString getSubject() const;
    MimeText* getTextPart() const;

private:
    EmailAddress* m_sender;
    QList<EmailAddress*> m_recipients;
    QString m_subject;
    MimeText* m_text;
};

#endif // MIMEMESSAGE_H
