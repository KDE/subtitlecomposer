/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lineswidget.h"

#include "appglobal.h"
#include "application.h"
#include "core/richtext/richdocument.h"
#include "actions/useractionnames.h"
#include "dialogs/actionwithtargetdialog.h"
#include "gui/treeview/linesitemdelegate.h"
#include "gui/treeview/linesselectionmodel.h"

#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

#include <KConfigGroup>
#include <KSharedConfig>


#define RESTORE_STATE false

using namespace SubtitleComposer;

LinesWidget::LinesWidget(QWidget *parent)
	: TreeView(parent),
	  m_scrollFollowsModel(true),
	  m_translationMode(false),
	  m_showingContextMenu(false),
	  m_itemsDelegate(new LinesItemDelegate(this)),
	  m_inlineEditor(nullptr)
{
	setModel(new LinesModel(this));
	selectionModel()->deleteLater();
	setSelectionModel(new LinesSelectionModel(model()));

	for(int column = 0, columnCount = model()->columnCount(); column < columnCount; ++column)
		setItemDelegateForColumn(column, m_itemsDelegate);

	QHeaderView *hdr = header();
	hdr->setSectionsClickable(false);
	hdr->setSectionsMovable(false);
	updateHeader();

	setUniformRowHeights(true);
	setItemsExpandable(false);
	setAutoExpandDelay(-1);
	setExpandsOnDoubleClick(false);
	setRootIsDecorated(false);
	setAllColumnsShowFocus(true);
	setSortingEnabled(false);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);

	m_gridPen.setColor(palette().mid().color().lighter(120));

	setAcceptDrops(true);
	viewport()->installEventFilter(this);

	connect(selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LinesWidget::onCurrentRowChanged);
}

LinesWidget::~LinesWidget()
{
}

void
LinesWidget::updateHeader()
{
	QHeaderView *hdr = header();
	for(int i = 0; i < LinesModel::ColumnCount; i++) {
		switch(i) {
		case LinesModel::Text:
			hdr->setSectionResizeMode(i, QHeaderView::Stretch);
			hdr->setSectionHidden(i, false);
			break;
		case LinesModel::Translation:
			hdr->setSectionResizeMode(i, QHeaderView::Stretch);
			hdr->setSectionHidden(i, !m_translationMode);
			break;
		default:
			hdr->setSectionResizeMode(i, QHeaderView::ResizeToContents);
			hdr->setSectionHidden(i, false);
			break;
		}
	}
	// do this last so columns get autosized
	hdr->setSectionResizeMode(LinesModel::Text, QHeaderView::Interactive);
	hdr->setSectionResizeMode(LinesModel::Translation, QHeaderView::Interactive);
	// give some small size to last column, it will stretch anyways
	hdr->resizeSection(LinesModel::Translation, 100);
}

void
LinesWidget::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
	auto ehint = LinesItemDelegate::ExtendedEditHint(hint);
	if(ehint == LinesItemDelegate::NoHint || ehint == LinesItemDelegate::SubmitModelCache || ehint == LinesItemDelegate::RevertModelCache) {
		TreeView::closeEditor(editor, hint);
	} else {
		QModelIndex index = currentIndex();

		switch(ehint) {
		case LinesItemDelegate::EditPreviousItem:
			if(m_translationMode) {
				if(index.column() == LinesModel::Translation)
					index = index.sibling(index.row(), LinesModel::Text);
				else if(index.row())
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
					index = index.sibling(moveCursor(MoveUp, Qt::NoModifier).row(), LinesModel::Translation);
#else
					index = moveCursor(MoveUp, Qt::NoModifier).siblingAtColumn(LinesModel::Translation);
#endif
				break;
			}
			// fallthrough
		case LinesItemDelegate::EditUpperItem:
			index = moveCursor(MoveUp, Qt::NoModifier);
			break;
		case LinesItemDelegate::EditNextItem:
			if(m_translationMode) {
				if(index.column() == LinesModel::Text)
					index = index.sibling(index.row(), LinesModel::Translation);
				else if(index.row() < model()->rowCount() - 1)
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
					index = index.sibling(moveCursor(MoveDown, Qt::NoModifier).row(), LinesModel::Text);
#else
					index = moveCursor(MoveDown, Qt::NoModifier).siblingAtColumn(LinesModel::Text);
#endif
				break;
			}
			// fallthrough
		case LinesItemDelegate::EditLowerItem:
			index = moveCursor(MoveDown, Qt::NoModifier);
			break;
		default: break;
		}

		QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::NoUpdate;
		if(selectionMode() != NoSelection) {
			flags = QItemSelectionModel::ClearAndSelect;
			switch(selectionBehavior()) {
			case SelectItems: break;
			case SelectRows: flags |= QItemSelectionModel::Rows; break;
			case SelectColumns: flags |= QItemSelectionModel::Columns; break;
			}
		}

		TreeView::closeEditor(editor, QAbstractItemDelegate::NoHint);

		QPersistentModelIndex persistent(index);
		selectionModel()->setCurrentIndex(persistent, flags);

		// currentChanged signal would have already started editing
		if((index.flags() & Qt::ItemIsEditable) && !(editTriggers() & QAbstractItemView::CurrentChanged))
			edit(persistent);
	}
}

