#include "kvfs.h"
#include <fcntl.h>
#include<sys/stat.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define METADATASIZE 144

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

kvfs_t* kvfs;
int sbfd;

void f_write(char *str){
	
	FILE* f;
	f = fopen("/home/vagrant/kvfs.log", "a+");
	fprintf(f, "%s\n", str);
	fclose(f); 

}

static int read_superblock(){

	const char* superblock_file = "/.superblock";
    	char fname[strlen(mountparent) + strlen(superblock_file)];
     	strcpy(fname, mountparent);
    	strcat(fname,superblock_file);
    	int fd;
	int num_files = 0;
	ssize_t read_sz;
	fnode_t fnode; 
       	fd = open(fname, O_RDWR);
   	if (fd == -1) {
        	printf("kv_fs_init(): open(): %s\n", strerror(errno));
        	exit(1);
    	}
	int i = 0;
	f_write("before kvfs size\n");
	kvfs = realloc(kvfs, sizeof(kvfs_t));
   	kvfs->size = 0;

	f_write("after free\n");
	read(fd, (void*) &num_files, sizeof(int));
	char a[10];
	sprintf(a, "p%d\n", num_files);
	f_write(a);
	f_write("after read\n");
	for(i = 0; i < num_files; i++){
		
		kvfs =  realloc(kvfs, sizeof(kvfs_t) + sizeof(fnode_t) * (kvfs->size + 1) );	
		//increment the size field in kvfs
		kvfs->size = kvfs->size + 1;
		read_sz = read(fd, (void*) &fnode, sizeof(fnode));
		if(sizeof(fnode) != read_sz){
			printf("corrupt superblock\n");
			exit(1);
		}
		kvfs->data[i] = fnode;
	}
		 	
	f_write("right before close\n");
	close(fd);
   

   return 0;

}
static int write_superblock(){
	
	const char* superblock_file = "/.superblock";
    	char fname[strlen(mountparent) + strlen(superblock_file)];
     	strcpy(fname, mountparent);
    	strcat(fname,superblock_file);
    	int fd;
	int num_files = 0;
	ssize_t write_sz;
	fnode_t fnode; 
       	fd = open(fname, O_RDWR);
   	if (fd == -1) {
        	printf("kv_fs_init(): open(): %s\n", strerror(errno));
        	exit(1);
    	}
	write_sz = write(fd, (void*) &kvfs->size, sizeof(int));

	if(!write_sz == sizeof(int)){
		printf("corrupt superblock \n");
		exit(1);
	}
	
	write_sz = write(fd, (void*) kvfs->data, kvfs->size * sizeof(fnode_t));
	
	if(!write_sz == kvfs->size * sizeof(fnode_t)){

		printf("corrupt superblock\n");
		exit(1);
	}
	 	
	//initalize kvfs
	close(fd);
   
	return 0;
}

static void clear_superblock(){
	
	const char* superblock_file = "/.superblock";
    	char fname[strlen(mountparent) + strlen(superblock_file)];
     	strcpy(fname, mountparent);
    	strcat(fname,superblock_file);
    	int fd;
	fd = open(fname, O_RDONLY | O_WRONLY | O_TRUNC);
	
	close(fd);


}

static int lookup(const char *filename){

    int i = 0;
    ssize_t read_ret = 0;
    for(i = 0; i < kvfs->size; i++){
	f_write("this is what we found\n");
	f_write(kvfs->data[i].name);
    	if(strcmp(kvfs->data[i].name, filename) == 0){
		f_write(filename);
		f_write(kvfs->data[i].name);
		return i;
	}	
    }
    return -1;

}
 
static int kvfs_flush(int fd){

	clear_superblock();
	write_superblock();

	return 0;
}


static int kvfs_release(const char* pathname, struct fuse_file_info *info){
	return 0;
}

static int kvfs_open(const char *pathname, struct fuse_file_info *info){
	
    int index;

    index = lookup(pathname);
    
    if(index < 0)
	return  -EACCES;
    else{
	//Set Permissions
//	struct stat stbuf;
//	memcpy(&stbuf, kvfs->data[index].data, METADATASIZE);
//	memcpy(kvfs->data[index].data, &stbuf, METADATASIZE);

	return 0;
    }
	

	return -EACCES;
}

static void* kvfs_init(struct fuse_conn_info *conn) {

    const char* superblock_file = "/.superblock";
    char fname[strlen(mountparent) + strlen(superblock_file)];
    strcpy(fname, mountparent);
    strcat(fname,superblock_file);
    int fd;
    kvfs = calloc(1, sizeof(kvfs_t));
    kvfs->size = 0;
    if (access(fname, F_OK) == -1) {
        int size = 0;
        fd = open(fname, O_CREAT | O_WRONLY, 0644);
        write(fd, &size, sizeof(size));
        lseek(fd, SUPERBLOCK_SIZE - 1, SEEK_SET);
        write(fd, "\0", 1);
        close(fd);
    }
    else{
	int success = read_superblock();
	if(success < 0){
		printf("kv_fs_init(): read_superblock() %d\n", success);
		exit(1);	
	}
	//close(fd);
   }
    return NULL;
}

static int kvfs_access(const char *pathname, int mode){

	return 0;

}



