# ESP_AquariumServer

## Web
Standalone esp8266 server built to control the lighting time in aquariums, where the light intensity and duration is critical for a healthy plant growth.

<img src="pictures\webpage_screenshot.jpg" alt="Website screenshot" style="width: 500px;"/>

The  served dashboard, based on Materialize CSS, plain JS and Jquery, is used to display a light_intensity over time graph and to adjust the parameters.

Light curve is based on a gaussian function parametrized by three variables:
 
 - Time: Time range when the light should be turned on and of
 - Power: The maximum ouput power ever used by the controler
 - Floor: The power the light should be when it turns on/of. Also controls how smooth the curve gets

The time is automatically mantained via a ntp server and the configs are saved on flash every time you hit the `STORE` button.

## Hardware 

The only harwdare you need is:

- 1 esp8266
- 1 DS3231 RTC

The output is up to you to hook up to whatever you need. The PWM pins related to the color data shown on the web server is configured through these two assignments:

``` C++
//main.cpp
//In this case, D5 is mapped to "ww" and  D6 to "cw"
int color_pins[color_number] = {D5, D6};

char color_names[color_number][3] = {"ww", "cw"};

```

In my case, I used one logic level N-Channel mosfet to drive a 12v LED strip for each channel, getting a connection diagram like this

<img src="pictures\simple_schematic.jpg" alt="Website screenshot" style="width: 500px;"/>


## Enjoy

Now you can just enjoy your time without having to worry about the photoperiod of your plants:

<img src="pictures\aquarium result.jpg" alt="Website screenshot" style="width: 500px;"/>


