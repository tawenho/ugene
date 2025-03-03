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

#ifndef _U2_MSAEditor_TREE_VIEWER_H_
#define _U2_MSAEditor_TREE_VIEWER_H_

#include <QGraphicsLineItem>
#include <QMap>

#include <U2Algorithm/CreatePhyTreeSettings.h>

#include "U2View/TreeViewer.h"

namespace U2 {

typedef QMap<QString, QColor> GroupColorSchema;

class MSAEditorTreeViewerUI;
class MSAEditor;

class ColorGenerator {
public:
    ColorGenerator(int countOfColors, qreal lightness);

    void setCountOfColors(int counts);

    QColor getColor(int index) const;

    int getCountOfColors() const {
        return countOfColors;
    }

private:
    void generateColors();
    int countOfColors;
    QList<QColor> colors;
    QList<qreal> satValues;
    qreal delta;
    qreal hue;
    qreal lightness;
};

class MSAEditorTreeViewer : public TreeViewer {
    Q_OBJECT
public:
    MSAEditorTreeViewer(const QString &viewName, GObject *obj, GraphicsRectangularBranchItem *root, qreal scale);

    const CreatePhyTreeSettings &getCreatePhyTreeSettings() {
        return buildSettings;
    }
    const QString &getParentAlignmentName() {
        return alignmentName;
    }
    virtual OptionsPanel *getOptionsPanel() {
        return 0;
    }

    void setCreatePhyTreeSettings(const CreatePhyTreeSettings &newBuildSettings);
    void setParentAignmentName(const QString &_alignmentName) {
        alignmentName = _alignmentName;
    }

    QAction *getSortSeqsAction() const {
        return syncModeAction;
    }

    /**
     * Enables Tree & MSA synchronization. See 'syncModeAction' for the details about the sync mode.
     * Returns true if the enableSyncMode mode is enabled or false if it can't be enabled.
     * The 'sync' mode may not be enabled if the msa sequence list does not match tree branches anymore.
     */
    bool enableSyncMode();

    /** Disables Tree&MSA synchronization. See 'syncModeAction' for the details about the sync mode. */
    void disableSyncMode();

    /** Returns true if Tree and MSA are in 'sync' mode. */
    bool isSyncModeEnabled() const;

    void setMSAEditor(MSAEditor *newEditor);

    MSAEditor *getMsaEditor() const;

    /** Updates order of sequences and collapsed group states in the alignment using the tree state as the reference. */
    void orderAlignmentByTree();

protected:
    virtual QWidget *createWidget();

private slots:

    /** A callback for 'syncModeAction'. Enables or disables 'sync' mode based on the 'syncModeAction' checked state. */
    void sl_syncModeActionTriggered();

    void sl_refreshTree();

    /** Callback on alignment change. Disables sync mode if needed. See 'syncModeAction' for details. */
    void sl_alignmentChanged();

    /** Callback on alignment collapse model change. Disables sync mode if needed. See 'syncModeAction' for details. */
    void sl_alignmentCollapseModelChanged();

signals:
    void si_refreshTree(MSAEditorTreeViewer *treeViewer);

private:
    /** Update internal state of the syncModeAction: text, icon, checked state. */
    void updateSyncModeActionState(bool isSyncModeOn);

    /** Checks if Tree and MSA name lists are synchronized: use the same order & collapse states. */
    bool checkTreeAndMsaNameListsAreSynchronized() const;

    /** Checks if Tree and MSA can be synchronized. When there is no way to sync Tree & MSA the 'syncModeAction' is disabled. */
    bool checkTreeAndMsaCanBeSynchronized() const;

    /** Checks if Tree & MSA are topologically synchronized and if not disables 'sync' mode. */
    void disableSyncModeIfTreeAndMsaContentIsNotInSync();

    /** Re-calculates the tree using originally used parameters & the current MSA state. */
    QAction *refreshTreeAction;

    /**
     * Sync mode action is used to enable or disable 'sync' mode between Tree & MSA view.
     *
     * When sync mode is enabled:
     *  - The order of sequences in MSA follows the order of sequences in the Tree.
     *  - When a tree branch is collapsed/expanded the same transformation is done for the MSA list.
     *
     * The sync mode is automatically disabled on the following modifications from the MSA side:
     *  - A sequence is modified/renamed.
     *  - New sequences added/removed from MSA.
     *  - Order of sequences or grouping (collapsing) mode changed not by the Tree.
     *
     * The sync mode between the Tree and MSA can be re-enabled using this action if the Tree and MSA have the same number of sequences matched by name.
     *
     * States of the action:
     *  - Enabled: the sync mode is possible. Use 'isChecked' if the sync mode is ON or OFF.
     *  - Checked: the sync mode is ON. Note: the action can't be checked in the disabled state.
     */
    QAction *syncModeAction;
    QString alignmentName;
    CreatePhyTreeSettings buildSettings;
    MSAEditor *editor;
    MSAEditorTreeViewerUI *msaTreeViewerUi;
};

class U2VIEW_EXPORT MSAEditorTreeViewerUI : public TreeViewerUI {
    Q_OBJECT

public:
    MSAEditorTreeViewerUI(MSAEditorTreeViewer *treeViewer);
    virtual ~MSAEditorTreeViewerUI() {
        //Clear groups highlighting in the MSAEditor
        emit si_groupColorsChanged(GroupColorSchema());
    }

    /** Update tree sub-widgets state to match the current enableSyncMode mode. */
    void updateSyncModeState(bool isSyncModeOn);

    bool isCurTreeViewerSynchronized() const;

    void highlightBranches();

    /**
     * Return virtual grouping state for MSA that corresponds to the current tree state.
     * All sequences are ordered by 'y' position. All collapsed branches are mapped to the virtual groups.
     */
    QList<QStringList> getGroupingStateForMsa() const;

protected:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *me);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);

    virtual void onLayoutChanged(const TreeLayout &layout);
    void onSettingsChanged(TreeViewOption option, const QVariant &newValue);
    virtual void updateTreeSettings(bool setDefaultZoom = true);
    virtual void setTreeLayout(TreeLayout newLayout);

signals:
    void si_groupColorsChanged(const GroupColorSchema &schema);
    void si_zoomIn();
    void si_zoomOut();
    void si_resetZooming();

protected slots:
    void sl_zoomToAll();
    void sl_zoomToSel();
    void sl_zoomOut();

private slots:
    void sl_selectionChanged(const QStringList &selectedSequenceNameList);
    void sl_sequenceNameChanged(QString prevName, QString newName);
    void sl_onReferenceSeqChanged(qint64);
    void sl_onSceneRectChanged(const QRectF &);
    virtual void sl_rectLayoutRecomputed();
    void sl_onVisibleRangeChanged(QStringList visibleSeqs, int height);
    virtual void sl_onBranchCollapsed(GraphicsRectangularBranchItem *branch);

private:
    QList<GraphicsBranchItem *> getBranchItemsWithNames() const;

    QGraphicsLineItem *subgroupSelector;
    qreal subgroupSelectorPos;
    bool subgroupSelectionMode;
    ColorGenerator groupColors;

    bool isRectangularLayout;

    MSAEditorTreeViewer *const msaEditorTreeViewer;

    QTransform rectangularTransform;
};

class MSAEditorTreeViewerUtils {
private:
    MSAEditorTreeViewerUtils();

public:
    static QStringList getSeqsNamesInBranch(const GraphicsBranchItem *branch);
};

}    // namespace U2
#endif
