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

package com.theo.demo.breakpader;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.theo.breakpader.NativeBreakpader;

import java.io.File;


public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
    }

    private void initView() {
        findViewById(R.id.btInit).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                NativeBreakpader.init(getExternalCacheDir().getAbsolutePath());
            }
        });

        findViewById(R.id.btTestCrash).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                NativeBreakpader.testCrash();
            }
        });

        findViewById(R.id.btDumpSyms).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                long start = System.currentTimeMillis();
                String symbolsFilePath = MainActivity.this.getExternalCacheDir().getAbsolutePath() + "/libbreakpader.so.sysm";
                File symbolsFile = new File(symbolsFilePath);
                if (symbolsFile.exists()) {
                    symbolsFile.delete();
                }
                NativeBreakpader.dumpSymbolFile(MainActivity.this.getApplicationInfo().dataDir + "/lib/libbreakpader.so", symbolsFilePath);
                Log.d("test", "dumpSymbolFile cost:" + (System.currentTimeMillis() - start));
            }
        });
    }
}
