/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Renderer code and related utilities

*/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "qdgdf_video.h"

#include "render.h"
#include "map.h"
#include "game.h"

/******************
    Data
*******************/

/* the angle (in radians) of a scanline */
double _small_angle;

/* the ambient light */
int _ambient_light;

/* render height, _qdgdfv_screen_y_size casted to double */
double _render_height;

/* the z buffer */
double *_z_buffer = NULL;

/* correction factor for sprite angle rotation */
double _sprite_rotation_correction = 1;

/* the scanline buffer */
unsigned char *_scanline_buffer = NULL;

/* the rays */
struct ray_hit rv;
struct ray_hit rh;

/* the light table */
unsigned char _light_table[LIGHT_LEVELS][256];

/* conversion array for original freaks palette */
unsigned char _pal_conv[256];

/* perspective correct factor */
double _perspective_correct = 1;

/* the list of sprites */
struct {
    double dist;
    unsigned char *bmp;
    int sl;
    int hr;
    double bi;
    double o;
} _sprites[MAX_SPRITES];

int _num_sprites = 0;

/* frame counter */
int _frame_counter = 0;

/* submarine effect */
int _under_water = 0;
static double _under_water_distort = 1;

/* water distort factor */
double _water_distort_factor = 8.0;

/* the landscape */
unsigned char _landscape[LANDSCAPE_Y_SIZE][LANDSCAPE_X_SIZE];
int _use_landscape = 0;

/* blinking (weak) light */
int _blink_light = 0;

/* random seeds */
unsigned long _ground_seed = 0;
unsigned long _blink_seed = 0;
unsigned long _rain_seed = 0;

/* the rain */
#define MAX_RAIN_DROPS 64
struct rain {
    int x;
    int y;
    int speed;
} _rain[MAX_RAIN_DROPS];

int _use_rain = 0;
static int _raining_ticks = 0;
unsigned char _rain_colors[6];

/* lightning */
int _use_lightning = 0;
static int _lightning_now = 0;
static int _lightning_ticks = 0;
unsigned char _lightning_color;
int _lightning_ambient_light = 256;

/* boring floor (but much faster) */
int _boring_floor = 0;

/* half size of the vertical screen (for speed and clarity) */
static int _half_screen_y_size;

/* hscan distances */
double *_hscan_distances = NULL;

/* movement counter (to randomize the ground) */
int _move_count = 0;

/* the red blend color table */
unsigned char _red_color_blend[256];

/* the dim blend color table */
unsigned char _dim_color_blend[256];


/******************
    Code
 ******************/

/* basic toolkit */

/**
 * distance - Calculates a distance
 * @tx: horizontal distance
 * @ty: vertical distance
 *
 * Calculates a distance. @tx and @ty must be the difference
 * between the two points' coordinates.
 */
double distance(double tx, double ty)
{
    return sqrt((tx * tx) + (ty * ty));
}


/**
 * light_level - calculates the light level of a distance
 * @dist: the distance
 *
 * Calculates the light level of a distance. Depends upon
 * the ambient light. Light level won't be out of range.
 * Returns the light level.
 */
int light_level(double dist)
{
    int l;

    l = (LIGHT_LEVELS - 1) - ((int) dist / _ambient_light);

    /* test possible out of range conditions */
    if (l < 0)
        l = 0;
    else if (l >= LIGHT_LEVELS)
        l = LIGHT_LEVELS - 1;

    return l;
}


/**
 * hscan_distance - Calculate the distance of a horizontal scan
 * @hscan: horizontal scan
 *
 * Calculates the distance of a horizontal scan. The horizontal
 * scan is the half of the render height when at focal distance
 * and 1 when very, very far (the middle of the screen).
 */
double hscan_distance(int hscan)
{
    return (FOCAL_DISTANCE * _half_screen_y_size) / hscan;
}


/**
 * relative_height - Calculates the height relative to a distance
 * @dist: the distance
 *
 * Calculates the height of a bitmap (that is full screen size when
 * dist is the focal distance) relative to a distance.
 * Returns the height.
 */
int relative_height(double dist)
{
    return (FOCAL_DISTANCE * _render_height) / dist;
}


/**
 * bitmap_step - Calculates the bitmap step related to a distance
 * @dist: the distance
 *
 * Calculates the bitmap step related to a distance. This value will be
 * added to current texel position to point to the next one.
 */
