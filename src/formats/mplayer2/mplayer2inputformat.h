#ifndef MPLAYER2INPUTFORMAT_H
#define MPLAYER2INPUTFORMAT_H

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

#include "formats/inputformat.h"

#include <QRegExp>

namespace SubtitleComposer {
class MPlayer2InputFormat : public InputFormat
{
	friend class FormatManager;

protected:
	bool parseSubtitles(Subtitle &subtitle, const QString &data) const override
	{
		unsigned readLines = 0;

		for(int offset = 0; m_lineRegExp.indexIn(data, offset) != -1; offset += m_lineRegExp.matchedLength()) {
			Time showTime(m_lineRegExp.cap(1).toInt() * 100);
			Time hideTime(m_lineRegExp.cap(2).toInt() * 100);
			QString text(m_lineRegExp.cap(3).replace('|', '\n'));

			subtitle.insertLine(new SubtitleLine(text, showTime, hideTime));

			readLines++;
		}
		return readLines > 0;
	}

	MPlayer2InputFormat() :
		InputFormat(QStringLiteral("MPlayer2"), QStringList(QStringLiteral("mpl"))),
		m_lineRegExp(QStringLiteral("\\[(\\d+)\\]\\[(\\d+)\\]([^\n]+)\n"))
	{}

	mutable QRegExp m_lineRegExp;
};
}

#endif
