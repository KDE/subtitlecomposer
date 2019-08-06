#ifndef MPLAYERPLAYERPROCESS_H
#define MPLAYERPLAYERPROCESS_H

/*
 * Copyright (C) 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * Copyright (C) 2010-2019 Mladen Milinkovic <max@smoothware.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "mediadata.h"

#include <QObject>
#include <QString>
#include <QRegExp>
#include <QByteArray>
#include <QList>
#include <QTimer>
#include <QProcess>
#include <QWindow>

namespace SubtitleComposer {
class MPlayerProcess : public QProcess
{
	Q_OBJECT

public:
	explicit MPlayerProcess(QObject *parent = 0);
	virtual ~MPlayerProcess();

	const MediaData & mediaData();

	/**
	 * @brief start
	 * @param filePath
	 * @param winId
	 * @param audioStream
	 * @param audioStreamCount
	 * @return returns false only if the executable couldn't be found (in which case the process could not be started)
	 */
	bool start(const QString &filePath, WId winId, int audioStream = -1, int audioStreamCount = 1);

	void sendTogglePause();
	void sendSeek(double seconds);
	void sendFastSeek(double seconds);
	void sendFrameStep();

	void sendSpeedSet(double speed);

	void sendToggleMute();
	void sendVolume(double volume);
	void sendAudioStream(int audioStream);

	void sendQuit();

	inline quint8 version() const { return m_version; }

	inline const QString & revision() const { return m_revision; }

signals:
	void mediaDataLoaded();
	void playingReceived();
	void pausedReceived();
	void positionReceived(double seconds);
	void processExited();

private slots:
	void onReadyReadStandardOutput();
	void onWroteToStdin();
	void onTimeout();
	void onStateChanged(QProcess::ProcessState newState);

private:
	/// indicates in which state the player is left after the execution
	/// of a command: playing, paused or the same as before.
	typedef enum { Playing, Pausing, PausingKeep } CommandMode;

	void sendCommand(const char *cmd, CommandMode mode, bool block);
	void sendCommand(const QByteArray &cmd, CommandMode mode, bool block);
	void queueCommand(const char *cmd, CommandMode mode);
	void queueCommand(const QByteArray &cmd, CommandMode mode);

	void parseLine(const QString &line);

private:
	MediaData m_mediaData;

	QList<QByteArray> m_commandsQueue;
	QTimer m_commandsQueueTimer;

	QByteArray m_incompleteLine;

	bool m_isMediaDataLoaded;
	bool m_isPaused;
	bool m_emitPlaying;

	quint8 m_version;
	QString m_revision;

	QRegExp m_positionRegExp;
	QRegExp m_videoFrameRegExp;
	QRegExp m_generalTagRegExp;
	QRegExp m_audioTagRegExp;
	QRegExp m_pausedTagRegExp;
	QRegExp m_versionTagRegExp;
};
}
#endif
