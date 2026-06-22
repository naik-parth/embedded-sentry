// Embedded Challenge: Embedded_Sentry 
// Group no.: 96

// Member 1: Aryan Mamidwar
// net. id.: arm9337

// Member 2: Ilka Jean
// net. id.: ifj2007

// Member 3: Parth Naik
// net. id.: pn2325

#include <mbed.h>
#include "drivers/LCD_DISCO_F429ZI.h"
#include "stm32f4xx.h"
#include <cmath>
#include <vector>
#include <limits>

// --- Constants and Definitions ---
// Gyroscope registers (X, Y, Z axis)
#define I3G4250D_OUT_X_L 0x28  // Output data X low byte
#define I3G4250D_OUT_X_H 0x29  // Output data X high byte
#define I3G4250D_OUT_Y_L 0x2A  // Output data Y low byte
#define I3G4250D_OUT_Y_H 0x2B  // Output data Y high byte
#define I3G4250D_OUT_Z_L 0x2C  // Output data Z low byte
#define I3G4250D_OUT_Z_H 0x2D  // Output data Z high byte

//Gyrosope Control Registers
#define CTRL_REG1 0x20                  // Address of Control Register 1
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1 // Configuration for enabling gyroscope and setting data rate
#define CTRL_REG4 0x23                  // Address of Control Register 4
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0 // Configuration for setting full-scale range
#define CTRL_REG3 0x22                  // Address of Control Register 3
#define CTRL_REG3_CONFIG 0b0'0'0'0'1'000 // Enable data-ready interrupt

// Define the address to read the X-axis lower data
#define OUT_X_L 0x28 // Address of the gyroscope's X-axis lower byte data register

// Gyroscope scaling factor
#define GYRO_SCALE_FACTOR (17.5f * 0.017453292519943295769236907684886f / 1000.0f)

// Recording duration in ms
#define RECORDING_DURATION 2000

// SPI transfer and interrupt flags
#define SPI_FLAG 1
#define DATA_READY_FLAG 2 // Event flag for data-ready interrupt

// Samples per second
#define SAMPLES_PER_SECOND 100

// EventFlags object to synchronize asynchronous SPI transfers
EventFlags flags;

// --- SPI Transfer Callback Function ---
// Called automatically when an SPI transfer completes
void spi_cb(int event) {
    flags.set(SPI_FLAG);  // Set the SPI_FLAG to signal that transfer is complete
}

// Interrupt callback for the data-ready signal
void data_cb()
{
    // Set the DATA_READY_FLAG to signal the main thread
    flags.set(DATA_READY_FLAG);
}

// LCD
LCD_DISCO_F429ZI lcd;

// BUTTON for recording and entering key
DigitalIn button(PA_0);

// LED for indicating success
DigitalOut led(LED1);

// SPI initialization
SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel); // MOSI, MISO, SCK (you can change these pins as needed)

// Buffers for SPI data transfer
uint8_t write_buf[32], read_buf[32];

// Function prototypes
// --- Initialization Functions ---
void gyroscope_init(void) {
    spi.format(8, 3); // 8-bit data size, SPI mode 3
    spi.frequency(1'000'000); // SPI clock frequency set to 1 MHz

    // Configure CTRL_REG1 to enable gyroscope and set data rate
    write_buf[0] = CTRL_REG1;           // Register address
    write_buf[1] = CTRL_REG1_CONFIG;    // Configuration value
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb); // Perform SPI transfer
    flags.wait_all(SPI_FLAG);           // Wait for SPI transfer completion
    printf("CTRL_REG1 config write: 0x%02X\n", read_buf[1]);

    // Configure CTRL_REG4 to set full-scale range
    write_buf[0] = CTRL_REG4;           // Register address
    write_buf[1] = CTRL_REG4_CONFIG;    // Configuration value
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb); // Perform SPI transfer
    flags.wait_all(SPI_FLAG);  
    printf("CTRL_REG4 config write: 0x%02X\n", read_buf[1]);

    // Configure CTRL_REG3 to enable data-ready interrupt
    write_buf[0] = CTRL_REG3;           // Register address
    write_buf[1] = CTRL_REG3_CONFIG;    // Configuration value
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb); // Perform SPI transfer
    flags.wait_all(SPI_FLAG);           // Wait for SPI transfer completion
    printf("CTRL_REG3 config write: 0x%02X\n", read_buf[1]);
}
/* 
// A Caliberate function was created to caliberate the gyro before each use but it did not work in time 
void calibrate_gyroscope() {
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    const int num_samples = 100;  // Number of samples for calibration

    lcd.Clear(LCD_COLOR_YELLOW);
    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"CALIBRATING", CENTER_MODE);
    delay_ms(1000);

    for (int i = 0; i < num_samples; ++i) {
        uint16_t x, y, z;
        gyroscope_read(&x, &y, &z);

        sum_x += x;
        sum_y += y;
        sum_z += z;

        delay_ms(10);  // Delay between samples
        
    }

    // Calculate the average for each axis
    offset_x = sum_x / num_samples;
    offset_y = sum_y / num_samples;
    offset_z = sum_z / num_samples;

    //printf("Calibration complete. Offsets: X=%d, Y=%d, Z=%d\n", offset_x, offset_y, offset_z);
    // Add a delay after calibration is done
    lcd.Clear(LCD_COLOR_GREEN);
    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"CALIBRATION DONE", CENTER_MODE);
    delay_ms(2000);  // Add a 2-second delay after calibration
} */

