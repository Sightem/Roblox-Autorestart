#include "functions.h"
#include "Native.h"
#include "Lhf.h"

#include <conio.h>
#include <unordered_set>
#include <shlwapi.h>
#include <psapi.h>
#include <strsafe.h>
#include <lmcons.h>

#pragma comment(lib, "Shlwapi.lib")

DWORD Functions::GetProcessIDOfFileOpener(const std::string& path)
{
    std::wstring wpath(path.begin(), path.end());
    WCHAR* wpath2 = (WCHAR*)wpath.c_str();

    DWORD dwSession;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };
    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);

    if (dwError == ERROR_SUCCESS)
    {
        PCWSTR pszFile = wpath2;

        dwError = RmRegisterResources(dwSession, 1, &pszFile, 0, NULL, 0, NULL);
        if (dwError == ERROR_SUCCESS)
        {
            DWORD dwReason;
            UINT nProcInfoNeeded;
            UINT nProcInfo = 1;
            RM_PROCESS_INFO rgpi[1];

            dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgpi, &dwReason);
            RmEndSession(dwSession);

            return rgpi[0].Process.dwProcessId;
        }
    }
}

bool Functions::checkProcessExists(DWORD pid, const std::string& processName)
{
    bool processExists = false;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: " << GetLastError() << std::endl;
        return false;
    }

    if (Process32First(snapshot, &processEntry))
    {
        do
        {
            std::string currentProcessName(processEntry.szExeFile, strlen(processEntry.szExeFile));
            if (processEntry.th32ProcessID == pid && processName == currentProcessName)
            {
                processExists = true;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    else
    {
        std::cerr << "Error: " << GetLastError() << std::endl;
    }

    CloseHandle(snapshot);
    return processExists;
}

std::string Functions::searchFileForPattern(const std::string& filePath, const std::string& pattern)
{
    std::ifstream file(filePath);
    std::string line;
    std::regex regexPattern(pattern);
    std::smatch match;

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return "";
    }

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, regexPattern)) {
            return match[1].str();
        }
    }

    file.close();
    return "";
}

bool Functions::TerminateProcessesByName(const std::string& processName) 
{
    bool success = true;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, processName.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess == NULL) {
                    success = false;
                    continue;
                }

                BOOL terminateSuccess = TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
                if (terminateSuccess == 0) {
                    success = false;
                    continue;
                }
            }
        }
    }

    CloseHandle(snapshot);
    return success;
}

bool Functions::IsProcessRunning(const std::string& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (strcmp(processEntry.szExeFile, processName.c_str()) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return false;
}

std::string Functions::readEntireFile(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return "";
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    file.close();

    return contents.str();
}

std::string Functions::searchStringForPattern(const std::string& str, const std::string& pattern)
{
	std::regex regexPattern(pattern);
	std::smatch match;
    if (std::regex_search(str, match, regexPattern)) {
		return match[1].str();
	}
	return "";
}

std::string Functions::extractExeName(const std::string& path)
{
    std::regex exe_regex(R"(([\w\d_]+\.exe))", std::regex_constants::icase);
    std::smatch exe_match;

    if (std::regex_search(path, exe_match, exe_regex))
    {
        return exe_match.str(1);
    }
    else
    {
        return "";
    }
}

std::string Functions::getRobloxPath()
{
    std::string path;

    char value[255]{};
    DWORD BufferSize = 8192;
    RegGetValue(HKEY_CLASSES_ROOT, "roblox-player\\shell\\open\\command", "", RRF_RT_ANY, NULL, (PVOID)&value, &BufferSize);
    path = value;
    path = path.substr(1, path.length() - 5);

    return path;
}

std::string Functions::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d-%H-%M-%S");
    return timestamp.str();
}

