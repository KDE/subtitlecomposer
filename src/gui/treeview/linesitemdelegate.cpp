/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "linesitemdelegate.h"
#include "gui/treeview/lineswidget.h"
#include "gui/treeview/richdocumentptr.h"
#include "gui/treeview/richlineedit.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QTextOption>
#include <QTextBlock>

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
#define horizontalAdvance width
#endif

using namespace SubtitleComposer;

LinesItemDelegate::LinesItemDelegate(LinesWidget *parent)
	: QStyledItemDelegate(parent)
{
}

LinesItemDelegate::~LinesItemDelegate()
{
}

bool
LinesItemDelegate::eventFilter(QObject *object, QEvent *event)
{
	QWidget *editor = qobject_cast<QWidget *>(object);
	if(!editor)
		return false;

	if(event->type() == QEvent::KeyPress) {
		switch(static_cast<QKeyEvent *>(event)->key()) {
		case Qt::Key_Tab:
			emit commitData(editor);
			emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
			return true;

		case Qt::Key_Backtab:
			emit commitData(editor);
			emit closeEditor(editor, QAbstractItemDelegate::EditPreviousItem);
			return true;

		case Qt::Key_Up:
			emit commitData(editor);
			emit closeEditor(editor, QAbstractItemDelegate::EndEditHint(EditUpperItem));
			return true;

		case Qt::Key_Down:
			emit commitData(editor);
			emit closeEditor(editor, QAbstractItemDelegate::EndEditHint(EditLowerItem));
			return true;

		case Qt::Key_Enter:
		case Qt::Key_Return:
			if(static_cast<QKeyEvent *>(event)->modifiers()) {
				emit commitData(editor);
				emit closeEditor(editor, QAbstractItemDelegate::EndEditHint(EditLowerItem));
				return true;
			}
			if(qobject_cast<RichLineEdit *>(object))
				return false;
			break;

		default:
			break;
		}
	}

	return QStyledItemDelegate::eventFilter(object, event);
}

static const QIcon & markIcon() { static QIcon icon = QIcon::fromTheme(QStringLiteral("dialog-warning")); return icon; }
static const QIcon & errorIcon() { static QIcon icon = QIcon::fromTheme(QStringLiteral("dialog-error")); return icon; }
static const QIcon & anchorIcon() { static QIcon icon = QIcon::fromTheme(QStringLiteral("anchor")); return icon; }

inline static bool isRichDoc(const QModelIndex &index) { return index.column() >= LinesModel::Text; }

static void
drawTextPrimitive(QPainter *painter, const QStyle *style, const QStyleOptionViewItem &option, const QRect &rect, QPalette::ColorGroup cg, const QModelIndex &index)
{
	const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;
	Qt::Alignment alignment = QStyle::visualAlignment(option.direction, option.displayAlignment);

	QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0);  // remove width padding

	if(index.column() == LinesModel::ShowTime && index.data(LinesModel::AnchoredRole).toInt() == -1)
		cg = QPalette::Disabled;

	QColor textColor;
	if(option.state & QStyle::State_Selected)
		textColor = option.palette.color(cg, QPalette::HighlightedText);
	else
		textColor = option.palette.color(cg, QPalette::Text);

	painter->setPen(textColor);

	QString text = option.fontMetrics.elidedText(option.text, option.textElideMode, textRect.width());

	if(index.column() == LinesModel::Number && index.data(LinesModel::AnchoredRole).toInt() == 1) {
		int iconSize = qMin(textRect.width(), textRect.height()) - 3;
		QRect iconRect = QRect(textRect.right() - iconSize - 2, textRect.y() + 1, iconSize, iconSize);

		QIcon::Mode mode = QIcon::Normal;
		if(!(option.state & QStyle::State_Enabled))
			mode = QIcon::Disabled;
		else if(option.state & QStyle::State_Selected)
			mode = QIcon::Selected;
		QIcon::State state = option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

		textRect.setRight(textRect.right() - iconSize);

		anchorIcon().paint(painter, iconRect, option.decorationAlignment, mode, state);
	}

	if(index.column() && index.column() < LinesModel::Text) {
		int o = 0;
		// find start of non zero time
		for(int i = 0; ; i++) {
			if(i == text.length()) {
				o = i;
				break;
			}
			const QChar &ch = text.at(i);
			if(ch != QChar('0')) {
				if(ch != QChar(':') && ch != QChar('.'))
					break;
				o = i + 1;
			}
		}
		if(o) {
			// fix rect based on alignment
			if(alignment & Qt::AlignRight) {
				alignment = (alignment & ~Qt::AlignRight) | Qt::AlignLeft;
				textRect.setLeft(textRect.right() - painter->fontMetrics().horizontalAdvance(text));
			} else if(alignment & Qt::AlignHCenter) {
				alignment = (alignment & ~Qt::AlignHCenter) | Qt::AlignLeft;
				const int w = painter->fontMetrics().horizontalAdvance(text);
				textRect.setLeft(textRect.left() + (textRect.width() - w) / 2);
				textRect.setRight(textRect.left() + w);
			}
			// draw zero time semi-transparent
			const QString sub = text.left(o);
			text.remove(0, o);
			QColor altColor = textColor;
			altColor.setAlpha(textColor.alpha() * 2 / 5);
			painter->setPen(altColor);
			painter->drawText(textRect, alignment, sub);
			painter->setPen(textColor);
			textRect.setLeft(textRect.left() + painter->fontMetrics().horizontalAdvance(sub));
		}
	}
	painter->drawText(textRect, alignment, text);
}

