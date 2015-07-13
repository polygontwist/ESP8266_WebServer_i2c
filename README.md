# ESP8266_WebServer_i2c

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
