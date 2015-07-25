/*
  ESP8266-03 
  Webserver (for 32x32)

  -SD-Card
  -Zeit per NTP
  
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

//#include <EEPROM.h>           //evt. für Setupdaten, internen log


#include <SPI.h>
#include <SD.h>

Sd2Card card;
SdVolume volume;
SdFile root;
File file;
SdFile newFile;   //temorär zum löschen einer Datei, beim löschen oder überschreiben

const int chipSelect = 2;

//---------------internes Favicon-----------------------------//
#include "data.h"



bool showDebugatSerial=true;



//var
char charBuf[255];           //für Stringkonvertierungen


//Webserver
const char* ssid = "wifiap";
const char* password = "password";
//http://esp8266-server.de/IP-Steckdose.html  ->konfig per serielle

WiFiServer server(80);

IPAddress server_ip;
WiFiClient wwwclient;

//NTP
unsigned int NTP_localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); 
//  { 129,  6,  15, 28} time.nist.gov 
//  { 192, 43, 244, 18} time.nist.gov 
//  { 193, 79, 237, 14} ntp1.nl.net 


//  { 176,  9, 253, 76} de.pool.ntp.org
//  {   5, 83, 190, 253} 0.de.pool.ntp.org
//  { 178, 63,  64,  14} 1.de.pool.ntp.org
//  {  85,114, 132,  52} 2.de.pool.ntp.org
//  { 104,238, 167, 245} 3.de.pool.ntp.org

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte NTP_packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;    // A UDP instance to let us send and receive packets over UDP

unsigned long time_diff=0;      //Zeitunterschied NTP zu uptime
unsigned long timezone_diff=2;  //utc+2
int NTP_noconnect_counter=0;    //>1 =keine WiFi-Verbindung -->reconnect

word array_aktuelleUhrzeit[]={0,0,0 ,0,0,0};//stunde,minute,sekunde tag,mon,year
String s_uhrzeit="00:00:00";

unsigned long check_time=1000;          //alle 1000ms     
unsigned long last_check_time=0; //ms     

unsigned long check_NTPtime=1000*60*10; //10min     
unsigned long last_check_NTPtime=0; //ms     

int reconect_counter=0;
unsigned long verbindungen_counter=0;    

String header;
String content;
int httpStatus=0;
String stemp;

const char* List404[]={
    "apple-touch-icon"
    ,".psd"    
  };


#define pin1 16
#define pin2 0

bool blinkstate=LOW;

#define befehl_nichts 0 

//#define befehl_play 1 
//#define befehl_stop 2 
//#define befehl_delete 3 
#define befehl_upload 4 
//#define befehl_dateiliste 5
//#define befehl_info 6
//#define befehl_draw 7

byte befehl=0; //nichts
boolean idSD=0;


unsigned long loop_zeit;
String slog;

#define cahrBUFSIZ 255  //zu lesende Zeichenkette max Zeichen (für GET und [tag]-Auswertung)
char clientline[cahrBUFSIZ+1];   //GET/POST zur Ermittlung der angefragten Datei
char filename[cahrBUFSIZ+1];     //angeforderte Dateiname
char *p_search;              //Optionsvaribelnamen Pointer!
char p_search_value[cahrBUFSIZ+1];
boolean hatOptionen=false;
char *pointer;
uint32_t clindex = 0;
uint8_t NETc;//Puffer für eingelesenes Zeichen


uint64_t volumesize=0;//uint32_t
String SDInfo="-";
String fileinfo="";
     
void setup() {
  delay(100);
  Serial.begin(115200);
  
  pinMode(chipSelect, OUTPUT);
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  digitalWrite(pin1, LOW);
  digitalWrite(pin1, LOW);
  delay(100);
  
  digitalWrite(pin1, HIGH);
  delay(250);
  digitalWrite(pin2, HIGH);
  
  delay(1000);
   
  
  reconect();
  digitalWrite(pin1, LOW);
  
  get_NTP_Time();
   
 
  idSD=(SD.begin(chipSelect,SPI_FULL_SPEED)); 
  if(card.init(SPI_FULL_SPEED, chipSelect)){//
      switch (card.type()) {
        case SD_CARD_TYPE_SD1:
          SDInfo="SD1";
          break;
        case SD_CARD_TYPE_SD2:
          SDInfo="SD2";
          break;
        case SD_CARD_TYPE_SDHC:
          SDInfo="SDHC"; // *16GB Transkent SDHC Class10 von 32x32Pixel =OK
                                  // * 8GB Kingston SDHC Class4  = OK (Standartformatierung)
                                  // SD card blocks are always _512_ bytes
          break;
        default:
          SDInfo="Unknown";
      }

      
    if (!volume.init(&card)) {
        SDInfo+=" no FAT16/FAT32";
    }
    else{
        SDInfo+=" FAT"+String(volume.fatType(), DEC);
       
        uint64_t Gbyte=0;
        uint64_t Mbyte=0;
        uint64_t i64temp=0;
        
        volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
        Gbyte =volumesize* volume.clusterCount();       // we'll have a lot of clusters
        Gbyte *= 512;                            // SD card blocks are always 512 bytes
       
        Gbyte /= 1024;//kb
        Gbyte /= 1024;//mb
        Gbyte /= 1024;//gb
       
        Mbyte=volumesize* volume.clusterCount()*512;// -Gbyte*1024*1024*1024; 
        Mbyte/= 1024;//kb
        Mbyte/= 1024;//Mb

        SDInfo+=" size:"+String(int(Gbyte))+" GB ("+String(int(Mbyte))+" MB)";
        
        root.openRoot(&volume);
        //root.rewind();
   }
     
  }
  else{
    SDInfo+=" card.init Error";
  }

  digitalWrite(pin2, LOW);  
}

int i;
int wifistatus;
 
void loop() { //@80MHz
  loop_zeit = millis();

  //Zeit
  if(loop_zeit-last_check_time>check_time){//~einmal je Sekunde
      countClock(); //Zeit zählen, in array_aktuelleUhrzeit abgelegt
      
      blinkstate=!blinkstate;
      digitalWrite(pin1, blinkstate);

      if(loop_zeit-last_check_NTPtime>check_NTPtime){//~alle 30min
        get_NTP_Time(); 
        if(NTP_noconnect_counter>1){ reconect(); }
      }
      
  }

  //Wifi check
  wifistatus=WiFi.status();
  if (wifistatus != WL_CONNECTED)
  {
    //if(showDebugatSerial)Serial.println("reconect WiFi.status() != WL_CONNECTED "+String(wifistatus) );// =6
    //reconect();
  }

  //WebClient
  wwwclient = server.available();
  if (wwwclient) {
     digitalWrite(pin2, HIGH);
    if(wwwclient.connected()) {
      verbindungen_counter++;

      p_search_value[0]=0;     
      hatOptionen=false;
      befehl=befehl_nichts; //nichts
      
      clindex=0;
      NETc = wwwclient.read();                          //"GET /fufu.htm?val=456&v2=5 HTTP/1.1" 
      while (NETc!=13){//erste Zeile lesen
         if(NETc>31 && NETc<127){
           clientline[clindex] = NETc;//Array of Chars
           clindex++;
           if (clindex >= cahrBUFSIZ)break;
         }
         NETc = wwwclient.read(); 
     } 
      clientline[clindex] = 0;//null Termination

      fileinfo=s_uhrzeit+" "+clientline;

      Serial.print(s_uhrzeit);
      Serial.print(" ");
      Serial.println(clientline);

      (strstr(clientline, " HTTP"))[0] = 0;// Null-Terminierung              "GET /fufu.htm?val=456&v2=5"
  
      //Übergabeparameter ermitteln         
      if (strstr(clientline, "?") != 0){
              if (strstr(clientline, "&") != 0)   //fals & vorkommt 
                (strstr(clientline, "&"))[0] = 0; //abschneiden -> nur einen übergabepameter!
        
               p_search=strstr(clientline, "?");//Pointer
               p_search=p_search+1;                                            //  "val=456"

               //wird hier nicht weiter genutzt
               pointer=strstr(clientline, "=");
               pointer++; 

              for(i=0;i<clindex;i++)
                      p_search_value[i]=pointer[i];                           //  "456"

             (strstr(clientline, "="))[0] = 0;// "=" mit null ersten          //  "val"

              hatOptionen=true;

             if (strstr(p_search_value, "sethdiff") != 0){
                String si="";
                for(i=0;i<clindex;i++)  si+=p_search_value[i];
                timezone_diff = si.toInt();
              }

             
      }//if (strstr(clientline, "?") != 0){



      if (strstr(clientline, "GET") != 0)
         pointer = clientline + 5; // look after the "GET /" (5 chars)
       else{
         pointer = clientline + 6; // look after the "POST /" (6 chars)
         befehl=befehl_upload;
       }


      for(i=0;i<clindex;i++)
                  filename[i]=pointer[i];
     
      String stemp="index.htm";
      if((filename[0]==0)){
           for(i=0;i<stemp.length();i++)
                filename[i]=stemp[i];
                
          filename[stemp.length()]=0;
        }

        
/*    Serial.print("getfile:");
      Serial.print(filename);
      Serial.print(" clientline:");
      Serial.println(clientline);
*/

      wwwclient.flush(); //Discard any bytes that have been written to the client but not yet read. 


       bool is404=false;
       int l=sizeof(List404)/sizeof(char *);
       stemp=filename;
       for(i=0;i<l;i++){
         if(stemp.indexOf(List404[i])!= -1 )is404=true;
        }
        
      if(is404){
          //return 404
          httpStatus=404;
          send404(clientline);
          wwwclient.stop();   
          digitalWrite(pin2, LOW);
          return;  //exit
        }
      
      //Anfrage positiv, parameter verarbeiten, Antwort senden
      httpStatus=200;
     
     //SD-Card vorhanden?
      root.rewind();
     
     int dateistatus=423;

  //   boolean Status_SDisReady=SD.begin(chipSelect);//

      //TODO:spezial-datei?

     if(idSD){//datei von sd-card holen
          if (newFile.open(&root, filename, O_READ)) {
                   dateistatus=200;
                   sendeDateiToClient(wwwclient,newFile,filename);
                   newFile.close(); 
           }  
           else
              dateistatus=404;   
       }
        
        fileinfo+=" "+String(dateistatus);
     
      

      if(!idSD){//no SD or 404
          //internes Favicon || index.htm
          if (strstr(filename, "favicon.ico") != 0)
            sendFavIcon();
            else
            sendIndexHTM();
      }
      
      
      delay(1);
    }// if(wwwclient.connected())

    wwwclient.stop();
    digitalWrite(pin2, LOW);   
  }
  //kein weiterer code, da früher abgebrochen werden kann!
}//loop


