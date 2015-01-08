#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
  
#define BAUDRATE B115200 // define BAUD rate
#define MODEMDEVICE "/dev/ttyUSB0" // define serial port
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1

#define CTRL_STMT_SIZE 4
#define MAX_MSG_SIZE 256

#define NO_CON_MSG_SIZE 7
  
volatile int STOP = FALSE;




   /**
      defines how many control statements be checked. You have to edit this value to the number of control statements you want to check for.
   */
   int num_ctrl_stmts;

/**
      defines the control statements. You have to add your control statement as a char sequence of 4 chars.
   */
   char **ctrl_stmts;
   






void setSpeed(char *val) {
   int s = atoi(val);

   printf("set speed to '%d\%'\n", s);
}


void setDirection(char *val) {
   int d = atoi(val);
   
   printf("set direction to '%d'\n", d);
}







/**
   this function is called everytime a valid control statement was received to specify the action to be started.
   param stmt_no: the index of the read control statement
   param payload: the payload as a char array
   
   Here you should add a new case with the index of your control statement.
*/
void action(int stmt_no, char *payload, int fd) {
   switch(stmt_no) {
   
   case 0:
      checkConnection(fd, 0);
      break;
   
   case 1:
      //printf("command mode on\n");
      checkConnection(fd, 1);
      break;
   
   case 2:
      //printf("command mode off!\n");
      break;
   
   case 3:
      // reset speed and direction to defaults
      printf("no connection!\n");
      setSpeed("0");
      setDirection("50");
      checkConnection(fd, 2);
      break;
   
   case 4:
      // connection still alive
      printf("connection alive!\n");
      checkConnection(fd, 2);
      break;
      
   case 5:
      STOP = TRUE;
      break;
   
   case 6:
      setSpeed(payload);
      break;
      
   case 7:
      setDirection(payload);
      break;
      
   default:
      printf("statement number '%d' not defined!\n", stmt_no);
      break;
   }
   
}








void checkConnection(int fd, int mode) {

   int num_write_cmds = 3;
   char **write_cmds = (char **) malloc(num_write_cmds * sizeof(char *));
   write_cmds[0] = "$$$"; // to enter command mode
   write_cmds[1] = "GK\n"; // to check, whether a device is connected or not
   write_cmds[2] = "---\n"; // to leave command mode

   
   int n = 0, res, i;
   
   n = strlen(write_cmds[mode]);
   //printf("cmd = %s\n", write_cmds[mode]);
   //printf("n = %d\n", n);
   write(fd, write_cmds[mode], n);

   
   
   
}



void parseMsg(char *buf, int length, int fd) {
   
   int found = FALSE, i;
   
   
   //printf("got message '%s'\n", buf);
   
   for(i = 0; i < num_ctrl_stmts; i++) {
      if(strncmp(buf, ctrl_stmts[i], strlen(ctrl_stmts[i])) == 0) {
         //printf("received control statement '%s'\n", ctrl_stmts[i]);

         //printf("payload: '%s'\n", &buf[CTRL_STMT_SIZE]);


         action(i, &buf[CTRL_STMT_SIZE], fd);


         found = TRUE;

         break;
      }
   }
   
   if(found == FALSE) {
      printf("message '%s' could not be interpreted\n", buf);
   }
   
   printf("\n");
}



void *listenBluetooth() {

   
   struct termios oldtio, newtio;
   int fd, n, res;

   /**
      defines the maximum payload size. You should change the value to be able to receive a payload greater than 255 chars.
   */
   char read_buf[255];
   char write_buf[255];
   char c;

   fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
   if (fd <0) {perror(MODEMDEVICE); exit(-1); }

   // save current port settings
   tcgetattr(fd, &oldtio);
   
   /** 
      set params of serial interface
   */
   bzero(&newtio, sizeof(newtio));
   newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR;
   newtio.c_oflag = 0;

   // set input mode (non-canonical, no echo,...)
   //newtio.c_lflag = 0;

   /* 
    initialize all control characters 
    default values can be found in /usr/include/termios.h, and are given
    in the comments, but we don't need them here
   */

   newtio.c_cc[VTIME]    = 10;     // timeout reading the next characters (10th of sec)
   newtio.c_cc[VMIN]     = 0;     // defines how many character to be read

   // set new port settings
   tcflush(fd, TCIFLUSH);
   tcsetattr(fd,TCSANOW,&newtio);

   
   n = 0;
   int count = 0;
   int blah = 0;
   int timeout = 0;
   while(STOP == FALSE)
   {
   
      res = read(fd, &c, 1);
      //printf("res = %d\n", res);
      
      if(res == 0) {
         printf("nothing\n");
         timeout++;
         if(timeout >= 1) {
            checkConnection(fd, 0);
            timeout = 0;
         }
      }
      
      else {
         switch(c) {
            case '\r':
               //printf("backslash r received\n");
               break;
               
            case '\n':
               if(n > 0) {
                  read_buf[n] = '\0';
                  parseMsg(read_buf, n, fd);
                  n = 0;
               }
               else {
                  //printf("moep\n");
               }
               break;
               
            default:
               read_buf[n] = c;
               n++;
               timeout = 0;
               break;
         }
      }
      
      if(STOP == TRUE) {
         setSpeed("0");
         setDirection("50");
         break;
      }
      
      //printf("blahblah blub %d\n", blah++);
   
   }
   
   
   // restore saved port settings
   tcsetattr(fd, TCSANOW, &oldtio);
}


 
void main() {

   int res, i, err;
   
   
   num_ctrl_stmts = 8;
   ctrl_stmts = (char **) malloc(num_ctrl_stmts * sizeof(char *));
   ctrl_stmts[0] = "CHECK"; // to check bluetooth connection
   ctrl_stmts[1] = "CMD"; // command mode on
   ctrl_stmts[2] = "END"; // command mode off
   ctrl_stmts[3] = "0,0,0"; // not connected
   ctrl_stmts[4] = "1,0,0"; // connected
   ctrl_stmts[5] = "STOP"; // kill application
   ctrl_stmts[6] = "spee"; // modify speed
   ctrl_stmts[7] = "dire"; // modify direction
  

  
   pthread_t btThread;
   
   
   err = pthread_create(&btThread, NULL, &listenBluetooth, NULL);
   if(err != 0)
   {
      printf("Konnte Thread nicht erzeugen\n");
   }
   
   
   
   pthread_join(btThread, NULL);
   
   
}































