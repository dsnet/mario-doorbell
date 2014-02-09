# Mario Themed Doorbell #

## Introduction ##

Who wants a boring old doorbell that simply goes "ding dong" when pressed?
I sure didn't! That's why I decided to design my own doorbell that provides
ample entertainment for the guest at the door. Now, introducing the new
Mario Doorbell; it's flashy, it's got coins, and it's guaranteed to increase the
frequency of ding-dong ditchers or your money back! In fact, your guests won't
even be at your door to visit you, they'll be too busy racking up coins to even
notice you.

[![door-button](http://code.digital-static.net/mario-doorbell/raw/tip/media/door-button.jpg)](http://www.youtube.com/watch?v=j20RfiTt6zI)

Basically, every time a guest presses the coin button, the counter displayed
will increment and the coin sound from Mario will be played. As a special
bonus, every 10 coins will cause the life-up sound to be played, and every 100
coins will cause the mushroom upgrade sound to be played. Technically, the
life-up sound should have been for every 100 coins, but the mushroom upgrade
sound was so obnoxiously loud and long, that I switched those two clips.


## Implementation ##

The doorbell system was split into two separate sub-projects. The first is the
door button that sits outside the apartment and is responsible for counting the
coins and transmitting which sound clip to play over UART. The second is the
door ringer that sits inside the apartment and is responsible for listening for
the sound clip to play and to generate the audio signal for the selected sound
clip. The interaction between the two sub-projects is as shown:

![diagram](http://code.digital-static.net/mario-doorbell/raw/tip/doc/diagram_lite.png)

The door button basically comprises of a PIC16F628A microcontroller connected to
a dual 7-segment LED display. I tried to keep the costs of this portion as low
as possible in the unfortunate event that someone steals my precious doorbell.
The ugliness of circuit board and electronics is masked by a Mario themed decal
shown below. The image of the coin itself is the button that guests press.

![decal](http://code.digital-static.net/mario-doorbell/raw/tip/media/decal.png)

The door ringer is built on a PIC16F687 microcontroller connected to a MCP4822
DAC and a 25LC1024 EEPROM. When the MCU receives a signal over UART from the
button, it will play the sound clip by reading sample bytes out of the EEPROM
chip and feed it into the DAC chip. The door ringer does not have any speakers,
so it is necessary to plug an external speaker into the audio jack port on the
circuit board.


## File Structure ##

* **board**: Circuit board schematics or PCB layouts
* **doc**: Documentation related files
* **media**: Multimedia files such as photographs or decals
* **mikroc**: C sub-projects targeted at the microcontroller realm
* **mikroc/door_button**: Project for controlling the door button
* **mikroc/door_ringer**: Project for playing sound samples as the door ringer
* **mikroc/hex_convert**: Program to convert Wave files to EEPROM hex dump

