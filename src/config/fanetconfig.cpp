/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetconfig.h"
#include "logger.h"
#include "config.h"

#include <QXmlStreamReader>
#include <QStringList>

FanetConfigData::FanetConfigData(int ivalWeather, int ivalNames, int inactivity, int maxAge) :
    QSharedData(),
    txintervalWeather(ivalWeather),
    txintervalNames(ivalNames),
    inactivityTimeout(inactivity),
    weatherDataMaxAge(maxAge)
{
}

FanetConfigData::FanetConfigData(const FanetConfigData &other) :
    QSharedData(other),
    txintervalWeather(other.txintervalWeather),
    txintervalNames(other.txintervalNames),
    inactivityTimeout(other.inactivityTimeout),
    weatherDataMaxAge(other.weatherDataMaxAge)
{
}

FanetConfigData::FanetConfigData() :
    QSharedData(),
    txintervalWeather(FANET_TXINTERVAL_WEATHER_DEFAULT),
    txintervalNames(FANET_TXINTERVAL_NAMES_DEFAULT),
    inactivityTimeout(FANET_INACTIVITY_TIMEOUT_DEFAULT),
    weatherDataMaxAge(FANET_WEATHER_DATA_MAXAGE)
{
}

FanetConfig::FanetConfig(int ivalWeather, int ivalNames, int inactivity, int maxAge) :
    m_d(new FanetConfigData(ivalWeather, ivalNames, inactivity, maxAge))
{
}

FanetConfig::FanetConfig(QXmlStreamReader &xml) :
    m_d(nullptr)
{
	Logger log("FanetConfig");
	bool success = false;
	QXmlStreamAttributes attr = xml.attributes();
	QStringList reqAttrKeys = QStringList() << CONFIG_ATTR_TXINTERVAL_WEATHER << CONFIG_ATTR_TXINTERVAL_NAMES << CONFIG_ATTR_INACTIVITY_TIMEOUT << CONFIG_ATTR_WEATHER_MAXAGE;
	int values[4]; // size see above

	for (int i = 0; i < reqAttrKeys.size(); i++)
	{
		bool convOk;
		const QString key = reqAttrKeys.at(i);
		if (!attr.hasAttribute(key))
		{
			log.error(QString("attribute '%1' is missing!").arg(key));
			return;
		}
		values[i] = attr.value(key).toInt(&convOk);
		if (!convOk || values[i] < 0)
		{
			log.error(QString("failed to parse attribute '%1': invalid value '%2'").arg(key, attr.value(key).toString()));
			return;
		}
	}

	// parse element end
	while(!success && !xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::EndElement:
				if (xml.name() != CONFIG_ELEMENT_FANET)
				{
					log.error(QString("unexpected end element: '%1' (expected '%2')").arg(xml.name(), CONFIG_ELEMENT_FANET));
					return;
				}
				success = true;
				break;
			case QXmlStreamReader::Characters: // no inner text expected!
				log.error(QString("unexpected text: '%1'").arg(xml.text()));
				return;
			case QXmlStreamReader::StartElement: // no child elements expected!
				log.error(QString("unexpected child element: '%1'").arg(xml.name()));
				return;
			default: // just ignore comments etc...
				break;
		}
	}

	// success :)
	m_d = new FanetConfigData(values[0], values[1], values[2], values[3]);
	log.info(QString("txintervalWeather=%1, txintervalNames=%2, inactivityTimeout=%3, weatherDataMaxAge=%4")
	         .arg(m_d->txintervalWeather).arg(m_d->txintervalNames).arg(m_d->inactivityTimeout).arg(m_d->weatherDataMaxAge));
}
