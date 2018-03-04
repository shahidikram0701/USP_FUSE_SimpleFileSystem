#include "fs.h"

/*
 * File operations
 */

static void *fs_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	#ifdef DEBUG
	printf("Initialising the file system!!\n");
	#endif
	return NULL;
}

static int fs_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
	#ifdef DEBUG
	printf("getattr - ");
	printf("%s\n", path);
	#endif

	int res = 0;
	char *path2;
	path2 = (char *)malloc(sizeof(path));
	strcpy(path2, path);
	memset(stbuf, 0, sizeof(struct stat));

	inode *ino;
	path_to_inode(path, &ino);

	/*
	 * For the given path first find its inode then load stuff from inode of that file into the stbuf
	 */
	printf("before isDir path - %s\n", path2);
	int directory_flag = isDir(path2);
	if (directory_flag == 1) {
		printf("getdir - Directory\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
	}
	else if (directory_flag == 0){
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = ino -> size;
	}
	else{
		res = -ENOENT;
	}
	printf("return valueof getdir = %d\n", res);
	return res;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	#ifdef DEBUG
	printf("ReadDir - %s\n", path);
	#endif
	(void) offset;
	(void) fi;
	(void) flags;

	inode *ino;
	path_to_inode(path, &ino);

	printf("ino returned for %s - %u\n", path, ino);

	if (ino == NULL){
		return -ENOENT;
	}
	else{
		dirent *temp;
		filler(buf, ".", NULL, 0, 0);
		filler(buf, "..", NULL, 0, 0);
		printf("filled some stuff\n");

		if(strcmp(path, "/") == 0){
			temp = root_directory;
		}
		else{
			temp = (dirent *)(ino -> data);
			printf("ino -> data = %u\n", temp);
		}
		while((temp -> filename != NULL)){
			printf("I'm here\n");
			filler(buf, temp -> filename, NULL, 0, 0);
			temp++;
			//temp = (dirent *)temp
		}
	}
	return 0;
}


static int fs_mkdir(const char *path, mode_t mode)
{
	#ifdef DEBUG
	printf("mkdir\n");
	printf("%s\n", path);
	printf("%d", mode);
	#endif

	inode *ino;
	allocate_inode(path, &ino, true);
	print_inode(ino);
	printf("inode address for %s - %u\n", path, ino);
	//printf("Here!!\n");
	char *token = strtok(path, "/");
	dirent *temp;
	temp = root_directory;
	while (token != NULL){
		printf("token = %s\n", token);
		printf("temp -> filename = %s\n", temp->filename);
		while((temp -> filename != NULL) && (strcmp((temp -> filename), token) != 0)){
			temp++;
		}
		if(temp -> filename == NULL){
			// Add the directory to temp
			//printf("temp = %u\n", temp);
			printf("HERE\n");
			temp = (dirent *)temp;
			//printf("temp = %u\n temp -> filename = %u\n", temp, temp -> filename);
			(temp -> filename) = (char *)malloc(15);
			strcpy((temp -> filename), token);
			//printf("temp = %d\n", temp);
			//printf("%s\n", temp->filename);
			temp -> file_inode = ino;
			return 0;
		}
		else{
			if((temp -> file_inode) -> directory){
				temp = (dirent *)((temp -> file_inode) -> data);
			}
		}
		token = strtok(NULL, "/");
	}
	return -1;
}

static int fs_rmdir(const char *path)
{
	/*
	 * Here we remove a directory only if the directory is empty
	 */
	#ifdef DEBUG
	printf("\trmdir\n");
	printf("path : %s\n", path);
	#endif

	// Get the last directory
	int i;
	i = strlen(path);
	while(path[i] != '/'){
		i--;
	}
	char *subpath;
	subpath = (char *)malloc(strlen(path)+ 1);
	strncpy(subpath, path, i);
	subpath[i] = '/';
	subpath[i+1] = '\0';
	#ifdef DEBUG
	printf("Subpath = %s\n", subpath);
	printf("Name of directory to delete = %s\n", path + (i+1));
	#endif

	inode *ino;
	dirent *temp;
	if(strcmp(subpath, "/") == 0){
		temp = root_directory;
	}
	else{
		path_to_inode(subpath, &ino);
		temp = ino->data;
	}
	//printf("ASDF\n");
	// Get the inode for Subpath and then remove stuff


	printf("ino -> data = %u\n", temp);
	printf("root_directory = %u\n", root_directory);
	while((temp -> filename != NULL || (temp -> file_inode) != NULL) && strcmp(path + (i+1), temp -> filename) != 0){
		temp ++;
	}
	if(temp -> filename != NULL){
		if(temp -> file_inode != NULL){
			if(((dirent *)((temp -> file_inode) -> data)) -> filename != NULL){
				#ifdef DEBUG
				printf("Directoy isnt empty!!\n");
				#endif
				return -1;
			}
			temp ->filename = NULL;
			freemap[((((temp -> file_inode) -> data) - datablks) / BLK_SIZE)] = 1;
			(temp -> file_inode) -> used = false;
		}
	}
	return 0;
}

