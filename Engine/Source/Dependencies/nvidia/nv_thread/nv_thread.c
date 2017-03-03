//----------------------------------------------------------------------------------
// File:        jni/nv_thread/nv_thread.c
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

#include <jni.h>

#include "nv_thread.h"
#include <android/log.h>
#include <pthread.h>

static JavaVM* s_vm = NULL;
static pthread_key_t s_jniEnvKey = 0;

#define MODULE "NVThread"

#include <nv_debug.h>

void NVThreadInit(JavaVM* vm)
{
	s_vm = vm;
}

JNIEnv* NVThreadGetCurrentJNIEnv()
{
	JNIEnv* env = NULL;
    if (s_jniEnvKey)
	{
		env = (JNIEnv*)pthread_getspecific(s_jniEnvKey);
	}
	else
	{
		pthread_key_create(&s_jniEnvKey, NULL);
	}

	if (!env)
	{
		// do we have a VM cached?
		if (!s_vm)
		{
			__android_log_print(ANDROID_LOG_DEBUG, MODULE,  "Error - could not find JVM!");
			return NULL;
		}

		// Hmm - no env for this thread cached yet
		int error = (*s_vm)->AttachCurrentThread(s_vm, &env, NULL);
		__android_log_print(ANDROID_LOG_DEBUG, MODULE,  "AttachCurrentThread: %d, 0x%p", error, env);
		if (error || !env)
		{
			__android_log_print(ANDROID_LOG_DEBUG, MODULE,  "Error - could not attach thread to JVM!");
			return NULL;
		}

		pthread_setspecific(s_jniEnvKey, env);
	}

	return env;
}

typedef struct NVThreadInitStruct
{
	void* m_arg;
	void *(*m_startRoutine)(void *);
} NVThreadInitStruct;

static void* NVThreadSpawnProc(void* arg)
{
	NVThreadInitStruct* init = (NVThreadInitStruct*)arg;
	void *(*start_routine)(void *) = init->m_startRoutine;
	void* data = init->m_arg;
	void* ret;

	free(arg);

	NVThreadGetCurrentJNIEnv();

	ret = start_routine(data);

	if (s_vm)
		(*s_vm)->DetachCurrentThread(s_vm);

	return ret;
}

int NVThreadSpawnJNIThread(pthread_t *thread, pthread_attr_t const * attr,
    void *(*start_routine)(void *), void * arg)
{
	if (!start_routine)
		return -1;

	NVThreadInitStruct* initData = malloc(sizeof(NVThreadInitStruct));

	initData->m_startRoutine = start_routine;
	initData->m_arg = arg;

	int err = pthread_create(thread, attr, NVThreadSpawnProc, initData);

	// If the thread was not started, then we need to delete the init data ourselves
	if (err)
	{
		free(initData);
	}

	return err;
}

// on linuces, signals can interrupt sleep functions, so you might need to 
// retry to get the full sleep going. I'm not entirely sure this is necessary
// *here* clients could retry themselves when the exposed function returns
// nonzero
inline int __sleep(const struct timespec *req, struct timespec *rem)
{
	int ret = 1;
	int i;
	static const int sleepTries = 2;

	struct timespec req_tmp={0}, rem_tmp={0};

	rem_tmp = *req;
	for(i = 0; i < sleepTries; ++i)
	{
		req_tmp = rem_tmp;
		int ret = nanosleep(&req_tmp, &rem_tmp);
		if(ret == 0)
		{
			ret = 0;
			break;
		}
	}
	if(rem)
		*rem = rem_tmp;
	return ret;
}

int NVThreadSleep(unsigned long millisec)
{
    struct timespec req={0},rem={0};
    time_t sec  =(int)(millisec/1000);

    millisec     = millisec-(sec*1000);
    req.tv_sec  = sec;
    req.tv_nsec = millisec*1000000L;
    return __sleep(&req,&rem);
}

