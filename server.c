#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
  
#define BAUDRATE B115200 // define BAUD rate
#define MODEMDEVICE "/dev/ttyUSB0" // define serial port
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1

#define CTRL_STMT_SIZE 4
#define MAX_MSG_SIZE 256
#define TIMEOUT 1

#define READ_BUF_SIZE 100 // read buffer size
#define WRITE_BUF_SIZE 100 // write buffer size

#define NO_CON_MSG_SIZE 7
 


int STOP = FALSE;
int cmd_mode = FALSE;
int connection = FALSE;


/**
	defines how many commands used to check connection
*/
int num_write_cmds;
/**
	defines the commands used to check connection
*/
char **write_cmds;



/**
  defines how many control statements be checked. You have to edit this value to the number of control statements you want to check for.
*/
int num_ctrl_stmts;
/**
  defines the control statements. You have to add your control statement as a char sequence of 4 chars.
*/
char **ctrl_stmts;



/**
	forward declarations
*/
void checkConnection(int fd, int mode);
int doCalculation(char *params);
void setSpeed(char *val);
void setDirection(char *val);
void sendMessage(int fd, char* msg);
void action(int stmt_no, char *payload, int fd);
void parseMsg(char *buf, int length, int fd);

/*
	method to read and write from and to bluetooth device via serial connection
*/
void *listenBluetooth();


/**
	dummy function to do some calculation
*/
int doCalculation(char *params) {
	
	int val = atoi(params);
	printf("got value '%d'\n", val);
	return val * 5;
}


/**
	dummy function to set the speed according to the received value (value from 0 to 100)
*/
void setSpeed(char *val) {
	int s = atoi(val);

	printf("set speed to '%d'\n", s);
}


/**
	dummy function to set the direction according to the received value (value from 0 to 100)
*/
void setDirection(char *val) {
	int d = atoi(val);

	printf("set direction to '%d'\n", d);
}



/**
	function to send data passed in 'msg' over 'fd'
*/
void sendMessage(int fd, char* msg) {
	printf("sending msg '%s'\n", msg);
	int bytes = write(fd, msg, strlen(msg));
	if(bytes == -1)
		printf("error writing!\n");
}


/**
   this function is called everytime a valid control statement was received to specify the action to be started.
   \param stmt_no: the index of the read control statement
   \param payload: the payload as a char array
   
   Here you should add a new case with the index of your control statement.
*/
void action(int stmt_no, char *payload, int fd) {

	int res;

	//printf("got action '%d'\n", stmt_no);

	switch(stmt_no) {

		case 0:
			cmd_mode = TRUE;
			checkConnection(fd, 0);
			break;

		case 1:
			//printf("command mode on\n");
			checkConnection(fd, 1);
			break;

		case 2:
			//printf("command mode off!\n");
			cmd_mode = FALSE;
			if(connection == FALSE) {
				printf("no connection!\n");
				setSpeed("0");
				setDirection("50");
			}
			else {
				printf("connection alive!\n");
			}
			break;

		case 3:
			// reset speed and direction to defaults
			//printf("no connection!\n");
			connection = FALSE;
			checkConnection(fd, 2);
			break;

		case 4:
			// connection still alive
			//printf("connection alive!\n");
			connection = TRUE;
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

		case 8:
			res = doCalculation(payload);
			char str[WRITE_BUF_SIZE];
			sprintf(str, "%d", res);
			sendMessage(fd, str);
			break;

		default:
			printf("statement number '%d' not defined!\n", stmt_no);
			break;
	}

}







/**
	this functions send the commands to 'fd' to check the connection

	\param fd: 		the file descriptor used to write
	\param mode: 	indicates what command to be sent
*/
void checkConnection(int fd, int mode) {
   
   	//printf("check connection with mode '%d'\n", mode);
	int n = strlen(write_cmds[mode]);
	//printf("cmd = %s\n", write_cmds[mode]);
	//printf("n = %d\n", n);
	int bytes = write(fd, write_cmds[mode], n);
	if(bytes == -1)
		printf("error writing!\n");

}


/**
	this method indicates what command has been received

	\param buf:		the received message
	\param length:	the length of the message (command + payload)
	\param fd:		file descriptor (serial USB)
*/
void parseMsg(char *buf, int length, int fd) {

	int found = FALSE, i;


	//printf("got message '%s'\n", buf);


	// define what range of control statements to be checked
	int start, stop;

	// if command mode on, i.e. the case that the progress to check the connection is currently running,
	// only accept the messages of the command mode
	if(cmd_mode == TRUE) {
			start = 1;
			stop = 5;
	}
	// else check for all other statements
	else {
			start = 0;
			stop = num_ctrl_stmts;
	}

	//printf("start = %d, stop = %d\n", start, stop);

	for(i = start; i < stop; i++) {
		//printf("i = %d\n", i);
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


	//printf("\n");
}



void *listenBluetooth() {

   
	struct termios oldtio, newtio;
	int fd, n, res;

	/**
	  defines the maximum payload size. You should change the value to be able to receive a payload greater than 255 chars.
	*/
	char read_buf[READ_BUF_SIZE];
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
	int timeout = 0;
	while(STOP == FALSE)
	{

		res = read(fd, &c, 1);
		//printf("res = %d\n", res);

		// nothing received
		if(res == 0) {
			printf("nothing\n");
			//printf("timeout = %d\n", timeout);
			if(timeout >= TIMEOUT) {
				checkConnection(fd, 0);
				timeout = 0;
			}
			else {
				timeout++;
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
					if(n < READ_BUF_SIZE-1) {
						read_buf[n] = c;
						n++;
						timeout = 0;
					}
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

	return NULL;
}


 
int main() {

	int err;
	pthread_t btThread;



	/**
		init
	*/
	// IMPORTANT: this value has to be changed to the array size of 'ctrl_stmts'
	num_ctrl_stmts = 9;

	ctrl_stmts = (char **) malloc(num_ctrl_stmts * sizeof(char *));
	// ###############################################################
	// these control statements are required to check the connection
	// NOT REMOVE THEM
	ctrl_stmts[0] = "CHECK"; // to check bluetooth connection
	ctrl_stmts[1] = "CMD"; // command mode on
	ctrl_stmts[2] = "END"; // command mode off
	ctrl_stmts[3] = "0,0,0"; // not connected
	ctrl_stmts[4] = "1,0,0"; // connected
	// ###############################################################
	// here you can add your own statements
	ctrl_stmts[5] = "STOP"; // kill application
	ctrl_stmts[6] = "spee"; // modify speed
	ctrl_stmts[7] = "dire"; // modify direction
	ctrl_stmts[8] = "calc"; // test calculation
	


	// init the number of write commands
	num_write_cmds = 3;
	// init the write commands
	write_cmds = (char **) malloc(num_write_cmds * sizeof(char *));
	write_cmds[0] = "$$$"; // to enter command mode
	write_cmds[1] = "GK\n"; // to check, whether a device is connected or not
	write_cmds[2] = "---\n"; // to leave command mode





	// create thread
	err = pthread_create(&btThread, NULL, &listenBluetooth, NULL);
	if(err != 0)
	{
		printf("Konnte Thread nicht erzeugen\n");
	}


	// wait for thread to end
	pthread_join(btThread, NULL);

	return 0;
   
}































