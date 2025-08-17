/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fanetpayload.h"
#include "logger.h"
#include "QtNumeric"

static const int TemperatureInvalid = -274;

FanetPayload::FanetPayload() :
    m_type(PTInvalid),
    m_data()
{
}

FanetPayload::FanetPayload(PayloadType type, const QByteArray &data) :
    m_type(type),
    m_data(data)
{
}

FanetPayload FanetPayload::fromReceivedData(PayloadType type, const QByteArray &data)
{
	static const qsizetype PAYLOAD_SIZE_GROUNDTRACKING =  7;
	static const qsizetype PAYLOAD_SIZE_TRACKING_MIN   = 11; // min. size (+ 2 bytes optional for turn rate and QNE offset)
	static const qsizetype PAYLOAD_SIZE_HWINFO_OLD_MIN =  3; // Manufacturer (Byte 0) + Firmware (Byte 1, 2) + [optional: Uptime (Byte 3, 4)/other info]
	static const qsizetype PAYLOAD_SIZE_THERMAL        = 11; // position (0-5) + altitude/qual (6-7) + avg. climb (8) + avg. wind speed (9) + avg. wind heading (10)
	switch (type)
	{
		case PTGroundTracking: // must contain lat. + long. + GroundTrackingType = 7 Byte
			if (data.size() != PAYLOAD_SIZE_GROUNDTRACKING)
			{
				Logger log("FanetPayload");
				log.warning(QString("failed to parse payload for ground tracking: invalid size (expected: %1, got: %2)")
				            .arg(PAYLOAD_SIZE_GROUNDTRACKING).arg(data.size()));
				return FanetPayload(PTInvalid, data);
			}
			return FanetPayload(PTGroundTracking, data);
		case PTTracking:
			if (data.size() < PAYLOAD_SIZE_TRACKING_MIN)
			{
				Logger log("FanetPayload");
				log.warning(QString("failed to parse payload for tracking: size too small (expected: %1, got: %2)")
				            .arg(PAYLOAD_SIZE_TRACKING_MIN).arg(data.size()));
				return FanetPayload(PTInvalid, data);
			}
			return FanetPayload(PTTracking, data);
		case PTThermal:
			if (data.size() < PAYLOAD_SIZE_THERMAL)
			{
				Logger log("FanetPayload");
				log.warning(QString("failed to parse payload for thermal: size too small (expected: %1, got: %2)")
				            .arg(PAYLOAD_SIZE_THERMAL).arg(data.size()));
			}
			return FanetPayload(PTThermal, data);
		case PTName: // fall
		case PTMessage:
			return FanetPayload(type, data);
		case PTHWInfoOld:
			return (data.size() < PAYLOAD_SIZE_HWINFO_OLD_MIN) ? FanetPayload(PTInvalid, data) : FanetPayload(PTHWInfoOld, data);
		case PTHWInfo:
		{
			qsizetype expectedSize = 1; // min. size = 1 Byte: header only
			if (!data.isEmpty())
			{
				if ((data.at(0) & 0x80) != 0) // ping-pong/pull request?
				{
					Logger log("FanetPayload");
					log.warning(QString("received pull request for hw info: Not implemented!"));
					return FanetPayload(PTInvalid, data);
				}

				expectedSize += ((data.at(0) & 0x40) != 0) ? 3 : 0; // Hardware Subtype + Build Date? + 3Byte
				expectedSize += ((data.at(0) & 0x20) != 0) ? 3 : 0; // 24Bit icao address? + 3Byte
				expectedSize += ((data.at(0) & 0x10) != 0) ? 2 : 0; // uptime? + 2Byte
				expectedSize += ((data.at(0) & 0x08) != 0) ? 4 : 0; // Rx RSSI + Fanet Address? + +1 Bytes RSSI, +3Byte addr
				expectedSize += ((data.at(0) & 0x01) != 0) ? 1 : 0; // extened header? +1Byte
			}
			if (data.size() < expectedSize)
			{
				Logger log("FanetPayload");
				log.warning(QString("failed to parse payload for hw info: size too small (expected: %1, got: %2)")
				            .arg(expectedSize).arg(data.size()));
				return FanetPayload(PTInvalid, data);
			}
			return FanetPayload(PTHWInfo, data);
		}
		case PTService:
		{
			qsizetype expectedSize = 1; // min. size = 1 Byte: header only
			if (!data.isEmpty())
			{
				ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(data.at(0));
				expectedSize += header & ~(SHExtendedHeader | SHInternetGateway | SHSupportRemoteConfig) ? 6 : 0; // position is mandatory if any data is appended, +6byte
				expectedSize += header.testFlag(SHExtendedHeader) ? 1 : 0; // extended header adds an extra byte
				expectedSize += header.testFlag(SHTemperature) ? 1 : 0; // temperature = +1byte
				expectedSize += header.testFlag(SHWind) ? 3 : 0; // winddir + speed + gusts = +3byte
				expectedSize += header.testFlag(SHHumidity) ? 1 : 0; /// @todo humidity (+1byte: in 0.4% (%rh*10/4))
				expectedSize += header.testFlag(SHPressure) ? 2 : 0; /// @todo barometric pressure normailized (+2byte: in 10Pa, offset by 430hPa, unsigned little endian (hPa-430)*10)
				expectedSize += header.testFlag(SHStateOfCharge) ? 1 : 0; /// @todo state of charge  (+1byte lower 4 bits: 0x00 = 0%, 0x01 = 6.666%, .. 0x0F = 100%)
			}
			if (data.size() < expectedSize)
			{
				Logger log("FanetPayload");
				log.warning(QString("failed to parse payload for service msg: size too small (expected: %1, got: %2)")
				            .arg(expectedSize).arg(data.size()));
				return FanetPayload(PTInvalid, data);
			}
			return FanetPayload(PTService, data);
		}
		default:
			Logger("FanetPayload").warning(QString("failed to parse payload type %1 (not implemented)!").arg(type));
			return FanetPayload(PTInvalid, data);
			break;
	}
}