bool Functions::TerminateProcessTree(DWORD dwProcessId, UINT uExitCode)
{
    PROCESSENTRY32 pe;
    HANDLE hProcessSnap;
    HANDLE hProcess;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return false;
    }

    pe.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process.
    if (!Process32First(hProcessSnap, &pe))
    {
        std::cerr << "Process32First failed: " << GetLastError() << std::endl;
        CloseHandle(hProcessSnap);
        return false;
    }

    do
    {
        // If the current process is a child of the target process, terminate it and its child processes recursively.
        if (pe.th32ParentProcessID == dwProcessId)
        {
            TerminateProcessTree(pe.th32ProcessID, uExitCode);

            hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, uExitCode);
                CloseHandle(hProcess);
            }
        }
    } while (Process32Next(hProcessSnap, &pe));

    CloseHandle(hProcessSnap);
    return true;
}

void Functions::flushConsole()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;

    DWORD charsWritten;
    FillConsoleOutputCharacter(
        hConsole,        // Console handle
        ' ',             // Character to write (space)
        consoleSize,     // Number of characters to write
        { 0, 0 },        // Start coordinates (top-left corner)
        &charsWritten    // Receives the number of characters written
    );

    SetConsoleCursorPosition(hConsole, { 0, 0 });
}

void Functions::wait()
{
    std::cout << "Press any key to continue..." << std::endl;

    while (_kbhit()) {
        _getch();
    }

    _getch();
}

//NTSTATUS Functions::getAllSystemHandlers(_Out_ PSYSTEM_HANDLE_INFORMATION_EX* handles) 
NTSTATUS getAllSystemHandlers(_Out_ void* out)
{

    PSYSTEM_HANDLE_INFORMATION_EX* handles = (PSYSTEM_HANDLE_INFORMATION_EX*)out;

    NTSTATUS status;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfoEx;
    ULONG handleInfoSizeEx = 0x10000;

    handleInfoEx = (PSYSTEM_HANDLE_INFORMATION_EX)malloc(handleInfoSizeEx);

    while ((status = NtQuerySystemInformation(
        SystemExtendedHandleInformation,
        handleInfoEx,
        handleInfoSizeEx,
        NULL
    )) == STATUS_INFO_LENGTH_MISMATCH) {
        handleInfoEx = (PSYSTEM_HANDLE_INFORMATION_EX)realloc(handleInfoEx, handleInfoSizeEx *= 2);
        if (handleInfoEx == NULL)
            break;
    }

    if (!NT_SUCCESS(status) || handleInfoEx == NULL)
    {
        _tprintf(_T("   [-] NtQuerySystemInformation failed!\n"));
        *handles = NULL;
        free(handleInfoEx);
    }
    else {
        *handles = handleInfoEx;
    }

    return status;
}

size_t IhCount(_In_ PSYSTEM_HANDLE_INFORMATION_EX systemHandlesList) {
    size_t count = 0;


    for (int i = 0; i < systemHandlesList->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &systemHandlesList->Handles[i];
        if (handleInfo->HandleAttributes & (HANDLE_INHERIT)) {
            count++;
        }
    }

    return count;
}

NTSTATUS GetThreadBasicInformation(_In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_BASIC_INFORMATION BasicInformation)
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadBasicInformation,
        BasicInformation,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
    );
}

NTSTATUS GetProcessBasicInformation(_In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_BASIC_INFORMATION BasicInformation)
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        BasicInformation,
        sizeof(PROCESS_BASIC_INFORMATION),
        NULL
    );
}

