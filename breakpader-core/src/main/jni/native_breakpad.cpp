#include <common/linux/dump_symbols.h>
#include "base/common.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"
#include <fstream>
#include <limits>

#include "common/path_helper.h"
#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/source_line_resolver_base.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "processor/logging.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"

#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/pathname_stripper.h"

//using google_breakpad::WriteSymbolFile;
//using google_breakpad::WriteSymbolFileHeader;
//using google_breakpad::BasicSourceLineResolver;
//using google_breakpad::Minidump;
//using google_breakpad::MinidumpMemoryList;
//using google_breakpad::MinidumpThreadList;
//using google_breakpad::MinidumpProcessor;
//using google_breakpad::ProcessState;
//using google_breakpad::SimpleSymbolSupplier;
//using google_breakpad::scoped_ptr;

using namespace google_breakpad;

static char *class_nativebreakpader = "com/theo/breakpader/NativeBreakpader";


/**
 * API ZONE
 */

bool DumpCallback(const MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {
    LOGE("DumpCallback ===> succeeded %d", succeeded);
    return succeeded;
}

JNIEXPORT jint JNICALL Init(JNIEnv *env,
                            jobject jobj,
                            jstring crash_dump_path) {
    const char *path = (char *) env->GetStringUTFChars(crash_dump_path, NULL);
    MinidumpDescriptor descriptor(path);
    static ExceptionHandler eh(descriptor, NULL, DumpCallback, NULL, true, -1);
    env->ReleaseStringUTFChars(crash_dump_path, path);
    LOGD("nativeInit ===> breakpad initialized succeeded, dump file will be saved at %s", path);
    return 0;
}

JNIEXPORT jint JNICALL DumpSymbolsFile(JNIEnv *env, jobject jobj, jstring jso_file_path,
                                       jstring jsymbol_file_path) {
    jint RESULT = JNI_OK;
    const char *so_file_path = (char *) env->GetStringUTFChars(jso_file_path, NULL);
    const char *symbol_file_path = (char *) env->GetStringUTFChars(jsymbol_file_path, NULL);

    std::ofstream file;
    std::vector<string> debug_dirs;
    file.open(symbol_file_path);

    DumpOptions options(ALL_SYMBOL_DATA, true);
    if (!WriteSymbolFile(so_file_path, debug_dirs, options, file)) {
        LOGE("Failed to write symbol file.\n");
        RESULT = JNI_ERR;
        goto END;
    }

    RESULT = JNI_OK;

    END:
    file.close();
    env->ReleaseStringUTFChars(jso_file_path, so_file_path);
    env->ReleaseStringUTFChars(jsymbol_file_path, symbol_file_path);

    return RESULT;
}
// Separator character for machine readable output.
static const char kOutputSeparator = '|';

// PrintRegister prints a register's name and value to stdout.  It will
// print four registers on a line.  For the first register in a set,
// pass 0 for |start_col|.  For registers in a set, pass the most recent
// return value of PrintRegister.
// The caller is responsible for printing the final newline after a set
// of registers is completely printed, regardless of the number of calls
// to PrintRegister.
static const int kMaxWidth = 80;  // optimize for an 80-column terminal

static int PrintRegister(const char *name, uint32_t value, int start_col) {
    char buffer[64];
    LOGD(buffer, sizeof(buffer), " %5s = 0x%08x", name, value);

    if (start_col + static_cast<ssize_t>(strlen(buffer)) > kMaxWidth) {
        start_col = 0;
        LOGD("\n ");
    }
    fputs(buffer, stdout);

    return start_col + strlen(buffer);
}

// PrintRegister64 does the same thing, but for 64-bit registers.
static int PrintRegister64(const char *name, uint64_t value, int start_col) {
    char buffer[64];
    LOGD(buffer, sizeof(buffer), " %5s = 0x%016" PRIx64 , name, value);

    if (start_col + static_cast<ssize_t>(strlen(buffer)) > kMaxWidth) {
        start_col = 0;
        LOGD("\n ");
    }
    fputs(buffer, stdout);

    return start_col + strlen(buffer);
}

// StripSeparator takes a string |original| and returns a copy
// of the string with all occurences of |kOutputSeparator| removed.
static string StripSeparator(const string &original) {
    string result = original;
    string::size_type position = 0;
    while ((position = result.find(kOutputSeparator, position)) != string::npos) {
        result.erase(position, 1);
    }
    position = 0;
    while ((position = result.find('\n', position)) != string::npos) {
        result.erase(position, 1);
    }
    return result;
}


// PrintStackContents prints the stack contents of the current frame to stdout.
static void PrintStackContents(const string &indent,
                               const StackFrame *frame,
                               const StackFrame *prev_frame,
                               const string &cpu,
                               const MemoryRegion *memory,
                               const CodeModules* modules,
                               SourceLineResolverInterface *resolver) {
    // Find stack range.
    int word_length = 0;
    uint64_t stack_begin = 0, stack_end = 0;
    if (cpu == "x86") {
        word_length = 4;
        const StackFrameX86 *frame_x86 = static_cast<const StackFrameX86*>(frame);
        const StackFrameX86 *prev_frame_x86 =
                static_cast<const StackFrameX86*>(prev_frame);
        if ((frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESP) &&
            (prev_frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESP)) {
            stack_begin = frame_x86->context.esp;
            stack_end = prev_frame_x86->context.esp;
        }
    } else if (cpu == "amd64") {
        word_length = 8;
        const StackFrameAMD64 *frame_amd64 =
                static_cast<const StackFrameAMD64*>(frame);
        const StackFrameAMD64 *prev_frame_amd64 =
                static_cast<const StackFrameAMD64*>(prev_frame);
        if ((frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RSP) &&
            (prev_frame_amd64->context_validity &
             StackFrameAMD64::CONTEXT_VALID_RSP)) {
            stack_begin = frame_amd64->context.rsp;
            stack_end = prev_frame_amd64->context.rsp;
        }
    } else if (cpu == "arm") {
        word_length = 4;
        const StackFrameARM *frame_arm = static_cast<const StackFrameARM*>(frame);
        const StackFrameARM *prev_frame_arm =
                static_cast<const StackFrameARM*>(prev_frame);
        if ((frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_SP) &&
            (prev_frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_SP)) {
            stack_begin = frame_arm->context.iregs[13];
            stack_end = prev_frame_arm->context.iregs[13];
        }
    } else if (cpu == "arm64") {
        word_length = 8;
        const StackFrameARM64 *frame_arm64 =
                static_cast<const StackFrameARM64*>(frame);
        const StackFrameARM64 *prev_frame_arm64 =
                static_cast<const StackFrameARM64*>(prev_frame);
        if ((frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_SP) &&
            (prev_frame_arm64->context_validity &
             StackFrameARM64::CONTEXT_VALID_SP)) {
            stack_begin = frame_arm64->context.iregs[31];
            stack_end = prev_frame_arm64->context.iregs[31];
        }
    }
    if (!word_length || !stack_begin || !stack_end)
        return;

    // Print stack contents.
    LOGD("\n%sStack contents:", indent.c_str());
    for(uint64_t address = stack_begin; address < stack_end; ) {
        // Print the start address of this row.
        if (word_length == 4)
            LOGD("\n%s %08x", indent.c_str(), static_cast<uint32_t>(address));
        else
            LOGD("\n%s %016" PRIx64, indent.c_str(), address);

        // Print data in hex.
        const int kBytesPerRow = 16;
        string data_as_string;
        for (int i = 0; i < kBytesPerRow; ++i, ++address) {
            uint8_t value = 0;
            if (address < stack_end &&
                memory->GetMemoryAtAddress(address, &value)) {
                LOGD(" %02x", value);
                data_as_string.push_back(isprint(value) ? value : '.');
            } else {
                LOGD("   ");
                data_as_string.push_back(' ');
            }
        }
        // Print data as string.
        LOGD("  %s", data_as_string.c_str());
    }

    // Try to find instruction pointers from stack.
    LOGD("\n%sPossible instruction pointers:\n", indent.c_str());
    for (uint64_t address = stack_begin; address < stack_end;
         address += word_length) {
        StackFrame pointee_frame;

        // Read a word (possible instruction pointer) from stack.
        if (word_length == 4) {
            uint32_t data32 = 0;
            memory->GetMemoryAtAddress(address, &data32);
            pointee_frame.instruction = data32;
        } else {
            uint64_t data64 = 0;
            memory->GetMemoryAtAddress(address, &data64);
            pointee_frame.instruction = data64;
        }
        pointee_frame.module =
                modules->GetModuleForAddress(pointee_frame.instruction);

        // Try to look up the function name.
        if (pointee_frame.module)
            resolver->FillSourceLineInfo(&pointee_frame);

        // Print function name.
        if (!pointee_frame.function_name.empty()) {
            if (word_length == 4) {
                LOGD("%s *(0x%08x) = 0x%08x", indent.c_str(),
                       static_cast<uint32_t>(address),
                       static_cast<uint32_t>(pointee_frame.instruction));
            } else {
                LOGD("%s *(0x%016" PRIx64 ") = 0x%016" PRIx64,
                       indent.c_str(), address, pointee_frame.instruction);
            }
            LOGD(" <%s> [%s : %d + 0x%" PRIx64 "]\n",
                   pointee_frame.function_name.c_str(),
                   PathnameStripper::File(pointee_frame.source_file_name).c_str(),
                   pointee_frame.source_line,
                   pointee_frame.instruction - pointee_frame.source_line_base);
        }
    }
    LOGD("\n");
}

static void PrintStack(const CallStack *stack,
                       const string &cpu,
                       bool output_stack_contents,
                       const MemoryRegion* memory,
                       const CodeModules* modules,
                       SourceLineResolverInterface* resolver) {
    int frame_count = stack->frames()->size();
    if (frame_count == 0) {
        LOGD(" <no frames>\n");
    }
    for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
        const StackFrame *frame = stack->frames()->at(frame_index);
        LOGD("%2d  ", frame_index);

        uint64_t instruction_address = frame->ReturnAddress();

        if (frame->module) {
            LOGD("%s", PathnameStripper::File(frame->module->code_file()).c_str());
            if (!frame->function_name.empty()) {
                LOGD("!%s", frame->function_name.c_str());
                if (!frame->source_file_name.empty()) {
                    string source_file = PathnameStripper::File(frame->source_file_name);
                    LOGD(" [%s : %d + 0x%" PRIx64 "]",
                           source_file.c_str(),
                           frame->source_line,
                           instruction_address - frame->source_line_base);
                } else {
                    LOGD(" + 0x%" PRIx64, instruction_address - frame->function_base);
                }
            } else {
                LOGD(" + 0x%" PRIx64,
                       instruction_address - frame->module->base_address());
            }
        } else {
            LOGD("0x%" PRIx64, instruction_address);
        }
        LOGD("\n ");

        int sequence = 0;
        if (cpu == "x86") {
            const StackFrameX86 *frame_x86 =
                    reinterpret_cast<const StackFrameX86*>(frame);

            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EIP)
                sequence = PrintRegister("eip", frame_x86->context.eip, sequence);
            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESP)
                sequence = PrintRegister("esp", frame_x86->context.esp, sequence);
            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EBP)
                sequence = PrintRegister("ebp", frame_x86->context.ebp, sequence);
            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EBX)
                sequence = PrintRegister("ebx", frame_x86->context.ebx, sequence);
            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESI)
                sequence = PrintRegister("esi", frame_x86->context.esi, sequence);
            if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EDI)
                sequence = PrintRegister("edi", frame_x86->context.edi, sequence);
            if (frame_x86->context_validity == StackFrameX86::CONTEXT_VALID_ALL) {
                sequence = PrintRegister("eax", frame_x86->context.eax, sequence);
                sequence = PrintRegister("ecx", frame_x86->context.ecx, sequence);
                sequence = PrintRegister("edx", frame_x86->context.edx, sequence);
                sequence = PrintRegister("efl", frame_x86->context.eflags, sequence);
            }
        } else if (cpu == "ppc") {
            const StackFramePPC *frame_ppc =
                    reinterpret_cast<const StackFramePPC*>(frame);

            if (frame_ppc->context_validity & StackFramePPC::CONTEXT_VALID_SRR0)
                sequence = PrintRegister("srr0", frame_ppc->context.srr0, sequence);
            if (frame_ppc->context_validity & StackFramePPC::CONTEXT_VALID_GPR1)
                sequence = PrintRegister("r1", frame_ppc->context.gpr[1], sequence);
        } else if (cpu == "amd64") {
            const StackFrameAMD64 *frame_amd64 =
                    reinterpret_cast<const StackFrameAMD64*>(frame);

            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RAX)
                sequence = PrintRegister64("rax", frame_amd64->context.rax, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RDX)
                sequence = PrintRegister64("rdx", frame_amd64->context.rdx, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RCX)
                sequence = PrintRegister64("rcx", frame_amd64->context.rcx, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RBX)
                sequence = PrintRegister64("rbx", frame_amd64->context.rbx, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RSI)
                sequence = PrintRegister64("rsi", frame_amd64->context.rsi, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RDI)
                sequence = PrintRegister64("rdi", frame_amd64->context.rdi, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RBP)
                sequence = PrintRegister64("rbp", frame_amd64->context.rbp, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RSP)
                sequence = PrintRegister64("rsp", frame_amd64->context.rsp, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R8)
                sequence = PrintRegister64("r8", frame_amd64->context.r8, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R9)
                sequence = PrintRegister64("r9", frame_amd64->context.r9, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R10)
                sequence = PrintRegister64("r10", frame_amd64->context.r10, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R11)
                sequence = PrintRegister64("r11", frame_amd64->context.r11, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R12)
                sequence = PrintRegister64("r12", frame_amd64->context.r12, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R13)
                sequence = PrintRegister64("r13", frame_amd64->context.r13, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R14)
                sequence = PrintRegister64("r14", frame_amd64->context.r14, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R15)
                sequence = PrintRegister64("r15", frame_amd64->context.r15, sequence);
            if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RIP)
                sequence = PrintRegister64("rip", frame_amd64->context.rip, sequence);
        } else if (cpu == "sparc") {
            const StackFrameSPARC *frame_sparc =
                    reinterpret_cast<const StackFrameSPARC*>(frame);

            if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_SP)
                sequence = PrintRegister("sp", frame_sparc->context.g_r[14], sequence);
            if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_FP)
                sequence = PrintRegister("fp", frame_sparc->context.g_r[30], sequence);
            if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_PC)
                sequence = PrintRegister("pc", frame_sparc->context.pc, sequence);
        } else if (cpu == "arm") {
            const StackFrameARM *frame_arm =
                    reinterpret_cast<const StackFrameARM*>(frame);

            // Argument registers (caller-saves), which will likely only be valid
            // for the youngest frame.
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R0)
                sequence = PrintRegister("r0", frame_arm->context.iregs[0], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R1)
                sequence = PrintRegister("r1", frame_arm->context.iregs[1], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R2)
                sequence = PrintRegister("r2", frame_arm->context.iregs[2], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R3)
                sequence = PrintRegister("r3", frame_arm->context.iregs[3], sequence);

            // General-purpose callee-saves registers.
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R4)
                sequence = PrintRegister("r4", frame_arm->context.iregs[4], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R5)
                sequence = PrintRegister("r5", frame_arm->context.iregs[5], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R6)
                sequence = PrintRegister("r6", frame_arm->context.iregs[6], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R7)
                sequence = PrintRegister("r7", frame_arm->context.iregs[7], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R8)
                sequence = PrintRegister("r8", frame_arm->context.iregs[8], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R9)
                sequence = PrintRegister("r9", frame_arm->context.iregs[9], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R10)
                sequence = PrintRegister("r10", frame_arm->context.iregs[10], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R12)
                sequence = PrintRegister("r12", frame_arm->context.iregs[12], sequence);

            // Registers with a dedicated or conventional purpose.
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_FP)
                sequence = PrintRegister("fp", frame_arm->context.iregs[11], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_SP)
                sequence = PrintRegister("sp", frame_arm->context.iregs[13], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_LR)
                sequence = PrintRegister("lr", frame_arm->context.iregs[14], sequence);
            if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_PC)
                sequence = PrintRegister("pc", frame_arm->context.iregs[15], sequence);
        } else if (cpu == "arm64") {
            const StackFrameARM64 *frame_arm64 =
                    reinterpret_cast<const StackFrameARM64*>(frame);

            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X0) {
                sequence =
                        PrintRegister64("x0", frame_arm64->context.iregs[0], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X1) {
                sequence =
                        PrintRegister64("x1", frame_arm64->context.iregs[1], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X2) {
                sequence =
                        PrintRegister64("x2", frame_arm64->context.iregs[2], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X3) {
                sequence =
                        PrintRegister64("x3", frame_arm64->context.iregs[3], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X4) {
                sequence =
                        PrintRegister64("x4", frame_arm64->context.iregs[4], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X5) {
                sequence =
                        PrintRegister64("x5", frame_arm64->context.iregs[5], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X6) {
                sequence =
                        PrintRegister64("x6", frame_arm64->context.iregs[6], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X7) {
                sequence =
                        PrintRegister64("x7", frame_arm64->context.iregs[7], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X8) {
                sequence =
                        PrintRegister64("x8", frame_arm64->context.iregs[8], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X9) {
                sequence =
                        PrintRegister64("x9", frame_arm64->context.iregs[9], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X10) {
                sequence =
                        PrintRegister64("x10", frame_arm64->context.iregs[10], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X11) {
                sequence =
                        PrintRegister64("x11", frame_arm64->context.iregs[11], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X12) {
                sequence =
                        PrintRegister64("x12", frame_arm64->context.iregs[12], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X13) {
                sequence =
                        PrintRegister64("x13", frame_arm64->context.iregs[13], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X14) {
                sequence =
                        PrintRegister64("x14", frame_arm64->context.iregs[14], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X15) {
                sequence =
                        PrintRegister64("x15", frame_arm64->context.iregs[15], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X16) {
                sequence =
                        PrintRegister64("x16", frame_arm64->context.iregs[16], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X17) {
                sequence =
                        PrintRegister64("x17", frame_arm64->context.iregs[17], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X18) {
                sequence =
                        PrintRegister64("x18", frame_arm64->context.iregs[18], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X19) {
                sequence =
                        PrintRegister64("x19", frame_arm64->context.iregs[19], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X20) {
                sequence =
                        PrintRegister64("x20", frame_arm64->context.iregs[20], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X21) {
                sequence =
                        PrintRegister64("x21", frame_arm64->context.iregs[21], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X22) {
                sequence =
                        PrintRegister64("x22", frame_arm64->context.iregs[22], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X23) {
                sequence =
                        PrintRegister64("x23", frame_arm64->context.iregs[23], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X24) {
                sequence =
                        PrintRegister64("x24", frame_arm64->context.iregs[24], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X25) {
                sequence =
                        PrintRegister64("x25", frame_arm64->context.iregs[25], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X26) {
                sequence =
                        PrintRegister64("x26", frame_arm64->context.iregs[26], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X27) {
                sequence =
                        PrintRegister64("x27", frame_arm64->context.iregs[27], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_X28) {
                sequence =
                        PrintRegister64("x28", frame_arm64->context.iregs[28], sequence);
            }

            // Registers with a dedicated or conventional purpose.
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_FP) {
                sequence =
                        PrintRegister64("fp", frame_arm64->context.iregs[29], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_LR) {
                sequence =
                        PrintRegister64("lr", frame_arm64->context.iregs[30], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_SP) {
                sequence =
                        PrintRegister64("sp", frame_arm64->context.iregs[31], sequence);
            }
            if (frame_arm64->context_validity & StackFrameARM64::CONTEXT_VALID_PC) {
                sequence =
                        PrintRegister64("pc", frame_arm64->context.iregs[32], sequence);
            }
        } else if ((cpu == "mips") || (cpu == "mips64")) {
            const StackFrameMIPS* frame_mips =
                    reinterpret_cast<const StackFrameMIPS*>(frame);

            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_GP)
                sequence = PrintRegister64("gp",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_GP],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_SP)
                sequence = PrintRegister64("sp",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_SP],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_FP)
                sequence = PrintRegister64("fp",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_FP],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_RA)
                sequence = PrintRegister64("ra",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_RA],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_PC)
                sequence = PrintRegister64("pc", frame_mips->context.epc, sequence);

            // Save registers s0-s7
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S0)
                sequence = PrintRegister64("s0",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S0],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S1)
                sequence = PrintRegister64("s1",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S1],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S2)
                sequence = PrintRegister64("s2",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S2],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S3)
                sequence = PrintRegister64("s3",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S3],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S4)
                sequence = PrintRegister64("s4",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S4],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S5)
                sequence = PrintRegister64("s5",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S5],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S6)
                sequence = PrintRegister64("s6",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S6],
                                           sequence);
            if (frame_mips->context_validity & StackFrameMIPS::CONTEXT_VALID_S7)
                sequence = PrintRegister64("s7",
                                           frame_mips->context.iregs[MD_CONTEXT_MIPS_REG_S7],
                                           sequence);
        }
        LOGD("\n    Found by: %s\n", frame->trust_description().c_str());
// Print stack contents.
        if (output_stack_contents && frame_index + 1 < frame_count) {
            const string indent("    ");
            PrintStackContents(indent, frame, stack->frames()->at(frame_index + 1),
                               cpu, memory, modules, resolver);
        }
    }
}

JNIEXPORT jobject JNICALL TranslateDumpFile(JNIEnv *env, jobject jobj, jstring jdump_file_path,
                                            jstring jsymbol_files_dir) {

    const char *dump_file_path = (char *) env->GetStringUTFChars(jdump_file_path, NULL);
    const char *symbol_files_dir = (char *) env->GetStringUTFChars(jsymbol_files_dir, NULL);
    LOGD("TranslateDumpFile IN");
    LOGD("TranslateDumpFile dump_file_path:%s",dump_file_path);
    LOGD("TranslateDumpFile symbol_files_dir:%s",symbol_files_dir);

    std::vector<string> symbol_paths;
    symbol_paths.push_back(symbol_files_dir);

    scoped_ptr <SimpleSymbolSupplier> symbol_supplier;
    if (!symbol_paths.empty()) {
        // TODO(mmentovai): check existence of symbol_path if specified?
        symbol_supplier.reset(new SimpleSymbolSupplier(symbol_paths));
    }
    ProcessState process_state;
    BasicSourceLineResolver resolver;

    MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

    // Increase the maximum number of threads and regions.
    MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
    MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
    // Process the minidump.
    Minidump dump(dump_file_path);
    if (!dump.Read()) {
        LOGD("Minidump could not be read");
        return NULL;
    }


    if (minidump_processor.Process(&dump, &process_state) != PROCESS_OK) {
        LOGD("MinidumpProcessor::Process failed");
        return NULL;
    }

    // Print OS and CPU information.
    string cpu = process_state.system_info()->cpu;
    string cpu_info = process_state.system_info()->cpu_info;
    LOGD("Operating system: %s\n", process_state.system_info()->os.c_str());
    LOGD("                  %s\n",
           process_state.system_info()->os_version.c_str());
    LOGD("CPU: %s\n", cpu.c_str());
    if (!cpu_info.empty()) {
        // This field is optional.
        LOGD("     %s\n", cpu_info.c_str());
    }
    LOGD("     %d CPU%s\n",
           process_state.system_info()->cpu_count,
           process_state.system_info()->cpu_count != 1 ? "s" : "");
    LOGD("\n");

    // Print GPU information
    string gl_version = process_state.system_info()->gl_version;
    string gl_vendor = process_state.system_info()->gl_vendor;
    string gl_renderer = process_state.system_info()->gl_renderer;
    LOGD("GPU:");
    if (!gl_version.empty() || !gl_vendor.empty() || !gl_renderer.empty()) {
        LOGD(" %s\n", gl_version.c_str());
        LOGD("     %s\n", gl_vendor.c_str());
        LOGD("     %s\n", gl_renderer.c_str());
    } else {
        LOGD(" UNKNOWN\n");
    }
    LOGD("\n");

    // Print crash information.
    if (process_state.crashed()) {
        LOGD("Crash reason:  %s\n", process_state.crash_reason().c_str());
        LOGD("Crash address: 0x%" PRIx64 "\n", process_state.crash_address());
    } else {
        LOGD("No crash\n");
    }

    string assertion = process_state.assertion();
    if (!assertion.empty()) {
        LOGD("Assertion: %s\n", assertion.c_str());
    }

    // Compute process uptime if the process creation and crash times are
    // available in the dump.
    if (process_state.time_date_stamp() != 0 &&
        process_state.process_create_time() != 0 &&
        process_state.time_date_stamp() >= process_state.process_create_time()) {
        LOGD("Process uptime: %d seconds\n",
               process_state.time_date_stamp() -
               process_state.process_create_time());
    } else {
        LOGD("Process uptime: not available\n");
    }

    // If the thread that requested the dump is known, print it first.
    int requesting_thread = process_state.requesting_thread();
    if (requesting_thread != -1) {
        LOGD("\n");
        LOGD("Thread %d (%s)\n",
               requesting_thread,
               process_state.crashed() ? "crashed" :
               "requested dump, did not crash");
        PrintStack(process_state.threads()->at(requesting_thread), cpu,
                   false,
                   process_state.thread_memory_regions()->at(requesting_thread),
                   process_state.modules(), &resolver);
    }

    // Print all of the threads in the dump.
    int thread_count = process_state.threads()->size();
    for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
        if (thread_index != requesting_thread) {
            // Don't print the crash thread again, it was already printed.
            LOGD("\n");
            LOGD("Thread %d\n", thread_index);
        }
    }

    END:
    env->ReleaseStringUTFChars(jdump_file_path, dump_file_path);
    env->ReleaseStringUTFChars(jsymbol_files_dir, symbol_files_dir);
    LOGD("TranslateDumpFile OUT");
    return NULL;
}


/**
 * TEST ZONE
 */

JNIEXPORT jstring JNICALL TestJNI(JNIEnv *env, jobject jobj) {
    char *str = "Test JNI";
    return env->NewStringUTF(str);
}

JNIEXPORT jint JNICALL TestCrash(JNIEnv *env, jobject jobj) {
    LOGE("native crash capture begin");
//    char *str = NULL;
//    env->NewStringUTF(str);
//
//    int i = 0;
//    int test = 100 / i;
//    LOGE("native crash capture end %d", test);
    int* i = NULL;
    *i = 1;
    return 0;
}

static JNINativeMethod nativeMethods[] = {
        {"testJNI",           "()Ljava/lang/String;",                                                                      (void *) TestJNI},
        {"init",              "(Ljava/lang/String;)I",                                                                     (void *) Init},
        {"testCrash",         "()I",                                                                                       (void *) TestCrash},
        {"dumpSymbolFile",    "(Ljava/lang/String;Ljava/lang/String;)I",                                                   (void *) DumpSymbolsFile},
        {"translateDumpFile", "(Ljava/lang/String;Ljava/lang/String;)Lcom/theo/breakpader/NativeBreakpader$ProcessResult;", (void *) TranslateDumpFile},
};

JNIEXPORT int JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass javaClass = env->FindClass(class_nativebreakpader);
    if (javaClass == NULL) {
        return JNI_ERR;
    }
    if (env->RegisterNatives(javaClass, nativeMethods,
                             sizeof(nativeMethods) / sizeof(nativeMethods[0])) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
