/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2019 UniPro <ugene@unipro.ru>
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

#include <U2Core/U2IdTypes.h>
#include "GTUtilsProjectTreeView.h"

#include <drivers/GTKeyboardDriver.h>
#include <drivers/GTMouseDriver.h>
#include <primitives/GTWidget.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeWidget>

#include "ConfigureTrimmomaticStepsFiller.h"

namespace U2 {
using namespace HI;

#define GT_CLASS_NAME "ConfigureTrimmomaticStepsDialogFiller"

ConfigureTrimmomaticStepsDialogFiller::ConfigureTrimmomaticStepsDialogFiller(HI::GUITestOpStatus &os, const QList<TrimmingSteps>& _steps)
    : Filler(os, "TrimmomaticPropertyDialog"),
      steps(_steps) {}

#define GT_METHOD_NAME "commonScenario"
void ConfigureTrimmomaticStepsDialogFiller::commonScenario(){
    GTGlobals::sleep(1000);
    QWidget *dialog = QApplication::activeModalWidget();
    GT_CHECK(dialog != NULL, "dialog was not found");




    GTUtilsDialog::clickButtonBox(os, dialog, QDialogButtonBox::Ok);
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "initStringSteps"
const QMap<ConfigureTrimmomaticStepsDialogFiller::TrimmingSteps, QString> ConfigureTrimmomaticStepsDialogFiller::initStringSteps() {
    QMap<ConfigureTrimmomaticStepsDialogFiller::TrimmingSteps, QString> steps;
    steps.insert(ILLUMINACLIP, "ILLUMINACLIP");
    steps.insert(SLIDINGWINDOW, "SLIDINGWINDOW");
    steps.insert(LEADING, "LEADING");
    steps.insert(TRAILING, "TRAILING");
    steps.insert(CROP, "CROP");
    steps.insert(HEADCROP, "HEADCROP");
    steps.insert(MINLEN, "MINLEN");
    steps.insert(AVGQUAL, "AVGQUAL");
    steps.insert(TOPHRED33, "TOPHRED33");
    steps.insert(TOPHRED64, "TOPHRED64");

    return steps;
}
#undef GT_METHOD_NAME

const QMap<ConfigureTrimmomaticStepsDialogFiller::TrimmingSteps, QString> ConfigureTrimmomaticStepsDialogFiller::stringSteps = ConfigureTrimmomaticStepsDialogFiller::initStringSteps();

#undef GT_CLASS_NAME

}
