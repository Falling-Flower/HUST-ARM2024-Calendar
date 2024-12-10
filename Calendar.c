#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "linuxbmp.h"
#include <time.h>
#include "iconv.h"
#include "hzk16.h"
#include "ascii16x8.h"

#define RED_COLOR    0xF800
#define GREEN_COLOR  0x07E0
#define BLUE_COLOR   0x001F
#define WHITE_COLOR  0xFFFF
#define BLACK_COLOR  0x0000

char *fbp = 0;
struct fb_var_screeninfo vinfo;

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

void Glib_FilledRectangle(int x1,int y1,int x2,int y2,int color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

int code_convert(char *encFrom, char *encTo, char *inbuf, size_t *inlen, char *outbuf, size_t *outlen)
{
        /* @param encTo 目标编码方式
         * @param encFrom 源编码方式
         * 目的编码, TRANSLIT：遇到无法转换的字符就找相近字符替换
         *           IGNORE ：遇到无法转换字符跳过
         */
        iconv_t cd;
        size_t ret;
        char *tmpin = inbuf;
        char *tmpout = outbuf;

        cd = iconv_open(encTo, encFrom);
        if (cd==(iconv_t)-1)
        {
                perror("iconv_open");
                return -2;
        }
        /* 需要转换的字符串 */
        printf("inbuf=%s\n", inbuf);

        /* 打印需要转换的字符串的长度 */
        printf("inlen=%d\n", *inlen);

        ret = iconv(cd, &tmpin, inlen, &tmpout, outlen);
        if ( ret == -1)
        {
                printf ("iconv Error!\n");
                return -1;
        }
        /* 存放转换后的字符串 */
        printf("outbuf=%s\n", outbuf);

        //存放转换后outbuf剩余的空间
        printf("outlen=%d\n", *outlen);

        /* 关闭句柄 */
        iconv_close(cd);
        return 0;
}

void lcd_disp_hzk16(int x,int y,char *s,int color)
{
    char buffer[32];                            /* 32字节的字模缓冲区       */
    int i,j,k;
    unsigned char qh,wh;
    unsigned long location;

    while(*s)
    {
        printf("0x%x 0x%x\n",*s, *(s+1));
        qh=*s-0xa0;                             /* 计算区码                 */
        wh=*(s+1)-0xa0;                         /* 计算位码                 */
        location=(94*(qh-1)+(wh-1))*32L;        /* 计算字模在文件中的位置  */
        memcpy(buffer, &hzk16[location], 32);   /* 获取汉字字模               */
        for(i=0;i<16;i++)                       /* 每一行                  */
        {
            for(j=0;j<2;j++)                    /* 一行两个字节               */
            {
                for(k=0;k<8;k++)                /* 每个字节按位显示         */
                {
                    if(((buffer[i*2+j]>>(7-k)) & 0x1) != 0)
                        PutPixel(x+8*(j)+k,y+i,color); /* 显示一位 */
                }
            }
        }
        s+=2;                                               /* 下一个汉字    */
        x+=16;                                              /* 汉字间距     */
    }
}

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

// 计算给定日期是星期几，返回值0-6表示星期日到星期六
int get_day_of_week(int year, int month, int day) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    year -= month < 3;
    return (year + year/4 - year/100 + year/400 + t[month-1] + day) % 7;
}

// // 绘制单个字符到framebuffer
// void draw_char(int x, int y, char ch, unsigned int color) {
//     // 实现简单的ASCII字符绘制，这里省略具体实现
// 	fb_write_char(fb_fd, x, y, ch, color);
// }

// // 绘制字符串到framebuffer
// void draw_string(int x, int y, const char *str, unsigned int color) {
//     while (*str) {
//         lcd_disp_ascii16x8(x, y, *str++, color);
//         x += 8; // 假设每个字符宽度为8像素
//     }
// }

// 打印日历
void print_calendar(int year, int month, int highlight_day) {
    // 清除之前的日历绘制
    Glib_FilledRectangle(0, 0, vinfo.xres, vinfo.yres, BLACK_COLOR);

    // 打印月份标题
    char title[20];
    snprintf(title, sizeof(title), "%d-%02d", year, month);
    lcd_disp_ascii16x8(10, 10, title, WHITE_COLOR);

    // 打印星期标题
    char weekdays[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	int i;
    for (i = 0; i < 7; ++i) {
        lcd_disp_ascii16x8(10 + i*40, 30, weekdays[i], WHITE_COLOR);
    }

    // 获取该月的第一天是星期几
    int start_day = get_day_of_week(year, month, 1);

    // 获取该月有多少天
    int days_in_month;
    if (month == 2) {
        // 判断是否为闰年
        days_in_month = (year%4==0 && year%100!=0) || year%400==0 ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        days_in_month = 30;
    } else {
        days_in_month = 31;
    }

    // 打印日期
    int x_offset = start_day * 40 + 10;
    int y_offset = 50;
	int day;
    for (day = 1; day <= days_in_month; ++day) {
        if (x_offset >= vinfo.xres - 30) {
            x_offset = 10;
            y_offset += 20;
        }
        if (day == highlight_day) {
            lcd_disp_ascii16x8(x_offset, y_offset, " ", RED_COLOR); // 高亮显示选定的日期
            lcd_disp_ascii16x8(x_offset, y_offset, " ", RED_COLOR); // 高亮背景色
        }
        char day_str[3];
        snprintf(day_str, sizeof(day_str), "%2d", day);
        lcd_disp_ascii16x8(x_offset, y_offset, day_str, WHITE_COLOR);
        x_offset += 40;
    }
}

int main() {
    int fbfd = 0;
	struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
        printf("Error: cannot open framebuffer device.\n");
        exit(1);
    }

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

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(4);
    }

    // 获取当前时间
    time_t now;
    struct tm *current_time;
    time(&now);
    current_time = localtime(&now);

    // 设置默认查看当前月份
    int year = current_time->tm_year + 1900;
    int month = current_time->tm_mon + 1;
    int day = current_time->tm_mday;

    // 显示当前月份的日历，并高亮今天
    print_calendar(year, month, day);

    // 等待一段时间后退出
    sleep(10);

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
