#include <sys/io.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <chrono>
using namespace std; 

#define EC_COMMAND_PORT         0x66
#define EC_DATA_PORT            0x62
#define TEMP                    0x9E

#define FAN_OFF_TEMP            70  //temp below which fan is off
#define FAN_MAX_TEMP            90  //at which temperature the fan should be maximum ?
#define FAN_MIN_VALUE           100 //speed (between 0 and 255) at which fan will turn when FAN_OFF_TEMP is reached
#define FAN_PEAK_HOLD_TIME      10000  //when a maximum of fan rotation rate is reached, hold much time (ms) to hold it before allowing to decrease the value




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

int main (int argc, char *argv[])
{
    int lastFanSpeed = -1;                  //last fan speed value, used to avoid write speed if not necessary
    int slidingMaxFanSpeed = -1;            //last max speed value, used in combination with FAN_PEAK_HOLD_TIME
    unsigned int maxFanSpeedTime = 0;       //time at which the last max was reached, used in combination with FAN_PEAK_HOLD_TIME
    unsigned int lastTimeFanUpdate = 0;     //use this to periodically set the temp unconditionnaly (useful when wake of from sleep)
    while(1){
        int temp = GetLocalTemp();
        //dynamic fan speed is the computed instantaneous speed, whithout hysteresis (FAN_PEAK_HOLD_TIME)
        int dynamicFanSpeed = round((float)((float)(temp-FAN_OFF_TEMP)/(FAN_MAX_TEMP-FAN_OFF_TEMP)*(255-FAN_MIN_VALUE) + FAN_MIN_VALUE));
        if(dynamicFanSpeed<FAN_MIN_VALUE)dynamicFanSpeed=0;
        if(dynamicFanSpeed>255)dynamicFanSpeed=255;
        
        if( dynamicFanSpeed>slidingMaxFanSpeed || time()>maxFanSpeedTime+FAN_PEAK_HOLD_TIME ){ //update max values if max is overcome or if the time (FAN_PEAK_HOLD_TIME) is reached
            slidingMaxFanSpeed=dynamicFanSpeed;
            maxFanSpeedTime=time();
        }
        cout<<"T:"<<temp<<"°C";
        if(lastFanSpeed!=slidingMaxFanSpeed || lastTimeFanUpdate+10000 < time()){ //send value if it changed or if we didn't do it since more than 10s.
            setFanSpeed(slidingMaxFanSpeed);
            lastTimeFanUpdate=time();
            cout<<" set fan to "<<round((float)(slidingMaxFanSpeed)/255*100)<<"%";
        }
        cout<<endl;
        lastFanSpeed=slidingMaxFanSpeed;
        usleep(500000);
    }
    return 0;
}
