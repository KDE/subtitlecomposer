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

#include "gstreamerbackend.h"
#include "gstreamerconfigwidget.h"
#include "gstreamerconfig.h"
#include "gstreamer.h"
#include "helpers/languagecode.h"

#include <KLocalizedString>

#include <QTimer>
#include <QtMath>

#include <QDebug>
#include <QUrl>
#include <QResizeEvent>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#define MESSAGE_UPDATE_AUDIO_DATA 0
#define MESSAGE_UPDATE_VIDEO_DATA 1

#define MAX_VOLUME 3.548

using namespace SubtitleComposer;

#ifndef __GST_PLAY_ENUM_H__
/**
 * GstPlayFlags:
 * @GST_PLAY_FLAG_VIDEO: Enable rendering of the video stream
 * @GST_PLAY_FLAG_AUDIO: Enable rendering of the audio stream
 * @GST_PLAY_FLAG_TEXT: Enable rendering of subtitles
 * @GST_PLAY_FLAG_VIS: Enable rendering of visualisations when there is
 *       no video stream.
 * @GST_PLAY_FLAG_SOFT_VOLUME: Use software volume
 * @GST_PLAY_FLAG_NATIVE_AUDIO: only allow native audio formats, this omits
 *   configuration of audioconvert and audioresample.
 * @GST_PLAY_FLAG_NATIVE_VIDEO: only allow native video formats, this omits
 *   configuration of videoconvert and videoscale.
 * @GST_PLAY_FLAG_DOWNLOAD: enable progressice download buffering for selected
 *   formats.
 * @GST_PLAY_FLAG_BUFFERING: enable buffering of the demuxed or parsed data.
 * @GST_PLAY_FLAG_DEINTERLACE: deinterlace raw video (if native not forced).
 * @GST_PLAY_FLAG_FORCE_FILTERS: force audio/video filters to be applied if
 *   set.
 *
 * Extra flags to configure the behaviour of the sinks.
 */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
  GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10),
  GST_PLAY_FLAG_FORCE_FILTERS = (1 << 11),
} GstPlayFlags;
#endif /* __GST_PLAY_ENUM_H__ */

GStreamerBackend::GStreamerBackend()
	: PlayerBackend(),
	  m_nativeWindow(nullptr),
	  m_pipeline(nullptr),
	  m_pipelineBus(nullptr),
	  m_pipelineTimer(new QTimer(this)),
	  m_lengthInformed(false),
	  m_playbackRate(1.),
	  m_volume(.0),
	  m_muted(true)
{
	m_name = QStringLiteral("GStreamer");
	connect(m_pipelineTimer, SIGNAL(timeout()), this, SLOT(onPlaybinTimerTimeout()));
	connect(GStreamerConfig::self(), &GStreamerConfig::configChanged, this, &GStreamerBackend::reconfigure);
}

GStreamerBackend::~GStreamerBackend()
{
	GStreamer::deinit();
}

bool
GStreamerBackend::init(QWidget *videoWidget)
{
	if(!GStreamer::init())
		return false;

	if(!m_nativeWindow) {
		m_nativeWindow = new QWidget(videoWidget);
		m_nativeWindow->setAttribute(Qt::WA_DontCreateNativeAncestors);
		m_nativeWindow->setAttribute(Qt::WA_NativeWindow);
		connect(m_nativeWindow, &QWidget::destroyed, [&](){ m_nativeWindow = nullptr; });
	} else {
		m_nativeWindow->setParent(videoWidget);
	}

	m_nativeWindow->installEventFilter(this);
	onPlaybinTimerTimeout();
	return true;
}

void
GStreamerBackend::cleanup()
{
	return GStreamer::deinit();
}

QWidget *
GStreamerBackend::newConfigWidget(QWidget *parent)
{
	return new GStreamerConfigWidget(parent);
}

KCoreConfigSkeleton *
GStreamerBackend::config() const
{
	return GStreamerConfig::self();
}


