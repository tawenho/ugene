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

#ifndef _U2_GRAPHICS_CIRCULAR_BRANCH_ITEM_H_
#define _U2_GRAPHICS_CIRCULAR_BRANCH_ITEM_H_

#include <U2Core/Task.h>

#include "GraphicsBranchItem.h"

class QGraphicsItem;

namespace U2 {

class PhyNode;
class GraphicsButtonItem;
class GraphicsRectangularBranchItem;

class U2VIEW_EXPORT GraphicsCircularBranchItem : public GraphicsBranchItem {
    qreal height;
    Direction direction;
    bool visible;

public:
    GraphicsCircularBranchItem(QGraphicsItem *parent, qreal height, GraphicsRectangularBranchItem *from, double nodeValue = -1.0);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setVisibleW(bool v) {
        visible = v;
    }

protected:
    void setLabelPositions();
};

}    // namespace U2

#endif
