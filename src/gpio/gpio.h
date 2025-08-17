/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GPIO_H
#define GPIO_H

#include <QObject>
#include <QMap>

class QSerialPort;

class Gpio : public QObject
{
	Q_OBJECT
public:
	enum GpioPin
	{
		// special gpio
		None = 0x00,
		PinUartCTS = 0x0001, // in
		PinUartRTS = 0x0002, // out
		PinUartDTR = 0x0003, // out
		// Raspberry Pi B+ Header J8:
		PinRpi03   = 0x0103, // B+, Pin J8-03
		PinRpi05   = 0x0105, // B+, Pin J8-05
		PinRpi07   = 0x0107, // B+, Pin J8-07
		PinRpi08   = 0x0108, // B+, Pin J8-08, defaults to alt function 0 UART0_TXD
		PinRpi10   = 0x010A, // B+, Pin J8-10, defaults to alt function 0 UART0_RXD
		PinRpi11   = 0x010B, // B+, Pin J8-11
		PinRpi12   = 0x010C, // B+, Pin J8-12, can be PWM channel 0 in ALT FUN 5
		PinRpi13   = 0x010D, // B+, Pin J8-13
		PinRpi15   = 0x010F, // B+, Pin J8-15
		PinRpi16   = 0x0110, // B+, Pin J8-16
		PinRpi18   = 0x0112, // B+, Pin J8-18
		PinRpi19   = 0x0113, // B+, Pin J8-19, MOSI when SPI0 in use
		PinRpi21   = 0x0115, // B+, Pin J8-21, MISO when SPI0 in use
		PinRpi22   = 0x0116, // B+, Pin J8-22
		PinRpi23   = 0x0117, // B+, Pin J8-23, CLK when SPI0 in use
		PinRpi24   = 0x0118, // B+, Pin J8-24, CE0 when SPI0 in use
		PinRpi26   = 0x011A, // B+, Pin J8-26, CE1 when SPI0 in use
		PinRpi29   = 0x011D, // B+, Pin J8-29
		PinRpi31   = 0x011F, // B+, Pin J8-31
		PinRpi32   = 0x0120, // B+, Pin J8-32
		PinRpi33   = 0x0121, // B+, Pin J8-33
		PinRpi35   = 0x0123, // B+, Pin J8-35, can be PWM channel 1 in ALT FUN 5
		PinRpi36   = 0x0124, // B+, Pin J8-36
		PinRpi37   = 0x0125, // B+, Pin J8-37
		PinRpi38   = 0x0126, // B+, Pin J8-38
		PinRpi40   = 0x0128  // B+, Pin J8-40
	};

	enum GpioFunction
	{
		GFInput  = 0x00,
		GFOutput = 0x01,
		GFAlt0   = 0x04,
		GPAlt1   = 0x05,
		GFAlt2   = 0x06,
		GFAlt3   = 0x07,
		GFAlt4   = 0x03,
		GFAlt5   = 0x02
	};

	explicit Gpio(QObject *parent = nullptr);
	virtual ~Gpio();

	void setSerialPort(QSerialPort *uart) { m_uart = uart; }
	QSerialPort *serialPort() const { return m_uart; }

	void initPin(GpioPin pin, GpioFunction func, bool invert = false);
	void setGpio(GpioPin pin, bool value = true);
	void clearGpio(GpioPin pin);

	bool isInverted(GpioPin pin) const { return m_invert.value(pin, false); }

	static QString funcToString(GpioFunction func);
	static QString pinToString(GpioPin pin);
	static GpioPin stringToPin(const QString &pin);

protected:
	quint8 rpiPin(GpioPin pin);

private:
	QSerialPort *m_uart;
	bool m_initialized;
	QMap<GpioPin, bool> m_invert;
};

#endif // GPIO_H
