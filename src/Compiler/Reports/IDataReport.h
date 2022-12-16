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
#pragma once

#include <QStringList>
#include <QVector>

namespace FOEDAG {

/* Given interface represents the data report, holding single type of data:
 * Timing info, resource usage, cirtuit statistics etc.
 * It returns row and column data as strings. For simplicity, only
 * standard table data is supported, no parent items. It can be
 * extended if needed.
 */
class IDataReport {
 public:
  virtual ~IDataReport() = default;

  using LineValues = QStringList;
  using TableData = QVector<LineValues>;
  // Returns report column names
  virtual const LineValues &getColumns() const = 0;
  // Returns report data - rows of values
  virtual const TableData &getData() const = 0;
  // Returns report name
  virtual const QString &getName() const = 0;
};
}  // namespace FOEDAG
