/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VERSIONREPLY_H
#define VERSIONREPLY_H

#include "abstractfanetmessage.h"
#include <QByteArray>

class VersionReply : public AbstractFanetMessage
{
public:
	explicit VersionReply(const QByteArray &data = QByteArray());
	virtual ~VersionReply() = default;

	virtual bool isValid() const override;

	QByteArray version() const;

	virtual QByteArray serialize() const override;

private:
	QByteArray m_data;
};

#endif // VERSIONREPLY_H
