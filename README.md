# DALI-master-controller-using-esp32

More details are available here: https://github.com/Spectoda/DALI-Lighting-Interface-ESP32. Special thanks to the author for their valuable contributions. This repository is based on the link provided and is developed using the Arduino IDE to add multiple LED drivers to one group simultaneously.

USAGE (on Serial Monitor):
The ESP32 takes input commands from the Serial Monitor. The following commands are supported:

- 1: Blink all lamps
- 2: Scan short addresses
- 3: Commission short addresses
- 4: Delete short addresses
- 5: Read memory bank
- 6: Set group
If choosing '6', the frame format is: '6 ab cd ef'. Meaning:
  - '6': Select set_group mode
  - ' ': Any character (e.g., space = ' ')
  - 'ab': Short address for the first lamp in the series [0..63]
  - 'cd': Number of lamps in the series [1..63]
  - 'ef': Group selection [0..15]
Example: To add ten lamps with addresses 1 to 10 to group 15, send the serial command '6 01 10 15'.

![image](https://github.com/user-attachments/assets/0d5119ee-5054-4e6e-8803-19af8c54a970)

