/*--------------- gpio-galileo_sw.c ---------------
by:	Mr. Ryan D Aldrich
	Mr. Taric S Alani
	Mr. John A Nicholson
EECE.5520-201 Microprocessors II
ECE Dept.
UMASS Lowell
Based on specification by Dr. Yan Luo:
lab2busgalileo.pdf 2016-09-21 18:08:32
https://piazza-resources.s3.amazonaws.com/
irdp4redng6fb/itdgocs3n7f1si/lab2busgalileo.pdf

PURPOSE
This source code uses the Galileo Gen 2 to command 
the PIC microcontroller to send converted ADC values 
using a custom Strobe and Acknowledge communication 
protocol.  The PIC microcontroller responds with its 
status or data.  
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define Strobe     (40) // GPIO40, pin 8
#define GP_4       (6)  // GPIO6, pin 4
#define GP_5	   (0)  // GPIO0, pin 5
#define GP_6	   (1)  // GPIO1, pin 6
#define GP_7	   (38) // GPIO38, pin 7
#define GPIO_DIRECTION_IN      (1)  
#define GPIO_DIRECTION_OUT     (0)
#define ERROR                  (-1)
#define HIGH        (1)
#define LOW         (0)

/* user commands */
#define	MSG_RESET	0x0			/* reset the sensor to initial state */
#define MSG_PING	0x1			/* check if the sensor is working properly */
#define MSG_GET		0x2			/* obtain the most recent ADC result */

/* Sensor Device Responses */
#define MSG_ACK		0xF			/* acknowledgement to the commands */
#define MSG_NOTHING	0xE			/* reserved */

// system clock timer
void timer(unsigned int ms)
{
	// timer local variables
    clock_t start, end;
	long stop;

    stop = ms*(CLOCKS_PER_SEC/1000);
    start = end = clock();
    while((end - start) < stop) //do not use for(), cause it needs 3 clock cycle to execute.
        end = clock();
}

//open GPIO and set the direction
void openGPIO(int gpio, int direction )
{		
        //  initialize character arrays for system commands and clear them
        char buf[4] = {}; // 4 = size of "out" plus '\0'
		memset(buf,0,sizeof(buf));
		char str_dir_set[50] = {};  // 50 = size of system() call
		memset(str_dir_set,0,sizeof(str_dir_set));
		
		// detect direction from input arguement
		if (direction == GPIO_DIRECTION_IN)
		{
			strcat(buf,"in");  
		}
		else if (direction == GPIO_DIRECTION_OUT)
		{
			strcat(buf,"out");
		}
		// switch on gpio and create direction system commands
		// direction is a file that contains a string that is either "out" or "in"
		switch(gpio)
		{
			case Strobe:
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio40/direction", buf);
			system(str_dir_set);
			case GP_4: // pin 4 requires a linux and level shifter GPIO
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio6/direction", buf);
			system(str_dir_set);
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio36/direction", buf);
			system(str_dir_set);
			case GP_5: // pin 5 requires a linux and level shifter GPIO
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio0/direction", buf);
			system(str_dir_set);
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio18/direction", buf);
			system(str_dir_set);
			case GP_6:  // pin 6 requires a linux and level shifter GPIO
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio1/direction", buf);
			system(str_dir_set);
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio20/direction", buf);
			system(str_dir_set);
			case GP_7:
			sprintf(str_dir_set, "echo -n %s > /sys/class/gpio/gpio38/direction", buf);
			system(str_dir_set);
		}
		
}

