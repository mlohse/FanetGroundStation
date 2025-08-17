/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "holfuywidget.h"
#include "application.h"
#include "config.h"
#include "gpio.h"
#include "config/stationconfig.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QBuffer>
#include <QTimer>
#include <QUrl>

static const qint64 NETWORK_REPLY_SIZE_MAX = 5120; // do not parse more than 5kb
static const int  NETWORK_TIMEOUT          = 15 * 1000; // 15sec
static const char NETWORK_HOLFUY_URL[]     = "https://widget.holfuy.com/?station=%1&su=km/h&t=C&lang=en&mode=rose&size=160";
static const char DATA_DELIMITER_START[]   = "newWind(";
static const char DATA_DELIMITER_STOP[]    = ");";


HolfuyWidget::HolfuyWidget(int id, const QString &stationName, QObject *parent) :
    AbstractWeatherStation(parent),
    m_log(QString("HolfuyWidget-%1").arg(id)),
    m_id(id),
    m_winddir(-1),
    m_windspeed(-1),
    m_gustspeed(-1),
    m_temperature(AbstractWeatherStation::TemperatureInvalid * 10),
    m_name(stationName),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this))
{
	init();
}

HolfuyWidget::HolfuyWidget(const StationConfig &config, QObject *parent) :
    AbstractWeatherStation(config, parent),
    m_log(QString("HolfuyWidget-%1").arg(config.stationId())),
    m_id(config.stationId()),
    m_winddir(-1),
    m_windspeed(-1),
    m_gustspeed(-1),
    m_temperature(AbstractWeatherStation::TemperatureInvalid * 10),
    m_name(config.stationName()),
    m_lastUpdate(),
    m_reply(nullptr),
    m_netmgr(new QNetworkAccessManager(this)),
    m_timer(new QTimer(this))
{
	init();
}

HolfuyWidget::~HolfuyWidget()
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

void HolfuyWidget::init()
{
	Application *app = qobject_cast<Application*>(qApp);
	Gpio *gpio = app ? app->gpio() : nullptr;
	if (gpio)
	{
		gpio->initPin(LED_PIN_BLUE, Gpio::GFOutput);
	}

	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &HolfuyWidget::onTimeout);
}

QDateTime HolfuyWidget::lastUpdate() const
{
	return m_lastUpdate;
}

int HolfuyWidget::windDirection() const
{
	return m_winddir;
}

int HolfuyWidget::windSpeed() const
{
	return m_windspeed;
}

int HolfuyWidget::windGusts() const
{
	return m_gustspeed;
}

int HolfuyWidget::temperature() const
{
	return m_temperature;
}

int HolfuyWidget::stationId() const
{
	return m_id;
}

QString HolfuyWidget::stationName() const
{
	return m_name;
}

AbstractWeatherStation::WeatherDataFlags HolfuyWidget::availableData() const
{
	return (AbstractWeatherStation::WindDirection |
	        AbstractWeatherStation::WindSpeed |
	        AbstractWeatherStation::WindSpeedGust |
	        AbstractWeatherStation::Temperature);
}

void HolfuyWidget::update()
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
		const QUrl url(QString(NETWORK_HOLFUY_URL).arg(m_id));
		m_reply = m_netmgr->get(QNetworkRequest(url));
		connect(m_reply, &QNetworkReply::finished, this, &HolfuyWidget::onReplyFinished);
		m_timer->start(NETWORK_TIMEOUT);
	}
}

void HolfuyWidget::onReplyFinished()
{
	if (!m_reply)
	{
		return;
	}

	bool convOk;
	QTime time;
	QDateTime dt = QDateTime::currentDateTime().toLocalTime();
	QString html = QString::fromUtf8(m_reply->read(NETWORK_REPLY_SIZE_MAX));
	int dir, wind, gust, temp;
	int error = -1;
	int indexStart = html.indexOf(DATA_DELIMITER_START);
	int indexStop = html.indexOf(DATA_DELIMITER_STOP, indexStart);

	m_reply->deleteLater();
	m_reply = 0;

	if (indexStart > 0)
	{
		const int delsize = strlen(DATA_DELIMITER_START);
		const QString rawdata = html.mid(indexStart + delsize, indexStop - (indexStart + delsize));
		const QStringList data = rawdata.split(',', Qt::SkipEmptyParts);

		/// example data: 173,3,6.2,4,'02:09'
		/// format: <dir>,<wind>,<temparature>,<gusts>,'HH:mm'
		if (data.count() >= 5)
		{
			dir = data.at(0).toInt(&convOk);
			error = convOk ? 0 : 1;
			wind = data.at(1).toInt(&convOk) * 10;
			error += convOk ? 0 : 1;
			gust = data.at(3).toInt(&convOk) * 10;
			error += convOk ? 0 : 1;
			temp = static_cast<int>(data.at(2).toDouble(&convOk) * 10);
			error += convOk ? 0 : 1;
			time = QTime::fromString(data.at(4).mid(1, 5), Qt::TextDate);
			error += time.isValid() ? 0 : 1;
		}
		if (error)
		{
			m_log.warning(QString("Failed to parse weather station data from string: '%1'").arg(rawdata));
			return;
		}
		if (m_winddir != dir)
		{
			m_winddir = dir;
			emit windDirectionChanged(m_winddir);
		}
		if (m_windspeed != wind)
		{
			m_windspeed = wind;
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
		dt.setTime(time);
		if (dt.isValid() && m_lastUpdate != dt)
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
		return;
	}
	m_log.warning("reply contains no (valid) weather data!");
	emit updateFinished(false);
//	m_log.debug(QString("data: %1").arg(buf.data()));
}

void HolfuyWidget::onTimeout()
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
