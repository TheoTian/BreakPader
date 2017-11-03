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
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.theo.breakpader.NativeBreakpader;
import com.theo.breakpader.ProcessResult;
import com.theo.demo.breakpader.global.MyApplication;

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
                /**
                 * these may cost some time. you can try to call on sub thread.
                 */
                MyApplication.getBreakpader().init();
                MyApplication.getBreakpader().symbol(new File(MainActivity.this.getApplicationInfo().dataDir + "/lib/").listFiles());
            }
        });

        findViewById(R.id.btTestCrash).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                NativeBreakpader.testCrash();
            }
        });

        ListView listView = (ListView) findViewById(R.id.lvCrashList);
        final File[] filelist = new File(MyApplication.getBreakpader().getCrashRootDir()).listFiles();
        listView.setAdapter((new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1, new File(MyApplication.getBreakpader().getCrashRootDir()).list())));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                translate(filelist[position].getAbsolutePath());
            }
        });
    }

    private void translate(String crashFilePath) {
        ProcessResult processResult = MyApplication.getBreakpader().translate(crashFilePath);
        String result = "";
        result += "crashed:" + processResult.crashed + "\n";
        result += "crash_reason:" + processResult.crash_reason + "\n";
        result += "crash_address:" + processResult.crash_address + "\n\n";
        if (processResult.crash_stack_frames != null) {
            for (ProcessResult.StackFrame stackFrame : processResult.crash_stack_frames) {
                result += stackFrame.frame_index + " " + stackFrame.instruction
                        + " " + stackFrame.code_file + " + " + stackFrame.offset + "\n";
            }
        }
        ((TextView) findViewById(R.id.tvResult)).setText(result);
    }
}
