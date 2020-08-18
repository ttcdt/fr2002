/*

    frbmp.c - Converts bitmaps to filp code.

    Copyright (C) 2004 Angel Ortega <angel@triptico.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    http://www.triptico.com

*/

#include <stdio.h>
#include "render.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

int main(void)
{
	int n,m;
	int c;
	unsigned char b[BITMAP_WIDTH * BITMAP_HEIGHT];

	printf("(\n\n");

	for(;;)
	{
		if(!fread(b,BITMAP_WIDTH,BITMAP_HEIGHT,stdin))
			break;

		printf("\"\n");

		for(n=0;n < BITMAP_HEIGHT;n++)
		{
			for(m=0;m < BITMAP_WIDTH;m++)
			{
				c=b[(n * BITMAP_WIDTH) + m];

				if(c == 128)
					printf("..");
				else
					printf("%02X",c);
			}

			printf("\n");
		}

		printf("\"\n\n");
	}

	printf(")\n\n");

	return(0);
}

