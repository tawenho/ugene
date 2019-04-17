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

#include <QDebug>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QWizardPage>

#include <U2Core/AppContext.h>
#include <U2Core/BaseDocumentFormats.h>
#include <U2Core/DocumentModel.h>
#include <U2Core/ExternalToolRegistry.h>
#include <U2Core/ExternalToolRunTask.h>
#include <U2Core/GUrlUtils.h>
#include <U2Core/QObjectScopedPointer.h>
#include <U2Core/ScriptingToolRegistry.h>
#include <U2Core/SignalBlocker.h>
#include <U2Core/U2SafePoints.h>

#include <U2Designer/DelegateEditors.h>

#include <U2Gui/DialogUtils.h>
#include <U2Gui/GroupedComboBoxDelegate.h>
#include <U2Gui/HelpButton.h>
#include <U2Gui/LastUsedDirHelper.h>
#include <U2Gui/U2FileDialog.h>

#include <U2Lang/ActorPrototypeRegistry.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/ConfigurationEditor.h>
#include <U2Lang/ExternalToolCfg.h>
#include <U2Lang/HRSchemaSerializer.h>
#include <U2Lang/WorkflowEnv.h>
#include <U2Lang/WorkflowSettings.h>

#include "CfgExternalToolModel.h"
#include "CreateExternalProcessDialog.h"
#include "WorkflowEditorDelegates.h"
#include "util/WorkerNameValidator.h"

namespace U2 {

class ExecStringValidator : public QValidator {
public:
    ExecStringValidator(QObject *parent = 0)
        : QValidator(parent) {}
    State validate(QString &input, int &) const {
        if (input.contains("\"")) {
            return Invalid;
        }
        return Acceptable;
    }
};

#ifdef Q_OS_MAC
const QString CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET = "QLabel {font-size: 20pt; padding-bottom: 10px; color: #0c3762}";
#else
const QString CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET = "QLabel {font-size: 16pt; padding-bottom: 10px; color: #0c3762}";
#endif

#ifdef Q_OS_MAC
const QString CreateExternalProcessDialog::PAGE_TITLE_STYLE_SHEET = "QLabel {font-size: 20pt; padding-bottom: 10px; color: #0c3762}";
#else
const QString CreateExternalProcessDialog::PAGE_TITLE_STYLE_SHEET = "QLabel {font-size: 16pt; padding-bottom: 10px; color: #0c3762}";
#endif

CreateExternalProcessDialog::CreateExternalProcessDialog(QWidget *_p)
    : QWizard(_p),
      initialCfg(nullptr),
      cfg(nullptr),
      mode(Creating),
      lastPage(false)
{
    init();
}

CreateExternalProcessDialog::CreateExternalProcessDialog(QWidget *p, ExternalProcessConfig *existingCfg, bool lastPage)
    : QWizard(p),
      initialCfg(new ExternalProcessConfig(*existingCfg)),
      cfg(nullptr),
      mode(Editing),
      lastPage(lastPage)
{
    init();
    applyConfig(existingCfg);
}

namespace {

void clearModel(QAbstractItemModel *model) {
    int count = model->rowCount();
    while (count > 0) {
        model->removeRow(0);
        count--;
    }
}

void initPortModel(QAbstractItemModel *model, const QList<DataConfig> &portDataConfigs) {
    int row = 0;
    clearModel(model);
    foreach(const DataConfig &dataCfg, portDataConfigs) {
        model->insertRow(0, QModelIndex());     // TODO: Why 0?

        QModelIndex index = model->index(row, CfgExternalToolModel::COLUMN_NAME);
        model->setData(index, dataCfg.attrName);

        index = model->index(row, CfgExternalToolModel::COLUMN_DATA_TYPE);
        model->setData(index, dataCfg.type);

        index = model->index(row, CfgExternalToolModel::COLUMN_FORMAT);
        model->setData(index, dataCfg.format);

        index = model->index(row, CfgExternalToolModel::COLUMN_DESCRIPTION);
        model->setData(index, dataCfg.description);

        row++;
    }
}

void initAttributeModel(QAbstractItemModel *model, const QList<AttributeConfig> &attributeDataConfigs) {
    int row = 0;
    clearModel(model);
    foreach(const AttributeConfig &attrCfg, attributeDataConfigs) {
        model->insertRow(0, QModelIndex());     // TODO: Why 0?

        QModelIndex index = model->index(row, CfgExternalToolModelAttributes::COLUMN_NAME);
        model->setData(index, attrCfg.attrName);

        index = model->index(row, CfgExternalToolModelAttributes::COLUMN_DATA_TYPE);
        model->setData(index, attrCfg.type);

        index = model->index(row, CfgExternalToolModelAttributes::COLUMN_DESCRIPTION);
        model->setData(index, attrCfg.description);

        row++;
    }
}

}

void CreateExternalProcessDialog::applyConfig(ExternalProcessConfig *existingConfig) {
    initPortModel(ui.inputTableView->model(), existingConfig->inputs);
    initPortModel(ui.outputTableView->model(), existingConfig->outputs);
    initAttributeModel(ui.attributesTableView->model(), existingConfig->attrs);

    ui.nameLineEdit->setText(existingConfig->name);
    ui.descriptionTextEdit->setText(existingConfig->description);
    ui.pteTemplate->setPlainText(existingConfig->cmdLine);
    ui.prompterTextEdit->setText(existingConfig->templateDescription);
}

void CreateExternalProcessDialog::sl_addInput() {
    ui.inputTableView->model()->insertRow(0, QModelIndex());
    validateDataModel();
}

void CreateExternalProcessDialog::sl_addOutput() {
    ui.outputTableView->model()->insertRow(0, QModelIndex());
    validateDataModel();
}

void CreateExternalProcessDialog::sl_deleteInput() {
    QModelIndex index = ui.inputTableView->currentIndex();
    ui.inputTableView->model()->removeRow(index.row());
    validateDataModel();
}

void CreateExternalProcessDialog::sl_deleteOutput() {
    QModelIndex index = ui.outputTableView->currentIndex();
    ui.outputTableView->model()->removeRow(index.row());
    validateDataModel();
}

void CreateExternalProcessDialog::sl_addAttribute() {
    ui.attributesTableView->model()->insertRow(0, QModelIndex());
    validateAttributeModel();
}

void CreateExternalProcessDialog::sl_deleteAttribute() {
    QModelIndex index = ui.attributesTableView->currentIndex();
    ui.attributesTableView->model()->removeRow(index.row());
    validateAttributeModel();
}

CreateExternalProcessDialog::~CreateExternalProcessDialog() {
    delete initialCfg;
    delete cfg;
}

ExternalProcessConfig *CreateExternalProcessDialog::takeConfig() {
    ExternalProcessConfig *result = nullptr;
    qSwap(result, cfg);
    return result;
}

void CreateExternalProcessDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (lastPage) {
        for (int i= 0; i < (pageIds().size() - 1); i++) {
            next();
        }
    }
}

