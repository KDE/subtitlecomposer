/*
    SPDX-FileCopyrightText: 2015 Martin Steghöfer <martin@steghoefer.eu>
    SPDX-FileCopyrightText: 2015-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "config.h"

#include "application.h"
#include "appglobal.h"
#include "configs/configdialog.h"
#include "configs/generalconfigwidget.h"
#include "configs/errorsconfigwidget.h"
#include "configs/waveformconfigwidget.h"
#include "configs/playerconfigwidget.h"
#include "videoplayer/videoplayer.h"
#include "speechprocessor/speechprocessor.h"
#include "speechprocessor/speechplugin.h"

#include <KConfigDialog>
#include <KLocalizedString>

#include <sonnet/configwidget.h>

using namespace SubtitleComposer;

ConfigDialog::ConfigDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config) :
	KConfigDialog(parent, name, config),
	m_hasWidgetChanged(false)
{
	KPageWidgetItem *item;

	// General page
	item = addPage(new GeneralConfigWidget(this), i18nc("@title General settings", "General"));
	item->setHeader(i18n("General Settings"));
	item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));

	// Error Check page
	item = addPage(new ErrorsConfigWidget(this), i18nc("@title Error check settings", "Error Check"));
	item->setHeader(i18n("Error Check Settings"));
	item->setIcon(QIcon::fromTheme(QStringLiteral("games-endturn")));

	// Spelling page
	m_sonnetConfigWidget = new Sonnet::ConfigWidget(this);
	connect(m_sonnetConfigWidget, &Sonnet::ConfigWidget::configChanged, this, &ConfigDialog::widgetChanged);
	item = addPage(m_sonnetConfigWidget, i18nc("@title Spelling settings", "Spelling"));
	item->setHeader(i18n("Spelling Settings"));
	item->setIcon(QIcon::fromTheme(QStringLiteral("tools-check-spelling")));

	// Waveform page
	item = addPage(new WaveformConfigWidget(this), i18nc("@title Waveform settings", "Waveform"));
	item->setHeader(i18n("Waveform settings"));
	item->setIcon(QIcon::fromTheme(QStringLiteral("waveform")));

	// VideoPlayer page
	item = addPage(new PlayerConfigWidget(this), i18nc("@title Video player settings", "Video Player"));
	item->setHeader(i18n("Video Player Settings"));
	item->setIcon(QIcon::fromTheme(QStringLiteral("mediaplayer")));

	{ // SpeechProcessor plugin pages
		const SpeechProcessor *speechProcessor = app()->speechProcessor();
		const QMap<QString, SpeechPlugin *> plugins = speechProcessor->plugins();
		for(auto it = plugins.cbegin(); it != plugins.cend(); ++it) {
			if(QWidget *configWidget = it.value()->newConfigWidget(nullptr)) {
				item = addPage(configWidget, it.value()->config(), it.key());
				item->setHeader(i18nc("@title Speech recognition backend settings", "%1 backend settings", it.key()));
				item->setIcon(QIcon::fromTheme(it.key().toLower()));
			}
		}
	}

	resize(800, 600);
}

void
ConfigDialog::widgetChanged()
{
	m_hasWidgetChanged = true;
	updateButtons();
}

void
ConfigDialog::updateSettings()
{
	m_sonnetConfigWidget->save();
	m_hasWidgetChanged = false;
	KConfigDialog::updateSettings();
	settingsChangedSlot();
}

bool
ConfigDialog::hasChanged()
{
	return m_hasWidgetChanged || KConfigDialog::hasChanged();
}

