#ifndef ADDSOURCEFORM_H
#define ADDSOURCEFORM_H

#include <QWidget>

#include "source_grid.h"

namespace Ui {
class addSourceForm;
}

class addSourceForm : public QWidget {
  Q_OBJECT

 public:
  explicit addSourceForm(QWidget *parent = nullptr);
  ~addSourceForm();

  QList<filedata> getFileData();
  bool IsCopySource();

 private:
  Ui::addSourceForm *ui;

  sourceGrid *m_widgetGrid;
};

#endif  // ADDSOURCEFORM_H