void
LinesWidget::editCurrentLineInPlace(bool primaryText)
{
	QModelIndex curIdx = currentIndex();
	if(curIdx.isValid()) {
		const int col = primaryText || !m_translationMode ? LinesModel::Text : LinesModel::Translation;
		if(col != curIdx.column()) {
			curIdx = model()->index(curIdx.row(), col);
			setCurrentIndex(curIdx);
		}
		edit(curIdx);
	}
}

bool
LinesWidget::showingContextMenu()
{
	return m_showingContextMenu;
}

SubtitleLine *
LinesWidget::currentLine() const
{
	return qobject_cast<LinesSelectionModel *>(selectionModel())->currentLine();
}

int
LinesWidget::currentLineIndex() const
{
	const QModelIndex idx = currentIndex();
	return idx.isValid() ? idx.row() : -1;
}

int
LinesWidget::firstSelectedIndex() const
{
	int row = -1;
	const QItemSelection &selection = selectionModel()->selection();
	for(const QItemSelectionRange &r : selection) {
		if(row == -1 || r.top() < row)
			row = r.top();
	}
	return row;
}

int
LinesWidget::lastSelectedIndex() const
{
	int row = -1;
	const QItemSelection &selection = selectionModel()->selection();
	for(const QItemSelectionRange &r : selection) {
		if(r.bottom() > row)
			row = r.bottom();
	}
	return row;
}

bool
LinesWidget::selectionHasMultipleRanges() const
{
	const QItemSelection &selection = selectionModel()->selection();
	return selection.size() > 1;
}

RangeList
LinesWidget::selectionRanges() const
{
	RangeList ranges;

	const QItemSelection &selection = selectionModel()->selection();
	for(const QItemSelectionRange &r : selection) {
		ranges << Range(r.top(), r.bottom());
	}

	return ranges;
}

RangeList
LinesWidget::targetRanges(int target) const
{
	switch(target) {
	case ActionWithTargetDialog::AllLines:
		return Range::full();
	case ActionWithTargetDialog::Selection: return selectionRanges();
	case ActionWithTargetDialog::FromSelected: {
		int index = firstSelectedIndex();
		return index < 0 ? RangeList() : Range::upper(index);
	}
	case ActionWithTargetDialog::UpToSelected: {
		int index = lastSelectedIndex();
		return index < 0 ? RangeList() : Range::lower(index);
	}
	default:
		return RangeList();
	}
}

void
LinesWidget::loadConfig()
{
#if RESTORE_STATE
	KConfigGroup group(KSharedConfig::openConfig()->group("LinesWidget Settings"));

	QByteArray state;
	QStringList strState = group.readXdgListEntry("Columns State", QStringList() << QString());
	for(QStringList::ConstIterator it = strState.constBegin(), end = strState.constEnd(); it != end; ++it)
		state.append(char(it->toInt()));
	header()->restoreState(state);
#endif
}

void
LinesWidget::saveConfig()
{
	KConfigGroup group(KSharedConfig::openConfig()->group("LinesWidget Settings"));

	QStringList strState;
	QByteArray state = header()->saveState();
	for(int index = 0, size = state.size(); index < size; ++index)
		strState.append(QString::number(state[index]));
	group.writeXdgListEntry("Columns State", strState);
}

