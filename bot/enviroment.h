#ifndef ENVIROMENT_H
#define ENVIROMENT_H

#include <map>
#include <array>
#include <vector>

using namespace std;

enum E_state{
	E_STATE_NOENEMY,
	E_STATE_MOVING,
	E_STATE_MOVE_STOPPED,
	E_STATE_ATTACKING,
	E_STATE_KILLED_BEHEADABLE_MONSTER,
	E_STATE_MONSTER_BEHEADED,
	E_STATE_KILLED_MONSTER,
	E_STATE_PICKING,
	E_STATE_PAUSED,
	E_STATE_EMERGENCY,
	E_STATE_HEALER,
	E_STATE_STOPPED,
};

enum E_skill{
	E_SKILL_ATTACK,
	E_SKILL_EMERGENCY,
	E_SKILL_BUFF,
	E_SKILL_HEAL,
};

enum E_monster{
	E_MONSTER_PASSIVE,
	E_MONSTER_ATTACKING,
	E_MONSTER_KNEELING,
};

/////////////////////////////////////////////////////////////////////////////////////
// Map class 
/////////////////////////////////////////////////////////////////////////////////////
class Skill
{
private:
	int last_time_used;
public:
	int ID, cooldown, animation_time, mana_cost, type;

	Skill();
	Skill(byte skill_ID, int skill_cooldown, int skill_anim_time, int skill_mana, int skill_type);

	bool ready();
	void used();
};

/////////////////////////////////////////////////////////////////////////////////////
// Map class 
/////////////////////////////////////////////////////////////////////////////////////
class Map
{
private:
	int map_size = 0xFF;

public:
	Map(); 

	int get_height(int x, int y);
	double Map::linear_interpolation(double p1, double p2, double position);
};

/////////////////////////////////////////////////////////////////////////////////////
// Entity class - for each object
/////////////////////////////////////////////////////////////////////////////////////
class Entity
{
private:
public:
	int ID = -1;
	int X, Y, Z;

	// initialisation
	void init_entity(int new_id, int new_X, int new_Y, int new_Z);

	// position and distance
	void set_position(int new_X, int new_Y, int new_Z);
	double get_distance(int my_X, int my_Y);

	// id
	void set_id(int id);
	int get_id();

};

/////////////////////////////////////////////////////////////////////////////////////
// Item class 
/////////////////////////////////////////////////////////////////////////////////////
class Item : public Entity
{
private:
public:
	// initialisation
	Item();
	Item(int new_id, int new_X, int new_Y);
};

/////////////////////////////////////////////////////////////////////////////////////
// Item Living 
/////////////////////////////////////////////////////////////////////////////////////
class Living : public Entity
{
private:
public:
	int target_ID = -1;

	int HP, max_HP;
	int MP, max_MP;

	// initialisation
	void init_living(int new_id, int new_X, int new_Y, int new_Z, int new_HP, int new_max_HP, int new_MP, int new_max_MP);

	// health
	void set_hp(int hp);
	int add_to_hp(int delta);
	int get_hp();

	// mana
	void set_mp(int mp);
	int add_to_mp(int delta);
	int get_mp();

};

/////////////////////////////////////////////////////////////////////////////////////
// Monster class
/////////////////////////////////////////////////////////////////////////////////////
class Monster : public Living
{
private:
public:
	int state;

	Monster();
	Monster(int new_id, int new_X, int new_Y, int new_Z, int new_HP, int new_max_HP);

	// target
	void set_target(int id);
	int get_target();

};

/////////////////////////////////////////////////////////////////////////////////////
// Player class
/////////////////////////////////////////////////////////////////////////////////////
class Player : public Living
{
private:
public:
	int Z, dir;
	string name;

	Player();
	Player(int new_id, int new_X, int new_Y, int new_Z, string new_name);
};


/////////////////////////////////////////////////////////////////////////////////////
// Bot class
/////////////////////////////////////////////////////////////////////////////////////
class Bot : public Living
{
private:

public:
	Map heightmap;
	int dir;
	int start_X = 0, start_Y = 0;

	int state = E_STATE_STOPPED;

	int last_move_time = GetTickCount();
	int last_attack_time = GetTickCount();
	int last_med_time = GetTickCount();
	// a little defensife change because the bot gets stuck from time to time
	int mob_target_time;

	map<byte, Skill> skills;

	map<unsigned, Monster> monsters;
	map<unsigned, Player> players;
	map<unsigned, Item> items;
	vector<int> targets;

	Bot();

	void setup();

	double get_dist_to_monster(int id);
	double get_dist_to_item(int id);
	double get_dist_to_player(int id);

	bool nobody_is_around();
	int get_closest_monster_id();
	int get_centered_monster_id(double div);
	bool is_target_valid(int mob_id);
	int get_closest_item_id();

	byte get_attack_skill();
	byte get_buff_skill();

	int get_target();

	void cleanup();

	static Bot* instance();
};


#endif // ENVIROMENT_H