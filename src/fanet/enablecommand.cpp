/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "enablecommand.h"
#include "fanetprotocolparser.h"
#include <QByteArray>


static const char FANET_RX_ENABLE    = '1';
static const char FANET_RX_DISABLE   = '0';

EnableCommand::EnableCommand(bool enable) :
    AbstractFanetMessage(AbstractFanetMessage::FMTEnableCommand),
    m_enable(enable)
{
}

QByteArray EnableCommand::serialize() const
{
	QByteArray data(FanetProtocolParser::FANET_ENABLE_CMD);
	data.append(' ');
	data.append(m_enable ? FANET_RX_ENABLE : FANET_RX_DISABLE);
	return data;
}
