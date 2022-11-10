/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MAIN_TESTS_H
#define MAIN_TESTS_H

#include "core/range.h"
#include "core/rangelist.h"
#include "core/richstring.h"
#include "core/subtitle.h"
#include "core/subtitleline.h"
#include "core/subtitleiterator.h"

#include <QGlobal>
#include <QtCore/QString>

#include <QDebug>

using namespace SubtitleComposer;

void
showRanges(const RangeList &ranges)
{
	QString aux;
	for(RangeList::ConstIterator it = ranges.begin(), end = ranges.end(); it != end; ++it)
		aux += QString(" [%1,%2]").arg(it->start()).arg(it->end());

	qDebug() << QString("Showing ranges: %1").arg(aux.trimmed());
}

void
showSubtitle(const Subtitle &subtitle)
{
	qDebug() << "Showing subtitle";
	for(int index = 0, size = subtitle.linesCount(); index < size; ++index)
		qDebug() << QString("Line: %1").arg(subtitle.line(index)->primaryText().richString());
	qDebug() << "--------------------------";
}

void
iterateSubtitle(const Subtitle &subtitle, const RangeList &ranges)
{
	showRanges(ranges);

//  for ( int idx=0; idx < 3; ++idx )
	{
		qDebug() << "Iterating subtitle forwards from ranges";
		for(SubtitleIterator it(subtitle, ranges); it.current(); ++it)
			qDebug() << QString("Line: %1").arg(it.current()->primaryText().richString());

		qDebug() << "Iterating subtitle backwards from ranges";
		for(SubtitleIterator it(subtitle, ranges, true); it.current(); --it)
			qDebug() << QString("Line: %1").arg(it.current()->primaryText().richString());
	}

	qDebug() << "--------------------------";
}

void
testSubtitleIterator()
{
	Subtitle subtitle;

	for(int index = 0; index < 20; ++index)
		subtitle.insertLine(new SubtitleLine(QString("Line %1").arg(index)));

	showSubtitle(subtitle);

	iterateSubtitle(subtitle, Range::full());
	iterateSubtitle(subtitle, Range::lower(10));
	iterateSubtitle(subtitle, Range::upper(10));
	iterateSubtitle(subtitle, Range::lower(50));
	iterateSubtitle(subtitle, Range::upper(50));
	RangeList ranges;
	ranges << Range(1, 3);
	ranges << Range(5, 5);
	ranges << Range(11, 17);
	iterateSubtitle(subtitle, ranges);

//  SubtitleIterator it( subtitle, ranges, true );
//  for ( int idx = 0; idx < 100; ++idx )
//      ++it;
//  for ( int idx = 0; idx < 200; ++idx )
//      --it;

//  SubtitleIterator it( subtitle, Range( 1, 50 ), true );
//  for ( int idx = 0; idx < 100; ++idx )
//      ++it;
//  for ( int idx = 0; idx < 200; ++idx )
//      --it;

	/*QTime time;
	   time.start();

	   for ( SubtitleIterator it( subtitle, Range::full(), true ); it.current(); --it )
	   it.current()->text();

	   qDebug() << time.elapsed();

	   for ( SubtitleIterator it( subtitle, Range::full(), true ); it.current(); --it )
	   it.current()->text();

	   qDebug() << time.elapsed(); */
}

int
main(int, char **)
{
	// testSubtitleIterator();
}

#endif