FanetPayload FanetPayload::ackPayload()
{
	return FanetPayload(PTAck);
}

FanetPayload FanetPayload::namePayload(const QString &name)
{
	return FanetPayload(PTName, name.toLatin1());
}

FanetPayload FanetPayload::messagePayload(const QString &msg)
{
	QByteArray header(1, static_cast<char>(0)); // header: 0x00 (normal message)
	return FanetPayload(PTMessage, header.append(msg.toLatin1()));
}

FanetPayload FanetPayload::servicePayload(ServiceHeaderFlags header, const QGeoCoordinate &pos, int temp, int dir, int wind, int gusts, int humidity, int pressure)
{
	/**
	Service (Type = 4)
	[recommended interval: 40sec]

	[Byte 0]	Header	(additional payload will be added in order 6 to 1, followed by Extended Header payload 7 to 0 once defined)
	bit 7		Internet Gateway (no additional payload required, other than a position)
	bit 6		Temperature (+1byte in 0.5 degree, 2-Complement)
	bit 5		Wind (+3byte: 1byte Heading in 360/256 degree, 1byte speed and 1byte gusts in 0.2km/h (each: bit 7 scale 5x or 1x, bit 0-6))
	bit 4		Humidity (+1byte: in 0.4% (%rh*10/4))
	bit 3		Barometric pressure normailized (+2byte: in 10Pa, offset by 430hPa, unsigned little endian (hPa-430)*10)
	bit 2		Support for Remote Configuration (Advertisement)
	bit 1		State of Charge  (+1byte lower 4 bits: 0x00 = 0%, 0x01 = 6.666%, .. 0x0F = 100%)
	bit 0		Extended Header (+1byte directly after byte 0)
			The following is only mandatory if no additional data will be added. Broadcasting only the gateway/remote-cfg flag doesn't require pos information.
	[Byte 1-3 or Byte 2-4]	Position	(Little Endian, 2-Complement)
	bit 0-23	Latitude	(Absolute, see below)
	[Byte 4-6 or Byte 5-7]	Position	(Little Endian, 2-Complement)
	bit 0-23	Longitude   (Absolute, see below)
	+ additional data according to the sub header order (bit 6 down to 1)
	*/
	QByteArray data;
	data.append(static_cast<char>(header));

	// coordinates...
	const int lat = pos.isValid() ? qRound(pos.latitude() * 93206) : 0;
	const int lon = pos.isValid() ? qRound(pos.longitude() * 46603) : 0;
	data.append(lat & 0x000000FF);
	data.append((lat & 0x0000FF00) >>  8);
	data.append((lat & 0x00FF0000) >> 16);
	data.append(lon & 0x000000FF);
	data.append((lon & 0x0000FF00) >>  8);
	data.append((lon & 0x00FF0000) >> 16);

	// byte 7: temperature...
	if (header.testFlag(SHTemperature))
	{
		//data[i++] = static_cast<qint8>(qRound(temp / 5.0));
		data.append(static_cast<qint8>(qRound(temp / 5.0)));
	}

	// byte 8-10: Wind
	if (header.testFlag(SHWind))
	{
		data.append(static_cast<quint8>(qRound(dir * 256.0 / 360.0)));
		data.append(wind > 254 ? 0x80 | static_cast<quint8>(qRound(wind / 10.0)) : 0x7F & (wind >> 1));
		data.append(gusts > 254 ? 0x80 | static_cast<quint8>(qRound(gusts / 10.0)) : 0x7F & (gusts >> 1));
	}

	// byte 11: Humidity
	if (header.testFlag(SHHumidity))
	{
		data.append(static_cast<quint8>(qRound(humidity / 4.0)));
	}

	// byte 12-13: pressure
	if (header.testFlag(SHPressure))
	{
		const quint16 pres = (pressure - 430) * 10;
		data.append(pres & 0xFF);
		data.append((pres & 0xFF00) >> 8);
	}

	return FanetPayload(PTService, data);
}

