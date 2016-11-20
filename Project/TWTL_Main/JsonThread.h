#pragma once

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam);
unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam);

