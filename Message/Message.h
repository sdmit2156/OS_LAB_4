#pragma once
#include <windows.h>

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isEmpty;
};
