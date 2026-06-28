# Assignment-2-C-Language-MOTION-DETECTION-SYSTEM
Assignment 2 - Group C Language 
Members: 
1. OSAMA MOHAMMED SIDDIG MOHAMMED
2. Abdelrahman Mohamed Saad Said
3. Abdelaziz Alsayed Abdou Alsayed Mahmoud
##  Task Division

###  OSAMA MOHAMMED SIDDIG MOHAMMED – Sensor & Counting
- Implemented **HC-SR04 ultrasonic sensor logic** (`measureDistance` function).
- Added detection condition in `main()` to increment the counter when an object is detected.
- Responsible for **distance measurement and detection threshold logic**.

###  Abdelrahman Mohamed Saad Said – initializing Display & showing count on TM1637
- Implemented **TM1637 display driver** (`TM1637_Init`, `TM1637_WriteByte`, `updateDisplay`).
- Responsible for showing the **person count** on the 4-digit display.
- Ensured proper brightness and segment mapping.

###  Abdelaziz Alsayed Abdou Alsayed Mahmoud – Indicators & System
- Configured **GPIO pins** for LEDs and buzzer (`MX_GPIO_Init`).
- Implemented **System Clock Configuration** (`SystemClock_Config`).
- Responsible for **visual and audio feedback** (LEDs + buzzer) and system setup.

- ## 📎 GitHub Link
[Project Repository](https://github.com/Osama-4/Assignment-2-C-Language-MOTION-DETECTION-SYSTEM/tree/main)
