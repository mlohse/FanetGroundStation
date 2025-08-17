/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETADDRESS_H
#define FANETADDRESS_H

#include <qtypes.h>
#include <QString>
#include <QByteArray>

class FanetAddress
{
public:
	explicit FanetAddress(quint8 manufacturerId = 0, quint16 deviceId = 0);
	explicit FanetAddress(const QByteArray &data); // in format "manufacturer,id"
	explicit FanetAddress(quint32 addr);
	//static FanetAddress fromUint32(quint32 addr);

	~FanetAddress() = default;

	bool isValid() const;
	bool isBroadcast() const;
	quint8 manufacturerId() const { return m_manufacturerId; }
	quint16 deviceId() const { return m_deviceId; }

	QString manufacturerName() const;
	QByteArray toHex(char separator = ',') const;
	quint32 toUInt32() const;

private:
	quint8 m_manufacturerId;
	quint16 m_deviceId;
};

#endif // FANETADDRESS_H
