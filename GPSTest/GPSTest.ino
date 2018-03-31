#include <string.h>
#include <ctype.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SFE_BMP180.h>
 
#define BME280_ADDRESS 0x76

SFE_BMP180 pressure;

//Parameter for GPS
int byteGPS=-1;
char linea[300] = "";
char comandoGPR[7] = "$GPRMC";
int cont=0;
int bien=0;
int conta=0;
int indices[13];

//Parameter for BME280
unsigned long int hum_raw,temp_raw,pres_raw;
signed long int t_fine;

uint16_t dig_T1;
 int16_t dig_T2;
 int16_t dig_T3;
uint16_t dig_P1;
 int16_t dig_P2;
 int16_t dig_P3;
 int16_t dig_P4;
 int16_t dig_P5;
 int16_t dig_P6;
 int16_t dig_P7;
 int16_t dig_P8;
 int16_t dig_P9;
 int8_t  dig_H1;
 int16_t dig_H2;
 int8_t  dig_H3;
 int16_t dig_H4;
 int16_t dig_H5;
 int8_t  dig_H6;

double temp_act = 0.0, press_act = 0.0,hum_act=0.0, t, hum;
signed long int temp_cal;
unsigned long int press_cal,hum_cal;
double alltitude;

//Parameter for SD
File datafile;

void setup() {
  uint8_t osrs_t = 1;             //Temperature oversampling x 1
  uint8_t osrs_p = 1;             //Pressure oversampling x 1
  uint8_t osrs_h = 1;             //Humidity oversampling x 1
  uint8_t mode = 3;               //Normal mode
  uint8_t t_sb = 5;               //Tstandby 1000ms
  uint8_t filter = 0;             //Filter off 
  uint8_t spi3w_en = 0;           //3-wire SPI Disable
  
  uint8_t ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | mode;
  uint8_t config_reg    = (t_sb << 5) | (filter << 2) | spi3w_en;
  uint8_t ctrl_hum_reg  = osrs_h;

  Serial.begin(9600);
  Serial1.begin(9600);
// Initialize a buffer for received data
  for (int i=0;i<300;i++)
  {       
    linea[i]=' ';
  }   
  Serial.println("UART");
//Start I2C Protocal
  Wire.begin();
    
  writeReg(0xF2,ctrl_hum_reg);
  writeReg(0xF4,ctrl_meas_reg);
  writeReg(0xF5,config_reg);
  
  readTrim();                    
  Serial.println("BME");
//Initialize SD card
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(10)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:    
    
    while (1) ;
  }
  Serial.println("card initialized.");
  
}

