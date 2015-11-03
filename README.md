# ESP_WiFiWebServer.ino

Programiert über die Arduino-IDE.

ESP8266 WiFi Webserver mit i2c oLED und SHT21(Feutigkeit & Temperatur), 
Speichert Daten vom SHT21 im EEPROM zwischen (1Tag).
Benutzt für die aktuelle Zeit den time.nist.gov NTP server, er diehnt auch ale Erkennung ob die WLAN-Verbindungnoch steht

Gibt über HTTP folgende Infos aus:
- Begrüßung je nach Tageszeit
- Anzahl der HTTP-Anfragen
- Temperatur (-40°...+125°)
- Luftfeuchte (0..100%)
- Uhrzeit
- uptime
- Anzahl der Reconects
- HTML5-Canvas mit Temperatur- und Luftfeuchte-Kurve

![Datencanvas](https://raw.githubusercontent.com/polygontwist/ESP8266_WebServer_i2c/master/canvas.png)


# ESP_WiFiWebServer_SD.ino (ESP8266-03)
| Pin | 	Funktion | 	SD-Card| 
| -------------	|  ------------- |  -------------| 
| 1 	| VCC 3V3 | 	VDD| 
| 2 	| PD –> Vcc| |  	
| 3   | GPIO14 |  CLK| 
| 4 	| GPIO16 | (LED)	| 
| 5 	| GPIO12 	| DAT0(MISO)| 
| 6 	| RX 	| | 
| 7 	| GPIO13 	| CMD (MOSI)| 
| 8 	| TX 	| | 
| 9 	| GPIO15 –> GND | | 	
| 10 	| GPIO02 | 	CD| 
| 11 	| GPIO00 (flashen –> GND) | (LED) | 	
| 12 	| GND 	| GND | 
Pin=Pins der eigenen Adapterplatine

# getHex.htm
convert bin-File to Hex-Array for Arduino or other
z.B. für favicon.ico

# Bilder & Video vom Projekt
- https://instagram.com/p/5WSjv7zfvV/
- https://instagram.com/p/5mWe2uzfsn/
- https://instagram.com/p/5S36Kqzfqw/

Setting mit Teperatur/Luftfeuchtelogger:
![espo8266_templog](https://cloud.githubusercontent.com/assets/3751286/10920497/a3ae112c-8270-11e5-95b9-7c16cf35ed13.jpg)
