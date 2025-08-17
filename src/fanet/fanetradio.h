/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETRADIO_H
#define FANETRADIO_H

#include <QObject>
#include <QFlags>
#include "abstractfanetmessage.h"
#include "logger.h"
#include "config/radioconfig.h"

class FanetProtocolParser;
class FanetAddress;
class FanetPayload;
class QSerialPort;
class QTimer;
class Gpio;

class FanetRadio : public QObject
{
	Q_OBJECT

public:
	enum RadioState
	{
		RadioDisabled     = 0x00,
		RadioResetting    = 0x01,
		RadioInitializing = 0x02,
		RadioReady        = 0x03,
		// errors
		RadioError        = 0x80,
		RadioDevNotFound  = 0x81,
		RadioDevOpenFail  = 0x82,
		RadioInitTimeout  = 0x83,
		RadioComTimeout   = 0x84,
		RadioWrongFw      = 0x85
	};
	Q_ENUM(RadioState)

	explicit FanetRadio(const RadioConfig &config, Gpio *gpio, QObject *parent = nullptr);
	virtual ~FanetRadio();

	RadioState state() const { return m_state; }
	bool isReady() const { return m_state == RadioReady; }

	static QString radioStateStr(RadioState state);
	bool sendData(const FanetAddress &addr, const FanetPayload &data);

	void injectMessage(const QString &data);

	// stock firmware does not support (sender) address change needed for bradcasting weather data from different stations :(
	bool supportsAddressChange() const { return false; }

public slots:
	void init();
	void deinit();

private slots:
	void onReadyRead();
	void onTimeout();
	void onRadioInitialized(const AbstractFanetMessage *msg);
	void handleVersionReply(const AbstractFanetMessage *msg);
	void handleRegionReply(const AbstractFanetMessage *msg);
	void handleFanetReply(const AbstractFanetMessage *msg);
	void handleFanetPktRecv(const AbstractFanetMessage *msg);

signals:
	void radioStateChanged(FanetRadio::RadioState state);
	void messageReceived(quint32 addr, const FanetPayload &payload, bool broadcast);

protected:
	void setState(RadioState state);
	virtual void handleMessage(const AbstractFanetMessage *msg);
	virtual bool sendMessage(const AbstractFanetMessage *msg);

private:
	mutable Logger m_log;
	RadioConfig m_config;
	RadioState m_state;
	QSerialPort *m_uart;
	Gpio *m_gpio;
	QTimer *m_timer;
	FanetProtocolParser *m_parser;
};

#endif // FANETRADIO_H