static void
drawRichText(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect)
{
	painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	const RichDocument *doc = option.index.data(Qt::DisplayRole).value<RichDocumentPtr>();
	RichDocumentLayout *docLayout = doc->documentLayout();

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;
	painter->setPen(option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text));

	const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
	int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;

	QTextOption textOption;
	textOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
	textOption.setFlags(QTextOption::IncludeTrailingSpaces);
	textOption.setWrapMode(QTextOption::NoWrap);
	textOption.setTextDirection(option.direction);

	const QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0);
	qreal xOff = textRect.left();

	// prepare line seprator
	const qreal sepWidth = qreal(textRect.height()) / 2.;
	docLayout->separatorResize(QSizeF(sepWidth, textRect.height()));

	// layout and draw text
	for(QTextBlock bi = doc->begin(); bi != doc->end(); bi = bi.next()) {
		QTextLayout bl;
		bl.setCacheEnabled(true);
		bl.setFont(option.font);
		bl.setTextOption(textOption);
		QString text = bi.text() + QChar(QChar::LineSeparator);
		// replace certain non-printable characters with spaces (to avoid drawing boxes
		// when using fonts that don't have glyphs for such characters)
		QChar *uc = text.data();
		for(int i = 0; i < (int)text.length(); ++i) {
			if((uc[i].unicode() < 0x20 && uc[i].unicode() != 0x09)
			|| uc[i] == QChar::LineSeparator
			|| uc[i] == QChar::ParagraphSeparator
			|| uc[i] == QChar::ObjectReplacementCharacter)
				uc[i] = QChar(QChar::Space);
		}
		bl.setText(text);
		bl.setFormats(docLayout->applyCSS(bi.textFormats()));
		bl.beginLayout();
		for(;;) {
			QTextLine line = bl.createLine();
			if(!line.isValid())
				break;
			line.setLeadingIncluded(true);
			line.setLineWidth(10000);
			line.setPosition(QPointF(xOff, textRect.top() + (qreal(textRect.height()) - line.height()) / 2.));
			const int w = line.naturalTextWidth();
			xOff += w + sepWidth;
			line.setLineWidth(w);
		}
		bl.endLayout();

		const int n = bl.lineCount();
		for(int i = 0; i < n; i++) {
			const QTextLine &tl = bl.lineAt(i);
			tl.draw(painter, QPointF());
			docLayout->separatorDraw(painter, QPointF(tl.position().x() - sepWidth, tl.position().y() - tl.descent()));
		}
	}
}