QString removeEmptyLines(const QString &str) {
    QStringList res;
    foreach(const QString &s, str.split(QRegExp("(\n|\r)"))) {
        if (!s.trimmed().isEmpty()) {
            res.append(s);
        }
    }
    return res.join("\r\n");
}

void CreateExternalProcessDialog::accept() {
    CfgExternalToolModel *model;
    cfg = new ExternalProcessConfig();
    cfg->name = ui.nameLineEdit->text();
    cfg->description = removeEmptyLines(ui.descriptionTextEdit->toPlainText());
    cfg->templateDescription = removeEmptyLines(ui.prompterTextEdit->toPlainText());

    model = qobject_cast<CfgExternalToolModel *>(ui.inputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        cfg->inputs << item->itemData;
    }

    model = qobject_cast<CfgExternalToolModel *>(ui.outputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        cfg->outputs << item->itemData;
    }

    CfgExternalToolModelAttributes *aModel = qobject_cast<CfgExternalToolModelAttributes*>(ui.attributesTableView->model());
    foreach(AttributeItem *item, aModel->getItems()) {
        AttributeConfig attributeData;
        attributeData.attrName = item->getName();
        attributeData.type = item->getDataType();
        attributeData.description = item->getDescription();
        cfg->attrs << attributeData;
    }

    cfg->cmdLine = ui.pteTemplate->toPlainText();

    if (!validate()) {
        return;
    }

    if (nullptr != initialCfg) {
        if (!(*initialCfg == *cfg)) {
            int res = QMessageBox::question(this, tr("Warning"),
                tr("You have changed the structure of the element (name, slots, attributes' names and types). "
                "All elements on the scene would be removed. Do you really want to change it?\n"
                "You could also reset the dialog to the initial state."),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Reset, QMessageBox::No);
            if (QMessageBox::No == res) {
                return;
            } else if (QMessageBox::Reset == res) {
                applyConfig(initialCfg);
                return;
            }
        }
    }

    QString str = HRSchemaSerializer::actor2String(cfg);
    QString dir = WorkflowSettings::getExternalToolDirectory();
    QDir d(dir);
    if (!d.exists()) {
        d.mkdir(dir);
    }
    cfg->filePath = dir + cfg->name + ".etc";
    QFile file(cfg->filePath);
    file.open(QIODevice::WriteOnly);
    file.write(str.toLatin1());
    file.close();

    done(QDialog::Accepted);
}

bool CreateExternalProcessDialog::validate() {
    QString title = tr("Create Element");
    if (cfg->inputs.isEmpty() && cfg->outputs.isEmpty())  {
        QMessageBox::critical(this, title, tr("Please set the input/output data."));
        return false;
    }

    if (cfg->cmdLine.isEmpty()) {
        QMessageBox::critical(this, title, tr("Please set the command line to run external tool."));
        return false;
    }

    if (cfg->name.isEmpty()) {
        QMessageBox::critical(this, title, tr("Please set the name for the new element."));
        return false;
    }

    QRegExp invalidSymbols("[\\.,:;\\?]");
    if (cfg->name.contains(invalidSymbols)) {
        QMessageBox::critical(this, title, tr("Invalid symbols in the element name."));
        return false;
    }

    if (WorkflowEnv::getProtoRegistry()->getProto(cfg->name) && Creating == mode) {
        QMessageBox::critical(this, title, tr("Element with this name already exists."));
        return false;
    }

    invalidSymbols = QRegExp("\\W");
    QStringList nameList;
    foreach(const DataConfig & dc, cfg->inputs) {
        if (dc.attrName.isEmpty()) {
            QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            return false;
        }
        if (dc.attrName.contains(invalidSymbols)) {
            QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            return false;
        }
        nameList << dc.attrName;
    }
    foreach(const DataConfig & dc, cfg->outputs) {
        if (dc.attrName.isEmpty()) {
            QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            return false;
        }
        if (dc.attrName.contains(invalidSymbols)) {
            QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            return false;
        }
        nameList << dc.attrName;
    }
    foreach(const AttributeConfig & ac, cfg->attrs) {
        if (ac.attrName.isEmpty()) {
            QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            return false;
        }
        if (ac.attrName.contains(invalidSymbols)) {
            QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(ac.attrName));
            return false;
        }
        nameList << ac.attrName;
    }

    if (nameList.removeDuplicates() > 0) {
        QMessageBox::critical(this, title, tr("The same name of element parameters was found"));
        return false;
    }

    foreach(const QString &str, nameList) {
        if (!cfg->cmdLine.contains("$" + str)) {
            QObjectScopedPointer<QMessageBox> msgBox = new QMessageBox(this);
            msgBox->setWindowTitle(title);
            msgBox->setText(tr("You don't use parameter %1 in template string. Continue?").arg(str));
            msgBox->addButton(tr("Continue"), QMessageBox::ActionRole);
            QPushButton *cancel = msgBox->addButton(tr("Abort"), QMessageBox::ActionRole);
            msgBox->exec();
            CHECK(!msgBox.isNull(), false);
            if (msgBox->clickedButton() == cancel) {
                return false;
            }
        }
    }

    return true;
}

