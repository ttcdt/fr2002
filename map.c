/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Map managing and loader code

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "filp.h"
#include "qdgdf_video.h"
#include "qdgdf_audio.h"

#include "render.h"
#include "map.h"
#include "game.h"

#ifndef DATA_DIR
#define DATA_DIR "./data"
#endif

int _episode;

struct block _blocks[MAX_BLOCK_PATTERNS];
int _num_blocks = 0;

struct gate _gates[MAX_GATES_PER_EPISODE];
int _num_gates = 0;

struct demon _demons[MAX_DEMONS_PER_EPISODE];
int _num_demons = 0;

struct item _items[MAX_ITEMS];
int _num_items = 0;

struct bullet _bullets[MAX_BULLETS];

struct room _rooms[MAX_ROOMS_PER_EPISODE];
int _current_room = 0;
struct room *_crp;

unsigned char *_bmp_walls[MAX_BMP_WALLS];
int _num_bmp_walls = 0;

unsigned char *_bmp_demons[MAX_BMP_DEMONS];
int _num_bmp_demons = 0;

unsigned char *_bmp_items[MAX_ITEMS];
int _num_bmp_items = 0;
int _item = -1;

/* player data */
double _player_x;
double _player_y;
double _player_a;
int _player_health;
double _player_speed;
double _player_angle_speed;
int _adventure_total;

/* the name of the episode */
char _episode_name[120];

/* the name of the room */
char _room_name[80];

/* count of killable demons */
int _killable_demons = 0;

/* ticks per step */
int _ticks_per_step = 250;

/* player wins if demon 0 dies flag */
int _win_if_demon_0 = 0;

/* the bullet bitmaps */
unsigned char _player_bullet[BITMAP_WIDTH * BITMAP_HEIGHT];
unsigned char _demon_bullet[BITMAP_WIDTH * BITMAP_HEIGHT];

/* sounds */

int _p_shoot_sound = -1;
int _d_hurt_sound = -1;
int _d_die_sound = -1;
int _p_hurt_sound = -1;
int _p_die_sound = -1;
int _d_see_sound = -1;
int _b_crash_sound = -1;
int _pick_item_sound = -1;
int _pick_life_sound = -1;
int _soundtrack = -1;

/* the logo / title */
unsigned char _logo[LOGO_WIDTH * LOGO_HEIGHT];

unsigned char _fog_red;
unsigned char _fog_green;
unsigned char _fog_blue;

int _room_ambient_light;


/******************
    Code
*******************/

/**
 * rnd - Special random function
 * @seed: pointer to the random seed
 *
 * Special random function that allows multiple random seeds.
 * If seed is NULL, a special internal seed is used.
 * Returns a 16 bit random number.
 */
unsigned long rnd(unsigned long *seed)
{
    static unsigned long any_seed;

    if (seed == NULL)
        seed = &any_seed;

    *seed = (*seed * 58321) + 11113;
    return *seed >> 16;
}


#define D_BITMAP_WIDTH  64
#define D_BITMAP_HEIGHT 64

/**
 * adapt_bitmap - rotates and converts the bitmap
 * @des: destination buffer
 * @org: origin buffer
 *
 * Rotates and converts the palette of a bitmap.
 * Both buffers must have BITMAP_WIDTH * BITMAP_HEIGHT size.
 */
static void adapt_bitmap(unsigned char *des, const unsigned char *org)
{
    int n, m;
    int c;

    for (n = 0; n < D_BITMAP_HEIGHT; n++) {
        for (m = 0; m < D_BITMAP_WIDTH; m++) {
            c = *(org + n * D_BITMAP_WIDTH + m);
            *(des + m * BITMAP_WIDTH + n) = _pal_conv[c];
        }
    }
}


