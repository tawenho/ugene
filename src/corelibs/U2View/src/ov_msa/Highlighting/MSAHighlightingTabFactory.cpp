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

#include "MSAHighlightingTabFactory.h"

#include <QPixmap>

#include <U2Core/U2SafePoints.h>

#include "MSAHighlightingTab.h"

namespace U2 {

const QString MSAHighlightingFactory::GROUP_ID = "OP_MSA_HIGHLIGHTING";
const QString MSAHighlightingFactory::GROUP_ICON_STR = ":core/images/highlight.png";
const QString MSAHighlightingFactory::GROUP_DOC_PAGE = "54362601";

MSAHighlightingFactory::MSAHighlightingFactory() {
    objectViewOfWidget = ObjViewType_AlignmentEditor;
}

QWidget *MSAHighlightingFactory::createWidget(GObjectView *objView, const QVariantMap & /*options*/) {
    SAFE_POINT(objView != nullptr,
               QString("Internal error: unable to create widget for group '%1', object view is NULL.").arg(GROUP_ID),
               nullptr);

    MSAEditor *msa = qobject_cast<MSAEditor *>(objView);
    SAFE_POINT(msa != nullptr,
               QString("Internal error: unable to cast object view to MSAEditor for group '%1'.").arg(GROUP_ID),
               nullptr);

    return new MSAHighlightingTab(msa);
}

OPGroupParameters MSAHighlightingFactory::getOPGroupParameters() {
    return OPGroupParameters(GROUP_ID, QPixmap(GROUP_ICON_STR), QObject::tr("Highlighting"), GROUP_DOC_PAGE);
}

}    // namespace U2