void CreateExternalProcessDialog::sl_generateTemplateString() {
    QString cmd = "<My tool>";

    CfgExternalToolModel *model = qobject_cast<CfgExternalToolModel*>(ui.inputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        cmd += " $" + item->getName();
    }

    model = qobject_cast<CfgExternalToolModel*>(ui.outputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        cmd += " $" + item->getName();
    }

    CfgExternalToolModelAttributes *aModel = qobject_cast<CfgExternalToolModelAttributes*>(ui.attributesTableView->model());
    int i = 0;
    foreach(AttributeItem *item, aModel->getItems()) {
        i++;
        cmd +=  " -p" + QString::number(i) + " $" + item->getName();
    }

    ui.pteTemplate->setPlainText(cmd);
}

bool CreateExternalProcessDialog::validateProcessName(const QString &name, QString &error) {
    if (name.isEmpty()) {
        error = tr("Please set the name for the new element.");
        return false;
    }

    // TODO: it is not good. Something should be done to allow spaces in names.
    QRegExp spaces("\\s");
    if (name.contains(spaces)) {
        error = tr("Spaces in the element name.");
        return false;
    }

    QRegExp invalidSymbols("\\W");
    if (name.contains(invalidSymbols)) {
        error = tr("Invalid symbols in the element name.");
        return false;
    }

    if (WorkflowEnv::getProtoRegistry()->getProto(name) && Creating == mode) {
        error = tr("Element with this name already exists.");
        return false;
    }

    return true;
}

void CreateExternalProcessDialog::sl_validateName( const QString &text) {
    QString error;
    bool res = validateProcessName(text, error);

    button(QWizard::NextButton)->setEnabled(res);
}

void CreateExternalProcessDialog::sl_validateCmdLine() {
    button(QWizard::FinishButton)->setEnabled(!ui.pteTemplate->toPlainText().isEmpty());
}

void CreateExternalProcessDialog::validateDataModel(const QModelIndex &, const QModelIndex & ) {
    bool res = true;
    CfgExternalToolModel *model;

    QRegExp invalidSymbols("\\W");
    QStringList nameList;
    model = qobject_cast<CfgExternalToolModel*>(ui.inputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        if (item->getName().isEmpty()) {
            //QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            res = false;
        }
        if (item->getName().contains(invalidSymbols)) {
            //QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            res = false;
        }
        nameList << item->getName();
    }

    model = qobject_cast<CfgExternalToolModel*>(ui.outputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        if (item->getName().isEmpty()) {
            //QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            res = false;
        }
        if (item->getName().contains(invalidSymbols)) {
            //QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            res = false;
        }
        nameList << item->getName();
    }



    if (nameList.removeDuplicates() > 0) {
        //QMessageBox::critical(this, title, tr("The same name of element parameters was found"));
        res = false;
    }

    if (nameList.isEmpty()) {
        res = false;
    }
    button(QWizard::NextButton)->setEnabled(res);
}

void CreateExternalProcessDialog::validateAttributeModel(const QModelIndex &, const QModelIndex & ) {
    bool res = true;
    CfgExternalToolModel *model;

    QRegExp invalidSymbols("\\W");
    QStringList nameList;
    model = qobject_cast<CfgExternalToolModel*>(ui.inputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        if (item->getName().isEmpty()) {
            //QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            res = false;
        }
        if (item->getName().contains(invalidSymbols)) {
            //QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            res = false;
        }
        nameList << item->getName();
    }

    model = qobject_cast<CfgExternalToolModel*>(ui.outputTableView->model());
    foreach(CfgExternalToolItem *item, model->getItems()) {
        if (item->getName().isEmpty()) {
            //QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            res = false;
        }
        if (item->getName().contains(invalidSymbols)) {
            //QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(dc.attrName));
            res = false;
        }
        nameList << item->getName();
    }

    CfgExternalToolModelAttributes *aModel = qobject_cast<CfgExternalToolModelAttributes*>(ui.attributesTableView->model());
    foreach(AttributeItem *item, aModel->getItems()) {
        if (item->getName().isEmpty()) {
            //QMessageBox::critical(this, title, tr("For one or more parameter name was not set."));
            res = false;
        }
        if (item->getName().contains(invalidSymbols)) {
            //QMessageBox::critical(this, title, tr("Invalid symbols in a name.").arg(ac.attrName));
            res = false;
        }
        nameList << item->getName();
    }


    if (nameList.removeDuplicates() > 0) {
        //QMessageBox::critical(this, title, tr("The same name of element parameters was found"));
        res = false;
    }
    button(QWizard::NextButton)->setEnabled(res);
}

void CreateExternalProcessDialog::sl_validatePage(int id) {
    switch(id) {
        case 0:
//            sl_validateName(ui.nameLineEdit->text());
            break;
        case 1:
            validateDataModel();
            break;
        case 2:
            validateAttributeModel();
            break;      // should be there a break? I've added it because of warning, but maybe there should be a fallthrough
        case 3:
            sl_validateCmdLine();
            break;
    }
}