static void adjust_gates(void)
{
    struct block *b;
    struct gate *g;
    int n;

    /* travel the gates, moving the position to
       the nearest wall */

    for (n = 0; n < MAX_GATES_PER_ROOM && _crp->gates[n] != -1; n++) {
        g = &_gates[_crp->gates[n]];

        g->x = (g->bx * CELL_WIDTH) + (CELL_WIDTH / 2);
        g->y = (g->by * CELL_WIDTH) + (CELL_WIDTH / 2);

        b = MAP_BLOCK_PTR(g->bx - 1, g->by);

        if (b->face[0] != -1 || b->face[1] != -1 ||
            b->face[2] != -1 || b->face[3] != -1) {
            g->x -= (CELL_WIDTH / 2);
            continue;
        }

        b = MAP_BLOCK_PTR(g->bx + 1, g->by);

        if (b->face[0] != -1 || b->face[1] != -1 ||
            b->face[2] != -1 || b->face[3] != -1) {
            g->x += (CELL_WIDTH / 2);
            continue;
        }

        b = MAP_BLOCK_PTR(g->bx, g->by - 1);

        if (b->face[0] != -1 || b->face[1] != -1 ||
            b->face[2] != -1 || b->face[3] != -1) {
            g->y -= (CELL_WIDTH / 2);
            continue;
        }

        b = MAP_BLOCK_PTR(g->bx, g->by + 1);

        if (b->face[0] != -1 || b->face[1] != -1 ||
            b->face[2] != -1 || b->face[3] != -1) {
            g->y += (CELL_WIDTH / 2);
            continue;
        }
    }
}


/**
 * set_current_room - Sets the current room
 * @room: room to became the current room
 *
 * Sets @room as the current room.
 */
void set_current_room(int room)
{
    int n, m;

    _current_room = room;

    _crp = &_rooms[_current_room];

    _under_water = _blink_light = _use_landscape = _use_rain = _use_lightning = 0;

    _fog_red = _fog_green = _fog_blue = 0;
    _room_ambient_light = 64;

    _item = -1;
    for (n = 0; n < MAX_ITEMS; n++) {
        if (_items[n].room == _current_room) {
            _item = n;
            break;
        }
    }

    if (_crp->enter_callback != NULL)
        filp_execv(_crp->enter_callback);

    /* build the room's light table */
    qdgdfv_build_light_table_ext(&_light_table[0][0],
                     LIGHT_LEVELS, MAX_LIGHT, _fog_red, _fog_green, _fog_blue);

    filp_execf("/room_name /room_names %d @ =", _current_room + 1);

    /* reset all bullets */
    for (n = 0; n < MAX_BULLETS; n++)
        _bullets[n].from_player = -1;

    /* all demons have forgotten the player */
    for (n = 0; n < MAX_DEMONS_PER_ROOM && _crp->demons[n] != -1; n++)
        _demons[_crp->demons[n]].oblivion = 100000;
    m = n / 2;

    /* set the angle bias of all demons */
    for (n = 0; n < MAX_DEMONS_PER_ROOM && _crp->demons[n] != -1; n++)
        _demons[_crp->demons[n]].a_bias = ((double) (m - n)) / 24;

    adjust_gates();
}


int _filpf_rnd(void)
{
    filp_int_push(rnd(NULL) % filp_int_pop());
    return FILP_OK;
}


/**
 * _filpf_new_room - start a new room
 *
 * Start loading a new room.
 */
