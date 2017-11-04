/*
 *    Copyright 2017, theotian
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef BREAKPADER_BREAKPAD_WRAPPER_H
#define BREAKPADER_BREAKPAD_WRAPPER_H

#include <fstream>
#include <limits>

#include "base/common.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"
#include "common/linux/dump_symbols.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"
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

namespace breakpad_wrapper {

    struct struct_stack_frame {
        int frame_index;
        char *p_code_file;
        char *p_function_name;
        uint64_t instruction;
        uint64_t offset;
    };

    struct struct_translate_result {
        bool crashed;
        char *p_crash_reason;
        uint64_t crash_address;
        struct_stack_frame *p_stack_frames;
        int stack_frames_num;
    };

    int init_breakpad(const char *crash_dump_path);

    int dump_symbols_file(const char *so_file_path, const char *dump_symbol_path);

    struct_translate_result translate_crash_file(const char *crash_file_path,
                                          const char *symbol_files_dir);
}

#endif //BREAKPADER_BREAKPAD_WRAPPER_H
