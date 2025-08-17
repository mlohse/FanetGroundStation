/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ENABLECOMMAND_H
#define ENABLECOMMAND_H

#include "abstractfanetmessage.h"

class EnableCommand : public AbstractFanetMessage
{
public:
	explicit EnableCommand(bool enable = false);
	virtual ~EnableCommand() = default;

	virtual QByteArray serialize() const override;

	bool enable() const { return m_enable; }

private:
	bool m_enable;
};

#endif // ENABLECOMMAND_H
