/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ABSTRACTFANETMESSAGE_H
#define ABSTRACTFANETMESSAGE_H

#include <QByteArray>

class AbstractFanetMessage
{
public:
	enum FanetMessageType
	{
		FMTInvalid          = 0x00,
		// commands/command acks
		FMTVersionCommand   = 0x01,
		FMTRegionCommand    = 0x02,
		FMTEnableCommand    = 0x03,
		FMTTransmitCommand  = 0x04,
		//replies
		FMTReply            = 0x40,
		FMTVersionReply     = 0x41,
		FMTRegionReply      = 0x42,
		FMTFanetReply       = 0x44,
		// events/event acks
		FMTEvent            = 0x80,
		FMTPktReceivedEvent = 0x81
	};

	explicit AbstractFanetMessage(FanetMessageType type = FMTInvalid) : m_type(type) {}
	virtual ~AbstractFanetMessage() = default;

	FanetMessageType type() const { return m_type; }
	virtual bool isValid() const { return m_type != FMTInvalid; }
	bool isEvent() const { return m_type & FMTEvent; }
	bool isReply() const { return m_type & FMTReply; }
	bool isCommand() const { return m_type > FMTInvalid && m_type < FMTReply; }

	virtual QByteArray serialize() const = 0;

private:
	FanetMessageType m_type;
};

#endif // ABSTRACTFANETMESSAGE_H
