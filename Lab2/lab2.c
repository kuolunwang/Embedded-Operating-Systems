#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"

void decimal_to_Binary(unsigned char n, char* binary);
unsigned long decimal_to_7seghex(unsigned char n);
void led_setting(char* binary,int fd);
void seg_setting(unsigned char n,int fd); 
int calculate(char* e);
char prior(char op);
int ca(char op, int x, int y);

/* stack */
struct stack
{
    int top;
    unsigned char size;
    int *array;
};

struct stack* createstack(unsigned char size)
{
    struct stack* s = (struct stack*) malloc(sizeof(struct stack));
    s -> top = -1;
    s -> size = size;
    s -> array = (int*)malloc(s->size*sizeof(int));
    return s;
}

bool isempty(struct stack* s)
{
    if(s -> top == -1)
        return true;
    else 
        return false;
}

void push(int x, struct stack* s)
{
    if (s -> size -1 == s -> top)
        return;
    s -> array [++ s -> top] = x;
}

int pop(struct stack* s)
{
    if(isempty(s))
        return;
    return s -> array [ s -> top --];
}

int main()
{
    unsigned short key;
    int fd,ret;
    char exp[100] = {0};
    unsigned char index = 0; 
    char* postfix;
    lcd_write_info_t display;
    char binary[8] = {0};
    int value; 
    bool end = false;

    /* Open device /dev/lcd */
    if((fd =open("/dev/lcd",O_RDWR))< 0) 
    {
        printf("Open /dev/lcd faild.\n");
        exit(-1);
    }
    /* clear LCD */
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    printf("start\n");

    /* start */
    while(1)
    {
        ret = ioctl(fd, KEY_IOCTL_CHECK_EMTPY, &key);
        if(ret<0)
        {
            sleep(1);
            continue;
        }
        ret = ioctl(fd, KEY_IOCTL_GET_CHAR,&key);
        if(end)
        {
            end = false;
            display.Count = sprintf((char *) display.Msg, "");
            index = 0;
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            ioctl(fd, LCD_IOCTL_WRITE, &display);
        }
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
                display.Count = sprintf((char *) display.Msg, "%c", key & 0xff);
                exp[index++] = key & 0xff;
                break;
            case 'A':
                display.Count = sprintf((char *) display.Msg, "+");
                exp[index++] = '+';
                break;
            case 'B':
                display.Count = sprintf((char *) display.Msg, "-");
                exp[index++] = '-';
                break;
            case 'C':
                display.Count = sprintf((char *) display.Msg, "*");
                exp[index++] = '*';
                break;
            case 'D':
                display.Count = sprintf((char *) display.Msg, "/");
                exp[index++] = '/';
                break;
            case '#':
                display.Count = sprintf((char *) display.Msg, "=");
                exp[index++] = '\0';
                ioctl(fd, LCD_IOCTL_WRITE, &display);
                /* output experssion result */
                value = calculate(exp);
                if(value > 255)
                    value = 255;
                else if(value < 0)
                    value = 0;
                /* convert to binary */
                decimal_to_Binary(value, binary);
                /* show led with binary */ 
                led_setting(binary,fd);
                /* show input value */
                seg_setting(value,fd);
                /* output result to lcd */
                display.Count = sprintf((char *) display.Msg,"%d", value);
                end = true;
                break;
            case '*':
                display.Count = sprintf((char *) display.Msg, "");
                ioctl(fd, LCD_IOCTL_CLEAR, NULL);
                index = 0;
                break;
            default:
                break;
        }
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }

    /* close fd */
    close(fd);
}

/* priority */
char prior(char op)
{
    switch(op)
    {
        case '+':
        case '-':
            return 1;
            break;
        case '*':
        case '/':
            return 2;
            break;
        default:
            return 0;
    }
}

/* infix to postfix and calculate value */
int calculate(char* e)
{
    /* infix to postfix */
    int result[100] = {0};
    char index = 0;
    bool number = false;
    struct stack* s = createstack(100);
    while(*e != '\0')
    {
        switch(*e)
        {
            case '+':
            case '-':
            case '*':
            case '/':
                number = false;
                while(!isempty(s) && prior(*e) <= prior(s->array[s->top]))
                    result[index ++] = pop(s);
                push(*e,s);
                break;
            default:
                if(number)
                    result[index-1] = result[index -1] * 10  + *e-48;
                else
                    result[index++] = *e-48;
                number = true;
        }
        e++;
    }
    while(!isempty(s))
        result[index++] = pop(s);
    result[index] = '\0';

    /* postfix calculate */
    index = 0;
    int x=0,y=0;
    while(result[index] != '\0')
    {
        if(prior(result[index]))
        {
            x = pop(s);
            y = pop(s);
            push(ca(result[index],x,y),s);
        }
        else
            push(result[index],s);
        index++;
    }
    return pop(s);
}

/* operator calculate */
int ca(char op, int x, int y)
{
    switch(op)
    {
        case '+': 
            return(x+y);
        case '-':
            return(y-x);
        case '*':
            return(x*y);
        case '/':
            return(y/x);
    }
}

/* setting 7_seg and show number n*/
void seg_setting(unsigned char n, int fd)
{
    _7seg_info_t data_seg;

    ioctl(fd, _7SEG_IOCTL_ON, NULL);
    data_seg.Mode = _7SEG_MODE_HEX_VALUE;
    data_seg.Which = _7SEG_ALL;
    data_seg.Value = 0x0000+decimal_to_7seghex(n);
    ioctl(fd, _7SEG_IOCTL_SET, &data_seg);
}

/* setting led by binary*/
void led_setting(char* binary, int fd)
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
        if(binary[7-i])
            ioctl(fd, LED_IOCTL_BIT_SET, &data);
    }
}

/* convert decimanl to 7seg value */
unsigned long decimal_to_7seghex(unsigned char n)
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

/* convert decimanl to binary */
void decimal_to_Binary(unsigned char n, char* binary)
{
    char i = 0;

    for(i;i<8;i++)
        binary[i] =0;

    i = 0;
    while(n != 0)
    {
        binary[i] = n%2;
        n /= 2;
        i ++; 
    }
}
