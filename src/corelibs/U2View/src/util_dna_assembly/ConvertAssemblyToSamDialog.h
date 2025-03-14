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

#ifndef _U2_CONVERT_ASSEMBLY_TO_SAM_DIALOG_H_
#define _U2_CONVERT_ASSEMBLY_TO_SAM_DIALOG_H_

#include <QDialog>

#include <U2Core/GUrl.h>

class Ui_AssemblyToSamDialog;

namespace U2 {

class SaveDocumentController;

class U2VIEW_EXPORT ConvertAssemblyToSamDialog : public QDialog {
    Q_OBJECT

public:
    ConvertAssemblyToSamDialog(QWidget *parent = NULL, QString dbPath = "");

    const GUrl getDbFileUrl();
    const GUrl getSamFileUrl();

private slots:
    void sl_onSetDbPathButtonClicked();
    void accept();
    void reject();

private:
    void buildSamUrl(const GUrl &dbUrl);
    void initSaveController();

    Ui_AssemblyToSamDialog *ui;
    SaveDocumentController *saveController;

    static GUrl dbFileUrl;
};

}    // namespace U2

#endif    //  _U2_CONVERT_ASSEMBLY_TO_SAM_DIALOG_H_
