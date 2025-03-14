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

#ifndef _U2_MYSQL_UPGRADER_FROM_1_14_TO_1_15_H_
#define _U2_MYSQL_UPGRADER_FROM_1_14_TO_1_15_H_

#include "MysqlUpgrader.h"

namespace U2 {

class MysqlDbRef;

class MysqlUpgraderFrom_1_14_To_1_15 : public MysqlUpgrader {
public:
    MysqlUpgraderFrom_1_14_To_1_15(MysqlDbi *dbi);

    void upgrade(U2OpStatus &os) const;

private:
    void upgradeObjectDbi(U2OpStatus &os, MysqlDbRef *dbRef) const;
    static QString getParentFolderFromList(const QStringList &folders, const QString &folder);
    static void rollNewFolderPath(QString &originalPath, const QStringList &allFolders);
};

}    // namespace U2

#endif    // _U2_MYSQL_UPGRADER_FROM_1_14_TO_1_15_H_
