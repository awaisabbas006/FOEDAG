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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/CompilerOpenFPGA.h"
#include "Compiler/Constraints.h"
#include "NewProject/ProjectManager/project_manager.h"

using namespace FOEDAG;

extern const char* foedag_version_number;
extern const char* foedag_git_hash;
void CompilerOpenFPGA::Version(std::ostream* out) {
  (*out) << "Foedag OpenFPGA Compiler"
         << "\n";
  if (std::string(foedag_version_number) != "${VERSION_NUMBER}")
    (*out) << "Version : " << foedag_version_number << "\n";
  if (std::string(foedag_git_hash) != "${GIT_HASH}")
    (*out) << "Git Hash: " << foedag_git_hash << "\n";
  (*out) << "Built   : " << std::string(__DATE__) << "\n";
}

void CompilerOpenFPGA::Help(std::ostream* out) {
  (*out) << "----------------------------------" << std::endl;
  (*out) << "-----  FOEDAG OpenFPGA HELP  -----" << std::endl;
  (*out) << "----------------------------------" << std::endl;
  (*out) << "Options:" << std::endl;
  (*out) << "   --help           : This help" << std::endl;
  (*out) << "   --version        : Version" << std::endl;
  (*out) << "   --batch          : Tcl only, no GUI" << std::endl;
  (*out) << "   --replay <script>: Replay GUI test" << std::endl;
  (*out) << "   --script <script>: Execute a Tcl script" << std::endl;
  (*out) << "   --compiler <name>: Compiler name {openfpga...}, default is "
            "a dummy compiler"
         << std::endl;
  (*out) << "   --verific        : Uses Verific parser" << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help                       : This help" << std::endl;
  (*out) << "   create_design <name>       : Creates a design with <name> name"
         << std::endl;
  (*out) << "   target_device <name>       : Targets a device with <name> name"
         << std::endl;
  (*out) << "   architecture <vpr_file.xml> ?<openfpga_file.xml>? :"
         << std::endl;
  (*out) << "                                Uses the architecture file and "
            "optional openfpga arch file (For bitstream generation)"
         << std::endl;
  (*out) << "   bitstream_config_files -bitstream <bitstream_setting.xml> "
            "-sim <sim_setting.xml> -repack <repack_setting.xml>"
         << std::endl;
  (*out) << "                              : Uses alternate bitstream "
            "generation configuration files"
         << std::endl;
  (*out) << "   set_device_size XxY        : Device fabric size selection"
         << std::endl;
  (*out) << "   custom_synth_script <file> : Uses a custom Yosys templatized "
            "script"
         << std::endl;
  (*out) << "   custom_openfpga_script <file> : Uses a custom OpenFPGA "
            "templatized "
            "script"
         << std::endl;
  (*out) << "   set_channel_width <int>    : VPR Routing channel setting"
         << std::endl;
  (*out) << "   add_design_file <file>... <type> (-VHDL_1987, -VHDL_1993, "
            "-VHDL_2000, "
            "-VHDL_2008 (.vhd default), -V_1995, "
            "-V_2001 (.v default), -SV_2005, -SV_2009, -SV_2012, -SV_2017 (.sv "
            "default)) "
         << std::endl;
  (*out) << "   read_netlist <file>        : Read a netlist instead of an RTL "
            "design (Skip Synthesis)"
         << std::endl;
  (*out) << "   add_include_path <path1>...: As in +incdir+" << std::endl;
  (*out) << "   add_library_path <path1>...: As in +libdir+" << std::endl;
  (*out) << "   set_macro <name>=<value>...: As in -D<macro>=<value>"
         << std::endl;
  (*out) << "   set_top_module <top>       : Sets the top module" << std::endl;
  (*out) << "   add_constraint_file <file> : Sets SDC + location constraints"
         << std::endl;
  (*out) << "                                Constraints: set_pin_loc, "
            "set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   ipgenerate" << std::endl;
  (*out) << "   verific_parser <on/off>    : Turns on/off Verific parser"
         << std::endl;
  (*out) << "   synthesize <optimization>  : Optional optimization (area, "
            "delay, mixed, none)"
         << std::endl;
  (*out) << "   pnr_options <option list>  : VPR Options" << std::endl;
  (*out) << "   packing                    : Packing" << std::endl;
  (*out) << "   global_placement           : Analytical placer" << std::endl;
  (*out) << "   place                      : Detailed placer" << std::endl;
  (*out) << "   route                      : Router" << std::endl;
  (*out) << "   sta                        : Statistical Timing Analysis"
         << std::endl;
  (*out) << "   power                      : Power estimator" << std::endl;
  (*out) << "   bitstreamm                 : Bitstream generation" << std::endl;
  (*out) << "----------------------------------" << std::endl;
}

