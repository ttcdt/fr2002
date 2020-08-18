/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Main game code

*/

/******************
    Data
*******************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "filp.h"
#include "qdgdf_video.h"
#include "qdgdf_audio.h"

#include "render.h"
#include "map.h"
#include "game.h"


/* minimum distance to a wall */
int _minimum_distance = 10;

/* ticks elapsed on each frame */
int _ticks_elapsed = 0;

/* kill count */
int _player_kills = 0;

/* max player speed */
double _max_player_speed = 0.50;

/* max angle speed */
double _max_angle_speed = 0.004;

/* player deceleration when stopping */
double _player_stop_decel = 0.003;

/* player deceleration when not rotating */
double _player_angle_stop_decel = 0.00025;

/* collision distance */
double _coll_dist = 32;

/* max demon speed */
double _max_demon_speed = 0.60;

/* demon attack distance */
double _attack_dist = 64;

/* reload time for player */
int _player_reload_ticks = 250;

/* reload time for demons */
int _demon_reload_ticks = 700;

/* ticks until a demon forgets about us */
int _forget_player_ticks = 3000;

/* strength of the demon bullets */
int _demon_bullet_strength = 5;

/* distance the demons stop spreading */
int _spread_distance = 128;

/* max bullet speed */
double _max_bullet_speed = 0.65;

/* colors */
unsigned char _menu_tcolor;
unsigned char _menu_ocolor;
unsigned char _menu_color;
unsigned char _info_color;

/* global game flags */
int _player_cancelled = 0;
int _player_won = 0;

/* flag to show elapsed time on screen */
int _show_ticks = 0;

/* ticks to next item animation change */
int _item_anim_ticks = 400;
int _item_bitmap_offset = 0;

int _player_hit = 0;

/* number of items taken */
int _taken_items = 0;

/* distance to a door to be crossed */
double _door_cross_distance = 64;

/* distance to an item to be picked up */
double _item_pick_distance = 64;

/* message */
char _message_text[4096];
int _message_ticks = 0;
int _message_y = -32;
unsigned char _message_color = 0;

/******************
    Code
*******************/

/**
 * message - Shows a message on the game screen
 * @msg: message to be shown (filp string)
 * @ticks: ticks to last on screen
 *
 * Sets the message to be shown on the game screen.
 */
void message(char *msg, int ticks)
{
    int n;

    _message_color = qdgdfv_seek_color(0, 255, 0);

    _message_ticks = ticks;

    memset(_message_text, '\0', sizeof(_message_text));
    filp_exec(msg);
    filp_exec("/message_text #=");

    /* now break by line */
    _message_y = -32;
    for (n = 0; _message_text[n]; n++) {
        if (_message_text[n] == '\n') {
            _message_text[n] = '\0';
            _message_y -= 12;
        }
    }
}


/**
 * angle_move - Move in the direction of the angle
 * @a: the angle
 * @dist: the distance to move
 * @x: pointer to x position
 * @y: pointer to y position
 *
 * Try to move in the direction specified by the angle. If the movement
 * is possible (i.e. don't crash with walls), the coordinates are changed.
 * Returns nonzero if the movement could be done.
 */
int angle_move(double a, double dist, double *x, double *y, int test_ray)
{
    double ix, iy;
    struct block *b;
    struct ray_hit *r;
    int move = 2;

    if (dist < 0) {
        a += AOV_PI;
        dist = -dist;
    }

    if (test_ray) {
        start_ray(*x, *y, a);
        r = get_ray();

        if (r != NULL) {
            if (r->dist < _minimum_distance || r->dist < dist)
                return 0;
        }
    }

    ix = (sin(a) * dist);
    iy = (-cos(a) * dist);

    b = MAP_BLOCK_PTR(((int) (*x + ix * 2) / CELL_WIDTH), ((int) *y / CELL_WIDTH));

    if (b->face[0] != -1 || b->face[1] != -1 || b->face[2] != -1 || b->face[3] != -1) {
        ix = 0;
        move--;
    }

    b = MAP_BLOCK_PTR(((int) *x / CELL_WIDTH), ((int) (*y + iy * 2) / CELL_WIDTH));

    if (b->face[0] != -1 || b->face[1] != -1 || b->face[2] != -1 || b->face[3] != -1) {
        iy = 0;
        move--;
    }

    *x += ix;
    *y += iy;

    return move;

/*
    ix = (sin(a) * dist);
    iy = (-cos(a) * dist);

    start_ray(*x, *y, a);
    r = get_ray();

    if(r != NULL && r->dist > dist)
    {
        start_ray(*x + ix, *y + iy, a);
        r = get_ray();

        if(r != NULL && r->dist > _minimum_distance)
        {
            printf("%f\n",(float)r->dist);
            *x += ix;
            *y += iy;

            return 1;
        }
    }

    start_ray(*x + ix, *y, a);
    r = get_ray();

    if(r != NULL && r->dist > _minimum_distance)
    {
        *x += ix;

        return 1;
    }

    start_ray(*x, *y + iy, a);
    r = get_ray();

    if(r != NULL && r->dist > _minimum_distance)
    {
        *y += iy;

        return 1;
    }

    return 0;
*/
}


