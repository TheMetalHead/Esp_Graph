# Esp_Graph
A very simple oscilloscope using a Wemos Lolin D1 mini and 1.44" TFT display.

It has a full scale display of 0 to 3.3 volts and a screen refresh time of 1.28 to 8.96 seconds. The default is 5.12 seconds.

I used it to fault find the narrow band lambda sensor on my car.

Do not ask me for any help in diagnosis, instead I will direct you to the excellent website: https://secure.lambdapower.co.uk/default.asp that contains a wealth of information on oxygen sensors.

Only two connections are required. Ground and A0.

The maximum input voltage is 3.3 volts and has no proper overvoltage protection. It should  withstand a brief connection to 13.8 volts as it has a 220K ohm series input resistor.

# Parts used:
https://www.wemos.cc/en/latest/d1/d1_mini_3.1.0.html

https://www.wemos.cc/en/latest/d1_mini_shield/tft_1_4.html

https://www.wemos.cc/en/latest/d1_mini_shield/protoboard.html

![This is an image](https://github.com/TheMetalHead/Esp_Graph/blob/master/d1_mini_v3.1.0_1_16x16.jpg)
