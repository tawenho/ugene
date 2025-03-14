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

#include "MaEditor.h"

#include <QFontDialog>

#include <U2Algorithm/MsaHighlightingScheme.h>

#include <U2Core/AppContext.h>
#include <U2Core/Counter.h>
#include <U2Core/DocumentModel.h>
#include <U2Core/MultipleSequenceAlignmentObject.h>
#include <U2Core/SaveDocumentTask.h>
#include <U2Core/Settings.h>
#include <U2Core/U2DbiUtils.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include <U2Gui/ExportDocumentDialogController.h>
#include <U2Gui/ExportObjectUtils.h>
#include <U2Gui/GUIUtils.h>

#include <U2View/MSAEditorOffsetsView.h>
#include <U2View/MSAEditorOverviewArea.h>
#include <U2View/MSAEditorSequenceArea.h>

#include "MaEditorState.h"
#include "MaEditorTasks.h"
#include "helpers/ScrollController.h"

namespace U2 {

SNPSettings::SNPSettings()
    : seqId(U2MsaRow::INVALID_ROW_ID) {
}

const float MaEditor::zoomMult = 1.25;

MaEditor::MaEditor(GObjectViewFactoryId factoryId, const QString &viewName, GObject *obj)
    : GObjectView(factoryId, viewName),
      ui(NULL),
      resizeMode(ResizeMode_FontAndContent),
      minimumFontPointSize(6),
      maximumFontPointSize(24),
      zoomFactor(0),
      cachedColumnWidth(0),
      cursorPosition(QPoint(0, 0)),
      rowOrderMode(MaEditorRowOrderMode::Original),
      exportHighlightedAction(NULL),
      clearSelectionAction(NULL) {
    GCOUNTER(cvar, factoryId);

    maObject = qobject_cast<MultipleAlignmentObject *>(obj);
    objects.append(maObject);

    onObjectAdded(maObject);

    requiredObjects.append(maObject);

    if (!U2DbiUtils::isDbiReadOnly(maObject->getEntityRef().dbiRef)) {
        U2OpStatus2Log os;
        maObject->setTrackMod(os, TrackOnUpdate);
    }

    // SANGER_TODO: move to separate method
    // do that in createWidget along with initActions?
    saveAlignmentAction = new QAction(QIcon(":core/images/msa_save.png"), tr("Save alignment"), this);
    saveAlignmentAction->setObjectName("Save alignment");
    connect(saveAlignmentAction, SIGNAL(triggered()), SLOT(sl_saveAlignment()));

    saveAlignmentAsAction = new QAction(QIcon(":core/images/msa_save_as.png"), tr("Save alignment as"), this);
    saveAlignmentAsAction->setObjectName("Save alignment as");
    connect(saveAlignmentAsAction, SIGNAL(triggered()), SLOT(sl_saveAlignmentAs()));

    zoomInAction = new QAction(QIcon(":core/images/zoom_in.png"), tr("Zoom In"), this);
    zoomInAction->setObjectName("Zoom In");
    connect(zoomInAction, SIGNAL(triggered()), SLOT(sl_zoomIn()));

    zoomOutAction = new QAction(QIcon(":core/images/zoom_out.png"), tr("Zoom Out"), this);
    zoomOutAction->setObjectName("Zoom Out");
    connect(zoomOutAction, SIGNAL(triggered()), SLOT(sl_zoomOut()));

    zoomToSelectionAction = new QAction(QIcon(":core/images/zoom_reg.png"), tr("Zoom To Selection"), this);
    zoomToSelectionAction->setObjectName("Zoom To Selection");
    connect(zoomToSelectionAction, SIGNAL(triggered()), SLOT(sl_zoomToSelection()));

    resetZoomAction = new QAction(QIcon(":core/images/zoom_whole.png"), tr("Reset Zoom"), this);
    resetZoomAction->setObjectName("Reset Zoom");
    connect(resetZoomAction, SIGNAL(triggered()), SLOT(sl_resetZoom()));

    changeFontAction = new QAction(QIcon(":core/images/font.png"), tr("Change Font"), this);
    changeFontAction->setObjectName("Change Font");
    connect(changeFontAction, SIGNAL(triggered()), SLOT(sl_changeFont()));

    exportHighlightedAction = new QAction(tr("Export highlighted"), this);
    exportHighlightedAction->setObjectName("Export highlighted");
    connect(exportHighlightedAction, SIGNAL(triggered()), this, SLOT(sl_exportHighlighted()));
    exportHighlightedAction->setDisabled(true);

    copyConsensusAction = new QAction(tr("Copy consensus"), this);
    copyConsensusAction->setObjectName("Copy consensus");

    copyConsensusWithGapsAction = new QAction(tr("Copy consensus with gaps"), this);
    copyConsensusWithGapsAction->setObjectName("Copy consensus with gaps");

    connect(maObject, SIGNAL(si_lockedStateChanged()), SLOT(sl_lockedStateChanged()));
    connect(maObject,
            SIGNAL(si_alignmentChanged(const MultipleAlignment &, const MaModificationInfo &)),
            SLOT(sl_onAlignmentChanged(const MultipleAlignment &, const MaModificationInfo &)));
    connect(this, SIGNAL(si_zoomOperationPerformed(bool)), SLOT(sl_resetColumnWidthCache()));
    connect(this, SIGNAL(si_fontChanged(QFont)), SLOT(sl_resetColumnWidthCache()));
}

void MaEditor::sl_onAlignmentChanged(const MultipleAlignment &, const MaModificationInfo &) {
    updateActions();
}

void MaEditor::sl_selectionChanged(const MaEditorSelection &, const MaEditorSelection &) {
    updateActions();
}

QVariantMap MaEditor::saveState() {
    return MaEditorState::saveState(this);
}

Task *MaEditor::updateViewTask(const QString &stateName, const QVariantMap &stateData) {
    return new UpdateMaEditorTask(this, stateName, stateData);
}

int MaEditor::getAlignmentLen() const {
    return maObject->getLength();
}

int MaEditor::getNumSequences() const {
    return maObject->getNumRows();
}

bool MaEditor::isAlignmentEmpty() const {
    return getAlignmentLen() == 0 || getNumSequences() == 0;
}

const MaEditorSelection &MaEditor::getSelection() const {
    return ui->getSequenceArea()->getSelection();
}

QRect MaEditor::getSelectionRect() const {
    return getSelection().toRect();
}

int MaEditor::getRowContentIndent(int) const {
    return 0;
}

int MaEditor::getSequenceRowHeight() const {
    QFontMetrics fm(font, ui);
    return fm.height() * zoomMult;
}

int MaEditor::getColumnWidth() const {
    if (cachedColumnWidth == 0) {
        cachedColumnWidth = getUnifiedSequenceFontCharRect(font).width() * zoomMult;

        cachedColumnWidth = (int)(cachedColumnWidth * zoomFactor);
        cachedColumnWidth = qMax(cachedColumnWidth, MOBJECT_MIN_COLUMN_WIDTH);
    }
    return cachedColumnWidth;
}

QVariantMap MaEditor::getHighlightingSettings(const QString &highlightingFactoryId) const {
    const QVariant v = snp.highlightSchemeSettings.value(highlightingFactoryId);
    if (v.isNull()) {
        return QVariantMap();
    }
    CHECK(v.type() == QVariant::Map, QVariantMap());
    return v.toMap();
}

void MaEditor::saveHighlightingSettings(const QString &highlightingFactoryId, const QVariantMap &settingsMap /* = QVariant()*/) {
    snp.highlightSchemeSettings.insert(highlightingFactoryId, QVariant(settingsMap));
}

void MaEditor::setReference(qint64 sequenceId) {
    exportHighlightedAction->setEnabled(sequenceId != U2MsaRow::INVALID_ROW_ID);
    if (snp.seqId != sequenceId) {
        snp.seqId = sequenceId;
        emit si_referenceSeqChanged(sequenceId);
    }
    //REDRAW OTHER WIDGETS
}

void MaEditor::updateReference() {
    if (maObject->getRowPosById(snp.seqId) == -1) {
        setReference(U2MsaRow::INVALID_ROW_ID);
    }
}

void MaEditor::resetCollapsibleModel() {
    MaCollapseModel *collapsibleModel = ui->getCollapseModel();
    SAFE_POINT(collapsibleModel != nullptr, "CollapseModel is null!", );
    collapsibleModel->reset(getMaRowIds());
}

void MaEditor::sl_zoomIn() {
    GCounter::increment("Zoom in", getFactoryId());
    int pSize = font.pointSize();

    if (resizeMode == ResizeMode_OnlyContent) {
        setZoomFactor(zoomFactor * zoomMult);
    } else if (pSize < maximumFontPointSize && resizeMode == ResizeMode_FontAndContent) {
        font.setPointSize(pSize + 1);
        setFont(font);
    }

    bool resizeModeChanged = false;

    if (zoomFactor >= 1) {
        ResizeMode oldMode = resizeMode;
        resizeMode = ResizeMode_FontAndContent;
        resizeModeChanged = resizeMode != oldMode;
        setZoomFactor(1);
    }
    updateActions();

    emit si_zoomOperationPerformed(resizeModeChanged);
}

void MaEditor::sl_zoomOut() {
    GCounter::increment("Zoom out", getFactoryId());
    int fontPointSize = font.pointSize();

    bool resizeModeChanged = false;

    if (fontPointSize > minimumFontPointSize) {
        font.setPointSize(fontPointSize - 1);
        setFont(font);
    } else {
        SAFE_POINT(zoomMult > 0, QString("Incorrect value of MSAEditor::zoomMult"), );
        setZoomFactor(zoomFactor / zoomMult);
        ResizeMode oldMode = resizeMode;
        resizeMode = ResizeMode_OnlyContent;
        resizeModeChanged = resizeMode != oldMode;
    }
    updateActions();

    emit si_zoomOperationPerformed(resizeModeChanged);
}

void MaEditor::sl_zoomToSelection() {
    ResizeMode oldMode = resizeMode;
    int seqAreaWidth = ui->getSequenceArea()->width();
    MaEditorSelection selection = ui->getSequenceArea()->getSelection();
    if (selection.isEmpty()) {
        return;
    }
    int selectionWidth = selection.width();
    float pixelsPerBase = (seqAreaWidth / float(selectionWidth)) * zoomMult;
    int fontPointSize = int(pixelsPerBase / fontPixelToPointSize);
    if (fontPointSize >= minimumFontPointSize) {
        fontPointSize = qMin(fontPointSize, maximumFontPointSize);
        font.setPointSize(fontPointSize);
        setFont(font);
        resizeMode = ResizeMode_FontAndContent;
        setZoomFactor(1);
    } else {
        if (font.pointSize() != minimumFontPointSize) {
            font.setPointSize(minimumFontPointSize);
            setFont(font);
        }
        setZoomFactor(pixelsPerBase / (minimumFontPointSize * fontPixelToPointSize));
        resizeMode = ResizeMode_OnlyContent;
    }
    ui->getScrollController()->setFirstVisibleBase(selection.x());
    ui->getScrollController()->setFirstVisibleViewRow(selection.y());

    updateActions();

    emit si_zoomOperationPerformed(resizeMode != oldMode);
}

void MaEditor::sl_resetZoom() {
    GCounter::increment("Reset zoom", getFactoryId());
    QFont f = getFont();
    f.setPointSize(MOBJECT_DEFAULT_FONT_SIZE);
    setFont(f);
    setZoomFactor(MOBJECT_DEFAULT_ZOOM_FACTOR);
    ResizeMode oldMode = resizeMode;
    resizeMode = ResizeMode_FontAndContent;
    emit si_zoomOperationPerformed(resizeMode != oldMode);

    updateActions();
}

void MaEditor::sl_saveAlignment() {
    AppContext::getTaskScheduler()->registerTopLevelTask(new SaveDocumentTask(maObject->getDocument()));
}

void MaEditor::sl_saveAlignmentAs() {
    Document *srcDoc = maObject->getDocument();
    if (srcDoc == NULL) {
        return;
    }
    if (!srcDoc->isLoaded()) {
        return;
    }

    QObjectScopedPointer<ExportDocumentDialogController> dialog = new ExportDocumentDialogController(srcDoc, ui);
    dialog->setAddToProjectFlag(true);
    dialog->setWindowTitle(tr("Save Alignment"));
    ExportObjectUtils::export2Document(dialog);
}

void MaEditor::sl_changeFont() {
    bool ok = false;
    GCounter::increment("Change of the characters font", getFactoryId());
    // QFontDialog::DontUseNativeDialog - no color selector, affects only Mac OS
    QFont f = QFontDialog::getFont(&ok, font, widget, tr("Characters Font"), QFontDialog::DontUseNativeDialog);
    if (!ok) {
        return;
    }
    setFont(f);
    updateActions();
    emit si_completeUpdate();
}

void MaEditor::sl_lockedStateChanged() {
    updateActions();
}

void MaEditor::sl_exportHighlighted() {
    QObjectScopedPointer<ExportHighligtingDialogController> d = new ExportHighligtingDialogController(ui, (QWidget *)AppContext::getMainWindow()->getQMainWindow());
    d->exec();
    CHECK(!d.isNull(), );

    if (d->result() == QDialog::Accepted) {
        AppContext::getTaskScheduler()->registerTopLevelTask(new ExportHighlightingTask(d.data(), this));
    }
}

void MaEditor::sl_resetColumnWidthCache() {
    cachedColumnWidth = 0;
}

void MaEditor::initActions() {
    saveScreenshotAction = new QAction(QIcon(":/core/images/cam2.png"), tr("Export as image"), this);
    saveScreenshotAction->setObjectName("Export as image");
    connect(saveScreenshotAction, SIGNAL(triggered()), ui, SLOT(sl_saveScreenshot()));
    ui->addAction(saveScreenshotAction);

    showOverviewAction = new QAction(QIcon(":/core/images/msa_show_overview.png"), tr("Overview"), this);
    showOverviewAction->setObjectName("Show overview");
    showOverviewAction->setCheckable(true);
    showOverviewAction->setChecked(true);
    connect(showOverviewAction, SIGNAL(triggered()), ui->getOverviewArea(), SLOT(sl_show()));
    ui->addAction(showOverviewAction);

    clearSelectionAction = new QAction(tr("Clear selection"), this);
    clearSelectionAction->setShortcut(Qt::Key_Escape);
    connect(clearSelectionAction, SIGNAL(triggered()), SIGNAL(si_clearSelection()));
    ui->addAction(clearSelectionAction);

    connect(this, SIGNAL(si_clearSelection()), ui->getSequenceArea(), SLOT(sl_cancelSelection()));

    connect(ui->getSequenceArea(),
            SIGNAL(si_selectionChanged(const MaEditorSelection &, const MaEditorSelection &)),
            SLOT(sl_selectionChanged(const MaEditorSelection &, const MaEditorSelection &)));
}

void MaEditor::initZoom() {
    Settings *s = AppContext::getSettings();
    SAFE_POINT(s != NULL, "AppContext is NULL", );
    zoomFactor = s->getValue(getSettingsRoot() + MOBJECT_SETTINGS_ZOOM_FACTOR, MOBJECT_DEFAULT_ZOOM_FACTOR).toFloat();
    updateResizeMode();
}

void MaEditor::initFont() {
    Settings *s = AppContext::getSettings();
    SAFE_POINT(s != NULL, "AppContext is NULL", );
    font.setFamily(s->getValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_FAMILY, MOBJECT_DEFAULT_FONT_FAMILY).toString());
    font.setPointSize(s->getValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_SIZE, MOBJECT_DEFAULT_FONT_SIZE).toInt());
    font.setItalic(s->getValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_ITALIC, false).toBool());
    font.setBold(s->getValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_BOLD, false).toBool());

    updateFontMetrics();
}

