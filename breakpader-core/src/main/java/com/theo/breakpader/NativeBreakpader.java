/*
 *    Copyright 2017, $user
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

    public static class ProcessResult {

    }

    /**
     * API
     */

    /**
     * init dump client
     *
     * @param dump_path
     * @return result code
     */
    public static native int init(String dump_path);

    /**
     * dump so symbol file
     *
     * @param soPath
     * @param saveSyms
     * @return result code
     */
    public static native int dumpSymbolFile(String soPath, String saveSyms);

    /**
     * translate dump file to people readable content
     *
     * @param dumpFilePath
     * @param symbolFilesDir
     * @return process result
     */
    public static native ProcessResult translateDumpFile(String dumpFilePath, String symbolFilesDir);

    /**
     * test
     */
    public static native int testCrash();

    public static native String testJNI();
}
