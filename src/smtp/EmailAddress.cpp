// Source for EmailAddress
#include "EmailAddress.h"

EmailAddress::EmailAddress(const QString &address, const QString &name)
    : m_address(address), m_name(name)
{
}

QString EmailAddress::getAddress() const
{
    return m_address;
}

QString EmailAddress::getName() const
{
    return m_name;
}

void EmailAddress::setAddress(const QString &address)
{
    m_address = address;
}

void EmailAddress::setName(const QString &name)
{
    m_name = name;
}
