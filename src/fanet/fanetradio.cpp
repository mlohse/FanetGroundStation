/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetradio.h"
#include "fanetaddress.h"
#include "fanetpayload.h"
#include "abstractfanetmessage.h"
#include "fanetprotocolparser.h"
#include "transmitcommand.h"
#include "transmitreply.h"
#include "enablecommand.h"
#include "regioncommand.h"
#include "versioncommand.h"
#include "versionreply.h"
#include "receiveevent.h"
#include "config.h"
#include "gpio.h"

#include <QSerialPort>
#include <QIODevice>
#include <QTimer>
#include <QFile>

#include <QCoreApplication>

static const int FANET_RESET_MSEC           = 250;
static const int FANET_INIT_TIMEOUT_MSEC    = 10 * 1000; // 10sec.
static const int FANET_COM_TIMEOUT_MSEC     = 3 * 1000; // 3 sec.
static const int FANET_MSG_CODE_INITIALIZED = 1;
static const char FANET_EXPECTED_FW[]       = "202201131742";


FanetRadio::FanetRadio(const RadioConfig &config, Gpio *gpio, QObject *parent) :
    QObject(parent),
    m_log(QString("FanetRadio")),
    m_config(config),
    m_state(RadioDisabled),
    m_uart(new QSerialPort(config.uart(), this)),
    m_gpio(gpio),
    m_timer(new QTimer(this)),
    m_parser(new FanetProtocolParser(m_uart))
{
	if (gpio)
	{
		gpio->setSerialPort(m_uart);
		gpio->initPin(config.pinBoot(), Gpio::GFOutput, config.invertPinBoot());
		gpio->initPin(config.pinReset(), Gpio::GFOutput, config.invertPinReset());
		// LEDs are available on Raspi only...
		gpio->initPin(LED_PIN_GREEN, Gpio::GFOutput);
		gpio->initPin(LED_PIN_RED, Gpio::GFOutput);
		gpio->clearGpio(LED_PIN_GREEN);
		gpio->clearGpio(LED_PIN_RED);
	}

	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &FanetRadio::onTimeout);
	connect(m_uart, &QIODevice::readyRead, this, &FanetRadio::onReadyRead);
}

FanetRadio::~FanetRadio()
{
	if (m_gpio)
	{
		m_gpio->setSerialPort(nullptr);
	}
	if (m_uart && m_uart->isOpen())
	{
		if (m_uart->isOpen())
		{
			m_uart->close();
		}
		delete m_uart;
		m_uart = nullptr;
	}
	delete m_parser;
	m_parser = nullptr;
}

QString FanetRadio::radioStateStr(RadioState state)
{
	switch (state)
	{
		case RadioDisabled:     return "disabled";
		case RadioResetting:    return "resetting";
		case RadioInitializing: return "initializing";
		case RadioReady:        return "ready";
		case RadioError:        return "error";
		case RadioDevNotFound:  return "device not found";
		case RadioDevOpenFail:  return "device open failed";
		case RadioInitTimeout:  return "initialization timeout";
		case RadioComTimeout:   return "communication timeout";
		case RadioWrongFw:      return "wrong firmware version";
		default:                return "unknown/invalid";
	}
}

bool FanetRadio::sendData(const FanetAddress &addr, const FanetPayload &data)
{
	if (!addr.isValid())
	{
		m_log.warning("Failed to send data: invalid address!");
		return false;
	}
	if (m_state != RadioReady)
	{
		m_log.warning(QString("Failed to send data (%1) to '%2': Radio is not ready (current state: %3)")
		              .arg(FanetPayload::payloadTypeStr(data.type()), addr.toHex(':'), radioStateStr(m_state)));
		return false;
	}
	if (m_gpio)
	{
		m_gpio->setGpio(LED_PIN_GREEN);
	}
	const TransmitCommand cmd(addr, data);
	return sendMessage(&cmd);
}

void FanetRadio::injectMessage(const QString &data)
{
	AbstractFanetMessage *msg = m_parser->parseMessage(data.toLatin1());
	if (msg)
	{
		handleMessage(msg);
	}
}

void FanetRadio::setState(RadioState state)
{
	if (state != m_state)
	{
		m_log.info(QString("radio state changed: %1 -> %2").arg(radioStateStr(m_state), radioStateStr(state)));
		if (m_gpio)
		{
			switch (state)
			{
				case RadioResetting:
					m_gpio->setGpio(LED_PIN_GREEN, true);
					break;
				case RadioReady:
					m_gpio->setGpio(LED_PIN_GREEN, false);
					break;
				case RadioError: // fall
				case RadioDevNotFound: // fall
				case RadioDevOpenFail: // fall
				case RadioInitTimeout: // fall
				case RadioComTimeout: // fall
				case RadioWrongFw:
					m_gpio->clearGpio(LED_PIN_GREEN);
					m_gpio->setGpio(LED_PIN_RED);
					break;
				default:
					m_gpio->clearGpio(LED_PIN_RED);
					break;
			}
		}
		m_state = state;
		emit radioStateChanged(state);
	}
}

