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

#ifndef _U2_ACTOR_CFG_FILTER_PROXY_H_
#define _U2_ACTOR_CFG_FILTER_PROXY_H_

#include <QSortFilterProxyModel>

namespace U2 {

class ActorCfgFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    ActorCfgFilterProxyModel(QObject *p = NULL);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    virtual bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const;
};

}    // namespace U2

#endif    // _U2_ACTOR_CFG_FILTER_PROXY_H_
