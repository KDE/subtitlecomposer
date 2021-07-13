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

#include "timetest.h"
#include "core/time.h"

#include <QTest>                               // krazy:exclude=c++/includes

using namespace SubtitleComposer;

void
TimeTest::testConstructors()
{
	Time time;
	QVERIFY(time.toMillis() == 0);

	Time time6(-55);
	QVERIFY(time6.toMillis() == 0);

	Time time2(999);
	QVERIFY(time2.toMillis() == 999);

	Time time3(5, 5, 5, 5);
	QVERIFY(time3.toMillis() == 18305005);

	Time time4(time2);
	QVERIFY(time4.toMillis() == 999);
}

void
TimeTest::testSetters()
{
	Time time;

	time.setSecondsTime(555.663);
	QVERIFY(time.toMillis() == 555663);

	time.setMillisTime(64563);
	QVERIFY(time.toMillis() == 64563);

	// the following setters should all fail:
	QVERIFY(!time.setHours(-5));
	QVERIFY(time.toMillis() == 64563);
	QVERIFY(!time.setMinutes(60) && !time.setMinutes(-5) && !time.setMinutes(61));
	QVERIFY(time.toMillis() == 64563);
	QVERIFY(!time.setSeconds(60) && !time.setSeconds(-5) && !time.setSeconds(61));
	QVERIFY(time.toMillis() == 64563);
	QVERIFY(!time.setMillis(1000) && !time.setMillis(-5) && !time.setMillis(1001));
	QVERIFY(time.toMillis() == 64563);

	// the following setters should all succeed:
	time.setMillisTime(0);
	QVERIFY(time.setHours(23));
	QVERIFY(time.toMillis() == 82800000);
	QVERIFY(time.setMinutes(14));
	QVERIFY(time.toMillis() == 83640000);
	QVERIFY(time.setSeconds(33));
	QVERIFY(time.toMillis() == 83673000);
	QVERIFY(time.setMillis(356));
	QVERIFY(time.toMillis() == 83673356);

	time.setMillisTime(-500);
	QVERIFY(time.toMillis() == 0);
	time.shift(150);
	QVERIFY(time.toMillis() == 150);
	time.shift(-130);
	QVERIFY(time.toMillis() == 20);
}

void
TimeTest::testOperators()
{
	QVERIFY(Time(3600) == Time(3600));
	QVERIFY(Time(3600) == 3600);
	QVERIFY(Time(3600) != Time(3601));
	QVERIFY(Time(3600) != 3601);
	QVERIFY(Time(3600) <= Time(3600));
	QVERIFY(Time(3600) <= 3600);
	QVERIFY(Time(3600) <= Time(3601));
	QVERIFY(Time(3600) <= 3601);
	QVERIFY(Time(3600) < Time(3601));
	QVERIFY(Time(3600) < 3601);
	QVERIFY(Time(3600) >= Time(3600));
	QVERIFY(Time(3600) >= 3600);
	QVERIFY(Time(3600) >= Time(3599));
	QVERIFY(Time(3600) >= 3599);
	QVERIFY(Time(3600) > Time(3599));
	QVERIFY(Time(3600) > 3599);
}

QTEST_GUILESS_MAIN(TimeTest);
