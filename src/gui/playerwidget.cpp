/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "playerwidget.h"

#include "appglobal.h"
#include "application.h"
#include "actions/useractionnames.h"
#include "core/richtext/richdocument.h"
#include "videoplayer/videoplayer.h"
#include "widgets/layeredwidget.h"
#include "widgets/attachablewidget.h"
#include "widgets/pointingslider.h"
#include "widgets/timeedit.h"

#include <QEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>

#include <QMenu>
#include <QPushButton>
#include <QCursor>
#include <QLabel>
#include <QToolButton>
#include <QGroupBox>
#include <QGridLayout>

#include <KConfigGroup>
#include <KMessageBox>
#include <KLocalizedString>

using namespace SubtitleComposer;

#define HIDE_MOUSE_MSECS 1000
#define UNKNOWN_LENGTH_STRING (" / " + Time().toString(false) + ' ')

PlayerWidget::PlayerWidget(QWidget *parent) :
	QWidget(parent),
	m_subtitle(0),
	m_translationMode(false),
	m_showTranslation(false),
	m_pauseAfterPlayingLine(nullptr),
	m_fullScreenTID(0),
	m_fullScreenMode(false),
	m_lengthString(UNKNOWN_LENGTH_STRING),
	m_showPositionTimeEdit(SCConfig::showPositionTimeEdit())
{
	m_layeredWidget = new LayeredWidget(this);
	m_layeredWidget->setAcceptDrops(true);
	m_layeredWidget->installEventFilter(this);
	m_layeredWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_seekSlider = new PointingSlider(Qt::Horizontal, this);
	m_seekSlider->setTickPosition(QSlider::NoTicks);
	m_seekSlider->setMinimum(0);
	m_seekSlider->setMaximum(1000);
	m_seekSlider->setPageStep(10);
	m_seekSlider->setFocusPolicy(Qt::NoFocus);

	m_infoControlsGroupBox = new QWidget(this);
	m_infoControlsGroupBox->setAcceptDrops(true);
	m_infoControlsGroupBox->installEventFilter(this);

	QLabel *positionTagLabel = new QLabel(m_infoControlsGroupBox);
	positionTagLabel->setText(i18n("<b>Position</b>"));
	positionTagLabel->installEventFilter(this);

	m_positionLabel = new QLabel(m_infoControlsGroupBox);
	m_positionEdit = new TimeEdit(m_infoControlsGroupBox);
	m_positionEdit->setFocusPolicy(Qt::NoFocus);

	QLabel *lengthTagLabel = new QLabel(m_infoControlsGroupBox);
	lengthTagLabel->setText(i18n("<b>Length</b>"));
	lengthTagLabel->installEventFilter(this);
	m_lengthLabel = new QLabel(m_infoControlsGroupBox);

	QLabel *fpsTagLabel = new QLabel(m_infoControlsGroupBox);
	fpsTagLabel->setText(i18n("<b>FPS</b>"));
	fpsTagLabel->installEventFilter(this);
	m_fpsLabel = new QLabel(m_infoControlsGroupBox);
	m_fpsLabel->setMinimumWidth(m_positionEdit->sizeHint().width());        // sets the minimum width for the whole group

	QLabel *rateTagLabel = new QLabel(m_infoControlsGroupBox);
	rateTagLabel->setText(i18n("<b>Playback Rate</b>"));
	rateTagLabel->installEventFilter(this);
	m_rateLabel = new QLabel(m_infoControlsGroupBox);

	m_volumeSlider = new PointingSlider(Qt::Vertical, this);
	m_volumeSlider->setFocusPolicy(Qt::NoFocus);
	m_volumeSlider->setTickPosition(QSlider::NoTicks);
	m_volumeSlider->setPageStep(5);
	m_volumeSlider->setMinimum(0);
	m_volumeSlider->setMaximum(100);
	m_volumeSlider->setFocusPolicy(Qt::NoFocus);

	QGridLayout *videoControlsLayout = new QGridLayout();
	videoControlsLayout->setContentsMargins(0, 0, 0, 0);
	videoControlsLayout->setSpacing(2);
	videoControlsLayout->addWidget(createToolButton(this, ACT_PLAY_PAUSE, 16), 0, 0);
	videoControlsLayout->addWidget(createToolButton(this, ACT_STOP, 16), 0, 1);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SEEK_BACKWARD, 16), 0, 2);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SEEK_FORWARD, 16), 0, 3);
	videoControlsLayout->addItem(new QSpacerItem(2, 2), 0, 4);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SEEK_TO_PREVIOUS_LINE, 16), 0, 5);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SEEK_TO_NEXT_LINE, 16), 0, 6);
	videoControlsLayout->addItem(new QSpacerItem(2, 2), 0, 7);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SET_CURRENT_LINE_SHOW_TIME, 16), 0, 8);
	videoControlsLayout->addWidget(createToolButton(this, ACT_SET_CURRENT_LINE_HIDE_TIME, 16), 0, 9);
	videoControlsLayout->addItem(new QSpacerItem(2, 2), 0, 10);
	videoControlsLayout->addWidget(createToolButton(this, ACT_CURRENT_LINE_FOLLOWS_VIDEO, 16), 0, 11);
	videoControlsLayout->addItem(new QSpacerItem(2, 2), 0, 12);
	videoControlsLayout->addWidget(createToolButton(this, ACT_PLAY_RATE_DECREASE, 16), 0, 13);
	videoControlsLayout->addWidget(createToolButton(this, ACT_PLAY_RATE_INCREASE, 16), 0, 14);
	videoControlsLayout->addWidget(m_seekSlider, 0, 15);

	QGridLayout *audioControlsLayout = new QGridLayout();
	audioControlsLayout->setContentsMargins(0, 0, 0, 0);
	audioControlsLayout->addWidget(createToolButton(this, ACT_TOGGLE_MUTED, 16), 0, 0, Qt::AlignHCenter);
	audioControlsLayout->addWidget(m_volumeSlider, 1, 0, Qt::AlignHCenter);

	QGridLayout *infoControlsLayout = new QGridLayout(m_infoControlsGroupBox);
	infoControlsLayout->setSpacing(5);
	infoControlsLayout->addWidget(fpsTagLabel, 0, 0);
	infoControlsLayout->addWidget(m_fpsLabel, 1, 0);
	infoControlsLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding), 2, 0);
	infoControlsLayout->addWidget(rateTagLabel, 3, 0);
	infoControlsLayout->addWidget(m_rateLabel, 4, 0);
	infoControlsLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding), 5, 0);
	infoControlsLayout->addWidget(lengthTagLabel, 6, 0);
	infoControlsLayout->addWidget(m_lengthLabel, 7, 0);
	infoControlsLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding), 8, 0);
	infoControlsLayout->addWidget(positionTagLabel, 9, 0);
	infoControlsLayout->addWidget(m_positionLabel, 10, 0);
	infoControlsLayout->addWidget(m_positionEdit, 11, 0);

	m_mainLayout = new QGridLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(5);
	m_mainLayout->addWidget(m_infoControlsGroupBox, 0, 0, 2, 1);
	m_mainLayout->addWidget(m_layeredWidget, 0, 1);
	m_mainLayout->addLayout(audioControlsLayout, 0, 2);
	m_mainLayout->addLayout(videoControlsLayout, 1, 1);
	m_mainLayout->addWidget(createToolButton(this, ACT_TOGGLE_FULL_SCREEN, 16), 1, 2);

	m_fullScreenControls = new AttachableWidget(AttachableWidget::Bottom, 4);
	m_fullScreenControls->setAutoFillBackground(true);
	m_layeredWidget->setWidgetMode(m_fullScreenControls, LayeredWidget::IgnoreResize);

	m_fsSeekSlider = new PointingSlider(Qt::Horizontal, m_fullScreenControls);
	m_fsSeekSlider->setTickPosition(QSlider::NoTicks);
	m_fsSeekSlider->setMinimum(0);
	m_fsSeekSlider->setMaximum(1000);
	m_fsSeekSlider->setPageStep(10);

	m_fsVolumeSlider = new PointingSlider(Qt::Horizontal, m_fullScreenControls);
	m_fsVolumeSlider->setFocusPolicy(Qt::NoFocus);
	m_fsVolumeSlider->setTickPosition(QSlider::NoTicks);
	m_fsVolumeSlider->setPageStep(5);
	m_fsVolumeSlider->setMinimum(0);
	m_fsVolumeSlider->setMaximum(100);

	m_fsPositionLabel = new QLabel(m_fullScreenControls);
	QPalette fsPositionPalette;
	fsPositionPalette.setColor(m_fsPositionLabel->backgroundRole(), Qt::black);
	fsPositionPalette.setColor(m_fsPositionLabel->foregroundRole(), Qt::white);
	m_fsPositionLabel->setPalette(fsPositionPalette);
	m_fsPositionLabel->setAutoFillBackground(true);
	m_fsPositionLabel->setFrameShape(QFrame::Panel);
	m_fsPositionLabel->setText(Time().toString(false) + " /  " + Time().toString(false));
	m_fsPositionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_fsPositionLabel->adjustSize();
	m_fsPositionLabel->setMinimumWidth(m_fsPositionLabel->width());

	QHBoxLayout *fullScreenControlsLayout = new QHBoxLayout(m_fullScreenControls);
	fullScreenControlsLayout->setContentsMargins(0, 0, 0, 0);
	fullScreenControlsLayout->setSpacing(0);

	const int FS_BUTTON_SIZE = 32;
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_PLAY_PAUSE, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_STOP, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_SEEK_BACKWARD, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_SEEK_FORWARD, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addSpacing(3);
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_SEEK_TO_PREVIOUS_LINE, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_SEEK_TO_NEXT_LINE, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addSpacing(3);
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_PLAY_RATE_DECREASE, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_PLAY_RATE_INCREASE, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addSpacing(3);
	fullScreenControlsLayout->addWidget(m_fsSeekSlider, 9);
	fullScreenControlsLayout->addWidget(m_fsPositionLabel);
	fullScreenControlsLayout->addWidget(m_fsVolumeSlider, 2);
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_TOGGLE_MUTED, FS_BUTTON_SIZE));
	fullScreenControlsLayout->addWidget(createToolButton(m_fullScreenControls, ACT_TOGGLE_FULL_SCREEN, FS_BUTTON_SIZE));
	m_fullScreenControls->adjustSize();

	connect(m_volumeSlider, &QAbstractSlider::valueChanged, this, &PlayerWidget::onVolumeSliderMoved);
	connect(m_fsVolumeSlider, &QAbstractSlider::valueChanged, this, &PlayerWidget::onVolumeSliderMoved);

	connect(m_seekSlider, &QAbstractSlider::valueChanged, this, &PlayerWidget::onSeekSliderMoved);
	connect(m_fsSeekSlider, &QAbstractSlider::valueChanged, this, &PlayerWidget::onSeekSliderMoved);

	connect(m_positionEdit, &TimeEdit::valueChanged, this, &PlayerWidget::onPositionEditValueChanged);
	connect(m_positionEdit, &TimeEdit::valueEntered, this, &PlayerWidget::onPositionEditValueChanged);

	connect(SCConfig::self(), &KCoreConfigSkeleton::configChanged, this, &PlayerWidget::onConfigChanged);

	VideoPlayer *videoPlayer = VideoPlayer::instance();
	videoPlayer->init(m_layeredWidget);

	connect(videoPlayer, &VideoPlayer::fileOpened, this, &PlayerWidget::onPlayerFileOpened);
	connect(videoPlayer, &VideoPlayer::fileOpenError, this, &PlayerWidget::onPlayerFileOpenError);
	connect(videoPlayer, &VideoPlayer::fileClosed, this, &PlayerWidget::onPlayerFileClosed);
	connect(videoPlayer, &VideoPlayer::playbackError, this, &PlayerWidget::onPlayerPlaybackError);
	connect(videoPlayer, &VideoPlayer::playing, this, &PlayerWidget::onPlayerPlaying);
	connect(videoPlayer, &VideoPlayer::stopped, this, &PlayerWidget::onPlayerStopped);
	connect(videoPlayer, &VideoPlayer::positionChanged, this, &PlayerWidget::onPlayerPositionChanged);
	connect(videoPlayer, &VideoPlayer::durationChanged, this, &PlayerWidget::onPlayerLengthChanged);
	connect(videoPlayer, &VideoPlayer::fpsChanged, this, &PlayerWidget::onPlayerFramesPerSecondChanged);
	connect(videoPlayer, &VideoPlayer::playSpeedChanged, this, &PlayerWidget::onPlayerPlaybackRateChanged);
	connect(videoPlayer, &VideoPlayer::volumeChanged, this, &PlayerWidget::onPlayerVolumeChanged);
	connect(videoPlayer, &VideoPlayer::muteChanged, m_fsVolumeSlider, &QWidget::setDisabled);
	connect(videoPlayer, &VideoPlayer::muteChanged, m_volumeSlider, &QWidget::setDisabled);

	connect(videoPlayer, &VideoPlayer::leftClicked, this, &PlayerWidget::onPlayerLeftClicked);
	connect(videoPlayer, &VideoPlayer::rightClicked, this, &PlayerWidget::onPlayerRightClicked);
	connect(videoPlayer, &VideoPlayer::doubleClicked, this, &PlayerWidget::onPlayerDoubleClicked);

	onPlayerFileClosed();
	onConfigChanged();    // initializes the font

	setFullScreenMode(m_fullScreenMode);

	connect(app(), &Application::actionsReady, this, [this](){
		toolButton(this, ACT_STOP)->setDefaultAction(app()->action(ACT_STOP));
		toolButton(this, ACT_PLAY_PAUSE)->setDefaultAction(app()->action(ACT_PLAY_PAUSE));
		toolButton(this, ACT_SEEK_BACKWARD)->setDefaultAction(app()->action(ACT_SEEK_BACKWARD));
		toolButton(this, ACT_SEEK_FORWARD)->setDefaultAction(app()->action(ACT_SEEK_FORWARD));
		toolButton(this, ACT_SEEK_TO_PREVIOUS_LINE)->setDefaultAction(app()->action(ACT_SEEK_TO_PREVIOUS_LINE));
		toolButton(this, ACT_SEEK_TO_NEXT_LINE)->setDefaultAction(app()->action(ACT_SEEK_TO_NEXT_LINE));
		toolButton(this, ACT_SET_CURRENT_LINE_SHOW_TIME)->setDefaultAction(app()->action(ACT_SET_CURRENT_LINE_SHOW_TIME));
		toolButton(this, ACT_SET_CURRENT_LINE_HIDE_TIME)->setDefaultAction(app()->action(ACT_SET_CURRENT_LINE_HIDE_TIME));
		toolButton(this, ACT_CURRENT_LINE_FOLLOWS_VIDEO)->setDefaultAction(app()->action(ACT_CURRENT_LINE_FOLLOWS_VIDEO));
		toolButton(this, ACT_TOGGLE_MUTED)->setDefaultAction(app()->action(ACT_TOGGLE_MUTED));
		toolButton(this, ACT_TOGGLE_FULL_SCREEN)->setDefaultAction(app()->action(ACT_TOGGLE_FULL_SCREEN));
		toolButton(this, ACT_PLAY_RATE_DECREASE)->setDefaultAction(app()->action(ACT_PLAY_RATE_DECREASE));
		toolButton(this, ACT_PLAY_RATE_INCREASE)->setDefaultAction(app()->action(ACT_PLAY_RATE_INCREASE));

		toolButton(m_fullScreenControls, ACT_STOP)->setDefaultAction(app()->action(ACT_STOP));
		toolButton(m_fullScreenControls, ACT_PLAY_PAUSE)->setDefaultAction(app()->action(ACT_PLAY_PAUSE));
		toolButton(m_fullScreenControls, ACT_SEEK_BACKWARD)->setDefaultAction(app()->action(ACT_SEEK_BACKWARD));
		toolButton(m_fullScreenControls, ACT_SEEK_FORWARD)->setDefaultAction(app()->action(ACT_SEEK_FORWARD));
		toolButton(m_fullScreenControls, ACT_SEEK_TO_PREVIOUS_LINE)->setDefaultAction(app()->action(ACT_SEEK_TO_PREVIOUS_LINE));
		toolButton(m_fullScreenControls, ACT_SEEK_TO_NEXT_LINE)->setDefaultAction(app()->action(ACT_SEEK_TO_NEXT_LINE));
		toolButton(m_fullScreenControls, ACT_TOGGLE_MUTED)->setDefaultAction(app()->action(ACT_TOGGLE_MUTED));
		toolButton(m_fullScreenControls, ACT_TOGGLE_FULL_SCREEN)->setDefaultAction(app()->action(ACT_TOGGLE_FULL_SCREEN));
		toolButton(m_fullScreenControls, ACT_PLAY_RATE_DECREASE)->setDefaultAction(app()->action(ACT_PLAY_RATE_DECREASE));
		toolButton(m_fullScreenControls, ACT_PLAY_RATE_INCREASE)->setDefaultAction(app()->action(ACT_PLAY_RATE_INCREASE));
	});
}

