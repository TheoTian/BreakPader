#include "base/common.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"
#include "breakpad/src/client/linux/handler/minidump_descriptor.h"

static char *class_nativebreakpader = "com/theo/breakpader/NativeBreakpader";

bool DumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {
    LOGE("DumpCallback ===> succeeded %d", succeeded);
    return succeeded;
}

JNIEXPORT jint JNICALL Init(JNIEnv *env,
                            jobject obj,
                            jstring crash_dump_path) {
    const char *path = (char *) env->GetStringUTFChars(crash_dump_path, NULL);
    google_breakpad::MinidumpDescriptor descriptor(path);
    static google_breakpad::ExceptionHandler eh(descriptor, NULL, DumpCallback, NULL, true, -1);
    env->ReleaseStringUTFChars(crash_dump_path, path);
    LOGD("nativeInit ===> breakpad initialized succeeded, dump file will be saved at %s", path);
    return 0;
}

jstring TestJNI(JNIEnv *env, jobject jobj) {
    char *str = "Test JNI";
    return env->NewStringUTF(str);
}

JNIEXPORT jint JNICALL TestCrash(JNIEnv *env, jobject obj) {
    LOGE("native crash capture begin");
    char *ptr = NULL;
    *ptr = 1;

    LOGE("native crash capture end");
    return 0;
}


static JNINativeMethod nativeMethods[] = {
        {"testJNI",   "()Ljava/lang/String;",  (void *) TestJNI},
        {"init",      "(Ljava/lang/String;)I", (void *) Init},
        {"testCrash", "()I",                   (void *) TestCrash},
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
