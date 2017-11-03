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
#include "base/common.h"
#include "wrap/breakpad_wrapper.h"

static char *class_nativebreakpader = "com/theo/breakpader/NativeBreakpader";
static char *class_processresult = "com/theo/breakpader/ProcessResult";
static char *class_stackframe = "com/theo/breakpader/ProcessResult$StackFrame";
/**
 * API ZONE
 */

JNIEXPORT jint JNICALL Init(JNIEnv *env,
                            jobject jobj,
                            jstring crash_dump_path) {
    const char *path = (char *) env->GetStringUTFChars(crash_dump_path, NULL);

    breakpad_wrapper::init_breakpad(path);

    env->ReleaseStringUTFChars(crash_dump_path, path);
    LOGD("Init savePath: %s", path);
    return JNI_OK;
}

JNIEXPORT jint JNICALL DumpSymbolsFile(JNIEnv *env, jobject jobj, jstring jso_file_path,
                                       jstring jsymbol_file_path) {
    jint RESULT = JNI_OK;
    const char *so_file_path = (char *) env->GetStringUTFChars(jso_file_path, NULL);
    const char *symbol_file_path = (char *) env->GetStringUTFChars(jsymbol_file_path, NULL);

    breakpad_wrapper::dump_symbols_file(so_file_path, symbol_file_path);

    env->ReleaseStringUTFChars(jso_file_path, so_file_path);
    env->ReleaseStringUTFChars(jsymbol_file_path, symbol_file_path);

    return RESULT;
}

JNIEXPORT jobject JNICALL TranslateCrashFile(JNIEnv *env, jobject jobj, jstring jcrash_file_path,
                                             jstring jsymbol_files_dir) {
    LOGD("JNI translate_crash_file IN");
    const char *crash_file_path = (char *) env->GetStringUTFChars(jcrash_file_path, NULL);
    const char *symbol_files_dir = (char *) env->GetStringUTFChars(jsymbol_files_dir, NULL);

    breakpad_wrapper::struct_translate_result translateResult =
            breakpad_wrapper::translate_crash_file(crash_file_path, symbol_files_dir);

    LOGD("JNI translate_crash_file crashed:%d,p_crash_reason:%s,crash_address: 0x%"
                 PRIx64, translateResult.crashed, translateResult.p_crash_reason,
         translateResult.crash_address);

    jclass jprocessresult_class = env->FindClass(class_processresult);
    jmethodID jprocessresult_construct_method = env->GetMethodID(jprocessresult_class,
                                                                 "<init>",
                                                                 "()V");//构造函数ID

    jfieldID jfield_boolean_crashed = env->GetFieldID(jprocessresult_class, "crashed", "Z");
    jfieldID jfield_string_crash_reason = env->GetFieldID(jprocessresult_class, "crash_reason",
                                                          "Ljava/lang/String;");
    jfieldID jfield_long_crash_address = env->GetFieldID(jprocessresult_class, "crash_address",
                                                         "J");
    jfieldID jfield_object_array_frames = env->GetFieldID(jprocessresult_class,
                                                          "crash_stack_frames",
                                                          "[Lcom/theo/breakpader/ProcessResult$StackFrame;");
    /**
     * Get ProcessResult Object
     */
    jobject jprocessresult_obj = env->NewObject(jprocessresult_class,
                                                jprocessresult_construct_method);


    env->SetBooleanField(jprocessresult_obj, jfield_boolean_crashed, translateResult.crashed);
    env->SetLongField(jprocessresult_obj, jfield_long_crash_address, translateResult.crash_address);
    env->SetObjectField(jprocessresult_obj, jfield_string_crash_reason,
                        env->NewStringUTF(translateResult.p_crash_reason));

    /**
     * Get StackFrame Object
     */
    jclass jstackframe_class = env->FindClass(class_stackframe);
    jmethodID jstackframe_construct_method = env->GetMethodID(jstackframe_class,
                                                              "<init>",
                                                              "()V");//构造函数ID

    jobjectArray jstackframe_array = env->NewObjectArray(translateResult.stack_frames_num,
                                                         jstackframe_class, NULL);
    env->SetObjectField(jprocessresult_obj, jfield_object_array_frames, jstackframe_array);

    jfieldID jfield_int_frame_index = env->GetFieldID(jstackframe_class, "frame_index", "I");
    jfieldID jfield_string_code_file = env->GetFieldID(jstackframe_class, "code_file",
                                                       "Ljava/lang/String;");
    jfieldID jfield_long_instruction = env->GetFieldID(jstackframe_class, "instruction", "J");
    jfieldID jfield_string_function_name = env->GetFieldID(jstackframe_class, "function_name",
                                                           "Ljava/lang/String;");
    jfieldID jfield_long_offset = env->GetFieldID(jstackframe_class, "offset", "J");

    for (int i = 0; i < translateResult.stack_frames_num; i++) {
        breakpad_wrapper::struct_stack_frame frame = translateResult.p_stack_frames[i];

        jobject jstackframe_obj = env->NewObject(jstackframe_class,
                                                 jstackframe_construct_method);

        jstring jstring_code_file = env->NewStringUTF(frame.p_code_file);
        jstring jstring_fun_name = env->NewStringUTF(frame.p_function_name);

        env->SetIntField(jstackframe_obj, jfield_int_frame_index, frame.frame_index);
        env->SetObjectField(jstackframe_obj, jfield_string_code_file, jstring_code_file);
        env->SetLongField(jstackframe_obj, jfield_long_instruction, frame.instruction);
        env->SetObjectField(jstackframe_obj, jfield_string_function_name, jstring_fun_name);
        env->SetLongField(jstackframe_obj, jfield_long_offset, frame.offset);

        env->SetObjectArrayElement(jstackframe_array, i, jstackframe_obj);

        env->DeleteLocalRef(jstackframe_obj);
        env->DeleteLocalRef(jstring_code_file);
        env->DeleteLocalRef(jstring_fun_name);

        if (frame.p_code_file) {
            free(frame.p_code_file);
        }
        if (frame.p_function_name) {
            free(frame.p_function_name);
        }
    }

    if (translateResult.p_crash_reason) {
        free(translateResult.p_crash_reason);
    }

    env->ReleaseStringUTFChars(jcrash_file_path, crash_file_path);
    env->ReleaseStringUTFChars(jsymbol_files_dir, symbol_files_dir);
    LOGD("JNI translate_crash_file OUT");
    return jprocessresult_obj;
}


/**
 * TEST ZONE
 */

JNIEXPORT jstring JNICALL TestJNI(JNIEnv *env, jobject jobj) {
    char *str = "Test JNI";
    return env->NewStringUTF(str);
}

JNIEXPORT jint JNICALL TestCrash(JNIEnv *env, jobject jobj) {
    LOGE("native crash capture begin");
    int i = 0;
    int test = 100 / i;
    LOGE("native crash capture end %d", test);
    return 0;
}

static JNINativeMethod nativeMethods[] = {
        {"testJNI",            "()Ljava/lang/String;",                                                      (void *) TestJNI},
        {"init",               "(Ljava/lang/String;)I",                                                     (void *) Init},
        {"testCrash",          "()I",                                                                       (void *) TestCrash},
        {"dumpSymbolFile",     "(Ljava/lang/String;Ljava/lang/String;)I",                                   (void *) DumpSymbolsFile},
        {"translateCrashFile", "(Ljava/lang/String;Ljava/lang/String;)Lcom/theo/breakpader/ProcessResult;", (void *) TranslateCrashFile},
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