void CreateExternalProcessDialog::init() {
    ui.setupUi(this);

    new U2::HelpButton(this, button(QWizard::HelpButton), "2097199");

    button(QWizard::NextButton)->installEventFilter(this);

    connect(ui.addInputButton, SIGNAL(clicked()), SLOT(sl_addInput()));
    connect(ui.addOutputButton, SIGNAL(clicked()), SLOT(sl_addOutput()));
    connect(ui.deleteInputButton, SIGNAL(clicked()), SLOT(sl_deleteInput()));
    connect(ui.deleteOutputButton, SIGNAL(clicked()), SLOT(sl_deleteOutput()));
    connect(ui.addAttributeButton, SIGNAL(clicked()), SLOT(sl_addAttribute()));
    connect(ui.deleteAttributeButton, SIGNAL(clicked()), SLOT(sl_deleteAttribute()));
    connect(button(QWizard::NextButton), SIGNAL(clicked()), SLOT(sl_generateTemplateString())); //
    connect(this, SIGNAL(currentIdChanged(int)), SLOT(sl_validatePage(int)));

//    connect(ui.nameLineEdit, SIGNAL(textChanged(const QString &)), SLOT(sl_validateName(const QString &)));
    connect(ui.pteTemplate, SIGNAL(textChanged()), SLOT(sl_validateCmdLine()));

    ui.inputTableView->setModel(new CfgExternalToolModel(CfgExternalToolModel::Input));
    ui.outputTableView->setModel(new CfgExternalToolModel(CfgExternalToolModel::Output));
    ui.attributesTableView->setModel(new CfgExternalToolModelAttributes());

    connect(ui.inputTableView->model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(validateDataModel(const QModelIndex &, const QModelIndex &)));
    connect(ui.outputTableView->model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(validateDataModel(const QModelIndex &, const QModelIndex &)));
    connect(ui.attributesTableView->model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(validateAttributeModel(const QModelIndex &, const QModelIndex &)));

    ui.inputTableView->setItemDelegate(new ProxyDelegate());
    ui.outputTableView->setItemDelegate(new ProxyDelegate());
    ui.attributesTableView->setItemDelegate(new ProxyDelegate());

    ui.inputTableView->horizontalHeader()->setStretchLastSection(true);
    ui.outputTableView->horizontalHeader()->setStretchLastSection(true);
    ui.attributesTableView->horizontalHeader()->setStretchLastSection(true);

    ui.inputTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui.outputTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui.attributesTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    QFontMetrics fm(ui.inputTableView->font());
    const int columnWidth = static_cast<int>(fm.width(SEQ_WITH_ANNS) * 1.5);
    ui.inputTableView->setColumnWidth(1, columnWidth);
    ui.outputTableView->setColumnWidth(1, columnWidth);

    // TODO: validate template somehow
//    ui.templateLineEdit->setValidator(new ExecStringValidator(this));
    CmdlineElementNameValidator *nameValidator = new CmdlineElementNameValidator(ui.nameLineEdit, QString());
    ui.nameLineEdit->setValidator(nameValidator);
    connect(nameValidator, SIGNAL(si_nameValidationComplete(bool)), button(QWizard::NextButton), SLOT(setEnabled(bool)));

//    button(QWizard::NextButton)->setEnabled(!ui.nameLineEdit->text().isEmpty());

    ui.elementNamePageTitle->setStyleSheet(PAGE_TITLE_STYLE_SHEET);
    ui.inputPageTitle->setStyleSheet(PAGE_TITLE_STYLE_SHEET);
    ui.outputPageTitle->setStyleSheet(PAGE_TITLE_STYLE_SHEET);
    ui.parametersPageTitle->setStyleSheet(PAGE_TITLE_STYLE_SHEET);
    ui.templatePageTitle->setStyleSheet(PAGE_TITLE_STYLE_SHEET);

    DialogUtils::setWizardMinimumSize(this);
}

CmdlineElementNameValidator::CmdlineElementNameValidator(QLineEdit *_lineEdit, const QString &_allowedName)
    : QValidator(_lineEdit),
      lineEdit(_lineEdit),
      allowedName(_allowedName)
{
    SAFE_POINT(nullptr != lineEdit, "lineEdit is nullptr", );
}

QValidator::State CmdlineElementNameValidator::validate(QString &name, int &pos) const {
    // TODO: maybe rewrite WorkerNameValidator to do the same?
    static const QRegularExpression invalidSymbols(WorkerNameValidator::INVALID_SYMBOLS_REGEXP);
    int invalidSymbolsCount = 0;
    QRegularExpressionMatchIterator iterator = invalidSymbols.globalMatch(name);
    for (; iterator.hasNext(); ++invalidSymbolsCount, iterator.next());
    if (0 < invalidSymbolsCount) {
        name.remove(invalidSymbols);
        pos -= invalidSymbolsCount;
    }

    // TODO: it is not good. Something should be done to allow spaces in names.
    static const QRegularExpression spaces("\\s");
    int spacesCount = 0;
    iterator = spaces.globalMatch(name);
    for (; iterator.hasNext(); ++spacesCount, iterator.next());
    if (0 < spacesCount) {
        name.remove(spaces);
        pos -= spacesCount;
    }

    State result = Acceptable;
    if (name.isEmpty()) {
        result = Intermediate;
    }

    // TODO: maybe roll the name in case of duplicating?
    if (Acceptable == result && !allowedName.isEmpty() &&
                                allowedName != name &&
                                nullptr != WorkflowEnv::getProtoRegistry()->getProto(name)) {
        result = Intermediate;
    }

    emit si_nameValidationComplete(Acceptable == result);       // TODO: to remove
    return result;
}

const QString CreateExternalProcessNamePage::WORKER_NAME_FIELD = "worker-name";

CreateExternalProcessNamePage::CreateExternalProcessNamePage()
    : QWizardPage(nullptr)
{
    setupUi(this);

    lblTitle->setStyleSheet(CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET);

    leName->setValidator(new CmdlineElementNameValidator(leName, QString()));
    registerField(WORKER_NAME_FIELD + "*", leName);
}

const QString CreateExternalProcessBinaryPage::BINARY_PATH_FIELD = "binary-path";
const QString CreateExternalProcessBinaryPage::USE_CUSTOM_TOOL_FIELD = "use-custom-tool";
const QString CreateExternalProcessBinaryPage::EMBEDDED_TOOL_ID_FIELD = "embedded-tool-id";
const QString CreateExternalProcessBinaryPage::LAUNCHER_ID_FIELD = "launcher-id";
const QString CreateExternalProcessBinaryPage::LAST_USED_DIR_DOMAIN = "CreateExternalProcessWizard";

