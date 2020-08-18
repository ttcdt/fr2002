/*

	frconv.c

*/

#include <stdio.h>
#include <string.h>
#include "filp.h"

struct
{
	int demon_id;
	int hit;
	int ticks;
	int move;
} demondesc[16];

char wbmp[10];

char block[11];

#define FB_WALL 	0
#define FB_DEMON	1
#define FB_ITEM 	2
#define FB_GATE 	3
#define FB_OPAQUE	4
#define FB_EMPTY	5

#define BLOCK_SIZE	64

struct
{
	char walls[4];
} wblocks[1000];

int map[100][100];


int main(int argc, char * argv[])
{
	int epi;
	FILE * ef;
	FILE * mf;
	FILE * en;
	FILE * es;
	FILE * f;
	int n,c;
	char tmp[256];
	int cc,fc,tx,ty;
	int ddn,ddi;
	int x,y;
	struct filp_val * v;
	int _max_demons=0;

	if(argc==1)
	{
		printf("Usage: frconv episode_number\n");
		exit(1);
	}

	filp_startup();

	epi=atoi(argv[1]);

	sprintf(tmp,"e%d/lang_en.filp",epi);
	en=fopen(tmp,"w");
	sprintf(tmp,"e%d/lang_es.filp",epi);
	es=fopen(tmp,"w");

	/* get episode name and write to language files */
	sprintf(tmp,"/home/angel/frozen/freaks/id.fr%1d", epi);
	f=fopen(tmp,"r");

	fgets(tmp,sizeof(tmp),f);
	tmp[strlen(tmp)-1]='\0';
	fprintf(es,"/episode_name \"%s\" set\n\n",tmp);
	fgets(tmp,sizeof(tmp),f);
	tmp[strlen(tmp)-1]='\0';
	fprintf(en,"/episode_name \"%s\" set\n\n",tmp);
	fclose(f);

	fprintf(en,"/room_names (\n");
	fprintf(es,"/room_names (\n");

	/* open episode file */
	sprintf(tmp,"e%d/map.filp",epi);
	ef=fopen(tmp,"w");

	fprintf(ef,"/* Episode %d - map */ \n",epi);

	/* open mutable things file */
	sprintf(tmp,"e%d/episode.filp",epi);
	mf=fopen(tmp,"w");

	fprintf(mf,"/* Episode %d - mutable data */ \n",epi);
	fprintf(mf,"\n%d load_map\n",epi);

	/* open initial player info */
	sprintf(tmp,"/home/angel/frozen/freaks/good.fr%1d", epi);
	f=fopen(tmp,"r");

	c=getc(f); fprintf(mf,"\n%d ",c * 90);
	c=getc(f); getc(f); fprintf(mf,"%d ",c * BLOCK_SIZE);
	c=getc(f); getc(f); fprintf(mf,"%d ",c * BLOCK_SIZE);
	getc(f); getc(f); getc(f); getc(f);
	c=getc(f); fprintf(mf, "%d player_info\n",c);
	fprintf(mf,"0 set_current_room\n");
	fprintf(mf,"0 0 0 0 0 0 0 0 0 0 set_inventory\n");
	fclose(f);

	/* load the ids file */
	filp_execf("'e%d/ids.filp' load",epi);

	for(n=0;n<100;n++)
	{
		int item=-1;
		int itemx,itemy;
		int i,wb=0;

		sprintf(tmp,"/home/angel/frozen/freaks/lab%02d.fr%1d", n, epi);

		if((f=fopen(tmp,"r"))==NULL)
			continue;

		printf("%s\n",tmp);

		cc=getc(f);
		fc=getc(f);

		fread(tmp,40,1,f);
		fprintf(es,"\t\"%s\"\t\t/* %d */ \n",tmp,n);
		fread(tmp,40,1,f);
		fprintf(en,"\t\"%s\"\t\t/* %d */ \n",tmp,n);

		tx=getc(f);
		tx=(getc(f)<<8)+tx;
		ty=getc(f);
		ty=(getc(f)<<8)+ty;

		fprintf(ef,"\n/* room %d (%s) */ \n\n",n, tmp);
		fprintf(ef,"%d %d %d %d %d new_room\n\n", cc, fc, tx, ty, n);

		fprintf(mf,"\n/* room %d (%s) */ \n\n",n, tmp);

		ddn=getc(f);
		ddn=(getc(f)<<8)+ddn;

		fread(wbmp,10,1,f);

		/* load the demondesc */
		for(ddi=0;ddi<ddn;ddi++)
		{
			demondesc[ddi].demon_id=getc(f);
			demondesc[ddi].hit=getc(f);
			demondesc[ddi].ticks=getc(f);
			demondesc[ddi].move=getc(f);
		}

		/* reset the map */
		for(y=0;y<ty;y++)
			for(x=0;x<tx;x++)
				map[y][x]=-1;

		/* load the blocks */
		for(y=0;y<ty;y++)
		{
			for(x=0;x<tx;x++)
			{
				fread(block,11,1,f);

				switch(block[0])
				{
				case FB_ITEM:

					item=(int) block[3]+1;
					itemx=x;
					itemy=y;

					block[0]=FB_EMPTY;

					break;

				case FB_DEMON:

					filp_execf("$demon_ids %d @",
						demondesc[(int)block[3]].demon_id+1);
					v=filp_pop();

					{
						char * demon_type;
						char * demon_mode;

						switch(demondesc[(int)block[3]].move)
						{
						case 0: demon_type="STATIC"; break;
						case 1: demon_type="FOLLOW"; break;
						case 2: demon_type="SHOOT"; break;
						default: demon_type="UNKNOWN"; break;
						}

						switch(block[4])
						{
						case 0: demon_mode="MOVE"; break;
						case 1: demon_mode="ATTACK"; break;
						case 2: demon_mode="BLEED"; break;
						case 3: demon_mode="DEAD"; break;
						case 4: demon_mode="FROZEN"; break;
						default: demon_mode="UNKNOWN"; break;
						}

					fprintf(mf,"%d \"%s\" \"%s\" %d %d %d \"%s\" %d %d demon\n",
						n,v->value, demon_mode, block[5],
						demondesc[(int)block[3]].hit,
						demondesc[(int)block[3]].ticks,
						demon_type,
						(x*BLOCK_SIZE)+block[7],
						(y*BLOCK_SIZE)+block[8]);
					}

					filp_destroy_value(v);

					if(demondesc[(int)block[3]].move)
						_max_demons++;

					block[0]=FB_EMPTY;

					break;

				case FB_GATE:

					if(block[7]==-1)
					{
						fprintf(ef,"%d %d %d %d %d gate\n",
							block[8],
							((int)block[9]*BLOCK_SIZE) + 32,
							((int)block[10]*BLOCK_SIZE) + 32,
							x, y);
					}
					else
					if(block[7]==-2)
					{
						fprintf(ef,"%d %d exit_gate\n",
							x, y);
					}
					else
					{
						filp_execf("$item_ids %d @", (int)block[7]+1);
						v=filp_pop();

						fprintf(ef,"%d %d %d %d %d \"%s\" locked_gate\n",
							block[8],
							((int)block[9]*BLOCK_SIZE) + 32,
							((int)block[10]*BLOCK_SIZE) + 32,
							x, y,
							v->value);

						filp_destroy_value(v);
					}

					block[0]=FB_WALL;

					/* fall through */

				case FB_WALL:

					if(block[3]==-1 && block[4]==-1 &&
					   block[5]==-1 && block[6]==-1)
						block[0]=FB_EMPTY;

					break;
				}

				if(block[0]==FB_WALL)
				{
					if(block[3]!=-1) block[3]=wbmp[block[3]];
					if(block[4]!=-1) block[4]=wbmp[block[4]];
					if(block[5]!=-1) block[5]=wbmp[block[5]];
					if(block[6]!=-1) block[6]=wbmp[block[6]];

					/* search an equal block */
					for(i=0;i<wb;i++)
					{
						if(memcmp(wblocks[i].walls,&block[3],4)==0)
							break;
					}

					/* if not found, add to database */
					if(i==wb)
					{
						memcpy(wblocks[i].walls,&block[3],4);
						wb++;
					}

					map[y][x]=i;
				}
			}
		}

		if(item!=-1)
		{
			filp_execf("$item_ids %d @", (int)item);
			v=filp_pop();

			fprintf(ef,"\n\"%s\" %d %d item\n", v->value,
				(itemx * BLOCK_SIZE) + 32,
				(itemy * BLOCK_SIZE) + 32);

			filp_destroy_value(v);
		}

		fclose(f);

		/* write the block database */
		fprintf(ef,"\n");

		for(i=0;i<wb;i++)
		{
			int q;

			fprintf(ef,"/* %02X */ ",i);

			for(q=0;q<4;q++)
			{
				if(wblocks[i].walls[q]!=-1)
				{
					filp_execf("$wall_ids %d @",
						(int)wblocks[i].walls[q]+1);
					v=filp_pop();
					fprintf(ef,"\"%s\" ",v->value);
					filp_destroy_value(v);
				}
				else
					fprintf(ef,"NULL ");
			}

			fprintf(ef,"block\n");
		}

		/* write the map */

		fprintf(ef,"\n\"\n");

		for(y=0;y<ty;y++)
		{
			for(x=0;x<tx;x++)
			{
				if(map[y][x]==-1)
					fprintf(ef,"..");
				else
					fprintf(ef,"%02X",map[y][x]);
			}
			fprintf(ef,"\n");
		}
		fprintf(ef,"\" map\n");
	}

	fprintf(en,") set\n");
	fclose(en);

	fprintf(es,") set\n");
	fclose(es);

	fclose(ef);
	fclose(mf);

	filp_shutdown();

	return(0);
}
