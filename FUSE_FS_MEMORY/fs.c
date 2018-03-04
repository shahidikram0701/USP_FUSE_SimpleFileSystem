#include "fs_functions.c"

int main(int argc, char *argv[])
{
	fs = calloc(1, FS_SIZE);

	inodes = (inode *)fs;
	freemap = (int *)(inodes + N_INODES*sizeof(inode));
	datablks = (char *)(freemap + FREEMAP_BLKS*BLK_SIZE);

	printf("fs = %u\n", fs);
  //printf("inode_map = %u\n", inode_map);
  printf("inodes = %u\n", inodes);
  printf("freemap = %u\n", freemap);
  printf("datablks = %u\n", datablks);

	printf("Welcome!!\n\n");

	initialise_inodes(inodes);
	//printf("freemap = %u\t%d\nfreemap + 1 = %u\t%d\n", freemap, *(freemap), freemap + 1, *(freemap + 1));
	initialise_freemap(freemap);
	//printf("freemap = %u\t%d\nfreemap + 1 = %u\t%d\n", freemap, *(freemap), freemap + 1, *(freemap + 1));

	// Initialising the root directory
	root_directory = (dirent *)datablks;

	printf("root_directory = %u\n", root_directory);
	printf("size of dirent = %d\n", sizeof(dirent));
	// Adding a welcome file to the root_directory
	//printf("%u\n%u\n", root_directory, root_directory -> filename);
	char *q = (char *)malloc(10);
	printf("root_directory -> filename in memory = %u\n", q);
	(root_directory -> filename) = q;
	strcpy(root_directory -> filename, "Welcome");
	//printf("%u\n", root_directory -> filename);
	//printf("Here\n");
	root_directory -> file_inode = return_first_unused_inode(inodes);
	(root_directory -> file_inode) -> id = 1;
	(root_directory -> file_inode) -> size = 30;
	(root_directory -> file_inode) -> data = ( datablks + (return_offset_of_first_free_datablock(freemap)*BLK_SIZE));
	(root_directory -> file_inode) -> directory = false;
	(root_directory -> file_inode) -> last_accessed = 0;
	(root_directory -> file_inode) -> last_modified = 0;
	(root_directory -> file_inode) -> link_count = 1;

	printf("inode of Welcome = %u\n", root_directory -> file_inode);
	strcpy(((root_directory -> file_inode) -> data), "Welcome To Our File System!!!\n");

	//printf("%d\n", return_offset_of_first_free_datablock(freemap));
	/*
	char *x = (char *)malloc(10);
	strcpy(x, "/Welcome");
	printf("inodes = %u\n", inodes);
	inode *t =  path_to_inode(x);
	printf("path_to_inode = \n%u\n", t);
	printf("Here\n");
	*/
	//printf("%d\n%d\n", inodes->used, (inodes + 1) -> used);
	return fuse_main(argc, argv, &fs_oper, NULL);
}

int initialise_inodes(inode* i){
	// Initalise the inodes
	// return 0 on success and -1 on some error

	#ifdef DEBUG
	printf("Initialising inodes\n");
	#endif

	for(int x = 0; x < N_INODES; x++){
		i -> used = false;
		i ++;
	}
	return 0;
}

int initialise_freemap(int* map){
	// Initalise the freemap
	// return 0 on success and -1 on some error

	#ifdef DEBUG
	printf("Initialising freemap\n");
	#endif
	int x;
	*(map) = 0;
	for(x = 1; x < DBLKS; x++){
		*(map + x) = 1;
	}
	*(map + x) = -1; // No more datablocks
	return 0;
}


inode* return_first_unused_inode(inode *i){
	// just search 100 inodes and return 1st unused inode's address
	int ix;
	for(ix = 0; ix < N_INODES; ix++,i++){
		if(i->used == false){
			i -> used = true;
			return i;
		}
	}
	return NULL;
}

int return_offset_of_first_free_datablock(int *freemap){
	// returns first unused datablock(offset from freemap)
	//printf("HERE\n");
	int i;
	for(i = 0; i < DBLKS; i++){
		printf("%d\n", freemap[i]);
		if(freemap[i] == 1){
			freemap[i] = 0; // no more free
			return (i);
		}
	}
	return -1;
}

