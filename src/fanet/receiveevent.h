/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef RECEIVEEVENT_H
#define RECEIVEEVENT_H

#include "abstractfanetmessage.h"
#include "fanetaddress.h"
#include "fanetpayload.h"
#include <QByteArray>
#include <QString>

class ReceiveEvent : public AbstractFanetMessage
{
public:
	explicit ReceiveEvent(const QByteArray &data = QByteArray());
	explicit ReceiveEvent() = delete;
	virtual ~ReceiveEvent() = default;

	virtual bool isValid() const override;

	FanetAddress address() const { return m_addr; }
	FanetPayload payload() const { return m_payload; }
	bool broadcast() const { return m_broadcast; }
	QString signature() const { return m_sig; }

	QString toString() const;

	virtual QByteArray serialize() const override;

protected:
	FanetAddress m_addr;
	FanetPayload m_payload;
	QString m_sig;
	bool m_broadcast;
};

#endif // RECEIVEEVENT_H
