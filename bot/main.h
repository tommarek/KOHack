#ifndef MAIN_H
#define MAIN_H

#include <Windows.h>
#include <vector>

using namespace std;

// global variables for sending
extern int g_socket;
extern int g_flags;

int WINAPI my_send(SOCKET socket, vector<char> *packet, int flags);
void fake_recv_packet(vector<char> *packet);

#endif // MAIN_H