/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "abstractweatherstation.h"
#include "holfuyapi.h"
#include "holfuywidget.h"
#include "windbirdapi.h"

#include <QTimer>


AbstractWeatherStation::AbstractWeatherStation(QObject *parent) :
    QObject(parent),
    m_updateIntervalSecs(0),
    m_timer(new QTimer(this)),
    m_config()
{
	connect(m_timer, &QTimer::timeout, this, &AbstractWeatherStation::update);
}

AbstractWeatherStation::AbstractWeatherStation(const StationConfig &config, QObject *parent) :
    QObject(parent),
    m_updateIntervalSecs(0),
    m_timer(new QTimer(this)),
    m_config(config)
{
	connect(m_timer, &QTimer::timeout, this, &AbstractWeatherStation::update);
	if (config.updateInterval() > 0)
	{
		setUpdateInterval(config.updateInterval());
	}
}

AbstractWeatherStation::~AbstractWeatherStation()
{
	if (m_timer->isActive())
	{
		m_timer->stop();
	}
}

AbstractWeatherStation* AbstractWeatherStation::fromConfig(const StationConfig &config, QObject *parent)
{
	if (config.isValid())
	{
		switch (config.stationType())
		{
			case StationConfig::HolfuyApi:
				return new HolfuyApi(config, parent);
			case StationConfig::HolfuyWidget:
				return new HolfuyWidget(config, parent);
			case StationConfig::Windbird:
				return new WindbirdApi(config, parent);
			default:
				break;
		}
	}
	return nullptr;
}

void AbstractWeatherStation::setUpdateInterval(int secs)
{
	if (secs != m_updateIntervalSecs)
	{
		if (secs > 0)
			m_timer->start(secs * 1000);
		else
			m_timer->stop();

		m_updateIntervalSecs = secs;
		emit updateIntervalChanged(secs);
	}
}
