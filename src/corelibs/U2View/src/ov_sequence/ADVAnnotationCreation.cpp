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

#include "ADVAnnotationCreation.h"

#include <QFileInfo>

#include <U2Core/AnnotationSelection.h>
#include <U2Core/AnnotationTableObject.h>
#include <U2Core/AppContext.h>
#include <U2Core/CreateAnnotationTask.h>
#include <U2Core/DNAAlphabet.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/DNASequenceSelection.h>
#include <U2Core/GObjectReference.h>
#include <U2Core/GObjectUtils.h>
#include <U2Core/LoadDocumentTask.h>
#include <U2Core/QObjectScopedPointer.h>
#include <U2Core/Task.h>
#include <U2Core/U2SafePoints.h>

#include <U2Gui/CreateAnnotationDialog.h>
#include <U2Gui/CreateAnnotationWidgetController.h>

#include "ADVSequenceObjectContext.h"
#include "AnnotatedDNAView.h"
#include "AnnotationsTreeView.h"

namespace U2 {

ADVCreateAnnotationsTask::ADVCreateAnnotationsTask(AnnotatedDNAView *sequenceView, const GObjectReference &aobjRef, const QString &group, const QList<SharedAnnotationData> &data, bool selectNewAnnotations)
    : Task(tr("Create annotations task"), TaskFlags_NR_FOSCOE), sequenceView(sequenceView), selectNewAnnotations(selectNewAnnotations) {
    LoadUnloadedDocumentTask::addLoadingSubtask(this, LoadDocumentTaskConfig(true, aobjRef, new LDTObjectFactory(this)));
    createAnnotationsTask = new CreateAnnotationsTask(aobjRef, data, group);
    addSubTask(createAnnotationsTask);
}

Task::ReportResult ADVCreateAnnotationsTask::report() {
    if (hasError() || sequenceView.isNull()) {
        return ReportResult_Finished;
    }
    AnnotationTableObject *annotationObject = createAnnotationsTask->getAnnotationTableObject();
    if (!sequenceView->getAnnotationObjects().contains(annotationObject)) {
        // for documents loaded during annotation creation object is added here.
        QString errorMessage = sequenceView->addObject(annotationObject);
        if (!errorMessage.isEmpty()) {
            setError(errorMessage);
        }
    }
    if (!hasError() && selectNewAnnotations) {
        AnnotationSelection *selection = sequenceView->getAnnotationsSelection();
        selection->setAnnotations(createAnnotationsTask->getResultAnnotations());
    }
    return ReportResult_Finished;
}

//////////////////////////////////////////////////////////////////////////
/// ADVAnnotationCreation

ADVAnnotationCreation::ADVAnnotationCreation(AnnotatedDNAView *c)
    : QObject(c) {
    ctx = c;
    createAction = new QAction(QIcon(":core/images/create_annotation_icon.png"), tr("New annotation..."), this);
    createAction->setObjectName("create_annotation_action");
    createAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    createAction->setShortcutContext(Qt::WindowShortcut);
    connect(createAction, SIGNAL(triggered()), SLOT(sl_createAnnotation()));
}

void ADVAnnotationCreation::sl_createAnnotation() {
    ADVSequenceObjectContext *seqCtx = ctx->getActiveSequenceContext();
    SAFE_POINT(seqCtx != nullptr, "Invalid sequence context detected!", );
    CreateAnnotationModel m;
    m.useUnloadedObjects = true;
    m.useAminoAnnotationTypes = seqCtx->getAlphabet()->isAmino();
    m.sequenceObjectRef = GObjectReference(seqCtx->getSequenceObject());
    m.sequenceLen = seqCtx->getSequenceObject()->getSequenceLength();
    if (!seqCtx->getSequenceSelection()->isEmpty()) {
        m.data->location->regions << seqCtx->getSequenceSelection()->getSelectedRegions();
    }

    //setup default object and group if possible from AnnotationsTreeView
    AnnotationsTreeView *tv = ctx->getAnnotationsView();
    AVItem *ai = tv->currentItem();
    if (ai != nullptr && !ai->isReadonly()) {
        AnnotationTableObject *aobj = ai->getAnnotationTableObject();
        if (seqCtx->getAnnotationGObjects().contains(aobj)) {
            m.annotationObjectRef = aobj;
            AnnotationGroup *ag = ai->getAnnotationGroup();
            if (ag != aobj->getRootGroup()) {
                m.groupName = ag->getGroupPath();
            }
        }
    }

    QObjectScopedPointer<CreateAnnotationDialog> d = new CreateAnnotationDialog(ctx->getWidget(), m);
    const int rc = d->exec();
    CHECK(!d.isNull(), );

    if (rc != QDialog::Accepted) {
        return;
    }

    QList<SharedAnnotationData> data;
    data << m.data;
    ADVCreateAnnotationsTask *t = new ADVCreateAnnotationsTask(ctx, m.annotationObjectRef, m.groupName, data);
    AppContext::getTaskScheduler()->registerTopLevelTask(t);
}

}    // namespace U2
