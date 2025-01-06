#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#include "iconv.h"
#include "hzk16.h"
#include "ascii16x8.h"

#define RED_COLOR    0xF800
#define GREEN_COLOR  0x07E0
#define BLUE_COLOR   0x001F

char *fbp = 0;
int xmax = 0, ymax = 0;

void PutPixel(unsigned int x,unsigned int y,unsigned int c)
{
    if(x<xmax && y<ymax) {
    	*(fbp + y * xmax*2 + x *2) = 0x00FF&c;
    	*(fbp + y * xmax*2 + x *2 +1) = (0xFF00&c)>>8;
    }
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

/*****************************************************************************
// Function name    : lcd_disp_hzk16
// Description      : 在LCD的(x,y)坐标处以colour颜色显示s中的汉字
// Return type      : void
// Argument         : int x : x坐标
// Argument         : int y : y坐标
// Argument         : char *s : 待显示字符串
// Argument         : int colour : 显示颜色
*****************************************************************************/
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
/*
void ShowPIC()
{
    int x = 0, y = 0;
    int k = 0;
    int color;
    
    for(y = 0; y < 480; y++)
        for(x = 0; x < 640; x++)
        {
            color = (Image[k+1]<<8)+ Image[k];
            PutPixel(x, y, color);
            k+=2;
        }
}
*/
int main(int argc, char **argv)
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    int k;

    char *text="华中科技大学电信学院";
    char inbuf[1024] = {};
    size_t inlen;
    char outbuf[1024] = {};
    size_t outlen = 1024;
    strcpy (inbuf, text);
    inlen = strlen(inbuf);

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

    memset(fbp,0xff,screensize);
    xmax = vinfo.xres;
    ymax = vinfo.yres;

    printf("UTF8Len=%d, GBLen=%d\n", inlen, outlen);
    k=code_convert("UTF-8", "GB2312//IGNORE", inbuf, &inlen, outbuf, &outlen);
    printf("UTF8Len=%d, GBLen=%d, k=%d\n", inlen, outlen, k);
    lcd_disp_hzk16(78,48+18+18,outbuf, RED_COLOR);
    lcd_disp_ascii16x8(78,66+18+18, "EIC School,HUST 2020", BLUE_COLOR);

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
