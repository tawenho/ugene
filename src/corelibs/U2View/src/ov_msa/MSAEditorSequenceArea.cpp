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

#include "MSAEditorSequenceArea.h"

#include <QApplication>
#include <QDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QTextStream>
#include <QWidgetAction>

#include <U2Algorithm/CreateSubalignmentTask.h>
#include <U2Algorithm/MsaHighlightingScheme.h>

#include <U2Core/AddSequencesToAlignmentTask.h>
#include <U2Core/AppContext.h>
#include <U2Core/ClipboardController.h>
#include <U2Core/DNAAlphabet.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/DNATranslation.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/MsaDbiUtils.h>
#include <U2Core/MultipleSequenceAlignment.h>
#include <U2Core/MultipleSequenceAlignmentObject.h>
#include <U2Core/ProjectModel.h>
#include <U2Core/QObjectScopedPointer.h>
#include <U2Core/SaveDocumentTask.h>
#include <U2Core/Settings.h>
#include <U2Core/Task.h>
#include <U2Core/TaskSignalMapper.h>
#include <U2Core/TaskWatchdog.h>
#include <U2Core/U2AlphabetUtils.h>
#include <U2Core/U2ObjectDbi.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/U2SequenceUtils.h>

#include <U2Formats/FastaFormat.h>

#include <U2Gui/DialogUtils.h>
#include <U2Gui/GUIUtils.h>
#include <U2Gui/LastUsedDirHelper.h>
#include <U2Gui/Notification.h>
#include <U2Gui/OPWidgetFactory.h>
#include <U2Gui/OptionsPanel.h>
#include <U2Gui/PositionSelector.h>
#include <U2Gui/ProjectTreeController.h>
#include <U2Gui/ProjectTreeItemSelectorDialog.h>

#include "Clipboard/SubalignmentToClipboardTask.h"
#include "CreateSubalignmentDialogController.h"
#include "ExportSequencesTask.h"
#include "MSAEditor.h"
#include "MaEditorNameList.h"
#include "helpers/ScrollController.h"
#include "view_rendering/SequenceAreaRenderer.h"

namespace U2 {

MSAEditorSequenceArea::MSAEditorSequenceArea(MaEditorWgt *_ui, GScrollBar *hb, GScrollBar *vb)
    : MaEditorSequenceArea(_ui, hb, vb) {
    setObjectName("msa_editor_sequence_area");
    setFocusPolicy(Qt::WheelFocus);

    initRenderer();

    connect(editor, SIGNAL(si_buildPopupMenu(GObjectView *, QMenu *)), SLOT(sl_buildContextMenu(GObjectView *, QMenu *)));
    connect(editor, SIGNAL(si_buildStaticMenu(GObjectView *, QMenu *)), SLOT(sl_buildStaticMenu(GObjectView *, QMenu *)));
    connect(editor, SIGNAL(si_buildStaticToolbar(GObjectView *, QToolBar *)), SLOT(sl_buildStaticToolbar(GObjectView *, QToolBar *)));

    selectionColor = Qt::black;
    editingEnabled = true;

    connect(ui->copySelectionAction, SIGNAL(triggered()), SLOT(sl_copySelection()));
    connect(ui->copyFormattedSelectionAction, SIGNAL(triggered()), SLOT(sl_copySelectionFormatted()));
    connect(ui->pasteAction, SIGNAL(triggered()), SLOT(sl_paste()));
    connect(ui->pasteBeforeAction, SIGNAL(triggered()), SLOT(sl_pasteBefore()));
    connect(ui->cutSelectionAction, SIGNAL(triggered()), SLOT(sl_cutSelection()));

    delColAction = new QAction(QIcon(":core/images/msaed_remove_columns_with_gaps.png"), tr("Remove columns of gaps..."), this);
    delColAction->setObjectName("remove_columns_of_gaps");
    delColAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Delete));
    delColAction->setShortcutContext(Qt::WidgetShortcut);
    addAction(delColAction);
    connect(delColAction, SIGNAL(triggered()), SLOT(sl_delCol()));

    createSubaligniment = new QAction(tr("Save subalignment..."), this);
    createSubaligniment->setObjectName("Save subalignment");
    connect(createSubaligniment, SIGNAL(triggered()), SLOT(sl_createSubalignment()));

    saveSequence = new QAction(tr("Export selected sequence(s)..."), this);
    saveSequence->setObjectName("Save sequence");
    connect(saveSequence, SIGNAL(triggered()), SLOT(sl_saveSequence()));

    removeAllGapsAction = new QAction(QIcon(":core/images/msaed_remove_all_gaps.png"), tr("Remove all gaps"), this);
    removeAllGapsAction->setObjectName("Remove all gaps");
    connect(removeAllGapsAction, SIGNAL(triggered()), SLOT(sl_removeAllGaps()));

    addSeqFromFileAction = new QAction(tr("Sequence from file..."), this);
    addSeqFromFileAction->setObjectName("Sequence from file");
    connect(addSeqFromFileAction, SIGNAL(triggered()), SLOT(sl_addSeqFromFile()));

    addSeqFromProjectAction = new QAction(tr("Sequence from current project..."), this);
    addSeqFromProjectAction->setObjectName("Sequence from current project");
    connect(addSeqFromProjectAction, SIGNAL(triggered()), SLOT(sl_addSeqFromProject()));

    toggleSequenceRowOrderAction = new QAction(QIcon(":core/images/collapse.png"), tr("Switch on/off collapsing"), this);
    toggleSequenceRowOrderAction->setObjectName("toggle_sequence_row_order_action");
    toggleSequenceRowOrderAction->setCheckable(true);
    connect(toggleSequenceRowOrderAction, SIGNAL(toggled(bool)), SLOT(sl_toggleSequenceRowOrder(bool)));

    refreshSequenceRowOrder = new QAction(QIcon(":core/images/collapse_update.png"), tr("Update collapsed groups"), this);
    refreshSequenceRowOrder->setObjectName("refresh_sequence_row_order_action");
    refreshSequenceRowOrder->setEnabled(false);
    connect(refreshSequenceRowOrder, SIGNAL(triggered()), SLOT(sl_groupSequencesByContent()));

    reverseComplementAction = new QAction(tr("Replace selected rows with reverse-complement"), this);
    reverseComplementAction->setObjectName("replace_selected_rows_with_reverse-complement");
    connect(reverseComplementAction, SIGNAL(triggered()), SLOT(sl_reverseComplementCurrentSelection()));

    reverseAction = new QAction(tr("Replace selected rows with reverse"), this);
    reverseAction->setObjectName("replace_selected_rows_with_reverse");
    connect(reverseAction, SIGNAL(triggered()), SLOT(sl_reverseCurrentSelection()));

    complementAction = new QAction(tr("Replace selected rows with complement"), this);
    complementAction->setObjectName("replace_selected_rows_with_complement");
    connect(complementAction, SIGNAL(triggered()), SLOT(sl_complementCurrentSelection()));

    connect(editor->getMaObject(), SIGNAL(si_lockedStateChanged()), SLOT(sl_lockedStateChanged()));

    connect(this, SIGNAL(si_startMaChanging()), ui, SIGNAL(si_startMaChanging()));
    connect(this, SIGNAL(si_stopMaChanging(bool)), ui, SIGNAL(si_stopMaChanging(bool)));

    connect(ui->getCollapseModel(), SIGNAL(si_toggled()), SLOT(sl_modelChanged()));
    connect(editor, SIGNAL(si_fontChanged(QFont)), SLOT(sl_fontChanged(QFont)));
    connect(editor, SIGNAL(si_referenceSeqChanged(qint64)), SLOT(sl_completeUpdate()));

    connect(editor->getMaObject(), SIGNAL(si_alphabetChanged(const MaModificationInfo &, const DNAAlphabet *)), SLOT(sl_alphabetChanged(const MaModificationInfo &, const DNAAlphabet *)));

    setMouseTracking(true);

    updateColorAndHighlightSchemes();
    sl_updateActions();
}

