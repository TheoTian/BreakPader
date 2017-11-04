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

package com.theo.demo.breakpader.global;

import android.app.Application;

import com.theo.breakpader.Breakpader;

/**
 * Author: theotian
 * Date: 17/11/3
 * Describe:
 */

public class MyApplication extends Application {

    private static Breakpader sBreakpaderInstance;

    @Override
    public void onCreate() {
        super.onCreate();
        //new the instance and set the root directory to dump
        sBreakpaderInstance = new Breakpader(getExternalCacheDir() + "/breakpader/");
    }

    public static Breakpader getBreakpader() {
        return sBreakpaderInstance;
    }
}