void MaEditor::updateResizeMode() {
    resizeMode = font.pointSize() >= minimumFontPointSize && zoomFactor < 1.0f ? ResizeMode_OnlyContent : ResizeMode_FontAndContent;
}

void MaEditor::addCopyPasteMenu(QMenu *m) {
    QMenu *cm = m->addMenu(tr("Copy/Paste"));
    cm->menuAction()->setObjectName(MSAE_MENU_COPY);
}

void MaEditor::addExportMenu(QMenu *m) {
    QMenu *em = m->addMenu(tr("Export"));
    em->menuAction()->setObjectName(MSAE_MENU_EXPORT);
    em->addAction(exportHighlightedAction);
    if (!ui->getSequenceArea()->getCurrentHighlightingScheme()->getFactory()->isRefFree() &&
        getReferenceRowId() != U2MsaRow::INVALID_ROW_ID) {
        exportHighlightedAction->setEnabled(true);
    } else {
        exportHighlightedAction->setDisabled(true);
    }
}

void MaEditor::addLoadMenu(QMenu *m) {
    QMenu *lsm = m->addMenu(tr("Add"));
    lsm->menuAction()->setObjectName(MSAE_MENU_LOAD);
}

void MaEditor::addAlignMenu(QMenu *m) {
    QMenu *em = m->addMenu(tr("Align"));
    em->setIcon(QIcon(":core/images/align.png"));
    em->menuAction()->setObjectName(MSAE_MENU_ALIGN);
}

