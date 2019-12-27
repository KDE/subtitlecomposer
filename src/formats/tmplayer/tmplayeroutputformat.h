#ifndef TMPLAYEROUTPUTFORMAT_H
#define TMPLAYEROUTPUTFORMAT_H

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

#include "formats/outputformat.h"
#include "core/subtitleiterator.h"

namespace SubtitleComposer {
class TMPlayerOutputFormat : public OutputFormat
{
	friend class FormatManager;

protected:
	QString dumpSubtitles(const Subtitle &subtitle, bool primary) const override
	{
		QString ret;

		for(SubtitleIterator it(subtitle); it.current(); ++it) {
			const SubtitleLine *line = it.current();

			Time showTime = line->showTime();
			ret += QString::asprintf(m_timeFormat, showTime.hours(), showTime.minutes(), showTime.seconds());

			const SString &text = primary ? line->primaryText() : line->secondaryText();
			ret += text.string().replace('\n', '|');
			ret += '\n';

			// We behave like Subtitle Workshop here: to compensate for the lack of hide time
			// indication provisions in the format we add an empty line with the hide time.
			Time hideTime = line->hideTime();
			ret += QString::asprintf(m_timeFormat, hideTime.hours(), hideTime.minutes(), hideTime.seconds());

			ret += '\n';
		}
		return ret;
	}

	TMPlayerOutputFormat() :
		OutputFormat(QStringLiteral("TMPlayer"), QStringList() << QStringLiteral("sub") << QStringLiteral("txt")),
		m_timeFormat("%02d:%02d:%02d:")
	{}

	TMPlayerOutputFormat(const QString &name, const QStringList &extensions, const char *timeFormat) :
		OutputFormat(name, extensions),
		m_timeFormat(timeFormat)
	{}

	const char *m_timeFormat;
};

class TMPlayerPlusOutputFormat : public TMPlayerOutputFormat
{
	friend class FormatManager;

protected:
	TMPlayerPlusOutputFormat() :
		TMPlayerOutputFormat(QStringLiteral("TMPlayer+"), QStringList() << QStringLiteral("sub") << QStringLiteral("txt"), "%02d:%02d:%02d=")
	{}
};
}

#endif
