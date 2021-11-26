// Communicate with embedded controller.
#include <sys/io.h>
// For generating time to sleep.
#include <chrono>
// For sleeping.
#include <thread>
// For outputing data.
#include <iostream>
// For min and max calculations.
#include <math.h>
#include <string>


// Save current pragma diagnostic state.
#pragma clang diagnostic push
// Ignore endless loop.
#pragma ide diagnostic ignored "EndlessLoop"

// Embedded controller command port.
#define EC_COMMAND_PORT         0x66
// Embedded controller data port.
#define EC_DATA_PORT            0x62

// Command to get temperature.
#define COMMAND_TEMP            0x9E
// Command to set speed.
#define COMMAND_SPEED           0x99

// Temperature index.
#define TEMP_INDEX              1

// Id for fan.
#define FAN_ID                  0x01

// Min speed as percentage to run the fan.
#define FAN_SPEED_MIN           30
// Max speed as percentage to run the fan.
#define FAN_SPEED_MAX           100

// Minimum temperature from start to raise fan speed.
#define MIN_TEMP                70
// Maximun temperature that starts raising fan speed even if temperature is not raising.
#define MAX_TEMP                85

// Wait time between loops.
#define REFRESH_RATE            250
// Wait this many milliseconds to change speed when temperature is raising.
#define REACTION_TIME_MS_RAISE  1000
// Wait this many milliseconds to change speed when temperature is lowering or staying same.
#define REACTION_TIME_MS_LOWER  2000

// Increment as percentage to raise fan speed.
#define FAN_RAISE_INCREMENT     5
// Increment as percentage to lower fan speed.
#define FAN_LOWER_INCREMENT     1


/**
 * @brief Initialize embedded controller.
 * 
 */
static void EcInit()
{
    // Request permission to embedded controller's data port.
    if (ioperm(EC_DATA_PORT, 1, 1) != 0)
        throw std::runtime_error("No permission to embedded controller's data port!");
    
    // Request permission to embedded controller's command port.
    if (ioperm(EC_COMMAND_PORT, 1, 1) != 0)
        throw std::runtime_error("No permission to embedded controller's command port!"); 
}

/**
 * @brief Flush embedded controller.
 * 
 */
static void EcFlush()
{
    while ((inb(EC_COMMAND_PORT) & 0x1) == 0x1)
        inb(EC_DATA_PORT);
}


/**
 * @brief Send command to embedded controller.
 * 
 * @param command Command to send.
 */
static void SendCommand(int command)
{
    int tt = 0;
    while((inb(EC_COMMAND_PORT) & 2))
    {
        tt++;
        if(tt>30000)
            break;
    }
    outb(command, EC_COMMAND_PORT);
}

/**
 * @brief Write data to embedded controller.
 * 
 * @param data Data to write.
 */
static void WriteData(int data)
{
    while((inb(EC_COMMAND_PORT) & 2));

    outb(data, EC_DATA_PORT);
}

/**
 * @brief Read byte from embedded controller.
 * 
 * @return int Read byte.
 */
static int ReadByte()
{
    int i = 1000000;
    while (
        (
            inb(EC_COMMAND_PORT)
            &
            1
        ) == 0
        &&
        i > 0
    ) i -= 1;

    if (i == 0)
        return 0;
    else
        return inb(EC_DATA_PORT);
}

/**
 * @brief Set fan speed to given value.
 * 
 * @param speed Fan speed as percentage.
 */
static void setFanSpeed(unsigned int speed)
{
    // Initialize embedded controller.
    EcInit();
    // Send command to change speed.
    SendCommand(COMMAND_SPEED);
    // Set fan id.
    WriteData(FAN_ID);
    // Write speed as value from 0-255.
    WriteData(
        round(
            (
                std::min(
                    (float)speed,
                    (float)100
                )
                /
                100
            )
            *
            255
        )
    );
}

/**
 * @brief Get the local temperature.
 * 
 * @return int Temperature.
 */
static int GetLocalTemp()
{
    // Initialize embedded controller.
    EcInit();
    // Flush embedded controller.
    EcFlush();
    // Send command to get temperature.
    SendCommand(COMMAND_TEMP);
    // Write temperature index.
    WriteData(TEMP_INDEX);
    // Return with temperature.
    return ReadByte();
}

/**
 * @brief Main function.
 * 
 * @param argc Argument count.
 * @param argv Argument table.
 * @return int 
 */