// --- Reading Function ---
void gyroscope_read(uint16_t *x, uint16_t *y, uint16_t *z) 
{
    uint16_t raw_gx, raw_gy, raw_gz;

    // Initiate read for X, Y, Z axes (6 bytes)
    write_buf[0] = OUT_X_L | 0x80 | 0x40;
    spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
    flags.wait_all(SPI_FLAG);

    // Combine the high and low bytes for X, Y and Z axis
    raw_gx = (((uint16_t)read_buf[2]) << 8) | ((uint16_t)read_buf[1]);
    raw_gy = (((uint16_t)read_buf[4]) << 8) | ((uint16_t)read_buf[3]);
    raw_gz = (((uint16_t)read_buf[6]) << 8) | ((uint16_t)read_buf[5]);  
    //printf("[DEBUG] Raw Gyro Data: X=%d, Y=%d, Z=%d\n", raw_gx, raw_gy, raw_gz);

    *x = raw_gx * GYRO_SCALE_FACTOR;
    *y = raw_gy * GYRO_SCALE_FACTOR;
    *z = raw_gz * GYRO_SCALE_FACTOR;
    //printf("[DEBUG] Scaled Gyro Data: X=%f, Y=%f, Z=%f\n", *x, *y, *z);
}

// --- Delay function ---
void delay_ms(uint32_t ms) 
{
    HAL_Delay(ms);
}

// --- Gyroscope debugging function ---
void debug_gyro_registers() 
{
    write_buf[0] = 0x20 | 0x80 | 0x40;  // Start reading from CTRL_REG1
    spi.transfer(write_buf, 6, read_buf, 6, spi_cb);
    flags.wait_all(SPI_FLAG);

    printf("CTRL_REG1: 0x%02X \n", read_buf[1]);
    printf("CTRL_REG2: 0x%02X \n", read_buf[2]);
    printf("CTRL_REG3: 0x%02X \n", read_buf[3]);
    printf("CTRL_REG4: 0x%02X \n", read_buf[4]);
}

// --- Dynamic Time Warping Algorithm ---
int dynamic_time_warping(const std::vector<int> &seq1, const std::vector<int> &seq2) 
{
    // Get size of both the sequences
    size_t n = seq1.size();
    size_t m = seq2.size();

    int INF = std::numeric_limits<int>::max(); 
    std::vector<std::vector<int>> dtw(n + 1, std::vector<int>(m + 1, INF));
    dtw[0][0] = 0;

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            int cost = fabs(seq1[i - 1] - seq2[j - 1]);
            dtw[i][j] = cost + std::min({dtw[i - 1][j], dtw[i][j - 1], dtw[i - 1][j - 1]});
        }
    }
    return dtw[n][m]; // Return the local distance
}

void drawSmileyFace()
{
    // Set color and draw the face (Yellow Circle)
    lcd.SetTextColor(LCD_COLOR_YELLOW);
    lcd.FillCircle(120, 240, 35);  

    // Set color and draw the eyes (Blue Circles)
    lcd.SetTextColor(LCD_COLOR_BLUE);
    lcd.FillCircle(100, 220, 5);   // Left eye
    lcd.FillCircle(140, 220, 5);   // Right eye

    // Draw the smiling mouth: simulate with multiple small circles (Arc)
    lcd.SetTextColor(LCD_COLOR_BLUE);
    for (int i = 0; i < 30; ++i) {
        int x = 120 + 20 * cos(3.14 * i / 30);  // X coordinate for the mouth arc
        int y = 240 + 10 * sin(3.14 * i / 30);  // Y coordinate for the mouth arc (raised)
        lcd.FillCircle(x, y, 2);  // Small circles forming the smile arc
    }
}

