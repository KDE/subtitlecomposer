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

#include "mplayerplayerprocess.h"
#include "mplayerconfig.h"

#include <QApplication>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>

#include <QDebug>

#include <functional>

using namespace SubtitleComposer;
using namespace std;

#define MAX_VOLUME 1000

MPlayerProcess::MPlayerProcess(QObject *parent)
	: QProcess(parent),
	m_mediaData(),
	m_incompleteLine(),
	m_isMediaDataLoaded(false),
	m_isPaused(false),
	m_emitPlaying(false),
	m_version(0),
	m_revision(""),
	m_positionRegExp("^[AV]: *([0-9,:.-]+)"),
	m_videoFrameRegExp("^[AV]:.* (\\d+)\\/.\\d+"),
	m_generalTagRegExp("^(ID_.*)=(.*)"),
	m_audioTagRegExp("^ID_AID_(\\d+)_(LANG|NAME)=(.*)"),
	m_pausedTagRegExp("^ID_PAUSED"),
	m_versionTagRegExp("^MPlayer(\\d?) (\\S+) ")
{
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
	connect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(onWroteToStdin()));
	connect(&m_commandsQueueTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
	connect(this, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(onStateChanged(QProcess::ProcessState)));
}

MPlayerProcess::~MPlayerProcess()
{}

const MediaData &
MPlayerProcess::mediaData()
{
	return m_mediaData;
}

bool
MPlayerProcess::start(const QString &filePath, WId winId, int audioStream, int audioStreamCount)
{
	QString exePath = QStandardPaths::findExecutable(QUrl(MPlayerConfig::executablePath()).toLocalFile());
	if(exePath.isEmpty())
		return false;

	m_mediaData.reset();
	m_isMediaDataLoaded = false;
	m_isPaused = true;
	m_emitPlaying = false;
	m_incompleteLine.clear();

	QStringList args;

	if(audioStream >= 0 && audioStreamCount > 1)
		args << "-aid" << QString::number(audioStream);

	args << "-noquiet";
	args << "-nofs"; // no mplayer fullscreen mode
	args << "-identify"; // makes mplayer emit all kinds of additional information
	args << "-slave"; // enable slave mode so we can send commands to mplayer process
	args << "-input" << "nodefault-bindings:conf=/dev/null"; // disable mplayer input handling

	if(MPlayerConfig::videoOutputEnabled()) {
		args << "-vo" << MPlayerConfig::videoOutput();
		if(MPlayerConfig::videoOutput() == QLatin1String("vdpau")) {
			args << "-vc";
			if(MPlayerConfig::vdpau()) {
				args << "ffh264vdpau,ffmpeg12vdpau,ffwmv3vdpau,ffvc1vdpau,ffodivxvdpau,";
			} else {
				args << "ffh264vdpau,ffmpeg12vdpau,ffwmv3vdpau,ffvc1vdpau,";
			}
		}
	}

	if(MPlayerConfig::audioOutputEnabled())
		args << "-ao" << MPlayerConfig::audioOutput();

	if(MPlayerConfig::audioChannelsEnabled())
		args << "-channels" << QString::number(MPlayerConfig::audioChannels());

	args << "-zoom";                        // allow software scaling where hardware scaling is unavailable
	args << "-nokeepaspect";        // do not keep window aspect ratio when resizing windows

	if(MPlayerConfig::frameDropping())
		args << "-framedrop";

	if(MPlayerConfig::hardFrameDropping())
		args << "-hardframedrop";

	if(MPlayerConfig::autoSyncEnabled())
		args << "-autosync" << QString::number(MPlayerConfig::autoSyncFactor());

	args << "-wid" << QString::number(winId);       // set window id so that it gets embedded in our window
	args << "-noautosub";           // turn off automatic subtitle file loading

	if(MPlayerConfig::cacheEnabled()) {
		args << "-cache" << QString::number(MPlayerConfig::cacheSize());
		args << "-cache-min" << QString::number(99);
		args << "-cache-seek-min" << QString::number(99);
	}

	args << "-osdlevel" << QString::number(0);      // no OSD

	if(MPlayerConfig::volumeNormalization())
		args << "-af" << "volnorm=2"; // set volume normalization

	args << "-softvol";

	if(MPlayerConfig::volumeAmplificationEnabled())
		args << "-softvol-max" << QString::number(MPlayerConfig::volumeAmplification());

	args << filePath;

//  qDebug() << KStandardDirs::findExe( MPlayerConfig::executablePath() ) << " " << args.join(" ");

	setProcessChannelMode(QProcess::MergedChannels);
	QProcess::start(exePath, args);
	return waitForStarted(-1);
}

void
MPlayerProcess::sendTogglePause()
{
	if(m_isPaused) // set playing
		sendCommand("pause", Playing, false);
	else // set paused
		sendCommand("pause", Pausing, true);
}

void
MPlayerProcess::sendSeek(double seconds)
{
	sendCommand(QByteArray("seek % 2").replace('%', QByteArray::number(seconds)), PausingKeep, true);
}