void
LinesWidget::setSubtitle(Subtitle *subtitle)
{
	model()->setSubtitle(subtitle);
}

void
LinesWidget::setTranslationMode(bool enabled)
{
	if(m_translationMode == enabled)
		return;

	m_translationMode = enabled;
	updateHeader();
}

void
LinesWidget::setCurrentLine(SubtitleLine *line, bool clearSelection)
{
	if(!line)
		return;

	selectionModel()->setCurrentIndex(model()->index(line->index(), 0),
									  QItemSelectionModel::Select | QItemSelectionModel::Rows
									  | (clearSelection ? QItemSelectionModel::Clear : QItemSelectionModel::NoUpdate));
}

void
LinesWidget::setPlayingLine(SubtitleLine *line)
{
	model()->setPlayingLine(line);
}

void
LinesWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QModelIndex index = indexAt(viewport()->mapFromGlobal(e->globalPos()));
#else
	QModelIndex index = indexAt(viewport()->mapFromGlobal(e->globalPosition()).toPoint());
#endif
	if(index.isValid())
		emit lineDoubleClicked(model()->subtitle()->line(index.row()));
}

bool
LinesWidget::eventFilter(QObject *object, QEvent *event)
{
	if(object == viewport()) {
		switch(event->type()) {
		case QEvent::DragEnter:
		case QEvent::Drop:
			foreach(const QUrl &url, static_cast<QDropEvent *>(event)->mimeData()->urls()) {
				if(url.scheme() == QLatin1String("file")) {
					event->accept();
					if(event->type() == QEvent::Drop) {
						app()->openSubtitle(url);
					}
					return true; // eat event
				}
			}
			event->ignore();
			return true;

		case QEvent::DragMove:
			return true; // eat event

		default:
			;
		}
	}
	// standard event processing
	return TreeView::eventFilter(object, event);
}

inline static void
addAppAction(QMenu *menu, const char *actionName, bool checkable=false, bool checked=false)
{
	QAction *action = app()->action(actionName);
	if(checkable) {
		action->setCheckable(true);
		action->setChecked(checked);
	}
	menu->addAction(action);
	if(checkable)
		action->setCheckable(false);
}

void
LinesWidget::contextMenuEvent(QContextMenuEvent *e)
{
	SubtitleLine *subLine = static_cast<LinesSelectionModel *>(selectionModel())->currentLine();

	QMenu menu;
	addAppAction(&menu, ACT_SELECT_ALL_LINES);
	addAppAction(&menu, ACT_EDIT_CURRENT_LINE_IN_PLACE);
	menu.addSeparator();
	addAppAction(&menu, ACT_INSERT_BEFORE_CURRENT_LINE);
	addAppAction(&menu, ACT_INSERT_AFTER_CURRENT_LINE);
	addAppAction(&menu, ACT_REMOVE_SELECTED_LINES);
	menu.addSeparator();
	addAppAction(&menu, ACT_JOIN_SELECTED_LINES);
	addAppAction(&menu, ACT_SPLIT_SELECTED_LINES);
	menu.addSeparator();
	addAppAction(&menu, ACT_ANCHOR_TOGGLE);
	addAppAction(&menu, ACT_ANCHOR_REMOVE_ALL);
	menu.addSeparator();

	QMenu textsMenu(i18n("Texts"));
	addAppAction(&textsMenu, ACT_ADJUST_TEXTS);
	addAppAction(&textsMenu, ACT_UNBREAK_TEXTS);
	addAppAction(&textsMenu, ACT_SIMPLIFY_SPACES);
	addAppAction(&textsMenu, ACT_CHANGE_CASE);
	addAppAction(&textsMenu, ACT_FIX_PUNCTUATION);
	addAppAction(&textsMenu, ACT_TRANSLATE);
	textsMenu.addSeparator();
	addAppAction(&textsMenu, ACT_SPELL_CHECK);
	menu.addMenu(&textsMenu);

	QMenu stylesMenu(i18n("Styles"));
	const int styleFlags = subLine ? subLine->primaryDoc()->cummulativeStyleFlags() | subLine->secondaryDoc()->cummulativeStyleFlags() : 0;
	addAppAction(&stylesMenu, ACT_TOGGLE_SELECTED_LINES_BOLD, true, styleFlags & RichString::Bold);
	addAppAction(&stylesMenu, ACT_TOGGLE_SELECTED_LINES_ITALIC, true, styleFlags & RichString::Italic);
	addAppAction(&stylesMenu, ACT_TOGGLE_SELECTED_LINES_UNDERLINE, true, styleFlags & RichString::Underline);
	addAppAction(&stylesMenu, ACT_TOGGLE_SELECTED_LINES_STRIKETHROUGH, true, styleFlags & RichString::StrikeThrough);
	addAppAction(&stylesMenu, ACT_CHANGE_SELECTED_LINES_TEXT_COLOR);
	menu.addMenu(&stylesMenu);

	QMenu timesMenu(i18n("Times"));
	addAppAction(&timesMenu, ACT_SHIFT_SELECTED_LINES_FORWARDS);
	addAppAction(&timesMenu, ACT_SHIFT_SELECTED_LINES_BACKWARDS);
	timesMenu.addSeparator();
	addAppAction(&timesMenu, ACT_SHIFT);
	addAppAction(&timesMenu, ACT_DURATION_LIMITS);
	addAppAction(&timesMenu, ACT_AUTOMATIC_DURATIONS);
	addAppAction(&timesMenu, ACT_MAXIMIZE_DURATIONS);
	addAppAction(&timesMenu, ACT_FIX_OVERLAPPING_LINES);
	menu.addMenu(&timesMenu);

	QMenu errorsMenu(i18n("Errors"));
	addAppAction(&errorsMenu, ACT_TOGGLE_SELECTED_LINES_MARK, true, subLine ? subLine->errorFlags() & SubtitleLine::UserMark : 0);
	errorsMenu.addSeparator();
	addAppAction(&errorsMenu, ACT_DETECT_ERRORS);
	addAppAction(&errorsMenu, ACT_CLEAR_ERRORS);
	menu.addMenu(&errorsMenu);

	m_showingContextMenu = true;
	menu.exec(e->globalPos());
	m_showingContextMenu = false;

	e->ignore();

	TreeView::contextMenuEvent(e);
}