void
GStreamerBackend::setupVideoOverlay()
{
	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_pipeline), m_nativeWindow->winId());
	gst_video_overlay_expose(GST_VIDEO_OVERLAY(m_pipeline));
}

GstElement *
GStreamerBackend::createAudioSink()
{
	static const QString sinks(QStringLiteral(" pulsesink alsasink osssink gconfaudiosink artsdsink autoaudiosink"));

	if(GStreamerConfig::audioSinkAuto())
		return GStreamer::createElement(GStreamerConfig::audioSink() + sinks, "audiosink");
	return GStreamer::createElement(sinks, "audiosink");
}

GstElement *
GStreamerBackend::createVideoSink()
{
	static const QString sinks(QStringLiteral(" autovideosink glimagesink xvimagesink ximagesink"));

	if(GStreamerConfig::videoSinkAuto())
		return GStreamer::createElement(GStreamerConfig::videoSink() + sinks, "videosink");
	return GStreamer::createElement(sinks, "videosink");
}

bool
GStreamerBackend::openFile(const QString &path)
{
	m_lengthInformed = false;

	m_pipeline = GST_PIPELINE(gst_element_factory_make("playbin", "playbin"));
	GstElement *audiosink = createAudioSink();
	GstElement *videosink = createVideoSink();

	GstElement *audiobin = gst_bin_new("audiobin");
	GstElement *scaletempo = gst_element_factory_make("scaletempo", "scaletempo");
	GstElement *convert = gst_element_factory_make("audioconvert", "convert");
	GstElement *resample = gst_element_factory_make("audioresample", "resample");

	bool audiobin_ok = false;

	if(audiobin && scaletempo && convert && resample && audiosink) {
		GstPad *padSink = nullptr;
		gst_bin_add_many(GST_BIN(audiobin), scaletempo, convert, resample, audiosink, nullptr);
		audiobin_ok = gst_element_link(scaletempo, convert)
			&& gst_element_link(convert, resample)
			&& gst_element_link(resample, audiosink)
			&& (padSink = gst_element_get_static_pad(scaletempo, "sink")) != nullptr
			&& gst_element_add_pad(audiobin, gst_ghost_pad_new("sink", padSink));

		if(padSink)
			g_object_unref(padSink);
	}

	if(!audiobin_ok) {
		if(scaletempo)
			gst_object_unref(GST_OBJECT(scaletempo));
		if(convert)
			gst_object_unref(GST_OBJECT(convert));
		if(resample)
			gst_object_unref(GST_OBJECT(resample));
		if(audiobin)
			gst_object_unref(GST_OBJECT(audiobin));
		// output audio without scaletempo plugin
		audiobin = audiosink;
	}

	if(!m_pipeline || !audiosink || !videosink) {
		if(audiosink)
			gst_object_unref(GST_OBJECT(audiosink));
		if(videosink)
			gst_object_unref(GST_OBJECT(videosink));
		if(m_pipeline)
			gst_object_unref(GST_OBJECT(m_pipeline));
		m_pipeline = nullptr;
		return false;
	}

	QUrl fileUrl;
	fileUrl.setScheme("file");
	fileUrl.setPath(path);

	g_object_set(G_OBJECT(m_pipeline), "uri", fileUrl.url().toUtf8().constData(), nullptr);
	g_object_set(G_OBJECT(m_pipeline), "suburi", 0, nullptr);

	// disable embedded subtitles
	gint flags = 0;
	g_object_get(G_OBJECT(m_pipeline), "flags", &flags, nullptr);
	g_object_set(G_OBJECT(m_pipeline), "flags", flags & ~GST_PLAY_FLAG_TEXT, nullptr);

	// the volume is adjusted when file playback starts and it's best if it's initially at 0
	g_object_set(G_OBJECT(m_pipeline), "volume", (gdouble)0.0, nullptr);

	g_object_set(G_OBJECT(m_pipeline), "audio-sink", audiobin, nullptr);
	g_object_set(G_OBJECT(m_pipeline), "video-sink", videosink, nullptr);

	m_pipelineBus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	m_pipelineTimer->start(20);

	setupVideoOverlay();

	GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PLAYING, GST_CLOCK_TIME_NONE);

	return true;
}

