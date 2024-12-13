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
#define MAX_LINE_LENGTH (5 * 7 + 1)

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
char *fbp = 0;
int xmax = 0, ymax = 0;

void PutPixel(unsigned int x,unsigned int y,unsigned int c)
{
    if(x<xmax && y<ymax) {
    	*(fbp + y * xmax*2 + x *2) = 0x00FF&c;
    	*(fbp + y * xmax*2 + x *2 +1) = (0xFF00&c)>>8;
    }
}
/*****************************************************************************
// Function name    : lcd_disp_ascii16x8
// Description      : 在LCD的(x,y)坐标处以colour颜色显示s中的ASCII字符
// Return type      : void
// Argument         : int x : x坐标
// Argument         : int y : y坐标
// Argument         : char *s : 待显示字符串
// Argument         : int colour : 显示颜色
*****************************************************************************/
void lcd_disp_ascii16x8(int x,int y,char *s,int color)
{
    char buffer[16];        /* 16字节的字模缓冲区   */
    int i,j;
    unsigned long location;

    while(*s)
    {
        location=(*s)*16L;  /* 计算字模在文件中的位置 */
        memcpy(buffer, &arcii16x8[location], 16);   /* 获取ASCII字模 */
        for(i=0;i<16;i++)                       /* 每一行 */
        {
            for(j=0;j<8;j++)                /* 每个字节按位显示 */
            {
                if(((buffer[i]>>(7-j)) & 0x1) != 0)
                    PutPixel(x+j,y+i,color); /* 显示一位 */
            }
        }
        s+=1;   /* 下一个ASCII字符 */
        x+=8;   /* 字符间距 */
    }
}

//判断闰年 
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
	int startDay = dayOfWeek(year,month,1);
	int days = getDaysInMonth(year, month);
	int x, y, i;
	
	x = 100;
	y = 100;
	
	printf("%d year of calendar\n",year);
	lcd_disp_ascii16x8(x, y, months[month], BLUE_COLOR);
	y += 20;
	lcd_disp_ascii16x8(x, y, "Sun  Mon  Tue  Wed  Thu  Fri  Sat  ", BLUE_COLOR);
	y += 20;
	
	char *line = (char *)malloc(MAX_LINE_LENGTH); // 分配足够的空间
    if (!line) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    line[0] = '\0';
	
	for (i = 0; i < startDay; i++) {
        strcat(line, "     ");
    }
	int day;
    for (day = 1; day <= days; day++) {
    	char dayStr[5];
    	snprintf(dayStr, sizeof(dayStr), "%3d ", day);
    	strncat(line, dayStr, MAX_LINE_LENGTH - strlen(line) - 1);
        printf("%3d ", day);
        if ((startDay + day) % 7 == 0){
			lcd_disp_ascii16x8(x, y, line, BLUE_COLOR);
            printf("\n");
            line[0] = '\0';
            y += 20;
        }    
    }
}

int main(int argc, char **argv){
	int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    int k;
    
    //打开文件读写 
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
    //计算屏幕多少字节 
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
