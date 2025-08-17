/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HOLFUYWIDGET_H
#define HOLFUYWIDGET_H

#include "abstractweatherstation.h"
#include "logger.h"

class QTimer;
class QNetworkReply;
class QNetworkAccessManager;
class StationConfig;

/**
 * @class HolfuyWidget retrieves weather information via public Holfuy widget
 * (intended for testing, if you quickly want to try something out). No API key
 * is needed for this, however if you do have API access to the station usage of
 * class HolfuyApi is strongly recommended.
 */
class HolfuyWidget : public AbstractWeatherStation
{
	Q_OBJECT
public:
	explicit HolfuyWidget(int id, const QString &stationName, QObject *parent = nullptr);
	explicit HolfuyWidget(const StationConfig &config, QObject *parent = nullptr);
	virtual ~HolfuyWidget();

	QDateTime lastUpdate() const override;
	int windDirection() const override;
	int windSpeed() const override;
	int windGusts() const override;
	int temperature() const override;

	int stationId() const override;
	QString stationName() const override;

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
	int m_temperature;
	QString m_name;
	QDateTime m_lastUpdate;
	QNetworkReply *m_reply;
	QNetworkAccessManager *m_netmgr;
	QTimer *m_timer;

};

#endif // HOLFUYWIDGET_H