static int kvfs_getattr(const char *path, struct stat *stbuf)
{  
    int index;
    if (!strcmp(path, "/")) {
        return lstat(mountparent, stbuf);
    }
    else{
	index = lookup(path);
	if(index >= 0){
		memcpy(stbuf, &kvfs->data[index].data, METADATASIZE);
		

		return 0;
	}
	else{
		if(index == -1){
			return -ENOENT;
		}
	}
   }
   return -3;
}

static int kvfs_truncate(const char *path, off_t size)
{
    return 0;
}

static int kvfs_write(const char* filename, const char* data, size_t size, off_t off, struct fuse_file_info* info){

	struct stat st;

	if(off + size > DATA_SIZE - METADATASIZE)
		return 0;
	
	int index = lookup(filename);

	if(index >= 0){

		//copy metadata into local stat struct
		memcpy(&st, kvfs->data[index].data, METADATASIZE);
		//write buffer data to file
		memcpy(kvfs->data[index].data + METADATASIZE + off, data, size);
		//update file size in local stat buffer
		st.st_size = off + size;
		//copy metadata back to file system
		memcpy(kvfs->data[index].data, &st, METADATASIZE);
		
		return size;
	}
	else{
		return 0;
	}

}

static int kvfs_read(const char* pathname, char* buf, size_t size, off_t off, struct fuse_file_info* info ){

	size_t max_copy_size = DATA_SIZE - METADATASIZE;
	size_t copy_size = 0;
	int index = lookup(pathname);
	struct stat st;


	if(index >= 0){
				
		copy_size = max_copy_size < off + size ?  max_copy_size - off : size;
		memcpy(&st, kvfs->data[index].data, METADATASIZE);
		if(copy_size > st.st_size)
			copy_size = st.st_size;
 	
		memcpy(buf, kvfs->data[index].data + METADATASIZE + off, copy_size);
		return copy_size;
	}
	else{
		return 0;
	}
}

static int kvfs_mknod(const char* pathname, mode_t mode, dev_t dev){
    
	struct stat st;
        ssize_t bytes_written = 0;
        int index;

        
    	//read data from file
	fnode_t fnode;

	st.st_size = 0;
	st.st_mode = mode;
	st.st_dev = dev;
	st.st_uid = getuid();
	st.st_gid = getgid();
	st.st_nlink = 1;
	st.st_rdev = 0;
	st.st_blksize = 1024;
	st.st_blocks = 4;
	st.st_atime = time(NULL);
	st.st_mtime = time(NULL);
	st.st_ctime = time(NULL);

	//copy the pathname into the name field of fnode
	strcpy(fnode.name, pathname);
	//copy the stat buf into the first 144 bytes of the data field of fnode 
	memcpy(fnode.data, &st, METADATASIZE);
	//set the magic integer in fnode
	fnode.magic = 1;

	//increase size of kvfs data field
	kvfs =  realloc(kvfs, sizeof(kvfs_t) + sizeof(fnode_t) * (kvfs->size + 1) );	
	//increment the size field in kvfs
	kvfs->size = kvfs->size + 1;


	//copy entire fnode to the end of the kvfs data structure
	memcpy(&kvfs->data[(kvfs->size - 1)], &fnode, sizeof(fnode_t));	


	struct stat st1;
	memcpy(&st1, kvfs->data[kvfs->size-1].data, 144); 


	
	return 0; 

   }	

static int kvfs_utimens(const char* pathname, const struct timespec tv[2]){
	int index = lookup(pathname);

	struct stat stbuf;
	if(index >= 0){
		memcpy(&stbuf, kvfs->data[index].data, METADATASIZE);
		stbuf.st_atime = tv[0].tv_sec;
		stbuf.st_mtime = tv[1].tv_sec;
		stbuf.st_ctime = tv[1].tv_sec;
		memcpy(kvfs->data[index].data, &stbuf, METADATASIZE);
		
	}
	return 0;

}   	

static int kvfs_unlink(const char* pathname){

	int index = lookup(pathname);
	int i = 0;
	if(index >= 0){

		for(i = index + 1; i < kvfs->size; i++){

			kvfs->data[i-1] = kvfs->data[i];
			
		}

		kvfs->size = kvfs->size - 1;

		kvfs = realloc(kvfs, sizeof(kvfs_t) + sizeof(fnode_t) * (kvfs->size));
		clear_superblock();
		write_superblock();
		return 0;

	}
	else{
		return -EACCES;
	}



}

static int kvfs_rename(const char* name, const char* newname){

	int index = lookup(name);
	if(index >= 0){

		memset((void*) kvfs->data[index].name, 0, NAME_SIZE);
		strcpy(kvfs->data[index].name, newname);
		clear_superblock();
		write_superblock();	
		return 0;

	}
	else{
		return -EACCES;
	}



}
struct fuse_operations kvfs_oper = {
    .getattr    = kvfs_getattr,
    .truncate   = kvfs_truncate,
    .init       = kvfs_init,
    .open 	= kvfs_open,
    .flush	= kvfs_flush,
    .release    = kvfs_release,
    .mknod 	= kvfs_mknod,
    .utimens    = kvfs_utimens,
    .write      = kvfs_write,
    .read       = kvfs_read,
    .unlink     = kvfs_unlink,
    .rename     = kvfs_rename,
};


