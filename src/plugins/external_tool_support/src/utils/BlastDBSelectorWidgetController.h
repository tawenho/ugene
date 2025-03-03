/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2021 UniPro <ugene@unipro.ru>
 * http://ugene.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _U2_BLAST_DBSELECTOR_WIDGET_CONTROLLER_H_
#define _U2_BLAST_DBSELECTOR_WIDGET_CONTROLLER_H_

#include <ui_BlastDBSelectorWidget.h>

namespace U2 {

class BlastDBSelectorWidgetController : public QWidget, public Ui_BlastDBSelectorWidget {
    Q_OBJECT
public:
    BlastDBSelectorWidgetController(QWidget *parent = NULL);

    bool isNuclDatabase() const;
    bool isInputDataValid() const;
    QString getDatabasePath() const;
    bool validateDatabaseDir();
signals:
    void si_dbChanged();
private slots:
    void sl_onBrowseDatabasePath();
    void sl_lineEditChanged();

private:
    bool isNuclDB;
    bool inputDataValid;
};

}    // namespace U2
#endif    //_U2_BLAST_DBSELECTOR_WIDGET_CONTROLLER_H_
