/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef RADIOCONFIG_H
#define RADIOCONFIG_H

#include <QSharedData>
#include <QString>

#include "gpio.h"

class Logger;
class QXmlStreamReader;

class RadioConfigData : public QSharedData
{
public:
	RadioConfigData(const QString &uart, int txpwr, int freq, Gpio::GpioPin boot, Gpio::GpioPin reset, bool invertBoot, bool invertReset);
	RadioConfigData(const RadioConfigData &other);
	RadioConfigData();
	~RadioConfigData() = default;

	QString uartDev;
	int txPower;
	int frequency;
	Gpio::GpioPin pinBoot;
	Gpio::GpioPin pinReset;
	bool invertPinBoot;
	bool invertPinReset;
};

class RadioConfig
{
public:
	explicit RadioConfig(const QString &uart, int txpwr, int freq, Gpio::GpioPin boot, Gpio::GpioPin reset, bool invertBoot, bool invertReset);
	explicit RadioConfig(QXmlStreamReader &xml);
	RadioConfig(const RadioConfig &other) : m_d(other.m_d) {}
	RadioConfig() = default;
	virtual ~RadioConfig() = default;

	bool isValid() const;

	QString uart() const { return m_d ? m_d->uartDev : QString(); }
	int txPower() const { return m_d ? m_d->txPower : 0; }
	int frequency() const { return m_d ? m_d->frequency : 0; }
	Gpio::GpioPin pinBoot() const { return m_d ? m_d->pinBoot : Gpio::None; }
	Gpio::GpioPin pinReset() const { return m_d ? m_d->pinReset : Gpio::None; }
	bool invertPinBoot() const { return m_d ? m_d->invertPinBoot : false; }
	bool invertPinReset() const { return m_d ? m_d->invertPinReset : false; }

private:
	bool parsePin(const QString &str, Gpio::GpioPin *pin, bool *inverted);
	QExplicitlySharedDataPointer<RadioConfigData> m_d;
};

#endif // RADIOCONFIG_H
