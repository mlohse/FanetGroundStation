/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFlags>

/**
 * @class Logger enables the application to send log messages to standard out (console) and/or Syslog
 *        It uses the singleton pattern. However, further Logger-Objects can be instantiated holding
 *        only a class/module-name, allowing the reader to easily identify the origin of a log message.
 *
 * @note This class is not fully thread-safe!!!
 *       The main thread (owning the event queue) must be the first thread to either call @fn instance()
 *       or write a log message. Also @fn destroy() must be called from the main thread only - after all
 *       other threads (which may need to log) have been terminated.
 *       Further, please keep in ming that using qDebug(), qWarning(), qFatal(), qCritical() will result
 *       in an invokation of @fn Logger::log() - this must not happen before the global instance was
 *       created by the main thread.
 *       Besides these limitations it is perfectly safe to log from multiple threads simultaneously.
 */
class Logger
{
public:
	/**
	  * Log output targets flags
	  */
	enum LogTarget
	{
		LogDisabled  = 0x00, /** Hint: use this to disable the logger */
		LogToConsole = 0x01,
		LogToSyslog  = 0x02
	};
	Q_DECLARE_FLAGS(LogTargets, LogTarget)

	/**
	 * Log message type
	 */
	typedef enum
	{
		Critical = 0,
		Error,
		Warning,
		Notice,
		Info,
		Debug,
		UnknownType // logger internal use only (this must always be the last entry!)
	} LogType;

	/**
	 * Returns the global Logger instance.
	 * @warning Must be called by the main thread first! (e.g. by writing a log message before creating other threads)
	 *
	 * @return Logger instance.
	 */
	static Logger *instance();

	/**
	 * Destroys the global Logger instance.
	 * @warning Must only be called from main thread (after all other threads have been terminated)!!!
	 */
	static void destroy();

	/**
	 * Constructor.
	 *
	 * You can create private instances for convenience, that will pass the
	 * given @p className to the global Logger.
	 *
	 * @param className Name of the class instantiating the Logger. This name will
	 *                  be used in log messages. Do not leave empty!
	 */
	Logger(const QString &className);
	virtual ~Logger();

	/**
	 * Logs a debug message.
	 *
	 * @param message Message to log.
	 */
	virtual void debug(const QString &message);

	/**
	 * Logs an info message.
	 * @param message Message to log.
	 */
	virtual void info(const QString &message);

	/**
	 * Logs a notice message.
	 * @param message Message to log.
	 */
	virtual void notice(const QString &message);

	/**
	 * Logs a warning message.
	 * @param message Message to log.
	 */
	virtual void warning(const QString &message);

	/**
	 * Logs an error message.
	 * @param message Message to log.
	 */
	virtual void error(const QString &message);

	/**
	 * Logs a critical message and exits aplication with error code
	 * @param message Message to log.
	 * @note this function will not return!
	 */
	virtual void critical(const QString &message);

	/**
	 * Returns the className the Logger has been instaanciated with
	 */
	QString className() const { return m_className; }

	/**
	 * Log messages caught from QtLib (send by qDebug, qWarning, qCritical, qFatal)
	 *
	 * This is invoked by globalMsgHandler (see Logger.cpp)
	 * @param QtMsgType Type of message
	 * @param msg the message to be logged
	 */
	void logQtMsg(QtMsgType type, const QString &msg);

	/**
	 * Enables/Disables colorful logging to console
	 * When the logger is created it tries to auto-detect whether the terminal supports colors by
	 * evaluating the TERM environment variable
	 * @note this doesn't have any effect on Windows (always disabled) or the logfile
	 */
	static void setConsoleColors(bool enabled);

	/**
	 * @returns whether colorful logging to console is enabled
	 */
	static bool consoleColor();

	/**
	 * Defines where messages are being logged, eg. console, syslog, etc...
	 * @param targets new log output targets, @see LogTarget (multiple targets may be or'd together)
	 * @default LogTarget::LogToConsole
	 * @note: Please make sure to call QtCoreApplication::setApplicationName() before enabling 'LogToSyslog'
	 */
	static void setLogTargets(LogTargets targets);

	/**
	 * @returns current log output targets
	 */
	static LogTargets logTargets();

	/**
	 * Sets the max. log level
	 * @default Logger::Debug
	 * @param max level to be logged
	 */
	static void setLogLevel(LogType max);

	/**
	 * @returns the current max. log level
	 */
	static LogType logLevel();
	
protected:
	void log(const QString &className, LogType type, const QString &message);
	
private:
	static Logger *s_instance;
	static QAtomicInt s_maxLevel;

	struct Private;
	Private *const m_d;
	QString const m_className;

	/**
	 * Private constructor used by instance()
	 */
	Logger();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Logger::LogTargets)

#endif // LOGGER_H
