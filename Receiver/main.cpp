#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Message.h"

using namespace std;

const int MAX_SENDERS = 10;
const wchar_t* MUTEX_NAME = L"GlobalMsgMutex";

HANDLE mutex;
string filename;
int messageCount;
int senderCount;
HANDLE readyEvents[MAX_SENDERS];
PROCESS_INFORMATION senderInfos[MAX_SENDERS];
int head = 0;

void createBinaryFile() {
    fstream file(filename, ios::out | ios::binary);
    Message msg;
    msg.isEmpty = true;
    for (int i = 0; i < messageCount; ++i) {
        file.write((char*)&msg, sizeof(Message));
    }
    file.close();
}

void startSenders() {
    for (int i = 0; i < senderCount; ++i) {
        wstringstream evName;
        evName << L"EventReady_" << i;
        readyEvents[i] = CreateEventW(NULL, TRUE, FALSE, evName.str().c_str());

        wstringstream cmd;
        cmd << L"Sender.exe " << filename.c_str() << L" " << i << L" " << messageCount;

        STARTUPINFOW si = { sizeof(si) };
        ZeroMemory(&senderInfos[i], sizeof(PROCESS_INFORMATION));

        if (!CreateProcessW(NULL, (LPWSTR)cmd.str().c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &senderInfos[i])) {
            cerr << "[Receiver] Failed to start Sender " << i << endl;
        }
    }
}

void waitSendersReady() {
    WaitForMultipleObjects(senderCount, readyEvents, TRUE, INFINITE);
    cout << "[Receiver] All Senders are ready." << endl;
}

void readMessage() {
    WaitForSingleObject(mutex, INFINITE);

    fstream file(filename, ios::in | ios::out | ios::binary);
    Message msg;
    bool found = false;
    int start = head;

    for (int i = 0; i < messageCount; ++i) {
        int pos = (start + i) % messageCount;
        file.seekg(pos * sizeof(Message));
        file.read((char*)&msg, sizeof(Message));

        if (!msg.isEmpty) {
            cout << "[Receiver] Message: " << msg.text << endl;
            msg.isEmpty = true;
            file.seekp(pos * sizeof(Message));
            file.write((char*)&msg, sizeof(Message));

            head = (pos + 1) % messageCount;
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "[Receiver] No new messages.\n";
    }

    file.close();
    ReleaseMutex(mutex);
}


int main() {
    cout << "Enter binary file name: ";
    cin >> filename;
    cout << "Enter number of message slots: ";
    cin >> messageCount;

    mutex = CreateMutexW(NULL, FALSE, MUTEX_NAME);
    createBinaryFile();

    cout << "Enter number of Sender processes (max " << MAX_SENDERS << "): ";
    cin >> senderCount;

    startSenders();
    waitSendersReady();

    string cmd;
    while (true) {
        cout << "[Receiver] Enter command (read/exit): ";
        cin >> cmd;
        if (cmd == "read") readMessage();
        else if (cmd == "exit") break;
    }

    for (int i = 0; i < senderCount; ++i) {
        TerminateProcess(senderInfos[i].hProcess, 0);
        CloseHandle(senderInfos[i].hProcess);
        CloseHandle(senderInfos[i].hThread);
    }

    CloseHandle(mutex);
    return 0;
}
