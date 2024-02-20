/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "config.h"

#include "appglobal.h"
#include "application.h"
#include "actions/krecentfilesactionext.h"
#include "actions/useraction.h"
#include "actions/useractionnames.h"
#include "configs/configdialog.h"
#include "core/richtext/richdocument.h"
#include "core/subtitleiterator.h"
#include "core/undo/undostack.h"
#include "gui/currentlinewidget.h"
#include "gui/subtitlemeta/subtitlemetawidget.h"
#include "helpers/common.h"
#include "dialogs/actionwithtargetdialog.h"
#include "dialogs/shifttimesdialog.h"
#include "dialogs/adjusttimesdialog.h"
#include "dialogs/durationlimitsdialog.h"
#include "dialogs/autodurationsdialog.h"
#include "dialogs/changetextscasedialog.h"
#include "dialogs/fixoverlappingtimesdialog.h"
#include "dialogs/fixpunctuationdialog.h"
#include "dialogs/smarttextsadjustdialog.h"
#include "dialogs/changeframeratedialog.h"
#include "dialogs/insertlinedialog.h"
#include "dialogs/removelinesdialog.h"
#include "dialogs/intinputdialog.h"
#include "dialogs/subtitlecolordialog.h"
#include "errors/errorfinder.h"
#include "errors/errortracker.h"
#include "formats/formatmanager.h"
#include "formats/outputformat.h"
#include "formats/textdemux/textdemux.h"
#include "helpers/commondefs.h"
#include "gui/treeview/lineswidget.h"
#include "mainwindow.h"
#include "gui/playerwidget.h"
#include "scripting/scriptsmanager.h"
#include "speechprocessor/speechprocessor.h"
#include "utils/finder.h"
#include "utils/replacer.h"
#include "utils/speller.h"
#include "videoplayer/videoplayer.h"
#include "gui/waveform/waveformwidget.h"

#include <limits>

#include <QAction>
#include <QByteArrayList>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QStringBuilder>
#include <QTextCodec>
#include <QThread>
#include <QUndoGroup>
#include <QUndoStack>

#include <KActionCollection>
#include <KCharsets>
#include <KComboBox>
#include <KConfig>
#include <KMessageBox>
#include <KSelectAction>
#include <KStandardShortcut>
#include <KStandardAction>
#include <KToggleAction>
#include <KToolBar>
#include <kxmlgui_version.h>

using namespace SubtitleComposer;

static void
setupIconTheme(int argc, char **argv)
{
#ifdef SC_BUNDLE_SYSTEM_THEME
	QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << $(":/icons"));
#endif
	if(QIcon::themeName().isEmpty())
		QIcon::setThemeName($("breeze"));

	const QStringList fallbackPaths = {
		$(":/icons-fallback/breeze/actions/22"),
		$(":/icons-fallback/breeze/apps/256"),
		$(":/icons-fallback/breeze/apps/128"),
		$(":/icons-fallback/breeze/apps/48"),
		$(":/icons-fallback/breeze/apps/32"),
		$(":/icons-fallback/breeze/apps/16"),
	};

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths()
#else
	QIcon::setThemeSearchPaths(QIcon::themeSearchPaths()
#endif
			// access the icons through breeze theme path
			<< $(":/icons-fallback")
			// or directly as fallback
			<< fallbackPaths);

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	if(QIcon::fallbackThemeName().isEmpty())
		QIcon::setFallbackThemeName($("breeze"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	// LXQt just ignores QIcon::fallbackSearchPaths()
	QString platformThemeName = QString::fromLocal8Bit(qgetenv("QT_QPA_PLATFORMTHEME"));
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-platformtheme") == 0 && ++i < argc) {
			platformThemeName = QString::fromLocal8Bit(argv[i]);
			break;
		}
	}

	if(platformThemeName == $("lxqt"))
		QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << fallbackPaths);
#else
	Q_UNUSED(argc)
	Q_UNUSED(argv)
#endif
}

