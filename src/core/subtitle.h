/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SUBTITLE_H
#define SUBTITLE_H

#include "core/range.h"
#include "core/rangelist.h"
#include "core/time.h"
#include "core/richstring.h"
#include "core/subtitletarget.h"
#include "core/undo/undostack.h"
#include "helpers/objectref.h"
#include "formatdata.h"

#include <vector>

#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QUndoCommand)
QT_FORWARD_DECLARE_CLASS(QTextEdit)

namespace SubtitleComposer {
class RichDocument;
class SubtitleLine;
class UndoAction;
class RichCSS;

class Subtitle : public QObject, public QSharedData
{
	Q_OBJECT

	friend class UndoStack;

	friend class SubtitleLine;
	friend class SubtitleIterator;

	friend class UndoAction;

	friend class SubtitleAction;
	friend class SetFramesPerSecondAction;
	friend class InsertLinesAction;
	friend class RemoveLinesAction;
	friend class MoveLineAction;
	friend class EditStylesheetAction;

	friend class SubtitleLineAction;
	friend class SetLinePrimaryTextAction;
	friend class SetLineSecondaryTextAction;
	friend class SetLineTextsAction;
	friend class SetLineShowTimeAction;
	friend class SetLineHideTimeAction;
	friend class SetLineTimesAction;
	friend class SetLineErrorsAction;
	friend class ToggleLineMarkedAction;

	friend class SubtitleCompositeActionExecutor;

	friend class Format;
	friend class InputFormat;

public:
	static double defaultFramesPerSecond();
	static void setDefaultFramesPerSecond(double framesPerSecond);

	Subtitle(double framesPerSecond = defaultFramesPerSecond());
	virtual ~Subtitle();

/// primary data includes primary text, timing information, format data and all errors except secondary only errors
	void setPrimaryData(const Subtitle &from, bool usePrimaryData);
	void clearPrimaryTextData();

/// secondary data includes secondary text and secondary only errors
	void setSecondaryData(const Subtitle &from, bool usePrimaryData);
	void clearSecondaryTextData();

	inline bool isPrimaryDirty() const { return m_primaryDirtyState; }
	void clearPrimaryDirty();

	inline bool isSecondaryDirty() const { return m_secondaryDirtyState; }
	void clearSecondaryDirty();

	double framesPerSecond() const;
	void setFramesPerSecond(double framesPerSecond);
	void changeFramesPerSecond(double toFramesPerSecond, double fromFramesPerSecond = -1.0);

	bool isEmpty() const { return m_lines.empty(); }
	int linesCount() const { return m_lines.size(); }
	int lastIndex() const { return m_lines.size() - 1; }

	SubtitleLine * line(int index);
	const SubtitleLine * line(int index) const;

	inline SubtitleLine * firstLine() { return m_lines.empty() ? nullptr : m_lines.front().obj(); }
	inline const SubtitleLine * firstLine() const { return m_lines.empty() ? nullptr : m_lines.front().obj(); }

	inline SubtitleLine * lastLine() { return m_lines.empty() ? nullptr : m_lines.back().obj(); }
	inline const SubtitleLine * lastLine() const { return m_lines.empty() ? nullptr : m_lines.back().obj(); }

	inline int count() const { return m_lines.size(); }
	inline const SubtitleLine * at(const int i) const { return m_lines.at(i).obj(); }
	inline SubtitleLine * at(const int i) { return m_lines.at(i).obj(); }
	inline const SubtitleLine * operator[](const int i) const { return m_lines.at(i).obj(); }
	inline SubtitleLine * operator[](const int i) { return m_lines.at(i).obj(); }

	bool hasAnchors() const;
	bool isLineAnchored(int index) const;
	bool isLineAnchored(const SubtitleLine *line) const;
	void toggleLineAnchor(int index);
	void toggleLineAnchor(const SubtitleLine *line);
	void removeAllAnchors();

	void insertLine(SubtitleLine *line);
	SubtitleLine * insertNewLine(int index, bool timeAfter, SubtitleTarget target);
	void removeLines(const RangeList &ranges, SubtitleTarget target);

	void swapTexts(const RangeList &ranges);

	void splitLines(const RangeList &ranges);
	void joinLines(const RangeList &ranges);

	void shiftAnchoredLine(SubtitleLine *anchoredLine, const Time &newShowTime);

	void shiftLines(const RangeList &ranges, long msecs);
	void adjustLines(const Range &range, long firstTime, long lastTime);
	void sortLines(const Range &range);

	void applyDurationLimits(const RangeList &ranges, const Time &minDuration, const Time &maxDuration, bool canOverlap);
	void setMaximumDurations(const RangeList &ranges);
	void setAutoDurations(const RangeList &ranges, int msecsPerChar, int msecsPerWord, int msecsPerLine, bool canOverlap, SubtitleTarget calculationTarget);

	void fixOverlappingLines(const RangeList &ranges, const Time &minInterval = 100);

