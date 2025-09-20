// Header for MimeText
#ifndef MIMETEXT_H
#define MIMETEXT_H

#include <QString>

class MimeText
{
public:
    MimeText();
    void setText(const QString &text);
    QString toString() const;

private:
    QString m_text;
};

#endif // MIMETEXT_H