Application::Application(int &argc, char **argv) :
	QApplication(argc, argv),
	m_translationMode(false),
	m_textDemux(nullptr),
	m_speechProcessor(nullptr),
	m_lastFoundLine(nullptr),
	m_lastSubtitleUrl(QDir::homePath()),
	m_lastVideoUrl(QDir::homePath()),
	m_linkCurrentLineToPosition(false)
{
	AppGlobal::app = this;

	KLocalizedString::setApplicationDomain("subtitlecomposer");

	setupIconTheme(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
}

void
Application::createMainWindow()
{
	m_mainWindow = new MainWindow();
	connect(m_mainWindow, &QObject::destroyed, this, [&](){ m_mainWindow = nullptr; });

	m_finder = new Finder(m_mainWindow->m_linesWidget);
	m_replacer = new Replacer(m_mainWindow->m_linesWidget);
	m_errorFinder = new ErrorFinder(m_mainWindow->m_linesWidget);
	m_speller = new Speller(m_mainWindow->m_linesWidget);

	m_errorTracker = new ErrorTracker(this);

	QStatusBar *statusBar = m_mainWindow->statusBar();
	QMargins m = statusBar->contentsMargins();
	m.setRight(m.left() + 5);
	statusBar->setContentsMargins(m);

	m_labSubFormat = new QLabel();
	statusBar->addPermanentWidget(m_labSubFormat);

	m_labSubEncoding = new QLabel();
	statusBar->addPermanentWidget(m_labSubEncoding);

	m_textDemux = new TextDemux(m_mainWindow);
	statusBar->addPermanentWidget(m_textDemux->progressWidget());

	m_speechProcessor = new SpeechProcessor(m_mainWindow);
	statusBar->addPermanentWidget(m_speechProcessor->progressWidget());

	m_scriptsManager = new ScriptsManager(this);

	AppGlobal::undoStack = new UndoStack(m_mainWindow);

	UserActionManager *actionManager = UserActionManager::instance();
	actionManager->setLinesWidget(m_mainWindow->m_linesWidget);
	actionManager->setFullScreenMode(false);

	setupActions();

	connect(SCConfig::self(), &KCoreConfigSkeleton::configChanged, this, &Application::onConfigChanged);

	VideoPlayer *videoPlayer = VideoPlayer::instance();
	connect(videoPlayer, &VideoPlayer::fileOpened, this, &Application::onPlayerFileOpened);
	connect(videoPlayer, &VideoPlayer::playing, this, &Application::onPlayerPlaying);
	connect(videoPlayer, &VideoPlayer::paused, this, &Application::onPlayerPaused);
	connect(videoPlayer, &VideoPlayer::stopped, this, &Application::onPlayerStopped);
	connect(videoPlayer, &VideoPlayer::audioStreamsChanged, this, &Application::onPlayerAudioStreamsChanged);
	connect(videoPlayer, &VideoPlayer::textStreamsChanged, this, &Application::onPlayerTextStreamsChanged);
	connect(videoPlayer, &VideoPlayer::activeAudioStreamChanged, this, &Application::onPlayerActiveAudioStreamChanged);
	connect(videoPlayer, &VideoPlayer::muteChanged, this, &Application::onPlayerMuteChanged);

#define CONNECT_SUB(c, x) \
	connect(this, &Application::subtitleOpened, x, &c::setSubtitle); \
	connect(this, &Application::subtitleClosed, x, [&](){ x->setSubtitle(nullptr); });

	CONNECT_SUB(UserActionManager, UserActionManager::instance());
	CONNECT_SUB(PlayerWidget, m_mainWindow->m_playerWidget);
	CONNECT_SUB(SubtitleMetaWidget, m_mainWindow->m_metaWidget);
	CONNECT_SUB(LinesWidget, m_mainWindow->m_linesWidget);
	CONNECT_SUB(CurrentLineWidget, m_mainWindow->m_curLineWidget);
	CONNECT_SUB(Finder, m_finder);
	CONNECT_SUB(Replacer, m_replacer);
	CONNECT_SUB(ErrorFinder, m_errorFinder);
	CONNECT_SUB(Speller, m_speller);
	CONNECT_SUB(ErrorTracker, m_errorTracker);
	CONNECT_SUB(ScriptsManager, m_scriptsManager);
	CONNECT_SUB(WaveformWidget, m_mainWindow->m_waveformWidget);
	CONNECT_SUB(SpeechProcessor, m_speechProcessor);

#define CONNECT_TRANS(c, x) \
	connect(this, &Application::translationModeChanged, x, &c::setTranslationMode);

	CONNECT_TRANS(UserActionManager, UserActionManager::instance());
	CONNECT_TRANS(PlayerWidget, m_mainWindow->m_playerWidget);
	CONNECT_TRANS(WaveformWidget, m_mainWindow->m_waveformWidget);
	CONNECT_TRANS(LinesWidget, m_mainWindow->m_linesWidget);
	CONNECT_TRANS(CurrentLineWidget, m_mainWindow->m_curLineWidget);
	CONNECT_TRANS(Finder, m_finder);
	CONNECT_TRANS(Replacer, m_replacer);
	CONNECT_TRANS(ErrorFinder, m_errorFinder);
	CONNECT_TRANS(Speller, m_speller);

	connect(this, &Application::fullScreenModeChanged, actionManager, &UserActionManager::setFullScreenMode);

	connect(m_mainWindow->m_waveformWidget, &WaveformWidget::doubleClick, this, &Application::onWaveformDoubleClicked);
	connect(m_mainWindow->m_waveformWidget, &WaveformWidget::middleMouseDown, this, &Application::onWaveformMiddleMouse);
	connect(m_mainWindow->m_waveformWidget, &WaveformWidget::middleMouseMove, this, &Application::onWaveformMiddleMouse);
	connect(m_mainWindow->m_waveformWidget, &WaveformWidget::middleMouseUp, this, &Application::onWaveformMiddleMouse);

	connect(m_mainWindow->m_linesWidget, &LinesWidget::currentLineChanged, m_mainWindow->m_metaWidget, &SubtitleMetaWidget::setCurrentLine);

	connect(m_mainWindow->m_linesWidget, &LinesWidget::currentLineChanged, m_mainWindow->m_curLineWidget, &CurrentLineWidget::setCurrentLine);
	connect(m_mainWindow->m_linesWidget, &LinesWidget::lineDoubleClicked, this, &Application::onLineDoubleClicked);

	connect(m_mainWindow->m_playerWidget, &PlayerWidget::playingLineChanged, this, &Application::onPlayingLineChanged);

	connect(m_finder, &Finder::found, this, &Application::onHighlightLine);
	connect(m_replacer, &Replacer::found, this, &Application::onHighlightLine);
	connect(m_errorFinder, &ErrorFinder::found, this, [this](SubtitleLine *l){ onHighlightLine(l); });
	connect(m_speller, &Speller::misspelled, this, &Application::onHighlightLine);

	connect(m_textDemux, &TextDemux::onError, this, [&](const QString &message){ KMessageBox::error(m_mainWindow, message); });

	connect(m_speechProcessor, &SpeechProcessor::onError, this, [&](const QString &message){ KMessageBox::error(m_mainWindow, message); });

	m_mainWindow->setupGUI();

	// Workaround for https://phabricator.kde.org/D13808
	// menubar can't be hidden so always show it
	m_mainWindow->findChild<QMenuBar *>(QString(), Qt::FindDirectChildrenOnly)->show();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 1) && KXMLGUI_VERSION < QT_VERSION_CHECK(5, 49, 0)
	// show rest if we're on broken KF5/Qt
	m_mainWindow->findChild<QStatusBar *>(QString(), Qt::FindDirectChildrenOnly)->show();
	m_mainWindow->findChild<QDockWidget *>(QStringLiteral("player_dock"), Qt::FindDirectChildrenOnly)->show();
	m_mainWindow->findChild<QDockWidget *>(QStringLiteral("waveform_dock"), Qt::FindDirectChildrenOnly)->show();
	foreach(KToolBar *toolbar, m_mainWindow->toolBars())
		toolbar->show();
#endif

	m_scriptsManager->reloadScripts();

	loadConfig();
}

