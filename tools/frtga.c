/*

    export bitmaps as TGA

    Angel Ortega <angel@triptico.com>

*/

#include <stdio.h>
#include <string.h>

int pal[256];

#define BITMAP_HEIGHT   64
#define BITMAP_WIDTH    64
#define BITMAP_SIZE     BITMAP_HEIGHT * BITMAP_WIDTH
#define MAX_BITMAPS     1000

unsigned char bmps[BITMAP_SIZE * MAX_BITMAPS];

int load_pal(char *fn)
{
    FILE *f;
    int ret = 0;

    if ((f = fopen(fn, "rb")) != NULL) {
        int n;

        fseek(f, 7, 0);

        for (n = 0; n < 256; n++) {
            int r, g, b;

            r = fgetc(f) * 4;
            g = fgetc(f) * 4;
            b = fgetc(f) * 4;

            pal[n] = r << 16 | g << 8 | b;
        }
    }
    else {
        printf("ERROR: cannot open palette file '%s'\n", fn);
        ret = -1;
    }

    return ret;
}


int load_bmps(char *fn)
{
    FILE *f;
    int ret = -1;

    if ((f = fopen(fn, "rb")) != NULL) {
        ret = fread(bmps, BITMAP_SIZE, MAX_BITMAPS, f);
        fclose(f);
    }
    else
        printf("ERROR: cannot open bitmap file '%s'\n", fn);

    return ret;
}


int write_tga(unsigned char *bmp, char *fn)
{
    FILE *f;
    unsigned char hdr[18];
    int ret = 0;

    if ((f = fopen(fn, "wb")) != NULL) {
        int n;

        memset(hdr, '\0', sizeof(hdr));

        hdr[2]  = 2;
        hdr[12] = BITMAP_HEIGHT;
        hdr[14] = BITMAP_WIDTH;
        hdr[16] = 32;
        hdr[17] = 32;

        fwrite(hdr, sizeof(hdr), 1, f);

        for (n = 0; n < BITMAP_SIZE; n++) {
            int c = pal[bmp[n]];

            fputc(c         & 0xff, f);
            fputc((c >> 8)  & 0xff, f);
            fputc((c >> 16) & 0xff, f);

            fputc(bmp[n] != 128 ? 0xff : 0x00, f);
        }

        fclose(f);
    }
    else {
        printf("ERROR: cannot create TGA File '%s'\n", fn);
        ret = -1;
    }

    return ret;
}


int save_bmp_set(char *fn, char *prefix)
{
    int nbmps;

    if ((nbmps = load_bmps(fn)) != -1) {
        int n;
        unsigned char *bmp = bmps;

        for (n = 0; n < nbmps; n++) {
            char tmp[100];

            sprintf(tmp, "%s_%03d.tga", prefix, n);
            write_tga(bmp, tmp);

            bmp += BITMAP_SIZE;
        }
    }

    return nbmps;
}


int main(void)
{
    int ret = 0;

    if (load_pal("data/e1/pal.bin") >= 0) {
        save_bmp_set("data/e1/demons.bin", "demons");
        save_bmp_set("data/e1/walls.bin", "walls");
        save_bmp_set("data/e1/items.bin", "items_e1");
        save_bmp_set("data/e2/items.bin", "items_e2");
        save_bmp_set("data/e3/items.bin", "items_e3");
    }
    else
        ret = 1;

    return ret;
}
