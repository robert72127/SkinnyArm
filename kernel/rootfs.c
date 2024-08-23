#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"


struct cpio_newc_header {
		     char    c_magic[6];
		     char    c_ino[8];
		     char    c_mode[8];
		     char    c_uid[8];
		     char    c_gid[8];
		     char    c_nlink[8];
		     char    c_mtime[8];
		     char    c_filesize[8];
		     char    c_devmajor[8];
		     char    c_devminor[8];
		     char    c_rdevmajor[8];
		     char    c_rdevminor[8];
		     char    c_namesize[8];
		     char    c_check[8];
};

// each fs object in cpio archive comrpises a header record 
// with basic numeric metadata followed by the full pathname
// of the entry and the file data

// the header record stores a series of integer values
// that generally follow the fields in struct stat
// the header is followed by the path name of the entry 
// ( length of the path name) is stored in the header and any file data
// the end of archive is indicated by a special record
// with the pathname TRAILER!!!


struct inode{
    uint8_t *data;    
    uint8_t inode_number;
    uint32_t data_size;
};

struct file{
    uint8_t *file_name;
    struct file *parent; // points to parent node location
    // points to next file at given level
    struct file *next_file;
    union // type_specific
    {
        // if directory points to first children
        struct file *children;
        // file softlink points to
        struct file *softlink;
        // pointer to for hard links
        struct inode *ino;
    };
    uint8_t file_kind; // 0 is directory, 1 is softlink, 2 is hardlink
};

#define FILE_COUNT 256
#define INODE_COUNT 256
struct file files[FILE_COUNT];
struct inode inodes[INODE_COUNT];
uint8_t free_files;
uint8_t free_inodes;


static uint32_t ramfs_start = 0x8000000;

/*TODO parse*/
void init_ramfs(){




}

int search_file(uint8_t *name, struct file *f)
{    
    struct file *current_file;
    for(int i = 0; i < FILE_COUNT; i++){
        current_file = &files[i];
        if( strequal(current_file->file_name, name)){
            f = current_file;
            return 1;
        }
    }
    return 0;
}

int read(uint8_t *filename, uint32_t offset){
    struct file *current_file;
    if (!search_file(filename, current_file) ){
        return -1;
    }

    uint32_t file_data_size =  current_file->ino->data_size;
    if(offset > file_data_size){
        current_file = NULL;
       return -1;
    }
    else {
        return 0;
    }
}

int ls(uint8_t *filepath){
    struct file *current_file;
    if (!search_file(filepath, current_file) ){
        return -1;
    }
    while(current_file != NULL){
        printf(current_file->file_name);
        current_file =  current_file->next_file;
    }
    return 0;
}