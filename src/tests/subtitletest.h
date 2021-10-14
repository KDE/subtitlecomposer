/*
    SPDX-FileCopyrightText: 2021 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SUBTITLESORTTEST_H
#define SUBTITLESORTTEST_H

#include "core/subtitle.h"

#include <QObject>

class SubtitleTest : public QObject
{
	Q_OBJECT

public:
	SubtitleTest() : sub(new SubtitleComposer::Subtitle) {}

private slots:
	void testSort_data();
	void testSort();

private:
	QExplicitlySharedDataPointer<SubtitleComposer::Subtitle> sub;
};

#endif // SUBTITLESORTTEST_H