void path_to_inode(const char* path, inode **ino){
	// Given the path name it will return the inode address if it exists else it returns NULL
	#ifdef DEBUG
	printf("path_to_inode - path - %s\n", path);
	#endif

	if(strcmp("/", path) == 0){
		*ino = (root_directory->file_inode);
	}
	else{
		char *token = strtok(path, "/");
		dirent *temp;
		temp = root_directory;
		//printf("temp = %u\n", temp);
		//printf("root_directory -> filename = %u\n", (root_directory) -> filename);
		//printf("temp -> filename = %u\n", temp->filename);
    while (token != NULL){
			printf("token = %s\n", token);
			printf("temp -> filename = %s\n", temp->filename);
			printf("temp = %u\n", temp);
			while((temp -> filename != NULL) && (strcmp(temp -> filename, token) != 0)){
				temp++;
				printf("temp = %u\n", temp);
				printf("temp -> filename = %s\n", temp->filename);
			}
			if((temp -> filename) == NULL){
				#ifdef DEBUG
				printf("Inode doesnt exist!! for the path %s\n", path);
				#endif
				*ino = NULL;
			}
			else{
				//printf("BITCH PLEASE!\n");
				*ino = (temp -> file_inode);
				if((temp -> file_inode) -> directory){
					temp = (dirent *)((temp -> file_inode) -> data);
				}
			}
			token = strtok(NULL, "/");
		}
	}
}

int isDir(char *path){
	#ifdef DEBUG
	printf("isDir - %s", path);
	#endif
	if (strcmp("/", path) == 0){
		return 1;
	}
	else{
		char *token = strtok(path, "/");
		dirent *temp;
		temp = root_directory;
		//printf("temp = %u\n", temp);
		//printf("root_directory -> filename = %u\n", (root_directory) -> filename);
		//printf("temp -> filename = %u\n", temp->filename);
		int dir = -1;
    while (token != NULL){
			printf("token = %s\n", token);
			printf("temp -> filename = %s\n", temp->filename);
    	while((temp -> filename != NULL || (temp -> file_inode) != NULL)){
				if(temp -> filename != NULL){
					if((strcmp(temp -> filename, token) == 0)){
						break;
					}
				}
				temp ++;

			}
			if(temp->filename == NULL && temp -> file_inode == NULL){
				#ifdef DEBUG
				printf("Invalid Path - %s\n", path);
				#endif
				return -1;
				// Invalid path
			}
			else{
				if((temp -> file_inode) -> directory){
					dir = 1;
				}
				else{
					dir = 0;
				}
				if(dir == 1){
					temp = (dirent *)((temp -> file_inode) -> data);
				}
				// not handled : case where file/directory is there

			}
      token = strtok(NULL, "/");
  	}
		#ifdef DEBUG
		if(dir == 0){
			printf("%s - FILE\n", path);
		}
		else if(dir == 1){
			printf("%s - DIRECTORY\n", path);
		}
		else{
			printf("%s - ERROR PATH\n", path);
		}
		#endif
  return dir;
	}
}

void allocate_inode(char *path, inode **ino, bool dir){
	*ino = return_first_unused_inode(inodes);
	#ifdef DEBUG
	printf("ALLOCATING INODE\n");
	printf("inode address to allocate : %u\n", *ino);
	#endif
	(*ino) -> id = rand() % 5000;
	(*ino) -> size = 0;
	(*ino) -> data = ( datablks + (return_offset_of_first_free_datablock(freemap)*BLK_SIZE));
	(*ino) -> directory = dir;
	(*ino) -> last_accessed = 0;
	(*ino) -> last_modified = 0;
	(*ino) -> link_count = 2;
}

void print_inode(inode *i){
	printf("used : %d\n", i -> used);
	printf("id : %d\n", i -> id);
	printf("size : %d\n", i -> size);
	printf("data : %u\n", i -> data);
	printf("directory : %d\n", i -> directory);
	printf("last_accessed : %d\n", i -> last_accessed);
	printf("last_modified : %d\n", i -> last_modified);
	printf("link_count : %d\n", i -> link_count);
}

/*
void current_dir(char *path, dirent **d){
	if(strcmp(path, "/") == 0){
		*d = root_directory;
	}
	else{
		char *token = strtok(path, "/");
		dirent *temp;
		temp = root_directory;
		while (token != NULL){
			while((temp -> filename != NULL) && (strcmp(temp -> filename, token) != 0)){
				temp++;
			}
			if(temp -> filename == NULL){}
	}
}
*/
