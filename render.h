/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Renderer code and related utilities

*/


#define FOCAL_DISTANCE      100.0
#define BITMAP_HEIGHT       64
#define BITMAP_WIDTH        64
#define CELL_WIDTH      64
#define CELL_HEIGHT     64
#define BLOCK_BASE_MASK     ~(CELL_WIDTH - 1)
#define AOV_PI          3.14159265358979323846
#define DEG_TO_RAD      (AOV_PI / 180.0)
#define VISION_ANGLE        (60.0 * DEG_TO_RAD)
#define LIGHT_LEVELS        16
#define MAX_LIGHT       15
#define MAX_DIST        1000000
#define MAX_SPRITES     MAX_DEMONS_PER_ROOM+32
#define LANDSCAPE_X_SIZE    100
#define LANDSCAPE_Y_SIZE    480

extern int _ambient_light;
extern double _small_angle;

extern int _under_water;
extern double _water_distort_factor;

extern unsigned char _landscape[LANDSCAPE_Y_SIZE][LANDSCAPE_X_SIZE];
extern int _use_landscape;

extern unsigned long _ground_seed;
extern unsigned long _blink_seed;

extern int _blink_light;
extern int _use_rain;
extern int _use_lightning;

extern int _move_count;
extern int _boring_floor;

extern unsigned char _light_table[LIGHT_LEVELS][256];

extern unsigned char _pal_conv[256];

extern unsigned char _red_color_blend[256];
extern unsigned char _dim_color_blend[256];

struct ray_hit
{
    double cx;
    double cy;
    double dist;
    int face;
    unsigned char * bmp;

    int hor;
    double dx;
    double dy;
    double tx;
    double ty;
    double incx;
    double incy;
    double x;
    double y;
    int used;
};

double distance(double tx, double ty);
/* int light_level(double dist); */
double hscan_distance(int hscan);
int relative_height(double dist);
double bitmap_step(double dist);

void start_ray(double x, double y, double a);
struct ray_hit * get_ray(void);

extern int _num_sprites;
void add_sprite(double x, double y, double a,
        unsigned char * bmp, double sx, double sy);

void render(double x, double y, double a);

void render_startup(void);