bool
GStreamerBackend::closeFile()
{
	if(m_pipeline) {
		m_pipelineTimer->stop();
		GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_NULL, GST_CLOCK_TIME_NONE);
		GStreamer::freePipeline(&m_pipeline, &m_pipelineBus);
	}
	return true;
}

bool
GStreamerBackend::play()
{
	setupVideoOverlay();
	GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PLAYING, GST_CLOCK_TIME_NONE);

	return true;
}

bool
GStreamerBackend::pause()
{
	GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED, GST_CLOCK_TIME_NONE);

	return true;
}

bool
GStreamerBackend::seek(double seconds)
{
	gst_element_seek(GST_ELEMENT(m_pipeline), m_playbackRate,
		GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
		GST_SEEK_TYPE_SET, (gint64)(seconds * GST_SECOND),
		GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

	return true;
}

bool
GStreamerBackend::step(int frameOffset)
{
	GstState currentState = GST_STATE_VOID_PENDING, pendingState = GST_STATE_VOID_PENDING;
	gst_element_get_state(GST_ELEMENT(m_pipeline), &currentState, &pendingState, GST_CLOCK_TIME_NONE);

	if(currentState != GST_STATE_PAUSED && pendingState != GST_STATE_PAUSED)
		GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED, 0);
	return gst_element_seek(GST_ELEMENT(m_pipeline), m_playbackRate,
		GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
		GST_SEEK_TYPE_SET, m_currentPosition + gint64(frameOffset) * m_frameDuration,
		GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

bool
GStreamerBackend::playbackRate(double newRate)
{
	m_playbackRate = newRate;

	gint64 time;
	if(gst_element_query_position(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &time)) {
		emit positionChanged((double)time / GST_SECOND);

		gst_element_seek(GST_ELEMENT(m_pipeline), newRate,
			GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
			GST_SEEK_TYPE_SET, time, // we need to set the time otherwise playback will jump
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
		return true;
	}
	return false;
}

bool
GStreamerBackend::stop()
{
	GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_READY, GST_CLOCK_TIME_NONE);
	return true;
}

bool
GStreamerBackend::selectAudioStream(int streamIndex)
{
	g_object_set(G_OBJECT(m_pipeline), "current-audio", (gint)streamIndex, nullptr);
	return true;
}

bool
GStreamerBackend::setVolume(double volume)
{
	g_object_set(G_OBJECT(m_pipeline), "volume", (gdouble)(qPow(volume / 100., 3.) * MAX_VOLUME), nullptr);
	g_object_get(G_OBJECT(m_pipeline), "volume", &m_volume, nullptr); // fix volume jumping around when changing it from gui
	return true;
}

void
GStreamerBackend::onPlaybinTimerTimeout()
{
	if(!m_pipeline || !m_pipelineBus)
		return;

	gint64 time;
	if(!m_lengthInformed && gst_element_query_duration(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &time) && GST_CLOCK_TIME_IS_VALID(time)) {
		emit lengthChanged((double)time / GST_SECOND);
		m_lengthInformed = true;
	}
	if(gst_element_query_position(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &time)) {
		emit positionChanged((double)time / GST_SECOND);
		m_currentPosition = time;
	}

	GstState currentState = GST_STATE_VOID_PENDING;
	gst_element_get_state(GST_ELEMENT(m_pipeline), &currentState, nullptr, GST_CLOCK_TIME_NONE);
	if(currentState >= GST_STATE_READY) {
		gboolean muted = false;
		g_object_get(G_OBJECT(m_pipeline), "mute", &muted, nullptr);
		if(muted != m_muted) {
			m_muted = muted;
			emit muteChanged(muted);
		}
		if(!muted) {
			gdouble volume = -1.0;
			g_object_get(G_OBJECT(m_pipeline), "volume", &volume, nullptr);
			if(volume != m_volume) {
				m_volume = volume;
				emit volumeChanged(qPow(volume / MAX_VOLUME, .33333) * 100.);
			}
		}
	}

	GstQuery *rateQuery = gst_query_new_segment(GST_FORMAT_DEFAULT);
	if(gst_element_query(GST_ELEMENT(m_pipeline), rateQuery)) {
		gst_query_parse_segment(rateQuery, &m_playbackRate, nullptr, nullptr, nullptr);
		emit speedChanged(m_playbackRate);
	}
	gst_query_unref(rateQuery);

	GstMessage *msg;
	while(m_pipeline && m_pipelineBus && (msg = gst_bus_pop(m_pipelineBus))) {
		GstObject *src = GST_MESSAGE_SRC(msg);

		// we are only interested in error messages or messages directed to the playbin
		if(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR && src != GST_OBJECT(m_pipeline)) {
			gst_message_unref(msg);
			continue;
		}

#if defined(VERBOSE) || !defined(NDEBUG)
		GStreamer::inspectMessage(msg);
#endif

		switch(GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_STATE_CHANGED: {
			GstState old, current, target;
			gst_message_parse_state_changed(msg, &old, &current, &target);

			if(current == GST_STATE_PAUSED)
				emit stateChanged(VideoPlayer::Paused);
			else if(current == GST_STATE_PLAYING)
				emit stateChanged(VideoPlayer::Playing);
			else if(current == GST_STATE_READY)
				emit stateChanged(VideoPlayer::Ready);

			if(old == GST_STATE_READY) {
				updateTextData();
				updateAudioData();
				updateVideoData();
			}
			break;
		}

		case GST_MESSAGE_ERROR: {
			gchar *debug = nullptr;
			GError *error = nullptr;
			gst_message_parse_error(msg, &error, &debug);
			emit errorOccured(QString::fromUtf8(error->message));
			//setPlayerErrorState(QString(debug));
			g_error_free(error);
			g_free(debug);
			break;
		}

		default:
			break;
		}

		gst_message_unref(msg);
	}
}

void
GStreamerBackend::updateTextData()
{
	QStringList textStreams;

	gint n;
	g_object_get(m_pipeline, "n-text", &n, nullptr);
	for(gint i = 0; i < n; i++) {
		QString textStreamName;
		GstTagList *tags = nullptr;
		gchar *str;
		g_signal_emit_by_name(m_pipeline, "get-text-tags", i, &tags);
		if(tags) {
			textStreamName = i18n("Text Stream #%1", i);
			if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_NAME, &str)) {
				textStreamName += QStringLiteral(": ") + (const char *)str;
				g_free(str);
			} else if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
				textStreamName += QStringLiteral(": ") + LanguageCode::nameFromIso(str);
				g_free(str);
			}
			if(gst_tag_list_get_string(tags, GST_TAG_SUBTITLE_CODEC, &str)) {
				textStreamName += QStringLiteral(" [") + str + QStringLiteral("]");
				g_free(str);
			}
			gst_tag_list_free(tags);

			textStreams << textStreamName;
		}
	}

	emit textStreamsChanged(textStreams);
}