void FanetRadio::handleMessage(const AbstractFanetMessage *msg)
{
	switch (msg->type())
	{
		case AbstractFanetMessage::FMTPktReceivedEvent:
			handleFanetPktRecv(msg);
			break;
		case AbstractFanetMessage::FMTFanetReply:
			if (m_state == RadioInitializing)
			{
				onRadioInitialized(msg);
				break;
			}
			handleFanetReply(msg);
			break;
		case AbstractFanetMessage::FMTRegionReply:
			handleRegionReply(msg);
			break;
		case AbstractFanetMessage::FMTVersionReply:
			handleVersionReply(msg);
			break;
		default:
			m_log.debug(QString("ignored unexpected fanet message (type: %1)").arg(msg->type()));
			break;
	}
}

bool FanetRadio::sendMessage(const AbstractFanetMessage *msg)
{
	if (msg && msg->isValid() && msg->isCommand())
	{
		if (!m_uart->isOpen() && m_uart->isWritable())
		{
			m_log.error(QString("Cannot write to radio! Message dropped: '%1'").arg(msg->serialize()));
			return false;
		}
		const QByteArray buf = QByteArray(1, static_cast<char>(FanetProtocolParser::StartDelimiter))
		        .append(msg->serialize())
		        .append(static_cast<char>(FanetProtocolParser::EndDelimiter));
		m_log.debug(QString("Sending message: '%1'").arg(buf.trimmed()));
		if (m_uart->write(buf) != buf.length())
		{
			m_timer->stop();
			m_log.error(QString("Failed to write to radio: %1").arg(m_uart->errorString()));
			setState(RadioError);
			return false;
		}
		return true;
	}
	return false;
}

void FanetRadio::onRadioInitialized(const AbstractFanetMessage *msg)
{
	const TransmitReply *reply = dynamic_cast<const TransmitReply*>(msg);
	if (!reply || reply->replyType() != GenericReply::ReplyMsg ||
	        reply->code() != FANET_MSG_CODE_INITIALIZED)
	{
		m_log.warning(QString("received unexpected message: %1").arg(msg->serialize()));
		return;
	}

	m_log.info("Radio found, checking firmware version...");
	m_timer->start(FANET_COM_TIMEOUT_MSEC);
	const VersionCommand vercmd;
	sendMessage(&vercmd);
}

void FanetRadio::handleVersionReply(const AbstractFanetMessage *msg)
{
	const VersionReply *reply = dynamic_cast<const VersionReply*>(msg);
	m_timer->stop();
	if (!reply || reply->version().isEmpty())
	{
		m_log.error(QString("Radio firmware version check failed!"));
		setState(RadioWrongFw);
		return;
	}

	if (reply->version() != FANET_EXPECTED_FW)
	{
		m_log.error(QString("Wrong radio firmware version: '%1' (expected: '%2')").arg(reply->version(), FANET_EXPECTED_FW));
		setState(RadioWrongFw);
		return;
	}
	m_log.notice(QString("Firmware version: %1").arg(reply->version()));

	QString freqStr;
	RegionCommand::FanetFreq freq;
	switch (m_config.frequency())
	{
		case 915:
			freq = RegionCommand::Freq915Mhz;
			freqStr = "915MHz";
			break;
		case 868: // fall
		default:
			freq = RegionCommand::Freq868MHz;
			freqStr = "868MHz";
			break;
	}

	RegionCommand cmd(m_config.txPower(), freq);
	m_log.info(QString("Setting radio region: tx-power=%1dBm, frequency=%2").arg(cmd.txPower()).arg(freqStr));
	sendMessage(&cmd);
	m_timer->start(FANET_COM_TIMEOUT_MSEC);
}

void FanetRadio::handleRegionReply(const AbstractFanetMessage *msg)
{
	const GenericReply *reply = dynamic_cast<const GenericReply*>(msg);
	m_timer->stop();
	if (!reply || reply->replyType() != GenericReply::ReplyOk)
	{
		m_log.error(QString("Failed to set radio region/enabled: %1 - %2")
		            .arg(reply? reply->code() : -1).arg(reply ? reply->message() : ""));
		setState(RadioError);
		return;
	}

	if (m_state == RadioInitializing)
	{
		EnableCommand cmd(true);
		sendMessage(&cmd);
		m_timer->start(FANET_COM_TIMEOUT_MSEC);
		m_log.notice("Radio ready.");
		setState(RadioReady);
	}
}

