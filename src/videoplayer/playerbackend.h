#ifndef PLAYERBACKEND_H
#define PLAYERBACKEND_H

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

#include "videoplayer.h"

#include <QObject>
#include <QString>
#include <QWidget>

class SCConfig;

#define PlayerBackend_iid "org.kde.SubtitleComposer.PlayerBackend"

namespace SubtitleComposer {
class PlayerBackend : public QObject
{
	Q_OBJECT

	friend class VideoPlayer;

public:
// FIXME: there should be a way for backends to abort on error

	virtual ~PlayerBackend();

	inline const QString & name() const { return m_name; }

// If possible (i.e., configs are compatible), copies the config object into
// the player backend config. Ownership of config object it's not transferred.
	void setConfig();
	virtual QWidget * newConfigWidget(QWidget *parent) = 0;

	bool isDummy() const;

protected:
	/// ownership of the config object is transferred to this object
	PlayerBackend();

	/**
	 * @brief isInitialized - There can only be one initialized backend at the time (the active
	 *  backend). Since the active backend is also guaranteed to be initialized, this
	 *  return the same as isActiveBackend() method.
	 * @return true if initialize() has been successful on this backend; false otherwise
	 */
	bool isInitialized() const;
	bool isActiveBackend() const;

	/**
	 * @brief initialize - Perform any required initialization
	 * @param widgetParent
	 * @return
	 */
	virtual bool initialize(VideoWidget *videoWidget) = 0;

	/**
	 * @brief finalize - Cleanup anything that has been initialized by initialize(), excluding the
	 *  videoWidget() which is destroyed after calling fninalize() (all references to it must be
	 *  cleaned up, however)
	 */
	virtual void finalize() = 0;

	virtual bool reconfigure() = 0;

	inline VideoPlayer * player() const { return m_player; }

	virtual bool doesVolumeCorrection() const;
	virtual bool supportsChangingAudioStream(bool *onTheFly) const;

	/**
	 * @brief openFile - If the player is not left in a state where is about
	 *  to start playing after the call, it must set the content of playingAfterCall
	 *  to false; otherwise it's content must be set to true.
	 *  The function doesn't need to block until playback is actually started
	 * @param filePath
	 * @param playingAfterCall
	 * @return false if there is an error and the opening of the file must be aborted; true (all internal cleanup must be done before returning)
	 */
	virtual bool openFile(const QString &filePath, bool &playingAfterCall) = 0;

	/**
	 * @brief closeFile - Cleanup any internal structures associated with the opened file.
	 *  This function is called with the player already stopped.
	 *  videoWidget() might be NULL when this function is called.
	 */
	virtual void closeFile() = 0;

	/**
	 * @brief play
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool play() = 0;

	/**
	 * @brief pause
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool pause() = 0;

	/**
	 * @brief seek
	 * @param seconds
	 * @param accurate
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool seek(double seconds, bool accurate) = 0;

	/**
	 * @brief step
	 * @param frameOffset
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool step(int frameOffset) = 0;

	/**
	 * @brief stop
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool stop() = 0;

	/**
	 * @brief playbackRateNotify
	 * @param new playback rate
	 */
	void playbackRateNotify(double newRate);

	/**
	 * @brief playbackRate
	 * @param new playback rate
	 */
	virtual void playbackRate(double newRate) = 0;

	/**
	 * @brief setActiveAudioStream
	 * @param audioStream
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool setActiveAudioStream(int audioStream) = 0;

	/**
	 * @brief setVolume
	 * @param volume
	 * @return false if there is an error and playback must be aborted; true (all internal cleanup must be done before returning).
	 */
	virtual bool setVolume(double volume) = 0;

	inline void setPlayerVolume(double volume) { player()->notifyVolume(volume); }
	inline void setPlayerMuted(bool muted) { player()->notifyMute(muted); }

	/**
	 * @brief setPlayerPosition
	 * @param position value in seconds
	 */
	inline void setPlayerPosition(double position) { player()->notifyPosition(position); }

	/**
	 * @brief setPlayerLength
	 * @param length value in seconds
	 */
	inline void setPlayerLength(double length) { player()->notifyLength(length); }

	inline void setPlayerState(VideoPlayer::State state) { player()->notifyState(state); }

	inline void setPlayerErrorState(const QString &errorMessage = QString()) { player()->notifyErrorState(errorMessage); }

	inline void setPlayerFramesPerSecond(double framesPerSecond) { player()->notifyFramesPerSecond(framesPerSecond); }

	inline void setPlayerTextStreams(const QStringList &textStreams) { player()->notifyTextStreams(textStreams); }

	inline void setPlayerAudioStreams(const QStringList &audioStreams, int activeAudioStream) { player()->notifyAudioStreams(audioStreams, activeAudioStream); }

private:
	inline void setPlayer(VideoPlayer *player) { m_player = player; }
	virtual void setSCConfig(SCConfig *scConfig) = 0;

private:
	VideoPlayer *m_player;

protected:
	QString m_name;
};
}

Q_DECLARE_INTERFACE(SubtitleComposer::PlayerBackend, PlayerBackend_iid)

#endif
