// Source for MimeText
#include "MimeText.h"

MimeText::MimeText()
{
}

void MimeText::setText(const QString &text)
{
    m_text = text;
}

QString MimeText::toString() const
{
    return m_text;
}