double bitmap_step(double dist)
{
    return (BITMAP_HEIGHT * dist) / (_render_height * FOCAL_DISTANCE);
}


/**
 * its_raining - Generates the raining effect
 *
 * Generates the raining effect over the virtual screen.
 */
void its_raining(void)
{
    int n, m;
    unsigned char *ptr;
    static int _rain_speeds[] = { 6, 3, 2 };
    unsigned char *max;

    max = _qdgdfv_virtual_screen + _qdgdfv_screen_x_size * _qdgdfv_screen_y_size;

    /* init the rain colors, in needed */
    if (_rain_colors[0] == 0) {
        for (n = 0, m = 32; n < sizeof(_rain_colors);
             n++, m += (255 / sizeof(_rain_colors)))
            _rain_colors[n] = qdgdfv_seek_color(m, m, m);
    }

    _raining_ticks -= _ticks_elapsed;

    for (n = 0; n < MAX_RAIN_DROPS; n++) {
        if (_raining_ticks <= 0) {
            _rain[n].y += (_rain[n].speed * 4);

            if (_rain[n].speed == 0 || _rain[n].y > (int) (_qdgdfv_screen_y_size -
                                       sizeof(_rain_colors) *
                                       3)) {
                if (_rain[n].speed == 0)
                    _rain[n].y = rnd(&_rain_seed) % _qdgdfv_screen_y_size;
                else
                    _rain[n].y = rnd(&_rain_seed) % 16;

                _rain[n].x = rnd(&_rain_seed) % _qdgdfv_screen_x_size;
                _rain[n].speed = _rain_speeds[rnd(&_rain_seed) % 3];
            }
        }

        ptr = _qdgdfv_virtual_screen + _rain[n].x +
            (_rain[n].y * _qdgdfv_screen_x_size);

        for (m = 0; m < sizeof(_rain_colors); m += (8 / _rain[n].speed)) {
            if (ptr > max)
                break;

            *ptr = _rain_colors[m];
            ptr += _qdgdfv_screen_x_size;

            if (ptr > max)
                break;

            *ptr = _rain_colors[m];
            ptr += _qdgdfv_screen_x_size;

            if (ptr > max)
                break;

            *ptr = _rain_colors[m];
            ptr += _qdgdfv_screen_x_size;
        }
    }

    if (_raining_ticks <= 0)
        _raining_ticks = 50;
}


/**********************
      Ray casting
***********************/


/**
 * update_ray - Updates a ray
 * @r: a pointer to the ray intersection data
 *
 * Updates the ray, calculating the next block intersection
 * (skipping the ones with no paintable wall). The bitmap
 * scanline and the perspective-corrected distance is also
 * calculated. If the ray goes out of the map, the distance
 * is set to MAX_DIST, invalidating the ray.
 */
static void update_ray(struct ray_hit *r)
{
    double vd;
    int bx, by;
    struct block *b;
    int bmp_offset;
    unsigned char *bmp;

    if (r->dist == MAX_DIST)
        return;
    if (!r->used)
        return;

    for (;;) {
        if (!r->hor) {
            vd = r->y + (r->dy * (r->tx - r->x)) / r->dx;

            r->cx = r->tx;
            r->cy = vd;
            r->dist = (r->tx - r->x) / r->dx;

            if (r->incx < 0)
                r->cx--;
        }
        else {
            vd = r->x + (r->dx * (r->ty - r->y)) / r->dy;

            r->cx = vd;
            r->cy = r->ty;
            r->dist = (r->ty - r->y) / r->dy;

            if (r->incy < 0)
                r->cy--;
        }

        bx = (int) (r->cx / CELL_WIDTH);
        by = (int) (r->cy / CELL_WIDTH);

        /* test if out of map boundaries */
        if (bx < 0 || by < 0 || bx >= _crp->tx || by >= _crp->ty) {
            /* this ray is not usable anymore */
            r->dist = MAX_DIST;
            return;
        }

        /* point to next cross */
        r->tx += r->incx;
        r->ty += r->incy;

        /* test if this map block has something in the
           face we want */
        b = MAP_BLOCK_PTR(bx, by);

        if (b->face[r->face] != -1)
            break;
    }

    bmp_offset = ((int) vd) & (CELL_WIDTH - 1);

    /* set the real wall to be drawn */
    bmp = _bmp_walls[b->face[r->face]];
    r->bmp = &bmp[bmp_offset * CELL_WIDTH];

    r->dist *= _perspective_correct;

    if (_under_water)
        r->dist += _under_water_distort;

    r->used = 0;
}


