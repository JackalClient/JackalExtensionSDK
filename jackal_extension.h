#pragma once
#include <stdint.h>

#define JC_EXTENSION_ABI_VERSION 1u
#define JC_EXTENSION_API_VERSION 1u
#define JC_EXTENSION_OK 1
#define JC_EXTENSION_ERROR 0
#define JC_EXTENSION_BUFFER_TOO_SMALL (-1)
#define JC_EXTENSION_WRONG_THREAD (-2)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JcExtensionApiV1 {
    uint32_t size;
    uint32_t version;
    int (__cdecl* query)(void* context, const char* requestJsonUtf8, char* responseUtf8, uint32_t capacity, uint32_t* required);
    int (__cdecl* post)(void* context, const char* requestJsonUtf8, uint64_t* ticket);
} JcExtensionApiV1;

typedef struct JcExtensionHostV1 {
    uint32_t size;
    uint32_t version;
    const char* extensionDirectoryUtf8;
    const JcExtensionApiV1* api;
    void* context;
} JcExtensionHostV1;

typedef uint32_t (__cdecl* JcExtensionVersionFn)(void);
typedef int (__cdecl* JcExtensionInitializeFn)(const JcExtensionHostV1*);
typedef int (__cdecl* JcExtensionEnableFn)(const char* configUtf8);
typedef int (__cdecl* JcExtensionDisableFn)(void);
typedef int (__cdecl* JcExtensionIsEnabledFn)(void);
typedef int (__cdecl* JcExtensionUpdateFn)(const char* configUtf8);

#ifdef __cplusplus
}
#endif
