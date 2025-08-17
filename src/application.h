/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/src/qtsinglecoreapplication.h"
#include "weatherstation/abstractweatherstation.h"
#include "logger.h"
#include "fanet/fanetradio.h"
#include "config/fagsconfig.h"

class QCommandLineParser;
class FanetMessageDispatcher;
class FanetPayload;
class Gpio;

class Application : public QtSingleCoreApplication
{
	Q_OBJECT
public:
	explicit Application(int &argc, char **argv);
	virtual ~Application() Q_DECL_OVERRIDE;

	void configureCmdLineParser(QCommandLineParser &parser) const;
	bool isDaemon() const { return m_daemon; }
	Gpio *gpio() const { return m_gpio; }

private slots:
	void onMessageReceived(const QString &msg);

private:
	Logger m_log;
	bool m_daemon;
	FagsConfig m_config;
	FanetRadio *m_radio;
	Gpio *m_gpio;
	FanetMessageDispatcher *m_dispatcher;
	WeatherStationList m_stations;
};

#endif // APPLICATION_H
