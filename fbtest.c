#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "linuxbmp.h"

#define RED_COLOR    0xF800
#define GREEN_COLOR  0x07E0
#define BLUE_COLOR   0x001F

char *fbp = 0;
/*
 * framebuffer application code, the start code of Linux GUI application
 * compile :
 *          $/usr/local/arm/2.95.3/bin/arm-linux-gcc -o fbtest fbtest.c
 *          $cp fbtest /tftpboot/examples
 * run in target:
 *          #mount 192.168.1.180:/tftpboot/ /mnt/nfs
 *          #cd /mnt/nfs/examples
 *          #./fbtest
 */
 
void PutPixel(unsigned int x,unsigned int y,unsigned int c)
{
    if(x<640 && y<480) {
    	*(fbp + y * 640*2 + x *2) = 0x00FF&c;
    	*(fbp + y * 640*2 + x *2 +1) = (0xFF00&c)>>8;
    }
}
void Glib_Line(int x1,int y1,int x2,int y2,int color)
{
	int dx,dy,e;
	dx=x2-x1; 
	dy=y2-y1;
    
	if(dx>=0)
	{
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 1/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 2/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 8/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 7/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
	else //dx<0
	{
		dx=-dx;		//dx=abs(dx)
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 4/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 3/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 5/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 6/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
}

void Glib_Rectangle(int x1,int y1,int x2,int y2,int color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}

void Glib_FilledRectangle(int x1,int y1,int x2,int y2,int color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

 
int main(int argc, char **argv)
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    
    int x = 0, y = 0;
    int k;
    int xmax, ymax;
    //long int location = 0;

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
        printf("Error: cannot open framebuffer device.\n");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit(3);
    }

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    printf("%dx%d, %dbpp, screensize = %ld\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, screensize );

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fbfd, 0);
    if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

    x = 100; y = 100;       // Where we are going to put the pixel

    if(vinfo.bits_per_pixel == 16)  // 16bpp only
    {
       // 16bpp framebuffer test
        printf("16bpp framebuffer test\n");
        printf("two bytes in fbp is a pixel of LCD, just set the value of fbp to put color to LCD\n");
        printf("byte format:\n");
        printf("  bit:| 15 14 13 12 11 | 10 9 8 7 6 5 | 4 3 2 1 0 |\n");
        printf("      |     red        |   green      |    blue   |\n");

        // Red Screen
        printf("Red Screen\n");
        for(y = 0; y < vinfo.yres; y++)
        {
            for(x = 0; x < vinfo.xres; x++)
            {
                *(fbp + y * vinfo.xres*2 + x *2) = 0x00;
                *(fbp + y * vinfo.xres*2 + x *2 +1) = 0xF8;
            }
        }
        sleep(2);

        // Green Screen
        printf("Green Screen\n");
        for(y = 0; y < vinfo.yres; y++)
        {
            for(x = 0; x < vinfo.xres; x++)
            {
                *(fbp + y * vinfo.xres*2 + x*2) = 0xE0;
                *(fbp + y * vinfo.xres*2 + x*2 +1) = 0x07;
            }
        }
        sleep(2);

        // Blue Screen
        printf("Blue Screen\n");
        for(y = 0; y < vinfo.yres; y++)
        {
            for(x = 0; x < vinfo.xres; x++)
            {
                *(fbp + y * vinfo.xres*2 + x*2) = 0x1F;
                *(fbp + y * vinfo.xres*2 + x*2 +1) = 0x00;
            }
        }
        sleep(2);

        // White Screen
        printf("Black Screen\n");
        for(y = 0; y < vinfo.yres; y++)
        {
            for(x = 0; x < vinfo.xres; x++)
            {
                *(fbp + y * vinfo.xres*2 + x*2) = 0x00;
                *(fbp + y * vinfo.xres*2 + x*2 +1) = 0x00;
            }
        }
        sleep(2);
        
        printf("Red Rectangle\n");
        for(y = 100; y < 200; y++)
        {
            for(x = 50; x < 150; x++)
            {
                *(fbp + y * vinfo.xres*2 + x*2) = 0x00;
                *(fbp + y * vinfo.xres*2 + x*2 +1) = 0xF8;
            }
        }
        sleep(2);

        printf("Show Picture\n");
	xmax = vinfo.xres<640?vinfo.xres:640;
	ymax = vinfo.yres<480?vinfo.yres:480;
	k = 0;
	for(y = 0; y < ymax; y++)
	{
	    k = y * 640 *2;
	    for(x = 0; x < xmax; x++)
	    {
		*(fbp + y * vinfo.xres*2 + x*2) = Image[k];
                 *(fbp + y * vinfo.xres*2 + x*2 +1) = Image[k+1];
                 k+=2;
            }
	} 

    }else
    {
        printf("16bpp only!!!\n");
    }
     
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