QString FanetPayload::name() const
{
	return (m_type == PTName ? QString::fromLatin1(m_data) : QString());
}

QString FanetPayload::message() const
{
	return (m_type == PTMessage ? QString::fromLatin1(m_data) : QString());
}

QString FanetPayload::deviceType(quint8 manufacturerId) const
{
	quint8 deviceId = 0;
	switch (m_type)
	{
		case PTHWInfo:
			if ((m_data.at(0) & 0x40) != 0)
			{
				deviceId = m_data.at(((m_data.at(0) & 0x01) != 0) ? 2 : 1);
			}
			break;
		case PTHWInfoOld:
			deviceId = m_data.at(0);
			break;
		default:
			break;
	}
	return deviceFromId(manufacturerId, deviceId);
}

QString FanetPayload::firmwareBuild() const
{
	int index = 0;
	switch (m_type)
	{
		case PTHWInfo:
			if ((m_data.at(0) & 0x40) != 0)
			{
				index = ((m_data.at(0) & 0x01) != 0) ? 3 : 2;
			}
			break;
		case PTHWInfoOld:
			index = 1;
			break;
		default:
			break;
	}
	if (index)
	{
		const quint16 data = (static_cast<quint8>(m_data.at(index)) |
		                      static_cast<quint8>(m_data.at(index + 1)) << 8);
		const bool experimental = (data & 0x8000) != 0;
		const int day = data & 0x001F;
		const int mon = (data & 0x01E0) >> 5;
		const int year = ((data & 0x7E00) >> 9) + 2019;
		return QString("%1-%2-%3%4").arg(year).arg(mon).arg(day).arg(experimental ? " (experimental)" : "");
	}
	return "";
}

int FanetPayload::uptime() const
{
	switch (m_type)
	{
		case PTHWInfo:
			if ((m_data.at(0) & 0x10) != 0)
			{
				int index = ((m_data.at(0) & 0x01) != 0) ? 2 : 1; // header + extended header? + 1
				index += ((m_data.at(0) & 0x40) != 0) ? 3 : 0; // device type and fw build? + 3byte
				return (static_cast<quint8>(m_data.at(index)) | static_cast<quint8>(m_data.at(index + 1)) << 8);
			}
			break;
		case PTHWInfoOld:
			if (m_data.size() >= 5) // Uptime: Byte 3 and 4 (Bit 15-4) may hold uptime in 30sec. steps (optional!)
			{
				const int t = (((static_cast<quint8>(m_data.at(4)) & 0xF0) << 4) | static_cast<quint8>(m_data.at(3)));
				return (t >> 2); // convert to minutes
			}
			break;
		default:
			break;
	}
	return -1;
}

int FanetPayload::quality() const
{
	/**
	 * Thermal (Type = 9)
	 * [Byte 10]	Avg wind heading at thermal (attention: 90degree means the wind is coming from east and blowing towards west)
	 * bit 0-7		Value in 360/256 deg
	 */
	return (m_type == FanetPayload::PTThermal ? (100 * ((static_cast<quint8>(m_data.at(7)) & 0x70) >> 4) / 7) : -1);
}