// https://github.com/lnis-uofu/OpenFPGA/blob/master/openfpga_flow/misc/ys_tmpl_yosys_vpr_flow.ys
const std::string basicYosysScript = R"( 
# Yosys synthesis script for ${TOP_MODULE}
# Read source files
${READ_DESIGN_FILES}

# Technology mapping
hierarchy -top ${TOP_MODULE}
proc
${KEEP_NAMES}
techmap -D NO_LUT -map +/adff2dff.v

# Synthesis
flatten
opt_expr
opt_clean
check
opt -nodffe -nosdff
fsm
opt -nodffe -nosdff
wreduce
peepopt
opt_clean
opt -nodffe -nosdff
memory -nomap
opt_clean
opt -fast -full -nodffe -nosdff
memory_map
opt -full -nodffe -nosdff
techmap
opt -fast -nodffe -nosdff
clean

# LUT mapping
abc -lut ${LUT_SIZE}

# Check
synth -run check

# Clean and output blif
opt_clean -purge
write_blif ${OUTPUT_BLIF}
write_verilog -noexpr -nodec -defparam -norename ${OUTPUT_VERILOG}
  )";

bool CompilerOpenFPGA::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  Compiler::RegisterCommands(interp, batchMode);
  auto select_architecture_file = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify an architecture file");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (compiler->FileExists(expandedFile)) {
        use_orig_path = true;
      }

      if ((!use_orig_path) &&
          (!compiler->GetSession()->CmdLine()->Script().empty())) {
        std::filesystem::path script =
            compiler->GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        std::filesystem::path fullPath = scriptPath;
        fullPath.append(argv[i]);
        expandedFile = fullPath.string();
      }

      std::ifstream stream(expandedFile);
      if (!stream.good()) {
        compiler->ErrorMessage("Cannot find architecture file: " +
                               std::string(expandedFile));
        return TCL_ERROR;
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
      }
      stream.close();
      if (i == 1) {
        compiler->ArchitectureFile(expandedFile);
        compiler->Message("VPR Architecture file: " + expandedFile);
      } else {
        compiler->OpenFpgaArchitectureFile(expandedFile);
        compiler->Message("OpenFPGA Architecture file: " + expandedFile);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("architecture", select_architecture_file, this, 0);

  auto set_bitstream_config_files = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify a bitstream config file");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      std::string fileType;
      if (arg == "-bitstream") {
        fileType = "bitstream";
      } else if (arg == "-sim") {
        fileType = "sim";
      } else if (arg == "-repack") {
        fileType = "repack";
      } else {
        compiler->ErrorMessage(
            "Not a legal option for bitstream_config_files: " + arg);
        return TCL_ERROR;
      }
      i++;
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (compiler->FileExists(expandedFile)) {
        use_orig_path = true;
      }

      if ((!use_orig_path) &&
          (!compiler->GetSession()->CmdLine()->Script().empty())) {
        std::filesystem::path script =
            compiler->GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        std::filesystem::path fullPath = scriptPath;
        fullPath.append(argv[i]);
        expandedFile = fullPath.string();
      }

      std::ifstream stream(expandedFile);
      if (!stream.good()) {
        compiler->ErrorMessage("Cannot find bitstream config file: " +
                               std::string(expandedFile));
        return TCL_ERROR;
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
      }
      stream.close();
      if (fileType == "bitstream") {
        compiler->OpenFpgaBitstreamSettingFile(expandedFile);
        compiler->Message("OpenFPGA Bitstream Setting file: " + expandedFile);
      } else if (fileType == "sim") {
        compiler->OpenFpgaSimSettingFile(expandedFile);
        compiler->Message("OpenFPGA Simulation Setting file: " + expandedFile);
      } else if (fileType == "repack") {
        compiler->OpenFpgaRepackConstraintsFile(expandedFile);
        compiler->Message("OpenFPGA Repack Constraint file: " + expandedFile);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("bitstream_config_files", set_bitstream_config_files,
                      this, 0);

  auto custom_openfpga_script = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify an OpenFPGA script");
      return TCL_ERROR;
    }

    std::string expandedFile = argv[1];
    bool use_orig_path = false;
    if (compiler->FileExists(expandedFile)) {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(argv[1]);
      expandedFile = fullPath.string();
    }
    std::ifstream stream(expandedFile);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find OpenFPGA script: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    std::string script((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
    stream.close();
    compiler->OpenFPGAScript(script);
    return TCL_OK;
  };
  interp->registerCmd("custom_openfpga_script", custom_openfpga_script, this,
                      0);

  auto custom_synth_script = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a Yosys script");
      return TCL_ERROR;
    }

    std::string expandedFile = argv[1];
    bool use_orig_path = false;
    if (compiler->FileExists(expandedFile)) {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(argv[1]);
      expandedFile = fullPath.string();
    }
    std::ifstream stream(expandedFile);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find Yosys script: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    std::string script((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
    stream.close();
    compiler->YosysScript(script);
    return TCL_OK;
  };
  interp->registerCmd("custom_synth_script", custom_synth_script, this, 0);

  auto set_channel_width = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a channel width");
      return TCL_ERROR;
    }
    compiler->ChannelWidth(std::strtoul(argv[1], 0, 10));
    return TCL_OK;
  };
  interp->registerCmd("set_channel_width", set_channel_width, this, 0);

  auto set_device_size = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a device size: xXy");
      return TCL_ERROR;
    }
    compiler->DeviceSize(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("set_device_size", set_device_size, this, 0);

  auto verific_parser = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify on/off");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    compiler->SetUseVerific((arg == "on") ? true : false);
    return TCL_OK;
  };
  interp->registerCmd("verific_parser", verific_parser, this, 0);

  auto target_device = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Please select a device");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (compiler->LoadDeviceData(arg)) {
      compiler->ProjManager()->setTargetDevice(arg);
    } else {
      compiler->ErrorMessage("Invalid target device: " + arg);
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("target_device", target_device, this, 0);

  return true;
}

