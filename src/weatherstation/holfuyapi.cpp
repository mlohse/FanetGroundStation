/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "holfuyapi.h"
#include "application.h"
#include "config.h"
#include "gpio.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QTimer>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimeZone>

static const qint64  NETWORK_REPLY_SIZE_MAX = 1024; // do not parse more than 1kb of data
static const int     NETWORK_TIMEOUT        = 15 * 1000; // 15sec

static const char JSON_KEY_NAME[]           = "stationName";
static const char JSON_KEY_DATETIME[]       = "dateTime";
static const char JSON_KEY_TEMPERATURE[]    = "temperature";
//static const char JSON_KEY_HUMIDITY[]       = "humidity";
static const char JSON_KEY_WIND[]           = "wind";
static const char JSON_KEY_WIND_SPEED[]     = "speed";
static const char JSON_KEY_WIND_GUST[]      = "gust";
static const char JSON_KEY_WIND_DIR[]       = "direction";
static const char JSON_KEY_WIND_UNIT[]      = "unit";

static const char JSON_FORMAT_DATETIME[]    = "yyyy-MM-dd HH:mm:ss";

// '%1' is placeholder for station id, %2 is placeholder for API key
static const char HOLFUY_API_URL[]          = "http://api.holfuy.com/live/?s=%1&pw=%2&m=JSON&tu=C&su=km/h&avg=0&utc"; // newest station data
//static const char HOLFUY_API_URL[] = "http://api.holfuy.com/live/?s=%1&pw=%2&m=JSON&tu=C&su=km/h&avg=1&utc"; // 15min average data


HolfuyApi::HolfuyApi(int id, const QString &apiKey, const QString &stationName, QObject *parent) :
    AbstractWeatherStation(parent),
    m_log(QString("HolfuyApi-%1").arg(id)),
    m_id(id),
    m_winddir(0),
    m_windspeed(0),
    m_gustspeed(0),
    m_temperature(AbstractWeatherStation::TemperatureInvalid * 10),
    m_name(stationName),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this)),
    m_apiKey(apiKey)
{
	init();
}

HolfuyApi::HolfuyApi(const StationConfig &config, QObject *parent) :
    AbstractWeatherStation(config, parent),
    m_log(QString("HolfuyApi-%1").arg(config.stationId())),
    m_id(config.stationId()),
    m_winddir(0),
    m_windspeed(0),
    m_gustspeed(0),
    m_temperature(AbstractWeatherStation::TemperatureInvalid * 10),
    m_name(config.stationName()),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this)),
    m_apiKey(config.apiKey())
{
	init();
}

HolfuyApi::~HolfuyApi()
{
	Application *app = qobject_cast<Application*>(qApp);
	Gpio *gpio = app ? app->gpio() : nullptr;
	if (gpio)
	{
		gpio->clearGpio(LED_PIN_BLUE);
	}

	if (m_reply)
	{
		m_reply->abort();
		m_reply->deleteLater();
		m_reply = nullptr;
	}
}

void HolfuyApi::init()
{
	Application *app = qobject_cast<Application*>(qApp);
	Gpio *gpio = app ? app->gpio() : nullptr;
	if (gpio)
	{
		gpio->initPin(LED_PIN_BLUE, Gpio::GFOutput);
	}

	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &HolfuyApi::onTimeout);
}

QDateTime HolfuyApi::lastUpdate() const
{
	return m_lastUpdate;
}

int HolfuyApi::windDirection() const
{
	return m_winddir;
}

int HolfuyApi::windSpeed() const
{
	return m_windspeed;
}

int HolfuyApi::windGusts() const
{
	return m_gustspeed;
}

int HolfuyApi::temperature() const
{
	return m_temperature;
}

int HolfuyApi::stationId() const
{
	return m_id;
}

QString HolfuyApi::stationName() const
{
	return m_name;
}

AbstractWeatherStation::WeatherDataFlags HolfuyApi::availableData() const
{
	return (AbstractWeatherStation::WindDirection |
	        AbstractWeatherStation::WindSpeed |
	        AbstractWeatherStation::WindSpeedGust |
	        AbstractWeatherStation::Temperature); /// @todo: add humidity if available
}

void HolfuyApi::update()
{
	if (!m_reply) // request still running?
	{
		//m_log.debug("update...");
		Application *app = qobject_cast<Application*>(qApp);
		Gpio *gpio = app ? app->gpio() : nullptr;
		if (gpio)
		{
			gpio->clearGpio(LED_PIN_BLUE);
		}
		const QUrl url(QString(HOLFUY_API_URL).arg(m_id).arg(m_apiKey));
		m_reply = m_netmgr->get(QNetworkRequest(url));
		connect(m_reply, &QNetworkReply::finished, this, &HolfuyApi::onReplyFinished);
		m_timer->start(NETWORK_TIMEOUT);
	}
}