void
GStreamerBackend::updateAudioData()
{
	QStringList audioStreams;
	gint activeAudioStream;

	gint n;
	g_object_get(m_pipeline, "n-audio", &n, nullptr);
	for(gint i = 0; i < n; i++) {
		QString audioStreamName;
		GstTagList *tags = nullptr;
		guint rate;
		gchar *str;
		g_signal_emit_by_name(m_pipeline, "get-audio-tags", i, &tags);
		if(tags) {
			audioStreamName = i18n("Audio Stream #%1", i);
			if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
				audioStreamName += ": " + LanguageCode::nameFromIso(str);
				g_free(str);
			}
			if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &str)) {
				audioStreamName += QStringLiteral(" [") + str + "]";
				g_free(str);
			}
			if(gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &rate)) {
				audioStreamName += " " + QString::number(rate / 1000) + "kbps";
			}
			gst_tag_list_free(tags);

			audioStreams << audioStreamName;
		}
	}

	g_object_get(m_pipeline, "current-audio", &activeAudioStream, nullptr);

	emit audioStreamsChanged(audioStreams, activeAudioStream);
}

void
GStreamerBackend::updateVideoData()
{
	GstElement *videosink;
	g_object_get(m_pipeline, "video-sink", &videosink, nullptr);

	GstPad *videopad = gst_element_get_static_pad(GST_ELEMENT(videosink), "sink");
	if(!videopad)
		return;

	GstCaps *caps = gst_pad_get_current_caps(videopad);
	if(!caps)
		return;

	const GstStructure *capsStruct = gst_caps_get_structure(caps, 0);
	if(!capsStruct)
		return;

	gint width = 0, height = 0;
	gst_structure_get_int(capsStruct, "width", &width);
	gst_structure_get_int(capsStruct, "height", &height);

	double dar = 0.0;
	const GValue *par;
	if((par = gst_structure_get_value(capsStruct, "pixel-aspect-ratio"))) {
		dar = (double)gst_value_get_fraction_numerator(par) / gst_value_get_fraction_denominator(par);
		dar = dar * width / height;
	}

	emit resolutionChanged(width, height, dar);

	const GValue *fps;
	if((fps = gst_structure_get_value(capsStruct, "framerate"))) {
		int num = gst_value_get_fraction_numerator(fps);
		int den = gst_value_get_fraction_denominator(fps);
		emit fpsChanged((double)num / den);
		m_frameDuration = gint64(den) * GST_SECOND / gint64(num);
	}

	gst_caps_unref(caps);
	gst_object_unref(videopad);
}

