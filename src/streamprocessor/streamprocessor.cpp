/*
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "streamprocessor.h"
#include "helpers/languagecode.h"

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QPixmap>
#include <QImage>
#include <QRegularExpression>

#include <cinttypes>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

using namespace SubtitleComposer;

StreamProcessor::StreamProcessor(QObject *parent)
	: QThread(parent),
	  m_opened(false),
	  m_audioReady(false),
	  m_imageReady(false),
	  m_textReady(false),
	  m_avFormat(nullptr),
	  m_avStream(nullptr),
	  m_codecCtx(nullptr),
	  m_swResample(nullptr)
{
}

StreamProcessor::~StreamProcessor()
{
	close();
}

bool
StreamProcessor::open(const QString &filename)
{
	if(m_opened)
		close();

	m_filename = filename;
	m_audioStreamIndex = -1;
	m_imageStreamIndex = -1;
	m_textStreamIndex = -1;
	m_streamLen = m_streamPos = 0;

#if defined(VERBOSE) || !defined(NDEBUG)
	av_log_set_level(AV_LOG_VERBOSE);
#else
	av_log_set_level(AV_LOG_INFO);
#endif

	int ret;
	char errorText[1024];
	if((ret = avformat_open_input(&m_avFormat, filename.toUtf8().constData(), NULL, NULL)) < 0) {
		av_strerror(ret, errorText, sizeof(errorText));
		qWarning() << "Cannot open input file:" << errorText;
		return false;
	}
	if((ret = avformat_find_stream_info(m_avFormat, NULL)) < 0) {
		av_strerror(ret, errorText, sizeof(errorText));
		qWarning() << "Cannot find stream information:" << errorText;
		return false;
	}

#if defined(VERBOSE) || !defined(NDEBUG)
	av_dump_format(m_avFormat, 0, filename.toUtf8().constData(), 0);
#endif

	m_opened = true;

    return true;
}

void
StreamProcessor::close()
{
	if(isRunning()) {
		requestInterruption();
		wait();
	}

	if(m_swResample)
		swr_free(&m_swResample);
	if(m_codecCtx)
		avcodec_free_context(&m_codecCtx);
	if(m_avFormat)
		avformat_close_input(&m_avFormat);

	m_opened = false;
	m_audioReady = false;
	m_imageReady = false;
	m_textReady = false;

	QApplication::processEvents();
}

static inline bool
isImageSubtitle(int codecId)
{
	const AVCodecDescriptor *desc = avcodec_descriptor_get(static_cast<AVCodecID>(codecId));

	if(!desc)
		return false;

	return desc->props & AV_CODEC_PROP_BITMAP_SUB;
}

QStringList
StreamProcessor::listAudio()
{
	QStringList streamList;

	for(unsigned int i = 0; i < m_avFormat->nb_streams; i++) {
		AVStream *stream = m_avFormat->streams[i];
		if(stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
			continue;

		AVDictionaryEntry *lang = av_dict_get(stream->metadata, "lang", nullptr, AV_DICT_IGNORE_SUFFIX);
		streamList.append(QString("#%1 audio - %2 (%3)").arg(
					QString::number(stream->id),
					lang ? LanguageCode::nameFromIso(lang->value) : QStringLiteral("Unknown"),
					lang ? QString(lang->value) : QStringLiteral("--")));
	}

	return streamList;
}

QStringList
StreamProcessor::listText()
{
	QStringList streamList;

	for(unsigned int i = 0; i < m_avFormat->nb_streams; i++) {
		AVStream *stream = m_avFormat->streams[i];
		if(stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE || isImageSubtitle(stream->codecpar->codec_id))
			continue;

		AVDictionaryEntry *lang = av_dict_get(stream->metadata, "lang", nullptr, AV_DICT_IGNORE_SUFFIX);
		streamList.append(QString("#%1 text - %2 (%3)").arg(
					QString::number(stream->id),
					lang ? LanguageCode::nameFromIso(lang->value) : QStringLiteral("Unknown"),
					lang ? QString(lang->value) : QStringLiteral("--")));
	}

	return streamList;
}

QStringList
StreamProcessor::listImage()
{
	QStringList streamList;

	for(unsigned int i = 0; i < m_avFormat->nb_streams; i++) {
		AVStream *stream = m_avFormat->streams[i];
		if(stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE || !isImageSubtitle(stream->codecpar->codec_id))
			continue;

		AVDictionaryEntry *lang = av_dict_get(stream->metadata, "lang", nullptr, AV_DICT_IGNORE_SUFFIX);
		streamList.append(QString("#%1 image - %2 (%3) - %4x%5").arg(
					QString::number(stream->id),
					lang ? LanguageCode::nameFromIso(lang->value) : QStringLiteral("Unknown"),
					lang ? QString(lang->value) : QStringLiteral("--"),
					QString::number(stream->codecpar->width),
					QString::number(stream->codecpar->height)));
	}

	return streamList;
}

int
StreamProcessor::findStream(int streamType, int streamIndex, bool imageSub)
{
	for(unsigned int i = 0; i < m_avFormat->nb_streams; i++) {
		m_avStream = m_avFormat->streams[i];
		if(m_avStream->codecpar->codec_type != streamType
		|| (streamType == AVMEDIA_TYPE_SUBTITLE && imageSub != isImageSubtitle(m_avStream->codecpar->codec_id)))
			continue;

		if(streamIndex--)
			continue;

		int ret;
		char errorText[1024];

		const AVCodec *dec = avcodec_find_decoder(m_avStream->codecpar->codec_id);
		if(!dec) {
			qWarning() << "Failed to find decoder for stream" << i;
			return false;
		}

		m_codecCtx = avcodec_alloc_context3(dec);
		if(!m_codecCtx) {
			qWarning() << "Failed to allocate the decoder context for stream" << i;
			continue;
		}
		ret = avcodec_parameters_to_context(m_codecCtx, m_avStream->codecpar);
		if(ret < 0) {
			av_strerror(ret, errorText, sizeof(errorText));
			qWarning() << "Failed to copy decoder parameters to input decoder context for stream" << i << errorText;
			avcodec_free_context(&m_codecCtx);
			continue;
		}
		if(m_codecCtx->codec_type != streamType) {
			avcodec_free_context(&m_codecCtx);
			continue;
		}
		ret = avcodec_open2(m_codecCtx, dec, nullptr);
		if(ret < 0) {
			av_strerror(ret, errorText, sizeof(errorText));
			qWarning() << "Failed to open decoder for stream" << i << errorText;
			avcodec_free_context(&m_codecCtx);
			continue;
		}

		return i;
	}

	return -1;
}

bool
StreamProcessor::initAudio(int streamIndex, const WaveFormat &waveFormat)
{
	if(!m_opened)
		return false;

	m_audioStreamIndex = streamIndex;
	m_audioStreamFormat = waveFormat;
	m_imageReady = false;
	m_textReady = false;

	m_audioStreamCurrent = findStream(AVMEDIA_TYPE_AUDIO, streamIndex, false);
	m_audioReady = m_audioStreamCurrent != -1;

	if(!m_audioReady)
		return false;

	// update stream format so zero values are set to input stream format values
	if(m_audioStreamFormat.sampleRate() == 0)
		m_audioStreamFormat.setSampleRate(m_codecCtx->sample_rate);
	if(m_audioStreamFormat.bitsPerSample() == 0)
		m_audioStreamFormat.setBitsPerSample(m_codecCtx->bits_per_raw_sample);

	// figure sample format and update stream format
	const int bps = m_audioStreamFormat.bitsPerSample();
	if(bps == 8) {
		m_audioSampleFormat = AV_SAMPLE_FMT_U8;
		m_audioStreamFormat.setInteger(true);
	} else if(bps == 16) {
		m_audioSampleFormat = AV_SAMPLE_FMT_S16;
		m_audioStreamFormat.setInteger(true);
	} else if(bps == 32) {
		m_audioSampleFormat = m_audioStreamFormat.isInteger() ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_FLT;
	} else if(bps == 64) {
		m_audioSampleFormat = AV_SAMPLE_FMT_DBL;
		m_audioStreamFormat.setInteger(false);
	} else {
		qWarning() << "Invalid wave format requested:" << bps << "bits per sample";
		emit streamError(AVERROR_BUG, QStringLiteral("Invalid wave format requested"), QString::number(bps) + QStringLiteral(" bits per sample"));
		return false;
	}

	// figure channel layout or update stream format
	if(!m_codecCtx->channel_layout)
		m_codecCtx->channel_layout = av_get_default_channel_layout(m_codecCtx->channels);;

	if(m_audioStreamFormat.channels() == 0) {
		m_audioStreamFormat.setChannels(m_codecCtx->channels);
		m_audioChannelLayout = m_codecCtx->channel_layout;
	} else {
		m_audioChannelLayout = av_get_default_channel_layout(m_audioStreamFormat.channels());
	}

	// setup resampler if needed
	const bool convChannels = m_codecCtx->channel_layout != m_audioChannelLayout;
	const bool convSampleRate = m_codecCtx->sample_rate != m_audioStreamFormat.sampleRate();
	const bool convSampleFormat = m_codecCtx->sample_fmt != m_audioSampleFormat;
	if(convChannels || convSampleRate || convSampleFormat) {
		m_swResample = swr_alloc_set_opts(nullptr,
			m_audioChannelLayout, static_cast<AVSampleFormat>(m_audioSampleFormat), m_audioStreamFormat.sampleRate(),
			m_codecCtx->channel_layout, m_codecCtx->sample_fmt, m_codecCtx->sample_rate,
			0, nullptr);
		// NOTE: swr_convert_frame() will call swr_init() and swr_config_frame() which is better as it seems m_codecCtx can
		// end up with different config that what is actually in the stream
	}

	return true;
}

bool
StreamProcessor::initImage(int streamIndex)
{
	if(!m_opened)
		return false;

	m_imageStreamIndex = streamIndex;
	m_audioReady = false;
	m_textReady = false;

	m_imageStreamCurrent = findStream(AVMEDIA_TYPE_SUBTITLE, streamIndex, true);
	m_imageReady = m_imageStreamCurrent != -1;

	if(!m_imageReady)
		return false;

	return true;
}

bool
StreamProcessor::initText(int streamIndex)
{
	if(!m_opened)
		return false;

	m_textStreamIndex = streamIndex;
	m_audioReady = false;
	m_imageReady = false;

	m_textStreamCurrent = findStream(AVMEDIA_TYPE_SUBTITLE, streamIndex, false);
	m_textReady = m_textStreamCurrent != -1;

	if(!m_textReady)
		return false;

	return true;
}

bool
StreamProcessor::start()
{
	if(!m_opened || !(m_audioReady || m_imageReady || m_textReady))
		return false;

	QThread::start(LowPriority);

	return true;
}

void
StreamProcessor::processAudio()
{
	int ret;
	char errorText[1024];
	AVPacket *pkt = av_packet_alloc();
	Q_ASSERT(pkt != nullptr);
	AVFrame *frame = av_frame_alloc();
	Q_ASSERT(frame != nullptr);
	AVFrame *frameResampled = nullptr;

	if(m_swResample) {
		frameResampled = av_frame_alloc();
		Q_ASSERT(frameResampled != nullptr);
		frameResampled->channel_layout = m_audioChannelLayout;
		frameResampled->sample_rate = m_audioStreamFormat.sampleRate();
		frameResampled->format = m_audioSampleFormat;
	}

	const int64_t streamDuration = m_avStream->duration * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;
	const int64_t containerDuration = m_avFormat->duration * 1000 / AV_TIME_BASE;
	m_streamLen = streamDuration > containerDuration ? streamDuration : containerDuration;

	int64_t timeFrameStart = 0;
	int64_t timeFrameDuration = 0;
	int64_t timeFrameEnd = 0;
	int64_t timeResampleDelay = 0;

	bool conversionComplete = false;

	while(!conversionComplete && !isInterruptionRequested()) {
		ret = av_read_frame(m_avFormat, pkt);
		bool drainDecoder = ret == AVERROR_EOF;
		if(ret < 0 && !drainDecoder) {
			av_strerror(ret, errorText, sizeof(errorText));
			qWarning() << "Error reading packet" << errorText;
			emit streamError(ret, QStringLiteral("Error reading packet"), QString::fromUtf8(errorText));
			break;
		}

		if(pkt->stream_index == m_audioStreamCurrent || drainDecoder) {
			ret = avcodec_send_packet(m_codecCtx, pkt);
			if(ret < 0) {
				if(ret != AVERROR(EAGAIN)) {
					av_strerror(ret, errorText, sizeof(errorText));
					qWarning() << "Error decoding packet" << errorText;
					emit streamError(ret, QStringLiteral("Error decoding packet"), QString::fromUtf8(errorText));
				}
				break;
			}
			while(!conversionComplete && !isInterruptionRequested()) {
				ret = avcodec_receive_frame(m_codecCtx, frame);
				bool drainResampler = ret == AVERROR_EOF;
				if(ret < 0 && !drainResampler) {
					if(ret != AVERROR(EAGAIN)) {
						av_strerror(ret, errorText, sizeof(errorText));
						qWarning() << "Error decoding audio frame" << errorText;
						emit streamError(ret, QStringLiteral("Error decoding audio frame"), QString::fromUtf8(errorText));
					}
					break;
				}
				if(ret == 0) {
					if(frame->best_effort_timestamp)
						timeFrameStart = frame->best_effort_timestamp * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;
				}

				bool drainSampleBuffer = false;
				do {
					size_t frameSize;
					if(m_swResample) {
						ret = swr_convert_frame(m_swResample, frameResampled, drainSampleBuffer || drainResampler ? nullptr : frame);
						if(ret < 0) {
							av_strerror(ret, errorText, sizeof(errorText));
							qWarning() << "Error resampling audio frame" << errorText;
							emit streamError(ret, QStringLiteral("Error resampling audio frame"), QString::fromUtf8(errorText));
							break;
						}
						timeResampleDelay = -swr_get_delay(m_swResample, 1000);
						frameSize = frameResampled->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frameResampled->format));
						timeFrameDuration = frameResampled->nb_samples * 1000 / frameResampled->sample_rate;
					} else {
						frameSize = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
						if(frame->pkt_duration)
							timeFrameDuration = frame->pkt_duration * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;
					}
					timeFrameEnd = timeFrameStart + timeFrameDuration;

					if(drainResampler && (!frameResampled || frameResampled->nb_samples == 0)) {
						conversionComplete = true;
						break;
					}

					if(!drainResampler) {
						m_streamPos = timeFrameEnd;
						emit streamProgress(m_streamPos, m_streamLen);
					}

					if(m_swResample) {
						Q_ASSERT(frameResampled != nullptr);
						emit audioDataAvailable(frameResampled->data[0], qint32(frameSize * frameResampled->channels),
							&m_audioStreamFormat, qint64(timeFrameStart + timeResampleDelay), qint64(timeFrameDuration));

						drainSampleBuffer = swr_get_out_samples(m_swResample, 0) > 1000;
					} else {
						emit audioDataAvailable(frame->data[0], qint32(frameSize * frame->channels),
							&m_audioStreamFormat, qint64(timeFrameStart), qint64(timeFrameDuration));
					}
				} while(!conversionComplete && !isInterruptionRequested() && drainSampleBuffer);
			}
		}

		if(drainDecoder)
			break;

		av_packet_unref(pkt);
	}

	av_frame_free(&frame);
	if(frameResampled)
		av_frame_free(&frameResampled);

	av_packet_free(&pkt);

	emit streamFinished();
	QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}

void
StreamProcessor::processText()
{
	int ret;
	char errorText[1024];
	AVPacket *pkt = av_packet_alloc();
	Q_ASSERT(pkt != nullptr);

	const quint64 streamDuration = m_avStream->duration * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;
	const quint64 containerDuration = m_avFormat->duration * 1000 / AV_TIME_BASE;
	m_streamLen = streamDuration > containerDuration ? streamDuration : containerDuration;

	AVSubtitle subtitle;
	QString text;
	quint64 timeStart = 0;
	quint64 timeEnd = 0;
	QImage image;
	bool pixmapIsValid = false;

	const int streamIndex = m_textReady ? m_textStreamCurrent : m_imageStreamCurrent;

	while(av_read_frame(m_avFormat, pkt) >= 0) {
		if(pkt->stream_index == streamIndex) {
			int got_sub = 0;
			ret = avcodec_decode_subtitle2(m_codecCtx, &subtitle, &got_sub, pkt);
			if(ret < 0) {
				av_strerror(ret, errorText, sizeof(errorText));
				qWarning() << "Failed to decode subtitle:" << errorText;
				if(got_sub)
					avsubtitle_free(&subtitle);
				continue;
			}
			if(!got_sub)
				continue;

			const quint64 timeFrameStart = pkt->pts * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;
			const quint64 timeFrameEnd = timeFrameStart + pkt->duration * 1000 * m_avStream->time_base.num / m_avStream->time_base.den;

			if(timeFrameStart < timeEnd) // correct overlapping titles
				timeEnd = timeFrameStart - 10;
			if(!text.isEmpty()) {
				emit textDataAvailable(text.trimmed(), timeStart, timeEnd - timeStart);
				text.clear();
			}
			if(pixmapIsValid) {
				emit imageDataAvailable(image, timeStart, timeEnd - timeStart);
				pixmapIsValid = false;
			}

			timeStart = timeFrameStart + subtitle.start_display_time;
			if(subtitle.end_display_time) // end_display_time is relative to pkt.pts (timeFrameStart)
				timeEnd = timeFrameStart + subtitle.end_display_time;
			else
				timeEnd = timeFrameEnd;

			for(unsigned int i = 0; i < subtitle.num_rects; i++) {
				const AVSubtitleRect *sub = subtitle.rects[i];
				switch(sub->type) {
				case SUBTITLE_ASS: {
#if 1
					const char *assText = sub->ass;
					// FIXME: did ass format change with ffmpeg6? can't find any references
					const int textLocation = strncmp("Dialogue", assText, 8) ? 8 : 9;
					// Dialogue: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
					for(int c = textLocation; c && *assText; assText++) {
						if(*assText == ',')
							c--;
					}
#else
					const char *assText = "This is {\\b100}bold{\\b0} {\\b1\\i1}bolditalic{\\b0\\i0}\\N{\\u1}underline{\\u0} {\\s1}stricken{\\s0}\\n"
										  "{\\c&H0000ff&}red {\\c&H00ff00&}green {\\c&Hff0000&}blue{\\r}\\n"
										  "Another {\\b100}bold\\h{\\i1}bolditalic{\\b0\\i0} some{\\anidfsd} unsupported tag";
#endif
					QString assChunk = QString::fromUtf8(assText);

					assChunk
							.replace(QStringLiteral("\\N"), QStringLiteral("\n"))
							.replace(QStringLiteral("\\n"), QStringLiteral("\n"))
							.replace(QStringLiteral("\\h"), QStringLiteral(" "));

#define ANY_ASS_VAL "[^}\\\\]*"
#define ANY_ASS_TAG "(?:\\\\" ANY_ASS_VAL ")*"

					// replace rich text style tags
					for(;;) {
						const static QRegularExpression reStyle(QStringLiteral("\\{" ANY_ASS_TAG "\\\\([bius])(\\d+)" ANY_ASS_TAG "\\}"), QRegularExpression::CaseInsensitiveOption);
						QRegularExpressionMatch match = reStyle.match(assChunk);
						if(!match.hasMatch())
							break;
						assChunk.insert(match.capturedEnd(0), QString("<%1%2>")
									.arg(match.captured(2) == QStringLiteral("0") ? QStringLiteral("/") : QString(), match.captured(1)));
						assChunk.remove(match.capturedStart(1), match.capturedLength(2) + 1);
					}

					// replace text color tags
					const static QRegularExpression reStyleColor(QStringLiteral("\\{" ANY_ASS_TAG "\\\\c&H([a-z0-9]{2})([a-z0-9]{2})([a-z0-9]{2})&" ANY_ASS_TAG "\\}"), QRegularExpression::CaseInsensitiveOption);
					assChunk.replace(reStyleColor, QStringLiteral("<font color=\"#\\3\\2\\1\">"));

					// replace reset tags
					const static QRegularExpression reStyleReset(QStringLiteral("\\{" ANY_ASS_TAG "\\\\r" ANY_ASS_VAL ANY_ASS_TAG "\\}"), QRegularExpression::CaseInsensitiveOption);
					assChunk.replace(reStyleReset, QStringLiteral("</font></b></i></u></s>"));

					// remove unsupported/empty tags
					const static QRegularExpression reUnsupported(QStringLiteral("\\{" ANY_ASS_TAG "\\}"));
					assChunk.remove(reUnsupported);

					// append chunk
					if(!text.isEmpty())
						text.append(QChar('\n'));
					text.append(assChunk);

					break;
				}

				case SUBTITLE_BITMAP: {
					const uint32_t *palette = reinterpret_cast<const uint32_t *>(sub->data[1]);

					image = QImage(const_cast<const uchar *>(sub->data[0]), sub->w, sub->h, sub->linesize[0], QImage::Format_Indexed8);
					image.setColorCount(sub->nb_colors);
					for(int i = 0; i < sub->nb_colors; i++)
						image.setColor(i, static_cast<QRgb>(palette[i]));
					image.detach();

					pixmapIsValid = true;
					break;
				}

				default:
					Q_ASSERT(false);
					break;
				}
			}

			m_streamPos = timeFrameEnd;
			emit streamProgress(m_streamPos, m_streamLen);

			avsubtitle_free(&subtitle);
		}

		av_packet_unref(pkt);
	}

	av_packet_free(&pkt);

	if(!text.isEmpty())
		emit textDataAvailable(text.trimmed(), timeStart, timeEnd - timeStart);

	emit streamFinished();
	QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}

/*virtual*/ void
StreamProcessor::run()
{
	if(m_audioReady)
		processAudio();
	else if(m_imageReady || m_textReady)
		processText();
}
