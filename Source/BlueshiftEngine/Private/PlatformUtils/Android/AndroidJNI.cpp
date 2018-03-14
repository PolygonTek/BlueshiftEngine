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

#include "Precompiled.h"
#include "Core/Str.h"
#include "PlatformUtils/Android/AndroidJNI.h"

BE_NAMESPACE_BEGIN

ANativeActivity *   AndroidJNI::activity = nullptr;
jmethodID           AndroidJNI::javaMethod_showAlert = nullptr;

void AndroidJNI::Init(ANativeActivity *nativeActivity) {
    AndroidJNI::activity = nativeActivity;

    AndroidJNI::FindJavaClassesAndMethods();
}

void AndroidJNI::FindJavaClassesAndMethods() {
    JNIEnv *env = AndroidJNI::GetJavaEnv();

    jclass javaClassActivity = env->GetObjectClass(AndroidJNI::activity->clazz);

    javaMethod_showAlert = AndroidJNI::FindMethod(env, javaClassActivity, "showAlert", "(Ljava/lang/String;)V", false);

    env->DeleteLocalRef(javaClassActivity);
}

// Unregister this thread from the VM
static void DetachCurrentThreadDtor(void *p) {
    ANativeActivity *activity = (ANativeActivity *)p;
    activity->vm->DetachCurrentThread();
}

JNIEnv *AndroidJNI::GetJavaEnv() {
    JNIEnv *env;
    if (AndroidJNI::activity->vm->GetEnv((void **)&env, JNI_CURRENT_VERSION) == JNI_OK) {
        return env;
    }
    AndroidJNI::activity->vm->AttachCurrentThread(&env, nullptr);
    pthread_key_create((int32_t *)AndroidJNI::activity, DetachCurrentThreadDtor);

    return env;
}

jobject AndroidJNI::CreateObject(JNIEnv *env, const char *className) {
    jclass javaClass = env->FindClass(className);
    jmethodID ctor = env->GetMethodID(javaClass, "<init>", "()V");

    jobject object = env->NewObject(javaClass, ctor);
    jobject objectGlobal = env->NewGlobalRef(object);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(javaClass);

    return objectGlobal;
}

void AndroidJNI::DeleteObject(JNIEnv *env, jobject object) {
    if (!object) {
        return;
    }
    env->DeleteGlobalRef(object);
}

jclass AndroidJNI::LoadClass(JNIEnv *env, jobject activityObject, const char *className) {
    jclass javaActivityClass = env->FindClass("android/app/NativeActivity");
    jmethodID javaActivity_getClassLoader = env->GetMethodID(javaActivityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject javaClassLoader = env->CallObjectMethod(activityObject, javaActivity_getClassLoader);

    jclass javaClassLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID javaClassLoader_loadClass = env->GetMethodID(javaClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring javaClassName = env->NewStringUTF(className);
    jclass javaClass = (jclass)env->CallObjectMethod(javaClassLoader, javaClassLoader_loadClass, javaClassName);

    env->DeleteLocalRef(javaClassName);
    env->DeleteLocalRef(javaActivityClass);
    env->DeleteLocalRef(javaClassLoaderClass);

    return javaClass;
}

jclass AndroidJNI::FindClass(JNIEnv *env, const char *className, bool isOptional) {
    jclass javaClass = env->FindClass(className);
    if (!javaClass && !isOptional) {
        BE_FATALERROR(L"Failed to find %hs", className);
    }
    return javaClass;
}

jmethodID AndroidJNI::FindMethod(JNIEnv *env, jclass javaClass, const char *methodName, const char *methodSignature, bool isOptional) {
    jmethodID method = javaClass ? env->GetMethodID(javaClass, methodName, methodSignature) : nullptr;
    if (!method && !isOptional) {
        BE_FATALERROR(L"Failed to find %hs", methodName);
    }
    return method;
}

jmethodID AndroidJNI::FindStaticMethod(JNIEnv *env, jclass javaClass, const char *methodName, const char *methodSignature, bool isOptional) {
    jmethodID method = javaClass ? env->GetStaticMethodID(javaClass, methodName, methodSignature) : nullptr;
    if (!method && !isOptional) {
        BE_FATALERROR(L"Failed to find %hs", methodName);
    }
    return method;
}

jfieldID AndroidJNI::FindField(JNIEnv *env, jclass javaClass, const char *fieldName, const char *fieldType, bool isOptional) {
    jfieldID field = env->GetFieldID(javaClass, fieldName, fieldType);
    if (!field && !isOptional) {
        BE_FATALERROR(L"Failed to find %hs", fieldName);
    }
    return field;
}

void AndroidJNI::CallVoidMethod(JNIEnv *env, jobject object, jmethodID method, ...) {
    if (!object || !method) {
        return;
    }
    va_list args;
    va_start(args, method);
    env->CallVoidMethodV(object, method, args);
    va_end(args);
}

jobject AndroidJNI::CallObjectMethod(JNIEnv *env, jobject object, jmethodID method, ...) {
    if (!object || !method) {
        return nullptr;
    }
    va_list args;
    va_start(args, method);
    jobject ret = env->CallObjectMethodV(object, method, args);
    va_end(args);

    return ret;
}

int32_t AndroidJNI::CallIntMethod(JNIEnv *env, jobject object, jmethodID method, ...) {
    if (!object || !method) {
        return 0;
    }
    va_list args;
    va_start(args, method);
    jint ret = env->CallIntMethodV(object, method, args);
    va_end(args);
    return (int32_t)ret;
}

bool AndroidJNI::CallBooleanMethod(JNIEnv *env, jobject object, jmethodID method, ...) {
    if (!object || !method) {
        return false;
    }
    va_list args;
    va_start(args, method);
    jboolean ret = env->CallBooleanMethodV(object, method, args);
    va_end(args);

    return (bool)ret;
}

float AndroidJNI::CallFloatMethod(JNIEnv *env, jobject object, jmethodID method, ...) {
    if (!object || !method) {
        return 0.0f;
    }
    va_list args;
    va_start(args, method);
    jfloat ret = env->CallFloatMethodV(object, method, args);
    va_end(args);

    return (float)ret;
}

BE_NAMESPACE_END