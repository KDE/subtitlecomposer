#ifndef SUBTITLESORTTEST_H
#define SUBTITLESORTTEST_H
/*
 * SPDX-FileCopyrightText: 2021 Mladen Milinkovic <max@smoothware.net>
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

#include "core/subtitle.h"

#include <QObject>

class SubtitleTest : public QObject
{
	Q_OBJECT

	SubtitleComposer::Subtitle sub;

private slots:
	void testSort_data();
	void testSort();
};

#endif // SUBTITLESORTTEST_H
