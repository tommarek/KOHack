#include <Windows.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <array>
#include <list>

#include "handle_recieved_packets.h"
#include "handle_sent_packets.h"
#include "aes.h"
#include "table_decrypt_encrypt.h"
#include "main.h"
#include "packets.h"
#include "enviroment.h"

#include "process_packet.h"

using namespace std;

char* packet_offset;
DWORD U = 0;

// update the placeholder for size of the packet
void update_packet_size(vector<char> &packet){
	WORD size = packet.size();
	memcpy(packet.data(), &size, sizeof(size));
}

void set_U(int appender){
	U = ((appender >> 6) - (appender >> 15 << 9)) ^ 0x7E4D;
}

vector<char> extract_packet(vector<char> *buffer, int size){
	vector<char> packet(buffer->data(), buffer->data() + size);
	buffer->erase(buffer->begin(), buffer->begin() + size);

	return packet;
}

int check_for_packet(vector<char> *buffer){
	if (buffer->size() < 3){
		return -1;
	}

	// packet offset for reading
	reader r(&buffer->front());

	// SIZE
	short size;
	r.read(size);
	if (buffer->size() < size){
		return -1;
	}
	
	// return size of the packet or -1 if no packe found
	return size;
}


// process recieved packet
void process_recv_packet(vector<char> *packet){
	//init reader
	reader r(&packet->front());

	short size;
	char type;
	r.read(size);
	r.read(type);

	//cout << "got packet type: 0x" << hex << (int)type << endl;

	// process packet according to type
	switch (type) {
	case S2C_PLAYERINFO: // 0x11 login OK + info about characters to choose
		handle_second_pass_ok(packet, r);
		break;
	case S2C_ANS_LOGIN: // 0x2B login succeded
		handle_first_pass_ok(packet, r);
		break;
	case 0x22: // MOVEPLAYER_ON
		handle_move_player(packet, r);
		break;
	case 0x23: // MOVEPLAYER_END
		handle_move_player(packet, r);
		break;
	case 0x24: // MOVEMONSTER_ON
		handle_move_monster(packet, r);
		break;
	case 0x25: // MOVEMONSTER_END
		handle_move_monster(packet, r);
		break;
	case S2C_CREATEPLAYER: // 0x32 Player Appear
		handle_player_appeared_packet(packet, r);
		break;
	case S2C_CREATEMONSTER: // 0x33 Monster Appear
		handle_monster_appeared_packet(packet, r);
		break;
	case S2C_CREATEITEM: // 0x36 Item Drop
		handle_item_appeared_packet(packet, r);
		break;
	case S2C_REMOVEPLAYER:// 0x37 Player disappeared
		handle_player_disappeared_packet(packet, r);
		break;
	case S2C_REMOVEMONSTER: // 0x38 Monster disappeared
		handle_monster_disappeared_packet(packet, r);
		break;
	case S2C_REMOVEITEM: // 0x3B drop disappeared
		handle_item_disappeared_packet(packet, r);
		break;
	case S2C_ACTION: // 0x3d Mob Died
		handle_action_packet(packet, r);
		break;
	case S2C_ATTACK: // 0x3e attack packet
		handle_attack_packet(packet, r);
		break;
	case 0x42: // player stats
		break;
	case C2S_TELEPORT: // teleport packet
		handle_teleport_packet(packet, r);
		break;
	case S2C_UPDATEPROPERTY: // 0x45 update property - HP/MP
		process_update_property(packet, r);
		break;
	case S2C_HACKSHEILDCRC: // 0x6b S2C_HACKSHEILDCRC
		handle_u_update(packet, r);
		break;
	default:
		//cout << "recieved the packet type: 0x" << hex << (int)type << ", with lenght: " << dec << size << endl;
		break;
	}
	return;
}

// process sent packet
int process_send_packet(vector<char> *buffer){
	//init reader
	reader r1(&buffer->front());

	WORD size;
	r1.read(size);

	// create packet and remove it from the buffer
	vector<char> packet(buffer->data(), buffer->data() + size);
	buffer->erase(buffer->begin(), buffer->begin() + size);

	// set new offset in the packet
	reader r(&packet.front() + 2); // size was allready red -> +2

	// DECRYPT
	table_decode(send_key_engine, &packet);
	if (aes_set){
		decrypt_send_packet_aes(&packet);
	}

	// TYPE
	char type;
	r.read(type);

	//cout << "sent packet type 0x" << hex << (int)type << endl;

	switch (type) {
	case C2S_CHATTING: // 0x0E Format: Us - Chat
		handle_sent_chat_message(packet, r);
		break;
	case C2S_LOGIN: // 0x02 Format: Uss - Login
		break;
	case 0x75: // 0x75 Format: Ubs - 2nd Pass
		break;
	case C2S_ANS_CODE: // 0x9 Format: Ubd - Check Versions
		handle_check_version_packet(packet, r);
		break;
	case C2S_LOADPLAYER: // 0x0a Format: Uddd - LoadPlayer (player_id, 0, 1)
		handle_load_player(packet, r);
		break;
	case C2S_ATTACK: // 0x0c Format : Ubdd - Attack
		handle_attack(packet, r);
		break;
	case C2S_PRESKILL: // 0x0dFormat : Ubbd - Attack with Skill
		handle_preskill(packet, r);
		break;
	case C2S_SKILL: // 0x0dFormat : Ubbd - Attack with Skill
		handle_skill_attack(packet, r);
		break;
	case C2S_MOVE_ON: // 0x11 Format : Ubbb - Move
		handle_move_packet(packet, r);
		break;
	case C2S_MOVE_END: // 0x12 Format: Ubbb - MoveStop
		handle_move_end_packet(packet, r);
		break;
	case C2S_REST: // 0x1c Format: Ub - Rest
		handle_rest_packet(packet, r);
		//cout << "sent: Rest" << endl;
		break;
	case 0x1d: // Format : Uddd - Pick Item
		//cout << "sent: Pick Item" << endl;
		break;
	case C2S_USEITEM: // 0x1e Format: Ud - Use Item
		handle_use_item_packet(packet, r);
		break;
	default:
		handle_other_sent_packet(packet, r);
		//cout << "sent packet type 0x" << hex << (int)type << endl;
		break;
	}

	// sending the packet and returning its value
	return my_send(g_socket, &packet, g_flags);
}