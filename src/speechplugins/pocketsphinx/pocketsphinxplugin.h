/*
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef POCKETSPHINXPLUGIN_H
#define POCKETSPHINXPLUGIN_H

#include "speechprocessor/speechplugin.h"

struct cmd_ln_s;
struct ps_decoder_s;

namespace SubtitleComposer {
class PocketSphinxPlugin : public SpeechPlugin
{
	Q_OBJECT

	Q_PLUGIN_METADATA(IID SpeechPlugin_iid)
	Q_INTERFACES(SubtitleComposer::SpeechPlugin)

public:
	PocketSphinxPlugin();

	QWidget * newConfigWidget(QWidget *parent) override;
	KCoreConfigSkeleton * config() const override;

private:
	const QString & name() override;

	bool init() override;
	void cleanup() override;

	void processSamples(const qint16 *sampleData, qint32 sampleCount) override;
	void processComplete() override;

	void processUtterance();

private:
	cmd_ln_s *m_psConfig;
	ps_decoder_s *m_psDecoder;
	qint32 m_psFrameRate;

	QString m_lineText;
	int m_lineIn;
	int m_lineOut;

	bool m_utteranceStarted;
	bool m_speechStarted;
};
}

#endif // POCKETSPHINXPLUGIN_H
