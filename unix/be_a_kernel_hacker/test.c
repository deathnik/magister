#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#define O_RDONLY         00
#define O_WRONLY         01
#define O_RDWR           02


int main(int argc, char *argv[]){
	int fd = open("/dev/heap", O_RDWR);
	if(fd == NULL){
		printf("Could not open /dev/heap");
		return 1;
	}

	FILE * fp;
	char * line = NULL;
    size_t len = 0;
    ssize_t read_result;
    fp = fopen("test_data.txt", "r");
	for(int i=0; i < 3; ++i){
		read_result = getline(&line, &len, fp);
		if (read_result == -1){
			printf("ERROR: something went wrong while reading from test_data.txt");
			return 0;
		}
		write(fd, line, len);
	}

	char a[10000];
    read(fd, a, 9999);
    printf("Read first 3 from heap: %s\n",a);


    while ((read_result = getline(&line, &len, fp)) != -1) {
    	write(fd, line, len);
	}

	read(fd, a, 9999);
    printf("Read all other from heap: %s\n",a);
	return 0;
}

