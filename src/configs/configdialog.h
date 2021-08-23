#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

/*
 * SPDX-FileCopyrightText: 2015 Martin Steghöfer <martin@steghoefer.eu>
 * SPDX-FileCopyrightText: 2015-2019 Mladen Milinkovic <max@smoothware.net>
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

#include <KConfigDialog>

#include <sonnet/configwidget.h>

namespace SubtitleComposer {

class ConfigDialog : public KConfigDialog
{
	Q_OBJECT

public:
	ConfigDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config);

public slots:
	void widgetChanged();

public:
	void updateSettings() override;

protected:
	bool hasChanged() override;

private:
	bool m_hasWidgetChanged;
	Sonnet::ConfigWidget *m_sonnetConfigWidget;
};

}

#endif
