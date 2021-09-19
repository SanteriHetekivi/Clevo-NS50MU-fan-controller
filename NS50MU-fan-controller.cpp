#include <sys/io.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <chrono>
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
using namespace std;

#define EC_COMMAND_PORT         0x66
#define EC_DATA_PORT            0x62
#define TEMP                    0x9E

#define FAN_MIN_VALUE           40  //minimal rotation speed of the fan (0-255)

#define FAN_OFF_TEMP            65  //temp below which the fan is off
#define FAN_25P_TEMP            70  //temp at which fan will be spinning at it's 25% speed.
#define FAN_50P_TEMP            78  //temp at which fan will be spinning at it's 50% speed.
#define FAN_75P_TEMP            82  //temp at which fan will be spinning at it's 75% speed.
#define FAN_100P_TEMP           85  //at which temperature and above the fan should be at it's 100%?

#define REFRESH_RATE            250 //time to wait between each controller loop (ms)
#define MAX_FAN_SET_INTERVAL    2000//maximal time between two fan rate send command



static int EcInit()
{
    if (ioperm(EC_DATA_PORT, 1, 1) != 0)
    {
        return 1;
    }

    if (ioperm(EC_COMMAND_PORT, 1, 1) != 0)
    {
        return 1;
    }

    return 0;
}

static void EcFlush()
{
    while ((inb(EC_COMMAND_PORT) & 0x1) == 0x1)
    {
        inb(EC_DATA_PORT);
    }
}


static void SendCommand(int command)
{
    int tt = 0;
    while((inb(EC_COMMAND_PORT) & 2))
    {
        tt++;
        if(tt>30000)
        {
            break;
        }
    }

    outb(command, EC_COMMAND_PORT);
}


static void WriteData(int data)
{
    while((inb(EC_COMMAND_PORT) & 2));

    outb(data, EC_DATA_PORT);
}


static int ReadByte()
{
    int i = 1000000;
    while ((inb(EC_COMMAND_PORT) & 1) == 0 && i > 0)
    {
        i -= 1;
    }

    if (i == 0)
    {
        return 0;
    }
    else
    {
        return inb(EC_DATA_PORT);
    }
}

static void setFanSpeed(int speed){
    EcInit();
    SendCommand(0x99);
    WriteData(0x01); //ID
    WriteData(speed);
}

static int GetLocalTemp()
{
    int index = 1;
    EcInit();
    EcFlush();
    SendCommand(TEMP);
    WriteData(index);
    //ReadByte();
    int value = ReadByte();
    return value;
}

static unsigned int time(){
    chrono::milliseconds ms = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());
    unsigned int time = ms.count();
    return time;
}

unsigned int perc(unsigned int fanSpeed){
    return round((float)(fanSpeed)/255*100);
}

unsigned int unperc(unsigned int fanSpeed){
    return round((float)(fanSpeed)/100*255);
}


int main (int argc, char *argv[])
{
    int lastFanSpeed = -1;                  //last fan speed value, used to avoid write speed if not necessary
    int fanSpeed = -1;            //last max speed value, used in combination with FAN_PEAK_HOLD_TIME
    unsigned int maxFanSpeedTime = 0;       //time at which the last max was reached, used in combination with FAN_PEAK_HOLD_TIME
    unsigned int lastTimeFanUpdate = 0;     //use this to periodically set the temp unconditionally (useful when wake of from sleep)
    while(1){
        int temp = GetLocalTemp();

        if (temp <= FAN_OFF_TEMP){
            fanSpeed = 0;
//            cout << "1! FAN IS OFF";
        }
        else if (temp <= FAN_25P_TEMP){
            fanSpeed = unperc(25 * ( float(temp - FAN_OFF_TEMP) / float(FAN_25P_TEMP - FAN_OFF_TEMP) ) );
//            cout << "2! FAN IS "<<fanSpeed;
        }
        else if (temp <= FAN_50P_TEMP){
            float slider = ( float(temp - FAN_25P_TEMP) / float(FAN_50P_TEMP - FAN_25P_TEMP) );
            fanSpeed = unperc(25 + ( 25 * slider ) );
//            cout << "3! FAN IS "<<fanSpeed << " AND SLIDER IS " << slider;
        }
        else if (temp <= FAN_75P_TEMP){
            float slider = ( float(temp - FAN_50P_TEMP) / float(FAN_75P_TEMP - FAN_50P_TEMP) );
            fanSpeed = unperc(50 + slider );
//            cout << "4! FAN IS "<<fanSpeed << " AND SLIDER IS " << slider;
        }
        else if (temp <= FAN_100P_TEMP){ //75p is 85c and 100p is 90c
            float slider = ( float(temp - FAN_75P_TEMP) / float(FAN_100P_TEMP - FAN_75P_TEMP) );
            fanSpeed = unperc(75 + ( 25 * slider ) );
//            cout << "5! FAN IS "<<fanSpeed << " AND SLIDER IS " << slider;
        }
        else if (temp > FAN_100P_TEMP){
            fanSpeed = 255;
//            cout << "6! FAN IS MAX!";
        }
        // Make sure fan speed is within boundaries.
        if(fanSpeed<FAN_MIN_VALUE && fanSpeed != 0){fanSpeed=FAN_MIN_VALUE;}
        if(fanSpeed>255)fanSpeed=255;

        if(lastFanSpeed!=fanSpeed || lastTimeFanUpdate+MAX_FAN_SET_INTERVAL < time()){ 
        //send value if it changed or if we didn't do it since more than "MAX_FAN_SET_INTERVAL" seconds.
           setFanSpeed(max(FAN_MIN_VALUE,fanSpeed));
            lastTimeFanUpdate=time();
            cout<<"T:"<<temp<<"°C | set fan to "<<perc(fanSpeed)<<"% ("<<fanSpeed<<")";
        }
        cout<<endl;
        lastFanSpeed=fanSpeed;
        usleep(REFRESH_RATE*1000);
    }
    return 0;
}