# DALI-master-controller-using-esp32

More details in here https://github.com/Spectoda/DALI-Lighting-Interface-ESP32. Special thanks to the author for their valuable contributions.
This repository is based on this link and is developed on Arduino IDE for adding some LED drivers to one group in one time.

USAGE (on serial monitor)
ESP32 will take input command from Serial monitor. It works normally if you press from '1' to '5'.
  - 1 Blink all lamps
  - 2 Scan short addresses
  - 3 Commission short addresses
  - 4 Delete short addresses
  - 5 Read memory bank
  - 6 Set group ... If choosing '6', the frame be : '6 ab cd ef'. Meaning:
    + '6': choose set_group mode
    + ' ': it could be any character ( i used space = ' ')
    + 'ab': the short address for the first lamp in the series of lamps [0..63]
    + 'cd': The number of series of lamps [1..63]
    + 'ef': Choose group [0..15]

Example: I want to add ten lamps to group 15, which has addresses 1 to 10. I will send the serial command '6 01 10 15'

![image](https://github.com/user-attachments/assets/0d5119ee-5054-4e6e-8803-19af8c54a970)