	void fixPunctuation(const RangeList &ranges, bool spaces, bool quotes, bool englishI, bool ellipsis, SubtitleTarget target);

	void lowerCase(const RangeList &ranges, SubtitleTarget target);
	void upperCase(const RangeList &ranges, SubtitleTarget target);
	void titleCase(const RangeList &ranges, bool lowerFirst, SubtitleTarget target);
	void sentenceCase(const RangeList &ranges, bool lowerFirst, SubtitleTarget target);

	void breakLines(const RangeList &ranges, unsigned minLengthForLineBreak, SubtitleTarget target);
	void unbreakTexts(const RangeList &ranges, SubtitleTarget target);
	void simplifyTextWhiteSpace(const RangeList &ranges, SubtitleTarget target);

	void syncWithSubtitle(const Subtitle &refSubtitle);
	void appendSubtitle(const Subtitle &srcSubtitle, double shiftMsecsBeforeAppend);
	void splitSubtitle(Subtitle &dstSubtitle, const Time &splitTime, bool shiftSplitLines);

	void toggleStyleFlag(const RangeList &ranges, RichString::StyleFlag styleFlag);
	void changeTextColor(const RangeList &ranges, QRgb color);

	void setMarked(const RangeList &ranges, bool value);
	void toggleMarked(const RangeList &ranges);

	void clearErrors(const RangeList &ranges, int errorFlags);
	void checkErrors(const RangeList &ranges, int errorFlags);
	void recheckErrors(const RangeList &ranges);

	inline bool metaExists(const QByteArray &key) const { return m_metaData.contains(key); }
	inline int metaRemove(const QByteArray &key) { return m_metaData.remove(key); }
	inline const QString meta(const QByteArray &key) const { return m_metaData.value(key); }
	inline void meta(const QByteArray &key, const QString &value) { m_metaData.insert(key, value); }

	void stylesheetEdit(QTextEdit *textEdit);
	void stylesheetAppend(const QString &css);
	void stylesheetClear();
	inline const RichCSS *stylesheet() const { return m_stylesheet; }

signals:
	void primaryChanged();
	void secondaryChanged();

	void primaryDirtyStateChanged(bool dirty);
	void secondaryDirtyStateChanged(bool dirty);

	void framesPerSecondChanged(double fps);
	void linesAboutToBeInserted(int firstIndex, int lastIndex);
	void linesInserted(int firstIndex, int lastIndex);
	void linesAboutToBeRemoved(int firstIndex, int lastIndex);
	void linesRemoved(int firstIndex, int lastIndex);

	void compositeActionStart();
	void compositeActionEnd();

	void lineAnchorChanged(const SubtitleLine *line, bool anchored);

/// forwarded line signals
	void linePrimaryTextChanged(SubtitleLine *line);
	void lineSecondaryTextChanged(SubtitleLine *line);
	void lineShowTimeChanged(SubtitleLine *line);
	void lineHideTimeChanged(SubtitleLine *line);
	void lineErrorFlagsChanged(SubtitleLine *line);
	void lineMarkChanged(SubtitleLine *line);

private:
	inline int insertIndex(const Time &showTime) const { return insertIndex(showTime, 0, m_lines.empty() ? 0 : m_lines.size() - 1); }
	int insertIndex(const Time &showTime, int start, int end) const;
	void insertLine(SubtitleLine *line, int index);

	FormatData * formatData() const;
	void setFormatData(const FormatData *formatData);

	void beginCompositeAction(const QString &title) const;
	void endCompositeAction(UndoStack::DirtyMode dirtyOverride = UndoStack::Invalid) const;
	void processAction(UndoAction *action) const;

	bool isPrimaryDirty(int index) const;
	bool isSecondaryDirty(int index) const;
	void updateState();

	inline int normalizeRangeIndex(int index) const { return size_t(index) >= m_lines.size() ? m_lines.size() - 1 : index; }

	inline SubtitleLine * takeAt(const int i) { SubtitleLine *s = m_lines.at(i).obj(); m_lines.erase(m_lines.cbegin() + i); return s; }

	inline bool ignoreDocChanges(bool ignore) {
		bool r = m_ignoreDocChanges;
		m_ignoreDocChanges = ignore;
		return r;
	}

private:
	bool m_primaryDirtyState;
	int m_primaryCleanIndex;
	bool m_secondaryDirtyState;
	int m_secondaryCleanIndex;

	bool m_ignoreDocChanges = false;

	double m_framesPerSecond;
	mutable ObjectRefArray<SubtitleLine> m_lines;
	QList<QPointer<const SubtitleLine>> m_anchoredLines;

	QMap<QByteArray, QString> m_metaData;

	RichCSS *m_stylesheet;

	FormatData *m_formatData;

	static double s_defaultFramesPerSecond;
};

class SubtitleCompositeActionExecutor
{
public:
	SubtitleCompositeActionExecutor(const Subtitle *subtitle, const QString &title);
	~SubtitleCompositeActionExecutor();

private:
	const Subtitle *m_subtitle;
};
}

#include "subtitleline.h"

#endif
