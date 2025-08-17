/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FANETPAYLOAD_H
#define FANETPAYLOAD_H

#include <qtypes.h>
#include <QFlags>
#include <QString>
#include <QByteArray>
#include <QGeoCoordinate>

class FanetPayload
{
public:
	enum PayloadType // see protocol.txt
	{
		PTAck                 = 0x00,
		PTTracking            = 0x01,
		PTName                = 0x02,
		PTMessage             = 0x03,
		PTService             = 0x04,
		PTLandmarks           = 0x05,
		PTRemoteConfig        = 0x06,
		PTGroundTracking      = 0x07,
		PTHWInfoOld           = 0x08, // deprecated
		PTThermal             = 0x09,
		PTHWInfo              = 0x0A,
		PTInvalid             = 0xFF
	};

	enum AircraftType
	{
		ATOther               = 0x00,
		ATParaglider          = 0x01,
		ATHangglider          = 0x02,
		ATBallon              = 0x03,
		ATGlider              = 0x04,
		ATPoweredAicraft      = 0x05,
		ATHelicopter          = 0x06,
		ATUAV                 = 0x07
	};

	enum GroundTrackingType
	{
		GTOther               = 0x00,
		GTWalking             = 0x01,
		GTVehicle             = 0x02,
		GTBike                = 0x03,
		GTBoot                = 0x04,
		GTNeedARide           = 0x08,
		GTLandedWell          = 0x09,
		GTNeedTechSupport     = 0x0C,
		GTNeedMedicalHelp     = 0x0D,
		GTDistressCall        = 0x0E,
		GTDistressCallAuto    = 0x0F
	};

	enum ServiceHeader
	{
		SHExtendedHeader      = 0x01,
		SHStateOfCharge       = 0x02,
		SHSupportRemoteConfig = 0x04,
		SHPressure            = 0x08,
		SHHumidity            = 0x10,
		SHWind                = 0x20,
		SHTemperature         = 0x40,
		SHInternetGateway     = 0x80
	};
	Q_DECLARE_FLAGS(ServiceHeaderFlags, ServiceHeader)

	explicit FanetPayload();
	~FanetPayload() = default;

	static FanetPayload fromReceivedData(PayloadType type, const QByteArray &data);
	static FanetPayload ackPayload();
	static FanetPayload namePayload(const QString &name);
	static FanetPayload messagePayload(const QString &msg);
	static FanetPayload servicePayload(ServiceHeaderFlags header, const QGeoCoordinate &pos, int temp, int dir, int wind, int gusts, int humidity, int pressure);

	bool isValid() const { return m_type != PTInvalid; }
	PayloadType type() const { return m_type; }
	qsizetype size() const { return m_data.size(); }
	QByteArray data() const { return m_data; }

	QString name() const;
	QString message() const;
	QString deviceType(quint8 manufacturerId) const;
	QString firmwareBuild() const;
	QGeoCoordinate position() const;
	AircraftType aircraftType() const;
	GroundTrackingType groundTrackingType() const;
	bool onlineTracking() const;
	int temperature() const; // in deg. C x10 (fixed float)
	int dir() const; // in deg 0...359
	int wind() const; // in km/h x10 (fixed float)
	int gusts() const; // in km/h x10 (fixed float)
	int altitude() const; // in m
	int heading() const; // in deg
	int speed() const; // in km/h x10 (fixed float)
	int climb() const; // in m/s x10 (fixed float)
	int uptime() const; // in minutes
	int quality() const; // thermal confidence/quality in percent (0 - 100%), -1 if invalid/unknown

	static QString payloadTypeStr(PayloadType type);
	static QString aircraftTypeStr(AircraftType type);
	static QString groundTrackingTypeStr(GroundTrackingType type);
	static QString deviceFromId(quint8 manufacturerId, quint8 deviceId);

private:
	explicit FanetPayload(PayloadType type, const QByteArray &data = QByteArray());

	PayloadType m_type;
	QByteArray m_data;
};

#endif // FANETPAYLOAD_H
