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

#include "timeedit.h"
#include "timevalidator.h"

#include <QTime>
#include <QEvent>
#include <QKeyEvent>

#include <QDebug>

TimeEdit::TimeEdit(QWidget *parent) :
	QTimeEdit(parent),
	m_secsStep(100)
{
	setDisplayFormat("hh:mm:ss.zzz");
	setTimeRange(QTime(0, 0, 0, 0), QTime(23, 59, 59, 999));
	setWrapping(false);
	setAlignment(Qt::AlignHCenter);
	setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
	setCurrentSection(QDateTimeEdit::MSecSection);

	connect(this, &QDateTimeEdit::timeChanged, this, &TimeEdit::onTimeChanged);
}

int
TimeEdit::msecsStep() const
{
	return m_secsStep;
}

void
TimeEdit::setMSecsStep(int msecs)
{
	m_secsStep = msecs;
}

int
TimeEdit::value() const
{
	return QTime(0, 0, 0, 0).msecsTo(time());
}

void
TimeEdit::setValue(int value)
{
	if(wrapping()) {
		setTime(QTime(0, 0, 0, 0).addMSecs(value));
	} else {
		if(value < QTime(0, 0, 0, 0).msecsTo(minimumTime()))
			setTime(minimumTime());
		else if(value >= QTime(0, 0, 0, 0).msecsTo(maximumTime()))
			setTime(maximumTime());
		else
			setTime(QTime(0, 0, 0, 0).addMSecs(value));
	}
}

void
TimeEdit::onTimeChanged(const QTime &time)
{
	emit valueChanged(QTime(0, 0, 0, 0).msecsTo(time));
}

void
TimeEdit::stepBy(int steps)
{
	int oldValue = value();
	int newValue;

	setSelectedSection(currentSection());

	switch(currentSection()) {
	case QDateTimeEdit::MSecSection:
		newValue = oldValue + m_secsStep * steps;
		break;
	case QDateTimeEdit::SecondSection:
		newValue = oldValue + 1000 * steps;
		break;
	case QDateTimeEdit::MinuteSection:
		newValue = oldValue + 60000 * steps;
		break;
	case QDateTimeEdit::HourSection:
		newValue = oldValue + 3600000 * steps;
		break;
	default:
		return;
	}

	if(oldValue != newValue)
		setValue(newValue);
}

TimeEdit::StepEnabled
TimeEdit::stepEnabled() const
{
	if(wrapping()) {
		return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
	} else {
		QTime time = this->time();
		if(time == minimumTime())
			return QAbstractSpinBox::StepUpEnabled;
		else if(time == maximumTime())
			return QAbstractSpinBox::StepDownEnabled;
		else
			return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
	}
}

void
TimeEdit::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	if(key == Qt::Key_Return) {
		emit valueEntered(QTime(0, 0, 0, 0).msecsTo(time()));
	} else if(key == Qt::Key_Up) {
		stepUp();
	} else if(key == Qt::Key_Down) {
		stepDown();
	} else if(key == Qt::Key_PageUp) {
		setCurrentSection(QDateTimeEdit::MSecSection);
		stepUp();
	} else if(key == Qt::Key_PageDown) {
		setCurrentSection(QDateTimeEdit::MSecSection);
		stepDown();
	} else {
		QTimeEdit::keyPressEvent(event);
	}
}