void loop() {
  Serial.println("GPS");
   //digitalWrite(ledPin, HIGH);
  byteGPS=Serial1.read();         // Read a byte of the serial port
  if (byteGPS == -1) {           // See if the port is empty yet
    delay(10); 
  } 
  else {
     // note: there is a potential buffer overflow here!
    linea[conta]=byteGPS;        // If there is serial port data, it is put in the buffer
    conta++;                      
    if (byteGPS==13){            // If the received byte is = to 13, end of transmission
      // note: the actual end of transmission is <CR><LF> (i.e. 0x13 0x10)
      //digitalWrite(ledPin, LOW); 
      cont=0;
      bien=0;
      // The following for loop starts at 1, because this code is clowny and the first byte is the <LF> (0x10) from the previous transmission.
      for (int i=1;i<7;i++){     // Verifies if the received command starts with $GPR
        if (linea[i]==comandoGPR[i-1]){
          bien++;
        }
      }
      if(bien==6){               // If yes, continue and process the data
        for (int i=0;i<300;i++){
          if (linea[i]==','){    // check for the position of the  "," separator
            // note: again, there is a potential buffer overflow here!
            indices[cont]=i;
            cont++;
          }
          if (linea[i]=='*'){    // ... and the "*"
            indices[12]=i;
            cont++;
          }
        }
        /*
        for (int i=0;i<12;i++){
          switch(i){
            case 0 :Serial.print("Time: ");break;
            //case 1 :Serial.print("Status (A=OK,V=KO): ");break;
            case 2 :Serial.print("Latitude: ");break;
            //case 3 :Serial.print("Direction (N/S): ");break;
            case 4 :Serial.print("Longitude: ");break;
            //case 5 :Serial.print("Direction (E/W): ");break;
            case 6 :Serial.print("Velocity in knots: ");break;
            case 7 :Serial.print("Heading in degrees: ");break;
            //case 8 :Serial.print("Date UTC (DdMmAa): ");break;
            //case 9 :Serial.print("Magnetic degrees: ");break;
            //case 10 :Serial.print("(E/W): ");break;
            //case 11 :Serial.print("Mode: ");break;
            //case 12 :Serial.print("Checksum: ");break;
          }
          for (int j=indices[i];j<(indices[i+1]-1);j++){
            Serial.print(linea[j+1]); 
          }
          Serial.println("");
        }*/
        //Serial.print("Time: ");
        datafile.close();
        datafile = SD.open("data.txt", FILE_WRITE);
        if (! datafile) {
          //Serial.println("error opening datalog.txt");
          // Wait forever since we cant write data
          while (1) ;
        } 
        for (int j=indices[0];j<(indices[1]-4);j++)
        {
            Serial.print(linea[j+1]);
            datafile.print(linea[j+1]); 
        }
        Serial.print(" XYZ= ");
        datafile.print(" XYZ= ");
        for (int j=indices[2];j<(indices[3]-1);j++)
        {
            Serial.print(linea[j+1]); 
            datafile.print(linea[j+1]); 
        }
        Serial.print(" , ");         
        datafile.print(" , ");         
        for (int j=indices[4];j<(indices[5]-1);j++)
        {
            Serial.print(linea[j+1]); 
            datafile.print(linea[j+1]); 
        }
          //Read BME280
      readData();
  
      temp_cal = calibration_T(temp_raw);
      press_cal = calibration_P(pres_raw);
      hum_cal = calibration_H(hum_raw);
      temp_act = (double)temp_cal / 100.0;
      press_act = (double)press_cal / 100.0;  
      hum_act = (double)hum_cal / 1024.0;
      alltitude = pressure.altitude(press_act,1013.25);
      Serial.print(" , ");
      Serial.print(alltitude,2);
      Serial.print(" T= ");
      Serial.print(temp_act,2);
      Serial.print(" H= ");
      Serial.println(hum_act,2); 
      datafile.print(" , ");
      datafile.print(alltitude,2);
      datafile.print(" T= ");
      datafile.print(temp_act,2);
      datafile.print(" H= ");
      datafile.println(hum_act,2); 
       
      datafile.flush();        
      }
      conta=0;                    // Reset the buffer
      for (int i=0;i<300;i++){    //  
        linea[i]=' ';             
      }                 
    }
  }
}

 //-------------------------------------------------------------------------------------------

void readTrim()
{
    uint8_t data[32],i=0;
    Wire.beginTransmission(BME280_ADDRESS);
    Wire.write(0x88);
    Wire.endTransmission();
    Wire.requestFrom(BME280_ADDRESS,24);
    while(Wire.available()){
        data[i] = Wire.read();
        i++;
    }
    
    Wire.beginTransmission(BME280_ADDRESS);
    Wire.write(0xA1);
    Wire.endTransmission();
    Wire.requestFrom(BME280_ADDRESS,1);
    data[i] = Wire.read();
    i++;
    
    Wire.beginTransmission(BME280_ADDRESS);
    Wire.write(0xE1);
    Wire.endTransmission();
    Wire.requestFrom(BME280_ADDRESS,7);
    while(Wire.available()){
        data[i] = Wire.read();
        i++;    
    }
    dig_T1 = (data[1] << 8) | data[0];
    dig_T2 = (data[3] << 8) | data[2];
    dig_T3 = (data[5] << 8) | data[4];
    dig_P1 = (data[7] << 8) | data[6];
    dig_P2 = (data[9] << 8) | data[8];
    dig_P3 = (data[11]<< 8) | data[10];
    dig_P4 = (data[13]<< 8) | data[12];
    dig_P5 = (data[15]<< 8) | data[14];
    dig_P6 = (data[17]<< 8) | data[16];
    dig_P7 = (data[19]<< 8) | data[18];
    dig_P8 = (data[21]<< 8) | data[20];
    dig_P9 = (data[23]<< 8) | data[22];
    dig_H1 = data[24];
    dig_H2 = (data[26]<< 8) | data[25];
    dig_H3 = data[27];
    dig_H4 = (data[28]<< 4) | (0x0F & data[29]);
    dig_H5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F);
    dig_H6 = data[31];   
}
void writeReg(uint8_t reg_address, uint8_t data)
{
    Wire.beginTransmission(BME280_ADDRESS);
    Wire.write(reg_address);
    Wire.write(data);
    Wire.endTransmission();    
}


