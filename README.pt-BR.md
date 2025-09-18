# Sobre

[Read in English](https://dirceu-jr.github.io/anga/)

<img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/nature_people_FILL0_wght400_GRAD0_opsz48.svg" align="middle"> <img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/water_FILL0_wght400_GRAD0_opsz48.svg" align="middle"> <img src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/antenna_FILL0_wght400_GRAD0_opsz48.svg" align="middle">

I live in [Curitiba city](https://en.wikipedia.org/wiki/Curitiba), some say _the most ecological Brazilian city_.
Eu more em [Curitiba](https://en.wikipedia.org/wiki/Curitiba), a cidade brasileira mais ecológica.

Os problemas socioambientais urbanos identificados em Curitiba incluem poluição do ar, poluição da água e degradação ambiental.

Este projeto é desenvolvido com o objetivo de criar estações de monitoramento de <strong>baixo custo</strong>, permitindo a criação de uma rede densa de monitoramento com capacidade de localizar ocorrências de poluição e contaminação do ar.

Com propósitos comunitários e educacionais em mente, as informações sobre fornecedores, configurações e software são todas abertas e de código aberto.

# Status

Os protótipos estão enviando leituras/dados de CO, NO2 e BME688 para o ThingSpeak por meio de uma rede celular (NB-IoT).

Este é o protótipo com sensores de NO2, PM2,5 e CO instalados:

<img width="320" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/1758142216272.jpg">

# Hardware

Este é o hardware que provavelmente será usado:

- Placa-mãe: [LILYGO® TTGO T-SIM7000G](https://pt.aliexpress.com/item/4000542688096.html) (NB-IoT)
- Ou: [LILYGO® TTGO T-A7670G](https://pt.aliexpress.com/item/1005003036514769.html) (Cat-1)

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/lilygo-t-sim7000g.webp">

- <a href="https://www.dfrobot.com/product-2508.html">DFRobot CO (e potencialmente NO2 e outros gases)</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/co.avif">

- <a href="https://www.dfrobot.com/product-2871.html">Gravity: BME688</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/BME688.avif">

- <a href="https://www.dfrobot.com/product-2439.html">Gravity: PM2.5 Sensor de qualidade do ar</a>

<img width="260" src="https://raw.githubusercontent.com/dirceu-jr/anga/master/readme_files/pm25.webp">

# Dados em tempo real

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/1?results=60&title=CO"></iframe>

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/2?results=60&title=NO2"></iframe>

<iframe width="380" height="260" src="https://thingspeak.mathworks.com/channels/3022878/charts/4?results=60&title=PM2.5"></iframe>
