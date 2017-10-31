#include <common/linux/dump_symbols.h>
#include "base/common.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"
#include <fstream>

using google_breakpad::WriteSymbolFile;
using google_breakpad::WriteSymbolFileHeader;

static char *class_nativebreakpader = "com/theo/breakpader/NativeBreakpader";

JNIEXPORT jstring JNICALL TestJNI(JNIEnv *env, jobject jobj) {
    char *str = "Test JNI";
    return env->NewStringUTF(str);
}

JNIEXPORT jint JNICALL TestCrash(JNIEnv *env, jobject jobj) {
    LOGE("native crash capture begin");
    char *ptr = NULL;
    *ptr = 1;

    LOGE("native crash capture end");
    return 0;
}

bool DumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {
    LOGE("DumpCallback ===> succeeded %d", succeeded);
    return succeeded;
}

JNIEXPORT jint JNICALL Init(JNIEnv *env,
                            jobject jobj,
                            jstring crash_dump_path) {
    const char *path = (char *) env->GetStringUTFChars(crash_dump_path, NULL);
    google_breakpad::MinidumpDescriptor descriptor(path);
    static google_breakpad::ExceptionHandler eh(descriptor, NULL, DumpCallback, NULL, true, -1);
    env->ReleaseStringUTFChars(crash_dump_path, path);
    LOGD("nativeInit ===> breakpad initialized succeeded, dump file will be saved at %s", path);
    return 0;
}

JNIEXPORT jint JNICALL DumpSymbolsFile(JNIEnv *env, jobject jobj, jstring jso_path,
                                       jstring jsymbol_file_path) {
    jint RESULT = JNI_OK;
    const char *so_path = (char *) env->GetStringUTFChars(jso_path, NULL);
    const char *symbol_file_path = (char *) env->GetStringUTFChars(jsymbol_file_path, NULL);

    std::ofstream file;
    std::vector<string> debug_dirs;
    file.open(symbol_file_path);

    google_breakpad::DumpOptions options(ALL_SYMBOL_DATA, true);
    if (!WriteSymbolFile(so_path, debug_dirs, options, file)) {
        LOGE("Failed to write symbol file.\n");
        RESULT = JNI_ERR;
        goto END;
    }

    RESULT = JNI_OK;

    END:
    file.close();
    env->ReleaseStringUTFChars(jso_path, so_path);
    env->ReleaseStringUTFChars(jsymbol_file_path, symbol_file_path);

    return RESULT;
}


static JNINativeMethod nativeMethods[] = {
        {"testJNI",        "()Ljava/lang/String;",                    (void *) TestJNI},
        {"init",           "(Ljava/lang/String;)I",                   (void *) Init},
        {"testCrash",      "()I",                                     (void *) TestCrash},
        {"dumpSymbolFile", "(Ljava/lang/String;Ljava/lang/String;)I", (void *) DumpSymbolsFile},
};

JNIEXPORT int JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass javaClass = env->FindClass(class_nativebreakpader);
    if (javaClass == NULL) {
        return JNI_ERR;
    }
    if (env->RegisterNatives(javaClass, nativeMethods,
                             sizeof(nativeMethods) / sizeof(nativeMethods[0])) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