void HolfuyApi::onReplyFinished()
{
	if (!m_reply)
	{
		return;
	}

	QByteArray data = m_reply->read(NETWORK_REPLY_SIZE_MAX);
	QJsonDocument doc = QJsonDocument::fromJson(data);
	m_reply->deleteLater();
	m_reply = nullptr;

	m_log.debug(QString("json data: %1").arg(QString::fromLatin1(data)));

	if (doc.isNull())
	{
		m_log.warning(QString("Failed to parse json data: '%1'").arg(QString::fromLatin1(data)));
		emit updateFinished(false);
		return;
	}

	QJsonObject rootObj = doc.object();
	if (!rootObj.contains(JSON_KEY_DATETIME) || !rootObj.contains(JSON_KEY_WIND))
	{
		m_log.warning(QString("Received incomplete data: '%1'").arg(QString::fromLatin1(data)));
		emit updateFinished(false);
		return;
	}

	QJsonObject windObj = rootObj.value(JSON_KEY_WIND).toObject();
	if (!windObj.isEmpty())
	{
		int dir, speed, gust, temp;
		QDateTime dt = QDateTime::fromString(rootObj.value(JSON_KEY_DATETIME).toString(), JSON_FORMAT_DATETIME);
		dt.setTimeZone(QTimeZone::UTC);

		if (m_name.isEmpty() && rootObj.contains(JSON_KEY_NAME))
		{
			m_name = rootObj.value(JSON_KEY_NAME).toString();
			m_log.info(QString("station name updated: '%1'").arg(m_name));
			emit stationNameChanged(m_name);
		}

		QString unit(windObj.contains(JSON_KEY_WIND_UNIT) ? windObj.value(JSON_KEY_WIND_UNIT).toString() : "");
		if (unit != unitWindSpeed())
		{
			m_log.warning(QString("Wrong unit for wind (expected '%1', got '%2')!").arg(unitWindSpeed(), unit));
			emit updateFinished(false);
			return;
		}

		dir = windObj.contains(JSON_KEY_WIND_DIR) ? windObj.value(JSON_KEY_WIND_DIR).toInt(0) : 0;
		speed = windObj.contains(JSON_KEY_WIND_SPEED) ? (windObj.value(JSON_KEY_WIND_SPEED).toDouble() * 10) : 0;
		gust = windObj.contains(JSON_KEY_WIND_GUST) ? (windObj.value(JSON_KEY_WIND_GUST).toDouble() * 10) : 0;
		temp = rootObj.contains(JSON_KEY_TEMPERATURE) ? (rootObj.value(JSON_KEY_TEMPERATURE).toDouble(0) * 10) : 0;
		///@todo ready humidity if available

		if (m_winddir != dir)
		{
			m_winddir = dir;
			emit windDirectionChanged(m_winddir);
		}
		if (m_windspeed != speed)
		{
			m_windspeed = speed;
			emit windSpeedChanged(m_windspeed);
		}
		if (m_gustspeed != gust)
		{
			m_gustspeed = gust;
			emit windGustsChanged(m_gustspeed);
		}
		if (m_temperature != temp)
		{
			m_temperature = temp;
			emit temperatureChanged(m_temperature);
		}
		if (dt.isValid() && dt != m_lastUpdate)
		{
			m_lastUpdate = dt;
			emit lastUpdateChanged(m_lastUpdate);
		}

		m_log.info(QString("new data: wind=%1%2, gusts=%3%2, dir=%4, temp=%5%6, lastUpdate=%7")
		           .arg(m_windspeed / 10.0).arg(unitWindSpeed()).arg(m_gustspeed / 10.0).arg(m_winddir)
		           .arg(static_cast<double>(m_temperature / 10.0))
		           .arg(unitTemperature(), m_lastUpdate.time().toString()));

		Application *app = qobject_cast<Application*>(qApp);
		Gpio *gpio = app ? app->gpio() : nullptr;
		if (gpio)
		{
			gpio->setGpio(LED_PIN_BLUE);
		}
		emit updateFinished(true);
	}
}

void HolfuyApi::onTimeout()
{
	if (m_reply)
	{
		QNetworkReply *tmp = m_reply;
		m_log.warning(QString("Request timed out: %1").arg(tmp->request().url().toDisplayString()));
		m_reply = 0;
		tmp->disconnect();
		tmp->abort();
		tmp->deleteLater();
		emit updateFinished(false);
	}
}
