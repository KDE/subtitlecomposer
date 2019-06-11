#ifndef TEXTOVERLAYWIDGET_H
#define TEXTOVERLAYWIDGET_H

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

#include <QWidget>
#include <QFont>
#include <QPen>
#include <QColor>
#include <QImage>
#include <QBitmap>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

class TextOverlayWidget : public QWidget
{
	Q_OBJECT

public:
	TextOverlayWidget(QWidget *parent = 0);
	virtual ~TextOverlayWidget();

	QString text() const;

	int alignment() const;
	int pointSize() const;
	qreal pointSizeF() const;
	int pixelSize() const;
	QString family() const;
	QColor primaryColor() const;
	int outlineWidth() const;
	QColor outlineColor() const;

	QSize minimumSizeHint() const override;

	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void setText(const QString &text);
	void setAlignment(int alignment);
	void setPointSize(int pointSize);
	void setPointSizeF(qreal pointSizeF);
	void setPixelSize(int pixelSize);
	void setFamily(const QString &family);
	void setPrimaryColor(const QColor &color);
	void setOutlineWidth(int width);
	void setOutlineColor(const QColor &color);
	void setAntialias(bool antialias);

protected:
	void customEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

	void setDirty(bool updateRichText, bool updateTransColor, bool flickerless = false);

	void updateColors();
	void updateContents();

	QRect calculateTextRect() const;
	void setMonoMask();
	void setOutline();

private:
	QString m_text;
	bool m_antialias;

	int m_alignment;
	QFont m_font;                           // font family and size are stored here
	QColor m_primaryColor;
	QRgb m_primaryRGB;

	int m_outlineWidth;
	QColor m_outlineColor;
	QRgb m_outlineRGB;

	QColor m_transColor;
	QRgb m_transRGB;

	QTextDocument *m_textDocument;

	QBitmap m_noTextMask;
	QImage m_bgImage;

	bool m_dirty;
};

#endif
