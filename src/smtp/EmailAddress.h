// Header for EmailAddress
#ifndef EMAILADDRESS_H
#define EMAILADDRESS_H

#include <QString>

class EmailAddress
{
public:
    EmailAddress(const QString &address, const QString &name = "");

    QString getAddress() const;
    QString getName() const;

    void setAddress(const QString &address);
    void setName(const QString &name);

private:
    QString m_address;
    QString m_name;
};

#endif // EMAILADDRESS_H
