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
 * Date: 17/11/3
 * Describe:
 */
public class ProcessResult {
    public boolean crashed;
    public String crash_reason;
    public long crash_address;
    public StackFrame[] crash_stack_frames;

    public static class StackFrame {
        public int frame_index;
        public String code_file;
        public long instruction;
        public String function_name;//fill this in the future
        public long offset;
    }
}