/**
 * angle_exact_move - Move in the direction of the angle
 * @a: the angle
 * @dist: the distance to move
 * @x: pointer to x position
 * @y: pointer to y position
 *
 * Like angle_move(), but the movement should be exactly the
 * specified distance. If it could not be done, returns 0.
 * Used for bullets that disappear when a wall is hit.
 */
int angle_exact_move(double a, double dist, double *x, double *y)
{
    double ix, iy;
    struct block *b;
    struct ray_hit *r;

    start_ray(*x, *y, a);
    r = get_ray();

    if (r != NULL) {
        if (r->dist < _minimum_distance || r->dist < dist)
            return 0;
    }


    ix = (sin(a) * dist);
    iy = (-cos(a) * dist);

    b = MAP_BLOCK_PTR(((int) (*x + ix) / CELL_WIDTH), ((int) (*y + iy) / CELL_WIDTH));

    if (b->face[0] != -1 || b->face[1] != -1 || b->face[2] != -1 || b->face[3] != -1)
        return 0;

    *x += ix;
    *y += iy;

    return 1;
}


/**
 * look_at - Return the angle of a point
 * @xo: x origin
 * @yo: y origin
 * @xd: x destination
 * @yd: y destination
 *
 * Returns the angle that something at (xo, yo) must look to
 * to be facing (xd, yd).
 */
double look_at(double xo, double yo, double xd, double yd)
{
    double tx, tz, r;

    tx = yo - yd;
    tz = xo - xd;

    /* special values */
    if (tx == 0) {
        if (tz > 0)
            return AOV_PI / 2;
        else
            return (3 * AOV_PI) / 2;
    }

    r = atan(fabs(tz) / fabs(tx));

    if (tx > 0)
        r = AOV_PI - r;
    if (tz < 0)
        r = (2 * AOV_PI) - r;

    return r;
}


/**
 * move_demons - Process all demons
 * @x: x coord of player
 * @y: y coord of player
 *
 * Process all demons in the current room.
 */