CreateExternalProcessBinaryPage::CreateExternalProcessBinaryPage()
    : QWizardPage(nullptr),
      userSelectedLauncher("")
{
    setupUi(this);

    connect(tbBrowse, SIGNAL(clicked()), SLOT(sl_browse()));
    connect(rbCustom, SIGNAL(clicked()), SLOT(sl_toolTypeChanged()));
    connect(rbEmbedded, SIGNAL(clicked()), SLOT(sl_toolTypeChanged()));
    connect(cbEmbedded, SIGNAL(currentIndexChanged(int)), SLOT(sl_embeddedToolChanged(int)));

    initEmbeddedToolsList();
    initLaunchersList();

    sl_toolTypeChanged();
    cbEmbedded->setCurrentIndex(1); // TODO: change

    registerField(BINARY_PATH_FIELD + "*", lePath);
    registerField(USE_CUSTOM_TOOL_FIELD + "*", rbCustom);
    registerField(EMBEDDED_TOOL_ID_FIELD, cbEmbedded, "currentData");
    registerField(LAUNCHER_ID_FIELD, cbLauncher, "currentData");

    rbCustom->setChecked(true);
}

bool CreateExternalProcessBinaryPage::isComplete() const {
    if (rbCustom->isChecked()) {
        CHECK(!lePath->text().isEmpty(), false);
        QString path = lePath->text();
        int unusedPos = 0;
        CHECK(nullptr == lePath->validator() || QValidator::Acceptable == lePath->validator()->validate(path, unusedPos), false);
    }
    return true;
}

bool CreateExternalProcessBinaryPage::validatePage() {
    if (rbCustom->isChecked() && !QFileInfo(lePath->text()).exists()) {
        QObjectScopedPointer<QMessageBox> warning = new QMessageBox(QMessageBox::Question,
                                                                     tr("File doesn't exist"),
                                                                     tr("The file you've specified doesn't exist. "
                                                                        "You wouldn't be able to launch the workflow until you specify the correct file. "
                                                                        "Continue anyway?"),
                                                                     QMessageBox::Yes | QMessageBox::No);
        warning->setDefaultButton(QMessageBox::No);
        int result = warning->exec();
        return QMessageBox::Yes == result;
    }

    return true;
}

void CreateExternalProcessBinaryPage::sl_browse() {
    LastUsedDirHelper lod(LAST_USED_DIR_DOMAIN);
    lod.url = U2FileDialog::getOpenFileName(this, tr("Select a binary"), lod.dir);
    if (lod.url.isEmpty()) {
        return;
    }
    lePath->setText(lod.url);
}

void CreateExternalProcessBinaryPage::sl_toolTypeChanged() {
    const bool isCustom = rbCustom->isChecked();

    pathWidget->setEnabled(isCustom);
    cbLauncher->setEnabled(isCustom);
    cbEmbedded->setEnabled(!isCustom);

    if (isCustom) {
        cbLauncher->setCurrentIndex(cbLauncher->findData(userSelectedLauncher));
    } else {
        userSelectedLauncher = cbLauncher->currentData().toString();
    }
}

void CreateExternalProcessBinaryPage::sl_embeddedToolChanged(int index) {
    const QString toolName = cbEmbedded->itemData(index).toString();
    ExternalToolRegistry *etRegistry = AppContext::getExternalToolRegistry();
    ExternalTool *tool = etRegistry->getByName(toolName);
    SAFE_POINT(nullptr != tool, QString("Tool with name '%1' not found").arg(toolName), );

    const QString launcher = tool->getToolRunnerProgram();
    if (!launcher.isEmpty()) {
        cbLauncher->setCurrentIndex(cbLauncher->findData(launcher));
    } else {
        cbLauncher->setCurrentIndex(cbLauncher->findData(QString("")));
    }
}

namespace {

void removeModules(QList<ExternalTool *> &tools) {
    foreach (ExternalTool *tool, tools) {
        if (tool->isModule()) {
            tools.removeOne(tool);
        }
    }
}

bool lessThan(ExternalTool *tool1, ExternalTool *tool2) {
    return tool1->getName() < tool2->getName();
}

}

void CreateExternalProcessBinaryPage::initEmbeddedToolsList() {
    SignalBlocker signalBlocker(cbEmbedded);

    GroupedComboBoxDelegate *cbEmbeddedDelegate = new GroupedComboBoxDelegate();
    cbEmbedded->setItemDelegate(cbEmbeddedDelegate);

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(cbEmbedded->model());
    SAFE_POINT(nullptr != model, "Can't cast combobox model to a QStandardItemModel", );

    ExternalToolRegistry *etRegistry = AppContext::getExternalToolRegistry();
    QList<QList<ExternalTool *> > groupedTools = etRegistry->getAllEntriesSortedByToolKits();
    QMap<QString, QList<ExternalTool *> > sortedToolkits;
    QList<ExternalTool *> sortedStandaloneTools;

    for (int i = 0; i < groupedTools.size(); ++i) {
        removeModules(groupedTools[i]);
        if (1 < groupedTools[i].size()) {
            sortedToolkits.insert(groupedTools[i].first()->getToolKitName(), groupedTools[i]);
        } else {
            sortedStandaloneTools << groupedTools[i];
        }
    }
    std::sort(sortedStandaloneTools.begin(), sortedStandaloneTools.end(), lessThan);

    foreach (const QString &toolkit, sortedToolkits.keys()) {
        cbEmbeddedDelegate->addParentItem(model, toolkit);
        foreach (ExternalTool *tool, sortedToolkits[toolkit]) {
            cbEmbeddedDelegate->addChildItem(model, tool->getName(), tool->getName());
        }
    }

    foreach (ExternalTool *tool, sortedStandaloneTools) {
        cbEmbedded->addItem(tool->getName(), tool->getName());
    }
}

