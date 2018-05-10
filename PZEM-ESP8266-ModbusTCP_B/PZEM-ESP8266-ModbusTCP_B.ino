/*--------------------------------------------------------------
  Program:      PZEM-004 & ESP8266 12E + Modbus TCP/IP Slave

  Description:  Connection and communication Serial power meter Active PZEM-004
                Peacefair with Module ESP8266 as Slave Modbus TCP / IP, cropped version

                Conexión y comunicación Serial medidor de potencia Activa PZEM-004
                Peacefair con Modulo ESP8266 como Esclavo Modbus TCP / IP, versión recortada
  
  Hardware:     ESP8266 12E NodeMCU Lolin .
                PZEM-004
                
  Software:     Arduino IDE v1.8.3
  
  Date:         09 May 2018
   
  Modified or Created:       PDAControl   http://pdacontroles.com   http://pdacontrolen.com

  Complete Tutorial English: http://pdacontrolen.com/meter-pzem-004-esp8266-platform-iot-node-red-modbus-tcp-ip/
  
  Tutorial Completo Español: http://pdacontroles.com/medidor-pzem-004-esp8266-plataforma-iot-node-red-modbus-tcpip/

  Based: PZEM004T library for olehs                          https://github.com/olehs/PZEM004T

         SoftwareSerial for esp8266                          https://github.com/plerup/espsoftwareserial

--------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h> // Arduino IDE <1.6.6

#include <PZEM004T.h> //https://github.com/olehs/PZEM004T
PZEM004T pzem(&Serial);                                        /// use Serial
IPAddress ip(192,168,1,1);

const char* ssid       = "***********";
const char* password   = "***********";
int ModbusTCP_port     = 502;

  float v_mb =0;          
  float i_mb =0;          
  float p_mb =0;        
  float e_mb =0;

////////  Required for Modbus TCP / IP /// Requerido para Modbus TCP/IP  /////////
#define maxInputRegister    20
#define maxHoldingRegister    20

#define MB_FC_NONE 0
#define MB_FC_READ_REGISTERS 3              //implemented
#define MB_FC_WRITE_REGISTER 6              //implemented
#define MB_FC_WRITE_MULTIPLE_REGISTERS 16   //implemented
//
// MODBUS Error Codes
//
#define MB_EC_NONE 0
#define MB_EC_ILLEGAL_FUNCTION 1
#define MB_EC_ILLEGAL_DATA_ADDRESS 2
#define MB_EC_ILLEGAL_DATA_VALUE 3
#define MB_EC_SLAVE_DEVICE_FAILURE 4
//
// MODBUS MBAP offsets
//
#define MB_TCP_TID          0
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7
#define MB_TCP_REGISTER_START         8
#define MB_TCP_REGISTER_NUMBER         10

byte ByteArray[260];
unsigned int  MBHoldingRegister[maxHoldingRegister];

///https://www.prosoft-technology.com/kb/assets/intro_modbustcp.pdf
///http://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf


//led indicador de Conexion y desconexion de RED o maestro modbus tcp
//LED indicator of connection and disconnection of network or Modbus TCP Master
#define led_control  12  //D6    //led verde // green led 


#include <Ticker.h>    ///  https://github.com/esp8266/Arduino/tree/master/libraries/Ticker
Ticker event;
//////////////////////////////////////////////////////////////////////////

WiFiServer MBServer(ModbusTCP_port);

void ledcontrol () {
   int state = digitalRead(led_control);  // get the current state of GPIO1 pin
  digitalWrite(led_control, !state);     // set pin to the opposite state          

}

void setup() {

 Serial.begin(9600);
 pinMode(led_control, OUTPUT);    
  
 delay(100) ;
 WiFi.begin(ssid, password);
 delay(100) ;
 ///Serial.println(".");
 while (WiFi.status() != WL_CONNECTED) {
    delay(500);

  }

///     Print the IP address and 500ms then the port is enabled for communication on the PZEM meter
///     Imprime la direccion IP y 500ms despues el puerto se habilita para comunicacion en el medidor PZEM 
  
  MBServer.begin();
  Serial.println("Connected ");
  Serial.print("ESP8266 Slave Modbus TCP/IP ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(String(ModbusTCP_port));
 
  delay(500);

  pzem.setAddress(ip);

 // Nota:::  no utilizar mas el puerto serial  Serial.begin(9600); dado que estara ocupado en la comunicacion con el medidor PZEM
 // Note ::: no longer use serial port Serial.begin(9600);since he will be busy communicating with the PZEM meter
 
}


void loop() {


  // Check if a client has connected  // Modbus TCP/IP
  WiFiClient client = MBServer.available();
  if (!client) {
   return;
  }
 

    boolean flagClientConnected = 0;
    byte byteFN = MB_FC_NONE;
    int Start;
    int WordDataLength;
    int ByteDataLength;
    int MessageLength;
    
    // Modbus TCP/IP
    while (client.connected()) {
      

    /////////////////// timeout 5 sec - sin respuesta de servidor.. //////////////////////
    /////////////////// timeout 5 sec - no server response ..       //////////////////////
    
                 unsigned long timeout = millis();
                while (client.available() == 0) {
                  if (millis() - timeout > 5000) {
                    event.attach(0.6, ledcontrol);   // led flashing
                   
                    ///Serial.println(">>> Client Timeout !");
                    client.stop();
                    return;
                  }
                   digitalWrite(led_control, LOW);  // led off    
                   ///Serial.println(">>> Client Connect !");
                   }                   
                   
      ////////////////////////////////////////////////////////////////////////////////////          
                 
    if(client.available())
        
    {
        flagClientConnected = 1;
        int i = 0;
        while(client.available())
        {
            ByteArray[i] = client.read();
            i++;
        }
         client.flush();
/*
         Lectura de Medidor PZEM
         PZEM Meter Reading
*/
        float v = pzem.voltage(ip);          
        if(v >= 0.0){   v_mb =v;      } //V  
        
        float c = pzem.current(ip);
        if(c >= 0.0){   i_mb= c;       } //A                                                                                                                      
        
        float p = pzem.power(ip);
        if(p >= 0.0){   p_mb=p;       } //kW
        
        float e = pzem.energy(ip);          
        if(e >= 0.0){  e_mb =e;        } //kWh       
          