int main (int argc, char *argv[])
{
    // Init verbose status.
    bool verbose = false;

    // Generate verbose arguments.
    const unsigned int verbose_strs_count = 2;
    const std::string verbose_strs[
        verbose_strs_count
    ] = {
        std::string("-v"),
        std::string("--verbose"),
    };

    // Check for verbose argument.
    for(int i = 0; (!verbose && i < argc); ++i)
    {
        for(int y = 0; (!verbose && y < verbose_strs_count); ++y)
        {
            verbose = (
                argv[i] == verbose_strs[y]
            );
        }
    }

    // Speed of fan when last set.
    unsigned int lastFanSpeed = 0;
    // Set to this fan speed.
    unsigned int fanSpeed = FAN_SPEED_MIN;
    // Set fan speed.
    setFanSpeed(fanSpeed);


    // Sleep micro second amount.
    const std::chrono::duration<int64_t, std::milli> sleep_time = std::chrono::milliseconds(
        REFRESH_RATE
    );
    // Wait this many loops to change speed when temperature is raising.
    const unsigned int reaction_loops_raise = REACTION_TIME_MS_RAISE/REFRESH_RATE;
    // Wait this many loops to change speed when temperature is lowering or staying same.
    const unsigned int reaction_loops_lower = REACTION_TIME_MS_LOWER/REFRESH_RATE;

    // Temperature.
    unsigned int temp = GetLocalTemp();
    // Last loop temperature.
    unsigned int lastTemp = GetLocalTemp();

    // How many loops has temperature been raising.
    unsigned int raising_loops = 0;
    // How many loops has temperature been lowering or staying the same.
    unsigned int lowering_or_staying_loops = 0;
    
    // Infinite loop.
    while(true)
    {
        // Get temperature.
        temp = GetLocalTemp();
        // Print out temperature.
        if(verbose)
            std::cout << "Temperature: " << temp << " C" << std::endl << "\t";

        // If
        if(
            // temperature is over the max.
            MAX_TEMP < temp
            ||
            (
                // or min temperature has been reached
                MIN_TEMP < temp
                // and temperature is raising
                &&
                0 < (int)(
                    temp
                    -
                    lastTemp
                )
            )
        )
        {
            // Output information about it.
            if(verbose)
                std::cout << "Raising or over max!";
            // Increase raising loops.
            ++raising_loops;

            // If has been raising more than reaction time gives.
            if(reaction_loops_raise < raising_loops)
            {
                // Raise fan speed by defined increment.
                fanSpeed = std::min(
                    (int)(
                        fanSpeed
                        +
                        FAN_RAISE_INCREMENT
                    ),
                    FAN_SPEED_MAX
                );

                // Zero loop counters.
                lowering_or_staying_loops = 0;
                raising_loops = 0;
            }
        }
        // Fan speed is not raising or over the max.
        else
        {
            // Inform about it.
            if(verbose)
                std::cout << "Lowering or staying the same.";
            // Increase loop counter.
            ++lowering_or_staying_loops;

             // If has been lowering or staying the same, more than reaction time gives.
            if(reaction_loops_lower < lowering_or_staying_loops)
            {
                // Lower fan speed by defined increment.
                fanSpeed = std::max(
                    (int)(
                        fanSpeed
                        -
                        FAN_LOWER_INCREMENT
                    ),
                    FAN_SPEED_MIN
                );

                // Zero loop counters.
                lowering_or_staying_loops = 0;
                raising_loops = 0;
            }
        }
        if(verbose)
            std::cout << std::endl << "\t";

        // If fan speed changed.
        if(fanSpeed != lastFanSpeed)
        {
            // Output about the change.
            if(verbose)
                std::cout << "Changing fan speed " << lastFanSpeed << " % => " << fanSpeed << " %!";

            // Zero loop counters.
            lowering_or_staying_loops = 0;
            raising_loops = 0;

            // Set fan speed.
            setFanSpeed(
                fanSpeed
            );

            // Save fan speed as last fan speed.
            lastFanSpeed=fanSpeed;
            // Update last loop temperature from this loop temperature.
            lastTemp = temp;
        }
        // Fan speed did not change.
        else if(verbose)
            std::cout << "Keeping fan speed at " << fanSpeed << " %";
        if(verbose)
            std::cout << std::endl;

        // Sleep set micro seconds.
        std::this_thread::sleep_for(sleep_time);
    }

    // Shout not reach here.
    return 0;
}