bool CompilerOpenFPGA::IPGenerate() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_projManager->projectName()
           << "..." << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " IPs are generated!"
           << std::endl;
  m_state = State::IPGenerated;
  return true;
}

bool CompilerOpenFPGA::DesignChanged(
    const std::string& synth_script,
    const std::filesystem::path& synth_scrypt_path) {
  bool result = false;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(path /
                                m_projManager->projectName());  // setting path
  std::string output = m_projManager->projectName() + "_post_synth.blif";
  time_t time_netlist = Mtime(output);
  if (time_netlist == -1) {
    result = true;
  }
  for (const auto& lang_file : m_projManager->DesignFiles()) {
    std::vector<std::string> tokens;
    Tokenize(lang_file, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : m_projManager->includePathList()) {
    std::vector<std::string> tokens;
    Tokenize(path, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : m_projManager->libraryPathList()) {
    std::vector<std::string> tokens;
    Tokenize(path, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }

  std::ifstream script(synth_scrypt_path);
  if (!script.good()) {
    result = true;
  }
  std::stringstream buffer;
  buffer << script.rdbuf();
  if (synth_script != buffer.str()) {
    result = true;
  }
  std::filesystem::current_path(path);
  return result;
}

bool CompilerOpenFPGA::Synthesize() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << m_projManager->projectName() << "..."
           << std::endl;

  std::string yosysScript = InitSynthesisScript();

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF:
        Message("Skipping synthesis, gate-level design.");
        return true;
        break;
      default:
        break;
    }
  }

  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    std::string includes;
    for (auto path : m_projManager->includePathList()) {
      includes += path + " ";
    }
    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : m_projManager->libraryPathList()) {
      libraries += path + " ";
    }
    fileList += "verific -vlog-libdir " + libraries + "\n";

    std::string macros;
    for (auto& macro_value : m_projManager->macroList()) {
      macros += macro_value.first + "=" + macro_value.second + " ";
    }
    fileList += "verific -vlog-define " + macros + "\n";

    for (const auto& lang_file : m_projManager->DesignFiles()) {
      std::string lang;
      switch (m_projManager->designFileData(lang_file)) {
        case Design::Language::VHDL_1987:
          lang = "-vhdl87";
          break;
        case Design::Language::VHDL_1993:
          lang = "-vhdl93";
          break;
        case Design::Language::VHDL_2000:
          lang = "-vhdl2k";
          break;
        case Design::Language::VHDL_2008:
          lang = "-vhdl2008";
          break;
        case Design::Language::VERILOG_1995:
          lang = "-vlog95";
          break;
        case Design::Language::VERILOG_2001:
          lang = "-vlog2k";
          break;
        case Design::Language::SYSTEMVERILOG_2005:
          lang = "-sv2005";
          break;
        case Design::Language::SYSTEMVERILOG_2009:
          lang = "-sv2009";
          break;
        case Design::Language::SYSTEMVERILOG_2012:
          lang = "-sv2012";
          break;
        case Design::Language::SYSTEMVERILOG_2017:
          lang = "-sv";
          break;
        case Design::Language::VERILOG_NETLIST:
          lang = "";
          break;
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          lang = "BLIF";
          ErrorMessage("Unsupported file format:" + lang);
          return false;
      }
      fileList += "verific " + lang + " " + lang_file + "\n";
    }
    fileList += "verific -import " + m_projManager->DesignTopModule() + "\n";
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
  } else {
    // Default Yosys parser
    std::string macros = "verilog_defines ";
    for (auto& macro_value : m_projManager->macroList()) {
      macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    }
    macros += "\n";
    std::string includes;
    for (auto path : m_projManager->includePathList()) {
      includes += "-I" + path + " ";
    }
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}",
                             macros +
                                 "read_verilog ${READ_VERILOG_OPTIONS} "
                                 "${INCLUDE_PATHS} ${VERILOG_FILES}");
    std::string fileList;
    std::string lang;
    for (const auto& lang_file : m_projManager->DesignFiles()) {
      fileList += lang_file + " ";
      switch (m_projManager->designFileData(lang_file)) {
        case Design::Language::VHDL_1987:
        case Design::Language::VHDL_1993:
        case Design::Language::VHDL_2000:
        case Design::Language::VHDL_2008:
          ErrorMessage("Unsupported language (Yosys default parser)!");
          break;
        case Design::Language::VERILOG_1995:
        case Design::Language::VERILOG_2001:
        case Design::Language::SYSTEMVERILOG_2005:
          break;
        case Design::Language::SYSTEMVERILOG_2009:
        case Design::Language::SYSTEMVERILOG_2012:
        case Design::Language::SYSTEMVERILOG_2017:
          lang = "-sv";
          break;
        case Design::Language::VERILOG_NETLIST:
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          ErrorMessage("Unsupported language (Yosys default parser)!");
          break;
      }
    }
    yosysScript = ReplaceAll(yosysScript, "${INCLUDE_PATHS}", includes);
    std::string options = lang;

    yosysScript = ReplaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", options);
    yosysScript = ReplaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  }

  yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}",
                           m_projManager->DesignTopModule());
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_BLIF}",
      std::string(m_projManager->projectName() + "_post_synth.blif"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_VERILOG}",
                 std::string(m_projManager->projectName() + "_post_synth.v"));

  yosysScript = FinishSynthesisScript(yosysScript);

  std::string script_path = m_projManager->projectName() + ".ys";
  if (!DesignChanged(yosysScript, script_path)) {
    (*m_out) << "Design didn't change: " << m_projManager->projectName()
             << ", skipping synthesis." << std::endl;
    return true;
  }
  std::filesystem::remove(
      std::filesystem::path(m_projManager->projectName()) /
      std::string(m_projManager->projectName() + "_post_synth.blif"));
  std::filesystem::remove(
      std::filesystem::path(m_projManager->projectName()) /
      std::string(m_projManager->projectName() + "_post_synth.v"));
  // Create Yosys command and execute
  script_path =
      (std::filesystem::path(m_projManager->projectName()) / script_path)
          .string();
  std::ofstream ofs(script_path);
  ofs << yosysScript;
  ofs.close();
  if (!FileExists(m_yosysExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
    return false;
  }
  std::string command =
      m_yosysExecutablePath.string() + " -s " +
      std::string(m_projManager->projectName() + ".ys -l " +
                  m_projManager->projectName() + "_synth.log");
  (*m_out) << "Synthesis command: " << command << std::endl;
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " synthesis failed!");
    return false;
  } else {
    m_state = State::Synthesized;
    (*m_out) << "Design " << m_projManager->projectName() << " is synthesized!"
             << std::endl;
    return true;
  }
}

