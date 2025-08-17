/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TRANSMITREPLY_H
#define TRANSMITREPLY_H

#include "genericreply.h"
#include "fanetaddress.h"
#include <QByteArray>

class TransmitReply : public GenericReply
{
public:
	explicit TransmitReply(const QByteArray &data = QByteArray());
	virtual ~TransmitReply() = default;

	virtual bool isValid() const override;

	FanetAddress address() const { return m_addr; }

private:
	FanetAddress m_addr;
};

#endif // TRANSMITREPLY_H
