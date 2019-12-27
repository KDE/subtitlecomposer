#ifndef SUBVIEWER1OUTPUTFORMAT_H
#define SUBVIEWER1OUTPUTFORMAT_H

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
class SubViewer1OutputFormat : public OutputFormat
{
	friend class FormatManager;

protected:
	QString dumpSubtitles(const Subtitle &subtitle, bool primary) const override
	{
		QString ret(QStringLiteral("[TITLE]\n\n[AUTHOR]\n\n[SOURCE]\n\n[PRG]\n\n[FILEPATH]\n\n[DELAY]\n0\n[CD TRACK]\n0\n[BEGIN]\n" "******** START SCRIPT ********\n"));

		for(SubtitleIterator it(subtitle); it.current(); ++it) {
			const SubtitleLine *line = it.current();

			Time showTime = line->showTime();
			ret += QString::asprintf("[%02d:%02d:%02d]\n", showTime.hours(), showTime.minutes(), showTime.seconds());

			const SString &text = primary ? line->primaryText() : line->secondaryText();
			ret += text.string().replace('\n', '|');

			Time hideTime = line->hideTime();
			ret += QString::asprintf("\n[%02d:%02d:%02d]\n\n", hideTime.hours(), hideTime.minutes(), hideTime.seconds());
		}
		ret += "[END]\n" "******** END SCRIPT ********\n";

		return ret;
	}

	SubViewer1OutputFormat() :
		OutputFormat(QStringLiteral("SubViewer 1.0"), QStringList(QStringLiteral("sub")))
	{}
};
}

#endif
