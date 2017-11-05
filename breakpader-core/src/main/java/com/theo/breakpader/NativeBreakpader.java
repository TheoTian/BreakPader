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

package com.theo.breakpader;

/**
 * Author: theotian
 * Date: 17/10/30
 * Describe:
 */
public class NativeBreakpader {
    static {
        System.loadLibrary("breakpader");
    }

    /**
     * API
     */

    /**
     * init dump client
     *
     * @param crash_dump_dir crash dump directory
     * @return result code
     */
    public static native int init(String crash_dump_dir);

    /**
     * dump so symbol file
     *
     * @param soPath so file path
     * @param saveSyms save Symbol File Path
     * @return result code
     */
    public static native int dumpSymbolFile(String soPath, String saveSyms);

    /**
     * translate dump file to people readable content
     *
     * @param crashFilePath
     * @param symbolFilesDir
     * @return process result
     */
    public static native ProcessResult translateCrashFile(String crashFilePath, String symbolFilesDir);

    /**
     * test
     */
    /**
     * test Crash
     * @return
     */
    public static native int testCrash();

    public static native String testJNI();
}
