//===-- Interpreter -------------------------------------------------------===//
// main file interpreter.cpp
//===----------------------------------------------------------------------===//

//#define NDEBUG
#define DBT
#include "OiMachineModel.h"
#include "OiMemoryModel.h"
#include "StringRefMemoryObject.h"
#include "InterpUtils.h"
#include "FrontEnd/MC2IRStreamer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELF.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ToolOutputFile.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <fstream>

#ifndef NDEBUG
extern "C" {
#include "string.h"
}
#endif

namespace llvm {

namespace object {
  class COFFObjectFile;
  class ObjectFile;
  class RelocationRef;
}

extern cl::opt<std::string> ArchName;

// Various helper functions.
bool RelocAddressLess(object::RelocationRef a, object::RelocationRef b);
void DumpBytes(StringRef bytes);
void DisassembleInputMachO(StringRef Filename);
void printCOFFUnwindInfo(const object::COFFObjectFile* o);
void printELFFileHeader(const object::ObjectFile *o);

}
using namespace llvm;
using namespace object;
using namespace std;

namespace llvm {
  extern Target TheOiTarget, TheOielTarget;
}

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input object files>"));

static cl::list<std::string>
GuestArguments(cl::Positional, cl::desc("<guest program arguments>"));

static cl::opt<bool>
FullPath("fullpath", cl::desc("Display full path of source files when dumping DWARF info"));

static cl::opt<uint64_t>
Cap("cap", cl::desc("Specifies the maximum number of instructions to interpret"
                                "(Default 0 = unbounded)"),
          cl::init(0ULL));

extern cl::opt<int32_t> Verbosity;

static StringRef ToolName;

std::string TripleName = "mipsel-unknown-unknown";

static const Target *getTarget(const ObjectFile *Obj = NULL) {
  // Figure out the target triple.
  llvm::Triple TheTriple("unknown-unknown-unknown");

  // Get the target specific parser.
  const Target *TheTarget = &TheOielTarget; 
  if (!TheTarget) {
    errs() << "Could not load OpenISA target.";
    return 0;
  }

  // Update the triple name and return the found target.
  TripleName = "mipsel-unknown-unknown";
  return TheTarget;
}


#ifndef NDEBUG
static void PrintDILineInfo(DILineInfo dli) {
  //  if (PrintFunctions)
  //    outs() << (dli.getFunctionName() ? dli.getFunctionName() : "<unknown>")
  //           << "\n";
  dbgs() << (dli.getFileName() ? dli.getFileName() : "<unknown>") << ':'
         << dli.getLine() << ':' << dli.getColumn() << '\n';
}
#endif

