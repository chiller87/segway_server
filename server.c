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
   int num_ctrl_stmts = 4;

/**
      defines the control statements. You have to add your control statement as a char sequence of 4 chars.
   */
   char **ctrl_stmts = (char **) malloc(num_ctrl_stmts * sizeof(char *));
   
   ctrl_stmts[0] = "(0,0,0)"; // not connected
   ctrl_stmts[1] = "STOP"; // kill application
   ctrl_stmts[2] = "spee"; // to modify speed
   ctrl_stmts[3] = "dire"; // to modify direction





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
void action(int stmt_no, char *payload) {
   switch(stmt_no) {
   
   case 0:
      // reset speed and direction to defaults
      printf("no connection!");
      setSpeed("0");
      setDirection("50");
      break;
      
   case 1:
      STOP = TRUE;
      break;
   
   case 2:
      setSpeed(payload);
      break;
      
   case 3:
      setDirection(payload);
      break;
      
   default:
      printf("statement number '%d' not defined!\n", stmt_no);
      break;
   }
   
}








void checkConnection(int fd) {

   int num_write_cmds = 3;
   char **write_cmds = (char **) malloc(num_write_cmds * sizeof(char *));
   write_cmds[0] = "$$$"; // to enter command mode
   write_cmds[1] = "GK\n"; // to check, whether a device is connected or not
   write_cmds[2] = "---\n"; // to leave command mode

      
   int n = 0, res;
   
   char buf[255];
   
   for(n = 0; stmts[0][n] != '\0'; n++);
   printf("cmd = %s\n", stmts[0]);
   printf("n = %d\n", n);
   write(fd, stmts[0], n);
   
   res = read(fd, buf, response_sizes[0]);
   buf[res] = 0;
   printf("msg: %s\n", buf);
   
   printf("command mode on!\n-------------------------\n");

   sleep(2);
   
   
   
   for(n = 0; stmts[1][n] != '\0'; n++);
   printf("cmd = %s\n", stmts[1]);
   printf("n = %d\n", n);
   write(fd, stmts[1], n);
   
   sleep(2);
   
   res = read(fd, buf, response_sizes[1]);
   printf("read %d character\n", res);
   buf[res] = 0;
   printf("msg: [%s]\n", buf);
   
   printf("query finished!\n-------------------------\n");
   
   sleep(2);
   
   for(n = 0; stmts[2][n] != '\0'; n++);
   printf("cmd = %s\n", stmts[2]);
   printf("n = %d\n", n);
   write(fd, stmts[2], n);
   
   res = read(fd, buf, response_sizes[2]);
   printf("read %d character\n", res);
   buf[res] = 0;
   printf("msg: [%s]\n", buf);
   
   printf("command mode off!\n--------------------------\n");

   
}



void parseMsg(char *buf, int length) {
   
   int found = FALSE;
   
   buf[length] = 0;
   if(strncmp(buf, ctrl_stmts[0], NO_CON_MSG_SIZE) == 0) {
      found = TRUE;
      action(0, &buf[CTRL_STMT_SIZE]);
   }
   
   for(i = 1; i < num_ctrl_stmts; i++) {
      if(strncmp(buf, ctrl_stmts[i], CTRL_STMT_SIZE) == 0) {
         printf("received control statement '%s'\n", read_buf);

         printf("payload: '%s'\n", &buf[CTRL_STMT_SIZE]);

         action(i, &buf[CTRL_STMT_SIZE]);

         found = TRUE;

         break;
      }
   }
   
   if(found == FALSE) {
      printf("control statement '%c%c%c%c' does not exist!\n", buf[0], buf[1], buf[2], buf[3]);
   }
}



