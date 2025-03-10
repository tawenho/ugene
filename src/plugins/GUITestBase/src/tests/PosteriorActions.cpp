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

#include <GTGlobals.h>
#include <base_dialogs/MessageBoxFiller.h>
#include <drivers/GTKeyboardDriver.h>
#include <primitives/GTMenu.h>
#include <primitives/GTWidget.h>
#include <system/GTClipboard.h>
#include <system/GTFile.h>
#include <utils/GTUtilsDialog.h>

#include <QApplication>
#include <QDir>
#include <QGuiApplication>
#include <QProcess>
#include <QTreeView>

#include <U2Core/AppContext.h>
#include <U2Core/Log.h>
#include <U2Core/Task.h>

#include "GTUtilsMdi.h"
#include "GTUtilsProjectTreeView.h"
#include "GTUtilsTaskTreeView.h"
#include "PosteriorActions.h"
#include "runnables/ugene/ugeneui/SaveProjectDialogFiller.h"
#include "runnables/ugene/ugeneui/AnyDialogFiller.h"

#include "utils/GTUtilsMac.h"

namespace U2 {
namespace GUITest_posterior_actions {

POSTERIOR_ACTION_DEFINITION(post_action_0000) {
    // Release all hold keyboard modifier keys

    Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
    if (modifiers & Qt::ShiftModifier) {
        GTKeyboardDriver::keyRelease(Qt::Key_Shift);
    }

    if (modifiers & Qt::ControlModifier) {
        GTKeyboardDriver::keyRelease(Qt::Key_Control);
    }

    if (modifiers & Qt::AltModifier) {
        GTKeyboardDriver::keyRelease(Qt::Key_Alt);
    }

    if (modifiers != 0) {
        coreLog.info(QString("post_action_0000: next keyboard modifiers were pressed after the test: %1").arg(modifiers));
    }
}

POSTERIOR_ACTION_DEFINITION(post_action_0001) {
    // Close all popup widgets
    // Close all modal widgets
    // Clear the clipboard

    QWidget *popupWidget = QApplication::activePopupWidget();
    while (popupWidget != NULL) {

#ifdef Q_OS_MAC
    GTUtilsMac fakeClock;
    fakeClock.startWorkaroundForMacCGEvents(1, true);
    fakeClock.startWorkaroundForMacCGEvents(16000, false);
#endif
        GTWidget::close(os, popupWidget);
        popupWidget = QApplication::activePopupWidget();
    }

    QWidget *modalWidget = QApplication::activeModalWidget();
    while (modalWidget != NULL) {

#ifdef Q_OS_MAC
        GTUtilsMac fakeClock;
        fakeClock.startWorkaroundForMacCGEvents(1, true);
        fakeClock.startWorkaroundForMacCGEvents(16000, false);
#endif
        GTWidget::close(os, modalWidget);
        modalWidget = QApplication::activeModalWidget();
    }

    GTClipboard::clear(os);
}

POSTERIOR_ACTION_DEFINITION(post_action_0002) {
    // Clear the project
    // Close the project
    // Close all MDI windows
    // Cancel all tasks

#ifdef Q_OS_MAC
    GTUtilsMac fakeClock;
    fakeClock.startWorkaroundForMacCGEvents(1, true);
    fakeClock.startWorkaroundForMacCGEvents(16000, false);
#endif

    if (AppContext::getProject() != nullptr) {
#ifdef Q_OS_MAC
        GTWidget::click(os, GTUtilsProjectTreeView::getTreeView(os));
        GTKeyboardDriver::keyClick('a', Qt::ControlModifier);
        GTGlobals::sleep(100);

        GTUtilsDialog::waitForDialog(os, new AnyDialogFiller(os,
                                                             nullptr,
                                                             QDialogButtonBox::No));
        // Need to close second dialog on Mac
        GTUtilsDialog::waitForDialogWhichMayRunOrNot(os, new AnyDialogFiller(os,
                                                                             nullptr,
                                                                             QDialogButtonBox::No));

        GTKeyboardDriver::keyClick(Qt::Key_Delete);
        GTGlobals::sleep(500);
        GTUtilsTaskTreeView::waitTaskFinished(os, 100000);
        GTGlobals::sleep(2000);

        GTUtilsDialog::waitForDialog(os, new AppCloseMessageBoxDialogFiller(os));
        GTMenu::clickMainMenuItem(os, QStringList() << "File"
                                                    << "Close project");
        GTGlobals::sleep(500);
        GTUtilsTaskTreeView::waitTaskFinished(os, 10000);
        GTGlobals::sleep(2000);

        GTUtilsDialog::cleanup(os, GTUtilsDialog::NoFailOnUnfinished);
#else
        GTWidget::click(os, GTUtilsProjectTreeView::getTreeView(os));
        GTKeyboardDriver::keyClick('a', Qt::ControlModifier);
        GTGlobals::sleep(100);

        GTUtilsDialog::waitForDialog(os, new SaveProjectDialogFiller(os, QDialogButtonBox::No));
        GTUtilsDialog::waitForDialog(os, new AppCloseMessageBoxDialogFiller(os));
        GTKeyboardDriver::keyClick(Qt::Key_Delete);
        GTGlobals::sleep(500);
        GTKeyboardDriver::keyClick('q', Qt::ControlModifier);
        GTGlobals::sleep(500);

        GTUtilsDialog::cleanup(os, GTUtilsDialog::NoFailOnUnfinished);
#endif
    }

#ifdef Q_OS_MAC
    GTUtilsMac fakeClock2;
    fakeClock.startWorkaroundForMacCGEvents(1, true);
    fakeClock2.startWorkaroundForMacCGEvents(16000, false);
#endif

    GTUtilsMdi::closeAllWindows(os);
    AppContext::getTaskScheduler()->cancelAllTasks();
    GTUtilsTaskTreeView::waitTaskFinished(os, 10000);
}

POSTERIOR_ACTION_DEFINITION(post_action_0003) {
    if (qgetenv("UGENE_TEST_SKIP_BACKUP_AND_RESTORE") == "1") {    // Restored by the parent process
        qDebug("Skipping restore: UGENE_TEST_SKIP_BACKUP_AND_RESTORE = 1");
        return;
    }
    // Restore backup files
    if (QDir(testDir).exists()) {
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj1.uprj");
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj2-1.uprj");
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj2.uprj");
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj3.uprj");
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj4.uprj");
        GTFile::restore(os, testDir + "_common_data/scenarios/project/proj5.uprj");

        // Files from the projects above.
        GTFile::restore(os, testDir + "_common_data/scenarios/project/1.gb");
    }
}

POSTERIOR_ACTION_DEFINITION(post_action_0004) {
    if (qgetenv("UGENE_TEST_SKIP_BACKUP_AND_RESTORE") == "1") {    // Restored by the parent process
        qDebug("Skipping restore: UGENE_TEST_SKIP_BACKUP_AND_RESTORE = 1");
        return;
    }
    if (QDir(sandBoxDir).exists()) {
        GTFile::setReadWrite(os, sandBoxDir, true);
        QDir sandBox(sandBoxDir);
        const QStringList entryList = sandBox.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden);
        for (const QString &path: qAsConst(entryList)) {
            GTFile::removeDir(sandBox.absolutePath() + "/" + path);
        }
    }
}

}    // namespace GUITest_posterior_actions
}    // namespace U2
