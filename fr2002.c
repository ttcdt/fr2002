/*

    Freaks 2002

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filp.h"
#include "qdgdf_video.h"
#include "qdgdf_audio.h"

#include "render.h"
#include "map.h"
#include "game.h"

/******************
    Data
 ******************/

/******************
    Code
 ******************/

/**
 * misc_startup - Miscellaneous startup
 *
 * Starts up several miscellaneous things.
 */
void misc_startup(void)
{
    struct filp_val *v;

    /* loads the logo */
    filp_exec("\"$data_dir/e0/logo.pcx\"");
    v = filp_pop();

    qdgdfv_load_pcx(_logo, v->value, LOGO_WIDTH * LOGO_HEIGHT);

    /* language dependent strings */
    filp_exec("\"$data_dir/e0/strings.$lang.filp\" load");
}


int new_menu(void)
{
    int ret = 0;
    int o = 0;
    struct filp_val *v;

    filp_exec("/_file ( ) = /_menu ( ) = "
          "( 'data/e0/' 'data/e1/' 'data/e2/' 'data/e3/' 'data/e4/' { "
          " /_ #= /episode_name '' = "
          " { $_ \"strings.en.filp\" . load } eval pop "
          " { $_ \"strings.$lang.filp\" . load } eval pop "
          " $episode_name '' ne "
          " { /_menu 1 $episode_name ains /_file 1 $_ ains } if "
          "} foreach " "MSG_MENU_NEW $_menu adump '|' join .");

    v = filp_pop();
    filp_ref_value(v);

    qdgdfv_timer(1);

    while (_qdgdfv_key_enter)
        qdgdfv_input_poll();

    for (;;) {
        _ticks_elapsed = qdgdfv_timer(1);

        render(_player_x, _player_y, _player_a);

        qdgdfv_input_poll();

        if (menu(v->value, &o)) {
            if (o == -1)
                break;
            else {
                filp_execf("/_file %d @ 'episode.filp' . load", o + 1);
                ret = 1;
                break;
            }
        }

        qdgdfv_dump_virtual_screen();
    }

    filp_unref_value(v);

    return ret;
}


int options_menu(void)
{
    int ret = 0;
    int o = 0;
    struct filp_val *v;
    char *lang[] = { "en", "es", "la" };

    filp_exec("MSG_MENU_OPTS");

    v = filp_pop();
    filp_ref_value(v);

    qdgdfv_timer(1);

    while (_qdgdfv_key_enter)
        qdgdfv_input_poll();

    for (;;) {
        _ticks_elapsed = qdgdfv_timer(1);

        render(_player_x, _player_y, _player_a);

        qdgdfv_input_poll();

        if (menu(v->value, &o)) {
            if (o == -1)
                break;
            else {
                filp_execf("/lang '%s' =", lang[o]);
                filp_exec("\"$data_dir/e0/strings.$lang.filp\" load");
                break;
            }
        }

        qdgdfv_dump_virtual_screen();
    }

    while (_qdgdfv_key_enter)
        qdgdfv_input_poll();

    filp_unref_value(v);

    return ret;
}


/**
 * main_menu - Main menu processing
 *
 * Processes the main menu.
 */
int main_menu(void)
{
    int o, f;
    struct filp_val *v;
    int n, m, i;
    unsigned char *ptr;
    unsigned char *ptr2;
    unsigned char c;
    int ret = 0;

    filp_exec("\"$data_dir/e0/episode.filp\" load");

    filp_exec("MSG_MENU_MAIN");
    v = filp_pop();
    filp_ref_value(v);

    o = f = 0;
    i = (_qdgdfv_screen_x_size / 2) - (LOGO_WIDTH / 2);

    qdgdfa_play_sound(_soundtrack, 1);

    qdgdfv_timer(1);

    for (;;) {
        _ticks_elapsed = qdgdfv_timer(1);

        render(_player_x, _player_y, _player_a);

        /* transfer the logo */
        for (ptr = _qdgdfv_virtual_screen + i, n = 0; n < LOGO_HEIGHT; n++) {
            ptr2 = ptr;

            for (m = 0; m < LOGO_WIDTH; m++, ptr2++) {
                c = _logo[(n * LOGO_WIDTH) + m];

                if (c != 255)
                    *ptr2 = c;
            }

            ptr += _qdgdfv_screen_x_size;
        }

        qdgdfv_input_poll();

        if (f) {
            if (f == 1)
                show_info("$info_text");
            else
                show_info("$help_text");

            if (_qdgdfv_key_enter) {
                while (_qdgdfv_key_enter)
                    qdgdfv_input_poll();
                f = 0;
            }
            else
            if (_qdgdfv_key_escape) {
                while (_qdgdfv_key_escape)
                    qdgdfv_input_poll();
                f = 0;
            }
        }
        else {
            if (menu(v->value, &o)) {
                if (o == -1)
                    o = 0;
                else
                if (o == 0) {
                    if (new_menu())
                        break;
                }
                else
                if (o == 1) {
                    if (save_load_game_menu(0))
                        break;
                }
                else
                if (o == 2) {
                    options_menu();
                    filp_unref_value(v);

                    filp_exec("MSG_MENU_MAIN");
                    v = filp_pop();
                    filp_ref_value(v);
                }
                else
                if (o == 3) {
                    while (_qdgdfv_key_enter)
                        qdgdfv_input_poll();

                    f = 1;
                }
                else
                if (o == 4) {
                    while (_qdgdfv_key_enter)
                        qdgdfv_input_poll();

                    f = 2;
                }
                else
                if (o == 5) {
                    ret = 1;
                    break;
                }
            }
        }

        qdgdfv_dump_virtual_screen();
    }

    filp_unref_value(v);

    return ret;
}


extern const char _binary_fr_tar_start;
extern const char _binary_fr_tar_end;
extern const char binary_fr_tar_start;
extern const char binary_fr_tar_end;

int main(int argc, char *argv[])
{
#if CONFOPT_EMBED_NOUNDER == 1
    _qdgdf_embedded_tar_start = &binary_fr_tar_start;
    _qdgdf_embedded_tar_end = &binary_fr_tar_end;
#else
    _qdgdf_embedded_tar_start = &_binary_fr_tar_start;
    _qdgdf_embedded_tar_end = &_binary_fr_tar_end;
#endif

    strcpy(_qdgdfv_fopen_path, "");

    filp_external_fopen = _qdgdfv_fopen;

    filp_startup();

    _qdgdfa_16_bit = 1;
    _qdgdfv_scale = 2;
    _qdgdfv_scale2x = 0;
    _qdgdfv_screen_x_size = 320;
    _qdgdfv_screen_y_size = 240;
    strcpy(_qdgdfv_window_title, "Freaks 2.002 rel. " VERSION);
    strcpy(_qdgdfa_window_title,_qdgdfv_window_title);
    _qdgdfv_use_logger = 0;
    strcpy(_qdgdfv_logger_file, "fr2002.log");

    _qdgdfa_sound = 1;

    qdgdfa_startup();
    qdgdfv_startup();

    map_startup();

    filp_exec("\"$home_dir/config.filp\" load");
    filp_exec("'./setup.filp' load");

    render_startup();
    game_startup();
    misc_startup();

    for (;;) {
        if (main_menu())
            break;

        game_main_loop();
    }

    qdgdfv_shutdown();
    qdgdfa_shutdown();
    filp_shutdown();

    return 0;
}
