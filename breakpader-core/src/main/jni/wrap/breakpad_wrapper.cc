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
    static char *str_clone(char *p_src_str) {
        LOGD("str_clone src %s", p_src_str);
        size_t size = strlen(p_src_str) * sizeof(char) + 1;
        char *p_dst_str = (char *) malloc(size);
        memcpy(p_dst_str, p_src_str, size);
        return p_dst_str;
    }

    translate_result translate_crash_file(const char *crash_file_path,
                                          const char *symbol_files_dir) {

        translate_result result;

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
            result.crash_reason = str_clone((char *) process_state.crash_reason().c_str());
            LOGD("translate_crash_file result.crash_reason %s", result.crash_reason);
        }

        LOGD("translate_crash_file OUT");

        return result;
    }

}