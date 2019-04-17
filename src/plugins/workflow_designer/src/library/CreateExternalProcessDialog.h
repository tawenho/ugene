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

#ifndef _U2_CREATE_EXTERNAL_PROCESS_DIALOG_H_
#define _U2_CREATE_EXTERNAL_PROCESS_DIALOG_H_

#include <U2Lang/Attribute.h>
#include <U2Lang/ConfigurationEditor.h>
#include <U2Lang/Datatype.h>

#include "ui_CreateExternalProcessAttributesPage.h"
#include "ui_CreateExternalProcessBinaryPage.h"
#include "ui_CreateExternalProcessCommandTemplatePage.h"
#include "ui_CreateExternalProcessInputPage.h"
#include "ui_CreateExternalProcessNamePage.h"
#include "ui_CreateExternalProcessOutputPage.h"
#include "ui_ExternalProcessWorkerDialog.h"
#include "util/WorkerNameValidator.h"

namespace U2 {

class CfgExternalToolModel;
class CfgExternalToolModelAttributes;
class ExternalProcessConfig;

class CreateExternalProcessNamePage : public QWizardPage, private Ui_CreateExternalProcessNamePage {
public:
    CreateExternalProcessNamePage();

    static const QString WORKER_NAME_FIELD;
};

class CreateExternalProcessBinaryPage : public QWizardPage, private Ui_CreateExternalProcessBinaryPage {
    Q_OBJECT
public:
    CreateExternalProcessBinaryPage();

    bool isComplete() const override;
    bool validatePage() override;

    static const QString BINARY_PATH_FIELD;
    static const QString USE_CUSTOM_TOOL_FIELD;
    static const QString EMBEDDED_TOOL_ID_FIELD;
    static const QString LAUNCHER_ID_FIELD;

private slots:
    void sl_browse();
    void sl_toolTypeChanged();
    void sl_embeddedToolChanged(int index);

private:
    void initEmbeddedToolsList();
    void initLaunchersList();

    QString userSelectedLauncher;

    static const QString LAST_USED_DIR_DOMAIN;
};

class CreateExternalProcessInputPage : public QWizardPage, private Ui_CreateExternalProcessInputPage {
    Q_OBJECT
public:
    CreateExternalProcessInputPage();

    bool isComplete() const override;
    bool validatePage() override;

    static const QString INPUTS_NAMES_FIELD;

signals:
    void si_namesChanged();

private slots:
    void sl_addInput();
    void sl_deleteInput();
    void sl_saveNames();

private:
    CfgExternalToolModel *model;

    static char const * const NAMES_PROPERTY;
};

class CreateExternalProcessOutputPage : public QWizardPage, private Ui_CreateExternalProcessOutputPage {
    Q_OBJECT
public:
    CreateExternalProcessOutputPage();

    bool isComplete() const override;
    bool validatePage() override;

    static const QString OUTPUTS_NAMES_FIELD;

signals:
    void si_namesChanged();

private slots:
    void sl_addOutput();
    void sl_deleteOutput();
    void sl_saveNames();

private:
    CfgExternalToolModel *model;

    static char const * const NAMES_PROPERTY;
};

class CreateExternalProcessAttributesPage : public QWizardPage, private Ui_CreateExternalProcessAttributesPage {
    Q_OBJECT
public:
    CreateExternalProcessAttributesPage();

    bool isComplete() const override;
    bool validatePage() override;

    static const QString ATTRIBUTES_NAMES_FIELD;

signals:
    void si_namesChanged();

private slots:
    void sl_addAttribute();
    void sl_deleteAttribute();
    void sl_saveNames();

private:
    CfgExternalToolModelAttributes *model;

    static char const * const NAMES_PROPERTY;
};

class CommandTemplateValidator : public QObject {
    Q_OBJECT
public:
    CommandTemplateValidator(QTextEdit *textEdit);

private slots:
    void sl_textChanged();

private:
    QTextEdit *textEdit;

    static const QString UGENE_VARIABLE_COLOR;
    static const QString WORKER_PARAMETER_COLOR;
};

class CreateExternalProcessCommandTemplatePage : public QWizardPage, private Ui_CreateExternalProcessCommandTemplatePage {
    Q_OBJECT
public:
    CreateExternalProcessCommandTemplatePage();

    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

    static const QString COMMAND_TEMPLATE_FIELD;

private:
    static QString getToolPlaceholder(const QString &toolName);
    static QString getParameterPlaceholder(const QString &parameterName);

    static const QString PLACEHOLDER_MARKER;
};

class CreateExternalProcessWizard : public QWizard {
    Q_OBJECT
public:
    CreateExternalProcessWizard(QWidget *parentWidget);

    static const QString PAGE_TITLE_STYLE_SHEET;
};

class CreateExternalProcessDialog: public QWizard {
    Q_OBJECT
public:
    enum DialogMode {
        Creating,
        Editing
    };

    CreateExternalProcessDialog(QWidget *p = nullptr);
    CreateExternalProcessDialog(QWidget *p, ExternalProcessConfig *cfg, bool lastPage);
    ~CreateExternalProcessDialog();

    ExternalProcessConfig *takeConfig();

private slots:
    void accept();

    void sl_addInput();
    void sl_addOutput();
    void sl_deleteInput();
    void sl_deleteOutput();
    void sl_addAttribute();
    void sl_deleteAttribute();
    void sl_generateTemplateString();
    void sl_validateName(const QString &);
    void sl_validateCmdLine();
    void validateDataModel(const QModelIndex & i1 = QModelIndex(), const QModelIndex & i2 = QModelIndex());
    void validateAttributeModel(const QModelIndex & i1 = QModelIndex(), const QModelIndex & i2 = QModelIndex());
    void sl_validatePage(int);

private:
    void init();
    virtual void showEvent(QShowEvent *event);
    bool validate();
    bool validateProcessName(const QString &name, QString &error);
    void applyConfig(ExternalProcessConfig *existingConfig);

    Ui_CreateExternalProcessWorkerDialog ui;
    ExternalProcessConfig *initialCfg;
    ExternalProcessConfig *cfg;
    DialogMode mode;
    bool lastPage;

    static const int INFO_STRINGS_NUM = 5;
    static const QString PAGE_TITLE_STYLE_SHEET;
};

class CmdlineElementNameValidator : public QValidator {
    Q_OBJECT
public:
    CmdlineElementNameValidator(QLineEdit *lineEdit, const QString &allowedName);

    State validate(QString &name, int &pos) const override;

signals:
    void si_nameValidationComplete(bool isValid) const;

private:
    QLineEdit *lineEdit;
    const QString allowedName;
};

}   // namespace U2

#endif // _U2_CREATE_EXTERNAL_PROCESS_DIALOG_H_
