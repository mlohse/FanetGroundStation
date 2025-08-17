/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETMESSAGEDISPATCHER_H
#define FANETMESSAGEDISPATCHER_H

#include "logger.h"
#include "fanet/fanetradio.h"
#include "config/fanetconfig.h"
#include "weatherstation/abstractweatherstation.h"
#include <QDateTime>
#include <QTimer>

class FanetPayload;

class FanetMessageDispatcher : public QObject
{
	Q_OBJECT
public:
	FanetMessageDispatcher() = delete;
	explicit FanetMessageDispatcher(const FanetConfig &config, const WeatherStationList &stations, FanetRadio *radio, QObject *parent = nullptr);
	virtual ~FanetMessageDispatcher() Q_DECL_OVERRIDE;

public slots:
	void sendWeatherData();
	void sendStationNames();

private slots:
	void onTimeout();
	void disableWeatherUpdates();
	void enabledWeatherUpdates();
	void onRadioStateChanged(FanetRadio::RadioState state);
	void onFanetMessageReceived(quint32 addr, const FanetPayload &payload, bool broadcast);

private:
	Logger m_log;
	FanetConfig m_config;
	FanetRadio *m_radio;
	WeatherStationList m_stations;
	QDateTime m_lastNodeSeen;
	QDateTime m_lastWeatherUpdate;
	QDateTime m_lastNameUpdate;
	QTimer *m_timer;
};

#endif // FANETMESSAGEDISPATCHER_H
