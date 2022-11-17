/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef INSERTLINEDIALOG_H
#define INSERTLINEDIALOG_H

#include "actionwithtargetdialog.h"

namespace SubtitleComposer {
class InsertLineDialog : public ActionWithTextsTargetDialog
{
public:
	explicit InsertLineDialog(bool insertAfter, QWidget *parent = 0);
};
}
#endif
