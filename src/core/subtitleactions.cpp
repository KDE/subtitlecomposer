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

#include "core/subtitleactions.h"
#include "core/subtitleiterator.h"
#include "core/subtitleline.h"
#include "core/sstring.h"

#include <QObject>

#include <KLocalizedString>

using namespace SubtitleComposer;

// *** SubtitleAction
SubtitleAction::SubtitleAction(Subtitle &subtitle, SubtitleAction::DirtyMode dirtyMode, const QString &description)
	: UndoAction(dirtyMode, &subtitle, description),
	  m_subtitle(subtitle)
{}

SubtitleAction::~SubtitleAction()
{}


// *** SetFramesPerSecondAction
SetFramesPerSecondAction::SetFramesPerSecondAction(Subtitle &subtitle, double framesPerSecond)
	: SubtitleAction(subtitle, UndoAction::Both, i18n("Set Frame Rate")),
	  m_framesPerSecond(framesPerSecond)
{}

SetFramesPerSecondAction::~SetFramesPerSecondAction()
{}

void
SetFramesPerSecondAction::redo()
{
	double tmp = m_subtitle.m_framesPerSecond;
	m_subtitle.m_framesPerSecond = m_framesPerSecond;
	m_framesPerSecond = tmp;

	emit m_subtitle.framesPerSecondChanged(m_subtitle.m_framesPerSecond);
}


// *** InsertLinesAction
InsertLinesAction::InsertLinesAction(Subtitle &subtitle, const QList<SubtitleLine *> &lines, int insertIndex)
	: SubtitleAction(subtitle, UndoAction::Both, i18n("Insert Lines")),
	  m_insertIndex(insertIndex < 0 ? subtitle.linesCount() : insertIndex),
	  m_lastIndex(m_insertIndex + lines.count() - 1),
	  m_lines(lines)
{
	Q_ASSERT(m_insertIndex >= 0);
	Q_ASSERT(m_insertIndex <= m_subtitle.linesCount());
	Q_ASSERT(m_lastIndex >= 0);
	Q_ASSERT(m_insertIndex <= m_lastIndex);
}

InsertLinesAction::~InsertLinesAction()
{
	qDeleteAll(m_lines);
}

bool
InsertLinesAction::mergeWith(const QUndoCommand *command)
{
	const InsertLinesAction *currentAction = static_cast<const InsertLinesAction *>(command);
	if(&currentAction->m_subtitle != &m_subtitle)
		return false;

	if(currentAction->m_insertIndex == m_lastIndex + 1 || (m_insertIndex <= currentAction->m_lastIndex && currentAction->m_insertIndex <= m_lastIndex)) {
		m_lastIndex += currentAction->m_lastIndex - currentAction->m_insertIndex + 1;
		if(m_insertIndex > currentAction->m_insertIndex) {
			m_lastIndex -= m_insertIndex - currentAction->m_insertIndex;
			m_insertIndex = currentAction->m_insertIndex;
		}
		return true;
	}

	return false;
}

void
InsertLinesAction::redo()
{
	emit m_subtitle.linesAboutToBeInserted(m_insertIndex, m_lastIndex);

	SubtitleLine *line;
	int insertOffset = 0;
	int lineIndex = -1;

	while(!m_lines.isEmpty()) {
		line = m_lines.takeFirst();
		lineIndex = m_insertIndex + insertOffset++;
		setLineSubtitle(line);
		m_subtitle.m_lines.insert(lineIndex, line);
	}

	emit m_subtitle.linesInserted(m_insertIndex, m_lastIndex);
}

void
InsertLinesAction::undo()
{
	emit m_subtitle.linesAboutToBeRemoved(m_insertIndex, m_lastIndex);

	for(int index = m_insertIndex; index <= m_lastIndex; ++index) {
		SubtitleLine *line = m_subtitle.takeAt(m_insertIndex);
		clearLineSubtitle(line);
		m_lines.append(line);
	}

	emit m_subtitle.linesRemoved(m_insertIndex, m_lastIndex);
}


// *** RemoveLinesAction
RemoveLinesAction::RemoveLinesAction(Subtitle &subtitle, int firstIndex, int lastIndex)
	: SubtitleAction(subtitle, UndoAction::Both, i18n("Remove Lines")),
	  m_firstIndex(firstIndex),
	  m_lastIndex(lastIndex < 0 ? subtitle.lastIndex() : lastIndex),
	  m_lines()
{
	Q_ASSERT(m_firstIndex >= 0);
	Q_ASSERT(m_firstIndex <= m_subtitle.linesCount());
	Q_ASSERT(m_lastIndex >= 0);
	Q_ASSERT(m_lastIndex <= m_subtitle.linesCount());
	Q_ASSERT(m_firstIndex <= m_lastIndex);
}

RemoveLinesAction::~RemoveLinesAction()
{
	qDeleteAll(m_lines);
}

bool
RemoveLinesAction::mergeWith(const QUndoCommand *command)
{
	const RemoveLinesAction *currentAction = static_cast<const RemoveLinesAction *>(command);
	if(&currentAction->m_subtitle != &m_subtitle)
		return false;

	if(m_firstIndex == currentAction->m_firstIndex) {
		// currentAction removed lines immediately below those removed by this
		m_lastIndex += currentAction->m_lines.count();
		while(!currentAction->m_lines.isEmpty())
			m_lines.append(const_cast<RemoveLinesAction *>(currentAction)->m_lines.takeFirst());
		return true;
	}

	if(currentAction->m_lastIndex + 1 == m_firstIndex) {
		// currentAction removed lines immediately above those removed by this
		m_firstIndex = currentAction->m_firstIndex;
		while(!currentAction->m_lines.isEmpty())
			m_lines.prepend(const_cast<RemoveLinesAction *>(currentAction)->m_lines.takeLast());
		return true;
	}

	return false;
}

