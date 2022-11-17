/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fixoverlappingtimesdialog.h"

#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QSpinBox>

using namespace SubtitleComposer;

FixOverlappingTimesDialog::FixOverlappingTimesDialog(QWidget *parent) :
	ActionWithTargetDialog(i18n("Fix Overlapping Times"), parent)
{
	QGroupBox *settingsGroupBox = createGroupBox(i18nc("@title:group", "Settings"));

	m_minIntervalSpinBox = new QSpinBox(settingsGroupBox);
	m_minIntervalSpinBox->setSuffix(i18n(" msecs"));
	m_minIntervalSpinBox->setMinimum(1);
	m_minIntervalSpinBox->setMaximum(1000);
	m_minIntervalSpinBox->setValue(50);

	QLabel *minIntervalLabel = new QLabel(settingsGroupBox);
	minIntervalLabel->setText(i18n("Minimum interval between lines:"));
	minIntervalLabel->setBuddy(m_minIntervalSpinBox);

	createLineTargetsButtonGroup();

	QGridLayout *settingsLayout = createLayout(settingsGroupBox);
	settingsLayout->addWidget(minIntervalLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	settingsLayout->addWidget(m_minIntervalSpinBox, 0, 1);
}

Time
FixOverlappingTimesDialog::minimumInterval() const
{
	return Time(m_minIntervalSpinBox->value());
}

void
FixOverlappingTimesDialog::setMinimumInterval(const Time &time)
{
	m_minIntervalSpinBox->setValue(time.toMillis());
}