Application::~Application()
{
	// NOTE: The Application destructor is called after all widgets are destroyed
	// (NOT BEFORE). Therefore is not possible to save the program settings (nor do
	// pretty much anything) at this point.

	// delete m_mainWindow; the window is destroyed when it's closed
	AppGlobal::app = nullptr;
}

void
Application::loadConfig()
{
	KSharedConfig::Ptr config = KSharedConfig::openConfig();
	KConfigGroup group(config->group("Application Settings"));

	m_lastSubtitleUrl = QUrl(group.readPathEntry("LastSubtitleUrl", QDir::homePath()));
	m_recentSubtitlesAction->loadEntries(config->group("Recent Subtitles"));
	m_recentSubtitlesTrAction->loadEntries(config->group("Recent Translation Subtitles"));

	m_lastVideoUrl = QUrl(group.readPathEntry("LastVideoUrl", QDir::homePath()));
	m_recentVideosAction->loadEntries(config->group("Recent Videos"));

	VideoPlayer *videoPlayer = VideoPlayer::instance();
	videoPlayer->setMuted(group.readEntry<bool>("Muted", false));
	videoPlayer->setVolume(group.readEntry<double>("Volume", 100.0));

	((KToggleAction *)action(ACT_TOGGLE_MUTED))->setChecked(videoPlayer->isMuted());

	KConfigGroup wfGroup(config->group("Waveform Widget"));
	action(ACT_WAVEFORM_AUTOSCROLL)->setChecked(wfGroup.readEntry<bool>("AutoScroll", true));
	if(wfGroup.hasKey("Zoom"))
		m_mainWindow->m_waveformWidget->setZoom(wfGroup.readEntry<quint32>("Zoom", 0));

	m_mainWindow->loadConfig();
	m_mainWindow->m_playerWidget->loadConfig();
	m_mainWindow->m_linesWidget->loadConfig();
	m_mainWindow->m_curLineWidget->loadConfig();
}

void
Application::saveConfig()
{
	KSharedConfig::Ptr config = KSharedConfig::openConfig();
	KConfigGroup group(config->group("Application Settings"));

	group.writePathEntry("LastSubtitleUrl", m_lastSubtitleUrl.toString());
	m_recentSubtitlesAction->saveEntries(config->group("Recent Subtitles"));
	m_recentSubtitlesTrAction->saveEntries(config->group("Recent Translation Subtitles"));

	group.writePathEntry("LastVideoUrl", m_lastVideoUrl.toString());
	m_recentVideosAction->saveEntries(config->group("Recent Videos"));

	VideoPlayer *videoPlayer = VideoPlayer::instance();
	group.writeEntry("Muted", videoPlayer->isMuted());
	group.writeEntry("Volume", videoPlayer->volume());

	KConfigGroup wfGroup(config->group("Waveform Widget"));
	wfGroup.writeEntry("AutoScroll", m_mainWindow->m_waveformWidget->autoScroll());
	wfGroup.writeEntry("Zoom", m_mainWindow->m_waveformWidget->zoom());

	m_mainWindow->saveConfig();
	m_mainWindow->m_playerWidget->saveConfig();
	m_mainWindow->m_linesWidget->saveConfig();
	m_mainWindow->m_curLineWidget->saveConfig();
}

bool
Application::triggerAction(const QKeySequence &keySequence)
{
	QList<QAction *> actions = m_mainWindow->actionCollection()->actions();

	for(QList<QAction *>::ConstIterator it = actions.constBegin(), end = actions.constEnd(); it != end; ++it) {
		if((*it)->isEnabled()) {
			if(QAction * action = qobject_cast<QAction *>(*it)) {
				QKeySequence shortcut = action->shortcut();
				if(shortcut.matches(keySequence) == QKeySequence::ExactMatch) {
					action->trigger();
					return true;
				}
			} else {
				if((*it)->shortcut() == keySequence) {
					(*it)->trigger();
					return true;
				}
			}
		}
	}

	return false;
}