PlayerWidget::~PlayerWidget()
{
	m_fullScreenControls->deleteLater();
}

QWidget *
PlayerWidget::infoSidebarWidget()
{
	return m_infoControlsGroupBox;
}

QToolButton *
PlayerWidget::toolButton(QWidget *parent, const char *name)
{
	return parent->findChild<QToolButton *>(name);
}

QToolButton *
PlayerWidget::createToolButton(QWidget *parent, const char *name, int size)
{
	QToolButton *toolButton = new QToolButton(parent);
	toolButton->setObjectName(name);
	toolButton->setMinimumSize(size, size);
	toolButton->setIconSize(size >= 32 ? QSize(size - 6, size - 6) : QSize(size, size));
	toolButton->setAutoRaise(true);
	toolButton->setFocusPolicy(Qt::NoFocus);
	return toolButton;
}

void
PlayerWidget::loadConfig()
{
	onPlayerVolumeChanged(VideoPlayer::instance()->volume());
}

void
PlayerWidget::saveConfig()
{}

void
PlayerWidget::setFullScreenMode(bool fullScreenMode)
{
	if(m_fullScreenMode == fullScreenMode)
		return;

	m_fullScreenMode = fullScreenMode;

	if(m_fullScreenMode) {
		window()->hide();

		// Move m_layeredWidget to a temporary widget which will be
		// displayed in full screen mode.
		// Can not call showFullScreen() on m_layeredWidget directly
		// because restoring the previous state is buggy under
		// some desktop environments / window managers.

		auto *fullScreenWidget = new QWidget();
		fullScreenWidget->installEventFilter(this);
		auto *fullScreenLayout = new QHBoxLayout();
		fullScreenLayout->setContentsMargins(0, 0, 0, 0);
		fullScreenWidget->setLayout(fullScreenLayout);
		m_layeredWidget->setParent(fullScreenWidget);
		fullScreenLayout->addWidget(m_layeredWidget);
		fullScreenWidget->showFullScreen();

		m_layeredWidget->unsetCursor();
		m_layeredWidget->setMouseTracking(true);
		m_fullScreenControls->attach(m_layeredWidget);

		m_fullScreenTID = startTimer(HIDE_MOUSE_MSECS);

		VideoPlayer::instance()->subtitleOverlay().setBottomPadding(m_fullScreenControls->height());
	} else {
		if(m_fullScreenTID) {
			killTimer(m_fullScreenTID);
			m_fullScreenTID = 0;
		}

		m_fullScreenControls->dettach();
		m_layeredWidget->setMouseTracking(false);
		m_layeredWidget->unsetCursor();

		// delete temporary parent widget later and set this as parent again
		m_layeredWidget->parent()->deleteLater();
		m_layeredWidget->setParent(this);

		m_mainLayout->addWidget(m_layeredWidget, 0, 1);

		VideoPlayer::instance()->subtitleOverlay().setBottomPadding(0);

		window()->show();
	}
}