//write value
void writeGPIO(int gpio, int value)
{
	// initialize system call string for setting the value
	// value is a file that contains a string that is either "0" or "1"
    char str_st1[50] = {};  // 50 = size of system() call
	memset(str_st1,0,sizeof(str_st1));
	
	// write the first portion of the value system() call
    if (value==HIGH) // if value is HIGH
    {
		strcat(str_st1,"echo -n \"1\" > /sys/class/gpio/");
    }
    else if (value==LOW) // if value is LOW
    {
		strcat(str_st1,"echo -n \"0\" > /sys/class/gpio/");
    }
	// switch on gpio and create value system commands
	// writing to the value file requires the Linux GPIO
    switch (gpio)
    {
		case Strobe:
			strcat(str_st1,"gpio40/value\n");
			system(str_st1);
			break;
        case GP_4:
            strcat(str_st1,"gpio6/value\n");
            system(str_st1);
            break;
        case GP_5:
            strcat(str_st1,"gpio0/value\n");
            system(str_st1);
            break;
        case GP_6:
            strcat(str_st1,"gpio1/value\n");
            system(str_st1);
            break;
        case GP_7:
            strcat(str_st1,"gpio38/value\n");
            system(str_st1);
            break;
    }
        
}

// write commands on the bus to the PIC
void writeBUS(int command)
{
	// set pins 4-7 to the out direction
    openGPIO(GP_4, GPIO_DIRECTION_OUT);
    openGPIO(GP_5, GPIO_DIRECTION_OUT);
    openGPIO(GP_6, GPIO_DIRECTION_OUT);
    openGPIO(GP_7, GPIO_DIRECTION_OUT);
    
	// detect command and write GPIO values for that command
    if(command==MSG_RESET)
    {
        writeGPIO(GP_4,LOW);
        writeGPIO(GP_5,HIGH);
    }
    if (command==MSG_PING)
    {
        writeGPIO(GP_4,HIGH);
        writeGPIO(GP_5,LOW);
    }
    if(command==MSG_GET)
    {
        writeGPIO(GP_4,HIGH);
        writeGPIO(GP_5,HIGH);
    }
	
    timer(1);  // delay 1 microsecond
}

// declare data read variables
int gp4;
int gp5;
int gp6;
int gp7;

// read the output of the system() call that had printed
// the contents of value to the screen
void readBUS(void)
{   
    // declare file pointers
    FILE *fp1;
    FILE *fp2;
    FILE *fp3;
    FILE *fp4;
	
    // open value files on Linux system for reading to pointer
    fp1 = popen("cat /sys/class/gpio/gpio6/value", "r");
    fp2 = popen("cat /sys/class/gpio/gpio0/value", "r");
    fp3 = popen("cat /sys/class/gpio/gpio1/value", "r");
    fp4 = popen("cat /sys/class/gpio/gpio38/value", "r");
    
	// get values in files and put them in data read variables
    gp4 = (fgetc(fp1) & 1);
    gp5 = (fgetc(fp2) & 1);
    gp6 = (fgetc(fp3) & 1);
    gp7 = (fgetc(fp4) & 1);
	
	// close the system file pointers
	pclose(fp1);
	pclose(fp2);
	pclose(fp3);
	pclose(fp4);
	
	// print data values to the terminal screen as hex numbers
	printf("%x, %x, %x, %x\n", gp7, gp6, gp5, gp4);
}

// clear the BUS 
void clearBUS(void)
{
	// set pins 4-7 to high impedance
    openGPIO(GP_4, GPIO_DIRECTION_IN);
    openGPIO(GP_5, GPIO_DIRECTION_IN);
    openGPIO(GP_6, GPIO_DIRECTION_IN);
    openGPIO(GP_7, GPIO_DIRECTION_IN);
	
}

