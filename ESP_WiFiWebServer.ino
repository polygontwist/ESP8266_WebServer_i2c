/*
128x64 (16x8)

300k
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include <EEPROM.h>

//#include <DateTime.h>

#include <i2c_oled.h>//213.920
#include <i2c_sht21.h>

i2c_oled myOLED;
i2c_sht21 mySHT21;




//---------------FONT + GRAPHIC-----------------------------//
#include "data.h"

// OLED I2C bus address
#define OLED_address  0x3c
#define SHT_address  0x40

//#define Sensor_address  0x3c

const char* ssid = "yourSSID";
const char* password = "yourPassword";

char charBuf[17];

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);


static char texte[][16] ={
    "Connecting",
    "Verbindungen",
    ":",
    " "
};

uint16_t i=0;
unsigned long time;
unsigned long lasttime_sht;//merker um die Messung nur alle x Secunden zu machen
String stri;
int lastSec=0;
unsigned long stundendifferenz=2;//utc+2

float temperatur=0.0;
float humy=0.0;


WiFiClient client;
String header;
String content;
String serialstr;
String tempstr;
 
//NTP
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
//  { 192, 43, 244, 18} time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;    // A UDP instance to let us send and receive packets over UDP
unsigned long lasttime_NTP=-59;
unsigned long time_diff=0;
int noudpcounter=0;


//HTML
String indexHTM="<!DOCTYPE html>\r\n"
"<html><head><style>\r\n"
"body{font-family:Verdana;}\r\n"
"a{width:60px;margin-left:10px;}\r\n"
"h1{font-size:12pt;}\r\n"
"canvas{background-color:#eee;}\r\n"
"</style>\r\n"
"<meta content=\"width=device-width,initial-scale=2,maximum-scale=10,user-scalable=yes\" name=\"viewport\" />\r\n"
"<meta charset=\"utf-8\" /><link rel=\"shortcut icon\" href=\"favicon.ico\"></head>\r\n"
"<body>\r\n"
"<h1>$h1gtag</h1>\r\n"
"<p>$anfragen Anfragen</p>\r\n"
"<p>$temperatur &deg;C</p>\r\n"
"<p>$feuchte %</p>\r\n"
"<p>$zeit</p>\r\n"
"<p>uptime: $uptime</p>\r\n"
"<p>reconects: $reconnects</p>\r\n"
"<div id=\"data\"></div>\r\n"
"<script type=\"text/javascript\">\r\n"
"var data=[\r\n"
"$logdata\r\n"
"];\r\n"
"var cE=function(ziel,typ){var e=document.createElement(typ);ziel.appendChild(e);return e;}\r\n"
"var showdata=function(id){\r\n"
"var bb=720,hh=300,can=cE(document.getElementById(id),'canvas');can.width=bb;can.height=hh;\r\n"
"var t,s,xx,cc=can.getContext(\"2d\");"
"cc.font='15px Arial';\r\n"
"for(d=0;d<2;d++){"
" if(d==0)s='dd0000';else s='0000de';"
" cc.strokeStyle='#'+s;\r\n"
" cc.lineWidth=1;cc.beginPath();\r\n"
" if(d==0)yy=hh-Math.round(hh/(125+40)*(0+40));else yy=hh-Math.round(hh/100*50);\r\n"
" cc.moveTo(0,yy);cc.lineTo(bb,yy);cc.stroke();\r\n"
" cc.lineWidth=2;\r\n"
" if(d==0)cc.fillText(\"0°C\",3,yy-3);else cc.fillText(\"50%\",3,yy-3);\r\n"
" cc.beginPath();\r\n"
" for(t=0;t<data.length;t++){\r\n"
"   xx=Math.round(bb/(24*60)*(data[t][2]*60+data[t][3]));\r\n"
"   if(d==0)yy=hh-Math.round(hh/(125+40)*(data[t][d]+40));\r\n"
"   else yy=hh-Math.round(hh/100*data[t][d]);\r\n"
"   if(t==0)cc.moveTo(xx,yy);else cc.lineTo(xx,yy);\r\n"
" };"
" cc.stroke();"
" }"
"xx=Math.round(bb/(24*60)*($timeH*60+$timeMin));"
"cc.strokeStyle='#000';\r\n"
"cc.lineWidth=1;\r\n"
"cc.beginPath();"
"cc.moveTo(xx,0);"
"cc.lineTo(xx,hh);"
"cc.stroke();"
"}\r\n"
""
"window.onload=function(){showdata('data')}\r\n"
"</script></body></html>\r\n";

//endHTML

String aktuelleUhrzeit="00:00:00";
byte aaktuelleUhrzeit[]={0,0,0};
int verbindungsCounter=0;
int recontconuter=0;
    

int maxEE=4096;//4...4096

void setup() {
  Serial.begin(115200);
  delay(10);
  

  // Initialize I2C and OLED Display
  // Set SDA & SCL for ESP8266 to:
   Wire.begin(0, 2); //0=SDA, 2=SCL, alte Version Wire.pins(0,2)

  //EEPROM
  iniEE();

  //setupDisplay();   
  myOLED.init(OLED_address);
  myOLED.reset_display();
  
 
  myOLED.sendStrXY(texte[0],0,0);//Connecting
  //sendStrXY(ssid,14,0);
  
  WiFi.begin(ssid, password);
  
  String temp;
  while (WiFi.status() != WL_CONNECTED) {
    for(i=0;i<5;i++){
      temp=char(96+32+i);
      temp.toCharArray(charBuf,2);
      myOLED.sendStrXY(charBuf,0,15);
      delay(100);
    }
  }
  
  delay(1000);
  myOLED.clear_display();
  
 
  server.begin(); // Start the server
   
  // Print the IP address
  IPAddress ip = WiFi.localIP();
  String ipStr = "";
  ipStr+=char(101+32);//IP-Char
  ipStr+=String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  ipStr.toCharArray(charBuf,17);
  myOLED.sendStrXY(charBuf,0,0);
  
  myOLED.sendStrXY(texte[1],3,0);// Verbindungen 214.928
  
  //Uhr-Char
   temp=char(102+32);
   temp.toCharArray(charBuf,2);
   myOLED.sendStrXY(charBuf,6,0);
  
  //Feuchte/Temperatursensor
  mySHT21.init(SHT_address);
  lasttime_sht=0;//sec 
  
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
//delay(2000); 

/*  int i;
 for(i=0;i<64;i++){
  myOLED.drawPixel(i,i,0);//white
  }
 for(i=0;i<64;i++){
  myOLED.drawPixel(i+1,i,1);//black
  }
 for(i=0;i<64;i++){
  myOLED.drawPixel(i+2,i,2);//invert
  }
 myOLED.drawPixel(1,1,0);
  myOLED.drawPixel(2,2,0);
  myOLED.drawPixel(4,4,0);
  */
   
  //myOLED.getBuff();
   
 /* int ix,iy;
   for(ix=0;ix<8;ix++)
   for(iy=0;iy<8;iy++){
      myOLED.drawPixel(ix+8*1,iy,2);//invert
  }*/
 // myOLED.drawRect(0,0, 127,63 ,0);
}





