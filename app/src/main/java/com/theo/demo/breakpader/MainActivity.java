package com.theo.demo.breakpader;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.theo.breakpader.NativeBreakpader;

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
    }
}
