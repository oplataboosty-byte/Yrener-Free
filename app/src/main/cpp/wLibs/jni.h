
#ifndef _JNI_H_
#define _JNI_H_


#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Basic JNI types */
typedef int jint;
typedef signed char jbyte;
typedef unsigned char jboolean;


/* JNI versions */
#define JNI_VERSION_1_6 0x00010006

/* JNIEnv function pointers */
struct JNINativeInterface;


/* JavaVM structure */
struct JavaVM_;


/* Method and field IDs */


/* JNIEnv functions */


/* JavaVM functions */
struct JavaVM_ {
    jint (JNICALL *GetEnv)(JavaVM*, void**, jint);
};

/* Macros for export */
#define JNIEXPORT  __attribute__ ((visibility ("default")))
#define JNICALL

#ifdef __cplusplus
}
#endif

#endif /* _JNI_H_ */