bool
GStreamerBackend::eventFilter(QObject *obj, QEvent *event)
{
	bool res = QObject::eventFilter(obj, event);

	if(m_pipeline && GST_IS_VIDEO_OVERLAY(m_pipeline) && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
		QResizeEvent *evt = static_cast<QResizeEvent *>(event);
		if(evt->size().width() > 0 && evt->size().height() > 0)
			gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_pipeline), 0, 0, evt->size().width(), evt->size().height());
		else
			gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_pipeline), 0, 0, -1, -1);
		gst_video_overlay_expose(GST_VIDEO_OVERLAY(m_pipeline));
	}

	return res;
}

bool
GStreamerBackend::reconfigure()
{
	if(!m_pipeline || !GST_IS_PIPELINE(m_pipeline))
		return false;

	GstElement *oldsink = nullptr, *newsink;

	// replace video sink
	g_object_get(G_OBJECT(m_pipeline), "video-sink", &oldsink, nullptr);
	if(!oldsink || !GST_IS_ELEMENT(oldsink))
		return false;
	newsink = createVideoSink();
	g_object_set(G_OBJECT(m_pipeline), "video-sink", newsink, nullptr);
	g_object_unref(oldsink);

	// replace audio sink
	g_object_get(G_OBJECT(m_pipeline), "audio-sink", &oldsink, nullptr);
	if(!oldsink || !GST_IS_ELEMENT(oldsink))
		return false;
	newsink = createAudioSink();
	g_object_set(G_OBJECT(m_pipeline), "audio-sink", newsink, nullptr);
	g_object_unref(oldsink);

	// current position
	gint64 time = 0;
	gst_element_query_position(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &time);

	GstState state = GST_STATE_VOID_PENDING;
	gst_element_get_state(GST_ELEMENT(m_pipeline), &state, nullptr, GST_CLOCK_TIME_NONE);
	GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_NULL, GST_CLOCK_TIME_NONE);
	if(state == GST_STATE_PLAYING || state == GST_STATE_PAUSED) {
		GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PLAYING, GST_CLOCK_TIME_NONE);
		onPlaybinTimerTimeout();
		seek((double)time / GST_SECOND);
		if(state == GST_STATE_PAUSED)
			GStreamer::setElementState(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED, GST_CLOCK_TIME_NONE);
	}

	return true;
}
