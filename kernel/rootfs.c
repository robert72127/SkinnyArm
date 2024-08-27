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
    uint64_t inode_number;
    uint64_t device_number;
    uint32_t data_size;
};

const uint8_t DIR_IDENT[4] = {'4','1','E','D'};
const uint8_t FILE_IDENT[4] = {'8','1','A','4'}; 
enum FILETYPE {INVALID,DIRECTORY, REGULAR, };

struct file{
    uint8_t file_name[16];
    struct file *parent; // points to parent node location
    enum FILETYPE type;
    // points to next file at given level
    struct file *next_file;
    union // type_specific
    {
        // if directory points to first children
        struct file *children;
        // pointer to for hard links
        struct inode *ino;
    };
};

const uint8_t MAGIC[6] = {'0','7','0','7','0','1'};
const uint8_t END[10] = {'T','R','A','I','L','E','R', '!', '!', '!'};



#define FILE_COUNT 256
#define INODE_COUNT 256
#define MAX_FILENAME_SIZE 16
struct file files[FILE_COUNT];
struct inode inodes[INODE_COUNT];
uint8_t free_files;
uint8_t free_inodes;


#define RAMFS_START 0x8000000

static int64_t int_from_char(char c){
       switch (c){
            case '0':return 0;
            case '1':return 1;
            case '2':return 2;
            case '3':return 3;
            case '4':return 4;
            case '5':return 5;
            case '6':return 6;
            case '7':return 7;
            case '8':return 8;
            case '9':return 9;
            default:return -1;}  
}
static int64_t int_from_string(char *string, int size){
    char *byte_stream = string;
    uint64_t total = 0;
    if(size == 0){
        return total;
    } 
    total = int_from_char(*byte_stream);
    byte_stream++;
    if (total == -1){
        return -1;
    }

    uint64_t current;
    for(int i = 1; i < size; i++){
        current = int_from_char(*byte_stream);
        if(current == -1){ return current;}
        byte_stream++;
        total = total * 10 + current;
    }
    return total;
}

enum FILETYPE get_file_type(char * arr){
    int i = 0;
    for(; i < 4; i ++){
        if (arr[i] != FILE_IDENT[i]){
            break;
        }
    }
    if (i == 4){
        return REGULAR;
    }
    i = 0;
    for(; i < 4; i ++){
        if (arr[i] != DIR_IDENT[i]){
            break;
        }
    }
    if (i == 4){
        return DIRECTORY;
    }
    return INVALID;
}


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

int check_end_archive(uint8_t *bytestream){
    uint8_t *local_bt = bytestream;
    for(int i = 0; i < 10; i++){
        if(*local_bt != END[i]){
            return 0;
        }
        local_bt++;
    }
    return 1;
}

uint8_t c_dev[8];
uint8_t c_ino[8];
uint8_t c_mode[8];
uint8_t c_namesize[8];
uint8_t filesize[8];
uint8_t parent_filename[16];
uint8_t filename[16];

int init_ramfs(){
    uint8_t findex=0;
    uint8_t iindex=0;
    struct file *current_file = &files[0];
    struct inode *current_ino = &inodes[0];         
    
    uint8_t *byte_stream = (uint8_t *)(RAMFS_START);
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
        // c_ino[0] = 0;
        // c_ino[1] = 0;
        for(int i = 0; i < 8; i++){
            c_ino[i] = *byte_stream;
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
        for(int i = 0; i < 8; i++){
            c_namesize[i] = *byte_stream;
            byte_stream++;
        }

        // c_check ignore
        for(int i = 0; i < 8; i++){
            byte_stream++;
        }

        // then there is file name followed by nullbytes to so filename+header // 4
        int64_t file_size = int_from_string(&filesize, 8);


        // skip / in saved name , and add appropriately as child to parent node
        int64_t filename_size = int_from_string(&c_namesize, 8);
        //check once if name is "TRAILER!!!" is so it's end of archive
        // don't modify pointer
        if(check_end_archive(byte_stream)){
            return 0;
        }

        // switch filetype
        enum FILETYPE filetype = get_file_type(c_mode+4);
        switch (filetype)
        {
        // directory
        case DIRECTORY:
            current_file->type = DIRECTORY;
            current_file->children = NULL;
            break;
        case REGULAR:

            uint64_t device_nr = *(uint64_t *)(&c_dev);
            uint64_t inode_nr = *(uint64_t *)(&c_ino);
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
            current_file->type = REGULAR;
        default:
            current_file->type = INVALID;
            return -1;
        }
        
        uint8_t current_name_index = 0;
        for(int i = 0; i < filename_size; i++){
            if(*byte_stream == '/'){
                for(int j = 0; j < current_name_index; j++){
                    parent_filename[j] = filename[j];
                    filename[j] = '0';
                }
                current_name_index = 0;
            }
            if(*byte_stream == '\n'){
                filename[current_name_index] = *byte_stream;
                break;
            }

            filename[current_name_index % MAX_FILENAME_SIZE] = *byte_stream;
            byte_stream++;
            current_name_index++;
        }

        // then data which is padded to multiple of four bytes
        byte_stream += ((file_size + 3) / 4)  * 4;


        // save info about current file and continue parsing
        for(int i = 0; i < MAX_FILENAME_SIZE; i++){
            current_file->file_name[i] = filename[i];
        }
        
        current_file->next_file = NULL;
        // save parent
        if(search_file(parent_filename, current_file->parent)){
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


        //parse data and padding

        
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
        uart_puts(current_file->file_name);
        current_file =  current_file->next_file;
    }
    return 0;
}