#include "new_project_dialog.h"

#include <QDesktopWidget>
#include <QMessageBox>

#include "ui_new_project_dialog.h"
using namespace FOEDAG;

newProjectDialog::newProjectDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::newProjectDialog), m_index(INDEX_LOCATION) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  setWindowTitle(tr("New Project"));

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

  m_locationForm = new locationForm(this);
  ui->m_stackedWidget->insertWidget(1, m_locationForm);
  m_proTypeForm = new projectTypeForm(this);
  ui->m_stackedWidget->insertWidget(2, m_proTypeForm);
  m_addSrcForm = new addSourceForm(this);
  ui->m_stackedWidget->insertWidget(3, m_addSrcForm);
  m_addConstrsForm = new addConstraintsForm(this);
  ui->m_stackedWidget->insertWidget(4, m_addConstrsForm);
  m_devicePlanForm = new devicePlannerForm(this);
  ui->m_stackedWidget->insertWidget(5, m_devicePlanForm);
  m_sumForm = new summaryForm(this);
  ui->m_stackedWidget->insertWidget(6, m_sumForm);
  ui->m_stackedWidget->adjustSize();

  UpdateDialogView();

  m_projectManager = new ProjectManager(this);
}

newProjectDialog::~newProjectDialog() { delete ui; }

void newProjectDialog::tcl_command_test() { on_m_btnNext_clicked(); }

void newProjectDialog::on_m_btnBack_clicked() {
  m_index--;
  UpdateDialogView();
}

void newProjectDialog::on_m_btnNext_clicked() {
  m_index++;
  UpdateDialogView();
}

void newProjectDialog::on_m_btnFinish_clicked() {
  m_projectManager->CreateProject(m_locationForm->getProjectName(),
                                  m_locationForm->getProjectPath());
  m_projectManager->setProjectType(m_proTypeForm->getProjectType());
  m_projectManager->setCurrentFileSet(DEFAULT_FOLDER_SOURCE);
  QList<filedata> listFile = m_addSrcForm->getFileData();
  foreach (filedata fdata, listFile) {
    if ("<Local to Project>" == fdata.m_filePath) {
      m_projectManager->setDesignFile(fdata.m_fileName, false);
    } else {
      m_projectManager->setDesignFile(fdata.m_filePath + "/" + fdata.m_fileName,
                                      m_addSrcForm->IsCopySource());
    }
  }
  m_projectManager->FinishedProject();
  this->close();
}

void newProjectDialog::on_m_btnCancel_clicked() { this->close(); }
void newProjectDialog::UpdateDialogView() {
  if (INDEX_LOCATION == m_index) {
    ui->m_btnBack->setEnabled(false);
  } else {
    ui->m_btnBack->setEnabled(true);
  }

  if (INDEX_SUMMARYF == m_index) {
    ui->m_btnNext->setEnabled(false);
    ui->m_btnFinish->setEnabled(true);
    m_sumForm->setProjectName(m_locationForm->getProjectName(),
                              m_proTypeForm->getProjectType());
    m_sumForm->setDeviceInfo(m_devicePlanForm->getSelectedDevice());
    m_sumForm->setSourceCount(m_addSrcForm->getFileData().count(),
                              m_addConstrsForm->getFileData().count());
  } else {
    ui->m_btnNext->setEnabled(true);
    ui->m_btnFinish->setEnabled(false);
  }

  ui->m_stackedWidget->setCurrentIndex(m_index);
}
