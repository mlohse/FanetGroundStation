/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FAGSCONFIG_H
#define FAGSCONFIG_H

#include <QSharedData>
#include <QString>

#include "radioconfig.h"
#include "fanetconfig.h"
#include "stationconfig.h"

class Logger;
class QXmlStreamReader;

class FagsConfigData : public QSharedData
{
public:
	FagsConfigData(const FagsConfigData &other);
	FagsConfigData() = default;
	~FagsConfigData() = default;

	int majorVer;
	int minorVer;
	RadioConfig radio;
	FanetConfig fanet;
	StationConfigList stations;
};

class FagsConfig
{
public:
	static const int VERSION_INVALID = -1;

	explicit FagsConfig(const QString &xmlFile);
	FagsConfig(const FagsConfig &other) : m_d(other.m_d) {}
	FagsConfig() = default;
	virtual ~FagsConfig() = default;

	bool isValid() const { return m_d != nullptr; }
	int majorVersion() const { return m_d ? m_d->majorVer : VERSION_INVALID; }
	int minorVersion() const { return m_d ? m_d->minorVer : VERSION_INVALID; }

	RadioConfig radio() const { return m_d ? m_d->radio : RadioConfig(); }
	FanetConfig fanet() const { return m_d ? m_d->fanet : FanetConfig(); }
	StationConfigList stations() const { return m_d ? m_d->stations : StationConfigList(); }

private:
	bool parseElementFags(QXmlStreamReader &xml, Logger &log);
	bool parseElementRadio(QXmlStreamReader &xml, Logger &log);
	bool parseElementFanet(QXmlStreamReader &xml, Logger &log);
	bool parseElementStations(QXmlStreamReader &xml, Logger &log);

	QExplicitlySharedDataPointer<FagsConfigData> m_d;
};

#endif // FAGSCONFIG_H