std::string CompilerOpenFPGA::InitSynthesisScript() {
  // Default or custom Yosys script
  if (m_yosysScript.empty()) {
    m_yosysScript = basicYosysScript;
  }
  return m_yosysScript;
}

std::string CompilerOpenFPGA::FinishSynthesisScript(const std::string& script) {
  std::string result = script;
  // Keeps for Synthesis, preserve nodes used in constraints
  std::string keeps;
  if (m_keepAllSignals) {
    keeps += "setattr -set keep 1 w:\\*\n";
  }
  for (auto keep : m_constraints->GetKeeps()) {
    (*m_out) << "Keep name: " << keep << "\n";
    keeps += "setattr -set keep 1 " + keep + "\n";
  }
  result = ReplaceAll(result, "${KEEP_NAMES}", keeps);
  result = ReplaceAll(result, "${OPTIMIZATION}", "");
  result = ReplaceAll(result, "${LUT_SIZE}", std::to_string(m_lut_size));
  return result;
}

std::string CompilerOpenFPGA::BaseVprCommand() {
  std::string device_size = "";
  if (!m_deviceSize.empty()) {
    device_size = " --device " + m_deviceSize;
  }
  std::string netlistFile = m_projManager->projectName() + "_post_synth.blif";

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file;
        std::filesystem::path the_path = netlistFile;
        if (!the_path.is_absolute()) {
          netlistFile =
              std::filesystem::path(std::filesystem::path("..") / netlistFile)
                  .string();
        }
        break;
      }
      default:
        break;
    }
  }

  std::string pnrOptions;
  if (!PnROpt().empty()) pnrOptions = " " + PnROpt();

  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile + std::string(" --sdc_file ") +
                  std::string(m_projManager->projectName() + "_openfpga.sdc") +
                  std::string(" --route_chan_width ") +
                  std::to_string(m_channel_width) + device_size + pnrOptions);
  return command;
}

