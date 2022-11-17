/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "removelinesdialog.h"

using namespace SubtitleComposer;

RemoveLinesDialog::RemoveLinesDialog(QWidget *parent)
	: ActionWithTextsTargetDialog(i18n("Remove Selected Lines"), i18n("Remove From"), parent)
{
	setNonTranslationModeTarget(Both);
}
