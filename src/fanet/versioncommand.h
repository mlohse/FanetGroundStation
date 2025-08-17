/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VERSIONCOMMAND_H
#define VERSIONCOMMAND_H

#include "abstractfanetmessage.h"

class VersionCommand : public AbstractFanetMessage
{
public:
	explicit VersionCommand();
	virtual ~VersionCommand() = default;

	virtual QByteArray serialize() const override;
};

#endif // VERSIONCOMMAND_H