static int _filpf_new_room(void)
{
    struct block *b;
    int m, n;
    struct filp_val *v;

    v = filp_pop();

    if (v->type == FILP_NULL)
        v = NULL;
    else
        filp_ref_value(v);

    _current_room = filp_int_pop();

    qdgdfv_bangif("_filpf_new_room", _current_room < 0 ||
              _current_room > MAX_ROOMS_PER_EPISODE);

    _crp = &_rooms[_current_room];

    _crp->in_use = 1;

    if (_crp->enter_callback != NULL)
        filp_unref_value(_crp->enter_callback);

    _crp->enter_callback = v;

    _crp->ty = filp_int_pop();
    _crp->tx = filp_int_pop();
    _crp->floor_color = _pal_conv[(unsigned char) filp_int_pop()];
    _crp->ceiling_color = _pal_conv[(unsigned char) filp_int_pop()];

    for (n = 0; n < MAX_GATES_PER_ROOM; n++)
        _crp->gates[n] = -1;
    for (n = 0; n < MAX_DEMONS_PER_ROOM; n++)
        _crp->demons[n] = -1;

    if (_crp->map != NULL)
        free(_crp->map);
    _crp->map = (struct block *) malloc(sizeof(struct block) * _crp->tx * _crp->ty);

    for (n = 0; n < _crp->ty; n++) {
        for (m = 0; m < _crp->tx; m++) {
            b = MAP_BLOCK_PTR(m, n);
            b->face[0] = b->face[1] = b->face[2] = b->face[3] = -1;
        }
    }

    _num_blocks = 0;

    return FILP_OK;
}


/**
 * _filpf_ambient_light - sets the ambient light
 *
 * Sets the ambient light for a room.
 */
int _filpf_ambient_light(void)
{
    _room_ambient_light = filp_int_pop();

    return FILP_OK;
}


/**
 * _filpf_gate - define a gate
 *
 * Defines a gate for the current room.
 */
static int _filpf_gate(void)
{
    struct filp_val *v;
    int n;

    qdgdfv_bangif("_filpf_gate", _num_gates == MAX_GATES_PER_EPISODE);

    v = filp_pop();

    if (_gates[_num_gates].cross_callback != NULL)
        filp_unref_value(_gates[_num_gates].cross_callback);

    if (v->type == FILP_NULL)
        _gates[_num_gates].cross_callback = NULL;
    else {
        filp_ref_value(v);
        _gates[_num_gates].cross_callback = v;
    }

    _gates[_num_gates].by = filp_int_pop();
    _gates[_num_gates].bx = filp_int_pop();
    _gates[_num_gates].to_y = filp_real_pop();
    _gates[_num_gates].to_x = filp_real_pop();
    _gates[_num_gates].to_room = filp_int_pop();
    _gates[_num_gates].in_room = _current_room;

    /* store the gate in current room data */
    for (n = 0; n < MAX_GATES_PER_ROOM; n++) {
        if (_crp->gates[n] == -1) {
            _crp->gates[n] = _num_gates;
            break;
        }
    }

    qdgdfv_bangif("_filpf_gate", n == MAX_GATES_PER_ROOM);

    _num_gates++;

    return FILP_OK;
}


/**
 * _filpf_block - create a block pattern
 *
 * Creates a block 'pattern', to be repeated
 * when creating the map.
 */
static int _filpf_block(void)
{
    int n, b;
    struct filp_val *v;

    qdgdfv_bangif("_filpf_block", _num_blocks == MAX_BLOCK_PATTERNS);

    for (n = 3; n >= 0; n--) {
        v = filp_pop();

        if (v->type == FILP_NULL)
            b = -1;
        else {
            filp_push(v);
            filp_exec("$wall_ids # aseek");
            b = filp_int_pop() - 1;
        }

        _blocks[_num_blocks].face[n] = b;
    }

    _num_blocks++;

    return FILP_OK;
}


/**
 * _filpf_defmap - define the map
 *
 * Parses and defines the map, using the block patterns.
 */
int _filpf_defmap(void)
{
    char *ptr;
    int b;
    int x, y;
    struct block *bl;
    struct filp_val *v;

    v = filp_pop();
    ptr = v->value;

    for (y = 0; y < _crp->ty; y++) {
        ptr++;      /* skip \n */

        for (x = 0; x < _crp->tx; x++) {
            bl = MAP_BLOCK_PTR(x, y);

            if (*ptr != '.') {
                sscanf(ptr, "%02X", &b);

                qdgdfv_bangif("_filpf_defmap", b < 0 || b >= _num_blocks);

                memcpy(bl, &_blocks[b], sizeof(struct block));
            }

            ptr += 2;
        }
    }

    return FILP_OK;
}