void move_demons(double x, double y)
{
    int n, i;
    double dist;
    struct ray_hit *r;
    struct demon *d;
    double m;
    static int _item_ticks = 0;

    for (n = 0; n < MAX_DEMONS_PER_ROOM && _crp->demons[n] != -1; n++) {
        i = _crp->demons[n];
        d = &_demons[i];

        if (d->mode == MODE_FROZEN || d->mode == MODE_GONE)
            continue;

        /* calculate the step */
        d->ticks += _ticks_elapsed;

        if (d->ticks > _ticks_per_step) {
            d->step++;
            d->ticks %= _ticks_per_step;

            if (d->attack > 0)
                d->attack--;

            if (d->mode == MODE_BLEED) {
                d->health--;

                if (d->health > 0)
                    d->mode = MODE_MOVE;
                else {
                    if (d->type != TYPE_STATIC) {
                        qdgdfa_respawn_sound(_d_die_sound);
                        _player_kills++;
                    }

                    d->mode = MODE_DEAD;
/*
                    if((_player_health += d->strength) > 100)
                        _player_health = 100;
*/
                    /* if demon 0 is killed */
                    if (i == 0 && _win_if_demon_0)
                        _player_won = 1;
                }
            }
        }

        if (d->mode == MODE_DEAD) {
            d->offset = BITMAP_HEIGHT * BITMAP_WIDTH * 5;

            if (_player_health < 100) {
                /* if player health is not 100%,
                   dead bodies nearby can be picked
                   and life restored from them */
                dist = distance(x - d->x, y - d->y);

                if (dist < _attack_dist) {
                    if ((_player_health += d->strength) > 100)
                        _player_health = 100;

                    qdgdfa_respawn_sound(_pick_life_sound);

                    d->mode = MODE_GONE;
                }
            }

            continue;
        }

        d->offset = 0;

        if ((d->type == TYPE_FOLLOW || d->type == TYPE_SHOOT)
            && d->mode == MODE_ATTACK) {
            d->offset = BITMAP_HEIGHT * BITMAP_WIDTH * 2;

            if (d->attack == 0)
                d->mode = MODE_MOVE;
        }

        if ((d->type == TYPE_FOLLOW || d->type == TYPE_SHOOT) && d->mode == MODE_MOVE) {
            /* look at player */
            d->a = look_at(d->x, d->y, x, y);

            dist = distance(x - d->x, y - d->y);

            start_ray(d->x, d->y, d->a + AOV_PI);
            r = get_ray();

            /* if demon can cast a visual ray to player,
               chase him */
            if (r != NULL) {
                if (r->dist > dist) {
                    d->oblivion = 0;

                    /* if it's a shooter and its weapon
                       is reloaded, shoot */
                    if (d->type == TYPE_SHOOT) {
                        if (d->reload > 0)
                            d->reload -= _ticks_elapsed;
                        else {
                            d->mode = MODE_ATTACK;
                            shoot_bullet(d->x, d->y,
                                     d->a + AOV_PI, 0);
                            d->reload = _demon_reload_ticks;
                        }
                    }
                }
            }
            else {
                /* demon does not see player;
                   increment the oblivion counter */
                d->oblivion += _ticks_elapsed;
            }

            if (d->oblivion < _forget_player_ticks) {
                m = (_max_demon_speed * _ticks_elapsed) / d->speed;

                /* calculate the angle bias for each
                   demon, so they spread to surround the player */
                if (dist > _spread_distance)
                    d->a += d->a_bias;

                angle_move(_demons[i].a, -m, &_demons[i].x, &_demons[i].y, 0);
            }

            /* if the demon is in attack distance to the player,
               start attacking */
            if (dist < _attack_dist) {
                /* four steps attacking */
                d->attack = 4;
                d->offset = BITMAP_HEIGHT * BITMAP_WIDTH * 2;
                d->mode = MODE_ATTACK;

                _player_hit += 250;
                _player_health -= d->strength;

                if (_player_health <= 0)
                    qdgdfa_play_sound(_p_die_sound, 0);
                else
                    qdgdfa_play_sound(_p_hurt_sound, 0);
            }
        }

        /* sum the step */
        if (d->step & 1)
            d->offset += (BITMAP_HEIGHT * BITMAP_WIDTH);

        /* if it's bleeding... */
        if (d->mode == MODE_BLEED)
            d->offset = (BITMAP_HEIGHT * BITMAP_WIDTH * 4);
    }

    /* process item animation */
    /* this does not belong here */
    _item_ticks -= _ticks_elapsed;

    if (_item_ticks < 0) {
        _item_ticks = _item_anim_ticks;

        if (_item_bitmap_offset == 0)
            _item_bitmap_offset = BITMAP_WIDTH * BITMAP_HEIGHT;
        else
            _item_bitmap_offset = 0;
    }
}


/**
 * add_sprites - Adds the sprites to the render image
 * @x: the x coord of the POV
 * @y: the y coord of the POV
 * @a: the angle of the POV
 *
 * Adds all the sprites to the render image.
 */