void
MPlayerProcess::sendFastSeek(double seconds)
{
	const QByteArray seek("seek");

	for(QList<QByteArray>::Iterator it = m_commandsQueue.begin(), end = m_commandsQueue.end(); it != end;) {
		if((*it).contains(seek))
			it = m_commandsQueue.erase(it);
		else
			++it;
	}

	queueCommand(QByteArray("seek % 2").replace('%', QByteArray::number(seconds)), PausingKeep);
}

void
MPlayerProcess::sendFrameStep()
{
	queueCommand("framestep", PausingKeep);
}

void
MPlayerProcess::sendSpeedSet(double speed)
{
	const QByteArray seek("speed_set");

	for(QList<QByteArray>::Iterator it = m_commandsQueue.begin(), end = m_commandsQueue.end(); it != end;) {
		if((*it).contains(seek))
			it = m_commandsQueue.erase(it);
		else
			++it;
	}

	queueCommand(QByteArray("seek ").append(QByteArray::number(speed)), PausingKeep);
}

void
MPlayerProcess::sendToggleMute()
{
	sendCommand("mute", PausingKeep, true);
}

void
MPlayerProcess::sendVolume(double volume)
{
	sendCommand(QByteArray("volume % 1").replace('%', QByteArray::number(volume * (MPlayerConfig::volumeAmplification() / 100.0))), PausingKeep, false);
}

void
MPlayerProcess::sendAudioStream(int audioStream)
{
	sendCommand(QByteArray("switch_audio %").replace('%', QByteArray::number(audioStream)), PausingKeep, false);
}

void
MPlayerProcess::sendQuit()
{
	sendCommand("quit", PausingKeep, false);
}

void
MPlayerProcess::sendCommand(const char *cmd, MPlayerProcess::CommandMode mode, bool block)
{
	sendCommand(QByteArray(cmd), mode, block);
}

void
MPlayerProcess::sendCommand(const QByteArray &cmd, MPlayerProcess::CommandMode mode, bool block)
{
	static int count = 0;

	if(count) {
		qDebug() << "call to sendCommand already in progress";
		return;
	}

	if(state() != QProcess::Running)
		return;

	count++;

	if(mode == Pausing || (mode == PausingKeep && m_isPaused)) {
//		qDebug() << "sending pausing" << cmd;

		if(block) {
			QEventLoop loop;
			QTimer::singleShot(5000, Qt::VeryCoarseTimer, &loop, bind(&QEventLoop::exit, ref(loop), -1));
			connect(this, &MPlayerProcess::pausedReceived, &loop, &QEventLoop::quit);
			write("pausing " + cmd + '\n');
//			qDebug() << "WAITING";
			if(loop.exec() != 0)
				qDebug() << ">>>>>>>TIMEDOUT<<<<<<<";
//			qDebug() << "WAITED";
		} else {
			write("pausing " + cmd + '\n');
		}
	} else {
//		if(mode == Playing || (mode == PausingKeep && !m_isPaused))
//			qDebug() << "sending" << cmd;

		if(block) {
			QEventLoop loop;
			QTimer::singleShot(5000, Qt::VeryCoarseTimer, bind(&QEventLoop::exit, ref(loop), -1));
			connect(this, &MPlayerProcess::playingReceived, &loop, &QEventLoop::quit);
			m_emitPlaying = true;   // to make the playingReceived() signal be emitted again
			write(cmd + '\n');
//			qDebug() << "WAITING";
			if(loop.exec() != 0)
				qDebug() << ">>>>>>TIMEDOUT<<<<<<<";
//			qDebug() << "WAITED";
		} else {
			write(cmd + '\n');
		}
	}

	count--;
}

void
MPlayerProcess::queueCommand(const char *cmd, CommandMode mode)
{
	queueCommand(QByteArray(cmd), mode);
}

void
MPlayerProcess::queueCommand(const QByteArray &cmd, MPlayerProcess::CommandMode mode)
{
	switch(mode) {
	case Pausing:
		m_commandsQueue.append("pausing " + cmd + '\n');
		break;
	case PausingKeep:
		m_commandsQueue.append("pausing_keep " + cmd + '\n');
		break;
	case Playing:
		m_commandsQueue.append(cmd + '\n');
		break;
	}

	if(!m_commandsQueueTimer.isActive())
		m_commandsQueueTimer.start(100);
}

void
MPlayerProcess::onReadyReadStandardOutput()
{
	QByteArray newData = readAllStandardOutput();
	if(!newData.size())
		return;

	m_incompleteLine.append(newData);
	m_incompleteLine.replace(0x0D, '\n');

	for(int idx = m_incompleteLine.indexOf('\n'); idx > -1; idx = m_incompleteLine.indexOf('\n')) {
		parseLine(QString::fromLocal8Bit(m_incompleteLine.left(idx).constData()));
		m_incompleteLine = m_incompleteLine.mid(idx + 1);
	}
}

