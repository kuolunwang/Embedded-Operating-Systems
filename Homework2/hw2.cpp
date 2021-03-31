#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "sockop.h"
#include <semaphore.h>
#include <vector>
#include <algorithm>
#include <cstdint>

#define BUFSIZE 1024
using namespace std;
vector<int> vec;
int money = 0;


// park struct 
struct park
{
    bool reserve;
    bool parked;
    int plate_number;
};

struct park p[24]={0};
void *process(void *t);
void *th(void *arg);

char car_state(int num,struct park *p);
bool cancel(struct park *p,int pla_num,int connfd);
bool reserved_mode(struct park *p,int pla_num,int connfd);
bool parked_mode(struct park *p,int pla_num,int connfd);
bool parking_mode(struct park *p,int pla_num,int connfd);
bool check_in(struct park *p,int pla_num,int lot,int grid,int connfd);
bool reser(struct park *p,int pla_num,int lot,int grid,int connfd);
void show(struct park *p,int connfd);
bool pick_up(struct park *p,int pla_num,int connfd);
char car_state(int num,struct park *p);

// pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
// sem_t semaphore;

// void remove_semaphore(){ 
//     int de; 
//     printf ( "Clean up mutex\n" ) ;
//     de = pthread_mutex_destroy(&mutex1 ) ;
//     de = pthread_mutex_destroy(&mutex2 ) ;
// }

