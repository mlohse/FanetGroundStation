/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetaddress.h"
#include "logger.h"
#include <QByteArray>

static const quint8  MANUFACUTER_ID_INVALID = 0xff;
static const quint16 DEVICE_ID_INVALID      = 0xffff;

FanetAddress::FanetAddress(quint8 manufacturerId, quint16 deviceId) :
    m_manufacturerId(manufacturerId),
    m_deviceId(deviceId)
{
}

FanetAddress::FanetAddress(const QByteArray &data) :
    m_manufacturerId(MANUFACUTER_ID_INVALID),
    m_deviceId(DEVICE_ID_INVALID)
{
	// expected format:
	// byte 0 + 1: manufacturer ID in ASCII hex
	// byte 2:     ','
	// byte 3..6:  device ID in ASCII hex
	//     -> total size = 7
	// e.g. "11,45AA" -> manufacturerId: 0x11, deviceId: 0x45AA
	//      "B,32E"   -> manufacturerId: 0x0B, deviceId: 0x032E
	int sep = qMax(data.indexOf(','), data.indexOf(':'));
	int error = 0;
	if (sep > 0 && sep < 3 && data.size() > (sep + 1))
	{
		bool convOk;
		m_manufacturerId = data.mid(0, sep).toUShort(&convOk, 16) & 0xff;
		error += !convOk;
		m_deviceId = data.mid(sep + 1).toUShort(&convOk, 16);
		error += !convOk;
	}
	if (error)
	{
		Logger("FanetAddress").warning(QString("Failed to parse address from data: '%1'!").arg(data));
	}
}

FanetAddress::FanetAddress(quint32 addr) :
    m_manufacturerId(static_cast<quint8>((addr & 0x00ff0000) >> 16)),
    m_deviceId(static_cast<quint16>(addr & 0x0000ffff))
{
}

// FanetAddress FanetAddress::fromUint32(quint32 addr)
// {
// 	return FanetAddress(static_cast<quint8>((addr & 0x00ff0000) >> 16), static_cast<quint16>(addr & 0x0000ffff));
// }

bool FanetAddress::isValid() const
{
	return m_manufacturerId != MANUFACUTER_ID_INVALID && m_deviceId != DEVICE_ID_INVALID;
}

bool FanetAddress::isBroadcast() const
{
	return m_manufacturerId == 0 && m_deviceId == 0;
}

QString FanetAddress::manufacturerName() const
{
	// see protocol.txt
	switch (m_manufacturerId)
	{
		case 0x00: // fall
		case 0xFF: return "reserved/broadcast";
		case 0x01: return "Skytraxx";
		case 0x03: return "BitBroker.eu";
		case 0x04: return "AirWhere";
		case 0x05: return "Windline";
		case 0x06: return "Burnair.ch";
		case 0x07: return "SoftRF";
		case 0x08: return "GXAircom";
		case 0x09: return "Airtribune";
		case 0x0A: return "FLARM";
		case 0x0B: return "FlyBeeper";
		case 0x10: return "alfapilot";
		case 0x11: return "FANET+";
		case 0x20: return "XC Tracer";
		case 0xCB: return "Cloudbuddy";
		case 0xDD: // fall
		case 0xDE: // fall
		case 0xDF: // fall
		case 0xF0: return "reserved (compat.)";
		case 0xE0: return "OGN Tracker";
		case 0xE4: return "4aviation";
		case 0xFA: return "Various";
		case 0xFB: return "Expressif based stations";
		case 0xFC: // fall
		case 0xFD: return "Unregistered devices";
		case 0xFE: return "reserved/multicast";
		default:
			return "Invalid/Unknown";
	}
}

QByteArray FanetAddress::toHex(char separator) const
{
	QByteArray data;
	QByteArray device;
	QByteArray manufacturer(1, m_manufacturerId);
	device.append((m_deviceId & 0xff00) >> 8);
	device.append(m_deviceId & 0x00ff);
	data.append(manufacturer.toHex());
	data.append(separator);
	data.append(device.toHex());
	return data;
}

quint32 FanetAddress::toUInt32() const
{
	return (static_cast<quint32>(m_manufacturerId << 16) | m_deviceId);
}

