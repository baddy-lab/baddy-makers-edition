In order to install BADDY new firmware, you need:
- To install VScode with PlatformIO modules installed. Once VS code installed, install the PlatformIO add-on with it's C/C++ extensions
- Take and Android cable (data transferred enabled) to connect your BADDY to your laptop (mac or PC)
- Note that OTA update available (to BADDY WiFi network) and uncomment the lines with ";" int the platformio.ini file.

The libraries that need to be installed:
- ArduinoJson by Benoit Blanchon
- LedControl by Eberhard Fahle
- AsyncTCP and ESPAsynTCP by Hristo Gochkov
- mDNSResolver by Myles Eftos

Once main.cpp, platformini.io and libraries installed and ready, please build project and upload and monitor to check logs

For any question, please join BADDY Facebook Group - there is a dedicated discussion Group on Firmware updates
