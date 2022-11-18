/*
    SPDX-FileCopyrightText: 2021-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "webvttinputformat.h"

#include "core/richtext/richcss.h"
#include "core/richtext/richdocument.h"
#include "core/subtitle.h"
#include "core/subtitleline.h"
#include "helpers/common.h"

#include <QMap>
#include <QRegularExpression>
#include <QVector>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// QStringView use is unoptimized in Qt5, and some methods are missing pre 5.15
#include <QStringRef>
#define QStringView(x) QStringRef(&(x))
#define QStringView_ QStringRef
#define capturedView capturedRef
#else
#include <QStringView>
#define QStringView_ QStringView
#endif


using namespace SubtitleComposer;

WebVTTInputFormat::WebVTTInputFormat()
	: InputFormat($("WebVTT"), QStringList($("vtt")))
{
}

static int
skipTextLine(const QString &str, int off)
{
	for(; off < str.length(); off++) {
		if(str.at(off) == QChar::LineFeed)
			return off + 1;
	}
	return str.length();
}

static int
skipTextBlock(const QString &str, int off)
{
	for(bool prevLF = false; off < str.length(); off++) {
		const bool curLF = str.at(off) == QChar::LineFeed;
		if(prevLF && curLF)
			return off + 1;
		prevLF = curLF;
	}
	return str.length();
}

typedef bool (*charCompare)(QChar ch);

inline static int
skipChar(QStringView_ text, int off, const charCompare &cf)
{
	auto it = text.cbegin() + off;
	const auto end = text.cend();
	while(it != end && cf(*it))
		it++;
	return it - text.cbegin();
}

void
parseCueSettings(SubtitleLine *line, QStringView_ css)
{
	// https://developer.mozilla.org/en-US/docs/Web/API/WebVTT_API#cue_settings
	QMap<QByteArray, QStringView_> settings;
	int off = 0;
	while(off < css.size()) {
		off = skipChar(css, off, [](QChar c){ return c == QChar::Space || c == QChar::Tabulation; });
		int end = skipChar(css, off, [](QChar c){ return c != QChar(':'); });
		const QStringView_ key = css.mid(off, end - off);
		off = end + 1;
		end = skipChar(css, off, [](QChar c){ return c != QChar::Space && c != QChar::Tabulation; });
		const QStringView_ val = css.mid(off, end - off);
		if(!key.isEmpty() && !val.isEmpty())
			settings.insert(key.toLatin1(), val);
		off = end + 1;
	}

	SubtitleRect p;

	// vertical:rl|lr
	const QStringView_ csVert = settings.value("vertical");
	p.vertical = !csVert.isEmpty();
#if 0
	bool rl = p.vertical && csVert.compare(QByteArray("rl")) == 0; // vertical growing left/right (true/false)
#endif

	// align:<start|center|end|left|right>
	const QStringView_ csAlign = settings.value("align");
	if(csAlign.isEmpty())
		p.hAlign = SubtitleRect::CENTER;
	else if(csAlign.compare($("start")) == 0 || csAlign.compare($("left")) == 0) // FIXME: start should consider RTL?
		p.hAlign = SubtitleRect::START;
	else if(csAlign.compare($("end")) == 0 || csAlign.compare($("right")) == 0) // FIXME: end should consider RTL?
		p.hAlign = SubtitleRect::END;
	else
		p.hAlign = SubtitleRect::CENTER;

	// size:<n>%
	QStringView_ csSize = settings.value("size");
	if(csSize.back() == QChar('%'))
		csSize.chop(1);
	else
		qWarning() << "size css is missing '%'";
	const float posSize = csSize.isEmpty() ? 100 : csSize.toFloat();

	// position:<nFloat>%[,line-left|center|line-right]
	const QStringView_ csPos = settings.value("position");
	float pos = 0;
	int posAnchor = p.hAlign; // FIXME: should consider RTL
	{
		int n = skipChar(csPos, 0, [](QChar c){ return c >= QChar('0') && c <= QChar('9'); });
		if(n) {
			pos = csPos.mid(0, n).toFloat();
			if(n < csPos.size() && csPos.at(n) == QChar('%')) {
				qWarning() << "position css is missing '%'";
				n++;
			}
			if(n < csPos.size() && csPos.at(n) == QChar(',')) {
				n++;
				if(csPos.mid(n).compare($("line-left")) == 0)
					posAnchor = SubtitleRect::START;
				else if(csPos.mid(n).compare($("center")) == 0)
					posAnchor = SubtitleRect::CENTER;
				else if(csPos.mid(n).compare($("line-right")) == 0)
					posAnchor = SubtitleRect::END;
			}
		}
	}

	// line:<n>[%][,start|center|end]
#if 0
	const QStringView_ csLine = settings.value("line");
	float lineOff = 0.f;
	bool lineSnap = true;
	int lineAlign = SubtitleRect::START;
	{
		int n = skipChar(csLine, 0, [](QChar c){ return c >= QChar('0') && c <= QChar('9'); });
		if(n) {
			lineOff = csLine.mid(0, n).toFloat();
			if(n < csLine.size() && csLine.at(n) == QChar('%')) {
				lineSnap = false;
				n++;
			}
			if(n < csLine.size() && csLine.at(n) == QChar(',')) {
				n++;
				if(csLine.mid(n).compare($("center")) == 0)
					lineAlign = SubtitleRect::CENTER;
				else if(csLine.mid(n).compare($("end")) == 0)
					lineAlign = SubtitleRect::END;
			}
			p.vAlign = lineOff >= 0.f ? SubtitleRect::TOP : SubtitleRect::BOTTOM;
		} else {
			p.vAlign = SubtitleRect::BOTTOM;
		}
	}
#else
	// FIXME: line aligment is bad
	float lineOff = 0.f;
	p.vAlign = SubtitleRect::BOTTOM;
#endif

	if(p.vertical) {
		if(posAnchor == SubtitleRect::START) {
			p.top = pos;
			p.bottom = p.top + posSize;
		} else if(posAnchor == SubtitleRect::END) {
			p.bottom = 100.f - pos;
			p.top = p.bottom - posSize;
		} else { // posAnchor == SubtitleRect::CENTER
			p.top = pos - posSize / 2;
			p.bottom = p.top + posSize;
		}
		if(lineOff >= 0.f) {
			p.left = lineOff;
			p.right = 100.f;
		} else {
			p.left = 0.f;
			p.right = -lineOff;
		}
	} else {
		if(posAnchor == SubtitleRect::START) {
			p.left = pos;
			p.right = p.left + posSize;
		} else if(posAnchor == SubtitleRect::END) {
			p.right = 100.f - pos;
			p.left = p.right - posSize;
		} else { // posAnchor == SubtitleRect::CENTER
			p.left = pos - posSize / 2;
			p.right = p.left + posSize;
		}
		if(lineOff >= 0.f) {
			p.top = lineOff;
			p.bottom = 100.f;
		} else {
			p.top = 0.f;
			p.bottom = -lineOff;
		}
	}

	line->setPosition(p);
}

bool
WebVTTInputFormat::parseSubtitles(Subtitle &subtitle, const QString &data) const
{
	if(!data.startsWith($("WEBVTT")))
		return false;

	int off = skipTextBlock(data, 6);
	int end;
	const QStringView_ hdr = QStringView(data).mid(6, off - 6).trimmed();
	if(!hdr.isEmpty())
		subtitle.meta("comment.intro.0", hdr.toString());

	QVector<QStringView_> notes;
	staticRE$(reTime, "(?:([0-9]{2,}):)?([0-5][0-9]):([0-5][0-9])\\.([0-9]{3}) --> (?:([0-9]{2,}):)?([0-5][0-9]):([0-5][0-9])\\.([0-9]{3})\\b([^\\n]*)", REu);

	subtitle.stylesheetClear();

	// https://w3c.github.io/webvtt/
	while(off < data.length()) {
		if(QStringView(data).mid(off, 5) == $("STYLE")) {
			if(!notes.isEmpty()) { // store note before style
				int noteId = 0;
				for(const QStringView_ &note: notes)
					subtitle.meta(QByteArray("comment.top.") + QByteArray::number(noteId++), note.toString());
				notes.clear();
			}
			// NOTE: styles can't appear after first cue/line, even if we're not forbidding it
			end = skipTextBlock(data, off += 5);
			subtitle.stylesheetAppend(QStringView(data).mid(off, end - off).trimmed().toString());
			off = end;
			continue;
		}
		if(QStringView(data).mid(off, 4) == $("NOTE")) {
			end = skipTextBlock(data, off += 4);
			notes.push_back(QStringView(data).mid(off, end - off).trimmed());
			off = end;
			continue;
		}
		end = skipTextLine(data, off);
		QStringView_ cueId = QStringView(data).mid(off, end - off).trimmed();
		QStringView_ cueTime;
		off = end;
		if(cueId.contains($("-->"))) {
			cueTime = cueId;
			cueId = QStringView_();
		} else {
			end = skipTextLine(data, off);
			cueTime = QStringView(data).mid(off, end - off).trimmed();
			off = end;
		}
		QRegularExpressionMatch m = reTime.match(cueTime);
		if(!m.isValid()) {
			qWarning() << "Invalid WEBVTT subtitle";
			return false;
		}

		const Time showTime(m.capturedView(1).toInt(), m.capturedView(2).toInt(), m.capturedView(3).toInt(), m.capturedView(4).toInt());
		const Time hideTime(m.capturedView(5).toInt(), m.capturedView(6).toInt(), m.capturedView(7).toInt(), m.capturedView(8).toInt());
		QStringView_ cueSettings = m.capturedView(9);

		end = skipTextBlock(data, off);
		const QStringView_ cueText = QStringView(data).mid(off, end - off).trimmed();
		off = end;

		SubtitleLine *line = new SubtitleLine(showTime, hideTime);
		RichString stext;
		// TODO: handle voice/class tags
		// https://developer.mozilla.org/en-US/docs/Web/API/WebVTT_API#cue_payload_text_tags
		// TODO: handle pseudo classes
		// https://developer.mozilla.org/en-US/docs/Web/API/WebVTT_API#css_pseudo-classes
		stext.setRichString(cueText.toString());
		line->primaryDoc()->setRichText(stext, true);

		if(!notes.isEmpty()) {
			QString comment;
			for(const QStringView_ &note: notes) {
				if(!comment.isEmpty())
					comment.append(QChar::LineFeed);
				comment.append(note);
			}
			notes.clear();
			line->meta("comment", comment);
		}
		if(!cueSettings.isEmpty())
			parseCueSettings(line, cueSettings);
		if(!cueId.isEmpty())
			line->meta("id", cueId.toString());
		subtitle.insertLine(line);
	}

	if(!notes.isEmpty()) {
		int noteId = 0;
		for(const QStringView_ &note: notes)
			subtitle.meta(QByteArray("comment.bottom.") + QByteArray::number(noteId++), note.toString());
		notes.clear();
	}

	return true;
}
