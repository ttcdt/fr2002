/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Map data and code

*/

#define MAX_BLOCK_PATTERNS  64
#define MAX_GATES_PER_ROOM  24
#define MAX_DEMONS_PER_ROOM 80
#define MAX_GATES_PER_EPISODE   150
#define MAX_DEMONS_PER_EPISODE  768
#define MAX_ROOMS_PER_EPISODE   64
#define MAX_BULLETS     16
#define MAX_BMP_WALLS       256
#define MAX_BMP_DEMONS      128
#define MAX_ITEMS       10

/* a block pattern */
struct block
{
    int face[4];
};

/* a gate */
struct gate
{
    struct filp_val * cross_callback;
    int bx;
    int by;
    double x;
    double y;
    int in_room;
    int to_room;
    double to_x;
    double to_y;
};

/* demon modes */
#define MODE_MOVE   0
#define MODE_ATTACK 1
#define MODE_BLEED  2
#define MODE_DEAD   3
#define MODE_FROZEN 4
#define MODE_GONE   5

/* demon types */
#define TYPE_STATIC 0
#define TYPE_FOLLOW 1
#define TYPE_SHOOT  2

/* a demon */
struct demon
{
    int bmp;
    int strength;
    int speed;
    int type;
    int mode;
    int health;
    int step;
    int ticks;
    int reload;
    int offset;
    int oblivion;
    int attack;
    double x;
    double y;
    double a;
    double a_bias;
};

/* a bullet */
struct bullet
{
    int from_player;
    double x;
    double y;
    double a;
};

/* a room */
struct room
{
    int in_use;
    unsigned char floor_color;      /* sobra? */
    unsigned char ceiling_color;        /* sobra? */
    int tx;
    int ty;
    int gates[MAX_GATES_PER_ROOM];
    int demons[MAX_DEMONS_PER_ROOM];
    struct block * map;
    struct filp_val * enter_callback;
};

/* an item */
struct item
{
    int room;
    int taken;
    int x;
    int y;
};

extern int _episode;

extern struct demon _demons[MAX_DEMONS_PER_EPISODE];
extern struct bullet _bullets[MAX_BULLETS];

extern unsigned char * _bmp_walls[MAX_BMP_WALLS];
extern unsigned char * _bmp_demons[MAX_BMP_DEMONS];
extern unsigned char * _bmp_items[MAX_ITEMS];

extern unsigned char _player_bullet[BITMAP_WIDTH * BITMAP_HEIGHT];
extern unsigned char _demon_bullet[BITMAP_WIDTH * BITMAP_HEIGHT];

extern struct room _rooms[MAX_ROOMS_PER_EPISODE];
extern struct room * _crp;

extern struct gate _gates[MAX_GATES_PER_EPISODE];

extern struct item _items[MAX_ITEMS];
extern int _num_items;
extern int _item;

extern double _player_x;
extern double _player_y;
extern double _player_a;
extern int _player_health;
extern double _player_speed;
extern double _player_angle_speed;
extern int _killable_demons;
extern int _adventure_total;
extern int _ticks_per_step;
extern int _room_ambient_light;
extern int _win_if_demon_0;

extern char _room_name[80];

#define MAP_BLOCK_PTR(bx,by) &_crp->map[(_crp->tx*(by))+(bx)]

#define LOGO_WIDTH 280
#define LOGO_HEIGHT 56

extern unsigned char _logo[LOGO_WIDTH * LOGO_HEIGHT];

/* sounds */

extern int _p_shoot_sound;
extern int _d_hurt_sound;
extern int _d_die_sound;
extern int _p_hurt_sound;
extern int _p_die_sound;
extern int _d_see_sound;
extern int _b_crash_sound;
extern int _pick_item_sound;
extern int _pick_life_sound;
extern int _soundtrack;

unsigned long rnd(unsigned long * seed);
void set_current_room(int room);
void map_startup(void);
