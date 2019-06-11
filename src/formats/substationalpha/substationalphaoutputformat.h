#ifndef SUBSTATIONALPHAOUTPUTFORMAT_H
#define SUBSTATIONALPHAOUTPUTFORMAT_H

/*
 * Copyright (C) 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * Copyright (C) 2010-2018 Mladen Milinkovic <max@smoothware.net>
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
#include "core/formatdata.h"
#include "core/subtitleiterator.h"

namespace SubtitleComposer {
class SubStationAlphaOutputFormat : public OutputFormat
{
	friend class FormatManager;

public:
	QString fromSString(const SString &text) const
	{

		QString subtitle;

		int prevStyle = 0;
		QRgb prevColor = 0;
		for(int i = 0, sz = text.length(); i < sz; i++) {
			int curStyle = text.styleFlagsAt(i);
			QRgb curColor = (curStyle & SString::Color) != 0 ? text.styleColorAt(i) : 0;
			curStyle &= SString::Bold | SString::Italic | SString::Underline;
			if(prevStyle != curStyle) {
				int diff = curStyle ^ prevStyle;
				subtitle += '{';
				if(diff & SString::Bold)
					subtitle += curStyle & SString::Bold ? QStringLiteral("\\b1") : QStringLiteral("\\b0");
				if(diff & SString::Italic)
					subtitle += curStyle & SString::Italic ? QStringLiteral("\\i1") : QStringLiteral("\\i0");
				if(diff & SString::Underline)
					subtitle += curStyle & SString::Underline ? QStringLiteral("\\u1") : QStringLiteral("\\u0");
				subtitle += "}";
			}
			if(prevColor != curColor) {
				subtitle += QStringLiteral("{\\c&H");
				if(curColor != 0) {
					subtitle += (QChar('0') + QString::number(qBlue(curColor), 16)).toUpper().right(2);
					subtitle += (QChar('0') + QString::number(qGreen(curColor), 16)).toUpper().right(2);
					subtitle += (QChar('0') + QString::number(qRed(curColor), 16)).toUpper().right(2);
				} else {
					subtitle += QStringLiteral("00");
				}
				subtitle += QStringLiteral("&}");
			}

			subtitle += text.at(i);

			prevStyle = curStyle;
			prevColor = curColor;
		}

		if(prevStyle) {
			subtitle +='{';
			if(prevStyle & SString::Bold)
				subtitle += QStringLiteral("\\b0");
			if(prevStyle & SString::Italic)
				subtitle += QStringLiteral("\\i0");
			if(prevStyle & SString::Underline)
				subtitle += QStringLiteral("\\u0");
			subtitle += '}';
		}

		subtitle = subtitle.replace(QStringLiteral("\r\n"), QStringLiteral("\\N"));
		subtitle = subtitle.replace(QStringLiteral("\n"), QStringLiteral("\\N"));
		subtitle = subtitle.replace(QStringLiteral("\r"), QStringLiteral("\\N"));

		return subtitle;
	}

protected:
	static QString normalizeBlock(const QString &data) {
		int begin = 0, end = data.length() - 1;

		while(data.at(begin).isSpace() && begin < end)
			begin++;

		while(data.at(end).isSpace() && end >= begin)
			end--;

		return data.mid(begin, end - begin + 1) + QStringLiteral("\n\n");
	}

	virtual QString dumpSubtitles(const Subtitle &subtitle, bool primary) const override
	{
		FormatData *formatData = this->formatData(subtitle);

		QString ret = normalizeBlock(formatData ? formatData->value(QStringLiteral("ScriptInfo")) : m_defaultScriptInfo)
				+ normalizeBlock(formatData ? formatData->value(QStringLiteral("Styles")) : m_defaultStyles)
				+ normalizeBlock(m_events);

		for(SubtitleIterator it(subtitle); it.current(); ++it) {
			const SubtitleLine *line = it.current();
			QString showTimeArg, hideTimeArg;

			Time showTime = line->showTime();
			showTimeArg.sprintf("%01d:%02d:%02d.%02d",
												  showTime.hours(),
												  showTime.minutes(),
												  showTime.seconds(),
												  (showTime.mseconds() + 5) / 10);

			Time hideTime = line->hideTime();
			hideTimeArg.sprintf("%01d:%02d:%02d.%02d",
												  hideTime.hours(),
												  hideTime.minutes(),
												  hideTime.seconds(),
												  (hideTime.mseconds() + 5) / 10);

			formatData = this->formatData(line);

			ret += QString(formatData ? formatData->value(QStringLiteral("Dialogue")) : m_dialogueBuilder)
					.arg(showTimeArg)
					.arg(hideTimeArg)
					.arg(fromSString(primary ? line->primaryText() : line->secondaryText()));
		}
		return ret;
	}

	SubStationAlphaOutputFormat(
			const QString &name = QStringLiteral("SubStation Alpha"),
			const QStringList &extensions = QStringList(QStringLiteral("ssa")),
			const QString &scriptInfo = s_defaultScriptInfo,
			const QString &styles = s_defaultStyles,
			const QString &events = s_events,
			const QString &dialogueBuilder = s_dialogueBuilder) :
		OutputFormat(name, extensions),
		m_defaultScriptInfo(scriptInfo),
		m_defaultStyles(styles),
		m_events(events),
		m_dialogueBuilder(dialogueBuilder)
	{}

	const QString m_defaultScriptInfo;
	const QString m_defaultStyles;
	const QString m_events;
	const QString m_dialogueBuilder;
	const QString m_textBuilder;

	static const char *s_defaultScriptInfo;
	static const char *s_defaultStyles;
	static const char *s_events;
	static const char *s_dialogueBuilder;
};

const char *SubStationAlphaOutputFormat::s_defaultScriptInfo = "[Script Info]\n"
		"Title: Untitled\n"
		"ScriptType: V4.00\n"
		"Collisions: Normal\n"
		"Timer: 100\n"
		"WrapStyle: 0\n";
const char *SubStationAlphaOutputFormat::s_defaultStyles = "[V4 Styles]\n"
		"Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, "
		"Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding\n"
		"Style: Default, Sans, 24, 16777215, 16777215, 16777215, 12632256, -1, 0, 1, 1, 1, 6, 30, 30, 415, 0, 0\n";
const char *SubStationAlphaOutputFormat::s_events = "[Events]\n"
		"Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";
const char *SubStationAlphaOutputFormat::s_dialogueBuilder = "Dialogue: Marked=0,%1,%2,Default,,0000,0000,0000,,%3\n";

class AdvancedSubStationAlphaOutputFormat : public SubStationAlphaOutputFormat
{
	friend class FormatManager;

protected:
	AdvancedSubStationAlphaOutputFormat() :
		SubStationAlphaOutputFormat(QStringLiteral("Advanced SubStation Alpha"), QStringList(QStringLiteral("ass")), s_defaultScriptInfo, s_defaultStyles, s_events, s_dialogueBuilder)
	{}

	static const char *s_defaultScriptInfo;
	static const char *s_defaultStyles;
	static const char *s_events;
	static const char *s_dialogueBuilder;
};

const char *AdvancedSubStationAlphaOutputFormat::s_defaultScriptInfo = "[Script Info]\n"
		"Title: Untitled\n"
		"ScriptType: V4.00+\n"
		"Collisions: Normal\n"
		"Timer: 100\n"
		"WrapStyle: 0\n";
const char *AdvancedSubStationAlphaOutputFormat::s_defaultStyles = "[V4+ Styles]\n"
		"Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeThrough, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
		"Style: Default, Sans, 16, &H00FFFFFF, &H00FFFFFF, &H00674436, &HFFFFFFF8, -1, 0, 0, 0, 100, 90, 0, 0, 1, 2.1, 0.5, 2, 15, 15, 15, 0\n";
const char *AdvancedSubStationAlphaOutputFormat::s_events = "[Events]\n"
		"Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text\n";
const char *AdvancedSubStationAlphaOutputFormat::s_dialogueBuilder = "Dialogue: 0,%1,%2,Default,,0000,0000,0000,,%3\n";
}

#endif