void
PlayerWidget::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event);
	if(m_currentCursorPos != m_savedCursorPos) {
		m_savedCursorPos = m_currentCursorPos;
	} else if(!m_fullScreenControls->underMouse()) {
		if(m_layeredWidget->cursor().shape() != Qt::BlankCursor)
			m_layeredWidget->setCursor(QCursor(Qt::BlankCursor));
		if(m_fullScreenControls->isAttached())
			m_fullScreenControls->toggleVisible(false);
	}
}

bool
PlayerWidget::eventFilter(QObject *object, QEvent *event)
{
	if(object == m_layeredWidget) {
		switch(event->type()) {
		case QEvent::DragEnter:
		case QEvent::Drop:
			foreach(const QUrl &url, static_cast<QDropEvent *>(event)->mimeData()->urls()) {
				if(url.scheme() == QLatin1String("file")) {
					event->accept();
					if(event->type() == QEvent::Drop)
						app()->openVideo(url);
					return true; // eat event
				}
			}
			event->ignore();
			return true;

		case QEvent::DragMove:
			return true; // eat event

		case QEvent::KeyPress: {
			// NOTE: when on full screen mode, the keyboard input is received but
			// for some reason it doesn't trigger the correct actions automatically
			// so we process the event and handle the issue ourselves.
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
			if(m_fullScreenMode && keyEvent->key() == Qt::Key_Escape) {
				app()->action(ACT_TOGGLE_FULL_SCREEN)->trigger();
				return true;
			}
			return app()->triggerAction(QKeySequence((keyEvent->modifiers() & ~Qt::KeypadModifier) | keyEvent->key()));
		}

		case QEvent::MouseMove: {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			if(mouseEvent->globalPos() != m_currentCursorPos) {
				m_currentCursorPos = mouseEvent->globalPos();
#else
			if(mouseEvent->globalPosition() != m_currentCursorPos) {
				m_currentCursorPos = mouseEvent->globalPosition();
#endif
				if(m_layeredWidget->cursor().shape() == Qt::BlankCursor)
					m_layeredWidget->unsetCursor();
				if(m_fullScreenControls->isAttached())
					m_fullScreenControls->toggleVisible(true);
			}
			break;
		}

		default:
			break;
		}
	} else if(object == m_infoControlsGroupBox || object->parent() == m_infoControlsGroupBox) {
		if(event->type() != QEvent::MouseButtonRelease)
			return false;

		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

		if(mouseEvent->button() != Qt::RightButton)
			return false;

		QMenu menu;
		QAction *action = menu.addAction(i18n("Show editable position control"));
		action->setCheckable(true);
		action->setChecked(SCConfig::showPositionTimeEdit());

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		if(menu.exec(mouseEvent->globalPos()) == action)
#else
		if(menu.exec(mouseEvent->globalPosition().toPoint()) == action)
#endif
			SCConfig::setShowPositionTimeEdit(!SCConfig::showPositionTimeEdit());

		return true; // eat event
	} else if(m_fullScreenMode && object == m_layeredWidget->parentWidget() && event->type() == QEvent::Close) {
		app()->action(ACT_TOGGLE_FULL_SCREEN)->trigger();
		event->ignore();
		return true; // eat event
	}

	return false;
}

void
PlayerWidget::setSubtitle(Subtitle *subtitle)
{
	if(m_subtitle) {
		disconnect(m_subtitle.constData(), &Subtitle::linesInserted, this, &PlayerWidget::setPlayingLineFromVideo);
		disconnect(m_subtitle.constData(), &Subtitle::linesRemoved, this, &PlayerWidget::setPlayingLineFromVideo);

		m_subtitle = nullptr;

		setPlayingLine(nullptr);
	}

	m_subtitle = subtitle;

	if(m_subtitle) {
		connect(m_subtitle.constData(), &Subtitle::linesInserted, this, &PlayerWidget::setPlayingLineFromVideo);
		connect(m_subtitle.constData(), &Subtitle::linesRemoved, this, &PlayerWidget::setPlayingLineFromVideo);
	}
}

void
PlayerWidget::setTranslationMode(bool enabled)
{
	m_translationMode = enabled;

	if(!m_translationMode)
		setShowTranslation(false);
}

void
PlayerWidget::setShowTranslation(bool showTranslation)
{
	if(m_showTranslation == showTranslation)
		return;

	m_showTranslation = showTranslation;

	setPlayingLine(nullptr);
	setPlayingLineFromVideo();
}

void
PlayerWidget::increaseFontSize(int size)
{
	SCConfig::setFontSize(SCConfig::fontSize() + size);
	VideoPlayer::instance()->subtitleOverlay().setFontSize(SCConfig::fontSize());
}

void
PlayerWidget::decreaseFontSize(int size)
{
	SCConfig::setFontSize(SCConfig::fontSize() - size);
	VideoPlayer::instance()->subtitleOverlay().setFontSize(SCConfig::fontSize());
}

void
PlayerWidget::updatePlayingLine(const Time &videoPosition)
{
	if(!m_subtitle || m_subtitle->isEmpty()) {
		setPlayingLine(nullptr);
		return;
	}

	if(m_playingLine && m_playingLine->containsTime(videoPosition))
		return; // playing line is still valid

	SubtitleLine *firstLine = m_subtitle->firstLine();
	SubtitleLine *lastLine = m_subtitle->lastLine();
	bool pnValid = true;
	if(m_prevLine ? videoPosition < m_prevLine->showTime() : m_nextLine != firstLine)
		pnValid = false;
	else if(m_nextLine ? videoPosition > m_nextLine->hideTime() : m_prevLine != lastLine)
		pnValid = false;

	if(!pnValid) {
		// prev/next line are invalid
		m_prevLine = m_nextLine = nullptr;
		int first = 0;
		int last = m_subtitle->lastIndex();
		if(videoPosition < firstLine->showTime()) {
			m_nextLine = firstLine;
		} else if(videoPosition > lastLine->hideTime()) {
			m_prevLine = lastLine;
		} else {
			while(last - first > 1) {
				// log2 search lines
				int mid = (first + last) / 2;
				SubtitleLine *line = m_subtitle->at(mid);
				if(videoPosition <= line->hideTime())
					last = mid;
				if(videoPosition >= line->showTime())
					first = mid;
			}
			m_prevLine = m_subtitle->at(first);
			m_nextLine = m_subtitle->at(last);
		}
	}

	if(m_prevLine && m_prevLine->containsTime(videoPosition))
		setPlayingLine(m_prevLine);
	else if(m_nextLine && m_nextLine->containsTime(videoPosition))
		setPlayingLine(m_nextLine);
	else
		setPlayingLine(nullptr);
}

void
PlayerWidget::pauseAfterPlayingLine(const SubtitleLine *line)
{
	m_pauseAfterPlayingLine = line;
}

void
PlayerWidget::setPlayingLineFromVideo()
{
	updatePlayingLine(VideoPlayer::instance()->position() * 1000.);
}

void
PlayerWidget::setPlayingLine(SubtitleLine *line)
{
	if(m_subtitle && line && m_subtitle != line->subtitle())
		line = nullptr;

	if(m_playingLine == line)
		return;

	SubtitleTextOverlay &ovr = VideoPlayer::instance()->subtitleOverlay();
	if(m_playingLine) {
		disconnect(m_playingLine, &SubtitleLine::showTimeChanged, this, &PlayerWidget::setPlayingLineFromVideo);
		disconnect(m_playingLine, &SubtitleLine::hideTimeChanged, this, &PlayerWidget::setPlayingLineFromVideo);
		disconnect(m_playingLine, &SubtitleLine::positionChanged, &ovr, &SubtitleTextOverlay::forceRepaint);
	}

	emit playingLineChanged(m_playingLine = line);

	if(m_playingLine) {
		connect(m_playingLine, &SubtitleLine::showTimeChanged, this, &PlayerWidget::setPlayingLineFromVideo);
		connect(m_playingLine, &SubtitleLine::hideTimeChanged, this, &PlayerWidget::setPlayingLineFromVideo);
		connect(m_playingLine, &SubtitleLine::positionChanged, &ovr, &SubtitleTextOverlay::forceRepaint);
		ovr.setDoc(m_showTranslation ? m_playingLine->secondaryDoc() : m_playingLine->primaryDoc());
		ovr.setDocRect(&m_playingLine->pos());
	} else {
		ovr.setDoc(nullptr);
		ovr.setDocRect(nullptr);
	}
}

void
PlayerWidget::updatePositionEditVisibility()
{
	if(m_showPositionTimeEdit && VideoPlayer::instance()->state() >= VideoPlayer::Playing)
		m_positionEdit->show();
	else
		m_positionEdit->hide();
}

void
PlayerWidget::onVolumeSliderMoved(int value)
{
	VideoPlayer::instance()->setVolume(value);
}

void
PlayerWidget::onSeekSliderMoved(int value)
{
	VideoPlayer *videoPlayer = VideoPlayer::instance();
	pauseAfterPlayingLine(nullptr);
	videoPlayer->seek(videoPlayer->duration() * value / 1000.0);

	Time time((long)(videoPlayer->duration() * value));

	m_positionLabel->setText(time.toString());
	m_fsPositionLabel->setText(time.toString(false) + m_lengthString);

	if(m_showPositionTimeEdit)
		m_positionEdit->setValue(time.toMillis());
}

void
PlayerWidget::onPositionEditValueChanged(int position)
{
	if(m_positionEdit->hasFocus()) {
		pauseAfterPlayingLine(nullptr);
		VideoPlayer::instance()->seek(position / 1000.0);
	}
}

void
PlayerWidget::onConfigChanged()
{
	if(m_showPositionTimeEdit != SCConfig::showPositionTimeEdit()) {
		m_showPositionTimeEdit = SCConfig::showPositionTimeEdit();
		updatePositionEditVisibility();
	}

	SubtitleTextOverlay &subtitleOverlay = VideoPlayer::instance()->subtitleOverlay();
	subtitleOverlay.setTextColor(SCConfig::fontColor());
	subtitleOverlay.setFontFamily(SCConfig::fontFamily());
	subtitleOverlay.setFontSize(SCConfig::fontSize());
	subtitleOverlay.setOutlineColor(SCConfig::outlineColor());
	subtitleOverlay.setOutlineWidth(SCConfig::outlineWidth());
}

void
PlayerWidget::onPlayerFileOpened(const QString & /*filePath */)
{
	m_infoControlsGroupBox->setEnabled(true);

	updatePositionEditVisibility();
}

void
PlayerWidget::onPlayerFileOpenError(const QString &filePath, const QString &reason)
{
	QString message = i18n("<qt>There was an error opening media file %1.</qt>", filePath);
	if(!reason.isEmpty())
		message += "\n" + reason;
	KMessageBox::error(this, message);
}

void
PlayerWidget::onPlayerFileClosed()
{
	setPlayingLine(nullptr);

	m_infoControlsGroupBox->setEnabled(false);

	updatePositionEditVisibility();
	m_positionEdit->setValue(0);

	m_positionLabel->setText(i18n("<i>Unknown</i>"));
	m_lengthLabel->setText(i18n("<i>Unknown</i>"));
	m_fpsLabel->setText(i18n("<i>Unknown</i>"));
	m_rateLabel->setText(i18n("<i>Unknown</i>"));

	m_lengthString = UNKNOWN_LENGTH_STRING;
	m_fsPositionLabel->setText(Time().toString(false) + m_lengthString);

	m_seekSlider->setEnabled(false);
	m_fsSeekSlider->setEnabled(false);
}

void
PlayerWidget::onPlayerPlaybackError(const QString &errorMessage)
{
	if(errorMessage.isEmpty())
		KMessageBox::error(this, i18n("Unexpected error when playing file."), i18n("Error Playing File"));
	else
		KMessageBox::detailedError(this, i18n("Unexpected error when playing file."), errorMessage, i18n("Error Playing File"));
}

void
PlayerWidget::onPlayerPlaying()
{
	m_seekSlider->setEnabled(true);
	m_fsSeekSlider->setEnabled(true);

	updatePositionEditVisibility();
}

void
PlayerWidget::onPlayerPositionChanged(double seconds)
{
	const Time videoPosition(seconds * 1000.);

	// pause if requested
	if(m_pauseAfterPlayingLine) {
		const Time &pauseTime = m_pauseAfterPlayingLine->hideTime();
		if(videoPosition >= pauseTime) {
			VideoPlayer *videoPlayer = VideoPlayer::instance();
			m_pauseAfterPlayingLine = nullptr;
			videoPlayer->pause();
			videoPlayer->seek(pauseTime.toSeconds());
			return;
		}
	}

	m_positionLabel->setText(videoPosition.toString());
	m_fsPositionLabel->setText(videoPosition.toString(false) + m_lengthString);

	if(m_showPositionTimeEdit && !m_positionEdit->hasFocus()) {
		QSignalBlocker sb(m_positionEdit);
		m_positionEdit->setValue(videoPosition.toMillis());
	}

	updatePlayingLine(videoPosition);

	QSignalBlocker s1(m_seekSlider), s2(m_fsSeekSlider);
	const int sliderValue = int((seconds / VideoPlayer::instance()->duration()) * 1000.0);
	m_seekSlider->setValue(sliderValue);
	m_fsSeekSlider->setValue(sliderValue);
}

void
PlayerWidget::onPlayerLengthChanged(double seconds)
{
	if(seconds > 0) {
		m_lengthLabel->setText(Time((long)(seconds * 1000)).toString());
		m_lengthString = " / " + m_lengthLabel->text().left(8) + ' ';
	} else {
		m_lengthLabel->setText(i18n("<i>Unknown</i>"));
		m_lengthString = UNKNOWN_LENGTH_STRING;
	}
}

void
PlayerWidget::onPlayerFramesPerSecondChanged(double fps)
{
	m_fpsLabel->setText(fps > 0 ? QString::number(fps, 'f', 3) : i18n("<i>Unknown</i>"));
}

void
PlayerWidget::onPlayerPlaybackRateChanged(double rate)
{
	m_rateLabel->setText(rate > .0 ? QStringLiteral("%1x").arg(rate, 0, 'g', 3) : i18n("<i>Unknown</i>"));
}

void
PlayerWidget::onPlayerStopped()
{
	onPlayerPositionChanged(0);

	m_seekSlider->setEnabled(false);
	m_fsSeekSlider->setEnabled(false);

	setPlayingLine(nullptr);

	updatePositionEditVisibility();
}

void
PlayerWidget::onPlayerVolumeChanged(double volume)
{
	QSignalBlocker s1(m_volumeSlider), s2(m_fsVolumeSlider);
	m_volumeSlider->setValue(int(volume + 0.5));
	m_fsVolumeSlider->setValue(int(volume + 0.5));
}

void
PlayerWidget::onPlayerLeftClicked(const QPointF &point)
{
	Q_UNUSED(point);
	VideoPlayer::instance()->togglePlayPaused();
}

void
PlayerWidget::onPlayerRightClicked(const QPointF &point)
{
	static QMenu *menu = new QMenu(this);

	menu->clear();

	menu->addAction(app()->action(ACT_OPEN_VIDEO));
	menu->addAction(app()->action(ACT_CLOSE_VIDEO));

	menu->addSeparator();

	menu->addAction(app()->action(ACT_TOGGLE_FULL_SCREEN));

	menu->addSeparator();

	menu->addAction(app()->action(ACT_STOP));
	menu->addAction(app()->action(ACT_PLAY_PAUSE));
	menu->addAction(app()->action(ACT_SEEK_BACKWARD));
	menu->addAction(app()->action(ACT_SEEK_FORWARD));

	menu->addSeparator();

	menu->addAction(app()->action(ACT_SEEK_TO_PREVIOUS_LINE));
	menu->addAction(app()->action(ACT_SEEK_TO_NEXT_LINE));

	menu->addSeparator();

	menu->addAction(app()->action(ACT_PLAY_RATE_DECREASE));
	menu->addAction(app()->action(ACT_PLAY_RATE_INCREASE));

	menu->addSeparator();

	menu->addAction(app()->action(ACT_SET_ACTIVE_AUDIO_STREAM));
	menu->addAction(app()->action(ACT_INCREASE_VOLUME));
	menu->addAction(app()->action(ACT_DECREASE_VOLUME));
	menu->addAction(app()->action(ACT_TOGGLE_MUTED));

	menu->addSeparator();

	if(m_translationMode)
		menu->addAction(app()->action(ACT_SET_ACTIVE_SUBTITLE_STREAM));

	menu->addAction(app()->action(ACT_INCREASE_SUBTITLE_FONT));
	menu->addAction(app()->action(ACT_DECREASE_SUBTITLE_FONT));

	// NOTE do not use popup->exec() here!!! it freezes the application
	// when using the mplayer backend. i think it's related to the fact
	// that exec() creates a different event loop and the mplayer backend
	// depends on the main loop for catching synchronization signals
	menu->popup(point.toPoint());
}

void
PlayerWidget::onPlayerDoubleClicked(const QPointF &point)
{
	Q_UNUSED(point);
	app()->toggleFullScreenMode();
}
