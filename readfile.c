#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define DISKFILE  argv[1]

#define K   1024
#define ELEMENTS 5

typedef int           __le32;
typedef short int     __le16;
typedef unsigned char __u8;


struct ext2_dir_entry_2 *dentry;
__u8 block[K];
int fd;
int inosize;
int inotable;
struct ext2_inode *ino;           // inode

struct ext2_super_block {
    __le32	s_inodes_count;		/* Inodes count */
    __le32	s_blocks_count;		/* Blocks count */
    __le32	s_r_blocks_count;	/* Reserved blocks count */
    __le32	s_free_blocks_count;	/* Free blocks count */
    __le32	s_free_inodes_count;	/* Free inodes count */
    __le32	s_first_data_block;	/* First Data Block */
    __le32	s_log_block_size;	/* Block size */
    __le32	s_log_frag_size;	/* Fragment size */
    __le32	s_blocks_per_group;	/* # Blocks per group */
    __le32	s_frags_per_group;	/* # Fragments per group */
    __le32	s_inodes_per_group;	/* # Inodes per group */
    __le32	s_mtime;		/* Mount time */
    __le32	s_wtime;		/* Write time */
    __le16	s_mnt_count;		/* Mount count */
    __le16	s_max_mnt_count;	/* Maximal mount count */
    __le16	s_magic;		/* Magic signature */
    __le16	s_state;		/* File system state */
    __le16	s_errors;		/* Behaviour when detecting errors */
    __le16	s_minor_rev_level; 	/* minor revision level */
    __le32	s_lastcheck;		/* time of last check */
    __le32	s_checkinterval;	/* max. time between checks */
    __le32	s_creator_os;		/* OS */
    __le32	s_rev_level;		/* Revision level */
    __le16	s_def_resuid;		/* Default uid for reserved blocks */
    __le16	s_def_resgid;		/* Default gid for reserved blocks */
    __le32	s_first_ino; 		/* First non-reserved inode */
    __le16  s_inode_size; 		/* size of inode structure */
    __le16	s_block_group_nr; 	/* block group # of this superblock */
};

struct ext2_group_desc
{
    __le32  bg_block_bitmap;                /* Blocks bitmap block */
    __le32  bg_inode_bitmap;                /* Inodes bitmap block */
    __le32  bg_inode_table;         /* Inodes table block */
    __le16  bg_free_blocks_count;   /* Free blocks count */
    __le16  bg_free_inodes_count;   /* Free inodes count */
    __le16  bg_used_dirs_count;     /* Directories count */
    __le16  bg_pad;
    __le32  bg_reserved[3];
};
struct ext2_inode {
    __le16  i_mode;         /* File mode */
    __le16  i_uid;          /* Low 16 bits of Owner Uid */
    __le32  i_size;         /* Size in bytes */
    __le32  i_atime;        /* Access time */
    __le32  i_ctime;        /* Creation time */
    __le32  i_mtime;        /* Modification time */
    __le32  i_dtime;        /* Deletion Time */
    __le16  i_gid;          /* Low 16 bits of Group Id */
    __le16  i_links_count;  /* Links count */
    __le32  i_blocks;       /* Blocks count */
    __le32  i_flags;        /* File flags */
    __le32  unused;
    __le32  i_block[15];/* Pointers to blocks */
};

struct ext2_dir_entry {
    __le32  inode;                  /* Inode number */
    __le16  rec_len;                /* Directory entry length */
    __le16  name_len;               /* Name length */
    char    name[];                 /* File name, up to EXT2_NAME_LEN */
};

struct ext2_dir_entry_2 {
    __le32  inode;                  /* Inode number */
    __le16  rec_len;                /* Directory entry length */
    __u8    name_len;               /* Name length */
    __u8    file_type;
    char    name[];                 /* File name, up to EXT2_NAME_LEN */
};

char *typename[]={
        "UNKNOWN",
        "REG_FILE",
        "DIR",
        "CHRDEV",
        "BLKDEV",
        "FIFO",
        "SOCK",
        "SYMLINK",
};

int readblock(int fd, __u8 b[K], int blockno, int offset);
_Bool findfile(int inode, char**components);
void print(int inode);
char **split(char* path);

int main(int argc, char *argv[])
{
    struct ext2_super_block *sb;      // superblock
    struct ext2_group_desc *gdt;      // group descriptor table


// open disk imsage
    fd = open(DISKFILE, O_RDONLY);

// read  superblock
    readblock(fd, block, 1,0);
    sb = (struct ext2_super_block *) block;

// remember inode size
    inosize  = sb->s_inode_size;
    //fprintf(stderr,"inode size: %d\n", inosize);

// read group descriptor table
    readblock(fd, block, 2,0);
    gdt = (struct ext2_group_desc *) block;

// remember inode size
    inotable = gdt->bg_inode_table;                                                        /* Inodes table block */
    //fprintf(stderr,"inode table block: %d\n", inotable);


    for (int i = 2; i < argc; ++i){

        char **path = split(argv[i]);
        if(!findfile(2, path)){
            fprintf(stderr, "%s not found\n\n", argv[i]);
	    puts("-------------------------------------------------------\n");	
	}
	}


    return 0;

}

int readblock(int fd, __u8 b[K], int blockno, int offset)
{
    int n;

    lseek(fd, (K * blockno) + offset*inosize, SEEK_SET);
    n = read(fd, b, K);
    if(n == K)
        return 0;
    fprintf(stderr,"only found %d bytes reading block %d\n", n, blockno);
    exit(1);
}

_Bool findfile(int inode, char**components)
{
    readblock(fd, block, inotable, inode-1);
    ino = (struct ext2_inode *) (block);
    char *name;
    readblock(fd, block, ino->i_block[0], 0);
    int bytenumber = 0;
    while(bytenumber < K){
        dentry = (struct ext2_dir_entry_2 *) (block + bytenumber);
        name = block + bytenumber + sizeof(dentry);
        *(name + dentry->name_len) = 0;
        bytenumber += dentry->rec_len;

        // printf("%-12s %-10s %10d %6d %8d\n",
        //        name,
        //        typename[dentry->file_type],
        //        dentry->inode,
        //        dentry->rec_len,
        //        dentry->name_len);

        if (!strcmp(dentry->name, *components) && *(components+1) &&  dentry->file_type == 2) {
            if (dentry->inode)
                return findfile(dentry->inode, components + 1);
        }
        else if (!strcmp(dentry->name, *components) && !*(components+1) && dentry->file_type == 1)
            if(dentry->inode) {
                print(dentry->inode);
                return 1;
            }
    }

    return 0;
}


void print(int inode)
{
    readblock(fd, block, inotable, inode-1);
    ino = (struct ext2_inode *) block;
    int filesize = ino->i_size;
    readblock(fd, block, ino->i_block[0], 0);

    for (int i = 0;  i < filesize ; ++i) {
        putchar(block[i]);
    }
    printf("\n");
    puts("-------------------------------------------------------\n");

}

