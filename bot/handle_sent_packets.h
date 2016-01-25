#ifndef HANDLE_SENT_PACKETS_H
#define HANDLE_SENT_PACKETS_H

#include <vector>

using namespace std;

void handle_check_version_packet(vector<char> packet, reader r);
void handle_load_player(vector<char> packet, reader r);
void handle_attack(vector<char> packet, reader r);
void handle_preskill(vector<char> packet, reader r);
void handle_skill_attack(vector<char> packet, reader r);

void handle_move_packet(vector<char> packet, reader r);
void handle_move_end_packet(vector<char> packet, reader r);

void handle_rest_packet(vector<char> packet, reader r);

void handle_use_item_packet(vector<char> packet, reader r);

int handle_teleport_packet(vector<char> packet, reader r);

void handle_sent_chat_message(vector<char> packet, reader r);


void handle_other_sent_packet(vector<char> packet, reader r);

#endif // HANDLE_SENT_PACKETS_H