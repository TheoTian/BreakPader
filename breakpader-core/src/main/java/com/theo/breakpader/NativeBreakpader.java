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


    public static native int init(String dump_path);

    public static native int testCrash();

    public static native String testJNI();
}
