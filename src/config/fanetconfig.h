/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETCONFIG_H
#define FANETCONFIG_H

#include <QSharedData>

class Logger;
class QXmlStreamReader;

class FanetConfigData : public QSharedData
{
public:
	FanetConfigData(int ivalWeather, int ivalNames, int inactivity, int maxAge);
	FanetConfigData(const FanetConfigData &other);
	FanetConfigData();
	~FanetConfigData() = default;

	int txintervalWeather;
	int txintervalNames;
	int inactivityTimeout;
	int weatherDataMaxAge;
};

class FanetConfig
{
public:
	explicit FanetConfig(int ivalWeather, int ivalNames, int inactivity, int maxAge);
	explicit FanetConfig(QXmlStreamReader &xml);
	FanetConfig(const FanetConfig &other) : m_d(other.m_d) {}
	FanetConfig() = default;
	virtual ~FanetConfig() = default;

	bool isValid() const { return m_d != nullptr; }

	int txIntervalWeather() const { return m_d ? m_d->txintervalWeather : 0; }
	int txIntervalNames() const { return m_d ? m_d->txintervalNames : 0; }
	int inactivityTimeout() const { return m_d ? m_d->inactivityTimeout : 0; }
	int weatherDataMaxAge() const { return m_d ? m_d->weatherDataMaxAge : 0; }

private:
	QExplicitlySharedDataPointer<FanetConfigData> m_d;
};

#endif // FANETCONFIG_H
