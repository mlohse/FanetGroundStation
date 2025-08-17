/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "transmitreply.h"
#include <QByteArray>

static const char FANET_DATA_SEP = ',';

TransmitReply::TransmitReply(const QByteArray &data) :
    GenericReply(AbstractFanetMessage::FMTFanetReply, data),
    m_addr(m_reply == GenericReply::ReplyAck || m_reply == GenericReply::ReplyNack ?
               FanetAddress(data.mid(data.indexOf(FANET_DATA_SEP) + 1)) : FanetAddress())
{
}

bool TransmitReply::isValid() const
{
	switch (m_reply)
	{
		case GenericReply::ReplyAck:
		case GenericReply::ReplyNack:
			return m_addr.isValid() && GenericReply::isValid();
		default:
			return GenericReply::isValid();
	}
}