void
LinesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
	QStyleOptionViewItem option = opt;
	initStyleOption(&option, index);

	const QWidget *widget = option.widget;
	const QStyle *style = widget ? widget->style() : QApplication::style();

	painter->save();
	painter->setClipRect(option.rect);

	QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;

	// draw the background
	if(index.data(LinesModel::PlayingLineRole).toBool()) {
		const bool sel = option.state & QStyle::State_Selected;
		QColor bg = option.palette.color(cg, sel ? QPalette::Highlight : QPalette::Base);
		QColor fg = option.palette.color(cg, sel ? QPalette::HighlightedText : QPalette::Text);
		const int ld = fg.value() - bg.value();
		if(ld > 0)
			bg = bg.lighter(100 + ld / 2);
		else
			bg = bg.darker(100 + ld / -9);
		option.palette.setColor(cg, QPalette::Highlight, bg);
		option.state |= QStyle::State_Selected;
	}
	style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

	// draw the icon(s)
	const bool showMarkedIcon = index.data(LinesModel::MarkedRole).toBool();
	const bool showErrorIcon = index.data(LinesModel::ErrorRole).toBool();

	if(showMarkedIcon || showErrorIcon) {
		int iconSize = qMin(textRect.width(), textRect.height()) - 3;
		QRect iconRect = QRect(textRect.x() + 2, textRect.y() + 1, iconSize, iconSize);

		QIcon::Mode mode = QIcon::Normal;
		if(!(option.state & QStyle::State_Enabled))
			mode = QIcon::Disabled;
		else if(option.state & QStyle::State_Selected)
			mode = QIcon::Selected;
		QIcon::State state = option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

		if(showMarkedIcon) {
			markIcon().paint(painter, iconRect, option.decorationAlignment, mode, state);
			textRect.setX(textRect.x() + iconSize + 2);
			if(showErrorIcon)
				iconRect.translate(iconSize + 2, 0);
		}

		if(showErrorIcon) {
			errorIcon().paint(painter, iconRect, option.decorationAlignment, mode, state);
			textRect.setX(textRect.x() + iconSize + 2);
		}
	}
	// draw the text
	if(isRichDoc(index) || !option.text.isEmpty()) {
		if(option.state & QStyle::State_Editing) {
			painter->setPen(option.palette.color(cg, QPalette::Text));
			painter->drawRect(textRect.adjusted(0, 0, -1, -1));
		} else if(isRichDoc(index)) {
			drawRichText(painter, option, textRect);
		} else {
			drawTextPrimitive(painter, style, option, textRect, cg, index);
		}
	}
	// draw the focus rect
	if(option.state & QStyle::State_HasFocus) {
		QStyleOptionFocusRect frOption;
		frOption.QStyleOption::operator=(option);
		frOption.rect = style->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option, widget);
		frOption.state |= QStyle::State_KeyboardFocusChange;
		frOption.state |= QStyle::State_Item;
		frOption.backgroundColor = option.palette.color((option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);

		style->drawPrimitive(QStyle::PE_FrameFocusRect, &frOption, painter, widget);
	}

	painter->restore();
}

QString
LinesItemDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	if(value.userType() == qMetaTypeId<RichDocumentPtr>())
		return QString();
	return QStyledItemDelegate::displayText(value, locale);
}

QWidget *
LinesItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if(isRichDoc(index)) {
		RichLineEdit *ed = new RichLineEdit(option, parent);
		linesWidget()->m_inlineEditor = ed;
		connect(ed, &QObject::destroyed, this, [this, ed](){
			if(linesWidget()->m_inlineEditor == ed)
				linesWidget()->m_inlineEditor = nullptr;
		});
		return ed;
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void
LinesItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(isRichDoc(index)) {
		RichDocument *doc = index.data(Qt::EditRole).value<RichDocumentPtr>();
		if(!doc)
			return;
		RichLineEdit *edit = static_cast<RichLineEdit *>(editor);
		if(edit->document() != doc)
			edit->setDocument(doc);
	} else {
		QStyledItemDelegate::setEditorData(editor, index);
	}
}