void MaEditor::setFont(const QFont &f) {
    int pSize = f.pointSize();
    font = f;
    updateFontMetrics();
    font.setPointSize(qBound(minimumFontPointSize, pSize, maximumFontPointSize));
    updateResizeMode();
    emit si_fontChanged(font);

    Settings *s = AppContext::getSettings();
    s->setValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_FAMILY, f.family());
    s->setValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_SIZE, f.pointSize());
    s->setValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_ITALIC, f.italic());
    s->setValue(getSettingsRoot() + MOBJECT_SETTINGS_FONT_BOLD, f.bold());
    widget->update();
}

void MaEditor::updateFontMetrics() {
    QFontInfo fontInfo(font);
    fontPixelToPointSize = fontInfo.pixelSize() / fontInfo.pointSizeF();

    // Re-calculate the minimumFontPointSize based on the current font.
    // The minimum font size is computed using the similar logic with Assembly Browser:
    // it is equal to maximum font size value that can safely be rendered in the pre-defined on-screen cell of width = 7px.
    // This approach is good, because UGENE continues to draw sequence characters until there is enough pixels in the cell to draw a
    // meaningful shape regardless of the current device settings.
    // TODO: this logic should be refactored and centralized for all sequence views in UGENE with zooming support.
    const int minimumCellWidthToShowText = 7;
    const int minimumFontCharWidthInsideCell = minimumCellWidthToShowText - 2;    // Keep 1px left & right padding inside the cell.
    const int minimumSafeFontPointSize = 8;    // This value was historically used in UGENE as minimum with no known issues.
    QFont fontToEstimate = font;
    int estimatedMinimumFontPointSize = minimumSafeFontPointSize;    // Start with a safe value and estimate smaller values.
    while (fontToEstimate.pointSize() > 0) {
        int charWidth = getUnifiedSequenceFontCharRect(fontToEstimate).width();
        if (charWidth < minimumFontCharWidthInsideCell) {
            // The estimated char size is too small. Stop on the previous value.
            break;
        }
        estimatedMinimumFontPointSize = fontToEstimate.pointSize();
        fontToEstimate.setPointSize(fontToEstimate.pointSize() - 1);
    }
    minimumFontPointSize = estimatedMinimumFontPointSize;
}