static void ExecutionLoop(StringRef file, int argc, char **argv) {
  const Target *TheTarget = getTarget();
  if (!TheTarget)
    return;

  // Package up features to be passed to target/subtarget
  std::string FeaturesStr;
  //if (MAttrs.size()) {
  //SubtargetFeatures Features;
  //    for (unsigned i = 0; i != MAttrs.size(); ++i)
  //      Features.AddFeature(MAttrs[i]);
  //    FeaturesStr = Features.getString();
  //  }

  // Set up disassembler.
  OwningPtr<const MCAsmInfo> AsmInfo(TheTarget->createMCAsmInfo(TripleName));

  if (!AsmInfo) {
    errs() << "error: no assembly info for target " << TripleName << "\n";
    return;
  }

  OwningPtr<const MCSubtargetInfo> STI(
          TheTarget->createMCSubtargetInfo(TripleName, "", FeaturesStr));

  if (!STI) {
    errs() << "error: no subtarget info for target " << TripleName << "\n";
    return;
  }

  OwningPtr<const MCDisassembler> DisAsm(
          TheTarget->createMCDisassembler(*STI));
  if (!DisAsm) {
    errs() << "error: no disassembler for target " << TripleName << "\n";
    return;
  }

  OwningPtr<const MCRegisterInfo> MRI(TheTarget->createMCRegInfo(TripleName));
  if (!MRI) {
    errs() << "error: no register info for target " << TripleName << "\n";
    return;
  }

  OwningPtr<const MCInstrInfo> MII(TheTarget->createMCInstrInfo());
  if (!MII) {
    errs() << "error: no instruction info for target " << TripleName << "\n";
    return;
  }

  int AsmPrinterVariant = AsmInfo->getAssemblerDialect();

  OwningPtr<MCInstPrinter> InstPrinter(TheTarget->createMCInstPrinter(AsmPrinterVariant,
     *AsmInfo, *MII, *MRI, *STI));
  if (!InstPrinter) {
    errs() << "error: no instruction printer for target " << TripleName
           << '\n';
    return;
  }

  OwningPtr<OiMemoryModel> mem(new OiMemoryModel());
  OwningPtr<OiMachineModel> IP(new OiMachineModel(*AsmInfo, *MII, *MRI, &*mem,
                                                  *InstPrinter));
  // TheTarget->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, *MII, *MRI, *STI));
  if (!IP) {
    errs() << "error: no instruction printer for target " << TripleName
           << '\n';
    return;
  }

  uint64_t CurPC = mem->LoadELF(file.data());
  int i = 0;
  for (i = 0; i < argc; ++i) {
    if (file == argv[i])
      break;
  }
  argc -= i;
  IP->ConfigureUserLevelStack(argc, &argv[i]);
  error_code ec;
  uint64_t Size;
  uint64_t numEmulated = 0;
#ifdef DBT
  DenseMap<uint32_t, uint32_t> HotAddresses;
#endif

#ifndef NDEBUG
  raw_ostream &DebugOut = dbgs();
  OwningPtr<Binary> binary;
  ObjectFile *o;
  if (ec = createBinary(file, binary)) {
    errs() << ToolName << ": '" << file << "': " << ec.message() << ".\n";
    return;
  }
  if (! (o = dyn_cast<ObjectFile>(binary.get()))) {
    errs() << ToolName << ": '" << file << "': " << "Unrecognized file type.\n";
    return;
  }
  std::vector<std::pair<uint64_t, StringRef> > Symbols =
    GetSymbolsList(o);
  DenseMap<uint32_t, StringRef> SymbolMap;
  for (std::vector<std::pair<uint64_t, StringRef> >::iterator I = 
         Symbols.begin(), E = Symbols.end(); I != E; ++I) {
    SymbolMap.insert(*I);
  }
  OwningPtr<DIContext> DICtx(DIContext::getDWARFContext(o));
  string LastFileName;
  ifstream SourceFile;
  uint32_t LastLine = 0;
#else
  raw_ostream &DebugOut = nulls();
#endif

  do {
    MCInst Inst;
#ifndef NDEBUG
    StringRef Symbol = SymbolMap.lookup((uint32_t) CurPC);
    StringRef Dummy;
    if (Verbosity > 0) {
      if (CurPC != 0 && Symbol != Dummy) {
        DebugOut << "\e[1;35m[\e[45m\e[1;37m" << Symbol 
                 << "\e[0m\e[1;35m]\e[0m\n2C";
      }
    }
#ifdef DBT
    if (CurPC != 0 && Symbol != Dummy) {
        uint32_t funcFreq = HotAddresses[(uint32_t) CurPC];
        HotAddresses[(uint32_t) CurPC] = funcFreq + 1;
        if (funcFreq == 100) {
          DebugOut << "\n" << Symbol << " is hot!\n";
        }
    }
#endif
    if (Verbosity > 1 || Verbosity == -1) {
    int SpecFlags = DILineInfoSpecifier::FileLineInfo |
                    DILineInfoSpecifier::AbsoluteFilePath;
    //    if (PrintFunctions)
    //      SpecFlags |= DILineInfoSpecifier::FunctionName;
    //    if (PrintInlining) {
      DIInliningInfo InliningInfo =
        DICtx->getInliningInfoForAddress(CurPC, SpecFlags);
      uint32_t n = InliningInfo.getNumberOfFrames();
      if (n > 0) {
        for (uint32_t i = 0; i < n; i++) {
          DILineInfo dli = InliningInfo.getFrame(i);
          //          PrintDILineInfo(dli);
          if (!dli.getFileName())
            continue;
          uint32_t lineNum = dli.getLine();
          if (dli.getFileName() != LastFileName) {
            if (LastFileName != string()) {
              SourceFile.close();
            }
            LastFileName = dli.getFileName();
            SourceFile.open(LastFileName.c_str(), ios::in);            
          }  else if (lineNum == LastLine) {
            continue;
          }
          if (SourceFile.is_open()) {
            string lineContents;            
            SourceFile.seekg(0, ios::beg);
            for (int i = 0; i < lineNum; ++i) 
              if (!getline (SourceFile, lineContents))
                break;
            if (FullPath)
              DebugOut << "\e[1;35m[" << LastFileName << ":" <<
                lineNum << "]\e[0m  ";
            else
              DebugOut << "\e[1;35m[" << basename(LastFileName.c_str()) 
                       << ":" << lineNum << "]\e[0m  ";
            DebugOut << "\e[0;33m" << lineContents << "\e[0m\n";
            LastLine = lineNum;
          }
        }
      }
    } // end if (Verbosity > 1)
    if (Verbosity > 0) {
      DebugOut << "\e[1;32m[\e[1;37m0x" << format("%04" PRIx64, CurPC) 
               << "\e[1;32m]\e[0m";
    }
#endif
    if (DisAsm->getInstruction(Inst, Size, *mem, CurPC,
                               DebugOut, nulls())) {
      CurPC = IP->executeInstruction(&Inst, CurPC);
      ++numEmulated;
    } else {
      errs() << ToolName << ": warning: invalid instruction encoding\n";
      DumpBytes(StringRef(mem->memory + CurPC, Size));
      exit(1);
      if (Size == 0)
        Size = 1; // skip illegible bytes
    }
  } while (CurPC != 0 && (Cap == 0 || numEmulated < Cap));
  
}

int main(int argc, char **argv) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  // Initialize targets and assembly printers/parsers.
  //
  // No need to initialize the OpenISA target. It is initialized by a global
  // constructor.

  // Register the target printer for --version.
  cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);

  cl::ParseCommandLineOptions(argc, argv, "Open-ISA Static Binary Translator\n");
  //TripleName = Triple::normalize(TripleName);

  ToolName = argv[0];

  // Defaults to a.out if no filenames specified.
  if (InputFilename.size() == 0)
    InputFilename = std::string("a.out");

  ExecutionLoop(InputFilename, argc, argv);

  return 0;
}
