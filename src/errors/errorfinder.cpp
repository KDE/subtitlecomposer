/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "errorfinder.h"

#include "appglobal.h"
#include "application.h"
#include "core/subtitleiterator.h"
#include "errors/finderrorsdialog.h"
#include "gui/treeview/lineswidget.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>

#include <QDebug>

#include <KMessageBox>

using namespace SubtitleComposer;


ErrorFinder::ErrorFinder(QWidget *parent)
	: QObject(parent),
	  m_subtitle(nullptr),
	  m_dialog(new FindErrorsDialog(parent)),
	  m_translationMode(false),
	  m_iterator(nullptr)
{
}

ErrorFinder::~ErrorFinder()
{
	delete m_dialog;
	invalidate();
}

void
ErrorFinder::invalidate()
{
	delete m_iterator;
	m_iterator = nullptr;
}

QWidget *
ErrorFinder::parentWidget()
{
	return static_cast<QWidget *>(parent());
}

void
ErrorFinder::setSubtitle(Subtitle *subtitle)
{
	m_subtitle = subtitle;
	invalidate();
}

void
ErrorFinder::setTranslationMode(bool enabled)
{
	if(m_translationMode != enabled) {
		m_translationMode = enabled;
		invalidate();
	}
}

void
ErrorFinder::find(int searchFromIndex, bool findBackwards)
{
	if(!m_subtitle || m_subtitle->isEmpty())
		return;

	if(m_dialog->exec() != QDialog::Accepted)
		return;

	const RangeList targetRanges = app()->linesWidget()->targetRanges(m_dialog->selectedLinesTarget());

	{
		SubtitleCompositeActionExecutor executor(m_subtitle.constData(), i18n("Check Lines Errors"));

		if(m_dialog->clearOtherErrors())
			m_subtitle->clearErrors(targetRanges, SubtitleLine::AllErrors & ~SubtitleLine::UserMark);

		if(m_dialog->clearMarks())
			m_subtitle->setMarked(targetRanges, false);

		m_subtitle->checkErrors(targetRanges, m_dialog->selectedErrorFlags());
	}


	invalidate();

	m_findBackwards = findBackwards;
	m_targetErrorFlags = m_dialog->selectedErrorFlags() | SubtitleLine::UserMark;

	m_iterator = new SubtitleIterator(*m_subtitle, targetRanges);
	if(m_iterator->index() == SubtitleIterator::Invalid) {
		invalidate();
		return;
	}
	m_iterator->toIndex(searchFromIndex < 0 ? 0 : searchFromIndex);

	advance(false);
}

bool
ErrorFinder::findNext(int fromIndex)
{
	if(!m_iterator)
		return false;

	m_findBackwards = false;
	if(fromIndex != -1)
		m_iterator->toIndex(fromIndex);

	advance(true);
	return true;
}

bool
ErrorFinder::findPrevious(int fromIndex)
{
	if(!m_iterator)
		return false;

	m_findBackwards = true;
	if(fromIndex != -1)
		m_iterator->toIndex(fromIndex);

	advance(true);
	return true;
}

void
ErrorFinder::advance(bool advanceIteratorOnFirstStep)
{
	const int startIndex = m_iterator->index();
	bool searched = false;

	for(;;) {
		if(advanceIteratorOnFirstStep) {
			if(m_findBackwards)
				--(*m_iterator);
			else
				++(*m_iterator);

			if(m_iterator->index() < 0) {
				if(m_findBackwards)
					m_iterator->toLast();
				else
					m_iterator->toFirst();

//				const QString text = m_findBackwards
//						? (m_selection ? i18n("Beginning of selection reached.\nContinue from the end?") : i18n("Beginning of subtitle reached.\nContinue from the end?"))
//						: (m_selection ? i18n("End of selection reached.\nContinue from the beginning?") : i18n("End of subtitle reached.\nContinue from the beginning?"));
//				if(KMessageBox::warningContinueCancel(parentWidget(), text, i18n("Find Error")) != KMessageBox::Continue)
//					break;
			}
		} else {
			advanceIteratorOnFirstStep = true;
		}

		if(m_iterator->current()->errorFlags() & m_targetErrorFlags) {
			emit found(m_iterator->current());
			break;
		} else if(searched && startIndex == m_iterator->index()) {
			// searched through all lines and found no errors
			KMessageBox::information(parentWidget(), i18n("No errors matching given criteria were found!"), i18n("Find Error"));
			invalidate();
			return;
		}

		searched = true;
	}
}