void CreateExternalProcessBinaryPage::initLaunchersList() {
    cbLauncher->addItem(tr("No launcher"), QString(""));
    ExternalToolRegistry *etRegistry = AppContext::getExternalToolRegistry();
    foreach (ExternalTool *tool, etRegistry->getRunners()) {
        cbLauncher->addItem(tool->getName(), tool->getName());
    }
}

const QString CreateExternalProcessInputPage::INPUTS_NAMES_FIELD = "inputs-names";
char const * const CreateExternalProcessInputPage::NAMES_PROPERTY = "inputs-names-property";

CreateExternalProcessInputPage::CreateExternalProcessInputPage()
    : QWizardPage(nullptr)
{
    setupUi(this);

    lblTitle->setStyleSheet(CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET);

    connect(pbAdd, SIGNAL(clicked()), SLOT(sl_addInput()));
    connect(pbDelete, SIGNAL(clicked()), SLOT(sl_deleteInput()));
    connect(this, SIGNAL(si_namesChanged()), SIGNAL(completeChanged()));

    model = new CfgExternalToolModel(CfgExternalToolModel::Input);
    connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(sl_saveNames()));

    tvInput->setModel(model);
    tvInput->setItemDelegate(new ProxyDelegate());
    tvInput->horizontalHeader()->setStretchLastSection(true);
    tvInput->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    // TODO: do something with the metrics
    QFontMetrics fm(tvInput->font());
    const int columnWidth = static_cast<int>(fm.width(SEQ_WITH_ANNS) * 1.5);
    tvInput->setColumnWidth(1, columnWidth);

    registerField(INPUTS_NAMES_FIELD, this, NAMES_PROPERTY, SIGNAL(si_namesChanged()));
}

bool CreateExternalProcessInputPage::isComplete() const {
    bool res = true;
    QMap<QString, QList<CfgExternalToolItem *> > itemNames;

    foreach (CfgExternalToolItem *item, model->getItems()) {
        item->resetErrors();
        if (item->getName().isEmpty()) {
            item->addError(tr("The input name is empty"));
            res = false;
        } else {
            itemNames[item->getName()] << item;
        }
    }

    const QStringList outputsNames = wizard()->field(CreateExternalProcessOutputPage::OUTPUTS_NAMES_FIELD).toStringList();

    foreach (const QString &name, itemNames.keys()) {
        if (1 < itemNames[name].size()) {
            foreach (CfgExternalToolItem *item, itemNames[name]) {
                item->addError(tr("The input name is not unique, it has duplicates among other inputs"));
                res = false;
            }
        }

        if (outputsNames.contains(name)) {
            foreach (CfgExternalToolItem *item, itemNames[name]) {
                item->addError(tr("The input name is not unique, it has duplicates among outputs"));
                // Do not disable the "next" button, user should be able to see outputs page
            }
        }
    }

    return res;
}

bool CreateExternalProcessInputPage::validatePage() {
    if (model->getItems().isEmpty()) {
        QObjectScopedPointer<QMessageBox> question = new QMessageBox(QMessageBox::Question,
                                                                     tr("No inputs"),
                                                                     tr("You didn't specify any inputs. "
                                                                        "So the workflow element won't be able to get data from other elements in a workflow. "
                                                                        "Is it correct?"),
                                                                     QMessageBox::Yes | QMessageBox::No);
        question->setDefaultButton(QMessageBox::Yes);
        int result = question->exec();
        return QMessageBox::Yes == result;
    }
    return true;
}

void CreateExternalProcessInputPage::sl_addInput() {
    const int ignored = 0;
    model->insertRow(ignored, QModelIndex());
    tvInput->setCurrentIndex(model->index(model->rowCount() - 1, 0));
}

void CreateExternalProcessInputPage::sl_deleteInput() {
    model->removeRow(tvInput->currentIndex().row());
}

void CreateExternalProcessInputPage::sl_saveNames() {
    QStringList names;
    foreach (CfgExternalToolItem *item, model->getItems()) {
        names << item->getName();
    }
    setProperty(NAMES_PROPERTY, names);
    emit si_namesChanged();
}

const QString CreateExternalProcessOutputPage::OUTPUTS_NAMES_FIELD = "outputs-names";
char const * const CreateExternalProcessOutputPage::NAMES_PROPERTY = "outputs-names-property";

CreateExternalProcessOutputPage::CreateExternalProcessOutputPage()
    : QWizardPage(nullptr)
{
    setupUi(this);

    lblTitle->setStyleSheet(CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET);

    connect(pbAdd, SIGNAL(clicked()), SLOT(sl_addOutput()));
    connect(pbDelete, SIGNAL(clicked()), SLOT(sl_deleteOutput()));
    connect(this, SIGNAL(si_namesChanged()), SIGNAL(completeChanged()));

    model = new CfgExternalToolModel(CfgExternalToolModel::Output);
    connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(sl_saveNames()));

    tvOutput->setModel(model);
    tvOutput->setItemDelegate(new ProxyDelegate());
    tvOutput->horizontalHeader()->setStretchLastSection(true);
    tvOutput->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    // TODO: do something with the metrics
    QFontMetrics fm(tvOutput->font());
    const int columnWidth = static_cast<int>(fm.width(SEQ_WITH_ANNS) * 1.5);
    tvOutput->setColumnWidth(1, columnWidth);

    registerField(OUTPUTS_NAMES_FIELD, this, NAMES_PROPERTY, SIGNAL(si_namesChanged()));
}

