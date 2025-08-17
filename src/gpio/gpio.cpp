/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "gpio.h"
#include "logger.h"

#include <QSerialPort>
#if defined RPI_GPIO
    #include <bcm2835.h>
#endif


Gpio::Gpio(QObject *parent) :
    QObject(parent),
    m_uart(nullptr),
    m_initialized(false),
    m_invert()
{
#if defined RPI_GPIO
	Logger log("Gpio");
	if (bcm2835_init())
	{
		m_initialized = true;
		log.info("Library bcm2835 initialized.");
	} else
	{
		log.error("Failed to initialize lib bcm2835!");
	}
#endif
}

Gpio::~Gpio()
{
#if defined RPI_GPIO
	bcm2835_close();
#endif
}

void Gpio::initPin(GpioPin pin, GpioFunction func, bool invert)
{
	quint8 rpin = rpiPin(pin);
	if (rpin && m_initialized)
	{
		Logger("Gpio").debug(QString("Configuring pin %1 as %2%3")
		                     .arg(pinToString(pin), funcToString(func), invert ? " (inverted)" : ""));
#if defined RPI_GPIO
		bcm2835_gpio_fsel(rpin, static_cast<quint8>(func));
#endif
	}
	m_invert[pin] = invert;
}

void Gpio::setGpio(GpioPin pin, bool value)
{
#if defined RPI_GPIO
	quint8 rpin = rpiPin(pin);
	if (rpin && m_initialized)
	{
		Logger("Gpio").debug(QString("Setting gpio %1 to %2")
		                     .arg(pinToString(pin), isInverted(pin) ^ value ? "true" : "false"));
		bcm2835_gpio_write(rpin, isInverted(pin) ^ value ? HIGH : LOW);
		return;
	}
#endif
	if (m_uart && m_uart->isOpen())
	{
		switch (pin)
		{
			case PinUartRTS:
				m_uart->setRequestToSend(isInverted(pin) ^ value);
				break;
			case PinUartDTR:
				m_uart->setDataTerminalReady(isInverted(pin) ^ value);
				break;
			default:
				return;
		}
		Logger("Gpio").debug(QString("Setting uart %1 to %2")
		                     .arg(pinToString(pin), isInverted(pin) ^ value ? "true" : "false"));
	}
}

void Gpio::clearGpio(GpioPin pin)
{
	setGpio(pin, false);
}

QString Gpio::funcToString(GpioFunction func)
{
	switch (func)
	{
		case GFInput:  return "input";
		case GFOutput: return "output";
		case GFAlt0:   return "alt0";
		case GPAlt1:   return "alt1";
		case GFAlt2:   return "alt2";
		case GFAlt3:   return "alt3";
		case GFAlt4:   return "alt4";
		case GFAlt5:   return "alt5";
	}
	return "invalid";
}

QString Gpio::pinToString(GpioPin pin)
{
	switch (pin)
	{
		case PinUartCTS: return "CTS";
		case PinUartRTS: return "RTS";
		case PinUartDTR: return "DTR";
		case PinRpi03:   return "RpiJ8Pin03";
		case PinRpi05:   return "RpiJ8Pin05";
		case PinRpi07:   return "RpiJ8Pin07";
		case PinRpi08:   return "RpiJ8Pin08";
		case PinRpi10:   return "RpiJ8Pin10";
		case PinRpi11:   return "RpiJ8Pin11";
		case PinRpi12:   return "RpiJ8Pin12";
		case PinRpi13:   return "RpiJ8Pin13";
		case PinRpi15:   return "RpiJ8Pin15";
		case PinRpi16:   return "RpiJ8Pin16";
		case PinRpi18:   return "RpiJ8Pin18";
		case PinRpi19:   return "RpiJ8Pin19";
		case PinRpi21:   return "RpiJ8Pin21";
		case PinRpi22:   return "RpiJ8Pin22";
		case PinRpi23:   return "RpiJ8Pin23";
		case PinRpi24:   return "RpiJ8Pin24";
		case PinRpi26:   return "RpiJ8Pin26";
		case PinRpi29:   return "RpiJ8Pin29";
		case PinRpi31:   return "RpiJ8Pin31";
		case PinRpi32:   return "RpiJ8Pin32";
		case PinRpi33:   return "RpiJ8Pin33";
		case PinRpi35:   return "RpiJ8Pin35";
		case PinRpi36:   return "RpiJ8Pin36";
		case PinRpi37:   return "RpiJ8Pin37";
		case PinRpi38:   return "RpiJ8Pin38";
		case PinRpi40:   return "RpiJ8Pin40";
		default:         return "invalid pin";
	}
}

