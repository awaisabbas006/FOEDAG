#ifndef SOURCES_FORM_H
#define SOURCES_FORM_H

#include <QAction>
#include <QTreeWidget>
#include <QWidget>

namespace Ui {
class SourcesForm;
}

class SourcesForm : public QWidget {
  Q_OBJECT

 public:
  explicit SourcesForm(QWidget* parent = nullptr);
  ~SourcesForm();

 private:
  Ui::SourcesForm* ui;

  QTreeWidget* m_treeSrcHierachy;
  QAction* m_actRefresh;
  QAction* m_actAddFileSet;
  QAction* m_actAddSrc;
  QAction* m_actOpenFile;
  QAction* m_actRemoveFileSet;
  QAction* m_actRemoveFile;
  QAction* m_actSetAsTop;
  QAction* m_actSetAsTarget;
  QAction* m_actMakeActive;

  void UpdateSrcHierachyTree();
};

#endif  // SOURCES_FORM_H
