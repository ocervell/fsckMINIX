#include "fs.h"
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <minix/ipc.h>
#include "const.h"
#include "inode.h"
#include "type.h"
#include "mfsdir.h"
#include "super.h"
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include "buf.h"

#include <sys/stat.h>
#include <dirent.h>

/* Defines */
#define EXIT_OK                    0
#define EXIT_USAGE                 1
#define EXIT_UNRESOLVED            2
#define EXIT_ROOT_CHANGED          4
#define EXIT_CHECK_FAILED          8
#define EXIT_SIGNALED             12
#define INDCHUNK	((int) (CINDIR * ZONE_NUM_SIZE))

/* Global variables */
bitchunk_t *imap_disk;			 /* imap from the disk */
bitchunk_t *zmap_disk;			 /* zmap from the disk */
static struct super_block *sb;   /* super block */
unsigned int WORDS_PER_BLOCK;    /* # words in a block */
unsigned int BLK_SIZE;			 /* block size */
int NATIVE;
bit_t ORIGIN_IMAP;		 		 /* sb->s_isearch */
bit_t ORIGIN_ZMAP;		 		 /* sb->s_zsearch */
zone_t  FIRST;					 /* first data zone */
block_t BLK_IMAP;			 	 /* starting block for imap */
block_t BLK_ZMAP;			 	 /* starting block for zmap */
block_t BLK_ILIST;			 	 /* starting block for inode table */
unsigned int N_IMAP;			 /* # blocks used for imap */
unsigned int N_ZMAP;			 /* # blocks used for zmap */
unsigned int N_ILIST;			 /* # blocks used for inode table */
unsigned int NB_INODES;			 /* # inodes */
unsigned int NB_ZONES;			 /* # zones */
unsigned int NB_USED = 0;		 /* # used (zones or inodes) */
unsigned int NB_INODES_USED = 0; /* # zones used (from IMAP) */
unsigned int NB_ZONES_USED_Z = 0;/* # zones used (from ZMAP) */
unsigned int NB_ZONES_USED_I = 0;/* # zones used (from IMAP) */

int repair    = 0;
int markdirty = 0;
int type = 0;

/*===========================================================================*
 *				fs_inodewalker			     *
 *===========================================================================*/
int fs_inodewalker()
{
	/* Get the list of blocks in use by the system from the inode bitmap */
	printf("=== INODEWALKER ===\n");
	printf("Getting super node from device %llu ...\n", fs_dev);
	type = IMAP;
	sb = get_super(fs_dev);
	read_super(sb);
	lsuper();
	init_global();
	imap_disk = alloc_bitmap(N_IMAP);
	printf("Loading inode bitmap from disk ...\n");
	get_bitmap(imap_disk, IMAP);
	printf(" done.\n");
	sleep(3);
	//print_bitmap(imap_disk);
	int *list_inodes = get_list_used(imap_disk, IMAP);
	sleep(5);
	int *list_blocks;
	if ((list_blocks = get_list_blocks_from_inodes(list_inodes)) == NULL)
		return -1;
	free_bitmap(imap_disk);
	return 0;
}

/*===========================================================================*
 *				fs_zonewalker			     *
 *===========================================================================*/
int fs_zonewalker()
{
	/* Get the list of blocks used by the system from the zone bitmap */
	printf("=== ZONEWALKER ===\n");
	printf("Getting super node from device %llu ...\n", fs_dev);
	type = ZMAP;
	sb = get_super(fs_dev);
	read_super(sb);
	lsuper();
	sleep(3);
	init_global();
	zmap_disk = alloc_bitmap(N_ZMAP);
	printf("Loading zone bitmap from disk ...\n");
	get_bitmap(zmap_disk, ZMAP);
	printf(" done.\n\n");
	sleep(3);
	//print_bitmap(zmap_disk);
	int* list = get_list_used(zmap_disk, ZMAP);
	free_bitmap(zmap_disk);
	return 0;
}

/*===========================================================================*
 *				fs_recovery			        *
 *===========================================================================*/
/* System call recovery
int do_recovery(){
	printf("FUCK YOU EVEN MORE\n");
	printf("MODE: %d", m_in.m1_i1);
	return 0;
}

 System call damage inode
int do_damage_inode(){
	printf("FUCK YOU EVEN EVEN MORE\n");
	printf("INODE NR: %d", m_in.m1_i1);
	return 0;
}

 System call damage directory file
int do_damage_dir(){
	char* msg = m_in.m1_p1;
	printf("FUCK YOU EVEN EVEN MORE\n");
	printf("DIRECTORY PATH: %s", msg);
	return 0;
}
*/
/*===========================================================================*
 *				init_global		     *
 *===========================================================================*/