bool CompilerOpenFPGA::Packing() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  const std::string sdcOut =
      (std::filesystem::path(m_projManager->projectName()) /
       std::string(m_projManager->projectName() + "_openfpga.sdc"))
          .string();
  std::ofstream ofssdc(sdcOut);
  // TODO: Massage the SDC so VPR can understand them
  for (auto constraint : m_constraints->getConstraints()) {
    (*m_out) << "Constraint: " << constraint << "\n";
    // Parse RTL and expand the get_ports, get_nets
    // Temporary dirty filtering:
    std::vector<std::string> tokens;
    Tokenize(constraint, " ", tokens);
    constraint = "";
    // VPR does not understand: create_clock -period 2 clk -name <logical_name>
    // Pass the constraint as-is anyway
    for (uint32_t i = 0; i < tokens.size(); i++) {
      const std::string& tok = tokens[i];
      constraint += tok + " ";
    }

    // pin location constraints have to be translated to .place:
    if (constraint.find("set_pin_loc") != std::string::npos) {
      continue;
    }
    ofssdc << constraint << "\n";
  }
  ofssdc.close();

  std::string command = BaseVprCommand() + " --pack";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_pack.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();

  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() + " packing failed!");
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << m_projManager->projectName() << " is packed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA::GlobalPlacement() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_projManager->projectName()
           << "..." << std::endl;
  // TODO:
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << m_projManager->projectName()
           << " is globally placed!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::Placement() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
    return false;
  }
  (*m_out) << "Placement for design: " << m_projManager->projectName() << "..."
           << std::endl;
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  std::string command = BaseVprCommand() + " --place";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_place.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " placement failed!");
    return false;
  }
  m_state = State::Placed;
  (*m_out) << "Design " << m_projManager->projectName() << " is placed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA::Route() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Placed) {
    ErrorMessage("Design needs to be in placed state");
    return false;
  }
  (*m_out) << "Routing for design: " << m_projManager->projectName() << "..."
           << std::endl;
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  std::string command = BaseVprCommand() + " --route";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_route.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() + " routing failed!");
    return false;
  }
  m_state = State::Routed;
  (*m_out) << "Design " << m_projManager->projectName() << " is routed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  (*m_out) << "Analysis for design: " << m_projManager->projectName() << "..."
           << std::endl;
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  std::string command = BaseVprCommand() + " --analysis";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_sta.cmd"))
                        .string());
  ofs << command << " --disp on" << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " timing analysis failed!");
    return false;
  }

  (*m_out) << "Design " << m_projManager->projectName()
           << " is timing analysed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  (*m_out) << "Analysis for design: " << m_projManager->projectName() << "..."
           << std::endl;
  std::string command = BaseVprCommand() + " --analysis";
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " power analysis failed!");
    return false;
  }

  (*m_out) << "Design " << m_projManager->projectName() << " is power analysed!"
           << std::endl;
  return true;
}