bool CreateExternalProcessOutputPage::isComplete() const {
    bool res = true;
    QMap<QString, QList<CfgExternalToolItem *> > itemNames;

    foreach (CfgExternalToolItem *item, model->getItems()) {
        item->resetErrors();
        if (item->getName().isEmpty()) {
            item->addError(tr("The output name is empty"));
            res = false;
        } else {
            itemNames[item->getName()] << item;
        }
    }

    const QStringList inputsNames = wizard()->field(CreateExternalProcessInputPage::INPUTS_NAMES_FIELD).toStringList();

    foreach (const QString &name, itemNames.keys()) {
        if (1 < itemNames[name].size()) {
            foreach (CfgExternalToolItem *item, itemNames[name]) {
                item->addError(tr("The output name is not unique, it has duplicates among other outputs"));
                res = false;
            }
        }

        if (inputsNames.contains(name)) {
            foreach (CfgExternalToolItem *item, itemNames[name]) {
                item->addError(tr("The output name is not unique, it has duplicates among inputs"));
                res = false;
            }
        }
    }

    const QStringList outputsNames = wizard()->field(CreateExternalProcessOutputPage::OUTPUTS_NAMES_FIELD).toStringList();

    return res;
}

bool CreateExternalProcessOutputPage::validatePage() {
    if (model->getItems().isEmpty()) {
        QObjectScopedPointer<QMessageBox> question = new QMessageBox(QMessageBox::Question,
                                                                     tr("No outputs"),
                                                                     tr("You didn't specify any outputs. "
                                                                        "So the workflow element won't be able to pass data to other elements in a workflow. "
                                                                        "Is it correct?"),
                                                                     QMessageBox::Yes | QMessageBox::No);
        question->setDefaultButton(QMessageBox::Yes);
        int result = question->exec();
        return QMessageBox::Yes == result;
    }
    return true;
}

void CreateExternalProcessOutputPage::sl_addOutput() {
    const int ignored = 0;
    model->insertRow(ignored, QModelIndex());
    tvOutput->setCurrentIndex(model->index(model->rowCount() - 1, 0));
}

void CreateExternalProcessOutputPage::sl_deleteOutput() {
    model->removeRow(tvOutput->currentIndex().row());
}

void CreateExternalProcessOutputPage::sl_saveNames() {
    QStringList names;
    foreach (CfgExternalToolItem *item, model->getItems()) {
        names << item->getName();
    }
    setProperty(NAMES_PROPERTY, names);
    emit si_namesChanged();
}

const QString CreateExternalProcessAttributesPage::ATTRIBUTES_NAMES_FIELD = "attributes-names";
char const * const CreateExternalProcessAttributesPage::NAMES_PROPERTY = "attributes-names-property";

CreateExternalProcessAttributesPage::CreateExternalProcessAttributesPage()
    : QWizardPage(nullptr)
{
    setupUi(this);

    lblTitle->setStyleSheet(CreateExternalProcessWizard::PAGE_TITLE_STYLE_SHEET);

    connect(pbAdd, SIGNAL(clicked()), SLOT(sl_addAttribute()));
    connect(pbDelete, SIGNAL(clicked()), SLOT(sl_deleteAttribute()));
    connect(this, SIGNAL(si_namesChanged()), SIGNAL(completeChanged()));

    model = new CfgExternalToolModelAttributes();
    connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(sl_saveNames()));
    connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(sl_saveNames()));

    tvAttributes->setModel(model);
    tvAttributes->setItemDelegate(new ProxyDelegate());
    tvAttributes->horizontalHeader()->setStretchLastSection(true);
    tvAttributes->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    registerField(ATTRIBUTES_NAMES_FIELD, this, NAMES_PROPERTY, SIGNAL(si_namesChanged()));
}

bool CreateExternalProcessAttributesPage::isComplete() const {
    bool res = true;
    QMap<QString, QList<AttributeItem *> > itemNames;

    foreach (AttributeItem *item, model->getItems()) {
        item->resetErrors();
        if (item->getName().isEmpty()) {
            item->addError(tr("The attribute name is empty"));
            res = false;
        } else {
            itemNames[item->getName()] << item;
        }
    }

    foreach (const QString &name, itemNames.keys()) {
        if (1 < itemNames[name].size()) {
            foreach (AttributeItem *item, itemNames[name]) {
                item->addError(tr("The attribute name is not unique, it has duplicates among other attributes"));
                res = false;
            }
        }
    }

    return res;
}

bool CreateExternalProcessAttributesPage::validatePage() {
    if (model->getItems().isEmpty()) {
        QObjectScopedPointer<QMessageBox> question = new QMessageBox(QMessageBox::Question,
                                                                     tr("No attributes"),
                                                                     tr("You didn't specify any attributes. "
                                                                        "Is it correct?"),
                                                                     QMessageBox::Yes | QMessageBox::No);
        question->setDefaultButton(QMessageBox::Yes);
        int result = question->exec();
        return QMessageBox::Yes == result;
    }
    return true;
}

void CreateExternalProcessAttributesPage::sl_addAttribute() {
    const int ignored = 0;
    model->insertRow(ignored, QModelIndex());
    tvAttributes->setCurrentIndex(model->index(model->rowCount() - 1, 0));
}

void CreateExternalProcessAttributesPage::sl_deleteAttribute() {
    model->removeRow(tvAttributes->currentIndex().row());
}

void CreateExternalProcessAttributesPage::sl_saveNames() {
    QStringList names;
    foreach (AttributeItem *item, model->getItems()) {
        names << item->getName();
    }
    setProperty(NAMES_PROPERTY, names);
    emit si_namesChanged();
}

const QString CommandTemplateValidator::UGENE_VARIABLE_COLOR = "red";
const QString CommandTemplateValidator::WORKER_PARAMETER_COLOR = "blue";

CommandTemplateValidator::CommandTemplateValidator(QTextEdit *_textEdit)
    : QObject(_textEdit),
      textEdit(_textEdit)
{
    SAFE_POINT(nullptr != textEdit, "textEdit widget is nullptr", );
    connect(textEdit, SIGNAL(textChanged()), SLOT(sl_textChanged()));
}