MSAEditor *MSAEditorSequenceArea::getEditor() const {
    return qobject_cast<MSAEditor *>(editor);
}

bool MSAEditorSequenceArea::hasAminoAlphabet() {
    MultipleAlignmentObject *maObj = editor->getMaObject();
    SAFE_POINT(maObj != nullptr, "MultipleAlignmentObject is null in MSAEditorSequenceArea::hasAminoAlphabet()", false);
    const DNAAlphabet *alphabet = maObj->getAlphabet();
    SAFE_POINT(maObj != nullptr, "DNAAlphabet is null in MSAEditorSequenceArea::hasAminoAlphabet()", false);
    return DNAAlphabet_AMINO == alphabet->getType();
}

void MSAEditorSequenceArea::focusInEvent(QFocusEvent *fe) {
    QWidget::focusInEvent(fe);
    update();
}

void MSAEditorSequenceArea::focusOutEvent(QFocusEvent *fe) {
    QWidget::focusOutEvent(fe);
    exitFromEditCharacterMode();
    update();
}

// TODO: move this function into MSA?
/* Compares sequences of 2 rows ignoring gaps. */
static bool isEqualsIgnoreGaps(const MultipleAlignmentRowData *row1, const MultipleAlignmentRowData *row2) {
    if (row1 == row2) {
        return true;
    }
    if (row1->getUngappedLength() != row2->getUngappedLength()) {
        return false;
    }
    return row1->getUngappedSequence().seq == row2->getUngappedSequence().seq;
}

// TODO: move this function into MSA?
/* Groups rows by similarity. Two rows are considered equal if their sequences are equal with ignoring of gaps. */
static QList<QList<int>> groupRowsBySimilarity(const QList<MultipleAlignmentRow> &msaRows) {
    QList<QList<int>> rowGroups;
    QSet<int> mappedRows;    // contains indexes of the already processed rows.
    for (int i = 0; i < msaRows.size(); i++) {
        if (mappedRows.contains(i)) {
            continue;
        }
        const MultipleAlignmentRow &row = msaRows[i];
        QList<int> rowGroup;
        rowGroup << i;
        for (int j = i + 1; j < msaRows.size(); j++) {
            const MultipleAlignmentRow &next = msaRows[j];
            if (!mappedRows.contains(j) && isEqualsIgnoreGaps(next.data(), row.data())) {
                rowGroup << j;
                mappedRows.insert(j);
            }
        }
        rowGroups << rowGroup;
    }
    return rowGroups;
}