void add_sprites(double x, double y, double a)
{
    int n;
    struct demon *d;
    struct bullet *b;

    /* restart */
    _num_sprites = 0;

    /* add the demons */
    for (n = 0; n < MAX_DEMONS_PER_ROOM; n++) {
        if (_crp->demons[n] == -1)
            break;

        d = &_demons[_crp->demons[n]];

        if (d->mode != MODE_FROZEN && d->mode != MODE_GONE)
            add_sprite(x, y, a, _bmp_demons[d->bmp] + d->offset, d->x, d->y);
    }

    /* add the item, if any */
    if (_item != -1 && !_items[_item].taken)
        add_sprite(x, y, a, _bmp_items[_item] + _item_bitmap_offset,
               _items[_item].x, _items[_item].y);

    /* add the bullets */
    for (n = 0; n < MAX_BULLETS; n++) {
        b = &_bullets[n];

        if (b->from_player == 1)
            add_sprite(x, y, a, _player_bullet, b->x, b->y);
        else
        if (b->from_player == 0)
            add_sprite(x, y, a, _demon_bullet, b->x, b->y);
    }

#if 0
    for (n = 0; _crp->gates[n] != -1; n++) {
        struct gate *g;

        g = &_gates[_crp->gates[n]];
        add_sprite(x, y, a, _player_bullet, g->x, g->y);
    }
#endif
}


/**
 * move_player - Move the player
 *
 * Processes the player, mainly from user interaction.
 */
void move_player(void)
{
    int n;
    static int _reload_ticks = 0;
    double m, d;
    struct gate *g;
    struct demon *dm;
    double da = 0.0;

    if (_qdgdfv_key_f9) {
        struct filp_val *v;

        filp_exec("$home_dir '/' . time . '.tga' .");
        v = filp_pop();

        qdgdfv_write_tga(v->value, _qdgdfv_virtual_screen,
                        _qdgdfv_screen_x_size, _qdgdfv_screen_y_size, -1);

        while (_qdgdfv_key_f9)
            qdgdfv_input_poll();
    }

    if (_qdgdfv_key_alt_l) {
        if (_qdgdfv_key_left)
            da = -AOV_PI / 2;
        else
        if (_qdgdfv_key_right)
            da = AOV_PI / 2;
        else
            _player_speed = 0;
    }

    if (da == 0.0) {
        /* angle movement */
        if (_qdgdfv_key_left)
            _player_angle_speed = -_max_angle_speed;
        else
        if (_qdgdfv_key_right)
            _player_angle_speed = _max_angle_speed;
        else
        if (_player_angle_speed > 0) {
            /* _player_angle_speed /= 4; */
            _player_angle_speed -= (_player_angle_stop_decel * _ticks_elapsed);
    
            if (_player_angle_speed < 0)
                _player_angle_speed = 0;
        }
        else
        if (_player_angle_speed < 0) {
            _player_angle_speed += (_player_angle_stop_decel * _ticks_elapsed);
    
            if (_player_angle_speed > 0)
                _player_angle_speed = 0;
        }
    
        /* movement */
        if (_player_hit <= 0 && _qdgdfv_key_up)
            _player_speed = _max_player_speed;
        else
        if (_player_hit <= 0 && _qdgdfv_key_down)
            _player_speed = -_max_player_speed;
        else
        if (_player_speed > 0) {
            /* _player_speed /= 2; */
            _player_speed -= (_player_stop_decel * _ticks_elapsed);
    
            if (_player_speed < 0)
                _player_speed = 0;
        }
        else
        if (_player_speed < 0) {
            _player_speed += (_player_stop_decel * _ticks_elapsed);
    
            if (_player_speed > 0)
                _player_speed = 0;
        }
    }
    else
        _player_speed = _max_player_speed;

    /* sum angular speed */
    m = _player_angle_speed * _ticks_elapsed;

    _player_a += m;

    /* sum directional speed */
    m = _player_speed * _ticks_elapsed;
/*
    if(! angle_move(_player_a, m, &_player_x, &_player_y, 1))
        _player_speed = 0;
*/

    while (fabs(_player_speed) > 0.01) {
        if (angle_move(_player_a + da, m, &_player_x, &_player_y, 1))
            break;

        _player_speed /= 2;
        m /= 2;
    }

    if (fabs(_player_speed) > 0.01 || fabs(_player_angle_speed) > 0.0001)
        _move_count++;

    if (_reload_ticks > 0)
        _reload_ticks -= _ticks_elapsed;
    else
    if (_qdgdfv_key_control) {
        qdgdfa_respawn_sound(_p_shoot_sound);

        shoot_bullet(_player_x, _player_y, _player_a, 1);
        _reload_ticks = _player_reload_ticks;
        _ambient_light *= 2;
    }

    /* test item picking */
    if (_item != -1 && !_items[_item].taken) {
        d = distance(_player_x - _items[_item].x, _player_y - _items[_item].y);

        if (d < _item_pick_distance) {
            int n;

            qdgdfa_play_sound(_pick_item_sound, 0);
            _items[_item].taken = 1;

            for (n = 0; n < 10 && _items[n].taken; n++);

            if (n == 10)
                message("$full_inv_text", 15000);
            else
                message("MSG_PICK_ITEM", 6000);

            /* change all demons in MODE_FROZEN
               to MODE_MOVE */
            for (n = 0; n < MAX_DEMONS_PER_ROOM && _crp->demons[n] != -1; n++) {
                dm = &_demons[_crp->demons[n]];

                if (dm->mode == MODE_FROZEN)
                    dm->mode = MODE_MOVE;
            }
        }
    }

    /* test door crossing */
    for (n = 0; n < MAX_GATES_PER_ROOM && _crp->gates[n] != -1; n++) {
        g = &_gates[_crp->gates[n]];

        d = distance(_player_x - g->x, _player_y - g->y);

        if (d < _door_cross_distance) {
            _player_speed = 0;

            if (g->cross_callback != NULL) {
                int can;

                filp_execv(g->cross_callback);
                can = filp_int_pop();

                if (!can) {
                    message("MSG_DOOR_LOCKED", 6000);
                    continue;
                }
            }

            _player_x = g->to_x;
            _player_y = g->to_y;

            if (g->to_room != -1)
                set_current_room(g->to_room);

            break;
        }
    }
}