void init_global()
{
	/* Initialize all global variables for convenience of names */
	BLK_SIZE        = sb->s_block_size;
	FIRST       	= sb->s_firstdatazone;
	NB_INODES		= sb->s_ninodes;
	NB_ZONES		= sb->s_zones;
	N_IMAP 			= sb->s_imap_blocks;
	N_ZMAP 			= sb->s_zmap_blocks;
	N_ILIST			= (sb->s_ninodes + V2_INODES_PER_BLOCK(BLK_SIZE)-1) / V2_INODES_PER_BLOCK(BLK_SIZE);
	ORIGIN_IMAP		= sb->s_isearch;
	ORIGIN_ZMAP		= sb->s_zsearch;
	NATIVE			= sb->s_native;
	BLK_IMAP 		= 2;
	BLK_ZMAP 		= BLK_IMAP + N_IMAP;
	BLK_ILIST 		= BLK_ZMAP + N_ZMAP;
	WORDS_PER_BLOCK = BLK_SIZE / (int) sizeof(bitchunk_t);
}

/*===========================================================================*
 *				get_list_used	     *
 *===========================================================================*/
int* get_list_used(bitchunk_t *bitmap, int type)
{
	/* Get a list of unused blocks/inodes from the zone/inode bitmap */
	int* list;
	int nblk;
	int tot;
	bitchunk_t *buf;
	char* chunk;
	NB_USED = 0;
	if (type == IMAP){
		nblk = N_IMAP;
		tot  = NB_INODES;
		list = malloc(sizeof(int)*NB_INODES);
		printf("============= Used inodes ==============\n");
	}
	else /* type == ZMAP */ {
		nblk = N_ZMAP;
		tot  = NB_ZONES;
		list = malloc(sizeof(int)*NB_ZONES);
		printf("============= Used blocks ==============\n");
	}
	sleep(1);
	printf("\n=========================================\n");
	/* Loop through bitchunks in bitmap */
	for (int j = 0; j < FS_BITMAP_CHUNKS(BLK_SIZE)*nblk; ++j){
		chunk = int2binstr(bitmap[j]);
		/* Loop through bits in bitchunk */
		for (int k = 0; k < strlen(chunk); ++k){
			if (chunk[k] == '1'){
				list[NB_USED] = j*FS_BITCHUNK_BITS + k;
				printf("%d, ", list[NB_USED]);
				if (NB_USED % 5 == 0){
					printf("\n");
				}
				++NB_USED;
			}
		}
	}
	if (type == IMAP)    NB_INODES_USED  = NB_USED;
	else/*(type==ZMAP)*/ NB_ZONES_USED_Z = NB_USED;
	printf("\n=========================================\n\n");
	printf("Used: %d / %d \n", NB_USED, tot);
	return list;
}

/*===========================================================================*
 *				get_list_blocks_from_inodes			     *
 *===========================================================================*/