void MSAEditorSequenceArea::updateCollapseModel(const MaModificationInfo &modInfo) {
    if (!modInfo.rowContentChanged && !modInfo.rowListChanged) {
        return;
    }
    MaCollapseModel *collapseModel = ui->getCollapseModel();
    auto mode = editor->getRowOrderMode();
    if (mode == MaEditorRowOrderMode::Original) {
        // Synchronize collapsible model with a current alignment.
        collapseModel->reset(getEditor()->getMaRowIds());
        return;
    } else if (mode == MaEditorRowOrderMode::Free) {
        // Check if the modification is compatible with the current view state: all rows have view properties assigned. Reset to the Original order if not.
        QSet<qint64> maRowIds = getEditor()->getMaRowIds().toSet();
        QSet<qint64> viewModelRowIds = collapseModel->getAllRowIds();
        if (viewModelRowIds != maRowIds) {
            sl_toggleSequenceRowOrder(false);
        }
        return;
    }

    SAFE_POINT(mode == MaEditorRowOrderMode::Sequence, "Unexpected row order mode", );

    // Order and group rows by sequence content.
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    QList<QList<int>> rowGroups = groupRowsBySimilarity(msaObject->getRows());
    QVector<MaCollapsibleGroup> newCollapseGroups;

    QSet<qint64> maRowIdsOfNonCollapsedRowsBefore;
    for (int i = 0; i < collapseModel->getGroupCount(); i++) {
        const MaCollapsibleGroup *group = collapseModel->getCollapsibleGroup(i);
        if (!group->isCollapsed) {
            maRowIdsOfNonCollapsedRowsBefore += group->maRowIds.toSet();
        }
    }
    for (int i = 0; i < rowGroups.size(); i++) {
        const QList<int> &maRowsInGroup = rowGroups[i];
        QList<qint64> maRowIdsInGroup = msaObject->getMultipleAlignment()->getRowIdsByRowIndexes(maRowsInGroup);
        bool isCollapsed = !maRowIdsOfNonCollapsedRowsBefore.contains(maRowIdsInGroup[0]);
        newCollapseGroups << MaCollapsibleGroup(maRowsInGroup, maRowIdsInGroup, isCollapsed);
    }
    collapseModel->update(newCollapseGroups);
}

void MSAEditorSequenceArea::sl_buildStaticToolbar(GObjectView *v, QToolBar *t) {
    Q_UNUSED(v);

    t->addAction(ui->getUndoAction());
    t->addAction(ui->getRedoAction());
    t->addAction(removeAllGapsAction);
    t->addSeparator();

    t->addAction(toggleSequenceRowOrderAction);
    t->addAction(refreshSequenceRowOrder);
    t->addSeparator();
}

void MSAEditorSequenceArea::sl_buildStaticMenu(GObjectView *, QMenu *m) {
    buildMenu(m);
}

void MSAEditorSequenceArea::sl_buildContextMenu(GObjectView *, QMenu *m) {
    buildMenu(m);

    QMenu *editMenu = GUIUtils::findSubMenu(m, MSAE_MENU_EDIT);
    SAFE_POINT(editMenu != nullptr, "editMenu is null", );

    QList<QAction *> actions;
    actions << fillWithGapsinsSymAction << replaceCharacterAction << reverseComplementAction
            << reverseAction << complementAction << delColAction << removeAllGapsAction;

    editMenu->insertAction(editMenu->actions().first(), ui->delSelectionAction);
    if (rect().contains(mapFromGlobal(QCursor::pos()))) {
        editMenu->addActions(actions);
    }

    m->setObjectName("msa sequence area context menu");
}

void MSAEditorSequenceArea::initRenderer() {
    renderer = new SequenceAreaRenderer(ui, this);
}

void MSAEditorSequenceArea::buildMenu(QMenu *m) {
    QMenu *loadSeqMenu = GUIUtils::findSubMenu(m, MSAE_MENU_LOAD);
    SAFE_POINT(loadSeqMenu != nullptr, "loadSeqMenu is null", );
    loadSeqMenu->addAction(addSeqFromProjectAction);
    loadSeqMenu->addAction(addSeqFromFileAction);

    QMenu *editMenu = GUIUtils::findSubMenu(m, MSAE_MENU_EDIT);
    SAFE_POINT(editMenu != nullptr, "editMenu is null", );
    QList<QAction *> actions;

    MSAEditor *editor = getEditor();
    MsaEditorWgt *msaWgt = editor->getUI();
    QAction *editSequenceNameAction = msaWgt->getEditorNameList()->getEditSequenceNameAction();
    if (getSelection().height() != 1) {
        editSequenceNameAction->setDisabled(true);
    }
    actions << editSequenceNameAction << fillWithGapsinsSymAction << replaceCharacterAction << reverseComplementAction << reverseAction << complementAction << delColAction << removeAllGapsAction;
    editMenu->insertActions(editMenu->isEmpty() ? nullptr : editMenu->actions().first(), actions);
    editMenu->insertAction(editMenu->actions().first(), ui->delSelectionAction);

    QMenu *exportMenu = GUIUtils::findSubMenu(m, MSAE_MENU_EXPORT);
    SAFE_POINT(exportMenu != nullptr, "exportMenu is null", );
    exportMenu->addAction(createSubaligniment);
    exportMenu->addAction(saveSequence);
}

void MSAEditorSequenceArea::sl_fontChanged(QFont font) {
    Q_UNUSED(font);
    completeRedraw = true;
    repaint();
}

void MSAEditorSequenceArea::sl_alphabetChanged(const MaModificationInfo &mi, const DNAAlphabet *prevAlphabet) {
    updateColorAndHighlightSchemes();

    QString message;
    if (mi.alphabetChanged || mi.type != MaModificationType_Undo) {
        message = tr("The alignment has been modified, so that its alphabet has been switched from \"%1\" to \"%2\". Use \"Undo\", if you'd like to restore the original alignment.")
                      .arg(prevAlphabet->getName())
                      .arg(editor->getMaObject()->getAlphabet()->getName());
    }

    if (message.isEmpty()) {
        return;
    }
    const NotificationStack *notificationStack = AppContext::getMainWindow()->getNotificationStack();
    CHECK(notificationStack != nullptr, );
    notificationStack->addNotification(message, Info_Not);
}

