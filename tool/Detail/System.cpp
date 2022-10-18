
#if defined(_WINDOWS) || defined(WIN32)
#include <windows.h>
#else

#endif

size_t GetAvailMemSize(){
#if defined(_WINDOWS) || defined(WIN32)
    MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullAvailPhys;
#else

#endif
}