void readData()
{
    int i = 0;
    uint32_t data[8];
    Wire.beginTransmission(BME280_ADDRESS);
    Wire.write(0xF7);
    Wire.endTransmission();
    Wire.requestFrom(BME280_ADDRESS,8);
    while(Wire.available()){
        data[i] = Wire.read();
        i++;
    }
    pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    hum_raw  = (data[6] << 8) | data[7];
}


signed long int calibration_T(signed long int adc_T)
{
    
    signed long int var1, var2, T;
    var1 = ((((adc_T >> 3) - ((signed long int)dig_T1<<1))) * ((signed long int)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((signed long int)dig_T1)) * ((adc_T>>4) - ((signed long int)dig_T1))) >> 12) * ((signed long int)dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T; 
}

unsigned long int calibration_P(signed long int adc_P)
{
    signed long int var1, var2;
    unsigned long int P;
    var1 = (((signed long int)t_fine)>>1) - (signed long int)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11) * ((signed long int)dig_P6);
    var2 = var2 + ((var1*((signed long int)dig_P5))<<1);
    var2 = (var2>>2)+(((signed long int)dig_P4)<<16);
    var1 = (((dig_P3 * (((var1>>2)*(var1>>2)) >> 13)) >>3) + ((((signed long int)dig_P2) * var1)>>1))>>18;
    var1 = ((((32768+var1))*((signed long int)dig_P1))>>15);
    if (var1 == 0)
    {
        return 0;
    }    
    P = (((unsigned long int)(((signed long int)1048576)-adc_P)-(var2>>12)))*3125;
    if(P<0x80000000)
    {
       P = (P << 1) / ((unsigned long int) var1);   
    }
    else
    {
        P = (P / (unsigned long int)var1) * 2;    
    }
    var1 = (((signed long int)dig_P9) * ((signed long int)(((P>>3) * (P>>3))>>13)))>>12;
    var2 = (((signed long int)(P>>2)) * ((signed long int)dig_P8))>>13;
    P = (unsigned long int)((signed long int)P + ((var1 + var2 + dig_P7) >> 4));
    return P;
}

unsigned long int calibration_H(signed long int adc_H)
{
    signed long int v_x1;
    
    v_x1 = (t_fine - ((signed long int)76800));
    v_x1 = (((((adc_H << 14) -(((signed long int)dig_H4) << 20) - (((signed long int)dig_H5) * v_x1)) + 
              ((signed long int)16384)) >> 15) * (((((((v_x1 * ((signed long int)dig_H6)) >> 10) * 
              (((v_x1 * ((signed long int)dig_H3)) >> 11) + ((signed long int) 32768))) >> 10) + (( signed long int)2097152)) * 
              ((signed long int) dig_H2) + 8192) >> 14));
   v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((signed long int)dig_H1)) >> 4));
   v_x1 = (v_x1 < 0 ? 0 : v_x1);
   v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
   return (unsigned long int)(v_x1 >> 12);   
}

int Filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0)
  {
    flag_first = 1;

    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    
    i = sum / 10.0;
    return i;
  }
}