void MaEditor::setFirstVisiblePosSeq(int firstPos, int firstSeq) {
    if (ui->getSequenceArea()->isPosInRange(firstPos)) {
        ui->getScrollController()->setFirstVisibleBase(firstPos);
        ui->getScrollController()->setFirstVisibleMaRow(firstSeq);
    }
}

void MaEditor::setZoomFactor(double newZoomFactor) {
    zoomFactor = newZoomFactor;
    updateResizeMode();
    Settings *s = AppContext::getSettings();
    s->setValue(getSettingsRoot() + MOBJECT_SETTINGS_ZOOM_FACTOR, zoomFactor);
    sl_resetColumnWidthCache();
}

void MaEditor::updateActions() {
    zoomInAction->setEnabled(font.pointSize() < maximumFontPointSize);
    zoomOutAction->setEnabled(getColumnWidth() > MOBJECT_MIN_COLUMN_WIDTH);
    zoomToSelectionAction->setEnabled(font.pointSize() < maximumFontPointSize);
    changeFontAction->setEnabled(resizeMode == ResizeMode_FontAndContent);
    emit si_updateActions();
}

const QPoint &MaEditor::getCursorPosition() const {
    return cursorPosition;
}

void MaEditor::setCursorPosition(const QPoint &newCursorPosition) {
    CHECK(cursorPosition != newCursorPosition, );
    int x = newCursorPosition.x(), y = newCursorPosition.y();
    CHECK(x >= 0 && y >= 0 && x < getAlignmentLen() && y < getNumSequences(), );
    cursorPosition = newCursorPosition;
    emit si_cursorPositionChanged(cursorPosition);
}

QList<qint64> MaEditor::getMaRowIds() const {
    return maObject->getMultipleAlignment()->getRowsIds();
}

void MaEditor::selectRows(int firstViewRowIndex, int numberOfRows) {
    MaEditorSelection selection(0, firstViewRowIndex, getAlignmentLen(), numberOfRows);
    ui->getSequenceArea()->setSelection(selection);
}

QRect MaEditor::getUnifiedSequenceFontCharRect(const QFont &sequenceFont) const {
    QFontMetrics fontMetrics(sequenceFont, ui);
    return fontMetrics.boundingRect('W');
}

MaEditorRowOrderMode MaEditor::getRowOrderMode() const {
    return rowOrderMode;
}

void MaEditor::setRowOrderMode(MaEditorRowOrderMode mode) {
    rowOrderMode = mode;
}

}    // namespace U2
