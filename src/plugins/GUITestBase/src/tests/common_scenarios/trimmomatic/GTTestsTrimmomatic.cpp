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

#include <QTableWidget>
//#include <GTGlobals.h>
//#include <base_dialogs/GTFileDialog.h>
//#include <drivers/GTKeyboardDriver.h>
//#include <drivers/GTMouseDriver.h>
//#include <primitives/GTAction.h>
//#include <primitives/GTMenu.h>
//#include <primitives/GTTreeWidget.h>
//#include <primitives/GTTreeWidget.h>
//#include <primitives/GTWidget.h>
//#include <primitives/PopupChooser.h>
//#include <system/GTClipboard.h>
//#include <utils/GTUtilsDialog.h>
//
//#include <U2View/MSAEditor.h>

#include "GTTestsTrimmomatic.h"
#include "GTUtilsWorkflowDesigner.h"

#include <U2Core/AppContext.h>
#include <U2Core/AppResources.h>
#include <U2Core/AppSettings.h>
//#include "GTUtilsMsaEditor.h"
//#include "GTUtilsMsaEditorSequenceArea.h"
//#include "GTUtilsProjectTreeView.h"
//#include "GTUtilsTaskTreeView.h"
//#include "runnables/ugene/corelibs/U2Gui/util/RenameSequenceFiller.h"
//#include "runnables/ugene/corelibs/U2View/ov_msa/DeleteGapsDialogFiller.h"
//#include "runnables/ugene/corelibs/U2View/ov_msa/ExtractSelectedAsMSADialogFiller.h"
//#include "runnables/ugene/plugins_3rdparty/clustalw/ClustalWDialogFiller.h"
//#include "runnables/ugene/plugins_3rdparty/kalign/KalignDialogFiller.h"
//#include "runnables/ugene/plugins_3rdparty/umuscle/MuscleDialogFiller.h"