/**
 * move_bullets - Moves the bullets
 *
 * Moves the currently bullets in use (from player and demons).
 * The bullet is destroyed if it hits a wall or aim, inflicting
 * pain if necessary.
 */
void move_bullets(void)
{
    double m;
    int n, i;
    struct demon *d;
    struct bullet *b;

    for (n = 0; n < MAX_BULLETS; n++) {
        b = &_bullets[n];

        if (b->from_player == -1)
            continue;

        /* sum directional speed */
        m = _max_bullet_speed * _ticks_elapsed;

        if (b->from_player == 1) {
            for (i = 0; i < MAX_DEMONS_PER_ROOM && _crp->demons[i] != -1; i++) {
                d = &_demons[_crp->demons[i]];

                if (d->mode == MODE_FROZEN ||
                    d->mode == MODE_DEAD || d->mode == MODE_GONE)
                    continue;

                if (distance(d->x - b->x, d->y - b->y) < _coll_dist) {
                    qdgdfa_play_sound(_d_hurt_sound, 0);

                    /* the demon is hit */
                    d->mode = MODE_BLEED;
                    d->ticks = 0;
                    d->oblivion = 0;

                    /* anyway, the bullet is detroyed */
                    b->from_player = -1;
                    break;
                }
            }
        }
        else {
            if (distance(_player_x - _bullets[n].x,
                     _player_y - _bullets[n].y) < _coll_dist) {
                _player_hit += 250;
                _player_health -= _demon_bullet_strength;

                if (_player_health < 0)
                    qdgdfa_play_sound(_p_die_sound, 0);
                else
                    qdgdfa_play_sound(_p_hurt_sound, 0);

                b->from_player = -1;
            }
        }

        if (!angle_exact_move(b->a, m, &b->x, &b->y)) {
            /* bullet has crashed on a wall */
            qdgdfa_respawn_sound(_b_crash_sound);

            b->from_player = -1;
        }
    }
}


/**
 * shoot_bullet - Shoots a new bullet.
 * @x: x coord of the new bullet
 * @y: y coord of the new bullet
 * @a: angle of the new bullet
 * @from_player: true if the bullet is being fired by the player
 *
 * Shoots a new bullet from the provided coordinates. If @from_player
 * is nonzero, the bullets hurts the demons, or the player otherwise.
 * Returns 0 if the bullet could not be shoot (no room for a new
 * bullet information).
 */
int shoot_bullet(double x, double y, double a, int from_player)
{
    int n;
    struct bullet *b;

    for (n = 0; n < MAX_BULLETS; n++) {
        b = &_bullets[n];

        if (b->from_player == -1) {
            b->x = x;
            b->y = y;
            b->a = a;
            b->from_player = from_player;
            return 1;
        }
    }

    return 0;
}