//main
int main(void)
{ 
	openGPIO(Strobe, GPIO_DIRECTION_OUT);  // set Strobe GPIO direction
    unsigned char count = 0;			   // initialize count for the timing loop
    unsigned int data;					   // initialize output data variable
	
	// initialize timer variables
	time_t start_t, end_t, total_t;
	
	//intially export all the pins
    char command[40] = {};
	memset(command,0,sizeof(command));
    // pin 8 strobe
	strcpy( command, "echo -n \"40\" > /sys/class/gpio/export\n");
    system( command);
    // pin 4 D0 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"6\" > /sys/class/gpio/export\n");
    system(command);
	strcpy( command, "echo -n \"36\" > /sys/class/gpio/export\n");
	system(command);
    // pin 5 D1 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"0\" > /sys/class/gpio/export\n");
    system(command);
	strcpy( command, "echo -n \"18\" > /sys/class/gpio/export\n");
    system(command);
    // pin 6 D2 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"1\" > /sys/class/gpio/export\n");
    system(command);
	strcpy( command, "echo -n \"20\" > /sys/class/gpio/export\n");
    system(command);
    // pin 7 D3
	strcpy(command, "echo -n \"38\" > /sys/class/gpio/export\n");
    system(command);
		
		// enter infinite loop
        while(1)
        {
				switch(count)
				{
				case 0: // start Strobe LOW
                    writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 1: // send Reset command on Strobe HIGH
					writeBUS(MSG_RESET);
                    writeGPIO(Strobe, HIGH);
					break;
                case 2: // bring Strobe LOW and clear bus
                    writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 3: // bring Strobe HIGH and check for ACK
				    writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
                    if((gp4&&gp5&&gp6&&gp7)==1) // ACK is 1111
                    {
                        printf("Acknowledge MSG_RESET\n");
                    }
                    else 
                    {
                        printf("Error Reset\n");
                    }
					break;
                case 4:  // bring Strobe LOW and clear bus
                    writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 5:  // send Ping command on Strobe HIGH
					writeBUS(MSG_PING);	
					writeGPIO(Strobe, HIGH);
					break;
				case 6:  // bring Strobe LOW and clear bus
					writeGPIO(Strobe, LOW);
					clearBUS();
					break;
				case 7: // bring Strobe HIGH and check for ACK
					writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
                    if((gp4&&gp5&&gp6&&gp7)==1)  // ACK is 1111
                    {
                        printf("Acknowledge MSG_PING\n");
                    }
                    else 
                    {
                        printf("Error Ping\n");
                    }
					break;
                case 8:  // bring Strobe LOW and clear bus
                    writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 9:  // send Get command on Strobe HIGH
				    writeBUS(MSG_GET);
					timer(1);
                    writeGPIO(Strobe, HIGH);
					break;
                case 10:  // bring Strobe LOW and clear bus
                    writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 11:  // bring Strobe HIGH and check for ACK
					writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
                    if((gp4&&gp5&&gp6&&gp7)==1)  // ACK is 1111
                    {
                        printf("Acknowledge MSG_GET\n");
                    }
                    else 
                    {
                        printf("Error Get\n");
                    }
					break;
				case 12:  // bring Strobe LOW and clear bus
					writeGPIO(Strobe, LOW);
					clearBUS();
					break;
				case 13:  // bring Strobe HIGH and read first nibble
					writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
					data = gp4;
					data = ((gp5<<1)+data);
					data = ((gp6<<2)+data);
					data = ((gp7<<3)+data);
					break;
				case 14:  // bring Strobe LOW and clear bus
					writeGPIO(Strobe, LOW);
					clearBUS();
					break;
				case 15:  // bring Strobe HIGH and read second nibble
					writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
					data = ((gp4<<4)+data);
					data = ((gp5<<5)+data);
					data = ((gp6<<6)+data);
					data = ((gp7<<7)+data);
					break;
				case 16:  // bring Strobe LOW and clear bus
					writeGPIO(Strobe, LOW);
					clearBUS();
					break;
				case 17:  // bring Strobe HIGH and read third nibble
					writeGPIO(Strobe, HIGH);
					timer(10);
					readBUS();
					data = ((gp4<<8)+data);
					data = ((gp5<<9)+data);
					break;
				case 18:  // bring Strobe LOW and clear bus
					writeGPIO(Strobe, LOW);
					clearBUS();
					break;
                case 19:  // print ADC value to screen
					printf("Output = %u\n",data);
                    count = 0;  // reset count
					data = gp4;  // reset data value
					break;
				default:
					break;
				}
				timer(250); // 0.25 second delay
				count++;  // increment timing count
        }
}


