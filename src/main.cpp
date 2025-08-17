/**
 * SPDX-FileCopyrightText: 2025 Markus Lohse <mlohse@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <QMetaObject>
#include "application.h"
#include "logger.h"

void sig_handler(int signal)
{
	switch (signal)
	{
		case SIGINT:
			Logger("main").info("Received SIGINT, shutting down...");
			break;
		case SIGTERM:
			Logger("main").info("Received SIGTERM, shutting down...");
			break;
		default:
			return;
	}
	QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

int main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) // need to parse 'daemon' argument manually, as Application object cannot be created yet :(
	{
		const QString arg(argv[i]);
		if (arg == "-d" || arg == "--daemon") // fork to background (demonize)?
		{
			pid_t pid = fork();
			if (pid < 0)
			{
				return EXIT_FAILURE;
			}
			if (pid != 0)             // terminate the parent process
			{
				sleep(1);             // keep process running until child has written pid-file (fixes synchronisation issue with systemd)
				return EXIT_SUCCESS;
			}
			setsid();                 // make the process a group leader, session leader, and lose control tty
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			signal(SIGHUP, SIG_IGN);  // ignore sighup (which is send to child when it's parent terminates)
			umask(0);                 // loose file creation mode mask inherited by parent
			if ((pid = fork()) < 0)   // fork again, to make sure the process can't reaquire a terminal
				return EXIT_FAILURE;
			if (pid != 0)             // terminate the parent, child will get reparented to init process
				return EXIT_SUCCESS;
			signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE (reading, writing to non-opened pipes) just to be on the save side
			break;
		}
	}

	if (signal(SIGINT, sig_handler) == SIG_ERR || signal(SIGTERM, sig_handler) == SIG_ERR)
	{
		qFatal("Failed to register signal handler for SIGINT/SIGTERM, shutting down...");
		return 1;
	}

	Application app(argc, argv);
	return app.exec();
}