void loop() {
  //return;
  
  //aktive Zeit anzeigen
 time =(int) millis()/1000 +time_diff;//sec
 int h=(int) time/60/60;
 time=time-h*60*60;     
 int m=(int) time/60;
 time=time-m*60;//sec
 
 h=h+stundendifferenz;
 while(h>=24)h=h-24;
    
 if(time!=lastSec){//nur wenn nötig
	aktuelleUhrzeit="";
    stri=String(h,DEC);
    if(h<10)stri="0"+stri;
	aktuelleUhrzeit+=stri;
    stri.toCharArray(charBuf,3);
    myOLED.sendStrXY(charBuf,6,1); //hh
    myOLED.sendStrXY(texte[2],6,3);//":"    
    
	stri=String(m,DEC);
    if(m<10)stri="0"+stri;
	aktuelleUhrzeit+=":";
    aktuelleUhrzeit+=stri;    
    stri.toCharArray(charBuf,3);
    myOLED.sendStrXY(charBuf,6,4); //mm  
    myOLED.sendStrXY(texte[2],6,6);//":"  
	
    stri=String(time,DEC);
    if(time<10)stri="0"+stri; 
	aktuelleUhrzeit+=":";
    aktuelleUhrzeit+=stri;
    stri.toCharArray(charBuf,3);  
    myOLED.sendStrXY(charBuf,6,7); //ss
    lastSec=time;
	
	aaktuelleUhrzeit[0]=h;
	aaktuelleUhrzeit[1]=m;
	aaktuelleUhrzeit[2]=time;
   }
   
  
   if((m-lasttime_NTP)>5 || (m-lasttime_NTP)<0){//nur alle 5 min  
        lasttime_NTP=m;            // 4 - 59 = -55
         
        int thewifistatus =WiFi.status();
        Serial.print("WiFi.status()=");
        Serial.print(thewifistatus);
        Serial.print(" (");
        
        switch(thewifistatus){
          case WL_IDLE_STATUS: Serial.print("WL_IDLE_STATUS");break;
          case WL_NO_SSID_AVAIL: Serial.print("WL_NO_SSID_AVAIL");break;
          case WL_SCAN_COMPLETED: Serial.print("WL_SCAN_COMPLETED");break;
          case WL_CONNECTED: Serial.print("WL_CONNECTED");break;
          case WL_CONNECT_FAILED: Serial.print("WL_CONNECT_FAILED");break;
          case WL_CONNECTION_LOST: Serial.print("WL_CONNECTION_LOST");break;
          case WL_DISCONNECTED: Serial.print("WL_DISCONNECTED");break;        
        }
        //Serial.println("");
      
        thewifistatus =server.status();
        Serial.print(") server.status()=");
        Serial.print(thewifistatus);
        Serial.println(" ");
     /*
  CLOSED      = 0,  LISTEN      = 1,*  SYN_SENT    = 2,  SYN_RCVD    = 3,  ESTABLISHED = 4,
  FIN_WAIT_1  = 5,  FIN_WAIT_2  = 6,  CLOSE_WAIT  = 7,  CLOSING     = 8,  LAST_ACK    = 9,
  TIME_WAIT   = 10     */
        
        sendNTPpacket(timeServer); // send an NTP packet to a time server
        delay(1000);
        int cb = udp.parsePacket();
        if (!cb) {
          Serial.println("no packet yet");
          lasttime_NTP=m-5;      
          noudpcounter++;
          if(noudpcounter>1){
            reconect();//server reset
            noudpcounter=0;
          }    
        }
        else{
              noudpcounter=0;
              //Serial.print("packet received, length=");
              //Serial.println(cb);
              // We've received a packet, read the data from it
              udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
              
              //the timestamp starts at byte 40 of the received packet and is four bytes,
              // or two words, long. First, esxtract the two words:
              
              unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
              unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
              // combine the four bytes (two words) into a long integer
              // this is NTP time (seconds since Jan 1 1900):
              unsigned long secsSince1900 = highWord << 16 | lowWord;
              //Serial.print("Seconds since Jan 1 1900 = " );
              //Serial.println(secsSince1900);
              
              // now convert NTP time into everyday time:
              //Serial.print("Unix time = ");
              // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
              const unsigned long seventyYears = 2208988800UL;
              // subtract seventy years:
              unsigned long epoch = secsSince1900 - seventyYears;
              // print Unix time:
              //Serial.println(epoch);
              
              time_diff=epoch-(millis()/1000);
              
              // print the hour, minute and second:
              Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
              Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
              Serial.print(':');
              if ( ((epoch % 3600) / 60) < 10 ) {
              // In the first 10 minutes of each hour, we'll want a leading '0'
              Serial.print('0');
              }
              Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
              Serial.print(':');
              if ( (epoch % 60) < 10 ) {
              // In the first 10 seconds of each minute, we'll want a leading '0'
              Serial.print('0');
              }
              Serial.println(epoch % 60); // print the second
          }
  }
  
  
  if((time-lasttime_sht)>10){//nur alle 10 sec     
    lasttime_sht=time;    
    temperatur=mySHT21.readTemp();
    humy=mySHT21.readHumidity();

    stri=String(temperatur);
    stri+=char(95+32);      //sonderzeichen, als letztes im Font
    stri+="C  ";    
    stri.toCharArray(charBuf,15);
    myOLED.sendStrXY(charBuf,1,0);
 
    stri=String(humy);
    stri+="%  ";
    stri.toCharArray(charBuf,15);
    myOLED.sendStrXY(charBuf,1,8);
    
	//log temp,hum,hh,mm
	if(float(m/5)==int(m/5)){
		int adresse=((h*60+m)/5)*4 +6;// = 288(*4=1152)  +6
   /* Serial.print("merke Daten @");
    Serial.print(adresse);
    Serial.print(" ");
    Serial.print(byte(temperatur+40));
    Serial.print(" ");
    Serial.print(byte(humy));
    Serial.print(" ");
    Serial.print(byte(h));
    Serial.print(" ");
    Serial.print(byte(m));
    Serial.println(" ");*/
    EEPROM.write(adresse+0,byte(temperatur+40));
		EEPROM.write(adresse+1,byte(humy));
		EEPROM.write(adresse+2,byte(h));
		EEPROM.write(adresse+3,byte(m));
		EEPROM.commit();
	}

    //check WIFI
    stri="";
    switch( WiFi.status()){
      case WL_CONNECTED: 
        //stri="ok";
      break;
      case WL_CONNECT_FAILED: 
        stri="CONNECT_FAILED";
        Serial.println(stri);
        break;
      case WL_CONNECTION_LOST: 
        stri="CONNECTION_LOST";
        Serial.println(stri);
        break;
      case WL_DISCONNECTED: 
        stri="DISCONNECTED";
        Serial.println(stri);
        break;
    }
    if(stri!=""){
      stri.toCharArray(charBuf,15);
      myOLED.sendStrXY(charBuf,7,0);
    }
  }
  
  
  
  
  // Check if a client has connected
  //WiFiClient client = server.available();
  client = server.available();
  if (client) {
    
    while (client.connected()) {
    
     
            verbindungsCounter++;
            stri=String(verbindungsCounter);
            stri.toCharArray(charBuf,15);
            myOLED.sendStrXY(charBuf,4,0);
            
            // Read the first line of the request
            String req = client.readStringUntil('\r');  
           // req.toCharArray(charBuf,17);
           // myOLED.sendStrXY(charBuf,2,0);
            
            Serial.println(req);//get ... 
            
            client.flush(); //Discard any bytes that have been written to the client but not yet read. 
           
           
           //set stundendifferenz  192.168.0.68/?sethdiff=2
           if (req.indexOf("sethdiff=") != -1){
              int pos=req.indexOf("sethdiff=");
              String shd=req.substring(pos+9);
              pos=shd.indexOf("&");
              if(pos>-1){
                 shd=shd.substring(0,pos-1);
              }
              
              stundendifferenz = shd.toInt();
              Serial.print("set sethdiff=");
              Serial.println(stundendifferenz);
           }
           
           /*
            GET /favicon.ico HTTP/1.1
            GET / HTTP/1.1
            GET /?sethdiff=2  Zeitdifferenz in Stunden zu UTC
          
            ipad:
            GET / HTTP/1.1
            GET /apple-touch-icon-152x152-precomposed.png HTTP/1.1
            GET /apple-touch-icon-152x152.png HTTP/1.1
            GET /apple-touch-icon.png HTTP/1.1
            GET /apple-touch-icon.png HTTP/1.1
            GET /apple-touch-icon-precomposed.png HTTP/1.1
            GET /favicon.ico HTTP/1.1
            */            
            //client.flush();
            header="";
            content="";
            serialstr="";
            
            if (req.indexOf("/apple-touch-icon") != -1){
				send404(req);
            }    
            else
            if (req.indexOf("/favicon.ico") != -1){
				sendFavIcon();
            }    
            else
            {
				sendIndexHTM();
			}
         
         delay(1);        
         break; //while verlassen 
    }//while client.connected()
    
    client.stop();    
  }//if (client)
}

