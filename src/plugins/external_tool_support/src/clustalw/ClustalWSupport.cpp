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

#include "ClustalWSupport.h"

#include <QMainWindow>
#include <QMessageBox>

#include <U2Core/AppContext.h>
#include <U2Core/AppSettings.h>
#include <U2Core/MultipleSequenceAlignmentObject.h>
#include <U2Core/QObjectScopedPointer.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/UserApplicationsSettings.h>

#include <U2Gui/GUIUtils.h>

#include <U2View/MSAEditor.h>
#include <U2View/MaEditorFactory.h>

#include "ClustalWSupportRunDialog.h"
#include "ClustalWSupportTask.h"
#include "ExternalToolSupportSettings.h"
#include "ExternalToolSupportSettingsController.h"
#include "utils/AlignMsaAction.h"

namespace U2 {

const QString ClustalWSupport::ET_CLUSTAL_ID = "USUPP_CLUSTALW";
const QString ClustalWSupport::CLUSTAL_TMP_DIR = "clustal";

ClustalWSupport::ClustalWSupport()
    : ExternalTool(ClustalWSupport::ET_CLUSTAL_ID, "clustalw", "ClustalW") {
    if (AppContext::getMainWindow() != nullptr) {
        viewCtx = new ClustalWSupportContext(this);
        icon = QIcon(":external_tool_support/images/clustalx.png");
        grayIcon = QIcon(":external_tool_support/images/clustalx_gray.png");
        warnIcon = QIcon(":external_tool_support/images/clustalx_warn.png");
    }
#ifdef Q_OS_WIN
    executableFileName = "clustalw2.exe";
#else
#    if defined(Q_OS_UNIX)
    executableFileName = "clustalw2";
#    endif
#endif
    validationArguments << "-help";
    validMessage = "CLUSTAL";
    description = tr("<i>ClustalW</i> is a free sequence alignment software for DNA or proteins.");
    versionRegExp = QRegExp("CLUSTAL (\\d+\\.\\d+) Multiple Sequence Alignments");
    toolKitName = "ClustalW";
}

void ClustalWSupport::sl_runWithExtFileSpecify() {
    //Check that Clustal and temporary folder path defined
    if (path.isEmpty()) {
        QObjectScopedPointer<QMessageBox> msgBox = new QMessageBox;
        msgBox->setWindowTitle(name);
        msgBox->setText(tr("Path for %1 tool not selected.").arg(name));
        msgBox->setInformativeText(tr("Do you want to select it now?"));
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::Yes);
        const int ret = msgBox->exec();
        CHECK(!msgBox.isNull(), );

        switch (ret) {
            case QMessageBox::Yes:
                AppContext::getAppSettingsGUI()->showSettingsDialog(ExternalToolSupportSettingsPageId);
                break;
            case QMessageBox::No:
                return;
            default:
                assert(false);
        }
    }
    if (path.isEmpty()) {
        return;
    }
    U2OpStatus2Log os(LogLevel_DETAILS);
    ExternalToolSupportSettings::checkTemporaryDir(os);
    CHECK_OP(os, );

    //Call select input file and setup settings dialog
    ClustalWSupportTaskSettings settings;
    QObjectScopedPointer<ClustalWWithExtFileSpecifySupportRunDialog> clustalWRunDialog = new ClustalWWithExtFileSpecifySupportRunDialog(settings, AppContext::getMainWindow()->getQMainWindow());
    clustalWRunDialog->exec();
    CHECK(!clustalWRunDialog.isNull(), );

    if (clustalWRunDialog->result() != QDialog::Accepted) {
        return;
    }
    assert(!settings.inputFilePath.isEmpty());

    ClustalWWithExtFileSpecifySupportTask *clustalWSupportTask = new ClustalWWithExtFileSpecifySupportTask(settings);
    AppContext::getTaskScheduler()->registerTopLevelTask(clustalWSupportTask);
}

////////////////////////////////////////
//ExternalToolSupportMSAContext
ClustalWSupportContext::ClustalWSupportContext(QObject *p)
    : GObjectViewWindowContext(p, MsaEditorFactory::ID) {
}