/**
 * _filpf_item - define an item
 *
 * Defines an item in this room.
 */
int _filpf_item(void)
{
    int i, x, y;

    y = filp_int_pop();
    x = filp_int_pop();

    filp_exec("$item_ids # aseek");
    i = filp_int_pop() - 1;

    _items[i].x = x;
    _items[i].y = y;
    _items[i].room = _current_room;

    _num_items++;

    return FILP_OK;
}


/**
 * _filpf_demon - defines a demon
 * @room: the room where the demon is located
 * @bmp: bitmap id (visual shape)
 * @mode: starting mode id
 * @health: hits needed to be killed
 * @strength: pain inflicted to player
 * @speed: speed
 * @type: demon type id
 * @x: x position in map
 * @y: y position in map
 *
 * Defines a demon in the specified room.
 */
/** @room @bmp @mode @health @strength @speed @type @x @y demon */
int _filpf_demon(void)
{
    struct demon *d;
    int room, n;

    qdgdfv_bangif("_filpf_demon", _num_demons == MAX_DEMONS_PER_EPISODE);

    d = &_demons[_num_demons];

    memset(d, '\0', sizeof(struct demon));

    d->y = filp_real_pop();
    d->x = filp_real_pop();

    filp_exec("$demon_type_ids # aseek");
    d->type = filp_int_pop() - 1;

    d->speed = filp_int_pop() + 2;
    d->strength = filp_int_pop();
    d->health = filp_int_pop();

    filp_exec("$demon_mode_ids # aseek");
    d->mode = filp_int_pop() - 1;
    filp_exec("$demon_ids # aseek");
    d->bmp = filp_int_pop() - 1;

    d->attack = d->step = d->reload = d->offset = 0;
    d->ticks = rnd(NULL) & _ticks_per_step;

    room = filp_int_pop();

    /* add demon to room */
    for (n = 0; n < MAX_DEMONS_PER_ROOM; n++) {
        if (_rooms[room].demons[n] == -1)
            break;
    }

    qdgdfv_bangif("_filpf_demon", n == MAX_DEMONS_PER_ROOM);

    _rooms[room].demons[n] = _num_demons;

    _num_demons++;

    if (d->type != TYPE_STATIC) {
        _killable_demons++;

        if (d->mode == MODE_DEAD || d->mode == MODE_GONE)
            _player_kills++;
    }

    return FILP_OK;
}


/**
 * _filpf_player_info - sets the player info
 *
 * Sets all needed info about the player.
 */
int _filpf_player_info(void)
{
    _player_health = filp_real_pop();
    _player_y = filp_real_pop();
    _player_x = filp_real_pop();
    _player_a = filp_real_pop();

    return FILP_OK;
}


/**
 * _filpf_load_palette - loads an episode's palette
 *
 * Loads the palette of an episode and builds the
 * conversion matrix. This palette is the one that
 * came with the original Freaks episode.
 */
int _filpf_load_palette(void)
{
    struct filp_val *v;
    FILE *f;
    int n, r, g, b;
    unsigned char red;
    unsigned char black;

    v = filp_pop();

    if ((f = qdgdfv_fopen(v->value, "rb")) == NULL)
        qdgdfv_bang("_filpf_load_palette", v->value);

    fseek(f, 7, SEEK_SET);

    for (n = 0; n < 256; n++) {
        if (n == 128)
            _pal_conv[n] = 255;
        else {
            r = fgetc(f) * 4;
            g = fgetc(f) * 4;
            b = fgetc(f) * 4;

            _pal_conv[n] = qdgdfv_seek_color(r, g, b);
        }
    }

    fclose(f);

    /* fills the color blend arrays */
    red = qdgdfv_seek_color(255, 0, 0);
    black = qdgdfv_seek_color(0, 0, 0);

    for (n = 0; n < 256; n++) {
        _red_color_blend[n] = qdgdfv_blend_color(n, red, 50);
        _dim_color_blend[n] = qdgdfv_blend_color(n, black, 80);
    }

    return FILP_OK;
}


