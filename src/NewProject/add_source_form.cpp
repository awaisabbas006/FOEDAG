#include "add_source_form.h"

#include "ui_add_source_form.h"

addSourceForm::addSourceForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addSourceForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Sources"));
  ui->m_labelDetail->setText(
      tr("Specify design files,or directories containing those files,to add to "
         "your project."
         "Create a new source file on disk and add it to your project.You can "
         "also add and create source later."));

  m_widgetgrid = new sourceGrid(GT_SOURCE, ui->m_groupBox);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_groupBox);
  box->addWidget(m_widgetgrid);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_groupBox->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
}

addSourceForm::~addSourceForm() { delete ui; }

QList<filedata> addSourceForm::getfiledata() {
  return m_widgetgrid->getgriddata();
}

bool addSourceForm::iscopysource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}
