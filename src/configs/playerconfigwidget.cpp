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

#include "playerconfigwidget.h"

#include "videoplayer/videoplayer.h"

#include "widgets/layeredwidget.h"
#include "widgets/textoverlaywidget.h"

#include "application.h"

#include <KLocalizedString>

using namespace SubtitleComposer;

PlayerConfigWidget::PlayerConfigWidget(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);

	m_textOverlayWidget->setText(i18nc("Text for previewing the subtitles font settings", "<p>The Quick Brown Fox<br/>Jumps Over The Lazy Dog</p>"));
	m_textOverlayWidget->setFamily(SCConfig::fontFamily());
	m_textOverlayWidget->setFontSize(SCConfig::fontSize());
	m_textOverlayWidget->setPrimaryColor(SCConfig::fontColor());
	m_textOverlayWidget->setOutlineWidth(SCConfig::outlineWidth());
	m_textOverlayWidget->setOutlineColor(SCConfig::outlineColor());

	kcfg_FontFamily->setProperty("kcfg_property", QByteArray("currentText"));

	connect(kcfg_FontFamily, SIGNAL(activated(const QString &)), this, SLOT(onFamilyChanged(const QString &)));
	connect(kcfg_FontSize, SIGNAL(valueChanged(int)), this, SLOT(onSizeChanged(int)));
	connect(kcfg_FontColor, SIGNAL(activated(const QColor &)), this, SLOT(onPrimaryColorChanged(const QColor &)));
	connect(kcfg_OutlineColor, SIGNAL(activated(const QColor &)), this, SLOT(onOutlineColorChanged(const QColor &)));
	connect(kcfg_OutlineWidth, SIGNAL(valueChanged(int)), this, SLOT(onOutlineWidthChanged(int)));
}

PlayerConfigWidget::~PlayerConfigWidget()
{}

void
PlayerConfigWidget::onFamilyChanged(const QString &family)
{
	if(kcfg_FontFamily->findText(family) != -1)
		m_textOverlayWidget->setFamily(family);
}

void
PlayerConfigWidget::onSizeChanged(int size)
{
	m_textOverlayWidget->setFontSize(size);
}

void
PlayerConfigWidget::onPrimaryColorChanged(const QColor &color)
{
	m_textOverlayWidget->setPrimaryColor(color);
}

void
PlayerConfigWidget::onOutlineColorChanged(const QColor &color)
{
	m_textOverlayWidget->setOutlineColor(color);
}

void
PlayerConfigWidget::onOutlineWidthChanged(int width)
{
	m_textOverlayWidget->setOutlineWidth(width);
}
