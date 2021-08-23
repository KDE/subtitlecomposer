/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2018 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "smarttextsadjustdialog.h"

#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QSpinBox>

using namespace SubtitleComposer;

SmartTextsAdjustDialog::SmartTextsAdjustDialog(unsigned minLengthForLineBreak, QWidget *parent) :
	ActionWithTargetDialog(i18n("Break Lines"), parent)
{
	QGroupBox *settingsGroupBox = createGroupBox(i18nc("@title:group", "Settings"));

	m_minLengthForLineBreakSpinBox = new QSpinBox(settingsGroupBox);
	m_minLengthForLineBreakSpinBox->setMinimum(0);
	m_minLengthForLineBreakSpinBox->setMaximum(1000);
	m_minLengthForLineBreakSpinBox->setValue(minLengthForLineBreak);
	m_minLengthForLineBreakSpinBox->setSuffix(i18n(" characters"));

	QLabel *minLengthForLineBreakLabel = new QLabel(settingsGroupBox);
	minLengthForLineBreakLabel->setText(i18n("Minimum length for line break:"));
	minLengthForLineBreakLabel->setBuddy(m_minLengthForLineBreakSpinBox);

	createLineTargetsButtonGroup();
	createTextTargetsButtonGroup();

	QGridLayout *settingsLayout = createLayout(settingsGroupBox);
	settingsLayout->addWidget(minLengthForLineBreakLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	settingsLayout->addWidget(m_minLengthForLineBreakSpinBox, 0, 1);
}

unsigned
SmartTextsAdjustDialog::minLengthForLineBreak() const
{
	return m_minLengthForLineBreakSpinBox->value();
}
