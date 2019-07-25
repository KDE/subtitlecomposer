#ifndef SPLITSUBTITLEDIALOG_H
#define SPLITSUBTITLEDIALOG_H

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

#include "actiondialog.h"
#include "core/time.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
class TimeEdit;

namespace SubtitleComposer {
class SplitSubtitleDialog : public ActionDialog
{
	Q_OBJECT

public:
	SplitSubtitleDialog(QWidget *parent = 0);

	Time splitTime() const;
	bool shiftNewSubtitle() const;

	void show() override;

private slots:
	void setSplitTimeFromVideo();

private:
	TimeEdit *m_splitTimeEdit;
	QPushButton *m_splitTimeFromVideoButton;
	QCheckBox *m_shiftNewSubtitleCheckBox;
};
}
#endif
