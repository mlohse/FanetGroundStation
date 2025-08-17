/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "genericreply.h"
#include "logger.h"
#include "fanetprotocolparser.h"
#include <QByteArray>
#include <QStringList>

static const int  REPLY_CODE_INVALID = -1;
static const char FANET_DATA_SEP     = ',';
static const char FANET_REPLY_OK[]   = "OK";
static const char FANET_REPLY_MSG[]  = "MSG";
static const char FANET_REPLY_ERR[]  = "ERR";
static const char FANET_REPLY_ACK[]  = "ACK";
static const char FANET_REPLY_NACK[] = "NACK";


GenericReply::GenericReply(FanetMessageType type, const QByteArray &data) :
    AbstractFanetMessage(type),
    m_data(data),
    m_code(REPLY_CODE_INVALID),
    m_reply(ReplyOther),
    m_msg()
{
	QStringList tmp = QString::fromLatin1(m_data).trimmed().split(FANET_DATA_SEP, Qt::SkipEmptyParts);
	if (!tmp.isEmpty())
	{
		if (tmp.first() == FANET_REPLY_OK)
		{
			m_reply = ReplyOk;
			return; // done - no data
		}
		if (tmp.first() == FANET_REPLY_ACK)
		{
			m_reply = ReplyAck; ///@todo parse addr
			return;
		}
		if (tmp.first() == FANET_REPLY_NACK)
		{
			m_reply = ReplyNack; ///@todo parse addr
			return;
		}
		if (tmp.first() == FANET_REPLY_MSG)
		{
			m_reply = ReplyMsg;
		}
		if (tmp.first() == FANET_REPLY_ERR)
		{
			m_reply = ReplyError;
		}
		if (tmp.size() > 2) // parse (error-)code + message if available
		{
			m_code = tmp.at(1).toInt();
			m_msg = tmp.at(2);
		}
		if (m_reply == ReplyOther)
		{
			Logger("GenericReply").error(QString("De-serialization failed: Unknown type (%1)!").arg(tmp.first()));
		}
	}
}

bool GenericReply::isValid() const
{
	return m_reply != ReplyOther && AbstractFanetMessage::isValid();
}

QByteArray GenericReply::serialize() const
{
	switch (type())
	{
		case FMTVersionReply:
			return QByteArray(FanetProtocolParser::MSG_VERSION_REPLY).append(' ').append(m_data);
		case FMTRegionReply:
			return QByteArray(FanetProtocolParser::MSG_REGION_REPLY).append(' ').append(m_data);
		case FMTFanetReply:
			return QByteArray(FanetProtocolParser::MSG_FANET_REPLY).append(' ').append(m_data);
		default:
			Logger("GenericReply").error(QString("Serialization failed: Unknown type (%1)!").arg(type()));
			break;
	}
	return QByteArray();
}

