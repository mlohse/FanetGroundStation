/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cstdlib>
#include <syslog.h>

#include <QDebug>
#include <QtGlobal>
#include <QDateTime>
#include <QMutex>
#include <QIODevice>
#include <QMutexLocker>
#include <QTextStream>
#include <QCoreApplication>

#include "logger.h"
#include "config.h"

/**
 * This holds console colors and LogType description
 * when modifing make sure to keep the exact same order as defined in LogType
 */
static const QString LOGINFO[][2] = {{"\x1b\x5b;1;0;101m","CRITICAL"},
                                     {"\x1b\x5b;1;31;1m","ERROR"},
                                     {"\x1b\x5b;1;33;1m","WARNING"},
                                     {"\x1b\x5b;1;32;1m","NOTICE"},
                                     {"\x1b\x5b;1;34;1m","INFO"},
                                     {"\x1b\x5b;1;37;1m","debug"},
                                     {"", "unknown"}};

/**
 * Handler for catching messages from QtLib (send through qDebug, qWarning, etc.)
 */
void globalMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	//Logger::instance()->logQtMsg(type, QString("%1:%2: %3").arg(context.file).arg(context.line).arg(msg));
	Q_UNUSED(context);
	Logger::instance()->logQtMsg(type, msg);
}

Logger *Logger::s_instance = 0;
QAtomicInt Logger::s_maxLevel = Logger::Debug;

struct Logger::Private
{
	QTextStream console;
	bool consoleColors;
	Logger::LogTargets targets;
	QMutex mutex;

	Private() :
	    console(stdout, QIODevice::WriteOnly),
	    consoleColors(false),
	    targets(Logger::LogToConsole),
	    mutex()
	{
	}
};

Logger *Logger::instance()
{
	if (!s_instance)
	{
		Q_ASSERT_X(qApp->thread() == QObject().thread(), "Logger::instance()",
		           "Logger must be instantiated from main thread! Make sure to call instance() before creating any other threads.");
		s_instance = new Logger();
	}
	return s_instance;
}

void Logger::destroy()
{
	Q_ASSERT_X(qApp->thread() == QObject().thread(), "Logger::destroy()", "Logger must only be destroyed from main thread!");
	if (s_instance)
	{
		s_instance->log("Logger", Debug, "destroyed.");
		delete s_instance;
		s_instance = 0;
	}
}

Logger::Logger(const QString &className) :
    m_d(0),
    m_className(className)
{
	// 'className' helps identifying the origin of a log message a lot and therefore should always be set...
	Q_ASSERT_X(!className.isEmpty(), "Logger::Logger(QString)", "Classname must not be empty!");
}

Logger::~Logger()
{
	if (m_d)
	{
		qInstallMessageHandler(0);	// uninstall our customized message handler
		
		if (m_d->targets.testFlag(LogToSyslog))
		{
			closelog();
		}
		delete m_d;
	}
}

void Logger::debug(const QString &message)
{
	instance()->log(m_className, Debug, message);
}

void Logger::info(const QString &message)
{
	instance()->log(m_className, Info, message);
}

void Logger::notice(const QString &message)
{
	instance()->log(m_className, Notice, message);
}

void Logger::warning(const QString &message)
{
	instance()->log(m_className, Warning, message);
}

void Logger::error(const QString &message)
{
	instance()->log(m_className, Error, message);
}

void Logger::critical(const QString &message)
{
	instance()->log(m_className, Critical, message);
	std::exit(LOGGER_EXIT_CODE_CRITICAL);
}

Logger::Logger() :
    m_d(new Private())
{
	QByteArray term = qgetenv("TERM"); // try to auto-detect whether console supports colors...
	m_d->consoleColors = (term.contains("xterm") || term.contains("color"));
	qInstallMessageHandler(globalMsgHandler);
}

void Logger::log(const QString &className, LogType type, const QString &message)
{
	/// @note lock mutex after checking s_maxLevel to prevent blocking if no logging is done
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	if (s_maxLevel.load() < type || type == UnknownType)
#else
	if (s_maxLevel.loadRelaxed() < type || type == UnknownType)
#endif
	{
		return;
	}

	if (!m_d)
	{
		instance()->log(className, type, message);
		return;
	}

	QMutexLocker lock(&m_d->mutex);
	if (m_d->targets.testFlag(LogToConsole)) // log to console...
	{
		QString logMessage(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss: %1: %2: %3")
							   .arg(LOGINFO[type][1], className.isEmpty() ? "unknown" : className, message));

		if (message.endsWith('\n'))
		{
			logMessage.chop(1);
		}
		if (m_d->consoleColors)
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
			m_d->console << LOGINFO[type][0] << logMessage << "\x1b\x5b;1;0;0m" << endl; // colorful output
#else
			m_d->console << LOGINFO[type][0] << logMessage << "\x1b\x5b;1;0;0m" << Qt::endl; // colorful output
#endif
		} else
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
			m_d->console << logMessage << endl; // no colors
#else
			m_d->console << logMessage << Qt::endl; // no colors
#endif
		}
	}

	if (m_d->targets.testFlag(LogToSyslog))
	{
		static const int prio[] = { LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG };
		static_assert(sizeof(prio) == (sizeof(int) * UnknownType), "'prio' must map ALL Logger::LogType values to it's equivalent syslog prio");
		syslog(prio[type], "%s: %s", className.toUtf8().constData(), message.toUtf8().constData());
	}
}

void Logger::logQtMsg(QtMsgType type, const QString &msg)
{
	switch (type)
	{
		case QtDebugMsg:
			log("QDebug", Debug, msg);
			break;
		case QtWarningMsg:
			log("QWarning", Warning, msg);
			break;
		case QtCriticalMsg:
			log("QCritical", Error, msg);
			break;
		case QtFatalMsg:
			log("QFatal", Critical, msg);
			abort();
		default:
			break;
	}
}

void Logger::setConsoleColors(bool enabled)
{
	Logger *log = instance();
	QMutexLocker lock(&log->m_d->mutex);
	log->m_d->consoleColors = enabled;
}

bool Logger::consoleColor()
{
	Logger *log = instance();
	QMutexLocker lock(&log->m_d->mutex);
	return log->m_d->consoleColors;
}

void Logger::setLogTargets(LogTargets targets)
{
	Logger *log = instance();
	QMutexLocker lock(&log->m_d->mutex);
	if (log->m_d->targets.testFlag(LogToSyslog) && !targets.testFlag(LogToSyslog))
	{
		closelog();
	}
	if (!log->m_d->targets.testFlag(LogToSyslog) && targets.testFlag(LogToSyslog))
	{
		static const QByteArray ident(qApp ? qApp->applicationName().toUtf8() : ""); // must stay alive while log is open!
		openlog(ident.isEmpty() ? 0 : ident.constData(), LOG_ODELAY, LOG_DAEMON);    // if application name wasn't set (yet), use filename as fallback
	}
	log->m_d->targets = targets;
}

Logger::LogTargets Logger::logTargets()
{
	Logger *log = instance();
	QMutexLocker lock(&log->m_d->mutex);
	return log->m_d->targets;
}

void Logger::setLogLevel(LogType max)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	s_maxLevel.store(max);
#else
	s_maxLevel.storeRelaxed(max);
#endif
}

Logger::LogType Logger::logLevel()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return static_cast<Logger::LogType>(s_maxLevel.load());
#else
	return static_cast<Logger::LogType>(s_maxLevel.loadRelaxed());
#endif
}