/**
 * _filpf_load_walls - load the wall bitmaps
 *
 * Load the wall bitmaps.
 */
int _filpf_load_walls(void)
{
    struct filp_val *v;
    FILE *f;
    unsigned char b[BITMAP_WIDTH * BITMAP_HEIGHT];

    v = filp_pop();

    if ((f = qdgdfv_fopen(v->value, "rb")) == NULL)
        qdgdfv_bang("_filpf_load_walls", v->value);

    for (_num_bmp_walls = 0; _num_bmp_walls < MAX_BMP_WALLS; _num_bmp_walls++) {
        if (!fread(b, D_BITMAP_WIDTH, D_BITMAP_HEIGHT, f))
            break;

        if (_bmp_walls[_num_bmp_walls] != NULL)
            free(_bmp_walls[_num_bmp_walls]);

        _bmp_walls[_num_bmp_walls] = (unsigned char *)
            malloc(BITMAP_WIDTH * BITMAP_HEIGHT);

        adapt_bitmap(_bmp_walls[_num_bmp_walls], b);
    }

    fclose(f);

    return FILP_OK;
}



/**
 * _filpf_load_demons - load the demon bitmaps
 *
 * Load the demon bitmaps.
 */
int _filpf_load_demons(void)
{
    struct filp_val *v;
    FILE *f;
    unsigned char b[BITMAP_WIDTH * BITMAP_HEIGHT];
    int n, goon;
    unsigned char *ptr;

    v = filp_pop();

    if ((f = qdgdfv_fopen(v->value, "rb")) == NULL)
        qdgdfv_bang("_filpf_load_demons", v->value);

    for (_num_bmp_demons = 0, goon = 1; goon && _num_bmp_demons < MAX_BMP_DEMONS;
         _num_bmp_demons++) {
        if (_bmp_demons[_num_bmp_demons] != NULL)
            free(_bmp_demons[_num_bmp_demons]);

        ptr = _bmp_demons[_num_bmp_demons] = (unsigned char *)
            malloc(BITMAP_WIDTH * BITMAP_HEIGHT * 6);

        for (n = 0; n < 6; n++, ptr += BITMAP_WIDTH * BITMAP_HEIGHT) {
            if (!fread(b, D_BITMAP_WIDTH, D_BITMAP_HEIGHT, f)) {
                goon = 0;
                break;
            }

            adapt_bitmap(ptr, b);
        }
    }

    fclose(f);

    return FILP_OK;
}


/**
 * _filpf_load_items - load the item bitmaps
 *
 * Load the item bitmaps.
 */
int _filpf_load_items(void)
{
    struct filp_val *v;
    FILE *f;
    unsigned char b[BITMAP_WIDTH * BITMAP_HEIGHT];

    v = filp_pop();

    if ((f = qdgdfv_fopen(v->value, "rb")) == NULL)
        qdgdfv_bang("_filpf_load_items", v->value);

    for (_num_bmp_items = 0; _num_bmp_items < MAX_ITEMS; _num_bmp_items++) {
        if (!fread(b, D_BITMAP_WIDTH, D_BITMAP_HEIGHT, f))
            break;

        if (_bmp_items[_num_bmp_items] != NULL)
            free(_bmp_items[_num_bmp_items]);

        _bmp_items[_num_bmp_items] = (unsigned char *)
            malloc(BITMAP_WIDTH * BITMAP_HEIGHT * 2);

        adapt_bitmap(_bmp_items[_num_bmp_items], b);

        fread(b, D_BITMAP_WIDTH, D_BITMAP_HEIGHT, f);
        adapt_bitmap(_bmp_items[_num_bmp_items] + BITMAP_WIDTH * BITMAP_HEIGHT, b);
    }

    fclose(f);

    return FILP_OK;
}


/**
 * _filpf_load_font - Loads a font
 *
 * Loads a font.
 */