HRESULT custom_strcpy_s(LPTSTR dest, int destSize, const PWSTR src) {
    if (dest == nullptr || src == nullptr) {
        return E_INVALIDARG;
    }

    int srcLength = lstrlenW(src);

    if (srcLength >= destSize) {
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    for (int i = 0; i <= srcLength; ++i) {
        dest[i] = src[i];
    }

    return S_OK;
}


BOOL FormatObjectName(_In_ HANDLE DupHandle,
    _In_ LPTSTR TypeName,
    _In_ ULONG GrantedAccess,
    _Out_ LPTSTR* FormatedObjectName,
    _Out_ ULONG_PTR* RemoteProcessId)
{
    PVOID objectNameInfo;
    UNICODE_STRING objectName;
    ULONG returnLength;

    //_tprintf(_T("[%#x] Abriendo!\n"), GrantedAccess);
    /* Query the object name (unless it has an access of
          0x0012019f, on which NtQueryObject could hang. */
    if (
        GrantedAccess == 0x0012019f ||
        GrantedAccess == 0x1A019F ||
        GrantedAccess == 0x1048576f ||
        GrantedAccess == 0x120189
        )
    {

        //*FormatedObjectName = new TCHAR[10];
        _tcscpy_s(*FormatedObjectName, PTR_STR_LEN, _T(""));

        return FALSE;
    }

    objectNameInfo = malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(
        DupHandle,
        ObjectNameInformation,
        objectNameInfo,
        0x1000,
        &returnLength
    )))
    {
        // Reallocate the buffer and try again. 
        objectNameInfo = realloc(objectNameInfo, returnLength);
        if (!NT_SUCCESS(NtQueryObject(
            DupHandle,
            ObjectNameInformation,
            objectNameInfo,
            returnLength,
            NULL
        )))
        {
            // We have the type name, so just display that.
            free(objectNameInfo);
            return FALSE;
        }
    }

    /*--------------*/

    // Cast our buffer into an UNICODE_STRING
    //objectName = *(PUNICODE_STRING)GetObjectNameThreadParams.objectNameInfo;
    objectName = *(PUNICODE_STRING)objectNameInfo;

    // The object has a name
    if (objectName.Length)
    {
        // free default string
        delete[] * FormatedObjectName;
        *FormatedObjectName = new TCHAR[objectName.Length];
        custom_strcpy_s(*FormatedObjectName, objectName.Length, objectName.Buffer);

    }

    //free(GetObjectNameThreadParams.objectNameInfo);
    free(objectNameInfo);

    if (_tcscmp(TypeName, _T("Process")) == 0) {
        PROCESS_BASIC_INFORMATION pbasicInfo;

        if (!NT_SUCCESS(GetProcessBasicInformation(DupHandle, &pbasicInfo)))
            return FALSE;

        delete[] * FormatedObjectName;
        *FormatedObjectName = new TCHAR[400];

        _stprintf_s(*FormatedObjectName, 400, _T("HandleProcessPid(%llu)"), (ULONG_PTR)pbasicInfo.UniqueProcessId);
        *RemoteProcessId = (ULONG_PTR)pbasicInfo.UniqueProcessId;
    }
    else if (_tcscmp(TypeName, _T("Thread")) == 0) {
        THREAD_BASIC_INFORMATION tbasicInfo;

        if (!NT_SUCCESS(GetThreadBasicInformation(DupHandle, &tbasicInfo)))
            return FALSE;

        delete[] * FormatedObjectName;
        *FormatedObjectName = new TCHAR[400];

        _stprintf_s(*FormatedObjectName, 400, _T("HandleProcessPid(%llu) ThreadId(%llu)"),
            (ULONG_PTR)tbasicInfo.ClientId.UniqueProcess,
            (ULONG_PTR)tbasicInfo.ClientId.UniqueThread);
        *RemoteProcessId = (ULONG_PTR)tbasicInfo.ClientId.UniqueProcess;

    }


    return TRUE;
}

BOOL SetHandleStrings(_Out_ LHF_PHANDLE_DESCRIPTION lhfHhandle) {
    HANDLE processHandle;
    HANDLE dupHandle = NULL;
    POBJECT_TYPE_INFORMATION objectTypeInfo;



    // Setting default Handle type name
    lhfHhandle->TypeString = new TCHAR[PTR_STR_LEN];
    _tcscpy_s(lhfHhandle->TypeString, PTR_STR_LEN, _T(""));

    lhfHhandle->Name = new TCHAR[PTR_STR_LEN];
    _tcscpy_s(lhfHhandle->Name, PTR_STR_LEN, _T(""));

    if (!(processHandle = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, lhfHhandle->UniqueProcessId)))
    {
        return FALSE;
    }


    // Duplicate the handle so we can query it
    if (!NT_SUCCESS(NtDuplicateObject(
        processHandle,
        (HANDLE)lhfHhandle->Handle,
        GetCurrentProcess(),
        &dupHandle,
        0,
        0,
        DUPLICATE_SAME_ACCESS
    )))
    {
        CloseHandle(processHandle);
        return FALSE;
    }

    CloseHandle(processHandle);

    // Query the object type
    objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(
        dupHandle,
        ObjectTypeInformation,
        objectTypeInfo,
        0x1000,
        NULL
    )))
    {
        _tprintf(_T("   [%#x] NtQueryObject!\n"), lhfHhandle->Handle);
        CloseHandle(dupHandle);
        free(objectTypeInfo);
        return FALSE;
    }

    // Setting Handle type name
    /*
    _tcscpy_s(lhfHhandle->TypeString,
        PTR_STR_LEN,
        objectTypeInfo->Name.Buffer);
        */
    //using strcpy_s instead of _tcscpy_s
    custom_strcpy_s(lhfHhandle->TypeString,
        		PTR_STR_LEN,
        		objectTypeInfo->Name.Buffer);

    free(objectTypeInfo);


    FormatObjectName(dupHandle,
        lhfHhandle->TypeString,
        lhfHhandle->GrantedAccess,
        &(lhfHhandle->Name),
        &(lhfHhandle->RemoteProcessId));


    CloseHandle(dupHandle);
    return TRUE;
}