namespace U2{

namespace GUITest_common_scenarios_trimmomatic{
using namespace HI;

static const QString INPUT_DATA = "Input data";
static const QString TRIMMING_STEPS = "Trimming steps";
static const QString OUTPUT_FILE = "Output file";
static const QString GENERATE_DETAILED_LOG = "Generate detailed log";
static const QString LOG_FILE = "Log file";
static const QString NUMBER_OF_THREADS = "Number of threads";

static const QStringList INPUT_DATA_VALUES = { "SE reads",
                                               "PE reads" };

static const QString DEFAULT_TRIMMING_STEPS_VALUE = "Configure steps";

static const QString DEFAULT_OUTPUT_VALUE = "Auto";

static const QStringList GENERATE_DETAILED_LOG_VALUES = { "False",
                                                          "True" };

static const QString DEFAULT_LOG_FILE = "Auto";

GUI_TEST_CLASS_DEFINITION(test_0001){//DIFFERENCE: lock document is checked
    //1. Open WD
    GTUtilsWorkflowDesigner::openWorkflowDesigner(os);

    //2. Add the "Improve Reads with Trimmomatic" element to scene and click on it
    WorkflowProcessItem* element =  GTUtilsWorkflowDesigner::addElement(os, "Improve Reads with Trimmomatic");
    GTUtilsWorkflowDesigner::click(os, "Improve Reads with Trimmomatic");

    //Expected default parameters:
    //Input data, Trimming steps, Output file, Generate detailed log, Generate detailed log
    QStringList currentParameters = GTUtilsWorkflowDesigner::getAllParameters(os);
    CHECK_SET_ERR(currentParameters.size() == 5,
                  QString("Unexpected number of default parameters, expected: 5, current: %1")
                          .arg(currentParameters.size()));
    QStringList defaultParameters = { INPUT_DATA,
                                      TRIMMING_STEPS,
                                      OUTPUT_FILE,
                                      GENERATE_DETAILED_LOG,
                                      NUMBER_OF_THREADS };
    foreach(const QString& par, defaultParameters) {
        CHECK_SET_ERR(currentParameters.contains(par), QString("The default parameter \"%1\" is missed").arg(par));
    }

    {//3. Check "Input data"
        //Expected: current "Input data" value is "SE reads or contigs", input table has one line
        QString inputDataValue = GTUtilsWorkflowDesigner::getParameter(os, INPUT_DATA);
        CHECK_SET_ERR(inputDataValue == INPUT_DATA_VALUES.first(),
                      QString("Unexpected \"Input data\" value, expected: SE reads or contigs, current: %1")
                              .arg(inputDataValue));

        QTableWidget* inputTable = GTUtilsWorkflowDesigner::getInputPortsTable(os, 0);
        int row = inputTable->rowCount();
        CHECK_SET_ERR(row == 1,
                      QString("Unexpected \"Input data\" row count, expected: 1, current: %1")
                              .arg(row));

        //4. Set "Input data" value on "PE reads"
        GTUtilsWorkflowDesigner::setParameter(os,
                                              INPUT_DATA,
                                              INPUT_DATA_VALUES.last(),
                                              GTUtilsWorkflowDesigner::comboValue);

        //Expected: input table has two lines
        inputTable = GTUtilsWorkflowDesigner::getInputPortsTable(os, 0);
        row = inputTable->rowCount();
        CHECK_SET_ERR(row == 2,
                      QString("Unexpected \"Input data\" row count, expected: 2, current: %1")
                              .arg(row));
    }

    {//5. Check "Trimming steps"
        //Expected: Trimming steps value is "Configure steps"
        QString trimmingSteps = GTUtilsWorkflowDesigner::getParameter(os, TRIMMING_STEPS);
        CHECK_SET_ERR(trimmingSteps == DEFAULT_TRIMMING_STEPS_VALUE,
                      QString("Unexpected Bowtie2 output file default value, expected: %1, current: %2")
                              .arg(DEFAULT_TRIMMING_STEPS_VALUE)
                              .arg(trimmingSteps));
    }

    {//6. Check "Output file"
        //Expected: Output file value is "Auto"
        QString outputFileDefault = GTUtilsWorkflowDesigner::getParameter(os, OUTPUT_FILE);
        CHECK_SET_ERR(outputFileDefault == DEFAULT_OUTPUT_VALUE,
                      QString("Unexpected Bowtie2 output file default value, expected: %1, current: %2")
                              .arg(DEFAULT_OUTPUT_VALUE)
                              .arg(outputFileDefault));
    }

    {//7. Check "Generate detailde log"
        //Expected: Generate detailde log value is "False"
        QString generateLogDefault = GTUtilsWorkflowDesigner::getParameter(os, GENERATE_DETAILED_LOG);
        CHECK_SET_ERR(generateLogDefault == GENERATE_DETAILED_LOG_VALUES.first(),
                      QString("Unexpected Bowtie2 output file default value, expected: %1, current: %2")
                              .arg(GENERATE_DETAILED_LOG_VALUES.first())
                              .arg(generateLogDefault));

        //8. Set "generate detailed log" value to "True"
        GTUtilsWorkflowDesigner::setParameter(os,
                                              GENERATE_DETAILED_LOG,
                                              GENERATE_DETAILED_LOG_VALUES.last(),
                                              GTUtilsWorkflowDesigner::comboValue);
    }

    {//9. Check "Log file". This parameter had to appear
        //Expected: Output file value is "Auto"
        QString logFileDefault = GTUtilsWorkflowDesigner::getParameter(os, LOG_FILE);
        CHECK_SET_ERR(logFileDefault == DEFAULT_LOG_FILE,
                      QString("Unexpected Bowtie2 output file default value, expected: %1, current: %2")
                              .arg(DEFAULT_LOG_FILE)
                              .arg(logFileDefault));
    }

    {//10. Check "Number of Threads"
        //Expected: expected optimal for the current OS threads num
        int threads = GTUtilsWorkflowDesigner::getParameter(os, NUMBER_OF_THREADS).toInt();
        int expectedThreads = AppContext::getAppSettings()->getAppResourcePool()->getIdealThreadCount();
        CHECK_SET_ERR(threads == expectedThreads,
                      QString("Unexpected threads num, expected: %1, current: %2")
                              .arg(expectedThreads)
                              .arg(threads));
    }



}


}   // namespace GUITest_common_scenarios_trimmomatic
}   // namespace U2