void *listenBluetooth() {

   

   int fd, n;

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
   /*
   newtio.c_cc[VINTR]    = 0;     // Ctrl-c 
   newtio.c_cc[VQUIT]    = 0;     // Ctrl-\ //
   newtio.c_cc[VERASE]   = 0;     // del
   newtio.c_cc[VKILL]    = 0;     // @
   newtio.c_cc[VEOF]     = 4;     // Ctrl-d
   newtio.c_cc[VSWTC]    = 0;     // '\0'
   newtio.c_cc[VSTART]   = 0;     // Ctrl-q 
   newtio.c_cc[VSTOP]    = 0;     // Ctrl-s
   newtio.c_cc[VSUSP]    = 0;     // Ctrl-z
   newtio.c_cc[VEOL]     = 0;     // '\0'
   newtio.c_cc[VREPRINT] = 0;     // Ctrl-r
   newtio.c_cc[VDISCARD] = 0;     // Ctrl-u
   newtio.c_cc[VWERASE]  = 0;     // Ctrl-w
   newtio.c_cc[VLNEXT]   = 0;     // Ctrl-v
   newtio.c_cc[VEOL2]    = 0;     // '\0'
   */
   newtio.c_cc[VTIME]    = 0;     // timeout reading the next characters (10th of sec)
   newtio.c_cc[VMIN]     = 0;     // defines how many character to be read

   // set new port settings
   tcflush(fd, TCIFLUSH);
   tcsetattr(fd,TCSANOW,&newtio);
   
   //checkConnection(fd, write_cmds, response_sizes);


   /*
   // loop for input
   while (STOP==FALSE) {
   
      // reset found
      found = FALSE;
      
      // read control statement
      res = read(fd, read_buf, CTRL_STMT_SIZE);
      
      // so we can printf...
      read_buf[res] = 0;
      
      
      for(i = 0; i < num_ctrl_stmts; i++) {
         
         if(strncmp(read_buf, ctrl_stmts[i], 4) == 0) {
            printf("received control statement '%s'\n", read_buf);
            
            // read payload
            res = read(fd, read_buf, data_sizes[0]);
            // so we can printf...
            read_buf[res] = 0;
            
            printf("payload: '%s'\n", read_buf);
            
            action(i, read_buf);
            
            found = TRUE;
            
            break;
         }
      }
      
      if(found == FALSE) {
         printf("control statement '%s' does not exist!\n", read_buf);
         
         // read rest of unknown message
         res = read(fd, read_buf, sizeof(read_buf));
         read_buf[res] = 0;
         printf("rest: %s\n", read_buf);
      }
      

      // print the received msg
      //printf(":%s\n", buf);
      //if (buf[0]=='q') STOP=TRUE;
   }
   */
   
   n = 0;
   while(STOP == FALSE)
   {
   
      res = read(fd, &c, 1);
      if(c != '\n' && n < MAX_MSG_SIZE)
      {
         read_buf[n] = c;
         n++;
      }
      else if(c == '\n')
      {
         parseMsg(read_buf, n);
         n = 0;
      }
      
      if(STOP == TRUE) {
         break;
      }
   
   }
   
   
   // restore saved port settings
   tcsetattr(fd, TCSANOW, &oldtio);
}


 
void main() {

   int res, i, err;
   struct termios oldtio, newtio;
   
   
   

   
   /**
      defines the payload sizes for each control statement. You have to add the payload size as number of chars for your control statement.
   */
   //int *data_sizes = (int *) malloc(num_ctrl_stmts * sizeof(int));
   //data_sizes[0] = 3;
   //data_sizes[1] = 3;

   /*
   int *response_sizes = (int *) malloc(num_write_cmds * sizeof(int));
   response_sizes[0] = 5; // define response size for write_cmds[0]
   response_sizes[1] = 4; // define response size for write_cmds[1]
   response_sizes[2] = 7; // define response size for write_cmds[2]
   */
   
  
   pthread_t btThread;
   
   
   err = pthread_create(&btThread, NULL, &listenBluetooth, NULL);
   if(err != 0)
   {
      printf("Konnte Thread nicht erzeugen\n");
   }
   
   pthread_join(btThread, NULL);
   
   
}































