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

#ifndef _U2_GRAPHPACK_DEVIATION_H_
#define _U2_GRAPHPACK_DEVIATION_H_

#include <QAction>
#include <QBitArray>
#include <QList>
#include <QPair>

#include <U2View/GSequenceGraphView.h>

#include "DNAGraphPackPlugin.h"

namespace U2 {

class DeviationGraphFactory : public GSequenceGraphFactory {
    Q_OBJECT
public:
    enum GDeviationType { GC,
                          AT };
    DeviationGraphFactory(GDeviationType t, QObject *p);
    QList<QSharedPointer<GSequenceGraphData>> createGraphs(GSequenceGraphView *v);
    bool isEnabled(const U2SequenceObject *o) const;

private:
    QPair<char, char> devPair;
};

class DeviationGraphAlgorithm : public GSequenceGraphAlgorithm {
public:
    DeviationGraphAlgorithm(const QPair<char, char> &_p);

    void calculate(QVector<float> &res, U2SequenceObject *o, const U2Region &r, const GSequenceGraphWindowData *d, U2OpStatus &os);

private:
    void windowStrategyWithoutMemorize(QVector<float> &res, const QByteArray &seq, int startPos, const GSequenceGraphWindowData *d, int nSteps, U2OpStatus &os);
    void sequenceStrategyWithMemorize(QVector<float> &res, const QByteArray &seq, const U2Region &vr, const GSequenceGraphWindowData *d, U2OpStatus &os);
    QPair<int, int> matchOnStep(const QByteArray &seq, int begin, int end);

    QPair<char, char> p;
};

}    // namespace U2
#endif
