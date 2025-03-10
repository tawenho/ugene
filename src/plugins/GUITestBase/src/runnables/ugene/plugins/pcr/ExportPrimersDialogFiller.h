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

#ifndef _U2_EXPORT_PRIMERS_DIALOG_FILLER_H_
#define _U2_EXPORT_PRIMERS_DIALOG_FILLER_H_

#include "utils/GTUtilsDialog.h"

namespace U2 {
using namespace HI;

class ExportPrimersDialogFiller : public Filler {
public:
    enum ExportTarget { LocalFile,
                        SharedDb };

    ExportPrimersDialogFiller(HI::GUITestOpStatus &os, CustomScenario *scenario = NULL);

    void commonScenario();

    static QWidget *getDialog(HI::GUITestOpStatus &os);
    static void setExportTarget(HI::GUITestOpStatus &os, ExportTarget exportTarget);
    static void setFormat(HI::GUITestOpStatus &os, const QString &format);
    static void setFilePath(HI::GUITestOpStatus &os, const QString &filePath);
    static void setDatabase(HI::GUITestOpStatus &os, const QString &database);
    static void setFolder(HI::GUITestOpStatus &os, const QString &folder);
};

}    // namespace U2

#endif    // _U2_EXPORT_PRIMERS_DIALOG_FILLER_H_