void
LinesWidget::onCurrentRowChanged()
{
	auto sm = qobject_cast<LinesSelectionModel *>(selectionModel());
	emit currentLineChanged(sm->currentLine());
}

void
LinesWidget::drawHorizontalDotLine(QPainter *painter, int x1, int x2, int y)
{
	static int aux;

	if(x1 > x2) {
		aux = x1;
		x1 = x2;
		x2 = aux;
	}

	for(int x = x1 + 1; x <= x2; x += 2)
		painter->drawPoint(x, y);
}

void
LinesWidget::drawVerticalDotLine(QPainter *painter, int x, int y1, int y2)
{
	static int aux;

	if(y1 > y2) {
		aux = y1;
		y1 = y2;
		y2 = aux;
	}

	for(int y = y1 + 1; y <= y2; y += 2)
		painter->drawPoint(x, y);
}

void
LinesWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	TreeView::drawRow(painter, option, index);

	const int visibleColumns = m_translationMode ? LinesModel::ColumnCount : LinesModel::ColumnCount - 1;
	const int row = index.row();
	const bool rowSelected = selectionModel()->isSelected(index);
	const QPalette palette = this->palette();
	const QRect rowRect = QRect(visualRect(model()->index(row, 0)).topLeft(),
								visualRect(model()->index(row, visibleColumns - 1)).bottomRight());

	// draw row grid
	painter->setPen(m_gridPen);
	if(!rowSelected)
		drawHorizontalDotLine(painter, rowRect.left(), rowRect.right(), rowRect.bottom());
	for(int column = 0; column < visibleColumns - 1; ++column) {
		const QRect cellRect = visualRect(model()->index(row, column));
		drawVerticalDotLine(painter, cellRect.right(), rowRect.top(), rowRect.bottom());
	}
	if(index.row() == currentIndex().row()) {
		painter->setPen(palette.windowText().color());

		drawHorizontalDotLine(painter, rowRect.left(), rowRect.right(), rowRect.top());
		drawHorizontalDotLine(painter, rowRect.left(), rowRect.right(), rowRect.bottom());

		drawVerticalDotLine(painter, rowRect.left(), rowRect.top(), rowRect.bottom());
		drawVerticalDotLine(painter, rowRect.right(), rowRect.top(), rowRect.bottom());
	}
}
