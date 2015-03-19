#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "fio.h"
#include "filesystem.h"
#include "romfs.h"
#include "osdebug.h"
#include "hash-djb2.h"

#define TYPE_DIR 0xFFFF
#define TYPE_FILE 0x0000

struct romfs_fds_t {
    const uint8_t * file;
    uint32_t cursor;
    uint32_t size;
};

static struct romfs_fds_t romfs_fds[MAX_FDS];
extern size_t fio_printf(int fd, const char *format, ...);
static uint32_t get_unaligned(const uint8_t * d) {
    return ((uint32_t) d[0]) | ((uint32_t) (d[1] << 8)) | ((uint32_t) (d[2] << 16)) | ((uint32_t) (d[3] << 24));
}

static ssize_t romfs_read(void * opaque, void * buf, size_t count) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    uint32_t size = f -> size;
    
    if ((f->cursor + count) > size)
        count = size - f->cursor;

    memcpy(buf, f->file + f->cursor, count);
    f->cursor += count;

    return count;
}

static off_t romfs_seek(void * opaque, off_t offset, int whence) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    uint32_t size = f->size; 
    uint32_t origin;
    
    switch (whence) {
    case SEEK_SET:
        origin = 0;
        break;
    case SEEK_CUR:
        origin = f->cursor;
        break;
    case SEEK_END:
        origin = size;
        break;
    default:
        return -1;
    }

    offset = origin + offset;

    if (offset < 0)
        return -1;
    if (offset > size)
        offset = size;

    f->cursor = offset;

    return offset;
}

const uint8_t * romfs_get_file_by_hash(const uint8_t * romfs, uint32_t h, uint32_t * len) {
    const uint8_t * meta;

    for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 12) {
        if (get_unaligned(meta) == h) {
            if (len) {
                *len = get_unaligned(meta + 4);
            }
            return meta + 12;
        }
    }

    return NULL;
}

static int romfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;
    const uint8_t * file;
    int r = -1;

    file = romfs_get_file_by_hash(romfs, h, NULL);

    if (file) {
        r = fio_open(romfs_read, NULL, romfs_seek, NULL, NULL);
        if (r > 0) {
            uint32_t size = get_unaligned(file - 8);
            const uint8_t *filestart = file;
            while(*filestart) ++filestart;
            ++filestart;
            size -= filestart - file;
            romfs_fds[r].file = filestart;
            romfs_fds[r].cursor = 0;
            romfs_fds[r].size = size;
            fio_set_opaque(r, romfs_fds + r);
        }
    }
    return r;
}

static int romfs_ls(void * opaque, const char * path) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;
    int r = -1;
	
    const uint8_t * meta;

    for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 16) {
        if (get_unaligned(meta+12) == h) {		//hash_path position
            char name[256];
            for(int i =0; i<256; i++){
                if(!(*(name+i) = *(meta+16+i))) break;
            }
            fio_printf(1, name);
            fio_printf(1, "\r\n");
            r = 1;
        }
    }
	return r;
}

static int romfs_check(void * opaque, const char * path) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;

	
    const uint8_t * meta;

    for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 16) {
        if (get_unaligned(meta+12) == h) {		//hash_path position
            return 1;
        }
    }
    return 0;
}

static char* romfs_find(void * opaque, const char * path, const char *finding, int type){

	uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;
	int find_count = 0;
	const uint8_t * meta;
	int len = strlen(finding);
	if(type == 0){  //find dir
		char *a[5];
		char name[20];
		for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 16) {
			if (get_unaligned(meta+12) == h && get_unaligned(meta+8) == TYPE_DIR) {		//hash_path position
				int found = 1;
				char name[256];
				for(int i =0; i<len; i++){
					if(*(finding+i) != *(meta+16+i)) {
						found = 0;
						break;
					}				
				}
				if(found == 1) {
					for(int i =0; i<256; i++){
						if(!(*(name+i) = *(meta+16+i))) break;
					}
					a[find_count++] = name;
				}
			}
		}
		
		if(find_count == 1) return a[0];
		else if(find_count >0) {
			for(int i = 0; i < find_count; i++){
				fio_printf(1, a[i]);
			}			
		}
		
	} else{
		char *a[5];
		char name[20];
		for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 12) {
			if (get_unaligned(meta+12) == h && get_unaligned(meta+8) == TYPE_FILE) {		//hash_path position
				int found = 1;
				char name[256];
				for(int i =0; i<len; i++){
					if(*(finding) != *(meta+16+i)) {
						found = 0;
						break;
					}				
				}
				if(found == 1) {
					for(int i =0; i<256; i++){
						if(!(*(name+i) = *(meta+16+i))) break;
					}
					a[find_count++] = name;
				}
			}
		}
		
		if(find_count == 1) return name;
		else {
			for(int i = 0; i < find_count; i++){
				fio_printf(1, a[i]);
			}			
		}
	}
    
    return "";
	
	
}

void register_romfs(const char * mountpoint, const uint8_t * romfs) {
//    DBGOUT("Registering romfs `%s' @ %p\r\n", mountpoint, romfs);
    register_fs(mountpoint, romfs_open, romfs_ls, romfs_check, romfs_find, (void *) romfs);
}