/*
        Escritura de mediciones en Holding Registers
        Writing of measurements in Holding Registers
*/
         MBHoldingRegister[0] = 0;                 // Holding Registers[0] =0     not used
         MBHoldingRegister[1] = (v_mb *  10);      // Holding Registers[1]        Voltage  * 10       example:   127.2 (float) x 10 = 1272 (int) // quick solution  
         MBHoldingRegister[2] = (i_mb *  10);      // Holding Registers[2]        Current  * 10
         MBHoldingRegister[3] = (p_mb *  10);      // Holding Registers[3]        Power    * 10
         MBHoldingRegister[4] = (e_mb);            // Holding Registers[4]        Energy 

         
/*
 *                          EXAMPLES
 * 
          ///////// Holding Register [0]  A [9]   = 10 Holding Registers Escritura
          ///////// Holding Register [0]  A [9]   = 10 Holding Registers Writing                  
                 
                  MBHoldingRegister[1] = analogRead(A0);
                  MBHoldingRegister[2] = random(0,12);
                  MBHoldingRegister[3] = random(0,12);
                  MBHoldingRegister[4] = random(0,12);
                  MBHoldingRegister[5] = random(0,12);
                  MBHoldingRegister[6] = random(0,12);
                  MBHoldingRegister[7] = random(0,12);
                  MBHoldingRegister[8] = random(0,12);
                  MBHoldingRegister[9] = random(0,12);     
*/
        ///////// Holding Register [10]  A [19]  =  10 Holding  Registers Lectura
        ///////// Holding Register [10] A [19]   =  10 Holding Registers Reading

        /*
                int Temporal[10];                
                Temporal[0] = MBHoldingRegister[10];                
                Temporal[1] = MBHoldingRegister[11];                    
                Temporal[2] = MBHoldingRegister[12];
                Temporal[3] = MBHoldingRegister[13];
                Temporal[4] = MBHoldingRegister[14];
                Temporal[5] = MBHoldingRegister[15];
                Temporal[6] = MBHoldingRegister[16];
                Temporal[7] = MBHoldingRegister[17];
                Temporal[8] = MBHoldingRegister[18];
                Temporal[9] = MBHoldingRegister[19];   

       */       

        ////  rutine Modbus TCP
        byteFN = ByteArray[MB_TCP_FUNC];
        Start = word(ByteArray[MB_TCP_REGISTER_START],ByteArray[MB_TCP_REGISTER_START+1]);
        WordDataLength = word(ByteArray[MB_TCP_REGISTER_NUMBER],ByteArray[MB_TCP_REGISTER_NUMBER+1]);
     }
    
    // Handle request

    switch(byteFN) {
        case MB_FC_NONE:
            break;        
                 
        case MB_FC_READ_REGISTERS:  // 03 Read Holding Registers
            ByteDataLength = WordDataLength * 2;
            ByteArray[5] = ByteDataLength + 3; //Number of bytes after this one.
            ByteArray[8] = ByteDataLength;     //Number of bytes after this one (or number of bytes of data).
            for(int i = 0; i < WordDataLength; i++)
            {
                ByteArray[ 9 + i * 2] = highByte(MBHoldingRegister[Start + i]);
                ByteArray[10 + i * 2] =  lowByte(MBHoldingRegister[Start + i]);
            }
            MessageLength = ByteDataLength + 9;
            client.write((const uint8_t *)ByteArray,MessageLength);
      
            byteFN = MB_FC_NONE;
   
            break;
                         
        case MB_FC_WRITE_REGISTER:  // 06 Write Holding Register
            MBHoldingRegister[Start] = word(ByteArray[MB_TCP_REGISTER_NUMBER],ByteArray[MB_TCP_REGISTER_NUMBER+1]);
            ByteArray[5] = 6; //Number of bytes after this one.
            MessageLength = 12;
            client.write((const uint8_t *)ByteArray,MessageLength);
            byteFN = MB_FC_NONE;
            break;
            
        case MB_FC_WRITE_MULTIPLE_REGISTERS:    //16 Write Holding Registers
            ByteDataLength = WordDataLength * 2;
            ByteArray[5] = ByteDataLength + 3; //Number of bytes after this one.
            for(int i = 0; i < WordDataLength; i++)
            {
                MBHoldingRegister[Start + i] =  word(ByteArray[ 13 + i * 2],ByteArray[14 + i * 2]);
            }
            MessageLength = 12;
            client.write((const uint8_t *)ByteArray,MessageLength);    
            byteFN = MB_FC_NONE;
     
            break;
       }
    }
    

}
