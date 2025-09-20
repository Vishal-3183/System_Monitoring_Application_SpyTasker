// Source for MimeMessage
#include "MimeMessage.h"

MimeMessage::MimeMessage()
    : m_sender(nullptr), m_text(nullptr)
{
}

void MimeMessage::setSender(EmailAddress* address)
{
    m_sender = address;
}

void MimeMessage::addRecipient(EmailAddress* address)
{
    m_recipients.append(address);
}

void MimeMessage::setSubject(const QString &subject)
{
    m_subject = subject;
}

void MimeMessage::addPart(MimeText* part)
{
    m_text = part;
}

QString MimeMessage::getSender() const
{
    return m_sender ? m_sender->getAddress() : QString();
}

QList<EmailAddress*> MimeMessage::getRecipients() const
{
    return m_recipients;
}

QString MimeMessage::getSubject() const
{
    return m_subject;
}

MimeText* MimeMessage::getTextPart() const
{
    return m_text;
}
