/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef REGIONCOMMAND_H
#define REGIONCOMMAND_H

#include "abstractfanetmessage.h"

class RegionCommand : public AbstractFanetMessage
{
public:
	enum FanetFreq
	{
		FreqInvalid = -1,
		Freq868MHz  = 0,
		Freq915Mhz  = 1
	};

	explicit RegionCommand(int txPower = -1, FanetFreq freq = FreqInvalid);
	virtual ~RegionCommand() = default;

	virtual bool isValid() const override;
	virtual QByteArray serialize() const override;

	int txPower() const { return m_txPower; }
	FanetFreq freq() const { return m_freq; }

private:
	int m_txPower;
	FanetFreq m_freq;
};

#endif // REGIONCOMMAND_H