QGeoCoordinate FanetPayload::position() const
{
	const int POS_SIZE = 6; // coodinate size = 6byte
	int offset = 0;
	switch (m_type)
	{
		case PTService:
		{
			ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(m_data.at(0));
			offset += header.testFlags(SHExtendedHeader) ? 2 : 1; // extended header takes 1 extra byte
			if (m_data.size() < (POS_SIZE + offset)) // no payload?
			{
				return QGeoCoordinate();
			}
			// fall
		}
		case PTThermal:
		case PTTracking:
		case PTGroundTracking:
		{
			// latitude:  Byte 0 - 2 (Little Endian, 2-Complement)
			// longitude: Byte 3 - 5 (Little Endian, 2-Complement)
			const qint32 ilat = (static_cast<quint8>(m_data.at(offset + 0))       |
			                     static_cast<quint8>(m_data.at(offset + 1)) << 8  |
			                     static_cast<quint8>(m_data.at(offset + 2)) << 16 |
			                     ((m_data.at(offset + 2) & 0x80) ? 0xFF000000 : 0x00000000));
			const qint32 ilon = (static_cast<quint8>(m_data.at(offset + 3))       |
			                     static_cast<quint8>(m_data.at(offset + 4)) << 8  |
			                     static_cast<quint8>(m_data.at(offset + 5)) << 16 |
			                     ((m_data.at(offset + 5) & 0x80) ? 0xFF000000 : 0x00000000));
			const double lat = ilat / 93206.0;
			const double lon = ilon / 46603.0;
			return QGeoCoordinate(lat, lon);
		}
		default:
			return QGeoCoordinate();
	}
}

FanetPayload::AircraftType FanetPayload::aircraftType() const
{
	/**
	 * Tracking (Type = 1)
	 * [Byte 6-7]	Type		(Little Endian)
	 * bit 15 		Online Tracking
	 * bit 12-14	Aircraft Type
	 *		0: Other
	 *		1: Paraglider
	 *		2: Hangglider
	 *		3: Balloon
	 *		4: Glider
	 *		5: Powered Aircraft
	 *		6: Helicopter
	 *		7: UAV
	 * bit 11		Altitude Scaling 1->4x, 0->1x
	 * bit 0-10	Altitude in m
	*/
	if (m_type == PTTracking)
	{
		return static_cast<AircraftType>((m_data.at(7) >> 4) & 0x07);
	}
	return ATOther;
}

FanetPayload::GroundTrackingType FanetPayload::groundTrackingType() const
{
	/**
	 * Ground Tracking (Type = 7)
	 * [Byte 6]
	 * bit 7-4		Type
	 * 			0:    Other
	 * 			1:    Walking
	 * 			2:    Vehicle
	 * 			3:    Bike
	 * 			4:    Boot
	 * 			8:    Need a ride
	 * 			9:    Landed well
	 * 			12:   Need technical support
	 * 			13:   Need medical help
	 * 			14:   Distress call
	 * 			15:   Distress call automatically
	 * 			Rest: TBD
	 * bit 3-1		TBD
	 * bit 0		Online Tracking
	 */
	if (m_type == PTGroundTracking)
	{
		return static_cast<GroundTrackingType>((m_data.at(6) & 0xF0) >> 4);
	}
	return GTOther;
}

bool FanetPayload::onlineTracking() const
{
	switch (m_type)
	{
		case PTTracking:
			/**
			 * Tracking (Type = 1)
			 * [Byte 6-7]	Type		(Little Endian)
			 * bit 15 		Online Tracking
			 */
			return (m_data.at(7) & 0x80) != 0;
		case PTGroundTracking:
			/**
			 * Ground Tracking (Type = 7)
			 * [Byte 6]
			 * bit 0		Online Tracking
			 */
			return (m_data.at(6) & 0x01) != 0;
		default:
			return false;
	}
}

int FanetPayload::temperature() const
{
	ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(m_type == PTService ? m_data.at(0) : 0);
	if (header.testFlags(SHTemperature))
	{
		const int offset = header.testFlag(SHExtendedHeader) ? 8 : 7; // 1byte header + [1byte ext. header] + 6byte position
		return (static_cast<qint8>(m_data.at(offset)) * 5); // = data * 0.5 * 10 (fixed point: deg.C x10)
	}
	return (TemperatureInvalid * 10);
}