void MSAEditorSequenceArea::sl_updateActions() {
    MultipleAlignmentObject *maObj = editor->getMaObject();
    SAFE_POINT(maObj != nullptr, "alignment is null", );
    bool readOnly = maObj->isStateLocked();

    createSubaligniment->setEnabled(!isAlignmentEmpty());
    saveSequence->setEnabled(!isAlignmentEmpty());
    addSeqFromProjectAction->setEnabled(!readOnly);
    addSeqFromFileAction->setEnabled(!readOnly);
    toggleSequenceRowOrderAction->setEnabled(!readOnly && !isAlignmentEmpty());

    //Update actions of "Edit" group
    bool canEditAlignment = !readOnly && !isAlignmentEmpty();
    bool canEditSelectedArea = canEditAlignment && !selection.isEmpty();
    const bool isEditing = (maMode != ViewMode);
    ui->delSelectionAction->setEnabled(canEditSelectedArea);
    ui->pasteAction->setEnabled(!readOnly);
    ui->pasteBeforeAction->setEnabled(!readOnly);

    fillWithGapsinsSymAction->setEnabled(canEditSelectedArea && !isEditing);
    bool oneCharacterIsSelected = selection.width() == 1 && selection.height() == 1;
    replaceCharacterAction->setEnabled(canEditSelectedArea && oneCharacterIsSelected);
    delColAction->setEnabled(canEditAlignment);
    reverseComplementAction->setEnabled(canEditSelectedArea && maObj->getAlphabet()->isNucleic());
    reverseAction->setEnabled(canEditSelectedArea);
    complementAction->setEnabled(canEditSelectedArea && maObj->getAlphabet()->isNucleic());
    removeAllGapsAction->setEnabled(canEditAlignment);
}

void MSAEditorSequenceArea::sl_delCol() {
    QObjectScopedPointer<DeleteGapsDialog> dlg = new DeleteGapsDialog(this, editor->getMaObject()->getNumRows());
    dlg->exec();
    CHECK(!dlg.isNull(), );

    if (dlg->result() == QDialog::Accepted) {
        MaCollapseModel *collapsibleModel = ui->getCollapseModel();
        SAFE_POINT(collapsibleModel != nullptr, "NULL collapsible model!", );
        collapsibleModel->reset(editor->getMaRowIds());

        DeleteMode deleteMode = dlg->getDeleteMode();
        int value = dlg->getValue();

        // if this method was invoked during a region shifting
        // then shifting should be canceled
        cancelShiftTracking();

        MultipleSequenceAlignmentObject *msaObj = getEditor()->getMaObject();
        int gapCount = 0;
        switch (deleteMode) {
            case DeleteByAbsoluteVal:
                gapCount = value;
                break;
            case DeleteByRelativeVal: {
                int absoluteValue = qRound((msaObj->getNumRows() * value) / 100.0);
                if (absoluteValue < 1) {
                    absoluteValue = 1;
                }
                gapCount = absoluteValue;
                break;
            }
            case DeleteAll:
                gapCount = msaObj->getNumRows();
                break;
            default:
                FAIL("Unknown delete mode", );
        }

        U2OpStatus2Log os;
        U2UseCommonUserModStep userModStep(msaObj->getEntityRef(), os);
        Q_UNUSED(userModStep);
        SAFE_POINT_OP(os, );
        msaObj->deleteColumnsWithGaps(os, gapCount);
    }
}

void MSAEditorSequenceArea::sl_goto() {
    QDialog gotoDialog(this);
    gotoDialog.setModal(true);
    gotoDialog.setWindowTitle(tr("Go to Position"));
    PositionSelector *ps = new PositionSelector(&gotoDialog, 1, editor->getMaObject()->getLength(), true);
    connect(ps, SIGNAL(si_positionChanged(int)), SLOT(sl_onPosChangeRequest(int)));
    gotoDialog.exec();
}

void MSAEditorSequenceArea::sl_onPosChangeRequest(int position) {
    ui->getScrollController()->centerBase(position, width());
    setSelection(MaEditorSelection(position - 1, selection.y(), 1, 1));
}

void MSAEditorSequenceArea::sl_lockedStateChanged() {
    sl_updateActions();
}

void MSAEditorSequenceArea::sl_removeAllGaps() {
    MultipleSequenceAlignmentObject *msa = getEditor()->getMaObject();
    SAFE_POINT(!msa->isStateLocked(), tr("MSA is locked"), );

    // if this method was invoked during a region shifting
    // then shifting should be canceled
    cancelShiftTracking();

    U2OpStatus2Log os;
    U2UseCommonUserModStep userModStep(msa->getEntityRef(), os);
    Q_UNUSED(userModStep);
    SAFE_POINT_OP(os, );

    QMap<qint64, QList<U2MsaGap>> noGapModel;
    foreach (qint64 rowId, msa->getMultipleAlignment()->getRowsIds()) {
        noGapModel[rowId] = QList<U2MsaGap>();
    }

    msa->updateGapModel(os, noGapModel);

    MsaDbiUtils::trim(msa->getEntityRef(), os);
    msa->updateCachedMultipleAlignment();

    SAFE_POINT_OP(os, );

    ui->getScrollController()->setFirstVisibleBase(0);
    ui->getScrollController()->setFirstVisibleViewRow(0);
    SAFE_POINT_OP(os, );
}

