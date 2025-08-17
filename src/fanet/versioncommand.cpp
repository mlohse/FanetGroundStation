/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "versioncommand.h"
#include "fanetprotocolparser.h"
#include <QByteArray>


VersionCommand::VersionCommand() :
    AbstractFanetMessage(AbstractFanetMessage::FMTVersionCommand)
{
}

QByteArray VersionCommand::serialize() const
{
    return QByteArray(FanetProtocolParser::FANET_VERSION_CMD);
}
