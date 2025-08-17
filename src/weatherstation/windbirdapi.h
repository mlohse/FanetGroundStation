/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef WINDBIRDAPI_H
#define WINDBIRDAPI_H

#include "abstractweatherstation.h"
#include "logger.h"

class QTimer;
class QNetworkReply;
class QNetworkAccessManager;
class StationConfig;

class WindbirdApi : public AbstractWeatherStation
{
	Q_OBJECT
public:
	explicit WindbirdApi(int id, const QString &stationName, QObject *parent = nullptr);
	explicit WindbirdApi(const StationConfig &config, QObject *parent = nullptr);
	virtual ~WindbirdApi();

	QDateTime lastUpdate() const override; // data.measurements.date
	int windDirection() const override; // data.measurements.wind_heading
	int windSpeed() const override; // data.measurements.wind_speed_avg
	int windGusts() const override; // data.measurements.wind_speed_max

	int stationId() const override;
	QString stationName() const override; // data.meta.name

	WeatherDataFlags availableData() const override;

public slots:
	void update() override;

private slots:
	void onReplyFinished();
	void onTimeout();

protected:
	void init();

private:
	mutable Logger m_log;
	const int m_id;
	int m_winddir;
	int m_windspeed;
	int m_gustspeed;
	QString m_name;
	QDateTime m_lastUpdate;
	QNetworkReply *m_reply;
	QNetworkAccessManager *m_netmgr;
	QTimer *m_timer;
};

#endif // WINDBIRDAPI_H
