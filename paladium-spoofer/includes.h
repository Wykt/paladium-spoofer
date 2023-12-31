#pragma once
#include <windows.h> // win api
#include "detours.h" // hooking
#include <iostream> // yes

//java shit
#include <jni.h>
#include <jvmti.h>

extern JNIEnv* env;
extern jvmtiEnv* jvmti;