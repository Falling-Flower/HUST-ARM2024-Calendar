#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "linuxbmp.h"
#include <time.h>

#define RED_COLOR    0xF800
#define GREEN_COLOR  0x07E0
#define BLUE_COLOR   0x001F
#define WHITE_COLOR  0xFFFF
#define BLACK_COLOR  0x0000

char *fbp = 0;
struct fb_var_screeninfo vinfo;

// ... (其他代码保持不变)
void Glib_FilledRectangle(int x1,int y1,int x2,int y2,int color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

void PutPixel(unsigned int x, unsigned int y, unsigned int c) {
    if (x < vinfo.xres && y < vinfo.yres) {
        *(fbp + y * vinfo.xres_virtual * 2 + x * 2) = 0x00FF & c;
        *(fbp + y * vinfo.xres_virtual * 2 + x * 2 + 1) = (0xFF00 & c) >> 8;
    }
}

// 计算给定日期是星期几，返回值0-6表示星期日到星期六
int get_day_of_week(int year, int month, int day) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    year -= month < 3;
    return (year + year/4 - year/100 + year/400 + t[month-1] + day) % 7;
}

// 绘制单个字符到framebuffer
void draw_char(int x, int y, char ch, unsigned int color) {
    // 实现简单的ASCII字符绘制，这里省略具体实现
}

// 绘制字符串到framebuffer
void draw_string(int x, int y, const char *str, unsigned int color) {
    while (*str) {
        draw_char(x, y, *str++, color);
        x += 8; // 假设每个字符宽度为8像素
    }
}

// 打印日历
void print_calendar(int year, int month, int highlight_day) {
    // 清除之前的日历绘制
    Glib_FilledRectangle(0, 0, vinfo.xres, vinfo.yres, BLACK_COLOR);

    // 打印月份标题
    char title[20];
    snprintf(title, sizeof(title), "%d-%02d", year, month);
    draw_string(10, 10, title, WHITE_COLOR);

    // 打印星期标题
    char weekdays[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	int i;
    for (i = 0; i < 7; ++i) {
        draw_string(10 + i*40, 30, weekdays[i], WHITE_COLOR);
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
            draw_string(x_offset, y_offset, " ", RED_COLOR); // 高亮显示选定的日期
            draw_string(x_offset, y_offset, " ", RED_COLOR); // 高亮背景色
        }
        char day_str[3];
        snprintf(day_str, sizeof(day_str), "%2d", day);
        draw_string(x_offset, y_offset, day_str, WHITE_COLOR);
        x_offset += 40;
    }
}

int main() {
    int fbfd = 0;
    long int screensize = 0;

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
        printf("Error: cannot open framebuffer device.\n");
        exit(1);
    }

    // Get fixed screen information
    struct fb_fix_screeninfo finfo;
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
