#ifndef TABLE_DECRYPT_ENCRYPT_H
#define TABLE_DECRYPT_ENCRYPT_H

#include <vector>

using namespace std;

void table_decode(int key, vector<char> *packet);
void table_encode(int key, vector<char> *packet);

#endif // TABLE_DECRYPT_ENCRYPT_H
