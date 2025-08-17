/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fagsconfig.h"
#include "logger.h"
#include "config.h"

#include <QFile>
#include <QStringList>
#include <QXmlStreamReader>


FagsConfigData::FagsConfigData(const FagsConfigData &other) :
    QSharedData(other),
    majorVer(other.majorVer),
    minorVer(other.minorVer),
    radio(other.radio),
    fanet(other.fanet),
    stations(other.stations)
{
}

FagsConfig::FagsConfig(const QString &xmlFile) :
    m_d(new FagsConfigData())
{
	Logger log("FagsConfig");
	QFile file = QFile(xmlFile);
	QXmlStreamReader xml;
	bool xmlOk = true;
	if (xmlFile.isEmpty() || !file.exists())
	{
		log.error(QString("config file not found: '%1'").arg(xmlFile));
		return;
	}
	if (!file.open(QIODevice::ReadOnly))
	{
		log.error(QString("failed to open config file: %1").arg(file.errorString()));
		return;
	}
	xml.setDevice(&file);
	while (xmlOk && !xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::StartElement:
				if (xml.name() == CONFIG_ELEMENT_FAGS)
				{
					xmlOk = parseElementFags(xml, log);
					continue;
				}
				log.error(QString("unknown element: '%1'").arg(xml.name()));
				xmlOk = false;
				break;
			default:
				break;
		}
	}
}

bool FagsConfig::parseElementFags(QXmlStreamReader &xml, Logger &log)
{
	bool majorOk = false;
	bool minorOk = false;
	QXmlStreamAttributes attr = xml.attributes();
	if (attr.hasAttribute(CONFIG_ATTR_VERSION))
	{
		QStringList ver = attr.value(CONFIG_ATTR_VERSION).toString().split('.');
		if (ver.size() == 2)
		{
			m_d->majorVer = ver.at(0).toInt(&majorOk);
			m_d->minorVer = ver.at(1).toInt(&minorOk);
		}
	}
	if (!majorOk || !minorOk)
	{
		log.error(QString("failed to parse version number: '%1'").arg(attr.value(CONFIG_ATTR_VERSION).toString()));
		return false;
	}
	if (CONFIG_VER_MAJOR != m_d->majorVer || CONFIG_VER_MINOR > m_d->minorVer)
	{
		log.error(QString("config version mismatch! (expected version: %1.%2, got version: %3.%4")
		          .arg(CONFIG_VER_MAJOR).arg(CONFIG_VER_MINOR).arg(m_d->majorVer).arg(m_d->minorVer));
		return false;
	}

	while (!xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::StartElement:
				if (xml.name() == CONFIG_ELEMENT_RADIO)
				{
					if (!parseElementRadio(xml, log)) return false;
					continue;
				}
				if (xml.name() == CONFIG_ELEMENT_FANET)
				{
					if (!parseElementFanet(xml, log)) return false;
					continue;
				}
				if (xml.name() == CONFIG_ELEMENT_STATIONS)
				{
					if (!parseElementStations(xml, log)) return false;
					continue;
				}
				log.error(QString("unknown element: '%1'").arg(xml.name()));
				return false;
			case QXmlStreamReader::EndElement:
				if (xml.name() != CONFIG_ELEMENT_FAGS)
				{
					log.error(QString("unexpected end element: '%1' (expected '%2')").arg(xml.name(), CONFIG_ELEMENT_FAGS));
					return false;
				}
				return true; // element fags sucessfully parsed :)
			default:
				break;
		}
	}
	log.error(QString("failed to parse element '%1': %2!").arg(CONFIG_ELEMENT_FAGS, xml.errorString()));
	return false;
}

bool FagsConfig::parseElementRadio(QXmlStreamReader &xml, Logger &log)
{
	Q_UNUSED(log);
	m_d->radio = RadioConfig(xml);
	return m_d->radio.isValid();
}

bool FagsConfig::parseElementFanet(QXmlStreamReader &xml, Logger &log)
{
	Q_UNUSED(log);
	m_d->fanet = FanetConfig(xml);
	return m_d->fanet.isValid();
}

bool FagsConfig::parseElementStations(QXmlStreamReader &xml, Logger &log)
{
	while (!xml.atEnd() && !xml.hasError())
	{
		switch (xml.readNext())
		{
			case QXmlStreamReader::StartElement:
				if (xml.name() == CONFIG_ELEMENT_HOLFUYAPI ||
				    xml.name() == CONFIG_ELEMENT_HOLFUYWIDGET ||
				    xml.name() == CONFIG_ELEMENT_WINDBIRD)
				{
					StationConfig station(xml);
					if (!station.isValid())	return false;
					m_d->stations << station;
					continue;
				}
				log.error(QString("unknown element: '%1'").arg(xml.name()));
				return false;
			case QXmlStreamReader::EndElement:
				if (xml.name() != CONFIG_ELEMENT_STATIONS)
				{
					log.error(QString("unexpected end element: '%' (expected '%2')").arg(CONFIG_ELEMENT_STATIONS, xml.name()));
					return false;
				}
				return true; // element 'stations' sucessfully parsed :)
			default:
				break;
		}
	}

	log.error(xml.hasError() ? QString("parser error: %1").arg(xml.errorString()) : "unexpected end of file!");
	return false;
}