/**
 * menu - Processes a menu
 * @menuspec: string descripting the menu
 * @op: pointer to integer to hold the currently selected option
 *
 * Processes a menu using the provided @menuspec. This string
 * is in the format "menutitle;option|option|option". The @op
 * pointer to integer contains the currently selected operation
 * (must be set the first time). Returns 1 if the user has
 * made an action (moving the selection up and down is not
 * considered an action). @op will contain -1 if the user
 * cancelled (pressed ESC) or the option number if confirmed
 * (pressed ENTER), starting from 0.
 */
int menu(char *menuspec, int *op)
{
    char *ptr;
    unsigned char tmp[256];
    int nops, n, y, iy;
    static int _up_status = 0;
    static int _down_status = 0;

    _menu_tcolor = qdgdfv_seek_color(255, 0, 0);
    _menu_ocolor = qdgdfv_seek_color(255, 255, 0);
    _menu_color = qdgdfv_seek_color(255, 128, 0);
    _info_color = qdgdfv_seek_color(255, 255, 0);

    for (n = 0; *menuspec != ';'; n++) {
        tmp[n] = *menuspec;
        menuspec++;
    }
    tmp[n] = '\0';
    menuspec++;

    if (_qdgdfv_key_escape) {
        *op = -1;
        return 1;
    }

    if (_qdgdfv_key_enter)
        return 1;

    /* count the number of options */
    for (ptr = menuspec, nops = 1; *ptr; ptr++) {
        if (*ptr == '|')
            nops++;
    }

    if (_qdgdfv_key_up && !_up_status) {
        (*op)--;
        if (*op == -1)
            *op = nops - 1;
    }

    if (_qdgdfv_key_down && !_down_status) {
        (*op)++;
        if (*op == nops)
            *op = 0;
    }

    _up_status = _qdgdfv_key_up;
    _down_status = _qdgdfv_key_down;

    iy = (_qdgdfv_screen_y_size / 2) - ((nops * _qdgdfv_font_height) / 2);

    qdgdfv_font_print(-1, _qdgdfv_font_height, tmp, _menu_tcolor);

    for (ptr = menuspec, y = 0; *ptr; y++) {
        for (n = 0; *ptr != '|' && *ptr; n++, ptr++)
            tmp[n] = *ptr;
        tmp[n] = '\0';

        if (*ptr == '|')
            ptr++;

        if (y == *op)
            qdgdfv_font_print(-1, iy + (y * _qdgdfv_font_height),
                      tmp, _menu_ocolor);
        else
            qdgdfv_font_print(-1, iy + (y * _qdgdfv_font_height),
                      tmp, _menu_color);
    }

    return 0;
}


void show_info(char *info_var)
{
    int n, l;
    struct filp_val *v;

    filp_exec(info_var);
    filp_exec("/I # \"\n\" split ) = /I 0 @");

    /* number of lines in the info string */
    l = filp_int_pop();

    for (n = 0; n < l; n++) {
        filp_execf("/I %d @", n + 1);
        v = filp_pop();

        if (*v->value != '\a')
            qdgdfv_font_print(-1, (n * _qdgdfv_font_height),
                      (unsigned char *) v->value, _menu_ocolor);
    }

    filp_exec("/I '' =");
}


/**
 * save_load_game_menu - Shows the save or load game menu
 * @save: 1 if save, 0 if load
 *
 * Shows the save or load game menu. Returns 1 if the operation
 * was done, or 0 if user cancelled the operation (i.e. pressed
 * ESC).
 */
int save_load_game_menu(int save)
{
    int n;
    struct filp_val *v;
    int ret = 0;

    filp_execf("/info_only 1 = %s [", save ? "MSG_MENU_SAVE" : "MSG_MENU_LOAD");

    for (n = 0; n < 8; n++) {
        filp_exec("/save_game_string MSG_EMPTY_GAME =");
        filp_execf("\"$home_dir/savegame%d.filp\" load", n);
        filp_exec("$save_game_string");
    }

    filp_exec("'|' join .");

    v = filp_pop();
    filp_ref_value(v);

    filp_exec("/info_only 0 =");

    while (_qdgdfv_key_enter)
        qdgdfv_input_poll();

    n = 0;
    for (;;) {
        render(_player_x, _player_y, _player_a);

        qdgdfv_input_poll();

        if (menu(v->value, &n)) {
            if (n >= 0) {
                if (save) {
                    filp_execf("\"$home_dir/savegame%d.filp\" save_game",
                           n);
                    ret = 1;
                    break;
                }
                else {
                    if (!filp_execf
                        ("\"$home_dir/savegame%d.filp\" load", n)) {
                        ret = 1;
                        break;
                    }
                }
            }
            else
                break;
        }

        qdgdfv_dump_virtual_screen();
    }

    filp_unref_value(v);

    return ret;
}


