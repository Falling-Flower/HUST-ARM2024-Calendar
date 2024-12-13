
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"yl_show_font.h"

#define TRUE    1
#define FALSE   0

int days_in_month[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
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


int inputyear(void)
{
	int year;
	
	printf("Please enter a year (example: 2019) : ");
	scanf("%d", &year);
	return year;
}

int inputmonth(void)
{
	int month;
	printf("Please enter a month (example: 1) : ");
	scanf("%d", &month);
	return month;
}

int determinedaycode(int year)
{
	int daycode;
	
	int d1, d2, d3;
	d1 = (year - 1.)/ 4.;
	d2 = (year - 1.)/ 100.;
	d3 = (year - 1.)/ 400.;
	daycode = (year + d1 - d2 + d3) %7;
	return daycode;
}


int determineleapyear(int year)
{
	if(year% 4 == FALSE && year%100 != FALSE || year%400 == FALSE)
	{
		days_in_month[2] = 29;
		return TRUE;
	}
	else
	{
		days_in_month[2] = 28;
		return FALSE;
	}
}

void calendar(int year, int month, int daycode)
{
	 
	int  day,i, k;
	char row_rest[29] ;
	char string[10];
	int x, y;
	int cmp;


	lcd_clear_screen(BLACK);
	memset(row_rest, '\0',sizeof(row_rest));
	memset(string, '\0',sizeof(string));

	x = 100;
	y = 100;


	lcd_show_string(x, y, months[month]); //显示第一行
	y += 20;
	lcd_show_string(x, y, "Sun  Mon  Tue  Wed  Thu  Fri  Sat");//显示第二行

	y += 20;
	// Correct the position of the first day of month
	for( cmp=1; cmp<=month-1; cmp++)
	{
	   daycode = (daycode + days_in_month[cmp]) % 7;
	}
	// Correct the position for the first date
	for ( day = 1; day <= 1 + daycode * 5; day++ )
	{
	   strcat(row_rest, " ");
	}
	
	// Print all the dates for one month
	for ( day = 1; day <= days_in_month[month]; day++ )
	{
	   snprintf(string, 10,"%2d",day);
	   strcat(row_rest,string);

	   // Is day before Sat? Else start next line Sun.
	   if ( ( day + daycode ) % 7 > 0 )
	   {
	     strcat(row_rest, "   ");
	   }
	   else
	   {
		lcd_show_string(x, y, row_rest);
		memset(row_rest, '\0',sizeof(row_rest));
		y += 20;

	   }

	}
	lcd_show_string(x, y, row_rest);

    	
}

int main(int argc, char *argv[])
{
    
    
    int year, month, daycode, leapyear; //日历
    int ret;
    int daycode_init;

    year = inputyear();
    daycode = determinedaycode(year);

    daycode_init = daycode;

	while(1)
	{
	    month = inputmonth();
		if((month>=1)&&(month<=12))
		{
	
		    determineleapyear(year);
		    
		    fd_fb = open("/dev/fb0", O_RDWR);
		    if(fd_fb == -1)
		    {
			printf("can't open /dev/fb0!\n");
			return -1;
		    }
		    
		    ret = ioctl(fd_fb, FBIOGET_VSCREENINFO, &var);
		    if(ret == -1)
		    {
			printf("can't ioctl for /dev/fb0!\n");
			return -1;
		    }
		    
		    
		    ret = ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);
		    if(ret == -1)
		    {
			printf("can't ioctl for /dev/fb0!\n");
			return -1;
		    }
		    
		    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
		    line_width  = var.xres * var.bits_per_pixel / 8;
		    pixel_width = var.bits_per_pixel / 8;
		    
		    fbmem = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
		    if(fbmem == (char *)-1)
		    {
			printf("mmap for /dev/fb0 error!\n");
			return -1;
		    }
		    
		    fd_hzk16 = open("HZK16", O_RDONLY);
		    if(fd_hzk16 == -1)
		    {
			printf("can't open HZK16!\n");
			return -1;
		    }
		  
		    ret = fstat(fd_hzk16, &hzk16_stat);
		    if(ret == -1)
		    {
			printf("fstat for HZK16 is error!\n");
			return -1;
		    }
		    
		    hzk16mem = mmap(NULL, hzk16_stat.st_size, PROT_READ, MAP_SHARED, fd_hzk16, 0);
		    if(hzk16mem == (char *)-1)
		    {
			printf("mmap for HZK16 error!\n");
			return -1;
		    }
		    
		    lcd_clear_screen(BLACK);
		    
		    daycode = daycode_init;
		    calendar(year, month, daycode);
		    munmap(fbmem, screen_size);
		    munmap(hzk16mem, hzk16_stat.st_size);
		}
		else{break;}
    	}
    close(fd_fb);
    close(fd_hzk16);
    
    return 0;
}
