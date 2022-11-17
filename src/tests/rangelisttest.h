/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RANGELISTTEST_H
#define RANGELISTTEST_H

#include <QObject>

class RangeListTest : public QObject
{
	Q_OBJECT
private slots:
	void testConstructors();
	void testJoinAndTrim();
};

#endif
