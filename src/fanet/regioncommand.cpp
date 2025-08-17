/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "regioncommand.h"
#include "fanetprotocolparser.h"
#include "logger.h"
#include <QByteArray>

static const char FANET_FREQ868[]    = "868";
static const char FANET_FREQ915[]    = "915";
static const char FANET_DATA_SEP     = ',';
static const int  FANET_TXPOWER_MIN  = 2; // in dBm
static const int  FANET_TXPOWER_MAX  = 20;


RegionCommand::RegionCommand(int txPower, FanetFreq freq) :
    AbstractFanetMessage(AbstractFanetMessage::FMTRegionCommand),
    m_txPower(txPower),
    m_freq(freq)
{
	if (txPower < FANET_TXPOWER_MIN)
	{
		m_txPower = FANET_TXPOWER_MIN;
		Logger("RegionCommand").warning(QString("Tx power (%1dBm) is below minimum! Using allowed min. tx power: %2dBm")
		                                .arg(txPower).arg(FANET_TXPOWER_MIN));
	}
	if (txPower > FANET_TXPOWER_MAX)
	{
		m_txPower = FANET_TXPOWER_MAX;
		Logger("RegionCommand").warning(QString("Tx power (%1dBm) is above maximum! Using allowed max. tx power: %2dBm")
		                                .arg(txPower).arg(FANET_TXPOWER_MAX));
	}
}

bool RegionCommand::isValid() const
{
	return (m_freq != FreqInvalid && AbstractFanetMessage::isValid());
}

QByteArray RegionCommand::serialize() const
{
	QByteArray data(FanetProtocolParser::FANET_REGION_CMD);
	data.append(' ');
	switch(m_freq)
	{
		case Freq868MHz:
			data.append(FANET_FREQ868);
			break;
		case Freq915Mhz:
			data.append(FANET_FREQ915);
			break;
		default:
			Logger("RegionCommand").error("Serialization failed! Invalid frequency!");
			return QByteArray();
			break;
	}
	data.append(FANET_DATA_SEP);
	data.append(QString::number(m_txPower).toLatin1());
	return data;
}