//index.htm
void sendeDateiToClient(WiFiClient client,SdFile thefile,  char *name){
  
  String header="HTTP/1.1 200 OK\r\n";
  byte parsen=0;
  
  String ctype ="text/plain";
   if((strstr(name, ".htm") != 0) || (strstr(name, ".HTM") != 0)){
      ctype="text/html";
      parsen=1;
   }
   else
   if((strstr(name, ".css") != 0) || (strstr(name, ".CSS") != 0))
   ctype="text/css";
   else
   if((strstr(name, ".ico") != 0) || (strstr(name, ".ICO") != 0))
   ctype="image/icon";
   else
   if((strstr(name, ".jpg") != 0) || (strstr(name, ".JPG") != 0))
   ctype="image/jpeg";
   else
   if((strstr(name, ".png") != 0) || (strstr(name, ".PNG") != 0))
   ctype="image/png";
   else
   if((strstr(name, ".ico") != 0) || (strstr(name, ".ICO") != 0))
   ctype="image/x-icon";

 
   header+=("Content-Type: "+ctype+"\r\n");
   header+=("Server: ESP8266\r\n");
   if((strstr(name, ".png") ==0) && (strstr(name, ".PNG") ==0) && (strstr(name, ".ICO")==0) )
     header+=("CacheControl: no-cache\r\n");
   header+=("Connection: close\r\n");//close   Keep-Alive
   header+="\r\n";

   String sPuffer=header;
   String sPuffer2="";
   const int sPufferMax=1000;//zwischenspeichern für schnellere Überttragung; max ~60000
 
   
   int16_t c2 = thefile.read(); //muss int16 sein!!!
   while (c2 > -1){
         
        if(parsen==1){
          //zeilenweise & parsen
           if(char(c2)=='$'){//ab hier variabelname auslesen und ersetzen
              sPuffer2=char(c2);              
              c2 = thefile.read();//nächstes Zeichen
              while( (c2>64 && c2<91) || (c2>96 && c2<123) || (c2>47 && c2<58) ){
                  sPuffer2+=char(c2);
                  c2 = thefile.read();//nächstes Zeichen
              }
              sPuffer+=getVariabel(sPuffer2);
              sPuffer+=char(c2);
            }
            else
             sPuffer+=char(c2);
        }
        else        
          sPuffer+=char(c2);

       

       if(sPuffer.length()>sPufferMax){
          client.print(sPuffer);
          sPuffer="";
        }
       c2 = thefile.read();//nächstes Zeichen
    } 
    
  if(sPuffer.length()>0){
     client.print(sPuffer);
  }
     
}



