/*
 * Copyright (C) 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * Copyright (C) 2010-2019 Mladen Milinkovic <max@smoothware.net>
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

#include "selectablesubtitledialog.h"
#include "application.h"

#include <QTextCodec>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>
#include <QIcon>

#include <KUrlCompletion>
#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>

using namespace SubtitleComposer;

SelectableSubtitleDialog::SelectableSubtitleDialog(const QString &title, QWidget *parent)
	: ActionWithTargetDialog(title, parent)
{}

QGroupBox *
SelectableSubtitleDialog::createSubtitleGroupBox(const QString &title, bool addToLayout)
{
	m_subtitleGroupBox = createGroupBox(title, addToLayout);

	m_subtitleUrlLineEdit = new KLineEdit(m_subtitleGroupBox);
	m_subtitleUrlLineEdit->setCompletionObject(new KUrlCompletion());

	QLabel *subtitlePathLabel = new QLabel(m_subtitleGroupBox);
	subtitlePathLabel->setText(i18n("Path:"));
	subtitlePathLabel->setBuddy(m_subtitleUrlLineEdit);

	QPushButton *subtitleButton = new QPushButton(m_subtitleGroupBox);
	subtitleButton->setIcon(QIcon::fromTheme("document-open"));
	subtitleButton->setToolTip(i18n("Select subtitle"));
	int buttonSize = subtitleButton->sizeHint().height();
	subtitleButton->setFixedSize(buttonSize, buttonSize);

	connect(subtitleButton, SIGNAL(clicked()), SLOT(selectSubtitle()));

	m_subtitleEncodingComboBox = new KComboBox(m_subtitleGroupBox);
	m_subtitleEncodingComboBox->addItem(i18n("Autodetect"));
	m_subtitleEncodingComboBox->addItems(app()->availableEncodingNames());
	m_subtitleEncodingComboBox->setCurrentIndex(0);

	QLabel *subtitleEncodingLabel = new QLabel(m_subtitleGroupBox);
	subtitleEncodingLabel->setText(i18n("Encoding:"));
	subtitleEncodingLabel->setBuddy(m_subtitleEncodingComboBox);

	QHBoxLayout *subtitlePathLayout = new QHBoxLayout();
	subtitlePathLayout->addWidget(m_subtitleUrlLineEdit, 2);
	subtitlePathLayout->addWidget(subtitleButton);

	QHBoxLayout *subtitleEncodingLayout = new QHBoxLayout();
	subtitleEncodingLayout->addWidget(m_subtitleEncodingComboBox);
	subtitleEncodingLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

	m_subtitleLayout = createLayout(m_subtitleGroupBox);
	m_subtitleLayout->setColumnStretch(1, 2);
	m_subtitleLayout->addWidget(subtitlePathLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	m_subtitleLayout->addLayout(subtitlePathLayout, 0, 1, 1, 2);
	m_subtitleLayout->addWidget(subtitleEncodingLabel, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	m_subtitleLayout->addLayout(subtitleEncodingLayout, 1, 1);

	return m_subtitleGroupBox;
}

void
SelectableSubtitleDialog::selectSubtitle()
{
	/*
	OpenSubtitleDialog openDlg(true, subtitleUrl().isEmpty() ? app()->lastSubtitleDirectory() : subtitleUrl(), subtitleEncoding());

	if(openDlg.exec() == QDialog::Accepted) {
		m_subtitleUrlLineEdit->setText(openDlg.selectedUrl().toLocalFile());
		if(openDlg.selectedEncoding().isEmpty())
			m_subtitleEncodingComboBox->setCurrentItem(i18n("Autodetect"));
		else
			m_subtitleEncodingComboBox->setCurrentItem(openDlg.selectedEncoding().toUpper());
	}*/
}

QUrl
SelectableSubtitleDialog::subtitleUrl() const
{
	return QUrl(m_subtitleUrlLineEdit->text());
}

QString
SelectableSubtitleDialog::subtitleEncoding() const
{
	if(m_subtitleEncodingComboBox->currentIndex() == 0)
		return QString();
	else
		return m_subtitleEncodingComboBox->currentText();
}