BOOL GetPidIntegrity(_In_ ULONG_PTR UniqueProcessId,
    _Out_ IntegrityLevel* integrityCode,
    _Out_ LPTSTR* integrityStr) {
    HANDLE hToken = NULL;
    DWORD tokenInfoLength = 0;
    LPTSTR fileNameBuffer = new TCHAR[MAX_PATH];
    LPTSTR pathFileName = fileNameBuffer;

    // Setting default Handle type name
    *integrityCode = INTEGRITY_UNKNOWN;
    *integrityStr = new TCHAR[PTR_STR_LEN + MAX_PATH];
    _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("INTEGRITY_UNKNOWN "));

    SetLastError(NULL);
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, UniqueProcessId);

    if (processHandle == NULL) {
        delete[] fileNameBuffer;
        return FALSE;
    }

    //Get process image name
    if (!GetModuleFileNameEx((HMODULE)processHandle, NULL, fileNameBuffer, MAX_PATH)) {
        _tcscpy_s(pathFileName, MAX_PATH, _T(""));
    }
    else
        pathFileName = PathFindFileName(pathFileName);
    lstrcat(*integrityStr, pathFileName);

    bool getToken = OpenProcessToken(processHandle, TOKEN_QUERY, &hToken);

    if (getToken == 0) {
        delete[] fileNameBuffer;
        CloseHandle(processHandle);
        return FALSE;
    }

    if (GetTokenInformation(hToken, TokenIntegrityLevel,
        NULL, 0, &tokenInfoLength) ||
        ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(processHandle);
        CloseHandle(hToken);
        delete[] fileNameBuffer;
        return FALSE;
    }

    TOKEN_MANDATORY_LABEL* tokenLabel = (TOKEN_MANDATORY_LABEL*)malloc(tokenInfoLength);

    if (!GetTokenInformation(hToken, TokenIntegrityLevel,
        tokenLabel, tokenInfoLength, &tokenInfoLength))
    {
        CloseHandle(processHandle);
        CloseHandle(hToken);
        free(tokenLabel);
        delete[] fileNameBuffer;
        return FALSE;
    }

    DWORD integrityLevel = *GetSidSubAuthority(tokenLabel->Label.Sid,
        (DWORD)(UCHAR)(*GetSidSubAuthorityCount(tokenLabel->Label.Sid) - 1));

    CloseHandle(processHandle);
    CloseHandle(hToken);
    free(tokenLabel);

    if (integrityLevel < SECURITY_MANDATORY_LOW_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("UNTRUSTED_INTEGRITY "));
        *integrityCode = UNTRUSTED_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }
    if (integrityLevel < SECURITY_MANDATORY_MEDIUM_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("LOW_INTEGRITY "));
        *integrityCode = LOW_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }

    if (integrityLevel >= SECURITY_MANDATORY_MEDIUM_RID &&
        integrityLevel < SECURITY_MANDATORY_HIGH_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("MEDIUM_INTEGRITY "));
        *integrityCode = MEDIUM_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }

    if (integrityLevel < SECURITY_MANDATORY_SYSTEM_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("HIGH_INTEGRITY "));
        *integrityCode = HIGH_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }

    if (integrityLevel < SECURITY_MANDATORY_PROTECTED_PROCESS_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("SYSTEM_INTEGRITY "));
        *integrityCode = SYSTEM_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }

    if (integrityLevel >= SECURITY_MANDATORY_PROTECTED_PROCESS_RID) {
        _tcscpy_s(*integrityStr, PTR_STR_LEN + MAX_PATH, _T("PPL_INTEGRITY "));
        *integrityCode = PPL_INTEGRITY;
        lstrcat(*integrityStr, pathFileName);
        delete[] fileNameBuffer;
        return TRUE;
    }

    delete[] fileNameBuffer;
    return FALSE;
}

