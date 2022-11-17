/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fixpunctuationdialog.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>

using namespace SubtitleComposer;

FixPunctuationDialog::FixPunctuationDialog(QWidget *parent) :
	ActionWithTargetDialog(i18n("Fix Punctuation"), parent)
{
	QGroupBox *settingsGroupBox = createGroupBox(i18nc("@title:group", "Settings"));

	m_spacesCheckBox = new QCheckBox(settingsGroupBox);
	m_spacesCheckBox->setText(i18n("Fix and cleanup spaces"));
	m_spacesCheckBox->setChecked(true);

	m_quotesCheckBox = new QCheckBox(settingsGroupBox);
	m_quotesCheckBox->setText(i18n("Fix quotes and double quotes"));
	m_quotesCheckBox->setChecked(true);

	m_englishICheckBox = new QCheckBox(settingsGroupBox);
	m_englishICheckBox->setText(i18n("Fix English 'I' pronoun"));
	m_englishICheckBox->setChecked(true);

	m_ellipsisCheckBox = new QCheckBox(settingsGroupBox);
	m_ellipsisCheckBox->setText(i18n("Add ellipsis indicating non finished lines"));
	m_ellipsisCheckBox->setChecked(true);

	createLineTargetsButtonGroup();
	createTextTargetsButtonGroup();

	QGridLayout *settingsLayout = createLayout(settingsGroupBox);
	settingsLayout->addWidget(m_spacesCheckBox, 0, 0);
	settingsLayout->addWidget(m_quotesCheckBox, 1, 0);
	settingsLayout->addWidget(m_englishICheckBox, 2, 0);
	settingsLayout->addWidget(m_ellipsisCheckBox, 3, 0);
}

bool
FixPunctuationDialog::spaces() const
{
	return m_spacesCheckBox->isChecked();
}

bool
FixPunctuationDialog::quotes() const
{
	return m_quotesCheckBox->isChecked();
}

bool
FixPunctuationDialog::englishI() const
{
	return m_englishICheckBox->isChecked();
}

bool
FixPunctuationDialog::ellipsis() const
{
	return m_ellipsisCheckBox->isChecked();
}
