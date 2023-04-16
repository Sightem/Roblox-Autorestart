#pragma once
#include <windows.h>

#define PTR_STR_LEN 25
#define MAXIMUM_FILENAME_LENGTH 256

enum IntegrityLevel {
    UNTRUSTED_INTEGRITY,
    LOW_INTEGRITY,
    MEDIUM_INTEGRITY,
    HIGH_INTEGRITY,
    SYSTEM_INTEGRITY,
    PPL_INTEGRITY,
    INTEGRITY_UNKNOWN,
};

typedef struct _LHF_HANDLE_DESCRIPTION
{
    ULONG_PTR Handle;
    ULONG GrantedAccess;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR ParentUniqueProcessId;
    ULONG_PTR RemoteProcessId;
    IntegrityLevel ParentProcessIntegrity;
    IntegrityLevel ProcessIntegrity;
    USHORT ObjectTypeIndex;
    LPTSTR TypeString;
    LPTSTR Name;
    LPTSTR GrantedAccessString;
    LPTSTR ParentProcessIntegrityString;
    LPTSTR ProcessIntegrityString;

} LHF_HANDLE_DESCRIPTION, * LHF_PHANDLE_DESCRIPTION;

typedef struct _LHF_HANDLE_DESCRIPTION_LIST
{
    ULONG NumberOfLhfHandles = 0;
    LHF_PHANDLE_DESCRIPTION Handles;

} LHF_HANDLE_DESCRIPTION_LIST, * LHF_PHANDLE_DESCRIPTION_LIST;

struct GetObjectNameThreadStructParams
{
    HANDLE hObject;
    PVOID objectNameInfo;
};