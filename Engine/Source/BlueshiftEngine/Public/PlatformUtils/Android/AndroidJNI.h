// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <android/native_window_jni.h>
#include <android/native_activity.h>

BE_NAMESPACE_BEGIN

#define JNI_CURRENT_VERSION JNI_VERSION_1_6

class AndroidJNI {
public:
    static void         Init(ANativeActivity *nativeActivity);

    static JNIEnv *     GetJavaEnv();

    static jobject      CreateObject(JNIEnv *env, const char *class_name);
    static void         DeleteObject(JNIEnv *env, jobject obj);

    static jclass       LoadClass(JNIEnv *env, jobject activityObject, const char *className);

    static jclass       FindClass(JNIEnv *env, const char *className, bool isOptional);
    static jmethodID    FindMethod(JNIEnv *env, jclass javaClass, const char *methodName, const char *methodSignature, bool isOptional);
    static jmethodID    FindStaticMethod(JNIEnv *env, jclass javaClass, const char *methodName, const char *methodSignature, bool isOptional);
    static jfieldID     FindField(JNIEnv *env, jclass javaClass, const char *fieldName, const char *fieldType, bool isOptional);

    static void         CallVoidMethod(JNIEnv *env, jobject object, jmethodID method, ...);
    static jobject      CallObjectMethod(JNIEnv *env, jobject object, jmethodID method, ...);
    static int32_t      CallIntMethod(JNIEnv *env, jobject object, jmethodID method, ...);
    static bool         CallBooleanMethod(JNIEnv *env, jobject object, jmethodID method, ...);
    static float        CallFloatMethod(JNIEnv *env, jobject object, jmethodID method, ...);

    static ANativeActivity *activity;

    static jmethodID    javaMethod_showAlert;

private:
    static void         FindJavaClassesAndMethods();
};

BE_NAMESPACE_END