void FanetRadio::handleFanetReply(const AbstractFanetMessage *msg)
{
	const GenericReply *reply = dynamic_cast<const GenericReply*>(msg);
	if (reply && reply->isReply())
	{
		if (m_gpio)
		{
			m_gpio->clearGpio(LED_PIN_GREEN);
		}
		switch (reply->replyType())
		{
			case GenericReply::ReplyOk:
				m_log.debug(QString("Fanet command reply: ok"));
				break;
			case GenericReply::ReplyMsg:
				m_log.info(QString("Fanet command reply: %1 - %2").arg(reply->code()).arg(reply->message()));
				break;
			case GenericReply::ReplyAck:
				m_log.debug(QString("Fanet command: ack"));
				break;
			case GenericReply::ReplyNack:
				m_log.debug(QString("Fanet command: nack"));
				break;
			case GenericReply::ReplyError:
				m_log.error(QString("Fanet command failed: %1 - %2")
				            .arg(reply? reply->code() : -1).arg(reply ? reply->message() : ""));
				setState(RadioError);
				break;
			default:
				m_log.error("Unknown reply");
				break;
		}
	}
}

void FanetRadio::handleFanetPktRecv(const AbstractFanetMessage *msg)
{
	const ReceiveEvent *event = dynamic_cast<const ReceiveEvent*>(msg);
	if (event && event->isValid())
	{
		m_log.info(event->toString());
		emit messageReceived(event->address().toUInt32(), event->payload(), event->broadcast());
	}
}

void FanetRadio::init()
{
	if (!m_uart)
	{
		return; // should never happen, but just in case...
	}

	if (m_uart->isOpen())
	{
		deinit();
	}

	setState(RadioResetting);

	// configure serial port...
	m_uart->setBaudRate(FANET_BAUDRATE);
	m_uart->setDataBits(FANET_DATABITS);
	m_uart->setParity(FANET_PARITY);
	m_uart->setFlowControl(FANET_FLOWTYPE);

	if (!m_uart->open(QIODevice::ReadWrite))
	{
		setState(m_uart->error() == QSerialPort::DeviceNotFoundError ? RadioDevNotFound : RadioDevOpenFail);
		m_log.error(QString("failed to open serial port: %1 (%2)").arg(m_uart->errorString()).arg(m_uart->error()));
		return;
	}

	m_log.info(QString("serial port opened: %1, resetting radio...").arg(m_uart->portName()));
	if (m_gpio)
	{
		m_gpio->setGpio(FANET_PIN_BOOT);
		m_gpio->clearGpio(FANET_PIN_RESET);
	}
	m_timer->start(FANET_RESET_MSEC);

	// {
	// 	if (!m_dev || !m_dev->open(QIODevice::ReadWrite))
	// 	{
	// 		setState(RadioDevOpenFail);
	// 		m_log.error(QString("failed to open device: %1").arg(m_dev ? m_dev->errorString() : QString("null")));
	// 		return;
	// 	}
	// 	m_log.info("radio device opened.");
	// 	m_timer->start(FANET_RESET_MSEC);
	// }
}

void FanetRadio::deinit()
{
	if (m_gpio)
	{
		m_gpio->clearGpio(FANET_PIN_RESET);
	}
	if (m_timer->isActive())
	{
		m_timer->stop();
	}
	if (m_uart && m_uart->isOpen())
	{
		m_uart->close();
	}
	setState(RadioDisabled);
}

void FanetRadio::onTimeout()
{
	switch (m_state)
	{
		case RadioResetting:
			if (m_gpio)
			{
				m_gpio->setGpio(FANET_PIN_RESET);
			}
			setState(RadioInitializing);
			m_timer->start(FANET_INIT_TIMEOUT_MSEC); // wait for "#FNR MSG,1,initialized\n" -> onRadioInitialized()
			break;
		case RadioInitializing:
			m_log.error("Timeout initializing radio!");
			setState(RadioInitTimeout);
			break;
		case RadioReady:
			m_log.error("communication with radio timed out!");
			setState(RadioComTimeout);
			break;
		default:
			break;
	}
}

void FanetRadio::onReadyRead()
{
	AbstractFanetMessage *msg;
	while ((msg = m_parser->next()))
	{
		if (msg->isValid())
		{
			handleMessage(msg);
		}
		delete msg;
		msg = 0;
	}
}