void MSAEditorSequenceArea::sl_createSubalignment() {
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    const MaEditorSelection &selection = getSelection();
    QList<int> selectedRowIndexes = getSelectedMaRowIndexes();
    const MultipleAlignment &alignment = msaObject->getMultipleAlignment();
    QList<qint64> selectedRowIdList = selectedRowIndexes.isEmpty() ? alignment->getRowsIds() : alignment->getRowIdsByRowIndexes(selectedRowIndexes);
    U2Region selectedColumnsRegion = selection.isEmpty() ? U2Region(0, msaObject->getLength()) : selection.getXRegion();

    QObjectScopedPointer<CreateSubalignmentDialogController> dialog = new CreateSubalignmentDialogController(msaObject, selectedRowIdList, selectedColumnsRegion, this);
    dialog->exec();
    CHECK(!dialog.isNull(), );

    if (dialog->result() == QDialog::Accepted) {
        selectedColumnsRegion = dialog->getSelectedColumnsRegion();
        bool addToProject = dialog->getAddToProjFlag();
        QString path = dialog->getSavePath();
        selectedRowIdList = dialog->getSelectedRowIds();
        CreateSubalignmentSettings createSubalignmentSettings(selectedColumnsRegion, selectedRowIdList, path, true, addToProject, dialog->getFormatId());
        auto createSubAlignmentTask = new CreateSubalignmentAndOpenViewTask(msaObject, createSubalignmentSettings);
        AppContext::getTaskScheduler()->registerTopLevelTask(createSubAlignmentTask);
    }
}

void MSAEditorSequenceArea::sl_saveSequence() {
    CHECK(getEditor() != nullptr, );

    QWidget *parentWidget = (QWidget *)AppContext::getMainWindow()->getQMainWindow();
    QString suggestedFileName = editor->getMaObject()->getGObjectName() + "_sequence";
    QObjectScopedPointer<SaveSelectedSequenceFromMSADialogController> d = new SaveSelectedSequenceFromMSADialogController(parentWidget, suggestedFileName);
    const int rc = d->exec();
    CHECK(!d.isNull(), );

    if (rc == QDialog::Rejected) {
        return;
    }
    DocumentFormat *df = AppContext::getDocumentFormatRegistry()->getFormatById(d->getFormat());
    SAFE_POINT(df != nullptr, "Unknown document format", );
    QString extension = df->getSupportedDocumentFileExtensions().first();

    const MaEditorSelection &selection = editor->getSelection();
    int startSeq = selection.y();
    int endSeq = selection.y() + selection.height() - 1;
    MaCollapseModel *model = editor->getUI()->getCollapseModel();
    const MultipleAlignment &ma = editor->getMaObject()->getMultipleAlignment();
    QSet<qint64> seqIds;
    for (int i = startSeq; i <= endSeq; i++) {
        seqIds.insert(ma->getRow(model->getMaRowIndexByViewRowIndex(i))->getRowId());
    }
    ExportSequencesTask *exportTask = new ExportSequencesTask(getEditor()->getMaObject()->getMsa(), seqIds, d->getTrimGapsFlag(), d->getAddToProjectFlag(), d->getUrl(), d->getFormat(), extension, d->getCustomFileName());
    AppContext::getTaskScheduler()->registerTopLevelTask(exportTask);
}

void MSAEditorSequenceArea::sl_modelChanged() {
    MaCollapseModel *collapsibleModel = ui->getCollapseModel();
    if (!collapsibleModel->hasGroupsWithMultipleRows()) {
        toggleSequenceRowOrderAction->setChecked(false);
        refreshSequenceRowOrder->setEnabled(false);
    }
    MaEditorSequenceArea::sl_modelChanged();
}

void MSAEditorSequenceArea::sl_copySelection() {
    // Copies selection to the clipboard using FASTA format (the most simple format keeps sequence names).
    CHECK(getEditor() != nullptr, );
    CHECK(!selection.isEmpty(), );
    SAFE_POINT(isInRange(selection.topLeft()), "Selection top-left is not in range!", );
    SAFE_POINT(isInRange(selection.bottomRight()), "Selection bottom-right is not in range!", );

    MultipleSequenceAlignmentObject *maObj = getEditor()->getMaObject();
    MaCollapseModel *collapseModel = ui->getCollapseModel();
    QString textMimeContent;
    QString ugeneMimeContent;
    U2OpStatus2Log os;
    int len = selection.width();
    for (int viewRowIndex = selection.y(); viewRowIndex <= selection.bottom() && !os.hasError(); ++viewRowIndex) {    // bottom is inclusive
        int maRowIndex = collapseModel->getMaRowIndexByViewRowIndex(viewRowIndex);
        const MultipleSequenceAlignmentRow &row = maObj->getMsaRow(maRowIndex);
        QByteArray sequence = row->mid(selection.x(), len, os)->toByteArray(os, len);
        ugeneMimeContent.append(FastaFormat::FASTA_HEADER_START_SYMBOL)
            .append(row.data()->getName())
            .append('\n')
            .append(TextUtils::split(sequence, 80).join("\n"))
            .append('\n');

        bool isLastLine = viewRowIndex == selection.bottom();
        textMimeContent.append(sequence)
            .append(isLastLine ? "" : "\n");
    }
    auto mimeData = new QMimeData();
    mimeData->setText(textMimeContent);
    mimeData->setData(U2Clipboard::UGENE_MIME_TYPE, ugeneMimeContent.toUtf8());
    QApplication::clipboard()->setMimeData(mimeData);
}

void MSAEditorSequenceArea::sl_copySelectionFormatted() {
    const DocumentFormatId &formatId = getCopyFormattedAlgorithmId();
    QRect rectToCopy = selection.isEmpty() ? QRect(0, 0, editor->getAlignmentLen(), getViewRowCount()) : selection.toRect();
    auto coptTask = new SubalignmentToClipboardTask(getEditor(), rectToCopy, formatId);
    AppContext::getTaskScheduler()->registerTopLevelTask(coptTask);
}