int FanetPayload::dir() const
{
	ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(m_type == PTService ? m_data.at(0) : 0);
	if (header.testFlags(SHWind))
	{
		int offset = header.testFlag(SHExtendedHeader) ? 8 : 7; // 1byte header + [1byte ext. header] + 6byte position
		offset += header.testFlag(SHTemperature) ? 1 : 0; // add 1 byte for temperature
		return qRound((static_cast<quint8>(m_data.at(offset) * 360) / 256.0));
	}
	return -1;
}

int FanetPayload::wind() const
{
	ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(m_type == PTService ? m_data.at(0) : 0);
	if (header.testFlags(SHWind))
	{
		int offset = header.testFlag(SHExtendedHeader) ? 8 : 7; // 1byte header + [1byte ext. header] + 6byte position
		offset += header.testFlag(SHTemperature) ? 1 : 0; // add 1 byte for temperature
		quint8 data = m_data.at(offset + 1); // wind data: dir + wind + gusts
		const int scale = ((data & 0x80) != 0) ? 25 : 5;
		const int speed = data & 0x7F; // in 0.5 km/h
		return (speed * scale); // in km/h * 10 (fixed point)
	}
	return -1;
}

int FanetPayload::gusts() const
{
	ServiceHeaderFlags header = static_cast<ServiceHeaderFlags>(m_type == PTService ? m_data.at(0) : 0);
	if (header.testFlags(SHWind))
	{
		int offset = header.testFlag(SHExtendedHeader) ? 8 : 7; // 1byte header + [1byte ext. header] + 6byte position
		offset += header.testFlag(SHTemperature) ? 1 : 0; // add 1 byte for temperature
		quint8 data = m_data.at(offset + 2); // wind data: dir + wind + gusts
		const int scale = ((data & 0x80) != 0) ? 25 : 5;
		const int speed = data & 0x7F; // in 0.5 km/h
		return (speed * scale); // in km/h * 10 (fixed point)
	}
	return -1;
}

int FanetPayload::altitude() const
{
	/**
	 * Tracking (Type = 1) and Thermal (Type = 9)
	 * [Byte 6-7]	Type		(Little Endian)
	 * bit 11		Altitude Scaling 1->4x, 0->1x
	 * bit 0-10	Altitude in m
	*/
	switch (m_type)
	{
		case PTTracking:
		case PTThermal:
		{
			const int scale = ((m_data.at(7) & 0x08) != 0) ? 4 : 1; // scale x4 if Bit 11 is set
			const quint16 alt = static_cast<quint8>(m_data.at(6)) | (m_data.at(7) & 0x07) << 8; // Bit 0 - 10, little endian
			return (scale * alt);
		}
		default:
			return -1;
	}
}

int FanetPayload::heading() const
{
	/**
	 * Tracking (Type = 1) and Thermal (Type = 9)
	 * [Byte 10]	Heading
	 *		bit 0-7		Value		in 360/256 deg
	 */
	switch (m_type)
	{
		case PTTracking:
		case PTThermal:
		{
			const double data = static_cast<quint8>(m_data.at(10));
			return qRound((data * 360.0) / 256.0);
		}
		default:
			return -1;
	}
}

int FanetPayload::speed() const
{
	/**
	 * Tracking (Type = 1), [Byte 8] Speed (max 317.5km/h)
	 * Thermal  (Type = 9), [Byte 9] Avg. wind speed at thermal (max 317.5km/h)
	 *		bit 7		Scaling 	1->5x, 0->1x
	 *		bit 0-6		Value		in 0.5km/h
	 */
	quint8 data;
	switch (m_type)
	{
		case PTTracking: data = m_data.at(8); break;
		case PTThermal:  data = m_data.at(9); break;
		default: return -1;
	}
	const int scale = ((data & 0x80) != 0) ? 25 : 5;
	const int speed = data & 0x7F; // in 0.5 km/h
	return (speed * scale); // in km/h * 10 (fixed point)
}

int FanetPayload::climb() const
{
	/**
	 * Tracking (Type = 1), [Byte 9] Climb (max +/- 31.5m/s, 2-Complement)
	 * Thermal (Type = 9), [Byte 8] Avg climb of thermal (max +/- 31.5m/s, 2-Complement, climb of air NOT the paraglider)
	 * bit 7		Scaling 	1->5x, 0->1x
	 * bit 0-6		Value		in 0.1m/s
	 */
	quint8 data;
	switch (m_type)
	{
		case PTTracking: data = m_data.at(9); break;
		case PTThermal:  data = m_data.at(8); break;
		default: return -1;
	}
	const bool negative = (data & 0x40) != 0;
	const int scale = ((data & 0x80) != 0) ? 5 : 1;
	const int climb = static_cast<qint8>(negative ? (data | 0x80) : (data & 0x7F));
	return (climb * scale); // in m/s * 10 (fixed point)
}