const QStringList &
Application::availableEncodingNames() const
{
	static QStringList encodingNames;

	if(encodingNames.empty()) {
		QStringList encodings(KCharsets::charsets()->availableEncodingNames());
		for(QStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it) {
			QTextCodec *codec = QTextCodec::codecForName(it->toUtf8());
			if(codec)
				encodingNames.append(codec->name().toUpper());
		}
		encodingNames.sort();
	}

	return encodingNames;
}

void
Application::showPreferences()
{
	(new ConfigDialog(m_mainWindow, "scconfig", SCConfig::self()))->show();
}

/// BEGIN ACTION HANDLERS

Time
Application::videoPosition(bool compensate)
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	if(compensate && !videoPlayer->isPaused())
		return Time(double(videoPlayer->position()) * 1000. - SCConfig::grabbedPositionCompensation());
	else
		return Time(double(videoPlayer->position()) * 1000.);
}

void
Application::insertBeforeCurrentLine()
{
	static InsertLineDialog *dlg = new InsertLineDialog(true, m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		SubtitleLine *newLine;
		{
			LinesWidgetScrollToModelDetacher detacher(*m_mainWindow->m_linesWidget);
			SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
			newLine = appSubtitle()->insertNewLine(currentLine ? currentLine->index() : 0, false, dlg->selectedTextsTarget());
		}
		m_mainWindow->m_linesWidget->setCurrentLine(newLine, true);
	}
}

void
Application::insertAfterCurrentLine()
{
	static InsertLineDialog *dlg = new InsertLineDialog(false, m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		SubtitleLine *newLine;
		{
			LinesWidgetScrollToModelDetacher detacher(*m_mainWindow->m_linesWidget);

			SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
			newLine = appSubtitle()->insertNewLine(currentLine ? currentLine->index() + 1 : 0, true, dlg->selectedTextsTarget());
		}
		m_mainWindow->m_linesWidget->setCurrentLine(newLine, true);
	}
}

void
Application::removeSelectedLines()
{
	static RemoveLinesDialog *dlg = new RemoveLinesDialog(m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		RangeList selectionRanges = m_mainWindow->m_linesWidget->selectionRanges();

		if(selectionRanges.isEmpty())
			return;

		{
			LinesWidgetScrollToModelDetacher detacher(*m_mainWindow->m_linesWidget);
			appSubtitle()->removeLines(selectionRanges, dlg->selectedTextsTarget());
		}

		int firstIndex = selectionRanges.firstIndex();
		if(firstIndex < appSubtitle()->linesCount())
			m_mainWindow->m_linesWidget->setCurrentLine(appSubtitle()->line(firstIndex), true);
		else if(firstIndex - 1 < appSubtitle()->linesCount())
			m_mainWindow->m_linesWidget->setCurrentLine(appSubtitle()->line(firstIndex - 1), true);
	}
}

void
Application::joinSelectedLines()
{
	const RangeList &ranges = m_mainWindow->m_linesWidget->selectionRanges();

//  if ( ranges.count() > 1 && KMessageBox::Continue != KMessageBox::warningContinueCancel(
//      m_mainWindow,
//      i18n( "Current selection has multiple ranges.\nContinuing will join them all." ),
//      i18n( "Join Lines" ) ) )
//      return;

	appSubtitle()->joinLines(ranges);
}

void
Application::splitSelectedLines()
{
	appSubtitle()->splitLines(m_mainWindow->m_linesWidget->selectionRanges());
}

void
Application::selectAllLines()
{
	m_mainWindow->m_linesWidget->selectAll();
}

void
Application::gotoLine()
{
	IntInputDialog gotoLineDlg(i18n("Go to Line"), i18n("&Go to line:"), 1, appSubtitle()->linesCount(), m_mainWindow->m_linesWidget->currentLineIndex() + 1);

	if(gotoLineDlg.exec() == QDialog::Accepted)
		m_mainWindow->m_linesWidget->setCurrentLine(appSubtitle()->line(gotoLineDlg.value() - 1), true);
}

void
Application::find()
{
	m_lastFoundLine = nullptr;
	m_finder->find(m_mainWindow->m_linesWidget->selectionRanges(), m_mainWindow->m_linesWidget->currentLineIndex(), m_mainWindow->m_curLineWidget->focusedText(), false);
}

void
Application::findNext()
{
	if(!m_finder->findNext()) {
		m_lastFoundLine = nullptr;
		m_finder->find(m_mainWindow->m_linesWidget->selectionRanges(), m_mainWindow->m_linesWidget->currentLineIndex(), m_mainWindow->m_curLineWidget->focusedText(), false);
	}
}

void
Application::findPrevious()
{
	if(!m_finder->findPrevious()) {
		m_lastFoundLine = nullptr;
		m_finder->find(m_mainWindow->m_linesWidget->selectionRanges(), m_mainWindow->m_linesWidget->currentLineIndex(), m_mainWindow->m_curLineWidget->focusedText(), true);
	}
}

