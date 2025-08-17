/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "versionreply.h"
#include "fanetprotocolparser.h"

static const char FANET_VERSION_PREFIX[] = "build-";


VersionReply::VersionReply(const QByteArray &data) :
    AbstractFanetMessage(AbstractFanetMessage::FMTVersionReply),
    m_data(data.trimmed())
{
}

bool VersionReply::isValid() const
{
	return m_data.startsWith(FANET_VERSION_PREFIX) && AbstractFanetMessage::isValid();
}

QByteArray VersionReply::version() const
{
	if (m_data.startsWith(FANET_VERSION_PREFIX))
	{
		return m_data.mid(sizeof(FANET_VERSION_PREFIX) - 1); // remove "build-"
	}
	return QByteArray();
}

QByteArray VersionReply::serialize() const
{
	return QByteArray(FanetProtocolParser::MSG_VERSION_REPLY).append(' ').append(m_data);
}
