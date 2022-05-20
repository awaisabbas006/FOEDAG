/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CompilerDefines.h"

#include <QHeaderView>

#include "Compiler.h"
#include "TaskManager.h"
#include "TaskModel.h"
#include "TaskTableView.h"

QWidget *FOEDAG::prepareCompilerView(Compiler *compiler,
                                     TaskManager **taskManager) {
  TaskManager *tManager = new TaskManager;
  TaskModel *model = new TaskModel{tManager};
  TaskTableView *view = new TaskTableView{tManager};
  view->setModel(model);

  view->setColumnWidth(0, 30);
  view->setColumnWidth(1, 160);
  view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  view->horizontalHeader()->setStretchLastSection(true);

  compiler->setTaskManager(tManager);
  if (taskManager) *taskManager = tManager;
  return view;
}

uint FOEDAG::toTaskId(Compiler::Action action) {
  switch (action) {
    case Compiler::Action::Synthesis:
      return SYNTHESIS;
    case Compiler::Action::Global:
      return GLOBAL_PLACEMENT;
    case Compiler::Action::Detailed:
      return PLACEMENT;
    case Compiler::Action::Pack:
      return PACKING;
    case Compiler::Action::Routing:
      return ROUTING;
    case Compiler::Action::STA:
      return TIMING_SIGN_OFF;
    case Compiler::Action::Bitstream:
      return BITSTREAM;
    case Compiler::Action::Power:
      return POWER;
    case Compiler::Action::IPGen:
      return IP_GENERATE;
    case Compiler::Action::NoAction:
    case Compiler::Action::Batch:
      return TaskManager::invalid_id;
  }
  return TaskManager::invalid_id;
}
