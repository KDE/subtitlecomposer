/*
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WAVESUBTITLE_H
#define WAVESUBTITLE_H

#include <core/time.h>

#include <QImage>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTextLayout)

namespace SubtitleComposer {
class RichDocument;
class SubtitleLine;
class WaveRenderer;

enum DragPosition {
	DRAG_NONE = 0,
	DRAG_FORBIDDEN,
	DRAG_SHOW,
	DRAG_LINE,
	DRAG_HIDE
};

class WaveSubtitle : public QObject
{
	Q_OBJECT

public:
	explicit WaveSubtitle(SubtitleLine *line, WaveRenderer *parent);

	virtual ~WaveSubtitle();

	inline SubtitleLine * line() const { return m_line; }

	DragPosition draggableAt(double time, double msTolerance) const;

	void dragStart(DragPosition dragMode, double dragTime);
	inline void dragUpdate(double dragTime) { m_dragTime = dragTime; }
	DragPosition dragEnd(double dragTime);

	Time showTime() const;
	Time hideTime() const;

	const QImage & image() const;

private:
	SubtitleLine *m_line;
	WaveRenderer *m_rend;

	mutable QImage m_image;
	mutable bool m_imageDirty = true;

	DragPosition m_dragMode = DRAG_NONE;
	double m_dragTime = 0.;
	double m_dragTimeOffset = 0.;
};
}

#endif // WAVESUBTITLE_H