int _filpf_load_font(void)
{
    struct filp_val *v;

    v = filp_pop();

    qdgdfv_load_ktl_font(v->value);

    return FILP_OK;
}


/**
 * _filpf_set_inventory - sets the inventory map
 *
 * Set the taken flag of the 10 items of the inventory.
 */
int _filpf_set_inventory(void)
{
    int n;

    for (n = 0; n < 10; n++)
        _items[9 - n].taken = filp_int_pop();

    return FILP_OK;
}


/**
 * _filpf_item_taken - Tests if an item is taken
 *
 * Tests if an item is taken.
 */
static int _filpf_item_taken(void)
{
    int n;

    filp_exec("$item_ids # aseek");
    n = filp_int_pop() - 1;

    filp_int_push(_items[n].taken);

    return FILP_OK;
}


/**
 * _filpf_load_landscape - Loads the landscape bitmap
 *
 * Loads the landscape bitmap.
 */
int _filpf_load_landscape(void)
{
    struct filp_val *v;

    memset(_landscape, '\0', LANDSCAPE_X_SIZE * LANDSCAPE_Y_SIZE);

    v = filp_pop();

    qdgdfv_load_pcx((unsigned char *) _landscape, v->value,
            LANDSCAPE_X_SIZE * LANDSCAPE_Y_SIZE);

    return FILP_OK;
}


/**
 * _filpf_load_bullets - Load the bullet bitmaps
 *
 * Loads the bullet bitmaps.
 */
int _filpf_load_bullets(void)
{
    struct filp_val *v;

    v = filp_pop();
    qdgdfv_load_pcx((unsigned char *) _player_bullet, v->value,
            BITMAP_WIDTH * BITMAP_HEIGHT);

    v = filp_pop();
    qdgdfv_load_pcx((unsigned char *) _demon_bullet, v->value,
            BITMAP_WIDTH * BITMAP_HEIGHT);

    return FILP_OK;
}


/**
 * _filpf_set_accum_buffer - Sets the accumulation buffer
 *
 * Sets or resets the accumulation buffer.
 */
/** @percent set_accum_buffer */
int _filpf_set_accum_buffer(void)
{
    qdgdfv_set_accum_buffer(filp_int_pop());
    return FILP_OK;
}


/**
 * _filpf_load_map - load an episode's map
 *
 * Loads the map of an episode (and accesory data).
 */
int _filpf_load_map(void)
{
    int n;

    _episode = filp_int_pop();

    /* init things first */
    _num_gates = _num_demons = _num_items = 0;
    _water_distort_factor = 8.0;
    _killable_demons = _player_kills = _win_if_demon_0 = 0;

    _player_cancelled = _player_won = 0;

    for (n = 0; n < MAX_ROOMS_PER_EPISODE; n++)
        _rooms[n].in_use = 0;

    for (n = 0; n < MAX_ITEMS; n++)
        _items[n].room = -1;

    qdgdfa_reset();

    /* load the map */
    filp_exec("\"$data_dir/e$episode/map.filp\" load");

    return FILP_OK;
}


/**
 * _filpf_set_current_room - sets the current room
 *
 * Sets the player's current room.
 */
int _filpf_set_current_room(void)
{
    set_current_room(filp_int_pop());

    return FILP_OK;
}


/**
 * _filpf_fog_color - sets the room's fog color
 *
 * Sets the room's light fog color.
 */
int _filpf_fog_color(void)
{
    _fog_blue = filp_int_pop();
    _fog_green = filp_int_pop();
    _fog_red = filp_int_pop();

    return FILP_OK;
}


/**
 * _filpf_under_water - sets the under water flag
 *
 * Sets the under water flag.
 */
int _filpf_under_water(void)
{
    _under_water = 1;

    return FILP_OK;
}

/**
 * _filpf_landscape - sets the landscape flag
 *
 * Sets the landscape flag.
 */
int _filpf_landscape(void)
{
    _use_landscape = 1;

    return FILP_OK;
}