void send404(String req){
	header = "HTTP/1.1 404 Not Found\r\n";
	header += "Server: ESP8266\r\n";
	header += "Content-Type: text/plain\r\n";
	header += "Connection: close\r\n";
	header += "\r\n";
	header += "404"; 
	client.flush();
	client.print(header);
	Serial.print("send 404 ");
	Serial.println(req);
}


void sendFavIcon(){
	// header = "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n";
	// header+= "Connection: close\r\n";
	// header+= "CacheControl: cache\r\n\r\n"; 
	header = "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n";
	header+= "CacheControl: cache\r\n\r\n";

	int i;
	int anz=sizeof(favicon);
	for (i = 0; i <anz ; i++) {
	  header+=(char(favicon[i]));
	}    
	//geht schneller als client.print(char(favicon[i])); 
	client.flush();
	client.print(header);
	Serial.println("send fav");
}

void sendIndexHTM(){
	// Prepare the response
	header = "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html\r\n";//; charset=utf-8
	header += "CacheControl: no-cache\r\n";
	header += "Connection: close\r\n\r\n";
	client.flush();
	//client.print(header);
	
	int pos1=0;
	int pos2=0;
	int pos3=0;
	int pos4=0;
	int i;
	int adresse;
	String s;
	String tempstr;
  String sPuffer=header;
  const int sPufferMax=1000;//zwischenspeichern für schnellere Überttragung; max ~60000
  
	//html indexHTM
	while(indexHTM.indexOf("\r\n",pos2)>0){
		pos1=pos2;
		pos2=indexHTM.indexOf("\r\n",pos2)+2;
		s=indexHTM.substring(pos1,pos2);
        
		if(s.indexOf("$zeit") != -1){
		  s.replace("$zeit", aktuelleUhrzeit);
		}
		
		if(s.indexOf("$h1gtag") != -1){ 
			if(aaktuelleUhrzeit[0]<11)
			s.replace("$h1gtag", "Guten morgen.");
			else
			if(aaktuelleUhrzeit[0]<13)
			s.replace("$h1gtag", "Mahlzeit.");
			else
			if(aaktuelleUhrzeit[0]<18)
			s.replace("$h1gtag", "Guten Tag.");
			else
			s.replace("$h1gtag", "Guten Abend.");
		}
		
		if(s.indexOf("$anfragen") != -1)s.replace("$anfragen", String(verbindungsCounter));
		
		if(s.indexOf("$temperatur") != -1)s.replace("$temperatur",  String(temperatur));
		if(s.indexOf("$feuchte") != -1)s.replace("$feuchte",  String(humy));
		
		if (s.indexOf("$uptime") != -1){
			//Uptime
			time =(int) millis()/1000;//sec
			int days=(int) time/60/60/24;
			time=time-days*60*60*24;     
			int h=(int) time/60/60;
			time=time-h*60*60;             
			int m=(int) time/60;
			time=time-m*60;//sec

			tempstr=days;
			tempstr+=" Tage ";
			tempstr+=h;
			tempstr+=" Stunden ";
			tempstr+=m;
			tempstr+=" Minuten";
			s.replace("$uptime", tempstr);
		}
		
		if(s.indexOf("$reconnects") != -1)s.replace("$reconnects",  String(recontconuter));


    if(s.indexOf("$timeH") != -1)s.replace("$timeH",  String(aaktuelleUhrzeit[0]));

    if(s.indexOf("$timeMin") != -1)s.replace("$timeMin",  String(aaktuelleUhrzeit[1]));

    
		if (s.indexOf("$logdata") != -1){
			pos3=s.indexOf("$logdata");
			pos4=pos3+8;
      if(pos3>0){
  			tempstr=s.substring(0,pos3);
        sPuffer+=tempstr;
  			//client.print(tempstr);
      }
      
			//datatoutput
			for(i=0;i<288;i++){
				adresse=i*4+6;
				tempstr="";
				if(i>0)tempstr+=",";
						tempstr+="[";
						tempstr+=float(EEPROM.read(adresse+0)-40);//temp
						tempstr+=",";
						tempstr+=EEPROM.read(adresse+1);//hum
						tempstr+=",";
						tempstr+=EEPROM.read(adresse+2);//hh
						tempstr+=",";
						tempstr+=EEPROM.read(adresse+3);//min
						tempstr+="]\r\n";

        sPuffer+=tempstr;
        if(sPuffer.length()>sPufferMax){
           client.print(sPuffer);
           sPuffer="";
        }
			}
				
			s=s.substring(pos4);
		}
		
		sPuffer+=s;
    if(sPuffer.length()>sPufferMax){
		  client.print(sPuffer);
      sPuffer="";
		}
	}
  if(sPuffer.length()>0){
     client.print(sPuffer);
  }
	
	Serial.println("send IndexHTM");

}