String getVariabel(String s){
  String tempstr;
  
    if(s=="$zeit"){
      return s_uhrzeit;//Uhrzeit
    }
    
    if(s=="$h1gtag"){ 
      if(array_aktuelleUhrzeit[0]<11)
        return  "Guten morgen.";
      else
      if(array_aktuelleUhrzeit[0]<13)
        return "Mahlzeit.";
      else
      if(array_aktuelleUhrzeit[0]<18)
        return "Guten Tag.";
      else
        return "Guten Abend.";
    }
    
    
    if (s=="$uptime"){
      //Uptime
      unsigned long time =(int) millis()/1000;//sec
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
      return tempstr;
    }
    
    if(s=="$reconnects")return String(reconect_counter);
    if(s=="$anfragen")return String(verbindungen_counter);

    if(s=="$sdinfo")return SDInfo;
    if(s=="$fileinfo")return fileinfo;


    if(s=="$timeH")return String(array_aktuelleUhrzeit[0]);
    if(s=="$timeMin")return String(array_aktuelleUhrzeit[1]);
    
    return s;
}


void sendIndexHTM(){
  // Prepare the response
  header = "HTTP/1.1 200 OK\r\n";
  header += "Content-Type: text/html\r\n";//; charset=utf-8
  header += "CacheControl: no-cache\r\n";
  header += "Connection: close\r\n\r\n";
 
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
  while(indexHTM.indexOf("\r\n",pos2)>0){//Zeilenweise durchgehen
    pos1=pos2;
    pos2=indexHTM.indexOf("\r\n",pos2)+2;
    s=indexHTM.substring(pos1,pos2);

    if(s.indexOf("$") != -1){//Parameter auf der SEite gegen aktuelle Werte tauschen    
        if(s.indexOf("$zeit") != -1){
          s.replace("$zeit", s_uhrzeit );//Uhrzeit
        }
        
        if(s.indexOf("$h1gtag") != -1){ 
          if(array_aktuelleUhrzeit[0]<11)
          s.replace("$h1gtag", "Guten morgen.");
          else
          if(array_aktuelleUhrzeit[0]<13)
          s.replace("$h1gtag", "Mahlzeit.");
          else
          if(array_aktuelleUhrzeit[0]<18)
          s.replace("$h1gtag", "Guten Tag.");
          else
          s.replace("$h1gtag", "Guten Abend.");
        }
        
        
        if (s.indexOf("$uptime") != -1){
          //Uptime
          unsigned long time =(int) millis()/1000;//sec
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
        
        if(s.indexOf("$reconnects") != -1)s.replace("$reconnects",  String(reconect_counter));
        if(s.indexOf("$anfragen") != -1)s.replace("$anfragen",  String(verbindungen_counter));
    
        if(s.indexOf("$sdinfo") != -1)s.replace("$sdinfo",  SDInfo);
        if(s.indexOf("$fileinfo") != -1)s.replace("$fileinfo",  fileinfo);
    
   
        if(s.indexOf("$timeH") != -1)s.replace("$timeH",  String(array_aktuelleUhrzeit[0]));
        if(s.indexOf("$timeMin") != -1)s.replace("$timeMin",  String(array_aktuelleUhrzeit[1]));
    }
    sPuffer+=s;
    if(sPuffer.length()>sPufferMax){
      wwwclient.print(sPuffer);
      sPuffer="";
    }
  }
  if(sPuffer.length()>0){
     wwwclient.print(sPuffer);
  }
  
  if(showDebugatSerial)Serial.println("IndexHTM gesendet");
}

void sendFavIcon(){
  // header = "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n";
  // header+= "Connection: close\r\n";
  // header+= "CacheControl: cache\r\n\r\n"; 
  header = "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n";
  header+= "CacheControl: cache\r\n\r\n";

  int i;
  int anz=sizeof(favicon);
  for (i = 0; i <anz ; i++) {  //max 6000bytes!!!
    header+=(char(favicon[i]));
  }    
  //geht schneller als client.print(char(favicon[i])); 
  //wwwclient.flush();
  wwwclient.print(header);
  if(showDebugatSerial)Serial.println("favicon gesendet");
  delay(1);
}

void send404(String req){  
  content="404";                              //"<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>"
  header = "HTTP/1.1 404 Not Found\r\n";
  header += "Server: ESP8266\r\n";
  header += "Content-Length: ";
  header += content.length();
  header += "\r\nContent-Type: text/plain\r\n";//text/html
  header += "Connection: close\r\n";
  header += "\r\n";
  header += content; 
  wwwclient.print(header);
  delay(1);
}






String getDMY(){
  String re=String(array_aktuelleUhrzeit[3])+".";
  re+=String(array_aktuelleUhrzeit[4])+".";
  re+=String(array_aktuelleUhrzeit[5]);
  return re;  
}

void countClock(){
   //aktive Zeit anzeigen
   unsigned long zeit =(int) millis()/1000 +time_diff;//sec
   int h=(int) zeit/60/60;
   zeit=zeit-h*60*60;     
   int m=(int) zeit/60;
   zeit=zeit-m*60;//sec
   
   h=h+timezone_diff;
   while(h>=24)h=h-24;  

   array_aktuelleUhrzeit[0]=h;  //Zeit speichern
   array_aktuelleUhrzeit[1]=m;
   array_aktuelleUhrzeit[2]=zeit;

   s_uhrzeit="";  
   if(h<10)s_uhrzeit+="0";
   s_uhrzeit+=(String(h)+":");  
   if(m<10)s_uhrzeit+="0";
   s_uhrzeit+=(String(m)+":");  
   if(zeit<10)s_uhrzeit+="0";
   s_uhrzeit+=String(zeit);
  
  last_check_time=millis();
}


void reconect(){
  if(reconect_counter>0){
    if(showDebugatSerial)Serial.println("stop Service");
    udp.stop();
    //server.stop();
    WiFi.disconnect();
    delay(100);
  }

  WiFi.mode(WIFI_STA); //enum WiFiMode { WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 };
  WiFi.begin(ssid, password);
  if(showDebugatSerial)Serial.println("WiFi.begin");
  while (WiFi.status() != WL_CONNECTED) {
      delay(200);
  }
  Serial.println("WiFi WL_CONNECTED");
  
  server.begin();
  server_ip = WiFi.localIP();  
  
  Serial.println("IP:"+String(server_ip[0]) + "." + String(server_ip[1]) + "." + String(server_ip[2]) + "." + String(server_ip[3]));

  
  udp.begin(NTP_localPort);
  
  Serial.println("udp.begin "+String( udp.localPort() ));
  
  reconect_counter++;
}

void get_NTP_Time(){
  //return;
  
  sendNTPpacket(timeServer);
  delay(1000);                    //todo
  int cb = udp.parsePacket();
  if (!cb){
      NTP_noconnect_counter++;
  }
  else
  {
      udp.read(NTP_packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      unsigned long highWord = word(NTP_packetBuffer[40], NTP_packetBuffer[41]);
      unsigned long lowWord = word(NTP_packetBuffer[42], NTP_packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
       // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
    
      time_diff=epoch-(millis()/1000); //Zeitunterschied, localtime zu NTP in Sekunden

      NTP_noconnect_counter=0;
      /*
      if(showDebugatSerial){
           Serial.print("The UTC time is ");
           String s=String((epoch  % 86400L) / 3600) + ":";
           if ( ((epoch % 3600) / 60) < 10 ) s+="0";
           s+=String((epoch  % 3600) / 60)+":";
           if ( (epoch % 60) < 10 )s+="0";
           s+=String(epoch % 60);
           Serial.println(s);
      }  */     
    }
    last_check_NTPtime=millis();
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address){
  // set all bytes in the buffer to 0
  memset(NTP_packetBuffer, 0, NTP_PACKET_SIZE);           //Puffer mit 0 befüllen
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  NTP_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  NTP_packetBuffer[1] = 0;     // Stratum, or type of clock
  NTP_packetBuffer[2] = 6;     // Polling Interval
  NTP_packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  NTP_packetBuffer[12]  = 49;
  NTP_packetBuffer[13]  = 0x4E;
  NTP_packetBuffer[14]  = 49;
  NTP_packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(NTP_packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}




