
#include "EmailSender.h"

#include "smtp/SmtpClient.h"
#include "smtp/MimeMessage.h"
#include "smtp/MimeText.h"
#include "smtp/EmailAddress.h"

#include <QDebug>
#include <QSettings>

EmailSender::EmailSender(const QString& from, const QString& password)
    : senderEmail(from), senderPassword(password)
{
    QSettings settings("Spy_Tasker", "Authenticator");
    receiverEmail = settings.value("authenticator/email", "").toString();

    if (receiverEmail.isEmpty()) {
        qDebug() << "[EmailSender] Warning: No receiver email set in settings.";
    }
}

bool EmailSender::sendEmail(const QString& subject, const QString& body)
{
    using namespace SmtpClientNamespace;

    qDebug() << "=== EmailSender::sendEmail ===";
    qDebug() << "To:      " << receiverEmail;
    qDebug() << "From:    " << senderEmail;
    qDebug() << "Subject: " << subject;
    qDebug() << "Body:    " << body;

    SmtpClient smtp("smtp.gmail.com", 465, SmtpClient::SslConnection);
    smtp.setUser(senderEmail);
    smtp.setPassword(senderPassword);

    MimeMessage message;
    message.setSender(new EmailAddress(senderEmail, "Spy_Tasker"));
    message.addRecipient(new EmailAddress(receiverEmail));
    message.setSubject(subject);

    MimeText* text = new MimeText();
    text->setText(body);
    message.addPart(text);

    if (!smtp.connectToHost()) {
        qDebug() << "[SMTP] Failed to connect to host.";
        return false;
    }
    if (!smtp.login()) {
        qDebug() << "[SMTP] Failed to login.";
        return false;
    }
    if (!smtp.sendMail(message)) {
        qDebug() << "[SMTP] Failed to send email.";
        return false;
    }

    smtp.quit();
    return true;
}
