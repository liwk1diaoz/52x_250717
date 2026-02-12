// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIVE555DLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIVE555DLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LIVE555DLL_EXPORTS
#define LIVE555DLL_API __declspec(dllexport)
#else
#define LIVE555DLL_API __declspec(dllimport)
#endif

extern "C" LIVE555DLL_API int live555dll_Open(const char* szUrl);
extern "C" LIVE555DLL_API int live555dll_Close(void);