BOOL GetParentPid(_In_ ULONG_PTR pid,
    _Out_ PULONG_PTR ppid) {
    ULONG returnLength = 0;
    ULONG status = 0;
    ULONG ProcessInfoSizeEx = sizeof(PROCESS_BASIC_INFORMATION);

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);

    if (processHandle == NULL) {
        return FALSE;
    }

    PVOID ProcessInformation = malloc(ProcessInfoSizeEx);

    while ((status = NtQueryInformationProcess(
        processHandle,
        ProcessBasicInformation,
        ProcessInformation,
        ProcessInfoSizeEx,
        &returnLength
    )) == STATUS_INFO_LENGTH_MISMATCH) {
        ProcessInformation = realloc(ProcessInformation, ProcessInfoSizeEx *= 2);
        if (ProcessInformation == NULL)
            break;
    }

    if (!NT_SUCCESS(status) || ProcessInformation == NULL)
    {
        _tprintf(_T("   [-] NtQueryInformationProcess failed!\n"));
        free(ProcessInformation);
        CloseHandle(processHandle);
        return FALSE;
    }

    PPROCESS_BASIC_INFORMATION pbi = (PPROCESS_BASIC_INFORMATION)ProcessInformation;
    *ppid = (ULONG_PTR)pbi->InheritedFromUniqueProcessId;
    free(ProcessInformation);
    CloseHandle(processHandle);

    return TRUE;
}

BOOL GetLhfHandles(void* in, _Out_ void* out) {
    //cast to our own structure
    PSYSTEM_HANDLE_INFORMATION_EX systemHandlesList = (PSYSTEM_HANDLE_INFORMATION_EX)in;
    LHF_PHANDLE_DESCRIPTION_LIST* lhfHandles = (LHF_PHANDLE_DESCRIPTION_LIST*)out;

    size_t handleIndex = 0;

    *lhfHandles = (LHF_PHANDLE_DESCRIPTION_LIST)malloc(sizeof(LHF_HANDLE_DESCRIPTION_LIST));

    // Getting unmber of inherited handles
    (*lhfHandles)->NumberOfLhfHandles = IhCount(systemHandlesList);

    // There´s no interesting handles 
    if ((*lhfHandles)->NumberOfLhfHandles == 0)
        return FALSE;

    // Allocating memory for our own lhf list of handles structure
    (*lhfHandles)->Handles = (LHF_PHANDLE_DESCRIPTION)malloc(sizeof(LHF_HANDLE_DESCRIPTION) * ((*lhfHandles)->NumberOfLhfHandles));

    for (int i = 0; i < systemHandlesList->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &systemHandlesList->Handles[i];
        if (handleInfo->HandleAttributes & (HANDLE_INHERIT)) {

            // Case Process Handle: this is the handle of remote process inherited, default none.
            (*lhfHandles)->Handles[handleIndex].RemoteProcessId = -1;
            (*lhfHandles)->Handles[handleIndex].UniqueProcessId = handleInfo->UniqueProcessId;
            (*lhfHandles)->Handles[handleIndex].Handle = handleInfo->HandleValue;
            (*lhfHandles)->Handles[handleIndex].GrantedAccess = handleInfo->GrantedAccess;
            (*lhfHandles)->Handles[handleIndex].ObjectTypeIndex = handleInfo->ObjectTypeIndex;

            //(*lhfHandles)->Handles[handleIndex].GrantedAccessString;
            //https://github.com/processhacker/processhacker/blob/e96989fd396b28f71c080edc7be9e7256b5229d0/phlib/secdata.c

            SetHandleStrings(&((*lhfHandles)->Handles[handleIndex]));

            GetPidIntegrity((*lhfHandles)->Handles[handleIndex].UniqueProcessId,
                &((*lhfHandles)->Handles[handleIndex].ProcessIntegrity),
                &((*lhfHandles)->Handles[handleIndex].ProcessIntegrityString)
            );

            if (!GetParentPid((*lhfHandles)->Handles[handleIndex].UniqueProcessId,
                &((*lhfHandles)->Handles[handleIndex].ParentUniqueProcessId)))
            {
                (*lhfHandles)->Handles[handleIndex].ParentUniqueProcessId = -1;
            }

            GetPidIntegrity((*lhfHandles)->Handles[handleIndex].ParentUniqueProcessId,
                &((*lhfHandles)->Handles[handleIndex].ParentProcessIntegrity),
                &((*lhfHandles)->Handles[handleIndex].ParentProcessIntegrityString)
            );

            handleIndex++;
        }
    }
    return TRUE;
}

