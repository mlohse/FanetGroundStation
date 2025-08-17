/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GENERICREPLY_H
#define GENERICREPLY_H

#include "abstractfanetmessage.h"
#include <QByteArray>
#include <QString>

class GenericReply : public AbstractFanetMessage
{
public:
	enum ReplyType
	{
		ReplyOther = -1,
		ReplyOk,
		ReplyMsg,
		ReplyError,
		ReplyAck,
		ReplyNack
	};

	explicit GenericReply(AbstractFanetMessage::FanetMessageType type, const QByteArray &data = QByteArray());
	explicit GenericReply() = delete;
	virtual ~GenericReply() = default;

	virtual bool isValid() const override;
	virtual int code() const { return m_code; }
	virtual ReplyType replyType() const { return m_reply; }
	virtual QString message() const { return m_msg; }

	virtual QByteArray serialize() const override;

protected:
	const QByteArray m_data;
	int m_code;
	ReplyType m_reply;
	QString m_msg;
};

#endif // GENERICREPLY_H