void ClustalWSupportContext::initViewContext(GObjectView *view) {
    MSAEditor *msaEditor = qobject_cast<MSAEditor *>(view);
    SAFE_POINT(msaEditor != NULL, "Invalid GObjectView", );
    CHECK(msaEditor->getMaObject() != NULL, );

    bool objLocked = msaEditor->getMaObject()->isStateLocked();
    bool isMsaEmpty = msaEditor->isAlignmentEmpty();

    AlignMsaAction *alignAction = new AlignMsaAction(this, ClustalWSupport::ET_CLUSTAL_ID, view, tr("Align with ClustalW..."), 2000);
    alignAction->setObjectName("Align with ClustalW");

    addViewAction(alignAction);
    alignAction->setEnabled(!objLocked && !isMsaEmpty);

    connect(msaEditor->getMaObject(), SIGNAL(si_lockedStateChanged()), alignAction, SLOT(sl_updateState()));
    connect(msaEditor->getMaObject(), SIGNAL(si_alignmentBecomesEmpty(bool)), alignAction, SLOT(sl_updateState()));
    connect(alignAction, SIGNAL(triggered()), SLOT(sl_align_with_ClustalW()));
}

void ClustalWSupportContext::buildMenu(GObjectView *view, QMenu *m) {
    QList<GObjectViewAction *> actions = getViewActions(view);
    QMenu *alignMenu = GUIUtils::findSubMenu(m, MSAE_MENU_ALIGN);
    SAFE_POINT(alignMenu != NULL, "alignMenu", );
    foreach (GObjectViewAction *a, actions) {
        a->addToMenuWithOrder(alignMenu);
    }
}

void ClustalWSupportContext::sl_align_with_ClustalW() {
    //Check that Clustal and temporary folder path defined
    if (AppContext::getExternalToolRegistry()->getById(ClustalWSupport::ET_CLUSTAL_ID)->getPath().isEmpty()) {
        QObjectScopedPointer<QMessageBox> msgBox = new QMessageBox;
        msgBox->setWindowTitle("ClustalW");
        msgBox->setText(tr("Path for ClustalW tool is not selected."));
        msgBox->setInformativeText(tr("Do you want to select it now?"));
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::Yes);
        const int ret = msgBox->exec();
        CHECK(!msgBox.isNull(), );

        switch (ret) {
            case QMessageBox::Yes:
                AppContext::getAppSettingsGUI()->showSettingsDialog(ExternalToolSupportSettingsPageId);
                break;
            case QMessageBox::No:
                return;
            default:
                assert(false);
        }
    }
    if (AppContext::getExternalToolRegistry()->getById(ClustalWSupport::ET_CLUSTAL_ID)->getPath().isEmpty()) {
        return;
    }
    U2OpStatus2Log os(LogLevel_DETAILS);
    ExternalToolSupportSettings::checkTemporaryDir(os);
    CHECK_OP(os, );

    //Call run ClustalW align dialog
    AlignMsaAction *action = qobject_cast<AlignMsaAction *>(sender());
    SAFE_POINT(action != nullptr, "Sender is not 'AlignMsaAction'", );
    MSAEditor *msaEditor = action->getMsaEditor();
    MultipleSequenceAlignmentObject *obj = msaEditor->getMaObject();
    if (obj == nullptr || obj->isStateLocked()) {
        return;
    }

    ClustalWSupportTaskSettings settings;
    QObjectScopedPointer<ClustalWSupportRunDialog> clustalWRunDialog = new ClustalWSupportRunDialog(obj->getMultipleAlignment(), settings, AppContext::getMainWindow()->getQMainWindow());
    clustalWRunDialog->exec();
    CHECK(!clustalWRunDialog.isNull(), );

    if (clustalWRunDialog->result() != QDialog::Accepted) {
        return;
    }

    ClustalWSupportTask *clustalWSupportTask = new ClustalWSupportTask(obj->getMultipleAlignment(), GObjectReference(obj), settings);
    connect(obj, SIGNAL(destroyed()), clustalWSupportTask, SLOT(cancel()));
    AppContext::getTaskScheduler()->registerTopLevelTask(clustalWSupportTask);

    // Turn off rows collapsing
    msaEditor->resetCollapsibleModel();
}

}    // namespace U2