const std::string basicOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --write_rr_graph rr_graph.openfpga.xml --constant_net_method route --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT}  --analysis

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation settings
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

read_openfpga_bitstream_setting -f ${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

# Apply fix-up to clustering nets based on routing results
pb_pin_fixup --verbose

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --compress_routing --duplicate_grid_pin 

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream

build_fabric_bitstream
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

std::string CompilerOpenFPGA::InitOpenFPGAScript() {
  // Default or custom OpenFPGA script
  if (m_openFPGAScript.empty()) {
    m_openFPGAScript = basicOpenFPGABitstreamScript;
  }
  return m_openFPGAScript;
}

std::string CompilerOpenFPGA::FinishOpenFPGAScript(const std::string& script) {
  std::string result = script;

  std::string netlistFilePrefix = m_projManager->projectName() + "_post_synth";

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        std::filesystem::path the_path = lang_file;
        std::filesystem::path filename = the_path.filename();
        std::filesystem::path stem = filename.stem();
        netlistFilePrefix = stem.string();
        break;
      }
      default:
        break;
    }
  }

  result = ReplaceAll(result, "${VPR_ARCH_FILE}", m_architectureFile.string());
  result = ReplaceAll(result, "${NET_FILE}", netlistFilePrefix + ".net");
  result = ReplaceAll(result, "${PLACE_FILE}", netlistFilePrefix + ".place");
  result = ReplaceAll(result, "${ROUTE_FILE}", netlistFilePrefix + ".route");
  result = ReplaceAll(result, "${SDC_FILE}",
                      m_projManager->projectName() + "_openfpga.sdc");

  std::string netlistFile = m_projManager->projectName() + "_post_synth.blif";
  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file;
        std::filesystem::path the_path = netlistFile;
        if (!the_path.is_absolute()) {
          netlistFile =
              std::filesystem::path(std::filesystem::path("..") / netlistFile)
                  .string();
        }
        break;
      }
      default:
        break;
    }
  }
  result = ReplaceAll(result, "${VPR_TESTBENCH_BLIF}", netlistFile);

  std::string netlistFormat = "blif";
  result = ReplaceAll(result, "${OPENFPGA_VPR_CIRCUIT_FORMAT}", netlistFormat);
  if (m_deviceSize.size()) {
    result = ReplaceAll(result, "${OPENFPGA_VPR_DEVICE_LAYOUT}",
                        " --device " + m_deviceSize);
  } else {
    result = ReplaceAll(result, "${OPENFPGA_VPR_DEVICE_LAYOUT}", "");
  }
  result = ReplaceAll(result, "${OPENFPGA_VPR_ROUTE_CHAN_WIDTH}",
                      std::to_string(m_channel_width));
  result = ReplaceAll(result, "${OPENFPGA_ARCH_FILE}",
                      m_OpenFpgaArchitectureFile.string());

  result = ReplaceAll(result, "${OPENFPGA_SIM_SETTING_FILE}",
                      m_OpenFpgaSimSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                      m_OpenFpgaBitstreamSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_REPACK_CONSTRAINTS}",
                      m_OpenFpgaRepackConstraintsFile.string());
  return result;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (BitsOpt() == BitstreamOpt::NoBitsOpt) {
    if (m_state != State::Routed) {
      ErrorMessage("Design needs to be in routed state");
      return false;
    }
  }
  (*m_out) << "Bitstream generation for design: "
           << m_projManager->projectName() << "..." << std::endl;

  if (BitsOpt() == BitstreamOpt::NoBitsOpt) {
    (*m_out) << "Design " << m_projManager->projectName()
             << " bitstream is generated!" << std::endl;
    return true;
  }
  // This is WIP, have to force it to execute (bitstream force)

  std::string command = m_openFpgaExecutablePath.string() + " -f " +
                        m_projManager->projectName() + ".openfpga";

  std::string script = InitOpenFPGAScript();

  script = FinishOpenFPGAScript(script);

  std::string script_path = m_projManager->projectName() + ".openfpga";

  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string("fabric_bitstream.bit"));
  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string("fabric_independent_bitstream.xml"));
  // Create OpenFpga command and execute
  script_path =
      (std::filesystem::path(m_projManager->projectName()) / script_path)
          .string();
  std::ofstream sofs(script_path);
  sofs << script;
  sofs.close();
  if (!FileExists(m_openFpgaExecutablePath)) {
    ErrorMessage("Cannot find executable: " +
                 m_openFpgaExecutablePath.string());
    return false;
  }

  std::ofstream ofs(
      (std::filesystem::path(m_projManager->projectName()) /
       std::string(m_projManager->projectName() + "_bitstream.cmd"))
          .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " bitream generation failed!");
    return false;
  }
  m_state = State::BistreamGenerated;

  (*m_out) << "Design " << m_projManager->projectName()
           << " bitstream is generated!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::LoadDeviceData(const std::string& deviceName) {
  bool status = true;
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
  std::string devicefile =
      datapath / std::string("etc") / std::string("device.xml");
  QFile file(devicefile.c_str());
  if (!file.open(QFile::ReadOnly)) {
    ErrorMessage("Cannot open device file: " + devicefile);
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    ErrorMessage("Incorrect device file: " + devicefile);
    return false;
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  bool foundDevice = false;
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      std::string name = e.attribute("name").toStdString();
      if (name == deviceName) {
        foundDevice = true;
        QDomNodeList list = e.childNodes();
        for (int i = 0; i < list.count(); i++) {
          QDomNode n = list.at(i);
          if (!n.isNull() && n.isElement()) {
            if (n.nodeName() == "internal") {
              std::string file_type =
                  n.toElement().attribute("type").toStdString();
              std::string file = n.toElement().attribute("file").toStdString();
              std::string fullPath;
              if (FileExists(file)) {
                fullPath = file;  // Absolute path
              } else {
                fullPath = datapath / std::string("etc") /
                           std::string("devices") / file;
              }
              if (!FileExists(fullPath)) {
                ErrorMessage("Invalid device config file: " + fullPath + "\n");
                status = false;
              }
              if (file_type == "vpr_arch") {
                ArchitectureFile(fullPath);
              } else if (file_type == "openfpga_arch") {
                OpenFpgaArchitectureFile(fullPath);
              } else if (file_type == "bitstream_settings") {
                OpenFpgaBitstreamSettingFile(fullPath);
              } else if (file_type == "sim_settings") {
                OpenFpgaSimSettingFile(fullPath);
              } else if (file_type == "repack_settings") {
                OpenFpgaRepackConstraintsFile(fullPath);
              } else if (file_type == "pinmap_xml") {
                OpenFpgaPinmapXMLFile(fullPath);
              } else if (file_type == "pinmap_csv") {
                OpenFpgaPinmapCSVFile(fullPath);
              } else {
                ErrorMessage("Invalid device config type: " + file_type + "\n");
                status = false;
              }
            }
          }
        }
      }
    }

    node = node.nextSibling();
  }
  if (!foundDevice) {
    ErrorMessage("Incorrect device: " + deviceName + "\n");
    status = false;
  }

  return status;
}
