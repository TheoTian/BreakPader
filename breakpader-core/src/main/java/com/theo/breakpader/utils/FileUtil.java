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

package com.theo.breakpader.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

public class FileUtil {

    /**
     * check & create the directory
     *
     * @param dir
     * @return
     */
    public static boolean checkAndCreateDir(File dir) {
        if (dir == null) {
            return false;
        }
        if (dir.exists() && dir.isFile()) {
            return false;
        }
        if (!dir.exists()) {
            return dir.mkdirs();
        }
        return true;
    }

    /**
     * 监测并且创建新文件
     *
     * @return
     */
    public static boolean checkAndCreateFile(File file) {
        if (file == null) {
            return false;
        }

        if (!file.exists()) {
            try {
                return file.createNewFile();
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        return true;
    }

    /**
     * delete file
     *
     * @param file
     */
    public static void delete(File file) {
        if (file != null && file.exists()) {
            file.delete();
        }
    }


    /**
     * 写对象到文件
     *
     * @param file
     * @param obj
     */
    public static void writeObjectToFile(File file, Object obj) {
        if (file == null) {
            return;
        }
        checkAndCreateDir(file.getParentFile());
        checkAndCreateFile(file);
        FileOutputStream out;
        try {
            out = new FileOutputStream(file);
            ObjectOutputStream objOut = new ObjectOutputStream(out);
            objOut.writeObject(obj);
            objOut.flush();
            objOut.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * read file
     * file length must < Integer.MAX_VALUE
     *
     * @param file
     * @return
     */
    public static byte[] readFile(File file) {
        if (file == null && file.length() > Integer.MAX_VALUE) {
            return null;
        }
        byte[] data = new byte[(int) file.length()];

        try {
            FileInputStream is = new FileInputStream(file);
            is.read(data);
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return data;
    }

    /**
     * rename file to new name
     *
     * @param file
     * @param pathname
     * @return
     */
    public static boolean rename(File file, String pathname) {
        if (file == null) {
            return false;
        }
        return file.renameTo(new File(pathname));
    }

}