void LhfFree(_In_ LHF_PHANDLE_DESCRIPTION_LIST lhfHandles) {

    if (lhfHandles == NULL) {
        return;
    }
    for (unsigned int i = 0; i < lhfHandles->NumberOfLhfHandles; i++)
    {
        //delete(lhfHandles->Handles[i].GrantedAccessString);
        delete[] lhfHandles->Handles[i].ProcessIntegrityString;
        delete[] lhfHandles->Handles[i].Name;
        delete[] lhfHandles->Handles[i].TypeString;
        delete[] lhfHandles->Handles[i].ParentProcessIntegrityString;
    }

    free(lhfHandles->Handles);
    free(lhfHandles);
}

std::string getWindowsUsername()
{
    char username_buffer[UNLEN + 1];
    DWORD buffer_size = sizeof(username_buffer);

    if (GetUserName(username_buffer, &buffer_size)) {
        return std::string(username_buffer);
    }
    else {
        return "";
    }
}

const std::string windowsUsername = getWindowsUsername();
const std::string robloxPath = "C:\\Users\\" + windowsUsername + "\\AppData\\Local\\Roblox\\logs\\";
std::vector<std::string> Functions::getLogFiles()
{
    std::vector<std::string> logFiles;
    PSYSTEM_HANDLE_INFORMATION_EX systemHandlesList;
    LHF_PHANDLE_DESCRIPTION_LIST lhfHandles = NULL;

    if (!NT_SUCCESS(getAllSystemHandlers(&systemHandlesList))) {
        _tprintf(_T("   [-] GetAllSystemHandlers failed!\n"));
        exit(1);
    }

    if (!GetLhfHandles(systemHandlesList, &lhfHandles)) {
        _tprintf(_T("   [-] GetLhfHandles failed!\n"));
        exit(1);
    }

    free(systemHandlesList);

    for (unsigned int i = 0; i < lhfHandles->NumberOfLhfHandles; i++)
    {
        //Detection rules
        if (
            (std::string(lhfHandles->Handles[i].ProcessIntegrityString).find("RobloxPlayerBeta.exe") != std::wstring::npos) &&
            (std::string(lhfHandles->Handles[i].Name).find("\\Device\\Afd") == std::wstring::npos) &&
            (std::string(lhfHandles->Handles[i].TypeString).find("Mutant") == std::wstring::npos) &&
            (std::string(lhfHandles->Handles[i].TypeString).find("Thread") == std::wstring::npos)

            ) {

            std::regex re(R"(([\d+.]+_\w+Z_Player_\w+_last.log))");
            std::string str(lhfHandles->Handles[i].Name);
            std::smatch match;
            if (std::regex_search(str, match, re)) {
                //logFiles.push_back("C:" + match[1].str());
                logFiles.push_back(robloxPath + match[1].str());
            }
        }
    }

    LhfFree(lhfHandles);

    return logFiles;
}