/**
 * _filpf_blink_light - sets the blinking light flag
 *
 * Sets the blinking light flag.
 */
int _filpf_blink_light(void)
{
    _blink_light = 1;

    return FILP_OK;
}


/**
 * _filpf_raining - sets the raining flag
 *
 * Sets the raining flag.
 */
int _filpf_raining(void)
{
    _use_rain = 1;

    return FILP_OK;
}


/**
 * _filpf_lightning - sets the lightning flag
 *
 * Sets the lightning flag.
 */
int _filpf_lightning(void)
{
    _use_lightning = 1;

    return FILP_OK;
}


/**
 * _filpf_load_sound - loads a sound
 * @variable: variable name where the sound id will be stored
 * @sound_file: the .wav soundfile
 *
 * Loads a sound.
 */
/** @variable @sound_file load_sound */
int _filpf_load_sound(void)
{
    int n;
    struct filp_val *v;

    filp_exec("\"$data_dir/\" # .");

    v = filp_pop();

    n = qdgdfa_load_sound(v->value);

    filp_execf("%d set", n);

    return FILP_OK;
}


/**
 * _filpf_load_big_sound - loads a big sound
 * @variable: variable name where the sound id will be stored
 * @sound_file: the .wav soundfile
 *
 * Loads a big sound.
 */
/** @variable @sound_file load_big_sound */
int _filpf_load_big_sound(void)
{
    int n;
    struct filp_val *v;

    filp_exec("\"$data_dir/\" # .");

    v = filp_pop();

    n = qdgdfa_load_big_sound(v->value);

    filp_execf("%d set", n);

    return FILP_OK;
}


/**
 * _filpf_save_game - Saves the current game
 *
 * Saves the current game.
 */
int _filpf_save_game(void)
{
    FILE *f;
    int n, m;
    struct demon *d;
    struct filp_val *v;

    v = filp_pop();

    if ((f = qdgdfv_fopen(v->value, "w")) == NULL)
        return FILP_ERROR;

    fprintf(f, "/* saved game */\n\n");

    fprintf(f, "/save_game_string \"%s: %d%%\" set\n\n", _room_name, _adventure_total);
    fprintf(f, "$info_only { end } if\n\n");

    /* player data */
    fprintf(f, "%f %f %f %d player_info\n",
        (float) _player_a, (float) _player_x, (float) _player_y, _player_health);

    for (n = 0; n < MAX_ITEMS; n++)
        fprintf(f, "%d ", _items[n].taken);

    fprintf(f, "set_inventory\n\n");

    fprintf(f, "%d load_map\n", _episode);

    for (n = 0; n < MAX_ROOMS_PER_EPISODE; n++) {
        if (_rooms[n].in_use && _rooms[n].map != NULL) {
            fprintf(f, "\n/* room %d */\n\n", n);

            for (m = 0; m < MAX_DEMONS_PER_ROOM; m++) {
                if (_rooms[n].demons[m] == -1)
                    break;

                d = &_demons[_rooms[n].demons[m]];

                fprintf(f, "%d ", n);

                filp_execf("/demon_ids %d @", d->bmp + 1);
                v = filp_pop();
                fprintf(f, "\"%s\" ", v->value);

                filp_execf("/demon_mode_ids %d @", d->mode + 1);
                v = filp_pop();
                fprintf(f, "\"%s\" ", v->value);

                fprintf(f, "%d %d %d ", d->health, d->strength, d->speed - 2);

                filp_execf("/demon_type_ids %d @", d->type + 1);
                v = filp_pop();
                fprintf(f, "\"%s\" ", v->value);

                fprintf(f, "%f %f demon\n", (float) d->x, (float) d->y);
            }
        }
    }

    fprintf(f, "%d set_current_room\n", _current_room);

    fclose(f);

    return FILP_OK;
}


/**
 * map_startup - Starts up the map variables and code
 *
 * Starts up the map variables and code.
 */
