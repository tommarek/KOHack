#include <Windows.h>
#include <vector>
#include <array>
#include <iostream>

#include "aes.h"
#include "HsCryptLib.h"
//#include "aes_data.h"

#pragma comment (lib, "HsCryptLib.lib")

using namespace std;

// flag determining whether aes has been set
int aes_set = FALSE;

// INIX XOR DECRYPTION OF AES KEY
// XOR Key for Welcome Packet.. using this key the aes key is encrypted/decrypted
const unsigned char INIX_AES_KEY_XOR[52] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xAC, 0xAC, 0x8A, 0xAC,
	0xAC, 0xAC, 0x8A, 0xAC, 0xAC, 0xAC, 0x8A, 0xAC, 0xAC, 0xAC,
	0x8A, 0xAC, 0xAC, 0xAC, 0x8A, 0xAC, 0xC2, 0xC2, 0xFE, 0xC2,
	0xC2, 0xC2, 0xFE, 0xC2, 0xC2, 0xC2, 0xFE, 0xC2, 0xC2, 0xC2,
	0xFE, 0xC2, 0xC2, 0xC2, 0xFE, 0xC2, 0xC2, 0xB0, 0xB0, 0xB0,
	0xEA, 0xEA
};

// Position of AES-Key, in Welcome Packet
const unsigned char AES_KEY_POS[2][16] = {
	{ 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19, 21, 22, 23, 25 },
	{ 26, 27, 29, 30, 31, 33, 34, 35, 37, 38, 39, 41, 42, 43, 45, 46 }
};

// decrypt aes key from the welcome packet
void _aes_init_inix_key(array<char, 52> &aes_crypted_key, array<char, 16> &aes_decrypted_key){
	//xor InixKey
	for (unsigned i = 0; i < 52; ++i){
		aes_crypted_key[i] = aes_crypted_key[i] ^ INIX_AES_KEY_XOR[i];
	}
	// pick the right values that will be used for aes
	for (unsigned i = 0; i < 16; ++i){
		aes_decrypted_key[i] = (aes_crypted_key[AES_KEY_POS[0][i]] = aes_crypted_key[AES_KEY_POS[1][i]]);
	}
}

HSCRYPT_KEYINFO HsKeyInfo;

// generate session AES keys
DWORD aes_generate_keys(array<char, 52> &aes_crypted_key){
	array<char, 16> aes_decrypted_key;

	// decrypt aes key
	_aes_init_inix_key(aes_crypted_key, aes_decrypted_key);
	memcpy(HsKeyInfo.byInitKey, aes_decrypted_key.data(), 16);

	aes_set = TRUE;
	return _HsCrypt_InitCrypt(&HsKeyInfo);
}


void decrypt_recv_packet_aes(vector<char> *packet){
	_HsCrypt_GetDecMsg((unsigned char*)packet->data() + 3, packet->size() - 3, HsKeyInfo.AesDecKey, (unsigned char*)packet->data() + 3);
}

void decrypt_send_packet_aes(vector<char> *packet){
	//same as
	decrypt_recv_packet_aes(packet);
}


void encrypt_send_packet_aes(vector<char> *packet){
	_HsCrypt_GetEncMsg((unsigned char*)packet->data() + 3, packet->size() - 3, HsKeyInfo.AesEncKey, (unsigned char*)packet->data() + 3);
}

void encrypt_recv_packet_aes(vector<char> *packet){
	//same as
	encrypt_send_packet_aes(packet);
}