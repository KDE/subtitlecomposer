/*
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "plugin-config.h"

#include "pocketsphinxplugin.h"
#include "pocketsphinxconfigwidget.h"
#include "pocketsphinxconfig.h"
#include "videoplayer/waveformat.h"

#include <pocketsphinx.h>


using namespace SubtitleComposer;

PocketSphinxPlugin::PocketSphinxPlugin()
	: SpeechPlugin(),
	  m_psConfig(nullptr),
	  m_psDecoder(nullptr)
{
}

/*virtual*/ const QString &
PocketSphinxPlugin::name()
{
	static const QString name(QStringLiteral("PocketSphinx"));
	return name;
}

const WaveFormat &
PocketSphinxPlugin::waveFormat() const
{
	static const WaveFormat wf(16000, 1, 16, true);
	return wf;
}

/*virtual*/ bool
PocketSphinxPlugin::init()
{
	m_psConfig = cmd_ln_init(nullptr, ps_args(), true,
				 "-hmm", QUrl(PocketSphinxConfig::acousticModelPath()).toLocalFile().toUtf8().constData(),
				 "-lm", QUrl(PocketSphinxConfig::trigramModelFile()).toLocalFile().toUtf8().constData(),
				 "-dict", QUrl(PocketSphinxConfig::lexiconFile()).toLocalFile().toUtf8().constData(),
//				 "-frate", "100",
				 // Num of silence frames to keep after speech to silence transition. (pocketsphinx default: 50)
				 "-vad_postspeech", QByteArray::number(PocketSphinxConfig::vadPostSpeech()).constData(),
				 // Num of speech frames to keep before silence to speech transition. (pocketsphinx default: 20)
				 "-vad_prespeech", QByteArray::number(PocketSphinxConfig::vadPreSpeech()).constData(),
				 // Num of speech frames to trigger VAD from silence to speech. (pocketsphinx default: 10)
				 "-vad_startspeech", QByteArray::number(PocketSphinxConfig::vadStartSpeech()).constData(),
				 // Threshold for decision between noise and silence frames.
				 // Log-ratio between signal level and noise level. (pocketsphinx default: 2.0)
				 "-vad_threshold", QByteArray::number(PocketSphinxConfig::vadTreshold()).constData(),
				 nullptr);
	if(m_psConfig == nullptr) {
		qWarning() << "Failed to create PocketSphinx config object";
		return false;
	}

	m_psDecoder = ps_init(m_psConfig);
	if(m_psDecoder == nullptr) {
		qWarning() << "Failed to create PocketSphinx recognizer";
		return false;
	}

	m_psFrameRate = cmd_ln_int32_r(m_psConfig, "-frate");

	m_lineText.clear();
	m_lineIn = m_lineOut = 0;

	m_utteranceStarted = false;
	m_speechStarted = false;

	return true;
}

/*virtual*/ void
PocketSphinxPlugin::cleanup()
{
	if(m_psDecoder != nullptr) {
		ps_free(m_psDecoder);
		m_psDecoder = nullptr;
	}
	if(m_psConfig != nullptr) {
		cmd_ln_free_r(m_psConfig);
		m_psConfig = nullptr;
	}
}

void
PocketSphinxPlugin::processUtterance()
{
	qint32 score;
	char const *hyp = ps_get_hyp(m_psDecoder, &score);
	if(!hyp || !*hyp)
		return;

#ifdef HAS_NEW_PS_SEG_ITER
	ps_seg_t *iter = ps_seg_iter(m_psDecoder);
#else
	ps_seg_t *iter = ps_seg_iter(m_psDecoder, nullptr);
#endif
	while(iter != nullptr) {
		const char *word = ps_seg_word(iter);
		int wordIn, wordOut;
		ps_seg_frames(iter, &wordIn, &wordOut);
		if(*word == '<' || *word == '[') {
			// "<s>" "</s>" "<sil>" "[SPEECH]"
			if(!m_lineText.isEmpty()) {
				emit textRecognized(m_lineText,
									  double(m_lineIn) * 1000. / double(m_psFrameRate),
									  double(m_lineOut) * 1000. / double(m_psFrameRate));
				m_lineText.clear();
			}
		} else {
			QString sWord = QString::fromLatin1(word);

			// strip number suffix
			const char *pos = word;
			while(*pos && *pos != '(')
				pos++;
			sWord.truncate(pos - word);

			if(m_lineText.isEmpty()) {
				m_lineText = sWord;
				m_lineIn = wordIn;
			} else {
				m_lineText += ' ';
				m_lineText += sWord;
			}
			m_lineOut = wordOut;
		}

		iter = ps_seg_next(iter);
	}
	if(!m_lineText.isEmpty()) {
		emit textRecognized(m_lineText,
							  double(m_lineIn) * 1000. / double(m_psFrameRate),
							  double(m_lineOut) * 1000. / double(m_psFrameRate));
		m_lineText.clear();
	}
}

/*virtual*/ void
PocketSphinxPlugin::processSamples(const void *sampleData, qint32 sampleCount)
{
	if(!m_utteranceStarted) {
		ps_start_utt(m_psDecoder);
		m_utteranceStarted = true;
		m_speechStarted = false;
	}

	ps_process_raw(m_psDecoder, reinterpret_cast<const int16 *>(sampleData), sampleCount, false, false);

	if(ps_get_in_speech(m_psDecoder)) {
		m_speechStarted = true;
	} else {
		if(m_utteranceStarted && m_speechStarted) {
			ps_end_utt(m_psDecoder);
			processUtterance();
			m_speechStarted = false;
			m_utteranceStarted = false;
		}
	}
}

/*virtual*/ void
PocketSphinxPlugin::processComplete()
{
	if(m_psDecoder) {
		if(m_utteranceStarted)
			ps_end_utt(m_psDecoder);
		processUtterance();
	}
}

QWidget *
PocketSphinxPlugin::newConfigWidget(QWidget *parent)
{
	return new PocketSphinxConfigWidget(parent);
}

KCoreConfigSkeleton *
PocketSphinxPlugin::config() const
{
	return PocketSphinxConfig::self();
}
