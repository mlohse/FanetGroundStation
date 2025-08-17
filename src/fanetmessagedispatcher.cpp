/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetmessagedispatcher.h"
#include "fanet/fanetpayload.h"
#include "fanet/fanetaddress.h"

FanetMessageDispatcher::FanetMessageDispatcher(const FanetConfig &config, const WeatherStationList &stations, FanetRadio *radio, QObject *parent) :
    QObject(parent),
    m_log("FanetMessageDispatcher"),
    m_config(config),
    m_radio(radio),
    m_stations(stations),
    m_lastNodeSeen(),
    m_lastWeatherUpdate(),
    m_lastNameUpdate(),
    m_timer(new QTimer(this))
{
	connect(m_timer, &QTimer::timeout, this, &FanetMessageDispatcher::onTimeout);

	if (radio)
	{
		connect(radio, &FanetRadio::radioStateChanged, this, &FanetMessageDispatcher::onRadioStateChanged);
		connect(radio, &FanetRadio::messageReceived, this, &FanetMessageDispatcher::onFanetMessageReceived);
		radio->init();
	}
}

FanetMessageDispatcher::~FanetMessageDispatcher()
{
}

void FanetMessageDispatcher::sendWeatherData()
{
	m_lastWeatherUpdate = QDateTime::currentDateTimeUtc();
	const QDateTime maxAge = m_lastWeatherUpdate.addSecs(-m_config.weatherDataMaxAge());
	foreach (AbstractWeatherStation *station, m_stations)
	{
		if (station->lastUpdate() > maxAge)
		{
			const FanetPayload::ServiceHeaderFlags header(FanetPayload::SHWind | (station->availableData() & AbstractWeatherStation::Temperature ? FanetPayload::SHTemperature : 0));
			const QGeoCoordinate pos(station->config().position());
			const FanetPayload data(FanetPayload::servicePayload(header, pos, station->temperature(), station->windDirection(), station->windSpeed(), station->windGusts(), 0, 0));
			const FanetAddress bcAddr;

			/// @todo set fanet address of sender (1 address per station needed!) here, once supported
			m_radio->sendData(bcAddr, data);
		} else
		{
			m_log.debug(QString("Not sending weather data for station #%1 (%2): station has outdated data (last update: %1)")
			            .arg(station->stationId()).arg(station->stationName(), station->lastUpdate().toString()));
		}
		if (!m_radio->supportsAddressChange())
		{
			return; // skip other stations as radio does not support address change
		}
	}
}

void FanetMessageDispatcher::sendStationNames()
{
	m_lastNameUpdate = QDateTime::currentDateTimeUtc();
	foreach (AbstractWeatherStation *station, m_stations)
	{
		if (!station->stationName().isEmpty())
		{
			const FanetPayload name(FanetPayload::namePayload(station->stationName()));
			const FanetAddress bcAddr;

			/// @todo set fanet address of sender (1 address per station needed!) here, once supported
			m_radio->sendData(bcAddr, name);
		}

		if (!m_radio->supportsAddressChange())
		{
			return; // skip other stations as radio does not support address change
		}
	}
}

void FanetMessageDispatcher::onTimeout()
{
	const QDateTime current = QDateTime::currentDateTimeUtc();
	if (m_config.inactivityTimeout() > 0 && (!m_lastNodeSeen.isValid() || m_lastNodeSeen.secsTo(QDateTime::currentDateTimeUtc()) > m_config.inactivityTimeout()))
	{
		m_log.info(QString("No Fanet nodes seen within the last %1 minutes, disabling weather data broadcasting...")
		           .arg(m_config.inactivityTimeout() / 60));
		disableWeatherUpdates();
		return;
	}

	if (m_config.txIntervalNames() > 0 && (!m_lastNameUpdate.isValid() || m_lastNameUpdate.secsTo(current) > m_config.txIntervalNames()))
	{
		sendStationNames();
	}
	if (m_config.txIntervalWeather() > 0 && (!m_lastWeatherUpdate.isValid() || m_lastWeatherUpdate.secsTo(current) > m_config.txIntervalWeather()))
	{
		sendWeatherData();
	}
}

void FanetMessageDispatcher::disableWeatherUpdates()
{
	m_log.debug("Disabling weather updates...");
	foreach (AbstractWeatherStation *station, m_stations)
	{
		station->setUpdateInterval(0);
	}
	m_timer->stop();
}

void FanetMessageDispatcher::enabledWeatherUpdates()
{
	m_log.debug("Enabling weather updates...");
	foreach (AbstractWeatherStation *station, m_stations)
	{
		const StationConfig &config = station->config();
		station->setUpdateInterval(config.updateInterval());
		station->update();
	}
	m_timer->start(1000);
}

void FanetMessageDispatcher::onRadioStateChanged(FanetRadio::RadioState state)
{
	switch (state)
	{
		case FanetRadio::RadioReady:
			if (!m_radio->supportsAddressChange() && m_stations.size() > 1)
			{
				m_log.warning("Multiple weather stations configured but radio firmware does not support address change. "
				              "Bradcasting data from 1st weather station via fanet only!");
			}
			enabledWeatherUpdates();
			break;
		case FanetRadio::RadioError: // fall
		case FanetRadio::RadioComTimeout:
			m_log.error("Fanet radio has gone into error state!");
			disableWeatherUpdates();
			m_log.info("Trying to re-initialize radio...");
			m_radio->init();
			break;
		case FanetRadio::RadioDevNotFound:
		case FanetRadio::RadioDevOpenFail:
		case FanetRadio::RadioInitTimeout:
		case FanetRadio::RadioWrongFw:
			m_log.critical("Unrecoverable radio error!");
			break;
		default:
			break;
	}
}

void FanetMessageDispatcher::onFanetMessageReceived(quint32 addr, const FanetPayload &payload, bool broadcast)
{
	Q_UNUSED(broadcast)
	switch (payload.type())
	{
		case FanetPayload::PTTracking:
		case FanetPayload::PTGroundTracking:
			m_lastNodeSeen = QDateTime::currentDateTimeUtc();
			if (!m_timer->isActive())
			{
				m_log.info(QString("Fanet node seen (%1), enabling weather data broadcasting...").arg(FanetAddress(addr).toHex(':')));
				enabledWeatherUpdates();
			}
			break;
		default:
			break;
	}
}