#define ADJUST_RAY(d,i,t) if(d < 0) \
                i =- CELL_WIDTH; \
              else { \
                i = CELL_WIDTH; \
                t += CELL_WIDTH; \
              }

/**
 * start_ray - Casts a ray
 * @x: x position of POV
 * @y: y position of POV
 * @a: angle of POV
 *
 * Casts a ray, filling with initial values the two
 * structures that control the intersection of the
 * ray with the block boundaries.
 */
void start_ray(double x, double y, double a)
{
    double dx, dy;
    double tx, ty;
    double incx, incy;

    dy = -cos(a);
    dx = sin(a);

    tx = (double) (((int) x) & BLOCK_BASE_MASK);
    ty = (double) (((int) y) & BLOCK_BASE_MASK);

    ADJUST_RAY(dx, incx, tx);
    ADJUST_RAY(dy, incy, ty);

    rv.hor = 0;
    rh.hor = 1;
    rv.dx = rh.dx = dx;
    rv.dy = rh.dy = dy;
    rv.tx = rh.tx = tx;
    rv.ty = rh.ty = ty;
    rv.incx = rh.incx = incx;
    rv.incy = rh.incy = incy;
    rv.x = rh.x = x;
    rv.y = rh.y = y;
    rv.used = rh.used = 1;

    /* calculate what face will be found */
    rv.face = incx > 0 ? 3 : 1;
    rh.face = incy > 0 ? 0 : 2;

    /* ignore unreachable hits */
    rv.dist = dx ? 0 : MAX_DIST;
    rh.dist = dy ? 0 : MAX_DIST;
}


/**
 * get_ray - Gets the best ray intersection
 *
 * Gets the nearest intersection (from horizontal or
 * vertical) of the ray with the map block boundaries.
 * The intersection is marked as used for next
 * update. If no more intersections are available
 * (i.e. the ray is out of the map), NULL is returned.
 */
struct ray_hit *get_ray(void)
{
    struct ray_hit *r;

    update_ray(&rv);
    update_ray(&rh);

    /* no more to draw */
    if (rv.dist == MAX_DIST && rh.dist == MAX_DIST)
        return NULL;

    if (rv.dist < rh.dist)
        r = &rv;
    else
        r = &rh;

    r->used = 1;
    return r;
}


/**
 * draw_bmp - Draws a scanline from a bmp
 * @bmp: pointer to the bitmap scanline
 * @dist: the distance of the bitmap
 *
 * Draws a scanline from a bmp, using its relative position,
 * size and light level calculated by the distance. Transparencies
 * (color 255) and the z buffer is used.
 */
static void draw_bmp(const unsigned char *bmp, double dist)
{
    double bi, o;
    int li, hr, p;
    int n, c;

    if (dist <= 0.1)
        dist = 0.1;

    bi = bitmap_step(dist);
    li = light_level(dist);
    hr = relative_height(dist);

    p = _half_screen_y_size - (hr / 2);

    if (p < 0) {
        o = ((double) -p) * bi;
        p = 0;
        hr = _qdgdfv_screen_y_size;
    }
    else
        o = 0;

    hr += p;
    for (n = p; n < hr; n++) {
        c = bmp[(int) o];
        o += bi;

        if (c != 255) {
            /* test z buffer */
            if (dist < _z_buffer[n]) {
                _scanline_buffer[n] = _light_table[li][c];
                _z_buffer[n] = dist;
            }
        }
    }
}


/**
 * draw_landscape_scanline - Draws a landscape scanline
 * @a: POV angle
 * @sl: scanline
 *
 * Draws a landscape scanline over the scanline buffer.
 */
static void draw_landscape_scanline(double a, int sl)
{
    double bi, o;
    int n;
    unsigned char *ptr;
    unsigned char c;

    n = (sl + (int) (a * 90)) % LANDSCAPE_Y_SIZE;
    if (n < 0)
        n += LANDSCAPE_Y_SIZE;

    ptr = &_landscape[n][0];
    bi = LANDSCAPE_X_SIZE / _render_height;
    o = 0;

    for (n = 0; n < _half_screen_y_size; n++) {
        c = ptr[(int) o];
        _scanline_buffer[n] = c;
        o += bi;
    }
}


