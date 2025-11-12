# About

[Ler em Português](https://dirceu-jr.github.io/anga/README.pt-BR)

<img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/nature_people_FILL0_wght400_GRAD0_opsz48.svg" align="middle"> <img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/water_FILL0_wght400_GRAD0_opsz48.svg" align="middle"> <img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/antenna_FILL0_wght400_GRAD0_opsz48.svg" align="middle">

I live in [Curitiba city](https://en.wikipedia.org/wiki/Curitiba), some say, _the most ecological Brazilian city_.

Urban socio-environmental problems identified in Curitiba includes air pollution, water pollution and environmental degradation.

This project is developed with the objective of creating <strong>low-cost</strong> monitoring stations, enabling the creation of a dense monitoring network with the capacity to locate occurrences of air pollution and contamination.

With community and educational purposes in mind, information about suppliers, configurations, and software are all open and open-source.

# Status

There are two prototypes ready to be tested. One with CO, NO2 and PM2.5 sensors. Another with Bosch BME688 Air Quality sensor. Both will send readings/data to ThingSpeak over a cellular network (NB-IoT).

This is the prototype with NO2, PM2.5 and CO sensors:

<img width="320" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/1758142216272.jpg" alt="Prototype">

# Hardware

This is the probably hardware that will be used:

- Mainboard: [LILYGO® TTGO T-SIM7000G](https://pt.aliexpress.com/item/4000542688096.html) (NB-IoT)
- Or: [LILYGO® TTGO T-A7670G](https://pt.aliexpress.com/item/1005003036514769.html) (Cat-1)

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/lilygo-t-sim7000g.webp" alt="LILYGO® TTGO T-SIM7000G">

- <a href="https://www.dfrobot.com/product-2508.html">DFRobot CO (and potentially NO2 and other gases)</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/co.avif" alt="CO sensor">

- <a href="https://www.dfrobot.com/product-2871.html">Gravity: BME688</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/BME688.avif" alt="BME688 Air Quality">

- <a href="https://www.dfrobot.com/product-2439.html">Gravity: PM2.5 Air Quality Sensor</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/pm25.webp" alt="PM2.5 Air Quality Sensor">

# Real-Time Data

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/1?title=CO&width=1&height=1"></iframe>

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/2?title=NO2&width=1&height=1"></iframe>

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/4?title=PM2.5&width=1&height=1"></iframe>
