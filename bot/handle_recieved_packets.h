#ifndef HANDLE_RECIEVED_PACKETS_H
#define HANDLE_RECIEVED_PACKETS_H

#include <vector>

#include "process_packet.h"

using namespace std;

void handle_welcome_packet(vector<char> *packet, reader r);

void handle_monster_appeared_packet(vector<char> *packet, reader r);
void handle_monster_disappeared_packet(vector<char> *packet, reader r);
void handle_player_appeared_packet(vector<char> *packet, reader r);
void handle_player_disappeared_packet(vector<char> *packet, reader r);
void handle_item_appeared_packet(vector<char> *packet, reader r);
void handle_item_disappeared_packet(vector<char> *packet, reader r);

void handle_move_monster(vector<char> *packet, reader r);
void handle_move_player(vector<char> *packet, reader r);

void process_update_property(vector<char> *packet, reader r);

void handle_action_packet(vector<char> *packet, reader r);

void handle_attack_packet(vector<char> *packet, reader r);

void handle_teleport_packet(vector<char> *packet, reader r);

void handle_u_update(vector<char> *packet, reader r);
void handle_first_pass_ok(vector<char> *packet, reader r);
void handle_second_pass_ok(vector<char> *packet, reader r);

#endif // HANDLE_RECIEVED_PACKETS_H