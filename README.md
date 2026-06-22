## Overview
Embedded Sentry is a hardware-based security system that utilizes physical gestures as unlock keys. Users can record a specific motion using the microcontroller and subsequently use that same gesture to bypass the security lock.

## Usage & Application Flow
1. **Initialization:** Click the blue button to start the device.
2. **Recording:** Long press and release the same button to record a new gesture. 
3. **Unlocking:** Click the same button again and perform the gesture to attempt an unlock.
4. **Validation:**
   * **Success:** If the gesture is correct and falls under the accepted threshold, a success screen and key are shown.
   * **Failure:** If the gesture is wrong, a failure screen is displayed.

## Tech Stack & Hardware
* **Language:** C/C++
* **Hardware:** STMicroelectronics Discovery Board (STM32)
* **Components:** On-board accelerometer/gyroscope, LCD display, and push buttons.
