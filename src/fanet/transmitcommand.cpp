/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "transmitcommand.h"
#include "fanetprotocolparser.h"
#include <QByteArray>


TransmitCommand::TransmitCommand(const FanetAddress &addr, const FanetPayload &payload) :
    AbstractFanetMessage(AbstractFanetMessage::FMTTransmitCommand),
    m_addr(addr),
    m_payload(payload)
{
}

QByteArray TransmitCommand::serialize() const
{
	if (m_payload.type() == FanetPayload::PTInvalid)
	{
		return QByteArray();
	}
	// format: "FNT type,dest_manufacturer,dest_id,forward(0/1),req.ack(0/1),payload_length,payload_hex<,signature>"
	return QString("%1 %2,%3,%4,%5,%6,%7").arg(
	            FanetProtocolParser::FANET_TRANSMIT_CMD,
	            QString::number(m_payload.type()),
	            QString::fromLatin1(m_addr.toHex()),
	            QString(m_addr.isBroadcast() ? "0" : "1"), // forward
	            QString(m_addr.isBroadcast() ? "0" : "1"), // req. ack
	            QString::number(m_payload.data().length(), 16),
	            QString::fromLatin1(m_payload.data().toHex())).toLatin1();
}
