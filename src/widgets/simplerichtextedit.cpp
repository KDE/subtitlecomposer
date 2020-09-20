/*
 * Copyright (C) 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * Copyright (C) 2010-2018 Mladen Milinkovic <max@smoothware.net>
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

#include "simplerichtextedit.h"

#include "dialogs/subtitlecolordialog.h"

#include <QRegExp>
#include <QEvent>
#include <QMenu>
#include <QShortcutEvent>
#include <QContextMenuEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QAction>
#include <QIcon>
#include <QDebug>

#include <KStandardShortcut>
#include <KLocalizedString>

SimpleRichTextEdit::SimpleRichTextEdit(QWidget *parent)
	: KTextEdit(parent)
{
	enableFindReplace(false);
	setCheckSpellingEnabled(true);

	setAutoFormatting(KTextEdit::AutoNone);
	setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

	QTextDocument *textDocument = document();

	QTextOption textOption;
	textOption.setAlignment(Qt::AlignCenter);
	textOption.setWrapMode(QTextOption::NoWrap);
	textDocument->setDefaultTextOption(textOption);

	QFont defaultFont = font();
	defaultFont.setPointSize(defaultFont.pointSize() + 2);
	textDocument->setDefaultFont(defaultFont);

	QString styleSheet("p {" "   display: block;" "   white-space: pre;" "   margin-top: 0px;" "   margin-bottom: 0px;" "}");
	textDocument->setDefaultStyleSheet(styleSheet);

	setTextInteractionFlags(Qt::TextEditorInteraction);

	m_actions[Undo] = new QAction(this);
	m_actions[Undo]->setIcon(QIcon::fromTheme("edit-undo"));
	m_actions[Undo]->setText(i18n("Undo"));
	m_actions[Undo]->setShortcuts(KStandardShortcut::undo());
	connect(m_actions[Undo], &QAction::triggered, this, &QTextEdit::undo);

	m_actions[Redo] = new QAction(this);
	m_actions[Redo]->setIcon(QIcon::fromTheme("edit-redo"));
	m_actions[Redo]->setText(i18n("Redo"));
	m_actions[Redo]->setShortcuts(KStandardShortcut::redo());
	connect(m_actions[Redo], &QAction::triggered, this, &QTextEdit::redo);

	m_actions[Cut] = new QAction(this);
	m_actions[Cut]->setIcon(QIcon::fromTheme("edit-cut"));
	m_actions[Cut]->setText(i18n("Cut"));
	m_actions[Cut]->setShortcuts(KStandardShortcut::cut());
	connect(m_actions[Cut], &QAction::triggered, this, &QTextEdit::cut);

	m_actions[Copy] = new QAction(this);
	m_actions[Copy]->setIcon(QIcon::fromTheme("edit-copy"));
	m_actions[Copy]->setText(i18n("Copy"));
	m_actions[Copy]->setShortcuts(KStandardShortcut::copy());
	connect(m_actions[Copy], &QAction::triggered, this, &QTextEdit::copy);

#if !defined(QT_NO_CLIPBOARD)
	m_actions[Paste] = new QAction(this);
	m_actions[Paste]->setIcon(QIcon::fromTheme("edit-paste"));
	m_actions[Paste]->setText(i18n("Paste"));
	m_actions[Paste]->setShortcuts(KStandardShortcut::paste());
	connect(m_actions[Paste], &QAction::triggered, this, &QTextEdit::paste);
#endif

	m_actions[Delete] = new QAction(this);
	m_actions[Delete]->setIcon(QIcon::fromTheme("edit-delete"));
	m_actions[Delete]->setText(i18n("Delete"));
	m_actions[Delete]->setShortcut(QKeySequence::Delete);
	connect(m_actions[Delete], &QAction::triggered, this, &SimpleRichTextEdit::deleteText);

	m_actions[Clear] = new QAction(this);
	m_actions[Clear]->setIcon(QIcon::fromTheme("edit-clear"));
	m_actions[Clear]->setText(i18nc("@action:inmenu Clear all text", "Clear"));
	connect(m_actions[Clear], &QAction::triggered, this, &SimpleRichTextEdit::undoableClear);

	m_actions[SelectAll] = new QAction(this);
	m_actions[SelectAll]->setIcon(QIcon::fromTheme("edit-select-all"));
	m_actions[SelectAll]->setText(i18n("Select All"));
	m_actions[SelectAll]->setShortcut(QKeySequence::SelectAll);
	connect(m_actions[SelectAll], &QAction::triggered, this, &QTextEdit::selectAll);

	m_actions[ToggleBold] = new QAction(this);
	m_actions[ToggleBold]->setIcon(QIcon::fromTheme("format-text-bold"));
	m_actions[ToggleBold]->setText(i18nc("@action:inmenu Toggle bold style", "Bold"));
	m_actions[ToggleBold]->setShortcut(QKeySequence("Ctrl+B"));
	connect(m_actions[ToggleBold], &QAction::triggered, this, &SimpleRichTextEdit::toggleFontBold);

	m_actions[ToggleItalic] = new QAction(this);
	m_actions[ToggleItalic]->setIcon(QIcon::fromTheme("format-text-italic"));
	m_actions[ToggleItalic]->setText(i18nc("@action:inmenu Toggle italic style", "Italic"));
	m_actions[ToggleItalic]->setShortcut(QKeySequence("Ctrl+I"));
	connect(m_actions[ToggleItalic], &QAction::triggered, this, &SimpleRichTextEdit::toggleFontItalic);

	m_actions[ToggleUnderline] = new QAction(this);
	m_actions[ToggleUnderline]->setIcon(QIcon::fromTheme("format-text-underline"));
	m_actions[ToggleUnderline]->setText(i18nc("@action:inmenu Toggle underline style", "Underline"));
	m_actions[ToggleUnderline]->setShortcut(QKeySequence("Ctrl+U"));
	connect(m_actions[ToggleUnderline], &QAction::triggered, this, &SimpleRichTextEdit::toggleFontUnderline);

	m_actions[ToggleStrikeOut] = new QAction(this);
	m_actions[ToggleStrikeOut]->setIcon(QIcon::fromTheme("format-text-strikethrough"));
	m_actions[ToggleStrikeOut]->setText(i18nc("@action:inmenu Toggle strike through style", "Strike Through"));
	m_actions[ToggleStrikeOut]->setShortcut(QKeySequence("Ctrl+T"));
	connect(m_actions[ToggleStrikeOut], &QAction::triggered, this, &SimpleRichTextEdit::toggleFontStrikeOut);

	m_actions[ChangeTextColor] = new QAction(this);
	m_actions[ChangeTextColor]->setIcon(QIcon::fromTheme("format-text-color"));
	m_actions[ChangeTextColor]->setText(i18nc("@action:inmenu Change Text Color", "Text Color"));
	m_actions[ChangeTextColor]->setShortcut(QKeySequence("Ctrl+Shift+C"));
	connect(m_actions[ChangeTextColor], &QAction::triggered, this, &SimpleRichTextEdit::changeTextColor);

	m_actions[CheckSpelling] = new QAction(this);
	m_actions[CheckSpelling]->setIcon(QIcon::fromTheme("tools-check-spelling"));
	m_actions[CheckSpelling]->setText(i18n("Check Spelling..."));
	connect(m_actions[CheckSpelling], &QAction::triggered, this, &KTextEdit::checkSpelling);

	m_actions[ToggleAutoSpellChecking] = new QAction(this);
	m_actions[ToggleAutoSpellChecking]->setText(i18n("Auto Spell Check"));
	m_actions[ToggleAutoSpellChecking]->setCheckable(true);
	connect(m_actions[ToggleAutoSpellChecking], &QAction::triggered, this, &SimpleRichTextEdit::toggleAutoSpellChecking);

	m_actions[AllowTabulations] = new QAction(this);
	m_actions[AllowTabulations]->setText(i18n("Allow Tabulations"));
	connect(m_actions[AllowTabulations], &QAction::triggered, this, &SimpleRichTextEdit::toggleTabChangesFocus);

	QMenu *menu = createStandardContextMenu();
	menu->setParent(this);
	QList<QAction *> actions = menu->actions();
	m_insertUnicodeControlCharMenu = 0;
	for(QList<QAction *>::ConstIterator it = actions.constBegin(), end = actions.constEnd(); it != end; ++it) {
		if((*it)->menu()) {
			// this depends on Qt private implementation but at least is guaranteed
			// to behave reasonably if that implementation changes in the future.
			if(!strcmp((*it)->menu()->metaObject()->className(), "QUnicodeControlCharacterMenu")) {
				m_insertUnicodeControlCharMenu = (*it)->menu();
				break;
			}
		}
	}
}

SimpleRichTextEdit::~SimpleRichTextEdit()
{
	if(m_insertUnicodeControlCharMenu)
		delete m_insertUnicodeControlCharMenu->parent();
}

QAction *
SimpleRichTextEdit::action(int action) const
{
	return action >= 0 && action < ActionCount ? m_actions[action] : 0;
}

QList<QAction *> SimpleRichTextEdit::actions() const
{
	QList<QAction *> actions;
	for(int index = 0; index < ActionCount; ++index)
		actions.append(m_actions[index]);
	return actions;
}

SubtitleComposer::SString
SimpleRichTextEdit::richText()
{
	SubtitleComposer::SString richText(toPlainText());

	if(richText.length()) {
		QTextCursor cursor = textCursor();
		QTextCharFormat format;
		int styleFlags;
		QRgb styleColor;
		for(int position = 1, size = richText.length(); position <= size; ++position) {
			cursor.setPosition(position);
			format = cursor.charFormat();

			styleFlags = 0;
			if(format.fontWeight() == QFont::Bold)
				styleFlags |= SubtitleComposer::SString::Bold;
			if(format.fontItalic())
				styleFlags |= SubtitleComposer::SString::Italic;
			if(format.fontUnderline())
				styleFlags |= SubtitleComposer::SString::Underline;
			if(format.fontStrikeOut())
				styleFlags |= SubtitleComposer::SString::StrikeThrough;
			if(format.foreground().style() != Qt::NoBrush) {
				styleFlags |= SubtitleComposer::SString::Color;
				styleColor = format.foreground().color().toRgb().rgb();
			} else {
				styleColor = 0;
			}

			richText.setStyleFlagsAt(position - 1, styleFlags);
			richText.setStyleColorAt(position - 1, styleColor);
		}
	}

	return richText;
}

void
SimpleRichTextEdit::setRichText(const SubtitleComposer::SString &richText)
{
	setPlainText(richText.string());

	QTextCursor cursor = textCursor();
	cursor.setPosition(0);

	int currentStyleFlags = -1;
	QRgb currentStyleColor = 0;
	QTextCharFormat format;
	for(int position = 0, size = richText.length(); position < size; ++position) {
		if(currentStyleFlags != richText.styleFlagsAt(position) || ((richText.styleFlagsAt(position) & SubtitleComposer::SString::Color) && currentStyleColor != richText.styleColorAt(position))) {
			currentStyleFlags = richText.styleFlagsAt(position);
			currentStyleColor = richText.styleColorAt(position);
			format.setFontWeight(currentStyleFlags & SubtitleComposer::SString::Bold ? QFont::Bold : QFont::Normal);
			format.setFontItalic(currentStyleFlags & SubtitleComposer::SString::Italic);
			format.setFontUnderline(currentStyleFlags & SubtitleComposer::SString::Underline);
			format.setFontStrikeOut(currentStyleFlags & SubtitleComposer::SString::StrikeThrough);
			if((currentStyleFlags &SubtitleComposer::SString::Color) == 0)
				format.setForeground(QBrush());
			else
				format.setForeground(QBrush(QColor(currentStyleColor)));
		}

		cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
		cursor.mergeCharFormat(format);
		cursor.setPosition(position + 1);
	}

	clearUndoRedoHistory();
}

bool
SimpleRichTextEdit::hasSelection() const
{
	return textCursor().hasSelection();
}

QString
SimpleRichTextEdit::selectedText() const
{
	return textCursor().selectedText();
}

void
SimpleRichTextEdit::toggleFontItalic()
{
	setFontItalic(!fontItalic());
}

bool
SimpleRichTextEdit::fontBold()
{
	return fontWeight() == QFont::Bold;
}

void
SimpleRichTextEdit::setFontBold(bool enabled)
{
	setFontWeight(enabled ? QFont::Bold : QFont::Normal);
}

void
SimpleRichTextEdit::toggleFontBold()
{
	setFontBold(!fontBold());
}

void
SimpleRichTextEdit::toggleFontUnderline()
{
	setFontUnderline(!fontUnderline());
}

bool
SimpleRichTextEdit::fontStrikeOut()
{
	return currentFont().strikeOut();
}

void
SimpleRichTextEdit::setFontStrikeOut(bool enabled)
{
	QTextCursor cursor(textCursor());
	QTextCharFormat format;
	format.setFontStrikeOut(enabled);
	cursor.mergeCharFormat(format);
}

void
SimpleRichTextEdit::toggleFontStrikeOut()
{
	setFontStrikeOut(!fontStrikeOut());
}

void
SimpleRichTextEdit::changeTextColor()
{
	QColor color = SubtitleComposer::SubtitleColorDialog::getColor(textColor(), this);
	if(color.isValid()) {
		if(color.rgba() == 0) {
			QTextCursor cursor(textCursor());
			QTextCharFormat format;
			format.setForeground(QBrush(Qt::NoBrush));
			cursor.mergeCharFormat(format);
			setTextCursor(cursor);
		} else {
			setTextColor(color);
		}
	}
}

void
SimpleRichTextEdit::deleteText()
{
	QTextCursor cursor = textCursor();
	if(cursor.hasSelection())
		cursor.removeSelectedText();
	else
		cursor.deleteChar();
}

void
SimpleRichTextEdit::undoableClear()
{
	// Taken from KTextEdit
	QTextCursor cursor = textCursor();
	cursor.beginEditBlock();
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	cursor.removeSelectedText();
	cursor.endEditBlock();
}

void
SimpleRichTextEdit::clearUndoRedoHistory()
{
	if(isUndoRedoEnabled()) {
		setUndoRedoEnabled(false);      // clears the undo/redo history
		setUndoRedoEnabled(true);
	}
}

void
SimpleRichTextEdit::setSelection(int startIndex, int endIndex)
{
	QTextCursor cursor(textCursor());
	cursor.setPosition(startIndex);
	cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, endIndex - startIndex + 1);
	setTextCursor(cursor);
}

void
SimpleRichTextEdit::clearSelection()
{
	QTextCursor cursor(textCursor());
	cursor.clearSelection();
	setTextCursor(cursor);
}

void
SimpleRichTextEdit::toggleTabChangesFocus()
{
	setTabChangesFocus(!tabChangesFocus());
}

void
SimpleRichTextEdit::toggleAutoSpellChecking()
{
	setCheckSpellingEnabled(!checkSpellingEnabled());
}

void
SimpleRichTextEdit::setupWordUnderPositionCursor(const QPoint &globalPos)
{
	// Get the word under the (mouse-)cursor with apostrophes at the start/end
	m_selectedWordCursor = cursorForPosition(mapFromGlobal(globalPos));
	m_selectedWordCursor.clearSelection();
	m_selectedWordCursor.select(QTextCursor::WordUnderCursor);

	QString selectedWord = m_selectedWordCursor.selectedText();

	// Clear the selection again, we re-select it below (without the apostrophes).
	m_selectedWordCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, selectedWord.size());
	if(selectedWord.startsWith('\'') || selectedWord.startsWith('\"')) {
		selectedWord = selectedWord.right(selectedWord.size() - 1);
		m_selectedWordCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor);
	}

	if(selectedWord.endsWith('\'') || selectedWord.endsWith('\"'))
		selectedWord.chop(1);

	m_selectedWordCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selectedWord.size());
}

void
SimpleRichTextEdit::addToIgnoreList()
{
	highlighter()->ignoreWord(m_selectedWordCursor.selectedText());
	highlighter()->rehighlight();
	m_selectedWordCursor.clearSelection();
}

void
SimpleRichTextEdit::addToDictionary()
{
	highlighter()->addWordToDictionary(m_selectedWordCursor.selectedText());
	highlighter()->rehighlight();
	m_selectedWordCursor.clearSelection();
}

void
SimpleRichTextEdit::replaceWithSuggestion()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if(action) {
		m_selectedWordCursor.insertText(action->text());
		setTextCursor(m_selectedWordCursor);
		m_selectedWordCursor.clearSelection();
	}
}

QMenu *
SimpleRichTextEdit::createContextMenu(const QPoint &mouseGlobalPos)
{
	Qt::TextInteractionFlags interactionFlags = this->textInteractionFlags();
	QTextDocument *document = this->document();
	QTextCursor cursor = textCursor();

	const bool showTextSelectionActions = (Qt::TextEditable | Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse) & interactionFlags;

	QMenu *menu = new QMenu(this);

	if(interactionFlags & Qt::TextEditable) {
		m_actions[Undo]->setEnabled(document->isUndoAvailable());
		menu->addAction(m_actions[Undo]);

		m_actions[Redo]->setEnabled(document->isRedoAvailable());
		menu->addAction(m_actions[Redo]);

		menu->addSeparator();

		m_actions[Cut]->setEnabled(cursor.hasSelection());
		menu->addAction(m_actions[Cut]);
	}

	if(showTextSelectionActions) {
		m_actions[Copy]->setEnabled(cursor.hasSelection());
		menu->addAction(m_actions[Copy]);
	}

	if(interactionFlags & Qt::TextEditable) {
#if !defined(QT_NO_CLIPBOARD)
		m_actions[Paste]->setEnabled(canPaste());
		menu->addAction(m_actions[Paste]);
#endif
		m_actions[Delete]->setEnabled(cursor.hasSelection());
		menu->addAction(m_actions[Delete]);

		m_actions[Clear]->setEnabled(!document->isEmpty());
		menu->addAction(m_actions[Clear]);

		if(m_insertUnicodeControlCharMenu && interactionFlags & Qt::TextEditable) {
			menu->addSeparator();
			menu->addMenu(m_insertUnicodeControlCharMenu);
		}
	}

	if(showTextSelectionActions) {
		menu->addSeparator();

		m_actions[SelectAll]->setEnabled(!document->isEmpty());
		menu->addAction(m_actions[SelectAll]);
	}

	if(interactionFlags & Qt::TextEditable) {
		menu->addSeparator();

		m_actions[ToggleBold]->setCheckable(true);
		m_actions[ToggleBold]->setChecked(fontBold());
		menu->addAction(m_actions[ToggleBold]);

		m_actions[ToggleItalic]->setCheckable(true);
		m_actions[ToggleItalic]->setChecked(fontItalic());
		menu->addAction(m_actions[ToggleItalic]);

		m_actions[ToggleUnderline]->setCheckable(true);
		m_actions[ToggleUnderline]->setChecked(fontUnderline());
		menu->addAction(m_actions[ToggleUnderline]);

		m_actions[ToggleStrikeOut]->setCheckable(true);
		m_actions[ToggleStrikeOut]->setChecked(fontStrikeOut());
		menu->addAction(m_actions[ToggleStrikeOut]);

		menu->addAction(m_actions[ChangeTextColor]);

		menu->addSeparator();

		m_actions[CheckSpelling]->setEnabled(!document->isEmpty());
		menu->addAction(m_actions[CheckSpelling]);

		m_actions[ToggleAutoSpellChecking]->setChecked(checkSpellingEnabled());
		menu->addAction(m_actions[ToggleAutoSpellChecking]);

		if(checkSpellingEnabled()) {
			setupWordUnderPositionCursor(mouseGlobalPos);

			QString selectedWord = m_selectedWordCursor.selectedText();
			if(!selectedWord.isEmpty() && highlighter() && highlighter()->isWordMisspelled(selectedWord)) {
				QMenu *suggestionsMenu = menu->addMenu(i18n("Suggestions"));
				suggestionsMenu->addAction(i18n("Ignore"), this, &SimpleRichTextEdit::addToIgnoreList);
				suggestionsMenu->addAction(i18n("Add to Dictionary"), this, &SimpleRichTextEdit::addToDictionary);
				suggestionsMenu->addSeparator();
				QStringList suggestions = highlighter()->suggestionsForWord(m_selectedWordCursor.selectedText());
				if(suggestions.empty())
					suggestionsMenu->addAction(i18n("No suggestions"))->setEnabled(false);
				else {
					for(QStringList::ConstIterator it = suggestions.constBegin(), end = suggestions.constEnd(); it != end; ++it) 
						suggestionsMenu->addAction(*it, this, &SimpleRichTextEdit::replaceWithSuggestion);
				}
			}
		}

		menu->addSeparator();

		m_actions[AllowTabulations]->setCheckable(true);
		m_actions[AllowTabulations]->setChecked(!tabChangesFocus());
		menu->addAction(m_actions[AllowTabulations]);
	}

	return menu;
}

void
SimpleRichTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createContextMenu(event->globalPos());

	menu->exec(event->globalPos());

	for(int index = 0; index < ActionCount; ++index)
		m_actions[index]->setEnabled(true);

	delete menu;
}

bool
SimpleRichTextEdit::event(QEvent *event)
{
	if(event->type() == QEvent::ShortcutOverride) {
		// Stop our actions shorcuts from being propagated
		// to other actions when we have the focus.

		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		QKeySequence key(keyEvent->modifiers() + keyEvent->key());

		for(int index = 0; index < ActionCount; ++index) {
			if(m_actions[index]->shortcut().matches(key) == QKeySequence::ExactMatch) {
				event->accept();
				return true;
			}
		}
	}

	return KTextEdit::event(event);
}

void
SimpleRichTextEdit::keyPressEvent(QKeyEvent *event)
{
	QKeySequence key(event->modifiers() + event->key());

	for(int index = 0; index < ActionCount; ++index) {
		if(m_actions[index]->shortcut().matches(key) == QKeySequence::ExactMatch) {
			m_actions[index]->trigger();
			return;
		}
	}

	KTextEdit::keyPressEvent(event);
}