void
MPlayerProcess::onWroteToStdin()
{
	if(m_commandsQueue.empty())
		return;

	m_commandsQueue.removeFirst();
}

void
MPlayerProcess::onTimeout()
{
	if(!m_commandsQueue.empty())
		write(m_commandsQueue.first());
	else
		m_commandsQueueTimer.stop();
}

void
MPlayerProcess::onStateChanged(QProcess::ProcessState newState)
{
	if(newState == QProcess::NotRunning)
		emit processExited();
}

void
MPlayerProcess::parseLine(const QString &line)
{
	if(line.isEmpty())
		return;

//	qDebug() << "MP STDOUT:" << line;

	if(m_mediaData.videoFPS != 0.0 && m_videoFrameRegExp.indexIn(line) > -1) {
		// try to parse the position from the reported frame number
		if(!m_isMediaDataLoaded) {
			emit mediaDataLoaded();
			m_isMediaDataLoaded = true;
		}

		if(m_isPaused || m_emitPlaying) {
			m_isPaused = false;
			m_emitPlaying = false;
			emit playingReceived();
		}

		double position = m_videoFrameRegExp.cap(1).toInt() / m_mediaData.videoFPS;
		if(m_positionRegExp.indexIn(line) > -1) {
			double aux = m_positionRegExp.cap(1).toDouble();
			if(position - aux > 0.5 || position - aux < -0.5)
				// mplayer is reporting badly, use inaccurate (but safe) value instead
				position = aux;
		}

		emit positionReceived(position);
	} else if(m_positionRegExp.indexIn(line) > -1) {
		// parse the reported position in seconds

		if(!m_isMediaDataLoaded) {
			// in newer versions of mplayer, we must explicitly
			// hide the subtitles for formats with embedded ones
			sendCommand("sub_select -1", PausingKeep, false);

			emit mediaDataLoaded();
			m_isMediaDataLoaded = true;
		}

		if(m_isPaused || m_emitPlaying) {
			m_isPaused = false;
			m_emitPlaying = false;
			emit playingReceived();
		}

		emit positionReceived(m_positionRegExp.cap(1).toDouble());
	} else {
		// pause toggled
		if(m_pausedTagRegExp.indexIn(line) > -1) {
			m_isPaused = !m_isPaused;
			if(m_isPaused)
				emit pausedReceived();
			else
				emit playingReceived();
		}
		// The following things are not sent when the file has started to play
		// (or if sent, smplayer will ignore anyway...)
		// So not process anymore, if video is playing to save some time
		if(m_isMediaDataLoaded)
			return;

		// Matroska audio
		if(m_audioTagRegExp.indexIn(line) > -1) {
			int ID = m_audioTagRegExp.cap(1).toInt();
			if(m_audioTagRegExp.cap(2) == QLatin1String("NAME"))
				m_mediaData.audioTracks[ID].name = m_audioTagRegExp.cap(3);
			else
				m_mediaData.audioTracks[ID].language = m_audioTagRegExp.cap(3);
		} else
		// Generic things
		if(m_generalTagRegExp.indexIn(line) > -1) {
			QString tag = m_generalTagRegExp.cap(1);
			QString value = m_generalTagRegExp.cap(2);

			if(tag == QLatin1String("ID_AUDIO_ID")) {      // Generic audio
				int ID = value.toInt();
				if(!m_mediaData.audioTracks.contains(ID))
					m_mediaData.audioTracks.insert(ID, TrackData());
			} else if(tag == QLatin1String("ID_LENGTH")) {
				m_mediaData.duration = value.toDouble();
			} else if(tag == QLatin1String("ID_VIDEO_WIDTH")) {
				m_mediaData.hasVideo = true;
				m_mediaData.videoWidth = value.toInt();
			} else if(tag == QLatin1String("ID_VIDEO_HEIGHT")) {
				m_mediaData.hasVideo = true;
				m_mediaData.videoHeight = value.toInt();
			} else if(tag == QLatin1String("ID_VIDEO_ASPECT")) {
				m_mediaData.hasVideo = true;
				m_mediaData.videoDAR = value.toDouble();
				if(m_mediaData.videoDAR == 0.0 && m_mediaData.videoWidth != 0.0 && m_mediaData.videoHeight != 0)
					m_mediaData.videoDAR = (double)m_mediaData.videoWidth / m_mediaData.videoHeight;
			} else if(tag == QLatin1String("ID_VIDEO_FPS")) {
				bool ok;
				m_mediaData.videoFPS = value.toDouble(&ok);
				if(!ok)
					m_mediaData.videoFPS = 0.0;
			}
		} else if(m_version == 0 && m_versionTagRegExp.indexIn(line) > -1) {
			// parse version string
			bool ok;
			m_version = m_versionTagRegExp.cap(1).toInt(&ok);
			if(!ok)
				m_version = 1;

			m_revision = m_versionTagRegExp.cap(2);
		}
	}
}


