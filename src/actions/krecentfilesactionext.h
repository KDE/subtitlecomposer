#ifndef KRECENTFILESACTIONEXT_H
#define KRECENTFILESACTIONEXT_H

/*
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

#include <QMap>
#include <QUrl>

#include <KRecentFilesAction>

class KRecentFilesActionExt : public KRecentFilesAction
{
	Q_OBJECT

public:
	explicit KRecentFilesActionExt(QObject *parent);
	virtual ~KRecentFilesActionExt();

	static QString encodingForUrl(const QUrl &url);

	void loadEntries(const KConfigGroup &configGroup);
	void saveEntries(const KConfigGroup &configGroup);

	void addUrl(const QUrl &url, const QString &encoding, const QString &name);
	inline void addUrl(const QUrl &url, const QString &encoding) { addUrl(url, encoding, QString()); }
};

#endif