/**
 * init_scan_line - Initializes the scan line ray-casting.
 * @x: x position of POV
 * @y: y position of POV
 * @a: angle of POV
 * @sl: the scanline number
 *
 * Initializes the ray-casting for this scan line. The
 * perspective correction factor and underwater distortion are
 * calculated, the z buffer filled with MAX_DIST (i.e. cleared)
 * and the scanline buffer is filled with initial content.
 * This function takes into account the landscape, lightning
 * and under water effects.
 */
static void init_scan_line(double x, double y, double a, int sl)
{
    int n, li;
    double d;

    n = (sl - (_qdgdfv_screen_x_size / 2));
    d = (n * _small_angle);

    _perspective_correct = cos(d);
    a += d;

    if (_use_landscape)
        draw_landscape_scanline(a, sl);
    else if (_lightning_now)
        memset(_scanline_buffer, _lightning_color, _half_screen_y_size);

    /* reset the z buffer */
    for (n = 0; n < _qdgdfv_screen_y_size; n++)
        _z_buffer[n] = MAX_DIST;

    /* fill the scanline buffer with ceiling and floor */
    if (_boring_floor == 2) {
        if (!_use_landscape && !_lightning_now)
            memset(_scanline_buffer, _crp->ceiling_color, _half_screen_y_size);

        memset(_scanline_buffer + _half_screen_y_size,
               _crp->floor_color, _half_screen_y_size);
    }
    else {
        for (n = 1; n < _half_screen_y_size + 1; n++) {
            d = _hscan_distances[n - 1];

            if (_boring_floor)
                li = light_level(d);
            else
                li = light_level(d + (rnd(&_ground_seed) % _ambient_light));

            if (!_use_landscape && !_lightning_now) {
                _scanline_buffer[_half_screen_y_size - n] =
                    _light_table[li][_crp->ceiling_color];
            }

            _scanline_buffer[_half_screen_y_size + n - 1] =
                _light_table[li][_crp->floor_color];
        }
    }

    if (_under_water)
        _under_water_distort = sin((_frame_counter + sl) / 45.0)
            * _water_distort_factor;

    start_ray(x, y, a);
}


/**
 * transfer_scan_line - Transfers the scanline buffer to the screen
 * @sl: the scanline number
 *
 * Transfer the scanline buffer to specified scanline in the virtual screen.
 */
static void transfer_scan_line(int sl)
{
    int n;
    unsigned char *ptr;

    ptr = _qdgdfv_virtual_screen + sl;

    for (n = 0; n < _qdgdfv_screen_y_size; n++) {
        *ptr = _scanline_buffer[n];
        ptr += _qdgdfv_screen_x_size;
    }
}


/**
 * add_sprite - Adds a sprite to the rendering view
 * @x: the x coordinate of the POV
 * @y: the y coordinate of the POV
 * @a: the angle of the POV
 * @bmp: pointer to the sprite bitmap
 * @sx: the x coordinate of the sprite
 * @sy: the y coordinate of the sprite
 *
 * Adds a sprite to the rendering view, if it's inside the POV
 * viewing pyramid. A sprite is a graphic that is always facing
 * the POV and usually represents the enemies, objects and such.
 */
void add_sprite(double x, double y, double a, unsigned char *bmp, double sx, double sy)
{
    double dx, dy;
    double rx, ry;
    double tx, ty;
    double dist;
    double px;
    double bi, o;
    int hr, sl;

    dx = -cos(a);
    dy = -sin(a);

    tx = x - sx;
    ty = y - sy;

    /* rotates by the angle */
    ry = -(ty * dx) + (tx * dy);

    /* if it's behind the POV, exit */
    if (ry <= 0)
        return;

    rx = ((tx * dx) + (ty * dy)) * _sprite_rotation_correction;

    /* distance to shape */
    dist = sqrt((tx * tx) + (ty * ty));

    bi = bitmap_step(dist);
    hr = relative_height(dist);

    /* proyect */
    px = (rx * FOCAL_DISTANCE) / ry;

    sl = ((int) px) + (_qdgdfv_screen_x_size / 2) - (hr / 2);

    /* if the starting scanline is beyond left margin, adapt */
    if (sl < 0) {
        hr += sl;

        if (hr <= 0)
            return;
        o = bi * (-sl);

        sl = 0;
    }
    else
        o = 0;

    /* add to list */
    _sprites[_num_sprites].dist = dist;
    _sprites[_num_sprites].bmp = bmp;
    _sprites[_num_sprites].sl = sl;
    _sprites[_num_sprites].hr = hr;
    _sprites[_num_sprites].bi = bi;
    _sprites[_num_sprites].o = o;

    /* @#@ limits should be tested */
    _num_sprites++;
}