QString FanetPayload::payloadTypeStr(PayloadType type)
{
	switch (type)
	{
		case PTAck:            return "Ack";
		case PTTracking:       return "Tracking";
		case PTName:           return "Name";
		case PTMessage:        return "Message";
		case PTService:        return "Service";
		case PTLandmarks:      return "Landmarks";
		case PTRemoteConfig:   return "RemoteConfig";
		case PTGroundTracking: return "GroundTracking";
		case PTHWInfoOld:      return "HwInfo(deprecated)";
		case PTThermal:        return "Thermal";
		case PTHWInfo:         return "HwInfo";
		default:               return "Invalid";
	}
}

QString FanetPayload::aircraftTypeStr(AircraftType type)
{
	switch (type)
	{
		case ATParaglider:     return "Paraglider";
		case ATHangglider:     return "Hangglider";
		case ATBallon:         return "Ballon";
		case ATGlider:         return "Glider";
		case ATPoweredAicraft: return "PoweredAircraft";
		case ATHelicopter:     return "Helicopter";
		case ATUAV:            return "uav";
		default:               return "other";
	}
}

QString FanetPayload::groundTrackingTypeStr(GroundTrackingType type)
{
	switch (type)
	{
		case GTWalking:          return "Walking";
		case GTVehicle:          return "Vehicle";
		case GTBike:             return "Bike";
		case GTBoot:             return "Boot";
		case GTNeedARide:        return "Need a ride";
		case GTLandedWell:       return "Landed well";
		case GTNeedTechSupport:  return "Need technical support";
		case GTNeedMedicalHelp:  return "Need medical help";
		case GTDistressCall:     return "Distress call";
		case GTDistressCallAuto: return "Distress call (automatically)";
		default:                 return "Other";
	}
}

QString FanetPayload::deviceFromId(quint8 manufacturerId, quint8 deviceId)
{
	switch (manufacturerId)
	{
		case 0x00:
			return "reserved/invalid";
		case 0x01: // Skytraxx
			return (deviceId == 0x01 ? "Skytraxx Wind station" : "Skytraxx unknown");
		case 0x03:
			return "BitBroker.eu";
		case 0x04:
			return "AirWhere";
		case 0x05:
			return "Windline";
		case 0x06: // Burnair
			return (deviceId == 0x01 ? "Burnair base station WiFi" : "Burnair unknown");
		case 0x07:
			return "SoftRF";
		case 0x08:
			return "GXAircom";
		case 0x09:
			return "Airtribune";
		case 0x0A:
			return "FLARM";
		case 0x0B:
			return "FlyBeeper";
		case 0x0C:
			return "Leaf Vario";
		case 0x10:
			return "alfapilot";
		case 0x11: // FANET+
			switch (deviceId)
			{
				case 0x01: return "Skytraxx 3.0";
				case 0x02: return "Skytraxx 2.1";
				case 0x03: return "Skytraxx Beacon";
				case 0x04: return "Skytraxx 4.0";
				case 0x05: return "Skytraxx 5";
				case 0x06: return "Skytraxx 5mini";
				case 0x10: return "Naviter Oudie 5";
				case 0x11: return "Naviter Blade";
				case 0x12: return "Naviter Oudie N";
				case 0x20: return "Skybean Strato";
				default: return "FANET+ unknown";
			}
		case 0x20:
			return "XC Tracer";
		case 0xCB:
			return "Cloudbuddy";
		case 0xDD:
		case 0xDE:
		case 0xDF:
		case 0xF0:
			return "reserved/compat.";
		case 0xE0:
			return "OGN Tracker";
		case 0xE4:
			return "4aviation";
		case 0xFA:
			return "Various/GetroniX";
		case 0xFB: //
			return (deviceId == 0x01 ? "Skytraxx WiFi base station" : "Espressif base station");
		case 0xFC:
		case 0xFD:
			return "Unregistered device";
		default:
			break;
	}
	return QString("unknown");
}