void
Application::replace()
{
	m_replacer->replace(m_mainWindow->m_linesWidget->selectionRanges(), m_mainWindow->m_linesWidget->currentLineIndex(), m_mainWindow->m_curLineWidget->focusedText());
}

void
Application::spellCheck()
{
	m_speller->spellCheck(m_mainWindow->m_linesWidget->currentLineIndex());
}

void
Application::retrocedeCurrentLine()
{
	SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
	if(currentLine && currentLine->prevLine())
		m_mainWindow->m_linesWidget->setCurrentLine(currentLine->prevLine(), true);
}

void
Application::advanceCurrentLine()
{
	SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
	if(currentLine && currentLine->nextLine())
		m_mainWindow->m_linesWidget->setCurrentLine(currentLine->nextLine(), true);
}

void
Application::toggleSelectedLinesBold()
{
	appSubtitle()->toggleStyleFlag(m_mainWindow->m_linesWidget->selectionRanges(), RichString::Bold);
}

void
Application::toggleSelectedLinesItalic()
{
	appSubtitle()->toggleStyleFlag(m_mainWindow->m_linesWidget->selectionRanges(), RichString::Italic);
}

void
Application::toggleSelectedLinesUnderline()
{
	appSubtitle()->toggleStyleFlag(m_mainWindow->m_linesWidget->selectionRanges(), RichString::Underline);
}

void
Application::toggleSelectedLinesStrikeThrough()
{
	appSubtitle()->toggleStyleFlag(m_mainWindow->m_linesWidget->selectionRanges(), RichString::StrikeThrough);
}

void
Application::changeSelectedLinesColor()
{
	const RangeList range = m_mainWindow->m_linesWidget->selectionRanges();
	SubtitleIterator it(*appSubtitle(), range);
	if(!it.current())
		return;

	QColor color = SubtitleColorDialog::getColor(QColor(it.current()->primaryDoc()->styleColorAt(0)), m_mainWindow);
	if(color.isValid())
		appSubtitle()->changeTextColor(range, color.rgba());
}

void
Application::shiftLines()
{
	static ShiftTimesDialog *dlg = new ShiftTimesDialog(m_mainWindow);

	dlg->resetShiftTime();

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->shiftLines(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()), dlg->shiftTimeMillis());
}

void
Application::shiftSelectedLinesForwards()
{
	appSubtitle()->shiftLines(m_mainWindow->m_linesWidget->selectionRanges(), SCConfig::linesQuickShiftAmount());
}

void
Application::shiftSelectedLinesBackwards()
{
	appSubtitle()->shiftLines(m_mainWindow->m_linesWidget->selectionRanges(), -SCConfig::linesQuickShiftAmount());
}

void
Application::adjustLines()
{
	static AdjustTimesDialog *dlg = new AdjustTimesDialog(m_mainWindow);

	dlg->setFirstLineTime(appSubtitle()->firstLine()->showTime());
	dlg->setLastLineTime(appSubtitle()->lastLine()->showTime());

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->adjustLines(Range::full(), dlg->firstLineTime().toMillis(), dlg->lastLineTime().toMillis());
}

void
Application::changeFrameRate()
{
	static ChangeFrameRateDialog *dlg = new ChangeFrameRateDialog(appSubtitle()->framesPerSecond(), m_mainWindow);

	dlg->setFromFramesPerSecond(appSubtitle()->framesPerSecond());

	if(dlg->exec() == QDialog::Accepted) {
		appSubtitle()->changeFramesPerSecond(dlg->toFramesPerSecond(), dlg->fromFramesPerSecond());
	}
}

void
Application::enforceDurationLimits()
{
	static DurationLimitsDialog *dlg = new DurationLimitsDialog(SCConfig::minDuration(),
																SCConfig::maxDuration(),
																m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		appSubtitle()->applyDurationLimits(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
			dlg->enforceMinDuration() ? dlg->minDuration() : Time(),
			dlg->enforceMaxDuration() ? dlg->maxDuration() : Time(std::numeric_limits<double>::max()),
			!dlg->preventOverlap());
	}
}

void
Application::setAutoDurations()
{
	static AutoDurationsDialog *dlg = new AutoDurationsDialog(60, 50, 50, m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		appSubtitle()->setAutoDurations(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
									 dlg->charMillis(), dlg->wordMillis(), dlg->lineMillis(),
									 !dlg->preventOverlap(), dlg->calculationMode());
	}
}

void
Application::maximizeDurations()
{
	static auto *dlg = new ActionWithLinesTargetDialog(i18n("Maximize Durations"), m_mainWindow);

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->setMaximumDurations(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()));
}

void
Application::fixOverlappingLines()
{
	static FixOverlappingTimesDialog *dlg = new FixOverlappingTimesDialog(m_mainWindow);

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->fixOverlappingLines(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()), dlg->minimumInterval());
}

void
Application::breakLines()
{
	static SmartTextsAdjustDialog *dlg = new SmartTextsAdjustDialog(30, m_mainWindow);

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->breakLines(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
							   dlg->minLengthForLineBreak(), dlg->selectedTextsTarget());
}

void
Application::unbreakTexts()
{
	static ActionWithLinesAndTextsTargetDialog *dlg = new ActionWithLinesAndTextsTargetDialog(i18n("Unbreak Lines"),
																							  m_mainWindow);

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->unbreakTexts(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
								 dlg->selectedTextsTarget());
}

