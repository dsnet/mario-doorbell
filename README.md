# Mario Themed Doorbell #

## Introduction ##

Who wants a boring old doorbell that simply goes "ding dong" when pressed?
I sure didn't! That's why I decided to design my own doorbell that provides
ample entertainment for the guest at the door. Now, introducing the new
Mario Doorbell; it's flashy, it's got coins, and it's guaranteed to increase the
frequency of ding-dong ditchers or your money back! In fact, your guests won't
even be at your door to visit you, they'll be too busy racking up coins to even
notice you.

[![chart-error](http://code.digital-static.net/mario-doorbell/raw/tip/media/door-button.jpg)](http://www.youtube.com/watch?v=j20RfiTt6zI)

Basically, every time a guest presses the coin button, the counter displayed
will increment and the coin sound from Mario will be played. As a special
bonus, every 10 coins will cause the life-up sound to be played, and every 100
coins will cause the mushroom upgrade sound to be played. Technically, the
life-up sound should have been for every 100 coins, but the mushroom upgrade
sound was so obnoxiously loud and long, that I switched those two clips.


## Implementation ##


## File Structure ##

* **board**: Circuit board schematics or PCB layouts
* **doc**: Documentation related files
* **media**: Multimedia files such as photographs or decals
* **mikroc**: C sub-projects targeted at the microcontroller realm
* **mikroc/door_button**: Project for controlling the door button
* **mikroc/door_ringer**: Project for playing sound samples as the door ringer
* **mikroc/hex_convert**: Program to convert Wave files to EEPROM hex dump