void MSAEditorSequenceArea::sl_paste() {
    runPasteTask(false);
}

void MSAEditorSequenceArea::sl_pasteBefore() {
    runPasteTask(true);
}

#define IS_PASTE_BEFORE_PROPERTY_NAME "isPasteBefore"

void MSAEditorSequenceArea::runPasteTask(bool isPasteBefore) {
    MultipleAlignmentObject *msaObject = editor->getMaObject();
    if (msaObject->isStateLocked()) {
        return;
    }
    PasteFactory *pasteFactory = AppContext::getPasteFactory();
    SAFE_POINT(pasteFactory != nullptr, "PasteFactory is null", );

    bool isAddToProject = false;    // Do not add the pasted document to the project -> add it to the alignment.
    PasteTask *pasteTask = pasteFactory->createPasteTask(isAddToProject);
    CHECK(pasteTask != nullptr, );
    connect(new TaskSignalMapper(pasteTask), SIGNAL(si_taskFinished(Task *)), SLOT(sl_pasteTaskFinished(Task *)));
    pasteTask->setProperty(IS_PASTE_BEFORE_PROPERTY_NAME, QVariant::fromValue(isPasteBefore));
    AppContext::getTaskScheduler()->registerTopLevelTask(pasteTask);
}

void MSAEditorSequenceArea::sl_pasteTaskFinished(Task *_pasteTask) {
    CHECK(getEditor() != nullptr, );
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    CHECK(!msaObject->isStateLocked(), );

    PasteTask *pasteTask = qobject_cast<PasteTask *>(_pasteTask);
    CHECK(pasteTask != nullptr && !pasteTask->isCanceled() && !pasteTask->hasError(), );

    bool isPasteBefore = pasteTask->property(IS_PASTE_BEFORE_PROPERTY_NAME).toBool();
    const QList<Document *> &docs = pasteTask->getDocuments();

    int insertRowIndex = isPasteBefore ? (selection.isEmpty() ? 0 : selection.y()) : (selection.isEmpty() ? -1 : selection.bottom() + 1);
    auto task = new AddSequencesFromDocumentsToAlignmentTask(msaObject, docs, insertRowIndex, true);
    task->setErrorNotificationSuppression(true);    // we manually show warning message if needed when task is finished.
    connect(new TaskSignalMapper(task), SIGNAL(si_taskFinished(Task *)), SLOT(sl_addSequencesToAlignmentFinished(Task *)));
    AppContext::getTaskScheduler()->registerTopLevelTask(task);
}

void MSAEditorSequenceArea::sl_addSequencesToAlignmentFinished(Task *task) {
    AddSequencesFromDocumentsToAlignmentTask *addSeqTask = qobject_cast<AddSequencesFromDocumentsToAlignmentTask *>(task);
    CHECK(addSeqTask != nullptr, );
    const MaModificationInfo &mi = addSeqTask->getMaModificationInfo();
    if (!mi.rowListChanged) {
        const NotificationStack *notificationStack = AppContext::getMainWindow()->getNotificationStack();
        CHECK(notificationStack != nullptr, );
        notificationStack->addNotification(tr("No new rows were inserted: selection contains no valid sequences."), Warning_Not);
    }
}

void MSAEditorSequenceArea::sl_cutSelection() {
    CHECK(!selection.isEmpty(), );
    sl_copySelection();
    sl_delCurrentSelection();
}

void MSAEditorSequenceArea::sl_addSeqFromFile() {
    CHECK(getEditor() != nullptr, );
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    if (msaObject->isStateLocked()) {
        return;
    }

    QString filter = DialogUtils::prepareDocumentsFileFilterByObjType(GObjectTypes::SEQUENCE, true);

    LastUsedDirHelper lod;
    QStringList urls;
#ifdef Q_OS_MAC
    if (qgetenv(ENV_GUI_TEST).toInt() == 1 && qgetenv(ENV_USE_NATIVE_DIALOGS).toInt() == 0) {
        urls = U2FileDialog::getOpenFileNames(this, tr("Open file with sequences"), lod.dir, filter, 0, QFileDialog::DontUseNativeDialog);
    } else
#endif
        urls = U2FileDialog::getOpenFileNames(this, tr("Open file with sequences"), lod.dir, filter);

    if (!urls.isEmpty()) {
        lod.url = urls.first();
        sl_cancelSelection();
        int insertRowIndex = selection.isEmpty() ? -1 : selection.bottom() + 1;
        auto task = new AddSequencesFromFilesToAlignmentTask(msaObject, urls, insertRowIndex);
        TaskWatchdog::trackResourceExistence(msaObject, task, tr("A problem occurred during adding sequences. The multiple alignment is no more available."));
        AppContext::getTaskScheduler()->registerTopLevelTask(task);
    }
}

void MSAEditorSequenceArea::sl_addSeqFromProject() {
    CHECK(getEditor() != nullptr, );
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    if (msaObject->isStateLocked()) {
        return;
    }

    ProjectTreeControllerModeSettings settings;
    settings.objectTypesToShow.insert(GObjectTypes::SEQUENCE);

    QList<GObject *> objects = ProjectTreeItemSelectorDialog::selectObjects(settings, this);
    QList<DNASequence> objectsToAdd;
    U2OpStatus2Log os;
    foreach (GObject *obj, objects) {
        U2SequenceObject *seqObj = qobject_cast<U2SequenceObject *>(obj);
        if (seqObj) {
            objectsToAdd.append(seqObj->getWholeSequence(os));
            SAFE_POINT_OP(os, );
        }
    }
    if (objectsToAdd.size() > 0) {
        AddSequenceObjectsToAlignmentTask *addSeqObjTask = new AddSequenceObjectsToAlignmentTask(getEditor()->getMaObject(), objectsToAdd);
        AppContext::getTaskScheduler()->registerTopLevelTask(addSeqObjTask);
        sl_cancelSelection();
    }
}