void
Application::simplifySpaces()
{
	static ActionWithLinesAndTextsTargetDialog *dlg = new ActionWithLinesAndTextsTargetDialog(i18n("Simplify Spaces"),
																							  m_mainWindow);

	if(dlg->exec() == QDialog::Accepted)
		appSubtitle()->simplifyTextWhiteSpace(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
										   dlg->selectedTextsTarget());
}

void
Application::changeCase()
{
	static ChangeTextsCaseDialog *dlg = new ChangeTextsCaseDialog(m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		switch(dlg->caseOperation()) {
		case ChangeTextsCaseDialog::Upper:
			appSubtitle()->upperCase(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
								  dlg->selectedTextsTarget());
			break;
		case ChangeTextsCaseDialog::Lower:
			appSubtitle()->lowerCase(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
								  dlg->selectedTextsTarget());
			break;
		case ChangeTextsCaseDialog::Title:
			appSubtitle()->titleCase(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
								  dlg->lowerFirst(), dlg->selectedTextsTarget());
			break;
		case ChangeTextsCaseDialog::Sentence:
			appSubtitle()->sentenceCase(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
									 dlg->lowerFirst(), dlg->selectedTextsTarget());
			break;
		}
	}
}

void
Application::fixPunctuation()
{
	static FixPunctuationDialog *dlg = new FixPunctuationDialog(m_mainWindow);

	if(dlg->exec() == QDialog::Accepted) {
		appSubtitle()->fixPunctuation(m_mainWindow->m_linesWidget->targetRanges(dlg->selectedLinesTarget()),
								   dlg->spaces(), dlg->quotes(), dlg->englishI(),
								   dlg->ellipsis(), dlg->selectedTextsTarget());
	}
}

void
Application::openVideo(const QUrl &url)
{
	if(url.scheme() != QStringLiteral("file"))
		return;

	VideoPlayer *videoPlayer = VideoPlayer::instance();
	videoPlayer->closeFile();
	videoPlayer->openFile(url.toLocalFile());
}

void
Application::openVideo()
{
	QFileDialog openDlg(m_mainWindow, i18n("Open Video"), QString(), buildMediaFilesFilter());

	openDlg.setModal(true);
	openDlg.selectUrl(m_lastVideoUrl);

	if(openDlg.exec() == QDialog::Accepted) {
		m_lastVideoUrl = openDlg.selectedUrls().first();
		openVideo(m_lastVideoUrl);
	}
}

void
Application::toggleFullScreenMode()
{
	setFullScreenMode(!m_mainWindow->m_playerWidget->fullScreenMode());
}

void
Application::setFullScreenMode(bool enabled)
{
	if(enabled != m_mainWindow->m_playerWidget->fullScreenMode()) {
		m_mainWindow->m_playerWidget->setFullScreenMode(enabled);

		KToggleAction *toggleFullScreenAction = static_cast<KToggleAction *>(action(ACT_TOGGLE_FULL_SCREEN));
		toggleFullScreenAction->setChecked(enabled);

		emit fullScreenModeChanged(enabled);
	}
}

void
Application::seekBackward()
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	double position = videoPlayer->position() - SCConfig::seekJumpLength();
	m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
	videoPlayer->seek(position > 0.0 ? position : 0.0);
}

void
Application::seekForward()
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	double position = videoPlayer->position() + SCConfig::seekJumpLength();
	m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
	videoPlayer->seek(position <= videoPlayer->duration() ? position : videoPlayer->duration());
}

void
Application::stepBackward()
{
	VideoPlayer::instance()->step(-SCConfig::stepJumpLength());
}

void
Application::stepForward()
{
	VideoPlayer::instance()->step(SCConfig::stepJumpLength());
}

void
Application::seekToPrevLine()
{
	int selectedIndex = m_mainWindow->m_linesWidget->firstSelectedIndex();
	if(selectedIndex < 0)
		return;
	SubtitleLine *currentLine = appSubtitle()->line(selectedIndex);
	if(currentLine) {
		SubtitleLine *prevLine = currentLine->prevLine();
		if(prevLine) {
			m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
			VideoPlayer::instance()->seek(prevLine->showTime().toSeconds() - SCConfig::jumpLineOffset() / 1000.0);
			m_mainWindow->m_linesWidget->setCurrentLine(prevLine);
			m_mainWindow->m_curLineWidget->setCurrentLine(prevLine);
		}
	}
}

void
Application::playOnlyCurrentLine()
{
	int selectedIndex = m_mainWindow->m_linesWidget->firstSelectedIndex();
	if(selectedIndex < 0)
		return;
	SubtitleLine *currentLine = appSubtitle()->line(selectedIndex);
	if(currentLine) {
		VideoPlayer *videoPlayer = VideoPlayer::instance();
		if(!videoPlayer->isPlaying())
			videoPlayer->play();
		videoPlayer->seek(currentLine->showTime().toSeconds() - SCConfig::jumpLineOffset() / 1000.0);
		m_mainWindow->m_playerWidget->pauseAfterPlayingLine(currentLine);
	}
}

