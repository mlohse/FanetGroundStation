/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "receiveevent.h"
#include "logger.h"
#include "fanetprotocolparser.h"
#include <QByteArray>
#include <QStringList>

static const char FANET_DATA_SEP     = ',';

ReceiveEvent::ReceiveEvent(const QByteArray &data) :
    AbstractFanetMessage(AbstractFanetMessage::FMTPktReceivedEvent),
    m_addr(),
    m_payload(),
    m_sig(),
    m_broadcast(false)
{
	QStringList tmp = QString::fromLatin1(data).trimmed().split(FANET_DATA_SEP, Qt::SkipEmptyParts);
	if (tmp.size() < 7)
	{
		Logger("ReceiveEvent").warning(QString("Failed to parse fanet message: too short (%1)!")
		                               .arg(QString::fromLatin1(data)));
		return;
	}

	m_addr = FanetAddress(QString(tmp.at(0) + FANET_DATA_SEP + tmp.at(1)).toLatin1());
	m_broadcast = tmp.at(2).trimmed() == "1";
	m_sig = tmp.at(3);

	bool convOk = false;
	int iPayloadType = tmp.at(4).toInt(&convOk, 16);
	if (!convOk)
	{
		Logger("ReceiveEvent").warning(QString("Failed to parse fanet payload type (%1)!").arg(tmp.at(4)));
		return;
	}
	m_payload = FanetPayload::fromReceivedData(static_cast<FanetPayload::PayloadType>(iPayloadType), QByteArray::fromHex(tmp.at(6).toLatin1()));
}

bool ReceiveEvent::isValid() const
{
	return m_addr.isValid() && m_payload.isValid() && AbstractFanetMessage::isValid();
}

QString ReceiveEvent::toString() const
{
	if (!isValid())
	{
		return QString();
	}

	switch (m_payload.type())
	{
		case FanetPayload::PTName:
			return QString("%1 -> name: %2").arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.name());
		case FanetPayload::PTMessage:
			return QString("%1 -> message: %2").arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.message());
		case FanetPayload::PTTracking:
			return QString("%1 -> pos: %2, altitude: %3m, speed: %4km/h, climb: %5m/s, heading: %6deg., aircraft: %7")
			        .arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.position().toString()).arg(m_payload.altitude())
			        .arg(m_payload.speed() / 10.0).arg(m_payload.climb() / 10.0).arg(m_payload.heading())
			        .arg(FanetPayload::aircraftTypeStr(m_payload.aircraftType()));
		case FanetPayload::PTThermal:
			return QString("%1 -> Thermal @ pos: %2, quality: %3%, altitude: %4m, avg. climb: %5m/s, avg. wind speed: %6km/h, avg. wind heading: %7deg.")
			        .arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.position().toString()).arg(m_payload.quality())
			        .arg(m_payload.altitude()).arg(m_payload.climb() / 10.0).arg(m_payload.speed() / 10.0).arg(m_payload.heading());
		case FanetPayload::PTGroundTracking:
			return QString("%1 -> pos: %2, type: %3")
			        .arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.position().toString(),
			             FanetPayload::groundTrackingTypeStr(m_payload.groundTrackingType()));
		case FanetPayload::PTHWInfo:
		case FanetPayload::PTHWInfoOld:
			return QString("%1 -> device: %2, firmware: %3, uptime: %4min.")
			        .arg(QString::fromLatin1(m_addr.toHex(':')),m_payload.deviceType(m_addr.manufacturerId()), m_payload.firmwareBuild())
			        .arg(m_payload.uptime());
		case FanetPayload::PTService:
			return QString("%1 -> pos: %2, temperature: %3 C, direction: %4 deg., speed: %5 km/h, gusts: %6 km/h")
			        .arg(QString::fromLatin1(m_addr.toHex(':')), m_payload.position().toString()).arg(m_payload.temperature() / 10)
			        .arg(m_payload.dir()).arg(m_payload.wind() / 10).arg(m_payload.gusts() / 10);
		default:
			Logger("ReceiveEvent").warning(QString("Failed to convert event to string: payload type (%1) not implemented")
			                               .arg(m_payload.type()));
			return QString();
	}
}

QByteArray ReceiveEvent::serialize() const
{
	QByteArray tmp(FanetProtocolParser::MSG_FANET_RECEIVE);
	tmp.append(' ').append(m_addr.toHex());
	tmp.append(FANET_DATA_SEP).append(m_broadcast ? '1' : '0');
	tmp.append(FANET_DATA_SEP).append(m_sig.toLatin1());
	tmp.append(FANET_DATA_SEP).append(QString::number(m_payload.type(), 16).toLatin1());
	tmp.append(FANET_DATA_SEP).append(QString::number(m_payload.data().size(), 16).toLatin1());
	tmp.append(FANET_DATA_SEP).append(m_payload.data().toHex());
	return tmp;
}

