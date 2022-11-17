/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "adjusttimesdialog.h"
#include "widgets/timeedit.h"

#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>

#include <KLocalizedString>

using namespace SubtitleComposer;

AdjustTimesDialog::AdjustTimesDialog(QWidget *parent) :
	ActionDialog(i18n("Adjust"), parent)
{
	QGroupBox *settingsGroupBox = createGroupBox(i18nc("@title:group", "New Times"));

	m_firstLineTimeEdit = new TimeEdit(settingsGroupBox);

	QLabel *firstLineLabel = new QLabel(settingsGroupBox);
	firstLineLabel->setText(i18n("First spoken line:"));
	firstLineLabel->setBuddy(m_firstLineTimeEdit);

	m_lastLineTimeEdit = new TimeEdit(settingsGroupBox);

	QLabel *lastLineLabel = new QLabel(settingsGroupBox);
	lastLineLabel->setText(i18n("Last spoken line:"));
	lastLineLabel->setBuddy(m_lastLineTimeEdit);

	QGridLayout *settingsLayout = createLayout(settingsGroupBox);
	settingsLayout->addWidget(firstLineLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	settingsLayout->addWidget(m_firstLineTimeEdit, 0, 1);
	settingsLayout->addWidget(lastLineLabel, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	settingsLayout->addWidget(m_lastLineTimeEdit, 1, 1);
}

Time
AdjustTimesDialog::firstLineTime() const
{
	return Time(m_firstLineTimeEdit->value());
}

void
AdjustTimesDialog::setFirstLineTime(const Time &time)
{
	m_firstLineTimeEdit->setValue(time.toMillis());
}

Time
AdjustTimesDialog::lastLineTime() const
{
	return Time(m_lastLineTimeEdit->value());
}

void
AdjustTimesDialog::setLastLineTime(const Time &time)
{
	m_lastLineTimeEdit->setValue(time.toMillis());
}