void
Application::seekToCurrentLine()
{
	int selectedIndex = m_mainWindow->m_linesWidget->firstSelectedIndex();
	if(selectedIndex < 0)
		return;
	SubtitleLine *currentLine = appSubtitle()->line(selectedIndex);
	if(currentLine) {
		m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
		VideoPlayer::instance()->seek(currentLine->showTime().toSeconds() - SCConfig::jumpLineOffset() / 1000.0);
	}
}

void
Application::seekToNextLine()
{
	int selectedIndex = m_mainWindow->m_linesWidget->firstSelectedIndex();
	if(selectedIndex < 0)
		return;
	SubtitleLine *currentLine = appSubtitle()->line(selectedIndex);
	if(currentLine) {
		SubtitleLine *nextLine = currentLine->nextLine();
		if(nextLine) {
			m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
			VideoPlayer::instance()->seek(nextLine->showTime().toSeconds() - SCConfig::jumpLineOffset() / 1000.0);
			m_mainWindow->m_linesWidget->setCurrentLine(nextLine);
			m_mainWindow->m_curLineWidget->setCurrentLine(nextLine);
		}
	}
}

void
Application::playrateIncrease()
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	const double speed = videoPlayer->playSpeed();
	videoPlayer->playSpeed(speed + (speed >= 2.0 ? .5 : .1));
}

void
Application::playrateDecrease()
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	const double speed = videoPlayer->playSpeed();
	videoPlayer->playSpeed(speed - (speed > 2.0 ? .5 : .1));
}

void
Application::setCurrentLineShowTimeFromVideo()
{
	SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
	if(currentLine)
		currentLine->setShowTime(videoPosition(true));
}

void
Application::setCurrentLineHideTimeFromVideo()
{
	SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
	if(currentLine)
		currentLine->setHideTime(videoPosition(true));
}

void
Application::setActiveSubtitleStream(int subtitleStream)
{
	KSelectAction *activeSubtitleStreamAction = (KSelectAction *)action(ACT_SET_ACTIVE_SUBTITLE_STREAM);
	activeSubtitleStreamAction->setCurrentItem(subtitleStream);

	const bool translationSelected = bool(subtitleStream);
	m_mainWindow->m_playerWidget->setShowTranslation(translationSelected);
	m_mainWindow->m_waveformWidget->setShowTranslation(translationSelected);
	m_speller->setUseTranslation(translationSelected);
}


void
Application::anchorToggle()
{
	appSubtitle()->toggleLineAnchor(m_mainWindow->m_linesWidget->currentLine());
}

void
Application::anchorRemoveAll()
{
	appSubtitle()->removeAllAnchors();
}

void
Application::shiftToVideoPosition()
{
	SubtitleLine *currentLine = m_mainWindow->m_linesWidget->currentLine();
	if(currentLine)
		appSubtitle()->shiftLines(Range::full(), videoPosition(true).toMillis() - currentLine->showTime().toMillis());
}

/// END ACTION HANDLERS

void
Application::updateTitle()
{
	if(appSubtitle()) {
		if(m_translationMode) {
			const QString caption = QStringLiteral("%1%2 | %3").arg(
				m_subtitleUrl.isEmpty() ? i18n("Untitled") : m_subtitleFileName,
				appSubtitle()->isPrimaryDirty() ? $(" *") : QString(),
				m_subtitleTrUrl.isEmpty() ? i18n("Untitled Translation") : m_subtitleTrFileName
			);
			m_mainWindow->setCaption(caption, appSubtitle()->isSecondaryDirty());
		} else {
			m_mainWindow->setCaption(
						m_subtitleUrl.isEmpty() ? i18n("Untitled") : (m_subtitleUrl.isLocalFile() ? m_subtitleUrl.toLocalFile() : m_subtitleUrl.toString(QUrl::PreferLocalFile)),
						appSubtitle()->isPrimaryDirty());
		}
	} else {
		m_mainWindow->setCaption(QString());
	}
}

void
Application::onWaveformDoubleClicked(Time time)
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	if(videoPlayer->state() == VideoPlayer::Stopped)
		videoPlayer->play();

	m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
	videoPlayer->seek(time.toSeconds());
}

void
Application::onWaveformMiddleMouse(Time time)
{
	m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
	VideoPlayer::instance()->seek(time.toSeconds());
}

void
Application::onLineDoubleClicked(SubtitleLine *line)
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	if(videoPlayer->state() == VideoPlayer::Stopped)
		videoPlayer->play();

	int mseconds = line->showTime().toMillis() - SCConfig::seekOffsetOnDoubleClick();
	m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
	videoPlayer->seek(mseconds > 0 ? mseconds / 1000.0 : 0.0);

	if(videoPlayer->state() != VideoPlayer::Playing && SCConfig::unpauseOnDoubleClick())
		videoPlayer->play();

	m_mainWindow->m_waveformWidget->setScrollPosition(line->showTime().toMillis());
}

void
Application::onHighlightLine(SubtitleLine *line, bool primary, int firstIndex, int lastIndex)
{
	if(m_mainWindow->m_playerWidget->fullScreenMode()) {
		if(m_lastFoundLine != line) {
			m_lastFoundLine = line;

			m_mainWindow->m_playerWidget->pauseAfterPlayingLine(nullptr);
			VideoPlayer::instance()->seek(line->showTime().toSeconds());
		}
	} else {
		m_mainWindow->m_linesWidget->setCurrentLine(line, true);

		if(firstIndex >= 0 && lastIndex >= 0) {
			if(primary)
				m_mainWindow->m_curLineWidget->selectPrimaryText(firstIndex, lastIndex);
			else
				m_mainWindow->m_curLineWidget->selectTranslationText(firstIndex, lastIndex);
		}
	}
}