void CommandTemplateValidator::sl_textChanged() {
    SignalBlocker signalBlocker(textEdit);
    Q_UNUSED(signalBlocker);

    static const QRegularExpression ugeneVariablePlaceholder("(?<=^|\\s)(\\$\\$\\w+)(?=$|\\s)");
    static const QRegularExpression workerParameterPlaceholder("(?<=^|\\s)(\\$\\w+)(?=$|\\s)");

    QTextCursor cursor = textEdit->textCursor();
    const int position = cursor.position();

    QString text = textEdit->toPlainText();
    text.replace(ugeneVariablePlaceholder, QString("<span style=\"color:%1;\">\\1</span>").arg(UGENE_VARIABLE_COLOR));
    text.replace(workerParameterPlaceholder, QString("<span style=\"color:%1;\">\\1</span>").arg(WORKER_PARAMETER_COLOR));
    textEdit->setHtml("<span>" + text + "</span>");

    cursor.setPosition(position);
    textEdit->setTextCursor(cursor);
}

const QString CreateExternalProcessCommandTemplatePage::COMMAND_TEMPLATE_FIELD = "command-template";
const QString CreateExternalProcessCommandTemplatePage::PLACEHOLDER_MARKER = "$";

CreateExternalProcessCommandTemplatePage::CreateExternalProcessCommandTemplatePage()
    : QWizardPage(nullptr)
{
    setupUi(this);

    teTemplate->setWordWrapMode(QTextOption::WrapAnywhere);
    teTemplate->document()->setDefaultStyleSheet("span { white-space: pre-wrap; }");

    new CommandTemplateValidator(teTemplate);

    registerField(COMMAND_TEMPLATE_FIELD + "*", teTemplate, "plainText", SIGNAL(textChanged()));
}

void CreateExternalProcessCommandTemplatePage::initializePage() {
    QString templateString;

    const bool useCustomTool = wizard()->field(CreateExternalProcessBinaryPage::USE_CUSTOM_TOOL_FIELD).toBool();
    const QString customPath = wizard()->field(CreateExternalProcessBinaryPage::BINARY_PATH_FIELD).toString();
    const QString embeddedToolName = wizard()->field(CreateExternalProcessBinaryPage::EMBEDDED_TOOL_ID_FIELD).toString();
    const QString launcherId = wizard()->field(CreateExternalProcessBinaryPage::LAUNCHER_ID_FIELD).toString();

    if (!launcherId.isEmpty()) {
        ScriptingTool *launcherTool = AppContext::getScriptingToolRegistry()->getByName(launcherId);
        SAFE_POINT(nullptr != launcherTool, QString("Launcher with name '%1' not found in the registry").arg(launcherId), );

        templateString += getToolPlaceholder(launcherId);

        if (!useCustomTool) {
            ExternalTool *embeddedTool = AppContext::getExternalToolRegistry()->getByName(embeddedToolName);
            SAFE_POINT(nullptr != embeddedTool, QString("Tool with name '%1' not found in the registry").arg(embeddedToolName), );

            const QStringList additionalToolParameters = embeddedTool->getToolRunnerAdditionalOptions();    // it is tool's specific parameters, e.g. snpEff has additinal arguments for java
            if (!additionalToolParameters.isEmpty()) {
                templateString += " " + ExternalToolSupportUtils::prepareArgumentsForCmdLine(additionalToolParameters);
            }
        }

        templateString += " " + launcherTool->getRunParameters().join(" ");
        templateString += " ";
    }

    if (useCustomTool) {
        templateString += GUrlUtils::getQuotedString(customPath);
    } else {
        templateString += getToolPlaceholder(embeddedToolName);
    }

    const QStringList inputsNames = wizard()->field(CreateExternalProcessInputPage::INPUTS_NAMES_FIELD).toStringList();
    foreach (const QString &inputName, inputsNames) {
        templateString += " " + getParameterPlaceholder(inputName);
    }

    const QStringList outputsNames = wizard()->field(CreateExternalProcessOutputPage::OUTPUTS_NAMES_FIELD).toStringList();
    foreach (const QString &outputName, outputsNames) {
        templateString += " " + getParameterPlaceholder(outputName);
    }

    const QStringList attributesNames = wizard()->field(CreateExternalProcessAttributesPage::ATTRIBUTES_NAMES_FIELD).toStringList();
    foreach (const QString &attributeName, attributesNames) {
        templateString += " -p " + getParameterPlaceholder(attributeName);      // TODO: should we write quotes here for string parameters? Also '-p' looks not very clear.
    }

    teTemplate->setPlainText(templateString);
}

bool CreateExternalProcessCommandTemplatePage::isComplete() const {
    return !teTemplate->toPlainText().isEmpty();
}

bool CreateExternalProcessCommandTemplatePage::validatePage() {
    // TODO: implement
    // Check that all parameters are used
    // Check that it is parseable
    // Check that symbols that are supposed to be escaped are escaped
    return true;
}

QString CreateExternalProcessCommandTemplatePage::getToolPlaceholder(const QString &toolName) {
    return PLACEHOLDER_MARKER + PLACEHOLDER_MARKER + toolName.toUpper() + "_PATH";
}

QString CreateExternalProcessCommandTemplatePage::getParameterPlaceholder(const QString &parameterName) {
    return PLACEHOLDER_MARKER + parameterName.toUpper();
}

CreateExternalProcessWizard::CreateExternalProcessWizard(QWidget *_parentWidget)
    : QWizard(_parentWidget)
{
    addPage(new CreateExternalProcessNamePage());
    addPage(new CreateExternalProcessBinaryPage());
    addPage(new CreateExternalProcessInputPage());
    addPage(new CreateExternalProcessOutputPage());
    addPage(new CreateExternalProcessAttributesPage());
    addPage(new CreateExternalProcessCommandTemplatePage());

    setWizardStyle(ClassicStyle);
    setOption(IndependentPages);
//    layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);
    DialogUtils::setWizardMinimumSize(this);
}

}   // namespace U2
