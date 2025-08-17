/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetprotocolparser.h"
#include "abstractfanetmessage.h"
#include "versionreply.h"
#include "transmitreply.h"
#include "receiveevent.h"

#include <QIODevice>
#include <QDebug>

static const int MSG_SIZE_IDENTIFIER                = 3;
static const char MSG_INIT_IGNORE[]                 = "CCCCCC";

const char FanetProtocolParser::FANET_REGION_CMD[]  = "DGL";
const char FanetProtocolParser::FANET_TRANSMIT_CMD[]= "FNT";
const char FanetProtocolParser::FANET_VERSION_CMD[] = "DGV";
const char FanetProtocolParser::FANET_ENABLE_CMD[]  = "DGP";
const char FanetProtocolParser::MSG_VERSION_REPLY[] = "DGV";
const char FanetProtocolParser::MSG_REGION_REPLY[]  = "DGR";
const char FanetProtocolParser::MSG_FANET_REPLY[]   = "FNR";
const char FanetProtocolParser::MSG_FANET_RECEIVE[] = "FNF";


FanetProtocolParser::FanetProtocolParser(QIODevice *dev) :
    m_log("FanetProtocolParser"),
    m_dev(dev),
    m_buffer()
{
}

AbstractFanetMessage* FanetProtocolParser::parseMessage(QByteArray data) const
{
	if (data.size() > MSG_SIZE_IDENTIFIER)
	{
		const QByteArray buf(data.trimmed());
		if (buf.startsWith(QByteArrayView(MSG_FANET_RECEIVE)))
		{
			return new ReceiveEvent(buf.mid(MSG_SIZE_IDENTIFIER));
		}
		if (buf.startsWith(QByteArrayView(MSG_FANET_REPLY)))
		{
			return new TransmitReply(buf.mid(MSG_SIZE_IDENTIFIER));
		}
		if (buf.startsWith(QByteArrayView(MSG_VERSION_REPLY)))
		{
			return new VersionReply(buf.mid(MSG_SIZE_IDENTIFIER));
		}
		if (buf.startsWith(QByteArrayView(MSG_REGION_REPLY)))
		{
			return new GenericReply(AbstractFanetMessage::FMTRegionReply, buf.mid(MSG_SIZE_IDENTIFIER));
		}
		m_log.warning(QString("Message '%1' ignored! (raw data: 0x%2)")
		              .arg(QString(buf.mid(0, MSG_SIZE_IDENTIFIER)), buf.toHex()));
	}
	return nullptr;
}

AbstractFanetMessage* FanetProtocolParser::next()
{
	while (m_dev->isReadable() && m_dev->bytesAvailable())
	{
		char byte;
		if (m_dev->getChar(&byte))
		{
			switch (byte)
			{
			case StartDelimiter:
				if (!m_buffer.isEmpty() && !m_buffer.startsWith(MSG_INIT_IGNORE)) // ignore initialization progress (CCCC...)
				{
					m_log.warning(QString("discarding incomplete message: '0x%1' ('%2')").arg(m_buffer.toHex(), m_buffer));
				}
				m_buffer.clear();
				break;
			case EndDelimiter:
			{
				// message complete, pass data to corresponding message constructor...
				m_log.debug(QString("Msg received: '%1'").arg(m_buffer));
				AbstractFanetMessage *msg = parseMessage(m_buffer);
				m_buffer.clear();
				return msg;
			}
			default:
				m_buffer.append(byte);
				break;
			}
		}
	}

	return 0;
}
