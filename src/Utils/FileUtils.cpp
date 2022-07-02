/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "Utils/FileUtils.h"

#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <QProcess>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace FOEDAG {

bool FileUtils::fileExists(const std::filesystem::path& name) {
  std::error_code ec;
  return std::filesystem::exists(name, ec);
}

uint64_t FileUtils::fileSize(const std::filesystem::path& name) {
  std::error_code ec;
  return std::filesystem::file_size(name, ec);
}

bool FileUtils::fileIsDirectory(const std::filesystem::path& name) {
  return std::filesystem::is_directory(name);
}

bool FileUtils::fileIsRegular(const std::filesystem::path& name) {
  return std::filesystem::is_regular_file(name);
}

bool FileUtils::mkDirs(const std::filesystem::path& path) {
  // CAUTION: There is a known bug in VC compiler where a trailing
  // slash in the path will cause a false return from a call to
  // fs::create_directories.
  std::error_code err;
  std::filesystem::create_directories(path, err);
  return std::filesystem::is_directory(path);
}

bool FileUtils::rmDirRecursively(const std::filesystem::path& path) {
  static constexpr uintmax_t kErrorCondition = static_cast<std::uintmax_t>(-1);
  std::error_code err;
  return std::filesystem::remove_all(path, err) != kErrorCondition;
}

std::filesystem::path FileUtils::getFullPath(
    const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::path fullPath = std::filesystem::canonical(path, ec);
  return ec ? path : fullPath;
}

bool FileUtils::getFullPath(const std::filesystem::path& path,
                            std::filesystem::path* result) {
  std::error_code ec;
  std::filesystem::path fullPath = std::filesystem::canonical(path, ec);
  bool found = (!ec && fileIsRegular(fullPath));
  if (result != nullptr) {
    *result = found ? fullPath : path;
  }
  return found;
}

std::string FileUtils::getFileContent(const std::filesystem::path& filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  std::string result;

  if (in) {
    std::error_code err;
    const size_t prealloc = std::filesystem::file_size(filename, err);
    if (err.value() == 0) result.reserve(prealloc);

    char buffer[4096];
    while (in.good() && !in.eof()) {
      in.read(buffer, sizeof(buffer));
      result.append(buffer, in.gcount());
    }
  } else {
    result = "FAILED_TO_LOAD_CONTENT";
  }
  return result;
}

std::filesystem::path FileUtils::getPathName(
    const std::filesystem::path& path) {
  return path.has_parent_path() ? path.parent_path() : "";
}

std::filesystem::path FileUtils::basename(const std::filesystem::path& path) {
  return path.filename();
}

std::filesystem::path FileUtils::getPreferredPath(
    const std::filesystem::path& path) {
  return std::filesystem::path(path).make_preferred();
}

std::filesystem::path FileUtils::locateExecFile(
    const std::filesystem::path& path) {
  std::filesystem::path result;
  char* envpath = getenv("PATH");
  char* dir = nullptr;

  for (dir = strtok(envpath, ":"); dir; dir = strtok(NULL, ":")) {
    std::filesystem::path a_path = std::string(dir) / path;
    if (FileUtils::fileExists(a_path)) {
      return a_path;
    }
  }

  for (std::filesystem::path dir :
       {"/usr/bin", "/usr/local/bin", "~/.local/bin", "./"}) {
    std::filesystem::path a_path = dir / path;
    if (FileUtils::fileExists(a_path)) {
      return a_path;
    }
  }

  return result;
}

int FileUtils::ExecuteSystemCommand(const std::string& command,
                                    std::ostream* result) {
  QProcess* m_process = new QProcess;

  QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                   [result, m_process]() {
                     result->write(m_process->readAllStandardOutput(),
                                   m_process->bytesAvailable());
                   });

  QObject::connect(m_process, &QProcess::readyReadStandardError,
                   [result, m_process]() {
                     QByteArray data = m_process->readAllStandardError();
                     result->write(data, data.size());
                   });

  QString cmd{command.c_str()};
  QStringList args = cmd.split(" ");
  QString program = args.first();
  args.pop_front();  // remove program
  m_process->start(program, args);

  m_process->waitForFinished(-1);

  auto status = m_process->exitStatus();
  auto exitCode = m_process->exitCode();
  delete m_process;
  m_process = nullptr;

  return (status == QProcess::NormalExit) ? exitCode : -1;
}

}  // namespace FOEDAG
