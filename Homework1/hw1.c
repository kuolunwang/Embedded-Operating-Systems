#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include<algorithm>
#include <vector>
#include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"


void seg_setting(int n, int fd);
long decimal_to_7seghex(int n);
int keypad(int fd);
void clear(int fd);

// park struct 
struct park
{
    bool reserve;
    bool parked;
    int plate_number;
};

int ipn(int fd);
char car_state(int num,struct park *p);
void cancel(int fd,struct park *p,int pla_num);
void reserved_mode(int fd,struct park *p,int pla_num);
void parked_mode(int fd,struct park *p,int pla_num);
void parking_mode(int fd,struct park *p,int pla_num);
void check_in(int fd,struct park *p,int pla_num);
void reser(int fd,struct park *p,int pla_num);
void show(int fd,struct park *p);
void pick_up(int fd,struct park *p,int pla_num);
char car_state(int num,struct park *p);
void led(int fd,struct park* p, char par_lot);


int main()
{
    // initial park 
    struct park p[24]={0};
    int fd;

    unsigned short key;
    lcd_write_info_t display;
    _7seg_info_t data_seg;

    /* Open device /dev/lcd */
    if((fd =open("/dev/lcd",O_RDWR))< 0) 
    {
        printf("Open /dev/lcd faild.\n");
        exit(-1);
    }

    /* initial */
    clear(fd);
    printf("start\n");
    char state;
    int pla_num;

    /* start */
    while(1)
    {
        /* input plate number */
        pla_num = ipn(fd);
        state = car_state(pla_num,p);
        if(state == 1) //reserved
        {
            reserved_mode(fd,p,pla_num);
        }
        else if (state == 2) // parked
        {
            parked_mode(fd,p,pla_num);
        }
        else if(state ==3) // none
        {
            parking_mode(fd,p,pla_num);
        }
        else // invaild plate number 
        {
            clear(fd);
        }
    }
    close(fd);
}

/* you haven't parked your car yet */
void parking_mode(int fd,struct park *p,int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "1.show\n2.reserve\n3.check-in\n4.exit");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;

    while(1)
    {
        tmp = keypad(fd);
        switch(tmp)
        {
            case 1:
                show(fd,p);
                break;
            case 2:
                reser(fd,p,pla_num);
                break;
            case 3:
                check_in(fd,p,pla_num);
                break;
            default:
                ioctl(fd, LCD_IOCTL_CLEAR, NULL);
                display.Count = sprintf((char *) display.Msg, "1.show\n2.reserve\n3.check-in\n4.exit");
                ioctl(fd, LCD_IOCTL_WRITE, &display);
                break;
        }
        if(tmp <= 4 && tmp >= 2)
        {
            clear(fd);
            break;
        }
    }
}

/* reserve the park grid */
void reser(int fd,struct park *p,int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "Select parking lot:");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;
    char par_lot = 0,par_gr = 0;

    while(1)
    {
        while(1)
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Select parking lot:");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            tmp = keypad(fd);
            if(tmp>=1 && tmp <=3)
            {
                par_lot = tmp;
                break;
            }  
        }
        led(fd,p,par_lot);
        while(1)
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Select parking grid:");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            tmp = keypad(fd);
            if(tmp>=1 && tmp <=8)
            {
                par_gr = tmp;
                break;
            }
        }

        if(p[(par_lot-1)*8+par_gr-1].plate_number == 0)
        {
            p[(par_lot-1)*8+par_gr-1].plate_number = pla_num;
            p[(par_lot-1)*8+par_gr-1].parked = false;
            p[(par_lot-1)*8+par_gr-1].reserve = true;
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Have a nice day!");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
        else
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Error! Please select an ideal grid.");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
        break;
    }

    while(1)
    {
        tmp = keypad(fd);
        break;
    }
}

