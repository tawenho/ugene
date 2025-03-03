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

#ifndef _U2_MSA_EDITOR_NAME_LIST_H_
#define _U2_MSA_EDITOR_NAME_LIST_H_

#include "MaEditorNameList.h"

namespace U2 {

class MsaEditorNameList : public MaEditorNameList {
    Q_OBJECT
public:
    MsaEditorNameList(MaEditorWgt *ui, QScrollBar *nhBar);

protected:
    /** Double-click in MSA editor is bound to 'rename-sequence' action. */
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private slots:
    void sl_buildStaticMenu(GObjectView *view, QMenu *menu);
    void sl_buildContextMenu(GObjectView *view, QMenu *menu);

private:
    void buildMenu(QMenu *menu);

    MSAEditor *getEditor() const;
};

}    // namespace U2

#endif    // _U2_MSA_EDITOR_NAME_LIST_H_
