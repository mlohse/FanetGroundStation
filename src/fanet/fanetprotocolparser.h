/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETPROTOCOLPARSER_H
#define FANETPROTOCOLPARSER_H

#include "logger.h"
#include <QByteArray>

class QIODevice;
class AbstractFanetMessage;


class FanetProtocolParser
{
public:
	enum SpecialChars
	{
		StartDelimiter = 0x23, // '#'
		EndDelimiter   = 0x0a  // '\n' (LF)
	};

	static const char FANET_REGION_CMD[];
	static const char FANET_TRANSMIT_CMD[];
	static const char FANET_VERSION_CMD[];
	static const char FANET_ENABLE_CMD[];
	static const char MSG_VERSION_REPLY[];
	static const char MSG_REGION_REPLY[];
	static const char MSG_FANET_REPLY[];
	static const char MSG_FANET_RECEIVE[];

	explicit FanetProtocolParser(QIODevice *dev = 0);

	/*!
	 * \brief next
	 * Parses the next message from the input device. \b Important: It is the responsibility
	 * of the caller to delete any message object returned by this function!
	 * \return Message object or '0' if there is not enough data to be read from the input device
	 */
	AbstractFanetMessage *next();

	AbstractFanetMessage *parseMessage(QByteArray data) const;

private:
	Q_DISABLE_COPY(FanetProtocolParser)
	mutable Logger m_log;
	QIODevice *m_dev;
	QByteArray m_buffer;
};

#endif // FANETPROTOCOLPARSER_H