void MSAEditorSequenceArea::sl_toggleSequenceRowOrder(bool isOrderBySequence) {
    CHECK(getEditor() != nullptr, );
    GCOUNTER(cvar, "Switch collapsing mode");

    MaEditorRowOrderMode newMode = isOrderBySequence ? MaEditorRowOrderMode::Sequence : MaEditorRowOrderMode::Original;
    CHECK(editor->getRowOrderMode() != newMode, );

    editor->setRowOrderMode(newMode);
    updateRowOrderActionsState();
    if (isOrderBySequence) {
        sl_groupSequencesByContent();
    } else {
        ui->getCollapseModel()->reset(editor->getMaRowIds());
    }
    setSelection(MaEditorSelection());
    ui->getScrollController()->updateVerticalScrollBar();
    emit si_collapsingModeChanged();
}

void MSAEditorSequenceArea::sl_groupSequencesByContent() {
    MaModificationInfo mi;
    mi.alignmentLengthChanged = false;
    updateCollapseModel(mi);
}

void MSAEditorSequenceArea::reverseComplementModification(ModificationType &type) {
    CHECK(getEditor() != nullptr, );
    if (type == ModificationType::NoType) {
        return;
    }
    MultipleSequenceAlignmentObject *maObj = getEditor()->getMaObject();
    if (maObj == nullptr || maObj->isStateLocked()) {
        return;
    }
    if (!maObj->getAlphabet()->isNucleic()) {
        return;
    }
    if (selection.isEmpty()) {
        return;
    }
    assert(isInRange(selection.topLeft()));
    assert(isInRange(QPoint(selection.x() + selection.width() - 1, selection.y() + selection.height() - 1)));

    // if this method was invoked during a region shifting
    // then shifting should be canceled
    cancelShiftTracking();

    const MultipleSequenceAlignment ma = maObj->getMultipleAlignment();
    DNATranslation *trans = AppContext::getDNATranslationRegistry()->lookupComplementTranslation(ma->getAlphabet());
    if (trans == nullptr || !trans->isOne2One()) {
        return;
    }

    U2OpStatus2Log os;
    U2UseCommonUserModStep userModStep(maObj->getEntityRef(), os);
    Q_UNUSED(userModStep);
    SAFE_POINT_OP(os, );

    QList<int> selectedMaRows = getSelectedMaRowIndexes();

    QList<qint64> modifiedRowIds;
    for (int i = 0; i < selectedMaRows.size(); i++) {
        int maRowIndex = selectedMaRows[i];
        MultipleSequenceAlignmentRow currentRow = ma->getMsaRow(maRowIndex);
        QByteArray currentRowContent = currentRow->toByteArray(os, ma->getLength());
        switch (type.getType()) {
            case ModificationType::Reverse:
                TextUtils::reverse(currentRowContent.data(), currentRowContent.length());
                break;
            case ModificationType::Complement:
                trans->translate(currentRowContent.data(), currentRowContent.length());
                break;
            case ModificationType::ReverseComplement:
                TextUtils::reverse(currentRowContent.data(), currentRowContent.length());
                trans->translate(currentRowContent.data(), currentRowContent.length());
                break;
        }
        QString name = currentRow->getName();
        ModificationType oldType(ModificationType::NoType);
        if (name.endsWith("|revcompl")) {
            name.resize(name.length() - QString("|revcompl").length());
            oldType = ModificationType::ReverseComplement;
        } else if (name.endsWith("|compl")) {
            name.resize(name.length() - QString("|compl").length());
            oldType = ModificationType::Complement;
        } else if (name.endsWith("|rev")) {
            name.resize(name.length() - QString("|rev").length());
            oldType = ModificationType::Reverse;
        }
        ModificationType newType = type + oldType;
        switch (newType.getType()) {
            case ModificationType::NoType:
                break;
            case ModificationType::Reverse:
                name.append("|rev");
                break;
            case ModificationType::Complement:
                name.append("|compl");
                break;
            case ModificationType::ReverseComplement:
                name.append("|revcompl");
                break;
        }

        // Split the sequence into gaps and chars
        QByteArray seqBytes;
        QList<U2MsaGap> gapModel;
        MaDbiUtils::splitBytesToCharsAndGaps(currentRowContent, seqBytes, gapModel);

        maObj->updateRow(os, maRowIndex, name, seqBytes, gapModel);
        modifiedRowIds << currentRow->getRowId();
    }

    MaModificationInfo modInfo;
    modInfo.modifiedRowIds = modifiedRowIds;
    modInfo.alignmentLengthChanged = false;
    maObj->updateCachedMultipleAlignment(modInfo);
}

void MSAEditorSequenceArea::sl_reverseComplementCurrentSelection() {
    ModificationType type(ModificationType::ReverseComplement);
    reverseComplementModification(type);
}

void MSAEditorSequenceArea::sl_reverseCurrentSelection() {
    ModificationType type(ModificationType::Reverse);
    reverseComplementModification(type);
}

void MSAEditorSequenceArea::sl_complementCurrentSelection() {
    ModificationType type(ModificationType::Complement);
    reverseComplementModification(type);
}

