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

#include "core/time.h"

using namespace SubtitleComposer;

Time::Time(double mseconds)
{
	setMillisTime(mseconds);
}

Time::Time(int hours, int minutes, int seconds, int mseconds)
	: m_mseconds(0)
{
	setHours(hours);
	setMinutes(minutes);
	setSeconds(seconds);
	setMseconds(mseconds);
}

Time::Time(const Time &time)
	: m_mseconds(time.m_mseconds)
{

}

void
Time::setSecondsTime(double seconds)
{
	setMillisTime(seconds * 1000);
}

void
Time::setMillisTime(double mseconds)
{
	if(mseconds < 0.0)
		m_mseconds = 0.0;
	else if(mseconds > MaxMseconds)
		m_mseconds = MaxMseconds;
	else
		m_mseconds = mseconds;
}

QString
Time::toString(bool showMillis) const
{
	const int msec = m_mseconds + 0.5;
	const int hours = msec / 3600000;
	const int minutes = (msec % 3600000) / 60000;
	const int seconds = (msec % 60000) / 1000;

	if(showMillis)
		return QString::asprintf("%02d:%02d:%02d.%03d", hours, minutes, seconds, msec % 1000);

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

int
Time::hours() const
{
	return int(m_mseconds + 0.5) / 3600000;
}

bool
Time::setHours(int hours)
{
	if(hours < 0 || hours > 23)
		return false;

	m_mseconds += (hours - this->hours()) * 3600000;

	return true;
}

int
Time::minutes() const
{
	return (int(m_mseconds + 0.5) % 3600000) / 60000;
}

bool
Time::setMinutes(int minutes)
{
	if(minutes < 0 || minutes > 59)
		return false;

	m_mseconds += (minutes - this->minutes()) * 60000;

	return true;
}

int
Time::seconds() const
{
	return (int(m_mseconds + 0.5) % 60000) / 1000;
}

bool
Time::setSeconds(int seconds)
{
	if(seconds < 0 || seconds > 59)
		return false;

	m_mseconds += (seconds - this->seconds()) * 1000;

	return true;
}

int
Time::mseconds() const
{
	return int(m_mseconds + 0.5) % 1000;
}

bool
Time::setMseconds(int mseconds)
{
	if(mseconds < 0 || mseconds > 999)
		return false;

	m_mseconds += mseconds - this->mseconds();

	return true;
}

void
Time::shift(double mseconds)
{
	setMillisTime(m_mseconds + mseconds);
}

Time
Time::shifted(double mseconds) const
{
	return Time(m_mseconds + mseconds);
}

void
Time::adjust(double shiftMseconds, double scaleFactor)
{
	setMillisTime(shiftMseconds + m_mseconds * scaleFactor);
}

Time
Time::adjusted(double shiftMseconds, double scaleFactor) const
{
	return Time(shiftMseconds + m_mseconds * scaleFactor);
}

Time &
Time::operator=(const Time &time)
{
	if(this == &time)
		return *this;

	m_mseconds = time.m_mseconds;

	return *this;
}

Time &
Time::operator=(double mseconds)
{
	setMillisTime(mseconds);

	return *this;
}

Time
Time::operator+(const Time &time) const
{
	return Time(m_mseconds + time.m_mseconds);
}

Time
Time::operator+(double mseconds) const
{
	return Time(m_mseconds + mseconds);
}

Time &
Time::operator+=(const Time &time)
{
	setMillisTime(m_mseconds + time.m_mseconds);
	return *this;
}

Time &
Time::operator+=(double mseconds)
{
	setMillisTime(m_mseconds + mseconds);
	return *this;
}

Time
Time::operator-(const Time &time) const
{
	return Time(m_mseconds - time.m_mseconds);
}

Time
Time::operator-(double mseconds) const
{
	return Time(m_mseconds - mseconds);
}

Time &
Time::operator-=(const Time &time)
{
	setMillisTime(m_mseconds - time.m_mseconds);
	return *this;
}

Time &
Time::operator-=(double mseconds)
{
	setMillisTime(m_mseconds - mseconds);
	return *this;
}

bool
Time::operator==(const Time &time) const
{
	return m_mseconds == time.m_mseconds;
}

bool
Time::operator==(double mseconds) const
{
	return m_mseconds == mseconds;
}

bool
Time::operator!=(const Time &time) const
{
	return m_mseconds != time.m_mseconds;
}

bool
Time::operator!=(double mseconds) const
{
	return m_mseconds != mseconds;
}

bool
Time::operator<(const Time &time) const
{
	return m_mseconds < time.m_mseconds;
}

bool
Time::operator<(double mseconds) const
{
	return m_mseconds < mseconds;
}

bool
Time::operator<=(const Time &time) const
{
	return m_mseconds <= time.m_mseconds;
}

bool
Time::operator<=(double mseconds) const
{
	return m_mseconds <= mseconds;
}

bool
Time::operator>(const Time &time) const
{
	return m_mseconds > time.m_mseconds;
}

bool
Time::operator>(double mseconds) const
{
	return m_mseconds > mseconds;
}

bool
Time::operator>=(const Time &time) const
{
	return m_mseconds >= time.m_mseconds;
}

bool
Time::operator>=(double mseconds) const
{
	return m_mseconds >= mseconds;
}
