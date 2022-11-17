/*
    SPDX-FileCopyrightText: 2017-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "vobsubinputinitdialog.h"
#include "ui_vobsubinputinitdialog.h"

using namespace SubtitleComposer;

VobSubInputInitDialog::VobSubInputInitDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::VobSubInputInitDialog)
{
	ui->setupUi(this);
}

VobSubInputInitDialog::~VobSubInputInitDialog()
{
	delete ui;
}

void
VobSubInputInitDialog::streamListSet(const QStringList streams)
{
	ui->comboStream->addItems(streams);
}

int
VobSubInputInitDialog::streamIndex() const
{
	return ui->comboStream->currentIndex();
}

quint32
VobSubInputInitDialog::postProcessingFlags() const
{
	quint32 flags = 0;

	if(ui->ppAposQuote->isChecked())
		flags |= APOSTROPHE_TO_QUOTES;
	if(ui->ppSpacePunct->isChecked())
		flags |= SPACE_PUNCTUATION;
	if(ui->ppSpaceNumber->isChecked())
		flags |= SPACE_NUMBERS;
	if(ui->ppSpaceParen->isChecked())
		flags |= SPACE_PARENTHESES;
	if(ui->ppCharsOCR->isChecked())
		flags |= CHARS_OCR;

	return flags;
}
