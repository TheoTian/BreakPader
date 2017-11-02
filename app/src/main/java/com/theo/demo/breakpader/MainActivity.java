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

package com.theo.demo.breakpader;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.theo.breakpader.NativeBreakpader;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;


public class MainActivity extends AppCompatActivity {

    String symbolFileRoot;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
        symbolFileRoot = MainActivity.this.getExternalCacheDir().getAbsolutePath() + "/symbols/";
    }

    private void initView() {
        findViewById(R.id.btInit).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String dumpDir = getExternalCacheDir().getAbsolutePath() + "/dump/";
                new File(dumpDir).mkdirs();
                NativeBreakpader.init(dumpDir);
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
                String symbolsFilePath = MainActivity.this.getExternalCacheDir().getAbsolutePath() + "/libbreakpader.so.sym";
                File symbolsFile = new File(symbolsFilePath);
                if (symbolsFile.exists()) {
                    symbolsFile.delete();
                }
                NativeBreakpader.dumpSymbolFile(MainActivity.this.getApplicationInfo().dataDir + "/lib/libbreakpader.so", symbolsFilePath);
                try {
                    BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(symbolsFile)));
                    String line = null;
                    String moduleLine = null;

                    while ((line = reader.readLine()) != null) {
                        if (line.startsWith("MODULE")) {
                            moduleLine = line;
                        }
                    }

                    if (moduleLine == null) {
                        return;
                    }

                    String[] modules = moduleLine.split("\\s");
                    String os = null;
                    String architecture = null;
                    String id = null;
                    String name = null;
                    if (modules.length == 5) {
                        os = modules[1];
                        architecture = modules[2];
                        id = modules[3];
                        name = modules[4];
                    }


                    File symbolDir = new File(symbolFileRoot + name + "/" + id);
                    symbolDir.mkdirs();

                    symbolsFile.renameTo(new File(symbolDir.getAbsolutePath() + "/" + symbolsFile.getName()));

                } catch (Exception e) {
                    e.printStackTrace();
                }

                Log.d("test", "dumpSymbolFile cost:" + (System.currentTimeMillis() - start));
            }
        });

        findViewById(R.id.btTranslateDump).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                long start = System.currentTimeMillis();
                File dumpDir = new File(MainActivity.this.getExternalCacheDir().getAbsolutePath() + "/dump/");
                if (dumpDir.list().length <= 0) {
                    return;
                }
                NativeBreakpader.ProcessResult processResult = NativeBreakpader.translateCrashFile(dumpDir.listFiles()[0].getAbsolutePath(), symbolFileRoot);
                Log.d("test", "processResult:" + processResult.crashed + "," + processResult.crash_reason + "," + processResult.crash_address);
                Log.d("test", "translateCrashFile cost:" + (System.currentTimeMillis() - start));
            }
        });
    }
}
