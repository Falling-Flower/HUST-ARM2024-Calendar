#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#include "iconv.h"
#include "ascii16x8.h"

#define RED_COLOR    0xF800
#define GREEN_COLOR  0x07E0
#define BLUE_COLOR   0x001F

char *months[]=
{
	" ",
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

/*****************************************************************************
// Function name    : lcd_disp_ascii16x8
// Description      : ��LCD��(x,y)���괦��colour��ɫ��ʾs�е�ASCII�ַ�
// Return type      : void
// Argument         : int x : x����
// Argument         : int y : y����
// Argument         : char *s : ����ʾ�ַ���
// Argument         : int colour : ��ʾ��ɫ
*****************************************************************************/
void lcd_disp_ascii16x8(int x,int y,char *s,int color)
{
    char buffer[16];        /* 16�ֽڵ���ģ������   */
    int i,j;
    unsigned long location;

    while(*s)
    {
        location=(*s)*16L;  /* ������ģ���ļ��е�λ�� */
        memcpy(buffer, &arcii16x8[location], 16);   /* ��ȡASCII��ģ */
        for(i=0;i<16;i++)                       /* ÿһ�� */
        {
            for(j=0;j<8;j++)                /* ÿ���ֽڰ�λ��ʾ */
            {
                if(((buffer[i]>>(7-j)) & 0x1) != 0)
                    PutPixel(x+j,y+i,color); /* ��ʾһλ */
            }
        }
        s+=1;   /* ��һ��ASCII�ַ� */
        x+=8;   /* �ַ���� */
    }
}

//�ж����� 
int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDaysInMonth(int year, int month) {
    int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month];
}

// Blum's algorithm to find the day of the week for any date since March 1, 1 AD.
int dayOfWeek(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int k = year % 100;
    int j = year / 100;
    int f = day + 26 * (month + 1) / 10 + k + k / 4 + j / 4 + 5 * j;
    return (f + 6) % 7; // 0: Saturday, 1: Sunday, ..., 6: Friday
}

void printCalendar(){
	int year = 2024;
	int month = 12;
	int x,y;
	
	x = 100;
	y = 100;
	
	printf("%d year of calendar",year);
	lcd_disp_ascii16x8(x, y, months[month], BLUE_COLOR);
	y += 20;
	lcd_disp_ascii16x8(x, y, "Sun  Mon  Tue  Wed  Thu  Fri  Sat", BLUE_COLOR);
}

int main(int argc, char **argv){
	int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    int k;
    
    //���ļ���д 
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
    //������Ļ�����ֽ� 
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    printf("%dx%d, %dbpp, screensize = %ld\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, screensize );

	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fbfd, 0);
    if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

    memset(fbp,0xff,screensize);
    xmax = vinfo.xres;
    ymax = vinfo.yres;
	
	printCalendar();
	//lcd_disp_ascii16x8(78,66+18+18, "EIC School,HUST 2020", BLUE_COLOR); 
	munmap(fbp, screensize);
    close(fbfd);
    return 0;
} 
