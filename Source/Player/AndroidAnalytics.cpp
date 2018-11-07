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
#include "Application.h"
#include "android_native_app_glue.h"
#include "AndroidAnalytics.h"

static jmethodID javaMethod_initializeAnalytics = nullptr;
static jmethodID javaMethod_logAnalyticsEvent = nullptr;

void Analytics::RegisterLuaModule(LuaCpp::State *state) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jclass javaClassActivity = env->GetObjectClass(BE1::AndroidJNI::activity->clazz);

    javaMethod_initializeAnalytics = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeAnalytics", "(Ljava/lang/String;)V", false);
    javaMethod_logAnalyticsEvent = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "logAnalyticsEvent", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V", false);

    env->DeleteLocalRef(javaClassActivity);

    state->RegisterModule("analytics", [](LuaCpp::Module &module) {
        module["init"].SetFunc(Analytics::Init);
        module["log"].SetFunc(Analytics::Log);
    });
};

void Analytics::Init(const char *trackingID) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jstring javaTrackingID = BE1::Str(trackingID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeAnalytics, javaTrackingID);

    env->DeleteLocalRef(javaTrackingID);
}

void Analytics::Log(const char *category, const char *action, const char *label, long value) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jstring javaCategory = BE1::Str(category).ToJavaString(env);
    jstring javaAction = BE1::Str(action).ToJavaString(env);
    jstring javaLabel = BE1::Str(label).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_logAnalyticsEvent, javaCategory, javaAction, javaLabel, value);

    env->DeleteLocalRef(javaCategory);
    env->DeleteLocalRef(javaAction);
    env->DeleteLocalRef(javaLabel);
}
