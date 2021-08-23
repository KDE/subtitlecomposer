#ifndef OUTPUTFORMAT_H
#define OUTPUTFORMAT_H

/*
 * SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
 * SPDX-FileCopyrightText: 2010-2018 Mladen Milinkovic <max@smoothware.net>
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

#include "format.h"

namespace SubtitleComposer {
class OutputFormat : public Format
{
public:

	QString writeSubtitle(const Subtitle &subtitle, bool primary) const
	{
		return dumpSubtitles(subtitle, primary);
	}

protected:
	virtual QString dumpSubtitles(const Subtitle &subtitle, bool primary) const = 0;

	OutputFormat(const QString &name, const QStringList &extensions) : Format(name, extensions) {}
};
}

#endif
