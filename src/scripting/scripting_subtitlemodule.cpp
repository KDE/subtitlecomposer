/*
    SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>
    SPDX-FileCopyrightText: 2010-2018 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "scripting_subtitlemodule.h"
#include "scripting_subtitle.h"
#include "application.h"
#include "core/subtitletarget.h"

using namespace SubtitleComposer;

Scripting::SubtitleModule::SubtitleModule(QObject *parent) :
	QObject(parent)
{}

QObject *
Scripting::SubtitleModule::instance()
{
	return app()->subtitle() ? new Scripting::Subtitle(app()->subtitle(), this) : 0;
}

bool
Scripting::SubtitleModule::translationMode()
{
	return app()->translationMode();
}