void handler(int signum) {
    // remove_semaphore();
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// void endd(int signum){
void *th(void * arg){
    char buffer[BUFSIZE];
    int i;
    int grid;
    int lot;
    FILE *pfile;
    while(scanf("%s",buffer))
    {
        if(strcmp(buffer,"end") == 0)
        {
            pfile = fopen("result.txt","w");
            for(i =0;i<24;i++){
                if(p[i].plate_number != 0){
                    grid = (i%8)+1;
                    lot = (i+9-grid)/8;
                    sprintf(buffer,"%d %d %d\n",lot,grid,p[i].plate_number);
                    printf("%d %d %d\n",lot,grid,p[i].plate_number);
                    fwrite(buffer,1,strlen(buffer),pfile);
                }
            }
            printf("Total income : $%d\n",money);
            sprintf(buffer,"Total income : $%d\n",money);
            fwrite(buffer,1,strlen(buffer),pfile);
            fclose(pfile);
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    
    int sockfd, connfd;
    struct sockaddr_in cln;
    socklen_t cln_len = sizeof(cln);

    int pth_c, p;
    pthread_t threadd,thr;
    int t=0;
    
    if(argc != 2){
        errexit("Usasge: %s port\n", argv[0]);
    }

    sockfd = passivesock(argv[1], "tcp", 10);

    // signal(SIGCHLD, handler);
    // signal(SIGINT, endd);
    
    p = pthread_create(&thr, NULL, th, (void *)t);
    if(p)
    {
        printf("ERROR return code from pthread_create( ) is %d\n", p);
        exit(-1);
    }
    while(1){
        connfd = accept(sockfd, ( struct sockaddr *) &cln, &cln_len);
        // printf("%d\n",t);
        // pthread_mutex_lock( &mutex1 );
        pth_c = pthread_create(&threadd, NULL, process, (void *)connfd);
        
        // pthread_mutex_unlock( &mutex1 );
        if(pth_c)
        {
            printf("ERROR return code from pthread_create( ) is %d\n", pth_c);
            exit(-1);
        }
    }
    close(sockfd);

    return 0;
}

void *process(void *t)
{
    /* initial */
    char state;
    char snd[BUFSIZE], rcv[BUFSIZE];
    int connfd = (intptr_t)t;
    int n;
    bool con =false;
    int pla_num=0;

    /* start */
    while(1)
    {
        /* input plate number */
        memset(rcv,'\0',sizeof(rcv));
        if((n = read(connfd, rcv, BUFSIZE)) == -1){
            errexit("Error: read()\n");
        }
        sscanf(rcv,"%d",&pla_num);
        printf("%d\n",pla_num);
        if(pla_num < 0 || pla_num >9999)
        {
            memset(snd,'\0',sizeof(snd));
            n = sprintf(snd, "Login failed.");
            
            if((n = write(connfd, snd, n)) == -1)
                errexit("Error: write()\n");
        }
        else
        {
            // pthread_mutex_lock( &mutex2 );
            vector<int>::iterator it = find(vec.begin(), vec.end(), pla_num);
            // pthread_mutex_unlock( &mutex2 );
            if(it != vec.end()) //exist
            {
                memset(snd,'\0',sizeof(snd));
                n = sprintf(snd, "Login failed.");
                if((n = write(connfd, snd, n)) == -1){
                    errexit("Error: write()\n");
                }
            }
            else
            {
                // pthread_mutex_lock( &mutex1 );
                vec.push_back(pla_num);
                state = car_state(pla_num,p);
                memset(rcv,'\0',sizeof(rcv));
                break;
                // pthread_mutex_unlock( &mutex1 );
            }
        }
    }

    while(1)
    {   
        if(state == 1) //reserved
        {
            n = sprintf(snd, "You have reserved grid.");
            if((n = write(connfd,snd, n)) == -1){
                errexit("Error: write()\n");
            }
            while(con == false)
                con = reserved_mode(p,pla_num,connfd);
            break;
        }
        else if (state == 2) // parked
        {
            int grid,lot;
            int i = 0;
            for(i;i<24;i++)
            {
                if(p[i].plate_number == pla_num)
                {
                    grid = i % 8 +1;
                    lot = (i + 9 - grid)/8 ;
                }
            }
            n = sprintf(snd, "Your grid is at lot P%d grid %d.",lot,grid);
            if((n = write(connfd,snd, n)) == -1){
                errexit("Error: write()\n");
            }
            while(con == false)
                con = parked_mode(p,pla_num,connfd);
            break;
        }
        else // none
        {
            n = sprintf(snd, "You haven't reserved grid.");
            if((n = write(connfd,snd, n)) == -1){
                errexit("Error: write()\n");
            }
            while(con == false)
                con = parking_mode(p,pla_num,connfd);
            break;
        }
    }
    vector<int>::iterator it = find(vec.begin(), vec.end(), pla_num);
    vec.erase(it);
    close(connfd);
    pthread_exit(NULL);
}

/* you haven't parked your car yet */
bool parking_mode(struct park *p,int pla_num,int connfd)
{
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
    int lot,grid;
    char judge[BUFSIZE]; 
    char text[5];
    
    bool con =false;
        if((n = read(connfd, rcv ,BUFSIZE)) == -1){
            errexit("Error: read()");
        }
        sscanf(rcv,"%s %d %d",judge,&lot,&grid);
        strncpy(text,judge,4);
        if(strcmp(text , "show") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            show(p,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(judge , "reserve") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            con = reser(p,pla_num,lot,grid,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(judge , "check-in") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            con = check_in(p,pla_num,lot,grid,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(text , "exit") == 0)
        {
            n =sprintf(snd, "Logout.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
            con = true;
        }
        else
        {
            n =sprintf(snd, "Invaild command.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n"); 
        }
    return con;
}

/* reserve the park grid */
bool reser(struct park *p,int pla_num,int lot,int grid,int connfd)
{
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
    bool con =false;
    if(lot< 0 || lot >3 || grid >9 || grid <1){
        n = sprintf(snd, "Invaild command.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
        }
    }
    else if(p[(lot-1)*8+grid-1].plate_number == 0)
    {
        // pthread_mutex_lock( &mutex1 );
        n =sprintf(snd, "Reserve successful.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
        }
        p[(lot-1)*8+grid-1].plate_number = pla_num;
        p[(lot-1)*8+grid-1].parked = false;
        p[(lot-1)*8+grid-1].reserve = true;
        // pthread_mutex_unlock( &mutex1 );
        con = true;
    }
    else
    {
        n = sprintf(snd, "Error! Please select an ideal grid.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
        }
    }
    return con;
}

/* you already reserved park grid */
bool reserved_mode(struct park *p, int pla_num,int connfd)
{
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
    int lot,grid;
    char judge[BUFSIZE]; 
    char text[5];
    bool con =false;
        if((n = read(connfd, rcv ,BUFSIZE)) == -1){
            errexit("Error: read()\n");
        }
        sscanf(rcv,"%s %d %d",judge,&lot,&grid);
        strncpy(text,judge,4);
        if(strcmp(text , "show") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            show(p,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(judge , "cancel") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            con = cancel(p,pla_num,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(judge , "check-in") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            con = check_in(p,pla_num,lot,grid,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(text , "exit") == 0)
        {
            n =sprintf(snd, "Logout.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
            con = true;
        }
        else
        {
            n =sprintf(snd, "Invaild command.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
        }
    return con;
}

/* you have parked your car  */
bool parked_mode(struct park *p, int pla_num, int connfd)
{
    char snd[BUFSIZE], rcv[BUFSIZE];
    char text[5];
    int n;
    char judge[BUFSIZE]; 
    bool con =false;
        if((n = read(connfd, rcv ,BUFSIZE)) == -1){
            errexit("Error: read()\n");
        }
        sscanf(rcv,"%7s",judge);
        strncpy(text,judge,4);
        if(strcmp(text , "show") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            show(p,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(judge , "pick-up") == 0)
        {
            // pthread_mutex_lock( &mutex1 );
            con = pick_up(p,pla_num,connfd);
            // pthread_mutex_unlock( &mutex1 );
        }
        else if(strcmp(text , "exit") == 0)
        {
            n =sprintf(snd, "Logout.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
            con = true;
        }
        else
        {
            n =sprintf(snd, "Invaild command.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
        }
    return con;
}

/* parking your car */
bool check_in(struct park *p,int pla_num,int lot,int grid,int connfd)
{
    int tmp;
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
    int i = 0;
    bool con = false;

    for(i;i<24;i++)
    {
        if(p[i].plate_number == pla_num && p[i].reserve)
        {
            p[i].parked = true;
            n =sprintf(snd, "Check-in successful.");
            if((n = write(connfd,snd, n)) == -1)
                errexit("Error: write()\n");
            return true;
        }
    }

    if(lot< 0 || lot >3 || grid >9 || grid <1)
    {
        n = sprintf(snd, "Invaild command.");
        if((n = write(connfd,snd, n)) == -1)
        {
            errexit("Error: write()\n");
        }
    }
    else if(p[(lot-1)*8+grid-1].plate_number == 0)
    {
        // pthread_mutex_lock( &mutex1 );
        n =sprintf(snd, "Check-in successful.");
        if((n = write(connfd,snd, n)) == -1)
            errexit("Error: write()\n");
        p[(lot-1)*8+grid-1].plate_number = pla_num;
        p[(lot-1)*8+grid-1].parked = true;
        p[(lot-1)*8+grid-1].reserve = false;
        // pthread_mutex_unlock( &mutex1 );
        con = true;
    }
    else
    {
        n = sprintf(snd, "Error! Please select an ideal grid.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
        }
    }

    return con;
}

/* pick up your car */
bool pick_up(struct park *p,int pla_num,int connfd)
{
    bool re =false;
    int i = 0;
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
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

    if(re)
    {
        money += 30;
        n =sprintf(snd, "Parking fee: $30.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
        }
    }
    else
    {
        money += 40;
         n =sprintf(snd, "Parking fee: $40.");
        if((n = write(connfd,snd, n)) == -1){
            errexit("Error: write()\n");
       }
    }

    return true;

}

/* cancel reserved park grid */
bool cancel(struct park *p,int pla_num,int connfd)
{
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;
    int i = 0;

    for(i;i<24;i++)
    {
        if(p[i].plate_number == pla_num)
        {
            p[i].reserve = false;
            p[i].plate_number = 0;
            n = sprintf(snd, "Reserve fee: $20.");
            if((n = write(connfd,snd, n)) == -1){
                errexit("Error: write()\n");
            }
            money += 20;
            break;
        }
    }
    return true;
}

/* show the rest park grid on three park lots */
void show(struct park *p,int connfd)
{
    int i = 0;
    int p1,p2,p3;
    p1=p2=p3=8;
    int n;
    int j = 1;
    char show1[18]={0};
    char show2[18]={0};
    char show3[18]={0};
    char snd[BUFSIZE];
    show1[0]=show2[0]=show3[0]='|';
    for(i;i<24;i++)
    {
        if((p[i].parked) || (p[i].reserve))
        {
            if(i<=7)
                p1 -= 1;
            else if(i<=15)
                p2 -= 1;
            else
                p3 -= 1;
        }
        else
        {
            if(i<=7)
            {
                show1[j++] = i%8+1+48;
                show1[j++] = '|';
            }
            else if(i<=15)
            {
                show2[j++] = i%8+1+48;
                show2[j++] = '|';
            }
            else
            {
                show3[j++] = i%8+1+48;
                show3[j++] = '|';
            }
        }
        if((i+1)%8 == 0)
            j=1;
    }
    n = sprintf(snd, "P1: %d\ngrid %s\nP2: %d\ngrid %s\nP3: %d\ngrid %s\n", p1,show1,p2,show2,p3,show3);
    if((n = write(connfd,snd, n)) == -1){
        errexit("Error: write()\n");
    }
}

/* get car state */
char car_state(int num,struct park *p)
{
    int i=0;
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