void game_main_loop_menu(void)
{
    int n, o, i;
    struct filp_val *v;

    filp_exec("MSG_MENU_GAME");

    v = filp_pop();
    filp_ref_value(v);

    while (_qdgdfv_key_escape)
        qdgdfv_input_poll();

    o = i = 0;

    for (;;) {
        add_sprites(_player_x, _player_y, _player_a);
        render(_player_x, _player_y, _player_a);

        /* dim all the screen */
        for (n = 0; n < _qdgdfv_screen_x_size * _qdgdfv_screen_y_size; n++)
            _qdgdfv_virtual_screen[n] =
                _dim_color_blend[_qdgdfv_virtual_screen[n]];

        qdgdfv_input_poll();

        if (i) {
            show_info("$info_text");

            if (_qdgdfv_key_enter) {
                while (_qdgdfv_key_enter)
                    qdgdfv_input_poll();
                break;
            }
            else
            if (_qdgdfv_key_escape) {
                while (_qdgdfv_key_escape)
                    qdgdfv_input_poll();
                break;
            }
        }
        else {
            if (menu(v->value, &o)) {
                if (o == 1)
                    save_load_game_menu(1);
                else
                if (o == 2) {
                    while (_qdgdfv_key_enter)
                        qdgdfv_input_poll();

                    i = 1;
                }
                else
                if (o == 3)
                    _player_cancelled = 1;

                if (o != 2)
                    break;
            }
        }

        qdgdfv_dump_virtual_screen();
    }

    while (_qdgdfv_key_escape)
        qdgdfv_input_poll();

    filp_unref_value(v);
}


void game_epilogue(void)
/* what is shown after winning the episode */
{
    char palette[256 * 3];
    char bkgr[64000];
    FILE *f;
    struct filp_val *v;
    int n, m;
    unsigned char *msg;
    unsigned char color;

    /* load the palette */
    filp_exec("\"$data_dir/e$episode/pnext\"");
    v = filp_pop();
    f = qdgdfv_fopen(v->value, "rb");
    fseek(f, 7, 0);
    fread(palette, 256, 3, f);
    fclose(f);

    /* load the background bitmap */
    filp_exec("\"$data_dir/e$episode/next\"");
    v = filp_pop();
    f = qdgdfv_fopen(v->value, "rb");

    for (n = 0; n < sizeof(bkgr); n++) {
        int c = getc(f);
        char *p = &palette[c * 3];

        c = qdgdfv_seek_color(p[0] * 2, p[1] * 2, p[2] * 2);
        bkgr[n] = c;
    }

    filp_exec("$end_text");
    v = filp_pop();
    msg = (unsigned char *)v->value;
    m = 0;

    color = qdgdfv_seek_color(255, 255, 255);

    qdgdfv_timer(1);

    for (;;) {
        int x, y;
        unsigned char tmp[2] = " ";

        qdgdfv_clear_virtual_screen();

        /* transfer the background */
        for (n = 0; n < sizeof(bkgr); n++) {
            _qdgdfv_virtual_screen[320 * 20 + n] = bkgr[n];
        }

        x = 3;
        y = 20;

        for (n = 0; n < m && msg[n]; n++) {
            if (msg[n] == '\n') {
                x = 3;
                y += 12;
            }
            else {
                tmp[0] = msg[n];

                qdgdfv_font_print(x, y, tmp, color);

                x += qdgdfv_font_size(tmp);
            }
        }

        qdgdfv_dump_virtual_screen();

        if (qdgdfv_timer(0) > 100) {
            m++;
            qdgdfv_timer(1);
        }

        qdgdfv_input_poll();

        if (_qdgdfv_key_escape)
            break;

        if (_qdgdfv_key_enter)
            m = 1000;
    }

    while (_qdgdfv_key_escape)
        qdgdfv_input_poll();
}