void reconect(){
  Serial.println("#### restart WIFI ####");
  recontconuter++;
  
  Serial.println("recontectcounter:");
  Serial.println(recontconuter);
  udp.stop();
 // server.stop();
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(ssid, password);
  String temp;
  while (WiFi.status() != WL_CONNECTED) {
    for(i=0;i<5;i++){
      temp=char(96+32+i);
      temp.toCharArray(charBuf,2);
      myOLED.sendStrXY(charBuf,0,15);
      delay(100);
    }
  }
   myOLED.sendStrXY(texte[3],0,15);//" "
   server.begin();
   Serial.print("IP: ");
   
  IPAddress ip = WiFi.localIP();
  String ipStr = "";
  ipStr+=char(101+32);//IP-Char
  ipStr+=String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  ipStr.toCharArray(charBuf,17);
  myOLED.sendStrXY(charBuf,0,0);
 
  Serial.println(String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));
 
  udp.begin(localPort);
  Serial.print("udp.Local port: ");
  Serial.println(udp.localPort());

 
}

//4096  7...4095 (4088)/4=1022
//temp,feuchte,hh,mm  ->alle 2min-> 720*4=2880byte/Tag
//messung ist alle 10sec  
//adresse=(hh*60+min)[1440] /5 = 288  +6

void iniEE(){
	EEPROM.begin(maxEE);//4...4096
	if(
		readEE(0)==0xAA &&
		readEE(1)==0x66 &&
		readEE(2)==0xAA &&
		readEE(3)==0x66 &&
		readEE(4)==0xAA &&
		readEE(5)==0x00 
	){
	//OK
	}
	else{
	//clear
		EEPROM.write(0, 0xAA);
		EEPROM.write(1, 0x66);
		EEPROM.write(2, 0xAA);
		EEPROM.write(3, 0x66);
		EEPROM.write(4, 0xAA);
		EEPROM.write(5, 0x00);
		for (int i = 6; i < maxEE; i++)
			EEPROM.write(i, 0);
		
		EEPROM.commit();
		EEPROM.end();
		delay(10);
		EEPROM.begin(maxEE);
	}
}

byte readEE(int adresse){
  if(adresse>=maxEE)return 0;
  byte re=0;
  re = EEPROM.read(adresse);
  return re;
}
void writeEE(int adresse,byte val){
  if(adresse>=maxEE)return;
  EEPROM.write(adresse, val);
  EEPROM.commit();
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