void MSAEditorSequenceArea::sl_enableFreeRowOrderMode(const QList<QStringList> &collapsedGroups) {
    MultipleSequenceAlignmentObject *msaObject = getEditor()->getMaObject();
    QStringList rowNames = msaObject->getMultipleAlignment()->getRowNames();
    QList<qint64> rowIds = msaObject->getRowIds();

    // The result list of virtual groups.
    QVector<MaCollapsibleGroup> collapsibleGroupList;

    for (const QStringList &groupSequenceNameList : qAsConst(collapsedGroups)) {
        QList<int> maRowIndexList;
        QList<qint64> maRowIdList;
        for (const QString &sequenceName : qAsConst(groupSequenceNameList)) {
            int rowIndex = rowNames.indexOf(sequenceName);
            SAFE_POINT(rowIndex >= 0, "Row is not found: " + sequenceName, );
            maRowIndexList << rowIndex;
            maRowIdList << rowIds[rowIndex];
        }
        // All groups are collapsed by default for compatibility. TODO: make it configurable.
        bool isCollapsed = maRowIndexList.length() > 1;
        collapsibleGroupList << MaCollapsibleGroup(maRowIndexList, maRowIdList, isCollapsed);
    }
    editor->setRowOrderMode(MaEditorRowOrderMode::Free);
    updateRowOrderActionsState();
    ui->getCollapseModel()->update(collapsibleGroupList);
}

void MSAEditorSequenceArea::updateRowOrderActionsState() {
    auto mode = editor->getRowOrderMode();
    toggleSequenceRowOrderAction->setChecked(mode == MaEditorRowOrderMode::Sequence);
    refreshSequenceRowOrder->setEnabled(toggleSequenceRowOrderAction->isChecked());
}

ExportHighlightingTask::ExportHighlightingTask(ExportHighligtingDialogController *dialog, MaEditor *maEditor)
    : Task(tr("Export highlighting"), TaskFlags_FOSCOE | TaskFlag_ReportingIsSupported | TaskFlag_ReportingIsEnabled) {
    msaEditor = qobject_cast<MSAEditor *>(maEditor);
    startPos = dialog->startPos;
    endPos = dialog->endPos;
    startingIndex = dialog->startingIndex;
    keepGaps = dialog->keepGaps;
    dots = dialog->dots;
    transpose = dialog->transpose;
    url = dialog->url;
}

void ExportHighlightingTask::run() {
    QString exportedData = exportHighlighting(startPos, endPos, startingIndex, keepGaps, dots, transpose);
    QFile resultFile(url.getURLString());
    CHECK_EXT(resultFile.open(QFile::WriteOnly | QFile::Truncate), url.getURLString(), );
    QTextStream contentWriter(&resultFile);
    contentWriter << exportedData;
}

Task::ReportResult ExportHighlightingTask::report() {
    return ReportResult_Finished;
}

QString ExportHighlightingTask::generateReport() const {
    QString res;
    if (!isCanceled() && !hasError()) {
        res += "<b>" + tr("Export highlighting finished successfully") + "</b><br><b>" + tr("Result file:") + "</b> " + url.getURLString();
    }
    return res;
}

QString ExportHighlightingTask::exportHighlighting(int startPos, int endPos, int startingIndex, bool keepGaps, bool dots, bool transpose) {
    CHECK(msaEditor != nullptr, QString());
    SAFE_POINT(msaEditor->getReferenceRowId() != U2MsaRow::INVALID_ROW_ID, "Export highlighting is not supported without a reference", QString());
    QStringList result;

    MultipleAlignmentObject *maObj = msaEditor->getMaObject();
    assert(maObj != nullptr);

    const MultipleAlignment msa = maObj->getMultipleAlignment();

    U2OpStatusImpl os;
    int refSeq = msa->getRowIndexByRowId(msaEditor->getReferenceRowId(), os);
    SAFE_POINT_OP(os, QString());
    MultipleAlignmentRow row = msa->getRow(refSeq);

    QString header;
    header.append("Position\t");
    QString refSeqName = msaEditor->getReferenceRowName();
    header.append(refSeqName);
    header.append("\t");
    foreach (QString name, maObj->getMultipleAlignment()->getRowNames()) {
        if (name != refSeqName) {
            header.append(name);
            header.append("\t");
        }
    }
    header.remove(header.length() - 1, 1);
    result.append(header);

    int posInResult = startingIndex;

    for (int pos = startPos - 1; pos < endPos; pos++) {
        QString rowStr;
        rowStr.append(QString("%1").arg(posInResult));
        rowStr.append(QString("\t") + QString(msa->charAt(refSeq, pos)) + QString("\t"));
        bool informative = false;
        for (int seq = 0; seq < msa->getNumRows(); seq++) {    //FIXME possible problems when sequences have moved in view
            if (seq == refSeq)
                continue;
            char c = msa->charAt(seq, pos);

            const char refChar = row->charAt(pos);
            if (refChar == '-' && !keepGaps) {
                continue;
            }

            QColor unused;
            bool highlight = false;
            MSAEditorSequenceArea *sequenceArea = msaEditor->getUI()->getSequenceArea();
            MsaHighlightingScheme *scheme = sequenceArea->getCurrentHighlightingScheme();
            scheme->setUseDots(sequenceArea->getUseDotsCheckedState());
            scheme->process(refChar, c, unused, highlight, pos, seq);

            if (highlight) {
                rowStr.append(c);
                informative = true;
            } else {
                if (dots) {
                    rowStr.append(".");
                } else {
                    rowStr.append(" ");
                }
            }
            rowStr.append("\t");
        }
        if (informative) {
            header.remove(rowStr.length() - 1, 1);
            result.append(rowStr);
        }
        posInResult++;
    }

    if (!transpose) {
        QStringList transposedRows = TextUtils::transposeCSVRows(result, "\t");
        return transposedRows.join("\n");
    }

    return result.join("\n");
}

}    // namespace U2