/**
 * draw_sprites - Draws the sprites in the rendering view
 * @sl: the scanline
 *
 * Draws the sprites in the rendering view that are crossed
 * by current ray.
 */
static void draw_sprites(int sl)
{
    int n, i;
    unsigned char *bmp;

    for (n = 0; n < _num_sprites; n++) {
        if (_sprites[n].hr && sl >= _sprites[n].sl) {
            i = ((int) _sprites[n].o);
            i %= (BITMAP_HEIGHT - 1);
            i *= BITMAP_WIDTH;
            bmp = &_sprites[n].bmp[i];

            draw_bmp(bmp, _sprites[n].dist);

            _sprites[n].hr--;
            _sprites[n].o += _sprites[n].bi;
        }
    }
}


/**
 * render_scan_line - Renders a complete scanline
 * @x: x position of POV
 * @y: y position of POV
 * @a: angle of POV
 * @sl: scanline number
 *
 * Renders a complete scanline, transferring it to
 * the virtual screen.
 */
static void render_scan_line(double x, double y, double a, int sl)
{
    struct ray_hit *r;

    init_scan_line(x, y, a, sl);

    while ((r = get_ray()) != NULL)
        draw_bmp(r->bmp, r->dist);

    draw_sprites(sl);

    transfer_scan_line(sl);
}


/**
 * render - Renders a complete frame
 * @x: x position of POV
 * @y: y position of POV
 * @a: angle of POV
 *
 * Renders a complete frame over the virtual screen. It also
 * takes into account the lightning and raining effects.
 */
void render(double x, double y, double a)
{
    int n;

    _ambient_light = _room_ambient_light;

    /* seed for the ground random */
    _ground_seed = _move_count;

    if (_blink_light)
        _ambient_light += rnd(&_blink_seed) % 8;

    if (_use_lightning) {
        _lightning_ticks -= _ticks_elapsed;

        if (_lightning_ticks < 0) {
            _lightning_now ^= 1;

            if (_lightning_now)
                _lightning_ticks = 100;
            else
                _lightning_ticks = rnd(NULL) % 4000;
        }

        if (_lightning_now)
            _ambient_light = _lightning_ambient_light;
    }
    else
        _lightning_now = 0;

    for (n = 0; n < _qdgdfv_screen_x_size; n++)
        render_scan_line(x, y, a, n);

    if (_use_rain)
        its_raining();

    _frame_counter++;
}


/**
 * render_startup - Render engine startup
 *
 * Starts up the rendering engine.
 */
void render_startup(void)
{
    int n;

    /* calculates the render height, just for convenience */
    _render_height = (double) _qdgdfv_screen_y_size;

    /* calculate the angle of each scanline */
    _small_angle = VISION_ANGLE / (double) _qdgdfv_screen_x_size;

    /* alloc the z buffer and the scanline buffer */
    _z_buffer = (double *) malloc(sizeof(double) * _qdgdfv_screen_y_size);
    _scanline_buffer = (unsigned char *) malloc(_qdgdfv_screen_y_size);

    /* calculate the sprite rotation correction */
    _sprite_rotation_correction = (2.9 * (double) _qdgdfv_screen_x_size) / 320;

    /* init the rain struct */
    memset(_rain, '\0', sizeof(_rain));
    memset(_rain_colors, '\0', sizeof(_rain_colors));

    /* get the color of lightnings */
    _lightning_color = qdgdfv_seek_color(255, 255, 255);

    /* half the screen size, for clarity and speed */
    _half_screen_y_size = _qdgdfv_screen_y_size / 2;

    /* fill the hscan buffer */
    _hscan_distances = (double *) malloc(_half_screen_y_size * sizeof(double));

    for (n = 0; n < _half_screen_y_size; n++)
        *(_hscan_distances + n) = hscan_distance(n + 1);

}
