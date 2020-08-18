/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Main game code

*/

extern int _ticks_elapsed;
extern int _move_count;
extern int _player_kills;

extern int _player_cancelled;
extern int _player_won;

extern int _show_ticks;
extern int _move_count;

int angle_move(double a, double dist, double * x, double * y, int test_ray);
int angle_exact_move(double a, double dist, double * x, double * y);
double look_at(double xo, double yo, double xd, double yd);
void move_demons(double x, double y);
void add_sprites(double x, double y, double a);
void move_player(void);
void move_bullets(void);
int shoot_bullet(double x, double y, double a, int from_player);

int menu(char * menuspec, int * op);
void show_info(char * info_var);
int save_load_game_menu(int save);
void game_main_loop(void);
void game_startup(void);

