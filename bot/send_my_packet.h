#ifndef SEND_MY_PACKET_H
#define SEND_MY_PACKET_H

int send_login_packet();
int send_2nd_password();
int send_character_select(int id);

void send_pl_attack_monster(int id);
void send_attack_all_monsters_attacking_me();

void send_pl_skill_no_pre(int monster_id, int skill_id);
void send_pl_skill(int monster_id, int animation_time, int skill_id);
void send_fast_skill(int monster_id, int skill_id);
void send_buff_skill(int skill_id);

void send_use_item(int item_id);

void send_pick_item(int id, int X, int Y);
void send_pl_pick_all_items();
void send_fast_pick_all_items();

void send_rest_start();
void send_rest_stop();

void use_item(int item_id);
void use_medium_medicine();


void send_pl_move_to_loc(int X, int Y, int ending_distance);
void send_fast_move_to_loc(int X, int Y, int ending_distance);

#endif // SEND_MY_PACKET_H