/* you already reserved park grid */
void reserved_mode(int fd,struct park *p, int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "1.show\n2.cancel\n3.check-in\n4.exit");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;

    while(1)
    {
        tmp = keypad(fd);
        switch(tmp)
        {
            case 1:
                show(fd,p);
                break;
            case 2:
                cancel(fd,p,pla_num);
                break;
            case 3:
                check_in(fd,p,pla_num);
                break;
            default:
                ioctl(fd, LCD_IOCTL_CLEAR, NULL);
                display.Count = sprintf((char *) display.Msg, "1.show\n2.cancel\n3.check-in\n4.exit");
                ioctl(fd, LCD_IOCTL_WRITE, &display);
                break;
        }
        if(tmp <= 4 && tmp >= 2)
        {
            clear(fd);
            break;
        }
    }
}

/* you have parked your car  */
void parked_mode(int fd,struct park *p, int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "1.show\n2.pick-up");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;

    while(1)
    {
        tmp = keypad(fd);
        switch(tmp)
        {
            case 1:
                show(fd,p);
                break;
            case 2:
                pick_up(fd,p,pla_num);
                break;
            default:
                ioctl(fd, LCD_IOCTL_CLEAR, NULL);
                display.Count = sprintf((char *) display.Msg, "1.show\n2.pick-up");
                ioctl(fd, LCD_IOCTL_WRITE, &display);
                break;
        }
        if(tmp == 2)
        {
            clear(fd);
            break;
        }
    }
}

/* parking your car */
void check_in(int fd,struct park *p,int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "Select parking lot:");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;
    char par_lot = 0,par_gr = 0;
    bool already = false;

    while(1)
    {
        int i = 0;
        for(i;i<24;i++)
        {
            if(p[i].plate_number == pla_num && p[i].reserve)
            {
                p[i].parked = true;
                ioctl(fd, LCD_IOCTL_CLEAR, NULL);
                display.Count = sprintf((char *) display.Msg, "Have a nice day!");
                ioctl(fd, LCD_IOCTL_WRITE, &display);
                already = true;
                break;
            }
        }
        if(already)
            break;

        while(1)
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Select parking lot:");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            tmp = keypad(fd);
            if(tmp>=1 && tmp <=3)
            {
                par_lot = tmp;
                break;
            }  
        }
        led(fd,p,par_lot);
        while(1)
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Select parking grid:");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            tmp = keypad(fd);
            if(tmp>=1 && tmp <=8)
            {
                par_gr = tmp;
                break;
            }
        }

        if(p[(par_lot-1)*8+par_gr-1].plate_number == 0)
        {
            p[(par_lot-1)*8+par_gr-1].plate_number = pla_num;
            p[(par_lot-1)*8+par_gr-1].parked = true;
            p[(par_lot-1)*8+par_gr-1].reserve = false;
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Have a nice day!");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
        else
        {
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char *) display.Msg, "Error! Please select an ideal grid.");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
        break;
    }

    while(1)
    {
        tmp = keypad(fd);
        break;
    }
}

/* pick up your car */
void pick_up(int fd,struct park *p,int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);

    bool re =false;
    int tmp;
    int i = 0;
    for(i;i<24;i++)
    {
        if(p[i].plate_number == pla_num)
        {
            re = p[i].reserve;
            p[i].plate_number = 0;
            p[i].reserve = false;
            p[i].parked = false;
            break;
        }
    }

    char fee = 40;

    if(re)
        fee = 30;

    display.Count = sprintf((char *) display.Msg, "Parking fee: $%d",fee);
    ioctl(fd, LCD_IOCTL_WRITE, &display);



    while(1)
    {
        tmp = keypad(fd);

        if(tmp == -13)
            break;
    }
}

/* cancel reserved park grid */
void cancel(int fd,struct park *p,int pla_num)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.Count = sprintf((char *) display.Msg, "Reserve fee: $20");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    int tmp;
    int i = 0;
    for(i;i<24;i++)
    {
        if(p[i].plate_number == pla_num)
        {
            p[i].reserve = false;
            break;
        }
    }

    while(1)
    {
        tmp = keypad(fd);

        if(tmp == -13)
            break;
    }
}

