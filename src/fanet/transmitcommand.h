/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TRANSMITCOMMAND_H
#define TRANSMITCOMMAND_H

#include "abstractfanetmessage.h"
#include "fanetpayload.h"
#include "fanetaddress.h"

class TransmitCommand : public AbstractFanetMessage
{
public:
	explicit TransmitCommand(const FanetAddress &addr, const FanetPayload &payload);
	virtual ~TransmitCommand() = default;

	virtual QByteArray serialize() const override;

private:
	FanetAddress m_addr;
	FanetPayload m_payload;
};

#endif // TRANSMITCOMMAND_H
