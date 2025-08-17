/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ABSTRACTWEATHERSTATION_H
#define ABSTRACTWEATHERSTATION_H

#include <QByteArray>
#include <QDateTime>
#include <QFlags>
#include <QObject>
#include <QString>
#include <QList>
#include <config/stationconfig.h>

class QTimer;

class AbstractWeatherStation : public QObject
{
	Q_OBJECT
public:
	enum WeatherData
	{
		NoData = 0x00,
		WindSpeed = 0x01,
		WindSpeedGust = 0x02,
		WindDirection = 0x04,
		Temperature = 0x08,
		Humidity = 0x10
	};
	Q_DECLARE_FLAGS(WeatherDataFlags, WeatherData)

	const int TemperatureInvalid = -274;

	explicit AbstractWeatherStation(QObject *parent = nullptr);
	explicit AbstractWeatherStation(const StationConfig &config, QObject *parent = nullptr);
	virtual ~AbstractWeatherStation();

	static AbstractWeatherStation *fromConfig(const StationConfig &config, QObject *parent = nullptr);

	Q_PROPERTY(QDateTime lastUpdate READ lastUpdate NOTIFY lastUpdateChanged)
	Q_PROPERTY(int windDirection READ windDirection NOTIFY windDirectionChanged)
	Q_PROPERTY(int windSpeed READ windSpeed NOTIFY windSpeedChanged)
	Q_PROPERTY(int windGusts READ windGusts NOTIFY windGustsChanged)
	Q_PROPERTY(int temperature READ temperature NOTIFY temperatureChanged)
	Q_PROPERTY(int humidity READ humidity NOTIFY humidityChanged)
	Q_PROPERTY(int stationId READ stationId CONSTANT)
	Q_PROPERTY(QString stationName READ stationName NOTIFY stationNameChanged)
	Q_PROPERTY(QString unitTemperature READ unitTemperature CONSTANT)
	Q_PROPERTY(QString unitWindSpeed READ unitWindSpeed CONSTANT)
	Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval NOTIFY updateIntervalChanged)

	virtual QDateTime lastUpdate() const = 0;
	virtual int windDirection() const { return -1; }
	virtual int windSpeed() const { return -1; }
	virtual int windGusts() const { return -1; }
	virtual int temperature() const { return TemperatureInvalid * 10; }
	virtual int humidity() const { return -1; }

	virtual int stationId() const = 0;
	virtual QString stationName() const = 0;
	virtual QString unitTemperature() const { return "C"; }
	virtual QString unitWindSpeed() const { return "km/h"; }

	virtual WeatherDataFlags availableData() const = 0;
	virtual StationConfig config() const { return m_config; }
	int updateInterval() const { return m_updateIntervalSecs; } // 0 = disabled

public slots:
	virtual void update() = 0;
	void setUpdateInterval(int secs);

signals:
	void stationNameChanged(const QString &newName);
	void lastUpdateChanged(const QDateTime &newUpdate);
	void windDirectionChanged(int newWindDirection);
	void windSpeedChanged(int newWindSpeed);
	void windGustsChanged(int newWindGusts);
	void temperatureChanged(int newTemperature);
	void humidityChanged(int newHumidity);
	void updateFinished(bool success);
	void updateIntervalChanged(int newUpdateIntervalSecs);

private:
	int m_updateIntervalSecs;
	QTimer *m_timer;
	StationConfig m_config;
};

typedef QList<AbstractWeatherStation*> WeatherStationList;

Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractWeatherStation::WeatherDataFlags)

#endif // ABSTRACTWEATHERSTATION_H