/* show the rest park grid on three park lots */
void show(int fd,struct park *p)
{
    lcd_write_info_t display;
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);

    int i = 1;
    int tmp;
    char idle = 8;

    for(i;i<=24;i++)
    {
        if((p[i-1].parked) || (p[i-1].reserve))
            idle --;
        if(i % 8 == 0)
        {
            display.Count = sprintf((char *) display.Msg, "P%d %d\n",i/8,idle);
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            idle = 8;
        }
    }
    
    while(1)
    {
        tmp = keypad(fd);

        if(tmp == -13)
            break;
    }
}

/* login menu */
void clear(int fd)
{
    lcd_write_info_t display;
    _7seg_info_t data_seg;
    unsigned short data;

    ioctl(fd, _7SEG_IOCTL_ON, NULL);
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);

    /* Turn off all LED lamps */
    data = LED_ALL_OFF;
    ioctl(fd, LED_IOCTL_SET, &data);

    data_seg.Mode = _7SEG_MODE_PATTERN;
    data_seg.Which = _7SEG_D5_INDEX | _7SEG_D8_INDEX;
    data_seg.Value = 0x0000;
    ioctl(fd, _7SEG_IOCTL_SET, &data_seg);
    ioctl(fd, _7SEG_IOCTL_OFF, NULL);

    display.Count = sprintf((char *) display.Msg, "Input plate num:");
    ioctl(fd, LCD_IOCTL_WRITE, &display);
}

/* get car state */
char car_state(int num,struct park *p)
{
    int i=0;
    if(num >= 0)
    {
        for(i;i<24;i++)
        {
            if(p[i].plate_number == num)
            {
                if(p[i].parked)
                    return 2; // parked
                else
                    return 1; // reserved
            }
        }
        return 3; // none
    }
    else
        return 0;
}

/* output plate number */
int ipn(int fd)
{
    lcd_write_info_t display;
    int value = 0;
    int tmp;
    int index = 0;
    while(1)
    {
        tmp = keypad(fd);
        if(tmp >= 0)
        {
            index ++;
            value = value * 10 + tmp;
            display.Count = sprintf((char *) display.Msg, "%d", tmp);
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
        else if(tmp == -13)
            break;
    }

    if(index == 4)
    {
        seg_setting(value,fd);
        return value;
    }
    else
        return -1;
}

/* output keypad value */
int keypad(int fd)
{
    unsigned short key;
    int ret;

    /* initial */ 
    ioctl(fd, KEY_IOCTL_CLEAR, key);

    /* detect keypad */
    while(1)
    {
        ret = ioctl(fd, KEY_IOCTL_CHECK_EMTPY, &key);
        if(ret<0)
        {
            sleep(1);
            continue;
        }
        ret = ioctl(fd, KEY_IOCTL_GET_CHAR,&key);
        switch(key & 0xff)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '#':
                return (key & 0xff)-48;
                break;
            default:
                return -1;
                break;
        }
    }
}

/* setting 7_seg and show number n*/
void seg_setting(int n, int fd)
{
    _7seg_info_t data_seg;

    ioctl(fd, _7SEG_IOCTL_ON, NULL);
    data_seg.Mode = _7SEG_MODE_HEX_VALUE;
    data_seg.Which = _7SEG_ALL;
    data_seg.Value = 0x0000+decimal_to_7seghex(n);
    ioctl(fd, _7SEG_IOCTL_SET, &data_seg);
}

/* convert decimanl to 7seg value */
long decimal_to_7seghex(int n)
{
    char i = 0;
    unsigned long output = 0;

    while(n != 0)
    {
        output += n % 10 * pow(16,i);
        n /= 10;
        i ++; 
    }
    return output;
}

/* setting led with different park lot */
void led(int fd,struct park* p, char par_lot)
{
    int i=0;
    unsigned short data;
    int Led_control = 0;

    /* Turn off all LED lamps */
    data = LED_ALL_OFF;
    ioctl(fd, LED_IOCTL_SET, &data);

    for(i;i<8;i++)
    {
        data = Led_control + i;
        if(p[i+(par_lot-1)*8].parked || p[i+(par_lot-1)*8].reserve)
            ioctl(fd, LED_IOCTL_BIT_CLEAR, &data);
        else
            ioctl(fd, LED_IOCTL_BIT_SET, &data);
    }
}
