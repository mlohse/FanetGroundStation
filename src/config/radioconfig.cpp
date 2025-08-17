/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "radioconfig.h"
#include "logger.h"
#include "config.h"

#include <QXmlStreamReader>
#include <QStringList>

RadioConfigData::RadioConfigData(const QString &uart, int txpwr, int freq, Gpio::GpioPin boot, Gpio::GpioPin reset, bool invertBoot, bool invertReset) :
    QSharedData(),
    uartDev(uart),
    txPower(txpwr),
    frequency(freq),
    pinBoot(boot),
    pinReset(reset),
    invertPinBoot(invertBoot),
    invertPinReset(invertReset)
{
}

RadioConfigData::RadioConfigData(const RadioConfigData &other) :
    QSharedData(other),
    uartDev(other.uartDev),
    txPower(other.txPower),
    frequency(other.frequency),
    pinBoot(other.pinBoot),
    pinReset(other.pinReset),
    invertPinBoot(other.invertPinBoot),
    invertPinReset(other.invertPinReset)
{
}

RadioConfigData::RadioConfigData() :
    QSharedData(),
    uartDev(FANET_DEV_DEFAULT),
    txPower(FANET_TXPOWER_DEFAULT),
    frequency(FANET_FREQ_DEFAULT),
    pinBoot(FANET_PIN_BOOT),
    pinReset(FANET_PIN_RESET),
    invertPinBoot(FANET_PIN_INVERT_BOOT),
    invertPinReset(FANET_PIN_INVERT_RESET)
{
}

RadioConfig::RadioConfig(const QString &uart, int txpwr, int freq, Gpio::GpioPin boot, Gpio::GpioPin reset, bool invertBoot, bool invertReset) :
    m_d(new RadioConfigData(uart, txpwr, freq, boot, reset, invertBoot, invertReset))
{
}

RadioConfig::RadioConfig(QXmlStreamReader &xml) :
    m_d(nullptr)
{
	Logger log("RadioConfig");
	QString uart;
	int txPower, freq;
	bool invertBoot, invertReset, convOk, success = false;
	Gpio::GpioPin pinBoot, pinReset;
	QXmlStreamAttributes attr = xml.attributes();
	QStringList reqAttrKeys = QStringList() << CONFIG_ATTR_UART
	                                        << CONFIG_ATTR_PINBOOT
	                                        << CONFIG_ATTR_PINRESET
	                                        << CONFIG_ATTR_TXPOWER
	                                        << CONFIG_ATTR_FREQ;
	foreach (const QString &key, reqAttrKeys)
	{
		if (!attr.hasAttribute(key))
		{
			log.error(QString("attribute '%1' is missing!").arg(key));
			return;
		}
	}

	uart = attr.value(CONFIG_ATTR_UART).toString();
	if (uart.isEmpty())
	{
		log.error("uart device empty!");
		return;
	}
	if (!parsePin(attr.value(CONFIG_ATTR_PINBOOT).toString(), &pinBoot, &invertBoot))
	{
		log.error("failed to parse pin 'boot'!");
		return;
	}
	if (!parsePin(attr.value(CONFIG_ATTR_PINRESET).toString(), &pinReset, &invertReset))
	{
		log.error("failed to parse pin 'boot'!");
		return;
	}
	txPower = attr.value(CONFIG_ATTR_TXPOWER).toInt(&convOk);
	if (!convOk || txPower < FANET_TXPOWER_MIN || txPower > FANET_TXPOWER_MAX)
	{
		log.error(QString("failed to parse txpower: '%1' (expected: %1 - %2)")
		          .arg(attr.value(CONFIG_ATTR_TXPOWER).toString()).arg(FANET_TXPOWER_MIN).arg(FANET_TXPOWER_MAX));
		return;
	}

	freq = attr.value(CONFIG_ATTR_FREQ).toInt();
	switch (freq)
	{
		case 868:
		case 915:
			break; // success
		default:
			log.error(QString("failed to parse frequency: '%1' (expected '868' or '915')")
			          .arg(attr.value(CONFIG_ATTR_FREQ).toString()));
			return;
	}

	// parse element end
	while(!success && !xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::EndElement:
				if (xml.name() != CONFIG_ELEMENT_RADIO)
				{
					log.error(QString("unexpected end element: '%1' (expected '%2')").arg(xml.name(), CONFIG_ELEMENT_RADIO));
					return;
				}
				success = true;
				break;
			case QXmlStreamReader::Characters: // no inner text expected!
				log.error(QString("unexpected text: '%1'").arg(xml.text()));
				return;
			case QXmlStreamReader::StartElement: // no child elements expected!
				log.error(QString("unexpected child element: '%1'").arg(xml.name()));
				return;
			default: // just ignore comments etc...
				break;
		}
	}

	// success :)
	m_d = new RadioConfigData(uart, txPower, freq, pinBoot, pinReset, invertBoot, invertReset);
	log.info(QString("uart=%1, txpower=%2, frequency=%3, pin_boot=%4%5, pin_reset=%6%7")
	         .arg(uart, QString::number(txPower), QString::number(freq),
	              invertBoot ? "!" : "", Gpio::pinToString(pinBoot),
	              invertReset ? "!" : "", Gpio::pinToString(pinReset)));

}

bool RadioConfig::parsePin(const QString &str, Gpio::GpioPin *pin, bool *inverted)
{
	if (!pin || !inverted)
	{
		return false;
	}

	*inverted = str.startsWith('!');
	*pin = Gpio::stringToPin(QString(str).remove('!'));
	return (*pin != Gpio::None);
}

bool RadioConfig::isValid() const
{
	return (m_d && m_d->pinBoot != Gpio::None && m_d->pinReset != Gpio::None && !m_d->uartDev.isEmpty());
}
