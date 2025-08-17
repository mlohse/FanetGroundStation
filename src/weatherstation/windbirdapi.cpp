/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "windbirdapi.h"
#include "application.h"
#include "config.h"
#include "gpio.h"
#include "config/stationconfig.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QtNumeric>
#include <QTimer>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimeZone>

static const qint64  NETWORK_REPLY_SIZE_MAX = 2048; // do not parse more than 2kb of data
static const int     NETWORK_TIMEOUT        = 15 * 1000; // 15sec

static const char JSON_KEY_DATA[]           = "data";
static const char JSON_KEY_META[]           = "meta";
static const char JSON_KEY_NAME[]           = "name";
static const char JSON_KEY_ID[]             = "id";
static const char JSON_KEY_MEASUREMENTS[]   = "measurements";
static const char JSON_KEY_WIND_SPEED[]     = "wind_speed_avg";
static const char JSON_KEY_WIND_GUST[]      = "wind_speed_max";
static const char JSON_KEY_WIND_DIR[]       = "wind_heading";
static const char JSON_KEY_DATETIME[]       = "date";

static const char JSON_FORMAT_DATETIME[]    = "yyyy-MM-ddTHH:mm:ss.zzzt";
static const char WINDBIRD_API_URL[]        = "http://api.pioupiou.fr/v1/live/%1"; // %1 = placeholder for station id


WindbirdApi::WindbirdApi(int id, const QString &stationName, QObject *parent) :
    AbstractWeatherStation(parent),
    m_log(QString("WindbirdApi-%1").arg(id)),
    m_id(id),
    m_winddir(0),
    m_windspeed(0),
    m_gustspeed(0),
    m_name(stationName),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this))
{
	init();
}

WindbirdApi::WindbirdApi(const StationConfig &config, QObject *parent) :
    AbstractWeatherStation(config, parent),
    m_log(QString("WindbirdApi-%1").arg(config.stationId())),
    m_id(config.stationId()),
    m_winddir(0),
    m_windspeed(0),
    m_gustspeed(0),
    m_name(config.stationName()),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this))
{
	init();
}

WindbirdApi::~WindbirdApi()
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

void WindbirdApi::init()
{
	Application *app = qobject_cast<Application*>(qApp);
	Gpio *gpio = app ? app->gpio() : nullptr;
	if (gpio)
	{
		gpio->initPin(LED_PIN_BLUE, Gpio::GFOutput);
	}

	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &WindbirdApi::onTimeout);

	// OpenWindMap API community licence requires to show this message:
	m_log.info("Wind data (c) contributors of the OpenWindMap wind network <https://openwindmap.org>");
}

QDateTime WindbirdApi::lastUpdate() const
{
	return m_lastUpdate;
}

int WindbirdApi::windDirection() const
{
	return m_winddir;
}

int WindbirdApi::windSpeed() const
{
	return m_windspeed;
}

int WindbirdApi::windGusts() const
{
	return m_gustspeed;
}

int WindbirdApi::stationId() const
{
	return m_id;
}

QString WindbirdApi::stationName() const
{
	return m_name;
}

AbstractWeatherStation::WeatherDataFlags WindbirdApi::availableData() const
{
	return (AbstractWeatherStation::WindDirection |
	        AbstractWeatherStation::WindSpeed |
	        AbstractWeatherStation::WindSpeedGust);
}

void WindbirdApi::update()
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
		const QUrl url(QString(WINDBIRD_API_URL).arg(m_id));
		m_reply = m_netmgr->get(QNetworkRequest(url));
		connect(m_reply, &QNetworkReply::finished, this, &WindbirdApi::onReplyFinished);
		m_timer->start(NETWORK_TIMEOUT);
	}
}

void WindbirdApi::onReplyFinished()
{
	if (!m_reply)
	{
		return;
	}

	QByteArray data = m_reply->read(NETWORK_REPLY_SIZE_MAX);
	QJsonDocument doc = QJsonDocument::fromJson(data);
	m_reply->deleteLater();
	m_reply = nullptr;

//	m_log.debug(QString("json data: %1").arg(QString::fromLatin1(data)));

	if (doc.isNull())
	{
		m_log.warning(QString("Failed to parse json data: '%1'").arg(QString::fromLatin1(data)));
		emit updateFinished(false);
		return;
	}

	QJsonObject rootObj = doc.object();
	QJsonObject dataObj = rootObj.contains(JSON_KEY_DATA) ? rootObj.value(JSON_KEY_DATA).toObject() : QJsonObject();
	QJsonObject metaObj = dataObj.contains(JSON_KEY_META) ? dataObj.value(JSON_KEY_META).toObject() : QJsonObject();
	QJsonObject windObj = dataObj.contains(JSON_KEY_MEASUREMENTS) ? dataObj.value(JSON_KEY_MEASUREMENTS).toObject() : QJsonObject();

	if (metaObj.isEmpty() || windObj.isEmpty() || !windObj.contains(JSON_KEY_DATETIME))
	{
		m_log.warning(QString("Received incomplete data: '%1'").arg(QString::fromLatin1(doc.toJson(QJsonDocument::Compact))));
		emit updateFinished(false);
		return;
	}

	if (!dataObj.contains(JSON_KEY_ID) || dataObj.value(JSON_KEY_ID).toInt(-1) != m_id)
	{
		m_log.warning(QString("Received data for invalid/wrong station id: %1").arg(dataObj.value(JSON_KEY_ID).toInt(-1)));
		emit updateFinished(false);
		return;
	}

	if (m_name.isEmpty() && metaObj.contains(JSON_KEY_NAME))
	{
		m_name = metaObj.value(JSON_KEY_NAME).toString();
		m_log.info(QString("station name updated: '%1'").arg(m_name));
		emit stationNameChanged(m_name);
	}

	if (!windObj.isEmpty())
	{
		QDateTime dt = QDateTime::fromString(windObj.value(JSON_KEY_DATETIME).toString(), JSON_FORMAT_DATETIME);
		int dir = windObj.contains(JSON_KEY_WIND_DIR) ? qRound(windObj.value(JSON_KEY_WIND_DIR).toDouble()) : 0;
		int speed = windObj.contains(JSON_KEY_WIND_SPEED) ? qRound(windObj.value(JSON_KEY_WIND_SPEED).toDouble() * 10) : 0;
		int gust = windObj.contains(JSON_KEY_WIND_GUST) ? qRound(windObj.value(JSON_KEY_WIND_GUST).toDouble() * 10) : 0;

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
		if (dt.isValid() && dt != m_lastUpdate)
		{
			m_lastUpdate = dt;
			emit lastUpdateChanged(m_lastUpdate);
		}

		m_log.info(QString("new data: wind=%1%2, gusts=%3%2, dir=%4, lastUpdate=%7")
		           .arg(m_windspeed / 10.0).arg(unitWindSpeed()).arg(m_gustspeed / 10.0).arg(m_winddir)
		           .arg(m_lastUpdate.time().toString()));

		Application *app = qobject_cast<Application*>(qApp);
		Gpio *gpio = app ? app->gpio() : nullptr;
		if (gpio)
		{
			gpio->setGpio(LED_PIN_BLUE);
		}
		emit updateFinished(true);
	}
}

void WindbirdApi::onTimeout()
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
