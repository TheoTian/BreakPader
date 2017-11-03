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

import com.theo.breakpader.utils.FileUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Author: theotian
 * Date: 17/11/3
 * Describe:
 */

public class Breakpader {

    public static final int OK = 0;
    public static final int ERROR = -1;

    private String mRootDir;
    private String mCrashRootDir;//dump crash file root directory
    private String mSymbolRootDir;//dump symbol file root directory

    public Breakpader(String rootDir) {
        this(rootDir, rootDir + "/crash/", rootDir + "/symbol/");
    }

    public Breakpader(String rootDir, String crashRootDir, String symbolRootDir) {
        this.mRootDir = rootDir;
        this.mCrashRootDir = crashRootDir;
        this.mSymbolRootDir = symbolRootDir;
        FileUtil.checkAndCreateDir(new File(mRootDir));
        FileUtil.checkAndCreateDir(new File(mCrashRootDir));
        FileUtil.checkAndCreateDir(new File(mSymbolRootDir));
    }

    public int init() {
        if (mCrashRootDir == null) {
            return ERROR;
        }
        return NativeBreakpader.init(mCrashRootDir);
    }

    /**
     * symbol the so libs in order to translate the crash dump files
     *
     * @param libsFiles target so libs file
     * @return
     */
    public int symbol(File[] libsFiles) {
        if (libsFiles == null || mSymbolRootDir == null) {
            return ERROR;
        }

        for (File libPath : libsFiles) {
            symbol(libPath.getAbsolutePath());
        }

        return OK;
    }

    public int symbol(String[] libsPaths) {
        if (libsPaths == null || mSymbolRootDir == null) {
            return ERROR;
        }

        for (String libPath : libsPaths) {
            symbol(libPath);
        }

        return OK;
    }

    /**
     * symbol the lib to dump directory
     *
     * @param libPath
     * @return
     */
    public int symbol(String libPath) {

        if (libPath == null || mSymbolRootDir == null) {
            return ERROR;
        }
        /**
         * dump symbol file to temp
         */
        File symbolTemp = new File(mSymbolRootDir + "/" + new File(libPath).getName() + ".sym");
        if (symbolTemp.exists()) {
            symbolTemp.delete();
        }

        int result = NativeBreakpader.dumpSymbolFile(libPath, symbolTemp.getAbsolutePath());
        if (result == OK) {
            try {
                symbolTemp.renameTo(new File(getSymbolTargetPath(symbolTemp.getAbsolutePath(), mSymbolRootDir) + "/" + symbolTemp.getName()));
                return OK;
            } catch (Exception e) {
                return ERROR;
            }
        }

        return ERROR;
    }

    /**
     * translate crash file into human readable file
     *
     * @param crashFilePath
     * @return
     */
    public ProcessResult translate(String crashFilePath) {
        return NativeBreakpader.translateCrashFile(crashFilePath, mSymbolRootDir);
    }


    /**
     * get symbol target path
     * <p>
     * you must follow the rule to translate the from symbol files.
     * the directory architecture should be rootDir/libNameXXX.so/ID/libNameXXX.so.sym
     * <p>
     * you can get the ID from the symbols head line.
     * <p>
     * like: MODULE operatingsystem architecture id name
     *
     * @param symbolTemp  exist sym file temp path
     * @param dumpRootDir dump symbol file root directory
     * @return target directory
     */
    private String getSymbolTargetPath(String symbolTemp, String dumpRootDir) throws IOException {

        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new InputStreamReader(new FileInputStream(symbolTemp)));
            String line;
            String moduleLine = null;

            /**
             * find the MODULE start line
             */
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("MODULE")) {
                    moduleLine = line;
                }
            }

            if (moduleLine == null) {
                return null;
            }

            /**
             * analyze the module line
             */
            String[] moduleSplits = moduleLine.split("\\s");
            String os = null;
            String architecture = null;
            String id = null;
            String name = null;
            if (moduleSplits.length == 5) {
                os = moduleSplits[1];
                architecture = moduleSplits[2];
                id = moduleSplits[3];
                name = moduleSplits[4];
            }
            /**
             * get the target directory
             */
            String targetDir = dumpRootDir + "/" + name + "/" + id;
            File symbolDir = new File(targetDir);

            if (!symbolDir.exists()) {
                symbolDir.mkdirs();
            }

            return targetDir;
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
        return null;
    }

    public String getRootDir() {
        return mRootDir;
    }

    public String getCrashRootDir() {
        return mCrashRootDir;
    }

    public String getSymbolRootDir() {
        return mSymbolRootDir;
    }
}