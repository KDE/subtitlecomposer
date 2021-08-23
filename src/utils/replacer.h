#ifndef REPLACER_H
#define REPLACER_H

/*
 * SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * SPDX-FileCopyrightText: 2010-2020 Mladen Milinkovic <max@smoothware.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

#include "core/rangelist.h"
#include "core/subtitle.h"
#include "core/subtitleline.h"

#include <QExplicitlySharedDataPointer>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QDialog)
class KReplace;
class KReplaceDialog;

namespace SubtitleComposer {
class SubtitleIterator;

class Replacer : public QObject
{
	Q_OBJECT

public:
	explicit Replacer(QWidget *parent = 0);
	virtual ~Replacer();

	QWidget * parentWidget();

public slots:
	void setSubtitle(Subtitle *subtitle = 0);
	void setTranslationMode(bool enabled);

	void replace(const RangeList &selectionRanges, int currentIndex, const QString &text = QString());

signals:
	void found(SubtitleLine *line, bool primary, int startIndex, int endIndex);

private:
	void invalidate();

	QDialog * replaceNextDialog();

private slots:
	void onFindNext();
	void onHighlight(const QString &text, int matchingIndex, int matchedLength);
	void onReplace(const QString &text, int replacementIndex, int replacedLength, int matchedLength);

private:
	QExplicitlySharedDataPointer<const Subtitle> m_subtitle;
	bool m_translationMode;
	bool m_feedingPrimary;

	KReplace *m_replace;
	KReplaceDialog *m_dialog;
	QGroupBox *m_targetGroupBox;
	QRadioButton *m_targetRadioButtons[SubtitleTargetSize];
	SubtitleIterator *m_iterator;
	int m_firstIndex;
};
}
#endif