int* get_list_blocks_from_inodes(int* inodes)
{
	/* From a list of inode numbers, fetch each inode and list the
	 * blocks associated with it.
	 */
	int* list = malloc(sizeof(int)*NB_INODES*V2_NR_DZONES);;
	int used_zones = 0;
	int indirect_zones = 0;
	int double_indirect_zones = 0;
	struct inode *rip;
	int* zones;
	zone_t *indir, *double_indir;
	struct buf *buf;
	int i, j, k = 0;

	/* Fetch inodes from their number */
	for (i = 0; i != NB_INODES_USED; ++i){
		printf("INODE No. %d\n", i);

		/* If inode not found, return because it is not normal */
		if ((rip = get_inode(fs_dev, inodes[i])) == NULL){
			fatal("Inode not found\n");
			return NULL;
		}
		/* If inode has no link, no bother checking the zones */
		if (rip->i_nlinks == NO_LINK){
			printf("INODE No. %d is actually free !\n", inodes[i]);
			put_inode(rip);
			continue;
		}
		zones = rip->i_zone;

		/* Check direct zones (zones[0] --> zones[6]) */
		for (j = 0; j < V2_NR_DZONES; ++j){
			if (zones[j] == 0) break;
			list[used_zones] = zones[j];
			printf("\tZone %d : %d\n", j, list[used_zones]);
			used_zones++;
		}

		/* Check indirect zones (zones[7]) */
		if (zones[j] == 0) {
			put_inode(rip); //release inode
			continue; 		//skip iteration if no indirect zone
		}
		printf("\tZone %d : %d\n", j, zones[j]);
		indir = check_indir(zones[j]); //get list from indirect zone
		for (k = 0; k < BLK_SIZE/2; ++k){
			if (indir[k] == 0) break;
			printf("\t\tZone %d : %d\n", k, indir[k]);
			list[used_zones] = indir[k];
			used_zones++;
			indirect_zones++;
		}
		free(indir);

		/* Check double indirect zones (zones[8]) */
		j++;
		if (zones[j] == 0) {
			put_inode(rip); //release inode
			continue; 		//skip iteration if no indirect zone
		}
		printf("\tZone %d : %d\n", j, zones[j]);
		double_indir = check_double_indir(zones[j]);
		for (int k = 0; k < BLK_SIZE/2*BLK_SIZE/2; ++k){
			if (double_indir[k] == 0) break;
			printf("\t\tZone %d : %d\n", k, double_indir[k]);
			list[used_zones] = double_indir[k];
			used_zones++;
			double_indirect_zones++;
		}
		free(double_indir);

		/* We should check triple indirect zones here (zones[10]) */
		/* but none of our device has such big files that need    */
		/* triple indirect zones.								  */

		/* Free inode */
		put_inode(rip);
	}
	printf("============= Used zones ==============\n");
	sleep(2);
	for (int k = 0; k < used_zones; ++k){
		printf("%d, ", list[k]);
		if ((k % 5) == 0) printf("\n");
	}
	printf("\n==========================================\n\n");
	printf("Number of used zones:            %d\n", used_zones);
	printf("Number of indirect zones:        %d\n", indirect_zones);
	printf("Number of double indirect zones: %d\n", double_indirect_zones);
	NB_ZONES_USED_I = used_zones;
	return list;
}

/*===========================================================================*
 *				check_indir		     *
 *===========================================================================*/
int *check_indir(zone_t zno)
{
	/* Check an indirect zone and return a list of used zones */
	struct buf *buf;
	zone_t *indir;
	int used_zones = 0;
	int l = 0;
	int *list = calloc(sizeof(int), BLK_SIZE/2);

	if (zno == 0) return NULL; //return NULL if no block referenced

	buf = get_block(fs_dev, zno, 0);
	indir = b_v2_ind(buf);

	for (l = 0; l < BLK_SIZE/2; ++l){
		if (indir[l] == 0) break;
		list[used_zones] = indir[l];
		used_zones++;
	}

	return list;
}

/*===========================================================================*
 *				check_double_indir		     *
 *===========================================================================*/
int *check_double_indir(zone_t zno)
{
	/* Check a double indirect zone and return a list of used zones */
	struct buf *buf;
	zone_t *indir, *double_indir;
	int *list = calloc(sizeof(int), BLK_SIZE/2*BLK_SIZE/2);
	int used_zones = 0;
	int l = 0;

	if (zno == 0) return NULL; //return NULL if no block referenced
	buf = get_block(fs_dev, zno, 0);
	double_indir = b_v2_ind(buf);

	for (int i = 0; i < BLK_SIZE/2; ++i){
		if (double_indir[i] == 0) break;
		indir = check_indir(double_indir[i]);
		if (indir == NULL) return NULL;
		for (int j = 0; j < BLK_SIZE/2; ++j){
			list[used_zones] = indir[j];
			used_zones++;
		}
	}
	free(indir);
	return list;
}

/*===========================================================================*
 *				get_bitmap	         		*
 *===========================================================================*/
void get_bitmap(bitmap, type)
bitchunk_t *bitmap;
int type;
{
	/* Get a bitmap (imap or zmap) from disk */
	block_t *list;
	block_t bno;
	int nblk;
	if (type == IMAP){
		bno  = BLK_IMAP;
		nblk = N_IMAP;
	}
	else /* type == ZMAP */ {
		bno  = BLK_ZMAP;
		nblk = N_ZMAP;
	}
	register int i;
	register bitchunk_t *p;
	register struct buf *bp;
	register bitchunk_t *bf;
	p = bitmap;
	for (i = 0; i < nblk; i++, bno++, p += FS_BITMAP_CHUNKS(BLK_SIZE)){
		bp = get_block(fs_dev, bno, 0);
		for (int j = 0; j < FS_BITMAP_CHUNKS(BLK_SIZE); ++j){
			p[j]  = b_bitmap(bp)[j];
		}
	}
}

