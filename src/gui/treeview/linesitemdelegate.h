#ifndef LINESITEMDELEGATE_H
#define LINESITEMDELEGATE_H

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

#include <QStyledItemDelegate>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace SubtitleComposer {
class LinesWidget;

class LinesItemDelegate : public QStyledItemDelegate
{
public:
	typedef enum {
		NoHint = QAbstractItemDelegate::NoHint,
		EditNextItem = QAbstractItemDelegate::EditNextItem,
		EditPreviousItem = QAbstractItemDelegate::EditPreviousItem,
		SubmitModelCache = QAbstractItemDelegate::SubmitModelCache,
		RevertModelCache = QAbstractItemDelegate::RevertModelCache,
		EditUpperItem,
		EditLowerItem,
	} ExtendedEditHint;

	LinesItemDelegate(LinesWidget *parent);
	virtual ~LinesItemDelegate();

	inline LinesWidget * linesWidget() const { return qobject_cast<LinesWidget *>(parent()); }

	QString displayText(const QVariant &value, const QLocale &locale) const override;
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;

protected:
	bool eventFilter(QObject *object, QEvent *event) override;

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
}

#endif // LINESITEMDELEGATE_H