void
Application::onPlayingLineChanged(SubtitleLine *line)
{
	m_mainWindow->m_linesWidget->setPlayingLine(line);

	if(m_linkCurrentLineToPosition)
		m_mainWindow->m_linesWidget->setCurrentLine(line, true);
}

void
Application::onLinkCurrentLineToVideoToggled(bool value)
{
	if(m_linkCurrentLineToPosition != value) {
		m_linkCurrentLineToPosition = value;

		if(m_linkCurrentLineToPosition)
			m_mainWindow->m_linesWidget->setCurrentLine(m_mainWindow->m_playerWidget->playingLine(), true);
	}
}

void
Application::onPlayerFileOpened(const QString &filePath)
{
	m_recentVideosAction->addUrl(QUrl::fromLocalFile(filePath));
}

void
Application::onPlayerPlaying()
{
	QAction *playPauseAction = action(ACT_PLAY_PAUSE);
	playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
	playPauseAction->setText(i18n("Pause"));
}

void
Application::onPlayerPaused()
{
	QAction *playPauseAction = action(ACT_PLAY_PAUSE);
	playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
	playPauseAction->setText(i18n("Play"));
}

void
Application::onPlayerStopped()
{
	onPlayerPaused();
}

void
Application::onPlayerTextStreamsChanged(const QStringList &textStreams)
{
	QAction *demuxTextStreamAction = (KSelectAction *)action(ACT_DEMUX_TEXT_STREAM);
	QMenu *menu = demuxTextStreamAction->menu();
	menu->clear();
	int i = 0;
	foreach(const QString &textStream, textStreams)
		menu->addAction(textStream)->setData(QVariant::fromValue<int>(i++));
	demuxTextStreamAction->setEnabled(i > 0);
}

void
Application::onPlayerAudioStreamsChanged(const QStringList &audioStreams)
{
	KSelectAction *activeAudioStreamAction = (KSelectAction *)action(ACT_SET_ACTIVE_AUDIO_STREAM);
	activeAudioStreamAction->setItems(audioStreams);

	QAction *speechImportStreamAction = (KSelectAction *)action(ACT_ASR_IMPORT_AUDIO_STREAM);
	QMenu *speechImportStreamActionMenu = speechImportStreamAction->menu();
	speechImportStreamActionMenu->clear();
	int i = 0;
	foreach(const QString &audioStream, audioStreams)
		speechImportStreamActionMenu->addAction(audioStream)->setData(QVariant::fromValue<int>(i++));
	speechImportStreamAction->setEnabled(i > 0);
}

void
Application::onPlayerActiveAudioStreamChanged(int audioStream)
{
	KSelectAction *activeAudioStreamAction = (KSelectAction *)action(ACT_SET_ACTIVE_AUDIO_STREAM);
	if(audioStream >= 0) {
		activeAudioStreamAction->setCurrentItem(audioStream);
		m_mainWindow->m_waveformWidget->setAudioStream(VideoPlayer::instance()->filePath(), audioStream);
	} else {
		m_mainWindow->m_waveformWidget->setNullAudioStream(VideoPlayer::instance()->duration() * 1000);
	}
}

void
Application::onPlayerMuteChanged(bool muted)
{
	KToggleAction *toggleMutedAction = (KToggleAction *)action(ACT_TOGGLE_MUTED);
	toggleMutedAction->setChecked(muted);
}

void
Application::updateActionTexts()
{
	const int shiftAmount = SCConfig::linesQuickShiftAmount();
	const int jumpLength = SCConfig::seekJumpLength();

	action(ACT_SEEK_BACKWARD)->setStatusTip(i18np("Seek backwards 1 second", "Seek backwards %1 seconds", jumpLength));
	action(ACT_SEEK_FORWARD)->setStatusTip(i18np("Seek forwards 1 second", "Seek forwards %1 seconds", jumpLength));

	QAction *shiftSelectedLinesFwdAction = action(ACT_SHIFT_SELECTED_LINES_FORWARDS);
	shiftSelectedLinesFwdAction->setText(i18np("Shift %2%1 millisecond", "Shift %2%1 milliseconds", shiftAmount, "+"));
	shiftSelectedLinesFwdAction->setStatusTip(i18np("Shift selected lines %2%1 millisecond", "Shift selected lines %2%1 milliseconds", shiftAmount, "+"));

	QAction *shiftSelectedLinesBwdAction = action(ACT_SHIFT_SELECTED_LINES_BACKWARDS);
	shiftSelectedLinesBwdAction->setText(i18np("Shift %2%1 millisecond", "Shift %2%1 milliseconds", shiftAmount, "-"));
	shiftSelectedLinesBwdAction->setStatusTip(i18np("Shift selected lines %2%1 millisecond", "Shift selected lines %2%1 milliseconds", shiftAmount, "-"));
}

void
Application::onConfigChanged()
{
	updateActionTexts();
}