/*===========================================================================*
 *				print_bitmap	     		*
 *===========================================================================*/
void print_bitmap(bitmap)
bitchunk_t *bitmap;
{
	bitchunk_t *bf;
	int nblk;
	if (type == IMAP){
		printf("======== Inode bitmap (first chunks) =======\n");
		nblk = N_IMAP;
	}
	else /* type == ZMAP */{
		printf("======== Zone bitmap (first chunks) ========\n");
		nblk = N_ZMAP;
	}
	printf("\n=========================================\n");
	for (int j = 0; j < FS_BITMAP_CHUNKS(BLK_SIZE)*nblk; ++j){
		printf("%s\n", int2binstr(bitmap[j]));
	}
	printf("\n==========================================\n\n");
}

/*===========================================================================*
 *				int2binstr		     		*
 *===========================================================================*/
char *int2binstr(unsigned int i)
{
	/* Convert an int to a binary string */
	size_t bits = sizeof(unsigned int) * CHAR_BIT;
	char * str = malloc(bits + 1);
	if(!str) return NULL;
	str[bits] = 0;

	unsigned u = *(unsigned *)&i;
	for(; bits--; u >>= 1)
		str[bits] = u & 1 ? '1' : '0';

	return str;
}

/*===========================================================================*
 *				lsuper			     	    *
 *===========================================================================*/
void lsuper()
{
	/* Make a listing of the super block. */
	printf("ninodes       = %u\n", sb->s_ninodes);
	printf("nzones        = %d\n", sb->s_zones);
	printf("imap_blocks   = %u\n", sb->s_imap_blocks);
	printf("zmap_blocks   = %u\n", sb->s_zmap_blocks);
	printf("firstdatazone = %u\n", sb->s_firstdatazone_old);
	printf("log_zone_size = %u\n", sb->s_log_zone_size);
	printf("maxsize       = %d\n", sb->s_max_size);
	printf("block size    = %d\n", sb->s_block_size);
	printf("flags         = ");
	if(sb->s_flags & MFSFLAG_CLEAN) printf("CLEAN "); else printf("DIRTY ");
	printf("\n\n");
}

/*===========================================================================*
 *				list_inode			  	    *
 *===========================================================================*/
void list_inode(struct inode* i){
	/* List all fields of an inode */
	printf("nlinks = %d\n", i->i_nlinks);
	printf("uid	 = %d\n", i->i_uid);
	printf("gid	 = %d\n", i->i_gid);
	printf("size	 = %d\n", i->i_size);
	printf("atime	 = %d\n", i->i_atime);
	printf("mtime	 = %d\n", i->i_mtime);
	printf("ctime	 = %d\n", i->i_ctime);
	int* zones = i->i_zone;
	for (int i = 0; i < V2_NR_TZONES; ++i){
		printf("zone %d   = %d\n", i, zones[i]);
	}
}

/*===========================================================================*
 *				alloc			     		*
 *===========================================================================*/
char *alloc(nelem, elsize)
unsigned nelem, elsize;
{
	/* Allocate some memory and zero it. */
	char *p;

	if ((p = (char *)malloc((size_t)nelem * elsize)) == 0) {
		fatal("out of memory");
	}
	memset((void *) p, 0, (size_t)nelem * elsize);
	return(p);
}

/*===========================================================================*
 *				alloc_bitmap	     		*
 *===========================================================================*/
bitchunk_t *alloc_bitmap(nblk)
int nblk;
{
  /* Allocate `nblk' blocks worth of bitmap. */
  register bitchunk_t *bitmap;
  bitmap = (bitchunk_t *) alloc((unsigned) nblk, BLK_SIZE);
  *bitmap |= 1;
  return bitmap;
}
/*===========================================================================*
 *				free_bitmap		     		*
 *===========================================================================*/
void free_bitmap(p)
bitchunk_t *p;
{
  /* Deallocate the bitmap `p'. */
  free((char *) p);
}

/*===========================================================================*
 *				fatal			     		*
 *===========================================================================*/
void fatal(s)
char *s;
{
	/* Print the string `s' and exit. */
	printf("%s\nfatal\n", s);
	exit(EXIT_CHECK_FAILED);
}
