# Coup Control ESP32

This is a project I am putting together to control lighting and heat in my chicken coup.  

## Project goals

### Immediate 

- Control lighting on a schedule to maintain a minimum number of hours of "daylight".
- Control the temperature in the coup to keep it above freezing.  This is more for keeping the water from freezing than keeping the chickens warm.  However, it does get rather cold in the Michigan winters, so I'm sure it won't hurt the chickens either!

### Long Term

- Control the coup door
- Control a fan for cooling
- Add an external api to set the schedule for lighting, heat, etc.
- Possibly add a light sensor to help decide when to turn the light on and off

## Hardware

- ESP32
- A [4 channel relay board](https://smile.amazon.com/gp/product/B00KTEN3TM)
- A [BMP280](https://smile.amazon.com/dp/B07S98QBTQ) temperature and barometric pressure sensor
- 2 LED strip lights

## Implementation 

1. When the ESP32 starts, it gets the time from any open wifi access point it can fine.  Thank you [Frank Krueger](https://github.com/praeclarum) for your [MarriageClock](https://github.com/praeclarum/MarriageClock) project. 
1. It also initializes the BMP280 sensor.
1. On a set interval it checks the time and between September and April it will make sure the chickens have daylight from 8am to 9pm.
1. On a set interval it checks the temperature.  When the temp drops below 35, it turns on the heat. The heat stays on until the temp gets above 40.

## Want to help?

This is kind of a personal project, so I'm not expecting any outside influence, but if you have an idea, start an issue and we'll talk!