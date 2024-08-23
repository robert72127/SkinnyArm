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
    uint16_t inode_number;
    uint16_t device_number;
    uint32_t data_size;
};

struct file{
    uint8_t file_name[16];
    struct file *parent; // points to parent node location
    // points to next file at given level
    struct file *next_file;
    union // type_specific
    {
        // if directory points to first children
        struct file *children;
        // pointer to for hard links
        struct inode *ino;
    };
    uint8_t file_kind; // 0 is directory, 1 is regular directory
};

uint8_t MAGIC[] = "070701";

#define FILE_COUNT 256
#define INODE_COUNT 256
#define MAX_FILENAME_SIZE 16
struct file files[FILE_COUNT];
struct inode inodes[INODE_COUNT];
uint8_t free_files;
uint8_t free_inodes;


static uint32_t ramfs_start = 0x8000000;


int search_file(uint8_t *name, struct file *f);

// check if magic matches and moves byte stream by 6 bytes
int check_magic(uint8_t *byte_stream){
    for(int i = 0; i < 6; i++){
        if(*byte_stream != MAGIC[i]){
            return -1;
        }
        byte_stream++;
    }
    return 0;
}

uint8_t c_dev[8];
uint8_t c_ino[8];
uint8_t c_mode[6];
uint8_t c_namesize[8];
uint8_t filesize[8];
uint8_t parent_filename[16];
uint8_t filename[16];

void init_ramfs(){
    uint8_t findex=0;
    uint8_t iindex=0;
    struct file *current_file = &files[0];
    struct inode *current_ino = &inodes[0];         
    
    uint8_t *byte_stream = (uint8_t *)ramfs_start;
    while(1){
        // read magic, check if matches
        if(check_magic(byte_stream) == -1){
            return -1;
        } 
        // read c_dev will be saved to inode (inodes are equal if same inode and dev number)
        c_dev[0] = 0;
        c_dev[1] = 0;
        for(int i = 0; i < 6; i++){
            c_dev[i+2] = *byte_stream;
            byte_stream++;
        }
        // read ino save it into current ino
        c_ino[0] = 0;
        c_ino[1] = 0;
        for(int i = 0; i < 6; i++){
            c_ino[i+2] = *byte_stream;
            byte_stream++;
        }
     
        // read c_mode dir hard/soft link
        for(int i = 0; i < 8; i++){
            c_mode[i] = *byte_stream;
            byte_stream++;
        }

        // read c_uid don't care about user owner
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }
        // read c_gid don;t care about group 
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }
        // read c_nlink number of links to this file, skip
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }
        //  c_mtime // modification time skip
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }

        // c_filesize - save this in inode, also use this to know when 
        // new file will get started
        for(int i = 0; i < 8; i++){
            filesize[i] = *byte_stream;
            byte_stream++;
        }

        // c_devmajor skip
        // c_devminor skip 
        // c_rdevmajor skip
        // c_rdevminor skip 
        for(int i = 0; i < 32; i++){
            byte_stream++;
        }

        // c_namesize including trailing bytes save this
        for(int i = 0; i < 32; i++){
            c_namesize[i] = *byte_stream;
            byte_stream++;
        }

        // c_check ignore
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }

        // then there is file name followed by nullbytes to so filename+header // 4
        uint64_t file_size = *(uint64_t*)(&file_size);


        // skip / in saved name , and add appropriately as child to parent node
        uint64_t filename_size = *(uint64_t*)(&c_namesize);
        uint8_t current_name_index;
        for(int i = 0; i < filename_size; i++){
            if(*byte_stream == '/'){
                for(int j = 0; j < current_name_index; j++){
                    parent_filename[j] = filename[j];
                    filename[j] = '0';
                }
            }
            if(*byte_stream == '\n'){
                filename[current_name_index] = *byte_stream;
                break;
            }

            filename[current_name_index % MAX_FILENAME_SIZE] = *byte_stream;
            byte_stream++;
        }

        // then data which is padded to multiple of four bytes
        byte_stream += ((file_size + 3) / 4)  * 4;


        // save info about current file and continue parsing
        for(int i = 0; i < MAX_FILENAME_SIZE; i++){
            current_file->file_name[i] = filename[i];
        }
        
        current_file->next_file = NULL;
        // save parent
        if(search_file(&parent_filename, current_file->parent)){
            // find last sibiling file and set this file as next
            struct file *sibiling = current_file->parent->children;
            if(sibiling == NULL){
                current_file->parent->children = current_file;
            }
            else{
                while(sibiling->next_file != NULL){
                    sibiling = sibiling->next_file;
                }
                sibiling->next_file = current_file;
            }

        }

        // switch filetype
        uint64_t filetype = *(uint64_t*)(&c_mode);
        switch (filetype)
        {
        // directory
        case 0x40000:
            current_file->file_kind = 0;
            current_file->children = NULL;
            break;
        case 0x100000 :
            uint64_t device_nr = *(uint64_t*)(&c_dev);
            uint64_t inode_nr = *(uint64_t*)(&c_ino);
            // regular file with size 0 search for inode
            if(file_size == 0){
                for(int i = 0; i < INODE_COUNT; i++){
                    if (inodes[i].device_number == device_nr && inodes[i].inode_number == inode_nr){
                        current_file->ino = &inodes[i];
                        break;
                    }
                }
            }else{
                current_ino->device_number = device_nr;
                current_ino->inode_number = inode_nr;
                current_ino = &inodes[++iindex];         
            }
            current_file->file_kind = 1;
        default:
            return -1;
        }
        
        current_file = &files[++findex];
    }




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
    f = NULL;
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