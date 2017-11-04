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

#include "common/linux/dump_symbols.h"
#include "breakpad_wrapper.h"

#define MAX_STACK_FRAME_NUM 10

namespace breakpad_wrapper {

    using namespace google_breakpad;

    static bool dump_call_back(const MinidumpDescriptor &descriptor,
                               void *context,
                               bool succeeded) {
        LOGE("DumpCallback ===> succeeded %d", succeeded);
        return succeeded;
    }

    int init_breakpad(const char *crash_dump_path) {
        MinidumpDescriptor descriptor(crash_dump_path);
        static ExceptionHandler eh(descriptor, NULL, dump_call_back, NULL, true, -1);
        return RES_OK;
    }


    int dump_symbols_file(const char *so_file_path, const char *dump_symbol_path) {

        int RESULT = RES_OK;

        std::ofstream file;
        std::vector<string> debug_dirs;
        file.open(dump_symbol_path);

        DumpOptions options(ALL_SYMBOL_DATA, true);
        if (!WriteSymbolFile(so_file_path, debug_dirs, options, file)) {
            LOGE("Failed to write symbol file.\n");
            RESULT = RES_ERROR;
            goto END;
        }

        END:
        if (file) {
            file.close();
        }
        return RESULT;
    }

    /**
     * malloc and clone the same from src str
     * @param p_src_str source
     * @return dest
     */
    static char *str_clone(const char *p_src_str) {
        LOGD("str_clone src %s", p_src_str);
        size_t size = strlen(p_src_str) * sizeof(char) + 1;
        char *p_dst_str = (char *) malloc(size);
        memcpy(p_dst_str, p_src_str, size);
        return p_dst_str;
    }

    /**
     * fill stack frames data
     * @param stack
     * @param result
     */
    static void fill_stack_frames(IN const CallStack *stack, OUT struct_translate_result *result) {
        int frame_count = stack->frames()->size();
        result->stack_frames_num =
                (MAX_STACK_FRAME_NUM - frame_count) < 0 ? MAX_STACK_FRAME_NUM : frame_count;

        int stack_frame_size = sizeof(struct_stack_frame) * result->stack_frames_num;
        result->p_stack_frames = (struct_stack_frame *) malloc(stack_frame_size);

        memset(result->p_stack_frames, 0, stack_frame_size);


        for (int frame_index = 0; frame_index < result->stack_frames_num; ++frame_index) {
            const StackFrame *frame = stack->frames()->at(frame_index);
            result->p_stack_frames[frame_index].frame_index = frame_index;

            uint64_t instruction_address = frame->ReturnAddress();
            result->p_stack_frames[frame_index].instruction = instruction_address;

            if (frame->module) {
                result->p_stack_frames[frame_index].p_code_file = str_clone(
                        PathnameStripper::File(frame->module->code_file()).c_str());
//                LOGD("%s", PathnameStripper::File(frame->module->code_file()).c_str());
                if (!frame->function_name.empty()) {
                    result->p_stack_frames[frame_index].p_function_name = str_clone(
                            frame->function_name.c_str());
//                    LOGD("!%s", frame->function_name.c_str());
                    if (!frame->source_file_name.empty()) {
//                        string source_file = PathnameStripper::File(frame->source_file_name);
//                        LOGD(" [%s : %d + 0x%" PRIx64 "]",
//                               source_file.c_str(),
//                               frame->source_line,
//                               instruction_address - frame->source_line_base);
                        result->p_stack_frames[frame_index].offset =
                                instruction_address - frame->source_line_base;
                    } else {
                        result->p_stack_frames[frame_index].offset =
                                instruction_address - frame->function_base;
//                        LOGD(" + 0x%" PRIx64, instruction_address - frame->function_base);
                    }
                } else {
                    result->p_stack_frames[frame_index].offset =
                            instruction_address - frame->module->base_address();

//                    LOGD(" + 0x%" PRIx64,
//                           instruction_address - frame->module->base_address());
                }
            } else {
                result->p_stack_frames[frame_index].offset = instruction_address;

//                LOGD("0x%" PRIx64, instruction_address);
            }
            if (result->p_stack_frames[frame_index].p_function_name) {
                LOGD("frame_index:%d p_function_name:%p", frame_index,
                     result->p_stack_frames[frame_index].p_function_name);
            }

            LOGD("\n ");
        }
    }

    struct_translate_result translate_crash_file(const char *crash_file_path,
                                                 const char *symbol_files_dir) {

        struct_translate_result result;
        memset(&result, 0, sizeof(struct_translate_result));

        LOGD("translate_crash_file IN");
        LOGD("translate_crash_file crash_file_path:%s", crash_file_path);
        LOGD("translate_crash_file symbol_files_dir:%s", symbol_files_dir);

        std::vector<string> symbol_paths;
        symbol_paths.push_back(symbol_files_dir);

        scoped_ptr<SimpleSymbolSupplier> symbol_supplier;
        if (!symbol_paths.empty()) {
            symbol_supplier.reset(new SimpleSymbolSupplier(symbol_paths));
        }
        ProcessState process_state;
        BasicSourceLineResolver resolver;

        MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

        // Increase the maximum number of threads and regions.
        MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
        MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
        // Process the minidump.
        Minidump dump(crash_file_path);
        if (!dump.Read()) {
            LOGE("translate_crash_file Minidump could not be read");
            return result;
        }


        if (minidump_processor.Process(&dump, &process_state) != PROCESS_OK) {
            LOGE("translate_crash_file MinidumpProcessor::Process failed");
            return result;
        }

        result.crashed = process_state.crashed();
        /**
         * copy crash info into result
         */
        if (result.crashed) {
            result.crash_address = process_state.crash_address();
            result.p_crash_reason = str_clone((char *) process_state.crash_reason().c_str());
            LOGD("translate_crash_file result.p_crash_reason %s", result.p_crash_reason);

            // If the thread that requested the dump is known, print it first.
            int requesting_thread = process_state.requesting_thread();
            if (requesting_thread != -1) {
                fill_stack_frames(process_state.threads()->at(requesting_thread), &result);

                for (int i = 0; i < result.stack_frames_num; i++) {
                    struct_stack_frame frame = result.p_stack_frames[i];
                    LOGD("%d,%s,0x%"
                                 PRIx64
                                 ",0x%"
                                 PRIx64, frame.frame_index, frame.p_code_file, frame.instruction,
                         frame.offset);
                }
            }
        }

        LOGD("translate_crash_file OUT");

        return result;
    }

}