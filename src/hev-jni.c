/*
 ============================================================================
 Name        : hev-jni.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2014 everyone.
 Description : JNI
 ============================================================================
 */

#if defined(ANDROID)
#include <jni.h>
#include <pthread.h>

#include <stdio.h>
#include <signal.h>

#include "hev-jni.h"
#include "hev-main.h"

#define N_ELEMENTS(arr)		(sizeof (arr) / sizeof ((arr)[0]))

static JavaVM *java_vm;
static pthread_t service;
static pthread_key_t current_jni_env;

static void native_start_service (JNIEnv *env, jobject thiz,
			jstring local_address, jint local_port,
			jstring upstream_address);
static void native_stop_service (JNIEnv *env, jobject thiz);

static JNINativeMethod native_methods[] =
{
	{ "NativeStartService", "(Ljava/lang/String;ILjava/lang/String;I)V", (void *) native_start_service },
	{ "NativeStopService", "()V", (void *) native_stop_service },
};

static void
detach_current_thread (void *env)
{
	(*java_vm)->DetachCurrentThread (java_vm);
}

jint
JNI_OnLoad (JavaVM *vm, void *reserved)
{
	JNIEnv *env = NULL;
	jclass klass;

	java_vm = vm;
	if (JNI_OK != (*vm)->GetEnv (vm, (void**) &env, JNI_VERSION_1_4)) {
		return 0;
	}

	klass = (*env)->FindClass (env, "hev/dns/DNSFwdService");
	(*env)->RegisterNatives (env, klass, native_methods, N_ELEMENTS (native_methods));
	(*env)->DeleteLocalRef (env, klass);

	pthread_key_create (&current_jni_env, detach_current_thread);

	return JNI_VERSION_1_4;
}

static void *
thread_handler (void *data)
{
	char **argv = data;

	argv[0] = "hev-dns-forwarder";

	main (4, argv);

	free (argv[1]);
	free (argv[2]);
	free (argv[3]);
	free (argv);

	return NULL;
}

static void
native_start_service (JNIEnv *env, jobject thiz,
			jstring local_address, jint local_port,
			jstring upstream_address)
{
	char buf[16], **argv;
	const jbyte *bytes;

	argv = malloc (sizeof (char *) * 4);

	bytes = (*env)->GetStringUTFChars (env, local_address, NULL);
	argv[1] = strdup (bytes);
	(*env)->ReleaseStringUTFChars (env, local_address, bytes);

	snprintf (buf, 16, "%u", local_port);
	argv[2] = strdup (buf);

	bytes = (*env)->GetStringUTFChars (env, upstream_address, NULL);
	argv[3] = strdup (bytes);
	(*env)->ReleaseStringUTFChars (env, upstream_address, bytes);

	pthread_create (&service, NULL, thread_handler, argv);
}

static void
native_stop_service (JNIEnv *env, jobject thiz)
{
	if (service)
	  pthread_kill (service, SIGINT);
}

#endif

