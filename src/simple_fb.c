// SIMPLE BUFFER PROGRAM TO FILL THE SCREEN WITH A COLOR

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>


int main(){

	// fb_fd -> frame buffer file descriptor wiht read & write permissions
	int fb_fd = open("/dev/fb0", O_RDWR);
	
	if(fb_fd == -1 ) {
		perror("Error opening the file buffer! :( ");
		exit(1);
	}

	// we get the screen infomation using icotl system call
	struct fb_var_screeninfo vinfo;

	if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
		perror("Error reading variable information");
		exit(2);
	}

	// calculate screen size in bytes
	int screensize = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8;
	
	char *fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if((int) fbp == -1 ) {
		perror("Error mapping frame buffer device to memory");
		exit(3);
	}

	for(int i = 0; i < screensize ; i+=2 ){
		fbp[i] = 0xFF;
		fbp[i+1] = 0x00;
		fbp[i+2] = 0x00;
	}

	munmap(fbp, screensize);
	close(fb_fd);

	return 0;

}