void drawSadFace()
{
    // Set color and draw the face (Yellow Circle)
    lcd.SetTextColor(LCD_COLOR_YELLOW);
    lcd.FillCircle(120, 240, 35);  

    // Set color and draw the eyes (Black Circles)
    lcd.SetTextColor(LCD_COLOR_BLACK);
    lcd.FillCircle(100, 220, 5);   // Left eye
    lcd.FillCircle(140, 220, 5);   // Right eye

    // Draw the sad mouth: simulate with multiple small circles (Inverted Arc)
    lcd.SetTextColor(LCD_COLOR_BLACK);
    for (int i = 0; i < 30; ++i) {
        int x = 120 + 20 * cos(3.14 * i / 30);  // X coordinate for the sad arc
        int y = 240 - 10 * sin(3.14 * i / 30);  // Y coordinate for the sad arc (flipped)
        lcd.FillCircle(x, y, 2);  // Small circles forming the sad arc
    }
}

// Main program
int main() 
{
    InterruptIn int2(PA_2, PullDown);   // Initialize INT2 pin with pull-down resistor
    int2.rise(&data_cb);                // Attach the data-ready callback to the rising edge of INT2
    gyroscope_init();
    //debug_gyro_registers(); //Command used to debug the gyrpsope 
    lcd.DisplayOn();
    lcd.Clear(LCD_COLOR_WHITE);

    std::vector<int> recorded_x, recorded_y, recorded_z; // Vector to store recorded gyrocope data
    bool key_recorded = false; // Flag to indicate if the key has been recorded

    while (1) {
        if (button.read() == 1) { // Check if button is pressed
            uint32_t press_time = HAL_GetTick(); // Record the time button is pressed

            while (button.read() == 1) {} // Wait for button release

            if ((HAL_GetTick() - press_time) >= 2000) {
                lcd.Clear(LCD_COLOR_ORANGE);
                lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"RECORDING", CENTER_MODE);
                delay_ms(1000);

                // Clear the previous data and start recording gyroscope samples
                recorded_x.clear();
                recorded_y.clear();
                recorded_z.clear();

                for (int i = 0; i < RECORDING_DURATION / (1000 / SAMPLES_PER_SECOND); ++i) {
                    uint16_t x, y, z;
                    gyroscope_read(&x, &y, &z);
                    recorded_x.push_back(x);
                    recorded_y.push_back(y);
                    recorded_z.push_back(z);
                    delay_ms(1000 / SAMPLES_PER_SECOND); // Delay to match the sample rate
                }

                key_recorded = true; // Set the key_recorded flag
                lcd.Clear(LCD_COLOR_DARKBLUE);
                lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"RECORDING COMPLETE", CENTER_MODE);
                lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"CLICK TO UNLOCK", CENTER_MODE);
                delay_ms(1000);
            } else {
                if (!key_recorded) {
                    lcd.Clear(LCD_COLOR_MAGENTA);
                    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"NO KEY RECORDED", CENTER_MODE);
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"LONG PRESS", CENTER_MODE);
                    lcd.DisplayStringAt(0, LINE(8), (uint8_t *)"TO RECORD KEY", CENTER_MODE);
                    delay_ms(1000);
                    continue;
                }

                lcd.Clear(LCD_COLOR_WHITE);
                lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"ENTER KEY", CENTER_MODE);
                delay_ms(1000);

                // Record a new gesture for comparison 
                std::vector<int> entered_x, entered_y, entered_z;

                for (int i = 0; i < RECORDING_DURATION / (1000 / SAMPLES_PER_SECOND); ++i) {
                    uint16_t x, y, z;
                    gyroscope_read(&x, &y, &z);
                    entered_x.push_back(x);
                    entered_y.push_back(y);
                    entered_z.push_back(z);
                    delay_ms(1000 / SAMPLES_PER_SECOND);
                }

                // Calculate DTW distances
                int dist_x = dynamic_time_warping(recorded_x, entered_x);
                int dist_y = dynamic_time_warping(recorded_y, entered_y);
                int dist_z = dynamic_time_warping(recorded_z, entered_z);

                // Compare the distances to the threshold
                int threshold = 350.0f;
                //printf("[DEBUG] Recorded Gesture Sample: X=%f, Y=%f, Z=%f\n", recorded_x, recorded_y, recorded_z);
                //printf("[DEBUG] Input Gesture Sample: X=%f, Y=%f, Z=%f\n", entered_x, entered_y, entered_z);
                printf("[DEBUG] Distance: X=%d, Y=%d, Z=%d\n", dist_x, dist_y, dist_z);
                if (dist_x < threshold && dist_y < threshold && dist_z < threshold) {
                    lcd.Clear(LCD_COLOR_GREEN);
                    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"UNLOCK SUCCESSFUL", CENTER_MODE);
                    drawSmileyFace(); // Display a simley face
                    led = 1;
                    delay_ms(2000);
                    led = 0;
                } else {
                    lcd.Clear(LCD_COLOR_RED);
                    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"UNLOCK FAILED", CENTER_MODE);
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"CLICK TO RETRY", CENTER_MODE);
                    drawSadFace(); // Display a sad face
                    delay_ms(2000);
                }
            }
        }
    }
}