Gpio::GpioPin Gpio::stringToPin(const QString &pin)
{
	const QString rpiPinPrefix = "rpij8pin";
	const QString lpin = pin.toLower().trimmed();
	if (lpin.startsWith(rpiPinPrefix))
	{
		bool convOk = false;
		int num = QStringView(lpin).mid(rpiPinPrefix.size()).toInt(&convOk);
		if (convOk)
		{
			switch (num)
			{
				case  3: return PinRpi03;
				case  5: return PinRpi05;
				case  7: return PinRpi07;
				case  8: return PinRpi08;
				case 10: return PinRpi10;
				case 11: return PinRpi11;
				case 12: return PinRpi12;
				case 13: return PinRpi13;
				case 15: return PinRpi15;
				case 16: return PinRpi16;
				case 18: return PinRpi18;
				case 19: return PinRpi19;
				case 21: return PinRpi21;
				case 22: return PinRpi22;
				case 23: return PinRpi23;
				case 24: return PinRpi24;
				case 26: return PinRpi26;
				case 29: return PinRpi29;
				case 31: return PinRpi31;
				case 32: return PinRpi32;
				case 33: return PinRpi33;
				case 35: return PinRpi35;
				case 36: return PinRpi36;
				case 37: return PinRpi37;
				case 38: return PinRpi38;
				case 40: return PinRpi40;
				default: return None; // pin number doesn't exist
			}
		}
		return None; // failed to parse pin number
	}

	if (lpin == "cts") return PinUartCTS;
	if (lpin == "rts") return PinUartRTS;
	if (lpin == "dtr") return PinUartDTR;

	return None; // lookup failure
}

quint8 Gpio::rpiPin(GpioPin pin)
{
	switch (pin)
	{
#if defined RPI_GPIO
		case PinRpi03: return RPI_BPLUS_GPIO_J8_03;
		case PinRpi05: return RPI_BPLUS_GPIO_J8_05;
		case PinRpi07: return RPI_BPLUS_GPIO_J8_07;
		case PinRpi08: return RPI_BPLUS_GPIO_J8_08;
		case PinRpi10: return RPI_BPLUS_GPIO_J8_10;
		case PinRpi11: return RPI_BPLUS_GPIO_J8_11;
		case PinRpi12: return RPI_BPLUS_GPIO_J8_12;
		case PinRpi13: return RPI_BPLUS_GPIO_J8_13;
		case PinRpi15: return RPI_BPLUS_GPIO_J8_15;
		case PinRpi16: return RPI_BPLUS_GPIO_J8_16;
		case PinRpi18: return RPI_BPLUS_GPIO_J8_18;
		case PinRpi19: return RPI_BPLUS_GPIO_J8_19;
		case PinRpi21: return RPI_BPLUS_GPIO_J8_21;
		case PinRpi22: return RPI_BPLUS_GPIO_J8_22;
		case PinRpi23: return RPI_BPLUS_GPIO_J8_23;
		case PinRpi24: return RPI_BPLUS_GPIO_J8_24;
		case PinRpi26: return RPI_BPLUS_GPIO_J8_26;
		case PinRpi29: return RPI_BPLUS_GPIO_J8_29;
		case PinRpi31: return RPI_BPLUS_GPIO_J8_31;
		case PinRpi32: return RPI_BPLUS_GPIO_J8_32;
		case PinRpi33: return RPI_BPLUS_GPIO_J8_33;
		case PinRpi35: return RPI_BPLUS_GPIO_J8_35;
		case PinRpi36: return RPI_BPLUS_GPIO_J8_36;
		case PinRpi37: return RPI_BPLUS_GPIO_J8_37;
		case PinRpi38: return RPI_BPLUS_GPIO_J8_38;
		case PinRpi40: return RPI_BPLUS_GPIO_J8_40;
#endif
		default:
			return 0;
	}
}