static int fs_rename(const char *from, const char *to, unsigned int flags)
{
	return -1;
}

static int fs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	return -1;
}

static int fs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	#ifdef DEBUG
	printf("\tCreate called\n");
	#endif

	inode *ino = return_first_unused_inode(inodes);
	allocate_inode(path, &ino, false);

	char *token = strtok(path, "/");
	dirent *temp;
	temp = root_directory;
	//printf("temp = %u\n", temp);
	//printf("root_directory -> filename = %u\n", (root_directory) -> filename);
	//printf("temp -> filename = %u\n", temp->filename);

	char *file = (char *)malloc(15);
	while (token != NULL){
		//printf("token = %s\n", token);
		//printf("temp -> filename = %s\n", temp->filename);
		while((temp -> filename != NULL) && (strcmp(temp -> filename, token) != 0)){
			temp++;
		}
		if(temp -> filename != NULL){
				if((temp -> file_inode) -> directory){
					temp = (dirent *) (temp -> file_inode) -> data;
				}
				else{
					return -1;
				}
		}
		strcpy(file, token);
		token = strtok(NULL, "/");
	}
	temp -> filename = file;
	temp -> file_inode = ino;
	return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
	#ifdef DEBUG
	printf("Opening File - %s\n", path);
	#endif
	inode *ino;
	path_to_inode(path, &ino);
	printf("inode returned = %u\n", ino);
	if(ino == NULL){
		return -1;
	}
	#ifdef DEBUG
	printf("Successful open\n");
	#endif
	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	inode *ino;
	path_to_inode(path, &ino);
	size_t len;
	(void) fi;
	if(ino == NULL)
		return -ENOENT;

	len = ino->size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, (ino -> data) + offset, size);
	} else
		size = 0;

	return size;
}

static int fs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	#ifdef DEBUG
	printf("Write called!!\n");
	printf("Offset = %d\n", offset);
	#endif

	inode* ino;
	path_to_inode(path, &ino);

	memcpy((ino -> data + offset), (buf), size);
	ino -> size = ((ino -> size) + size);
	return 0;
}

static int fs_rm(const char *path){
	#ifdef DEBUG
	printf("rm called\n");
	#endif
	// Get the last directory
	int i;
	i = strlen(path);
	while(path[i] != '/'){
		i--;
	}
	char *subpath;
	subpath = (char *)malloc(strlen(path)+ 1);
	strncpy(subpath, path, i);
	subpath[i] = '/';
	subpath[i+1] = '\0';
	#ifdef DEBUG
	printf("Subpath = %s\n", subpath);
	printf("Name of directory to delete = %s\n", path + (i+1));
	#endif

	inode *ino;
	dirent *temp;
	if(strcmp(subpath, "/") == 0){
		temp = root_directory;
	}
	else{
		path_to_inode(subpath, &ino);
		temp = ino->data;
	}
	//printf("ASDF\n");
	// Get the inode for Subpath and then remove stuff


	printf("ino -> data = %u\n", temp);
	printf("root_directory = %u\n", root_directory);
	while((temp -> filename != NULL || (temp -> file_inode) != NULL) && strcmp(path + (i+1), temp -> filename) != 0){
		temp ++;
	}
	if(temp -> filename != NULL){
		if(temp -> file_inode != NULL){
			temp ->filename = NULL;
			//freemap[((((temp -> file_inode) -> data) - datablks) / BLK_SIZE)] = 1;
			(temp -> file_inode) -> used = false;
		}
	}
	return 0;
}