void map_startup(void)
{
    /* set the filp functions */
    filp_exec("/lang $filp_lang set");

    /* directories */
    filp_execf("/data_dir 'data' set", qdgdfv_app_dir());
    filp_execf("/home_dir '%s' '.fr2002' . set", qdgdfv_home_dir());
    filp_exec("$home_dir mkdir");

    /* various internal id arrays */
    filp_exec("/demon_type_ids ( 'STATIC' 'FOLLOW' 'SHOOT' ) set");
    filp_exec("/demon_mode_ids ( 'MOVE' 'ATTACK' 'BLEED' 'DEAD' 'FROZEN' 'GONE' ) set");

    filp_ext_int("episode", &_episode);

    filp_bin_code("rnd", _filpf_rnd);
    filp_bin_code("new_room", _filpf_new_room);
    filp_bin_code("ambient_light", _filpf_ambient_light);
    filp_bin_code("gate", _filpf_gate);
    filp_bin_code("block", _filpf_block);
    filp_bin_code("defmap", _filpf_defmap);
    filp_bin_code("item", _filpf_item);
    filp_bin_code("demon", _filpf_demon);
    filp_bin_code("player_info", _filpf_player_info);
    filp_bin_code("load_palette", _filpf_load_palette);
    filp_bin_code("load_walls", _filpf_load_walls);
    filp_bin_code("load_demons", _filpf_load_demons);
    filp_bin_code("load_items", _filpf_load_items);
    filp_bin_code("load_font", _filpf_load_font);
    filp_bin_code("load_landscape", _filpf_load_landscape);
    filp_bin_code("load_bullets", _filpf_load_bullets);
    filp_bin_code("set_inventory", _filpf_set_inventory);
    filp_bin_code("item_taken", _filpf_item_taken);
    filp_bin_code("set_accum_buffer", _filpf_set_accum_buffer);
    filp_bin_code("load_map", _filpf_load_map);
    filp_bin_code("set_current_room", _filpf_set_current_room);
    filp_bin_code("fog_color", _filpf_fog_color);
    filp_bin_code("under_water", _filpf_under_water);
    filp_bin_code("landscape", _filpf_landscape);
    filp_bin_code("blink_light", _filpf_blink_light);
    filp_bin_code("raining", _filpf_raining);
    filp_bin_code("lightning", _filpf_lightning);
    filp_bin_code("save_game", _filpf_save_game);

    filp_bin_code("load_sound", _filpf_load_sound);
    filp_bin_code("load_big_sound", _filpf_load_big_sound);

    filp_ext_int("p_shoot_sound", &_p_shoot_sound);
    filp_ext_int("d_hurt_sound", &_d_hurt_sound);
    filp_ext_int("d_die_sound", &_d_die_sound);
    filp_ext_int("p_hurt_sound", &_p_hurt_sound);
    filp_ext_int("p_die_sound", &_p_die_sound);
    filp_ext_int("d_see_sound", &_d_see_sound);
    filp_ext_int("b_crash_sound", &_b_crash_sound);
    filp_ext_int("pick_item_sound", &_pick_item_sound);
    filp_ext_int("pick_life_sound", &_pick_life_sound);
    filp_ext_int("soundtrack", &_soundtrack);

    filp_ext_real("water_distort_factor", &_water_distort_factor);

    filp_ext_string("episode_name", _episode_name, sizeof(_episode_name));
    filp_ext_string("room_name", _room_name, sizeof(_room_name));

    filp_ext_int("boring_floor", &_boring_floor);
    filp_ext_int("sound", &_qdgdfa_sound);
    filp_ext_int("screen_x_size", (int *) &_qdgdfv_screen_x_size);
    filp_ext_int("screen_y_size", (int *) &_qdgdfv_screen_y_size);
    filp_ext_int("gamma_correct", &_qdgdfv_gamma_correct);
    filp_ext_int("win_if_demon_0", &_win_if_demon_0);

    filp_ext_int("show_ticks", &_show_ticks);

    memset(_rooms, '\0', sizeof(_rooms));
}
