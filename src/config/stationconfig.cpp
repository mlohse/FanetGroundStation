/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stationconfig.h"
#include "logger.h"
#include "config.h"

#include <QXmlStreamReader>
#include <QStringList>


StationConfigData::StationConfigData(int stationType, int stationId, const QString &stationName, const QString &apiKey, const QGeoCoordinate &position, int updateInterval) :
    QSharedData(),
    type(stationType),
    id(stationId),
    name(stationName),
    key(apiKey),
    pos(position),
    ival(updateInterval)
{
}

StationConfigData::StationConfigData(const StationConfigData &other) :
    QSharedData(other),
    type(other.type),
    id(other.id),
    name(other.name),
    key(other.key),
    pos(other.pos),
    ival(other.ival)
{
}

StationConfigData::StationConfigData() :
    QSharedData(),
    type(StationConfig::UnknownStation),
    id(StationConfig::ID_INVALID),
    name(),
    key(),
    pos(),
    ival(0)
{
}

StationConfig::StationConfig(StationType stationType, int stationId, const QString &stationName, const QString &apiKey, const QGeoCoordinate &position, int updateInterval) :
    m_d(new StationConfigData(stationType, stationId, stationName, apiKey, position, updateInterval))
{
}

StationConfig::StationConfig(QXmlStreamReader &xml) :
    m_d(nullptr)
{
	Logger log("StationConfig");
	QString name, key, element = xml.name().toString();
	int id, ival;
	double lon, lat, alt;
	bool convOk, success = false;
	StationType type = UnknownStation;

	if (element == CONFIG_ELEMENT_HOLFUYAPI)    type = HolfuyApi; else
	if (element == CONFIG_ELEMENT_HOLFUYWIDGET) type = HolfuyWidget; else
	if (element == CONFIG_ELEMENT_WINDBIRD)     type = Windbird;

	if (type == UnknownStation)
	{
		log.error(QString("failed to parse station type: '%1'").arg(element));
		return;
	}

	QXmlStreamAttributes attr = xml.attributes();
	QStringList reqAttrKeys = QStringList() << CONFIG_ATTR_ID << CONFIG_ATTR_NAME << CONFIG_ATTR_POSLON << CONFIG_ATTR_POSLAT << CONFIG_ATTR_POSALT << CONFIG_ATTR_IVAL;
	if (type == HolfuyApi)
	{
		reqAttrKeys << CONFIG_ATTR_APIKEY;
	}
	foreach (const QString &key, reqAttrKeys)
	{
		if (!attr.hasAttribute(key))
		{
			log.error(QString("attribute '%1' is missing!").arg(key));
			return;
		}
	}

	// station id & name
	id = attr.value(CONFIG_ATTR_ID).toInt(&convOk);
	if (!convOk)
	{
		log.error(QString("failed to parse station id: '%1'").arg(attr.value(CONFIG_ATTR_ID)));
		return;
	}
	name = attr.value(CONFIG_ATTR_NAME).toString();

	// api key (HolfuyApi only)
	if (type == HolfuyApi)
	{
		key = attr.value(CONFIG_ATTR_APIKEY).toString();
	}

	// station position
	lon = attr.value(CONFIG_ATTR_POSLON).toDouble(&convOk);
	if (!convOk)
	{
		log.error(QString("failed to parse longitude: '%1'").arg(attr.value(CONFIG_ATTR_POSLON)));
	}
	lat = attr.value(CONFIG_ATTR_POSLAT).toDouble(&convOk);
	if (!convOk)
	{
		log.error(QString("failed to parse latitude: '%1'").arg(attr.value(CONFIG_ATTR_POSLAT)));
	}
	alt = attr.value(CONFIG_ATTR_POSALT).toDouble(&convOk);
	if (!convOk)
	{
		log.error(QString("failed to parse altitude: '%1'").arg(attr.value(CONFIG_ATTR_POSALT)));
	}

	// update interval
	ival = attr.value(CONFIG_ATTR_IVAL).toInt(&convOk);
	if (!convOk)
	{
		log.error(QString("failed to parse station update interval: '%1'").arg(attr.value(CONFIG_ATTR_IVAL)));
	}

	// parse element end
	while(!success && !xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::EndElement:
				if (xml.name() != element)
				{
					log.error(QString("unexpected end element: '%1' (expected '%2')").arg(xml.name(), element));
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
	m_d = new StationConfigData(type, id, name, key, QGeoCoordinate(lat, lon, alt), ival);
	log.info(QString("type=%1, id=%2, name=%3, apikey=%4 position=%5, update_interval=%6")
	         .arg(typeToString(type), QString::number(id), name, key.isEmpty() ? "<empty>" : "<hidden>",
	              m_d->pos.toString(), QString::number(ival)));

}

bool StationConfig::isValid() const
{
	return (m_d && m_d->type != StationConfig::UnknownStation && m_d->id > ID_INVALID && m_d->pos.isValid());
}

int StationConfig::stationId() const
{
	return m_d ? m_d->id : ID_INVALID;
}

QString StationConfig::stationName() const
{
	return m_d ? m_d->name : QString();
}

QString StationConfig::apiKey() const
{
	return m_d ? m_d->key : QString();
}

QGeoCoordinate StationConfig::position() const
{
	return m_d ? m_d->pos : QGeoCoordinate();
}

StationConfig::StationType StationConfig::stationType() const
{
	return m_d ? static_cast<StationType>(m_d->type) : UnknownStation;
}

int StationConfig::updateInterval() const
{
	return m_d ? m_d->ival : 0;
}

QString StationConfig::typeToString(StationConfig::StationType type)
{
	switch (type)
	{
		case HolfuyApi:    return "HolfuyApi";
		case HolfuyWidget: return "HolfuyWidget";
		case Windbird:     return "Windbird";
		default:           return "UnknownStation";
	}
}
