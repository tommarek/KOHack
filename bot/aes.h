#ifndef AES_H
#define AES_H

#include <array>
#include <vector>

using namespace std;

// global variables for encryption
extern int recv_key;
extern int send_key;
extern int send_key_engine;
extern int aes_set;

DWORD aes_generate_keys(array<char, 52> &aes_crypted_key);

void decrypt_recv_packet_aes(vector<char> *packet);

void encrypt_send_packet_aes(vector<char> *packet);
void decrypt_send_packet_aes(vector<char> *packet);

#endif // AES_H