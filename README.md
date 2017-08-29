# ESP32 mongoose web project

ESP32 is a an example project using esp-idf, mongoose networking lib and FreeRTOS.
It's in a running state, but needs review and validations.

1. Make the project.
2. Flash the esp32.
3. Connect to the open wifi access point ESP32_<esp32_mac_address>
4. Connect to 192.168.4.1 in a browser.
5. Choose Upload and upload files in main/relay_gpio/html.
6. Go back to Config and configure the wifi AP or Client.
7. Test the app in a browser: http://<esp_ip_adr>/relay.html
8. Enjoy :-)

You can push the EN (gpio 0) to bring the esp32 into open AP mode. 

All contributions are welcome!!

**3rd party libraries**:  
FreeRTOS: GPLv2  
Mongoose Networking Libary: GPLv2  
ESF-IDF: GPLv2  
  
**Custom bits**:  
Public Domain  