void
RemoveLinesAction::redo()
{
	emit m_subtitle.linesAboutToBeRemoved(m_firstIndex, m_lastIndex);

	for(int index = m_firstIndex; index <= m_lastIndex; ++index) {
		SubtitleLine *line = m_subtitle.takeAt(m_firstIndex);
		clearLineSubtitle(line);
		m_lines.append(line);
	}

	emit m_subtitle.linesRemoved(m_firstIndex, m_lastIndex);
}

void
RemoveLinesAction::undo()
{
	emit m_subtitle.linesAboutToBeInserted(m_firstIndex, m_lastIndex);

	int insertOffset = 0;
	int lineIndex = -1;

	while(!m_lines.isEmpty()) {
		SubtitleLine *line = m_lines.takeFirst();
		lineIndex = m_firstIndex + insertOffset++;
		setLineSubtitle(line);
		m_subtitle.m_lines.insert(lineIndex, line);
	}

	emit m_subtitle.linesInserted(m_firstIndex, m_lastIndex);
}


// *** MoveLineAction
MoveLineAction::MoveLineAction(Subtitle &subtitle, int fromIndex, int toIndex) :
	SubtitleAction(subtitle, UndoAction::Both, i18n("Move Line")),
	m_fromIndex(fromIndex),
	m_toIndex(toIndex < 0 ? subtitle.lastIndex() : toIndex)
{
	Q_ASSERT(m_fromIndex >= 0);
	Q_ASSERT(m_fromIndex <= m_subtitle.linesCount());
	Q_ASSERT(m_toIndex >= 0);
	Q_ASSERT(m_toIndex <= m_subtitle.linesCount());
	Q_ASSERT(m_fromIndex != m_toIndex);
}

MoveLineAction::~MoveLineAction()
{}

bool
MoveLineAction::mergeWith(const QUndoCommand *command)
{
	const MoveLineAction *currentAction = static_cast<const MoveLineAction *>(command);
	if(&currentAction->m_subtitle != &m_subtitle)
		return false;

	Q_ASSERT(command != this);

	// TODO: FIXME: this and currentAction were swapped in new Qt's implementation, so below code is not working
	// since move is used only when sorting - this will never be called
	if(currentAction->m_toIndex == m_fromIndex) {
		m_fromIndex = currentAction->m_fromIndex;
		return true;
	} else if(m_toIndex - m_fromIndex == 1 || m_fromIndex - m_toIndex == 1) {
		if(currentAction->m_toIndex == m_toIndex) {
			// when the distance between fromIndex and toIndex is 1, the action is the same as if the values were swapped
			m_toIndex = m_fromIndex;
			m_fromIndex = currentAction->m_fromIndex;
			return true;
		}
		if(currentAction->m_toIndex - currentAction->m_fromIndex == 1 || currentAction->m_fromIndex - currentAction->m_toIndex == 1) {
			// same as before, but now we consider inverting the previous action too
			if(currentAction->m_fromIndex == m_toIndex) {
				m_toIndex = m_fromIndex;
				m_fromIndex = currentAction->m_toIndex;
				return true;
			}
		}
	} else if(currentAction->m_toIndex - currentAction->m_fromIndex == 1 || currentAction->m_fromIndex - currentAction->m_toIndex == 1) {
		// again, same as before, but now we consider inverting only the previous action
		if(currentAction->m_fromIndex == m_fromIndex) {
			m_fromIndex = currentAction->m_toIndex;
			return true;
		}
	}

	return false;
}

void
MoveLineAction::redo()
{
	emit m_subtitle.linesAboutToBeRemoved(m_fromIndex, m_fromIndex);
	SubtitleLine *line = m_subtitle.takeAt(m_fromIndex);
	clearLineSubtitle(line);
	emit m_subtitle.linesRemoved(m_fromIndex, m_fromIndex);

	emit m_subtitle.linesAboutToBeInserted(m_toIndex, m_toIndex);
	setLineSubtitle(line);
	m_subtitle.m_lines.insert(m_toIndex, line);
	emit m_subtitle.linesInserted(m_toIndex, m_toIndex);
}

void
MoveLineAction::undo()
{
	emit m_subtitle.linesAboutToBeRemoved(m_toIndex, m_toIndex);
	SubtitleLine *line = m_subtitle.takeAt(m_toIndex);
	clearLineSubtitle(line);
	emit m_subtitle.linesRemoved(m_toIndex, m_toIndex);

	emit m_subtitle.linesAboutToBeInserted(m_fromIndex, m_fromIndex);
	setLineSubtitle(line);
	m_subtitle.m_lines.insert(m_fromIndex, line);
	emit m_subtitle.linesInserted(m_fromIndex, m_fromIndex);
}


// *** SwapLinesTextsAction
SwapLinesTextsAction::SwapLinesTextsAction(Subtitle &subtitle, const RangeList &ranges) :
	SubtitleAction(subtitle, UndoAction::Both, i18n("Swap Texts")),
	m_ranges(ranges)
{}

SwapLinesTextsAction::~SwapLinesTextsAction()
{}

void
SwapLinesTextsAction::redo()
{
	for(SubtitleIterator it(m_subtitle, m_ranges); it.current(); ++it) {
		SubtitleLine *line = it.current();
		SString aux = line->m_primaryText;

		line->m_secondaryText = line->m_primaryText;
		emit line->primaryTextChanged(line->m_primaryText);

		line->m_primaryText = aux;
		emit line->secondaryTextChanged(line->m_secondaryText);
	}
}
