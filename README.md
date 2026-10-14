# **HapticEMGTrainer**

This is a simple control system that uses a MyoWare2.0 EMG sensor that triggers a haptic motor upon a muscle signal event and measures the difference of time between the event and when the capacitive button is pressed.

# **Getting Started and Information**

This project runs on an ESP32 dev board, with a MyoWare 2.0 sensor and an open jumper cable that acts as a capacitive button. When a muscle event occurs the system will trigger the haptic motor briefly and wait for the user to touch the capacitive button. The program will tell you the difference in time between the muscle event and the touch. This is intended to help the user to train their reaction time. This project is for demo purposes only and is still in development. 

## **Prerequisites**

Ensure you have the following software installed on your development machine:

* Arduino IDE or PlatformIO  
* ESP32 Dev Kit  
* MyoWare 2.0 EMG Sensor  
* H-Bridge  
* Haptic Motor  
* Jumper cables and alligator clips

# **Usage**

To set up the system you will need to wire the ESP32 to an H-Bridge driver to connect the haptic motor. For this simple implementation, single jumper cable wired to a capacitive touch capable pin. The MyoWare 2.0 shield should be wired to a 3.3V output from the ESP32. 

The Myoware should be placed on your flexor carpi radialis which is used for opening and closing your hand.

Once the code is flashed to your ESP, you will need to calibrate the touch sensitivity as well as the EMG signal sensitivity. Balancing these sensitivities will help to ensure the loop runs appropriately. 

\*\*\*Note\*\*\*   
Do not attach EMG sensors should never be used on a circuit that is connected to 120V power. When running this program your laptop should not be plugged in. 