/**
 * game_main_loop - Main game loop
 *
 * Processes the main game loop, calling all other game
 * code. It only returns on user cancellation, winning or
 * loosing the game (dying).
 */
void game_main_loop(void)
{
    char tmp[25];
    int n, t;

    _player_hit = 0;
    _message_ticks = 0;

    qdgdfa_play_sound(_soundtrack, 1);

    qdgdfv_timer(1);

    while (!_player_won && !_player_cancelled && _player_health > 0) {
        qdgdfv_input_poll();

        /* game exit request */
        if (_qdgdfv_key_escape)
            game_main_loop_menu();

        if (_qdgdfv_key_f10)
            filp_console();

        /* process the player */
        move_player();

        /* process the bullets */
        move_bullets();

        /* process the demons */
        move_demons(_player_x, _player_y);

        /* add all the sprites to the rendering image */
        add_sprites(_player_x, _player_y, _player_a);

        /* calculate elapsed ticks, ensuring not to overload */
        t = _ticks_elapsed = qdgdfv_timer(1);

        if (_ticks_elapsed > 150)
            _ticks_elapsed = 150;

        /* order finally the painting */
        render(_player_x, _player_y, _player_a);

        /* number of items */
        for (n = _taken_items = 0; n < MAX_ITEMS; n++)
            _taken_items += _items[n].taken;

        /* adventure total */
        _adventure_total = (((_taken_items * 10) + _player_kills) * 100) /
            ((_num_items * 10) + _killable_demons);

        /* info printing */

        /* room name */
        qdgdfv_font_print(-1, -1, (unsigned char *) _room_name, _info_color);

        sprintf(tmp, "Total: %d%%", _adventure_total);
        qdgdfv_font_print(0, 0, (unsigned char *) tmp, _info_color);

        sprintf(tmp, "Life: %d%%", _player_health);
        qdgdfv_font_print(0, 12, (unsigned char *) tmp, _info_color);

        sprintf(tmp, "Kills: %d/%d", _player_kills, _killable_demons);
        qdgdfv_font_print(-2, 0, (unsigned char *) tmp, _info_color);

        sprintf(tmp, "Items: %d", _taken_items);
        qdgdfv_font_print(-2, 12, (unsigned char *) tmp, _info_color);

        if (_message_ticks > 0) {
            unsigned char *ptr = (unsigned char *)_message_text;
            int n = _message_y;

            _message_ticks -= _ticks_elapsed;

            do {
                qdgdfv_font_print(-1, n, ptr, _message_color);

                ptr += strlen((char *)ptr) + 1;
                n += 12;
            } while (*ptr);
        }

        if (_show_ticks) {
            sprintf(tmp, "%d", t);
            qdgdfv_font_print(0, -1, (unsigned char *) tmp, _info_color);
        }

        if (_player_hit > 0) {
            _player_hit -= _ticks_elapsed;

            for (n = 0; n < _qdgdfv_screen_x_size * _qdgdfv_screen_y_size; n++)
                _qdgdfv_virtual_screen[n] =
                    _red_color_blend[_qdgdfv_virtual_screen[n]];
        }

        qdgdfv_dump_virtual_screen();
    }

    if (_player_health <= 0) {
        qdgdfv_timer(1);

        message("MSG_YOU_DIED", 0);

        while (qdgdfv_timer(0) < 7000) {
            qdgdfv_input_poll();

            if (_qdgdfv_key_escape)
                break;

            add_sprites(_player_x, _player_y, _player_a);
            render(_player_x, _player_y, _player_a);

            qdgdfv_font_print(-1, -32,
                      (unsigned char *) _message_text, _info_color);

            qdgdfv_dump_virtual_screen();
        }
    }

    if (_player_won) {
        game_epilogue();
    }
}


/**
 * game_startup - Game initialization
 *
 * Initializes several game variables and code.
 */
void game_startup(void)
{
    filp_ext_string("message_text", _message_text, sizeof(_message_text));

    filp_ext_int("demon_bullet_strength", &_demon_bullet_strength);
    filp_ext_int("taken_items", &_taken_items);
    filp_ext_int("player_won", &_player_won);

    filp_ext_real("max_player_speed", &_max_player_speed);
    filp_ext_real("max_demon_speed", &_max_demon_speed);
    filp_ext_real("max_bullet_speed", &_max_bullet_speed);
}
