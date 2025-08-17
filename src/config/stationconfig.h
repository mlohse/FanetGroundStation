/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STATIONCONFIG_H
#define STATIONCONFIG_H

#include <QList>
#include <QString>
#include <QSharedData>
#include <QGeoCoordinate>

class Logger;
class QXmlStreamReader;

class StationConfigData : public QSharedData
{
public:
	StationConfigData(int stationType, int stationId, const QString &stationName, const QString &apiKey, const QGeoCoordinate &position, int updateInterval);
	StationConfigData(const StationConfigData &other);
	StationConfigData();
	~StationConfigData() = default;

	int type;
	int id;
	QString name;
	QString key;
	QGeoCoordinate pos;
	int ival;
};

class StationConfig
{
public:
	static const int ID_INVALID = -1;

	enum StationType
	{
		UnknownStation = 0,
		HolfuyApi,
		HolfuyWidget,
		Windbird
	};

	explicit StationConfig(StationType stationType, int stationId, const QString &stationName, const QString &apiKey, const QGeoCoordinate &position, int updateInterval);
	explicit StationConfig(QXmlStreamReader &xml);
	StationConfig(const StationConfig &other) : m_d(other.m_d) {}
	StationConfig() = default;
	virtual ~StationConfig() = default;

	bool isValid() const;
	int stationId() const;
	QString stationName() const;
	QString apiKey() const;
	QGeoCoordinate position() const;
	StationType stationType() const;
	int updateInterval() const; // in seconds

	static QString typeToString(StationType type);

private:
	QExplicitlySharedDataPointer<StationConfigData> m_d;
};

typedef QList<StationConfig> StationConfigList;

#endif // STATIONCONFIG_H
