/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application.h"
#include "config/fagsconfig.h"
#include "fanet/fanetradio.h"
#include "fanet/fanetaddress.h"
#include "fanet/fanetpayload.h"
#include "fanetmessagedispatcher.h"
#include "gpio.h"
#include "logger.h"
#include "config.h"

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QMetaObject>
#include <QStringList>
#include <QCommandLineParser>
#include <QCommandLineOption>


Application::Application(int &argc, char **argv) :
    QtSingleCoreApplication(QString("%1").arg(APP_NAME), argc, argv),
    m_log("Application"),
    m_daemon(false),
    m_config(),
    m_radio(nullptr),
    m_gpio(nullptr),
    m_dispatcher(nullptr),
    m_stations()
{
	const QString build = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(BUILD_TIMESTAMP) * 1000).toString();
	setApplicationName(APP_NAME);
	setOrganizationName(ORG_NAME);
	setOrganizationDomain(ORG_DOMAIN);
	setApplicationVersion(QString("%1 (build: %2)").arg(VERSION, build));

	QCommandLineParser parser;
	configureCmdLineParser(parser);
	parser.addVersionOption();
	parser.addHelpOption();
	parser.process(*this); // handles '--help' and '--version', terminates application when unknown options are given

	if (isRunning())
	{
		QStringList args;
		for (int i = 0; i < argc; i++)
		{
			args << QString(argv[i]);
		}
		sendMessage(args.join(';'));
		QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection); // event loop isn't running yet
		qDebug() << "Another instance is already running. Shutting down..."; // do not instantiate logger, use qDebug instead...
		return;
	}

	m_daemon = parser.isSet("daemon");
	m_log.setLogTargets(m_daemon ? Logger::LogToSyslog : Logger::LogToConsole);
	if (parser.isSet("loglevel"))
	{
		switch (parser.value("loglevel").at(0).toLatin1())
		{
			case '0': m_log.setLogLevel(Logger::Critical); break;
			case '1': m_log.setLogLevel(Logger::Error); break;
			case '2': m_log.setLogLevel(Logger::Warning); break;
			case '3': m_log.setLogLevel(Logger::Notice); break;
			case '4': m_log.setLogLevel(Logger::Info); break;
			case '5': m_log.setLogLevel(Logger::Debug); break;
			default:
				m_log.warning("unknown loglevel: '%1' (valid value range: 0..5)");
				break;
		}
	}

	QFile pid(PID_FILE);
	if (pid.open(QIODevice::WriteOnly))
	{
		pid.write(QString::number(applicationPid()).append(QChar::LineFeed).toLatin1());
		pid.close();
	}

	if (parser.isSet("config"))
	{
		const QString xmlfile(parser.value("config"));
		m_config = FagsConfig(xmlfile);
		m_log.notice(QString(m_config.isValid() ? "config sucessfully loaded: %1" : "failed to load config: %1").arg(xmlfile));
	}

	connect(this, &QtSingleCoreApplication::messageReceived, this, &Application::onMessageReceived);
	m_log.notice(QString("Fanet Ground Station daemon version %1 (build: %2) started.").arg(VERSION, build));

	m_gpio = new Gpio(this);
	m_radio = new FanetRadio(m_config.radio(), m_gpio, this);
	foreach (const StationConfig &conf, m_config.stations())
	{
		AbstractWeatherStation *station = AbstractWeatherStation::fromConfig(conf, this);
		if (station)
			m_stations << station;
		else
			m_log.error("Failed to construct station from config!");
	}
	m_dispatcher = new FanetMessageDispatcher(m_config.fanet(), m_stations, m_radio, this);
}

Application::~Application()
{
	QFile pidfile(PID_FILE);
	if (pidfile.open(QIODevice::ReadOnly))
	{
		QString pid(pidfile.readAll());
		pidfile.close();
		if (QString::number(applicationPid()).toLatin1() == pid.trimmed()) // remove pid file if it belongs to this process only!
		{
			pidfile.remove();
		}
	}

	if (m_dispatcher)
	{
		delete m_dispatcher;
		m_dispatcher = nullptr;
	}

	foreach (AbstractWeatherStation *station, m_stations)
	{
		delete station;
	}
	m_stations.clear();

	if (m_radio)
	{
		m_radio->deinit();
		delete m_radio;
		m_radio = nullptr;
	}
	if (m_gpio)
	{
		delete m_gpio;
		m_gpio = nullptr;
	}

	m_log.destroy();
}

void Application::configureCmdLineParser(QCommandLineParser &parser) const
{
	parser.addOption(QCommandLineOption(QStringList() << "q" << "quit", "Send 'quit' commmand to running instance"));
	parser.addOption(QCommandLineOption(QStringList() << "d" << "daemon", "Run in background as daemon"));
	parser.addOption(QCommandLineOption(QStringList() << "l" << "loglevel", "Sets the max. log level [0..5]", "loglevel"));
	parser.addOption(QCommandLineOption(QStringList() << "c" << "config", "Configuration file", "config"));
	parser.addOption(QCommandLineOption(QStringList() << "m" << "message", "send message to device, format: <manufacturerId>:<deviceId> <message>, e.g. '11:1234 helloworld'", "message"));
#ifdef FANET_MSG_DEBUG
	parser.addOption(QCommandLineOption(QStringList() << "i" << "inject", "inject fanet rx message, e.g. 'FNF 11,5C0B,1,0,A,6,5006FC0A0400' (debugging)", "inject"));
#endif
	parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
}

void Application::onMessageReceived(const QString &msg)
{
	m_log.debug(QString("onMessageReceived(): %1").arg(msg));
	QStringList args(msg.split(';', Qt::SkipEmptyParts));
	QCommandLineParser parser;
	configureCmdLineParser(parser);
	parser.parse(args); // just parse, do not terminate application on '--help' and '--version'

	if (parser.isSet("quit"))
	{
		m_log.notice("Received 'quit'-command, shutting down...");
		quit();
		return;
	}
	if (parser.isSet("message"))
	{
		const QString attr = parser.value("message");
		const int index = attr.indexOf(' ');
		const QString msg(attr.mid(index + 1));
		FanetAddress addr(attr.mid(0, index).toLatin1());
		FanetPayload payload(FanetPayload::messagePayload(msg));
		if (m_radio->sendData(addr, payload))
		{
			m_log.info(QString("%1 <- message: %2").arg(QString::fromLatin1(addr.toHex(':')), msg));
		}
	}
#ifdef FANET_MSG_DEBUG
	if (m_radio && parser.isSet("inject"))
	{
		const QString msg = parser.value("inject");
		m_radio->injectMessage(msg);
	}
#endif
//	if (parser.isSet("config"))
//	{
//		QString config = parser.value("config");
//		m_log.notice(QString("reloading config: %1").arg(config));
//		m_clients.clear();
//		if (!loadConfig(config))
//		{
//			m_log.error("missing config, shutting down...");
//			quit();
//			return;
//		}
//	}
	///@todo process received cmd line options
}
