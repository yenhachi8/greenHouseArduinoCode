/*
  ---------感測器腳位配置----------
  A0:溫濕度感測器
  A1:光感測器
  接腳P16:土壤感測器
  D2:超音波感測器

  P6 : 風扇 const int relayFan = 6;
  P7 : LED const int relayLed = 7;
  P8 : 抽水幫浦 const int relayPump = 8;
  P9 : 蜂鳴器 const int relayBuzzer = 9;
*/
#include "DHT.h"
#include <LWiFi.h>
#include <PubSubClient.h>
#include "Ultrasonic.h"
#include <math.h>
#include <String.h>
#include <stdlib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

//網路設定

char ssid[] = ""; //*改自己wifi
char pass[] = "";       //*改自己wifi密碼

int status = WL_IDLE_STATUS;
IPAddress server(140, 127, 196, 114); //*修改成欲作為伺服器的ip
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void printWifiStatus();
char receivedChar;

const char *topicPub = "7697toSQL";  //*修改成自己設定的主題
const char *topicSub = "phoneTo7697"; //*修改成自己訂閱的主題

String strJson = "";
char charJson[100];

const char *topicPubToPhone = "7697toPhone";
String strPubToPhone = "";


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tw.pool.ntp.org", 28800, 60000);

//溫濕度設定
#define DHTPIN A0
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float t; // 暫存溫度
float h; // 暫存
const int relayFan = 6;
String temp;
String humi;


//光感測器設定
#define LIGHT_SENSOR A1
const int relayLed = 7;
float Rsensor;
float lightValue;
String light;
char Light[15];

//土壤濕度感測器設定
#define MOISTURE A2
const int relayPump = 8;
float Msensor;
float mValue;
String mois;
char Mois[15];

//超音波設定
Ultrasonic ultrasonic(2); //*係數為欲設定之接腳 D2
long RangeInCentimeters;
const int relayBuzzer = 9;
String ultra;
char Ultra[15];

//其他
unsigned long prevMillis = 0; //記錄時間
unsigned long prevMillis_2 = 0;
const long interval_2 = 5000;
int timeHour;              //記錄標準時間，用來做燈亮控制
const long interval = 1000; //用來計算時間，1000毫秒=1秒


//預設系統是 ”自動“
char charPubToPhone[1400];
char ModeChar[80]; //顯示狀態到手機
char helpChar[500];
char manualHelpChar[200];
char instructChar[20];           //儲存指令，用來做比較控制使用
char thresholdNoteChar[250];
char showThresholdChar[150];
char thresholdTransChar[140];

bool manualHelpFlag = 1;
bool stateAutoManual = 0;    //=0自動 =1手動
bool modeFlag = 1;            //=0重新設定 =1設定完成
bool helpFlag = 1;          //
bool thresholdSetFlag = 1;       //設定自動模式門檻 預設為已設定好
bool thresholdInputFlag = 1;
bool showThresholdFlag = 1;

bool timeStartValueFlag = 1;
bool timeEndValueFlag = 1;

bool lightValueFlag = 1;
bool humiValueFlag = 1;
bool moisValueFlag = 1;
bool ultraValueFlag = 1;


//傳至ModeChar 顯示狀態
String ledState = "";
String fanState = "";
String pumpState = "";
String buzzerState = "";

//設定啟動的條件 ex buzzer啟動的條件是<15
int ledStart = 30;
int fanStartByTemperature = 30;
int fanStartByHumidity = 90;
int pumpStart = 20;
int buzzerStart = 15;
//設定偵測的時間
int timeStart = 8;
int timeEnd = 17;



void setup()
{
  Serial.begin(9600);
  dht.begin();
  pinMode(DHTPIN, INPUT);
  pinMode(MOISTURE, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);

  pinMode(relayLed, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(relayPump, OUTPUT);
  pinMode(relayBuzzer, OUTPUT);
//  fanState = "x";
//  ledState = "x";
//  pumpState = "x";
//  buzzerState = "x";
  //  fanState = "Fan OFF";
  //  ledState = "Led OFF";
  //  pumpState = "Pump OFF";
  //  buzzerState = "Buzzer OFF";

  client.setServer(server, 18883);
  client.setCallback(callback);
  timeClient.begin();

  strcpy(ModeChar, "輸入\"modeset\"來開始。\n輸入 \"help\" 得到更多說明。");

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
}
void callback(char *topic, byte *payload, unsigned int length)
{
  /*
    當訊息被傳送後就會啟動這個函數
    函式傳入三個參數，字元型態的topic、位元型態的payload、整數型態的length
  */
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // Message arrived []
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  memcpy(instructChar, payload, length);
  instructChar[length] = '\0'; //在字尾填入結束符號

  //    兩個字串比較
  if (strcmp(instructChar, "help") == 0)
  {
    helpFlag = !helpFlag;
    //help內容（在void loop裡）
  }
  if (strcmp(instructChar, "modeset") == 0)
  {
    //Serial.println("Please input mode: \"auto\", \"manual\"");
    //strcpy(ModeChar, "Setting...Please Input \"auto\" or \"manual\"!\n");
    modeFlag = !modeFlag;
  }
  if (strcmp(instructChar, "tset") == 0)
  {
    thresholdSetFlag = !thresholdSetFlag;
    thresholdInputFlag = !thresholdInputFlag;
  }
  if(strcmp(instructChar,"manualhelp")==0)
  {
    manualHelpFlag = !manualHelpFlag;
  }
  if(strcmp(instructChar,"showthreshold")==0)
  {
    showThresholdFlag = !showThresholdFlag;
  }

  Serial.println();
}

void loop()
{

  timeClient.update();
  timeHour = ((timeClient.getFormattedTime()).substring(0, 2)).toInt();

  //取得時間前兩位數、轉成整數型態、存入變數timeHour
  { //設定 help
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();

  }

  if ((millis() - prevMillis) > interval)
  {
    //不斷地傳資料
    { //感測器設定、傳送資料
      Serial.println("-----------------------------迴圈ON始---------------------------------");
      Serial.println("test");
      Serial.println(timeHour);

      //溫濕度感測
      t = dht.readTemperature();
      h = dht.readHumidity();
      temp = String(t);
      humi = String(h);
      if (isnan(t) || isnan(h))
      {
        Serial.println("Failed to read from DHT");
        t = 0;
        h = 0;
      }

      //光感測
      lightValue = analogRead(LIGHT_SENSOR);
      Rsensor = (float)(((lightValue / 3400) * 100));
      light = String(Rsensor);

      //超音波感測
      RangeInCentimeters = ultrasonic.MeasureInCentimeters();
      ultra = String(RangeInCentimeters);

      //土壤濕度感測
      mValue = analogRead(MOISTURE);
      Msensor = (float)(100 - ((mValue / 4095) * 100));
      mois = String(Msensor);
      
      //變數t取到小數點第二位，再轉換成c語言格式
      //c_str()函式返回一個指向正規C字串的指標, 內容與本string串相同


    }
    if (modeFlag) {

    }


    if (!modeFlag)
    {

      digitalWrite(relayLed, LOW);
      digitalWrite(relayFan, LOW);
      digitalWrite(relayPump, LOW);
      digitalWrite(relayBuzzer, LOW);
      fanState = "風扇： OFF";
      ledState = "Led OFF";
      pumpState = "抽水幫浦： OFF";
      buzzerState = "蜂鳴器： OFF";
      strcpy(ModeChar, "設定中...請輸入\"auto\" 或 \"manual\"! \n輸入 \"help\" 取得說明。\n");
      if (strcmp(instructChar, "auto") == 0) //相比=0 代表兩個字串相同s
      {
        modeFlag = 1;
        stateAutoManual = 0;

        strcpy(ModeChar, instructChar);
        Serial.println("Turn Auto Mode!");
      }
      if (strcmp(instructChar, "manual") == 0)
      {
        modeFlag = 1;
        stateAutoManual = 1;

        strcpy(ModeChar, instructChar);
        Serial.println("Turn Manual Mode!");
      }

      if (!modeFlag)
      {
        Serial.println("Please input \"auto\" or \"manual\"! ");
      }
    }




    if (stateAutoManual == 0 && modeFlag ) //自動模式
    {
      fanState = "風扇： 無值";
      ledState = "led：  無值";
      pumpState = "抽水幫浦： 無值";
      //buzzerState = "無值";
      //超音波
      if(timeEnd-timeStart<0){
        timeEnd+=24;
      }
      {
        if (RangeInCentimeters < buzzerStart)
        {
          digitalWrite(relayBuzzer, HIGH);
          buzzerState = "蜂鳴器： ON";
        }
        if (RangeInCentimeters >= buzzerStart)
        {
          digitalWrite(relayBuzzer, LOW);
          buzzerState = "蜂鳴器： OFF";
        }
      }
      if (timeHour >= timeStart && timeHour < timeEnd)
        //白天的時候才啟動 (預設8~17點)
        //到了晚上就不偵測
        //超音波再回圈之外，（24小時偵測）
      {
        Serial.println("Mode:Auto");

        //濕度感測
        if (h > fanStartByHumidity  )
        {
          digitalWrite(relayFan, HIGH);
          fanState = "風扇： ON";

        }
        if (h <= fanStartByHumidity )
        {
          digitalWrite(relayFan, LOW);
          fanState = "風扇： OFF";
        }
        //光感測
        {

          if (Rsensor < ledStart)
          {
            digitalWrite(relayLed, HIGH);

            ledState = "Led ON";
          }
          if (Rsensor >= ledStart)
          {
            digitalWrite(relayLed, LOW);

            ledState = "Led OFF";
          }
        }
        //土壤濕度
        {
          if (Msensor < pumpStart)
          {
            digitalWrite(relayPump, HIGH);
            pumpState = "抽水幫浦： ON";
          }
          if (Msensor >= pumpStart)
          {
            digitalWrite(relayPump, LOW);
            pumpState = "抽水幫浦： OFF";
          }
        }
      }
      
    }



    if (stateAutoManual == 1 && modeFlag == 1) //手動模式
    {
      Serial.println("Mode:Manual");
      //LED燈ONOFF
      if (strcmp(instructChar, "led_on") == 0)
      {
        digitalWrite(relayLed, HIGH);
        ledState = "Led ON";
      }
      if (strcmp(instructChar, "led_off") == 0)
      {
        digitalWrite(relayLed, LOW);
        ledState = "Led OFF";
      }
      //風扇ONOFF
      if (strcmp(instructChar, "fan_on") == 0)
      {
        digitalWrite(relayFan, HIGH);
        fanState = "風扇： ON";
      }
      if (strcmp(instructChar, "fan_off") == 0)
      {
        digitalWrite(relayFan, LOW);
        fanState = "風扇： OFF";
      }
      //幫浦ONOFF
      if (strcmp(instructChar, "pump_on") == 0)
      {
        digitalWrite(relayPump, HIGH);
        pumpState = "抽水幫浦： ON";
      }
      if (strcmp(instructChar, "pump_off") == 0)
      {
        digitalWrite(relayPump, LOW);
        pumpState = "抽水幫浦： OFF";
      }
      //蜂鳴器ONOFF
      if (strcmp(instructChar, "buzzer_on") == 0)
      {
        digitalWrite(relayBuzzer, HIGH);
        buzzerState = "蜂鳴器： ON";
      }
      if (strcmp(instructChar, "buzzer_off") == 0)
      {
        digitalWrite(relayBuzzer, LOW);
        buzzerState = "蜂鳴器： OFF";
      }
    }

    Serial.println("-----------狀態列start----------");
    Serial.print("StateAntoManual: ");
    Serial.println(stateAutoManual);
    Serial.print("modeFlag: ");
    Serial.println(modeFlag);

    Serial.print("thresholdSetFlag: ");
    Serial.println(thresholdSetFlag);
    Serial.println("-----------狀態列end----------");

    if (!helpFlag)
    {
      strcpy(helpChar, "\n您好,此溫室系統共有5個基礎指令,\
\n\"modeset\", \"auto\", \"manual\",\"tset\",\"help\",\"manualhelp\",\"showthreshold\".\
\n\"modeset\": 設定模式。\
\n\"auto\": 啟動自動模式。\
\n\"manual\": 啟動手動模式。\
\n\"tset\":設定在自動模式時主動元件（如風扇、Led燈）的啟動條件。\
\n\"timeset\": 設定在自動模式時啟動的時間。\
\n\"manualhelp\": 說明手動模式的指令。\
\n\"showthreshold\": 顯示起動條件之值。\
\n，再一次輸入 \"help\"以退出\n。");

    }
    if (helpFlag ) {
      strcpy(helpChar, "");
    }
    if(!manualHelpFlag){
      strcpy(manualHelpChar, "\n在手動模式下共有以下8個指令\
\n控制LED燈開關\
\nled_on\
\nled_off\
\n控制風扇開關\
\nfan_on\
\nfan_off\
\n控制蜂鳴器開關\
\nbuzzer_on\
\nbuzzer_off\
\n控制抽水幫浦開關\
\npump_on\
\npump_off\
\n再輸入一次\"manualhelp\"即可退出\n");
    }
    if(manualHelpFlag){
      strcpy(manualHelpChar, "");
    }     
    if(!showThresholdFlag){
      sprintf(thresholdTransChar, "\
\nLed啟動值 : %s,\
\n風扇      : %s,\
\n幫浦      : %s,\
\n蜂鳴器    :%s\
\n啟動時間   :%s\
\n結束時間   :%s\
\n",String(ledStart).c_str(),String(fanStartByHumidity).c_str(),String(pumpStart).c_str() ,String(buzzerStart).c_str(),String(timeStart).c_str(),String(timeEnd).c_str());
      
      strcpy(showThresholdChar, thresholdTransChar);
    }
    if(showThresholdFlag){
      strcpy(showThresholdChar, "");
    }

    if (!thresholdSetFlag )
    {

      if (!thresholdInputFlag) {
        Serial.println("----Threshold setting start----");
        strcpy(thresholdNoteChar, "\n\n請輸入欲改變之啟動值: 濕度：\"humi\"(風扇), 亮度：\"light\"（LED）, 土壤濕度：\"mois\"（幫浦）, 超音波：\"ultra\"（蜂鳴器）, 開始時間：\"timestart\", 結束時間：\"timeend\"\n再一次輸入 \"tset\" 即可退出\n");
        Serial.println("請輸入要改變的項目：");
        //
      }

      if (strcmp(instructChar, "humi") == 0) {
        Serial.println("\nPlease input a Humidity value to start Fan!");
        strcpy(thresholdNoteChar, "\n\n請輸入一整數值：(濕度)\n");
        humiValueFlag = 0;
        thresholdInputFlag = 1;

      }
      if (strcmp(instructChar, "light") == 0) {
        Serial.println("\nPlease input a light threshold value to start LED!");
        strcpy(thresholdNoteChar, "\n\n請輸入一整數值：（亮度）\n");
        lightValueFlag = 0;
        thresholdInputFlag = 1;
      }
      if (strcmp(instructChar, "mois") == 0) {
        Serial.println("\nPlease input a Soil Moisture value to start 抽水幫浦： !");
        strcpy(thresholdNoteChar, "\n請輸入一整數值：（土壤濕度）\n");
        moisValueFlag = 0;
        thresholdInputFlag = 1;

      }
      if (strcmp(instructChar, "ultra") == 0) {
        Serial.println("\nPlease input a UltraSonic distance to start 蜂鳴器：!");
        strcpy(thresholdNoteChar, "\n\n請輸入一整數值：（超音波感測器之距離）\n");
        ultraValueFlag = 0 ;
        thresholdInputFlag = 1;

      }
      if (strcmp(instructChar, "timestart") == 0) {
        Serial.println("\nPlease input a timestart value to set when to start auto !");
        strcpy(thresholdNoteChar, "\n\n請輸入一整數值：（開始時間）\n");
        timeStartValueFlag = 0;
        thresholdInputFlag = 1;

      }
      if (strcmp(instructChar, "timeend") == 0) {
        Serial.println("\nPlease input a timeend value to set when to end auto !");
        strcpy(thresholdNoteChar, "\n\n請輸入一整數值：（結束時間）\n");
        timeEndValueFlag = 0;
        thresholdInputFlag = 1;

      }
      if (!lightValueFlag ) {
        ledStart = atoi(instructChar);

        if (ledStart) {
          lightValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }
      if (!humiValueFlag ) {
        fanStartByHumidity = atoi(instructChar);

        if (fanStartByHumidity) {
          humiValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }
      if (!moisValueFlag ) {
        pumpStart = atoi(instructChar);

        if (pumpStart) {
          moisValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }
      if (!ultraValueFlag ) {
        buzzerStart = atoi(instructChar);

        if (buzzerStart) {
          lightValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }
      if (!timeStartValueFlag ) {
        timeStart = atoi(instructChar);

        if (timeStart) {
          timeStartValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }
      if (!timeEndValueFlag ) {
        timeEnd = atoi(instructChar);

        if (timeEnd) {
          timeEndValueFlag = 1;
          thresholdInputFlag = 0;
        }
      }

      Serial.println("----Threshold setting end----");
    }
    if (thresholdSetFlag ) {
      strcpy(thresholdNoteChar, "");
    }
    
    
    strJson = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + ",\"led\":" + String(light) +
              ",\"moisture\":" + String(mois) + ",\"ultrasonic\":" + String(ultra) + "}";

    strJson.toCharArray(charJson, 100); //將strJson轉成字元陣列並放入charJson裡，大小為50

    strPubToPhone = "Mode state:" + String(ModeChar) + String(helpChar) + String(thresholdNoteChar) \
                    + String(manualHelpChar)+String(showThresholdChar)+"\n溫度(ºC): " + String(t) + ",\n濕度(%): " + String(h) \
                    +",\n亮度(%): " + String(light) + "%,\n超音波距離(cm): " + String(ultra) + ",\n土壤濕度(%): " + String(mois)\
                    + "%,\n" + String(ledState) + "\n" + String(fanState) + "\n" + String(pumpState) + "\n" + String(buzzerState) ;


    strPubToPhone.toCharArray(charPubToPhone, sizeof(charPubToPhone));

    Serial.println("--------------pubToPhoneON始-----------------");
    Serial.println("test");
    Serial.println(charPubToPhone);
    client.publish(topicPubToPhone, charPubToPhone);
    Serial.println("--------------pubToPhone結束-----------------");
    Serial.println("--------------pubToMQTTON始-----------------");
    Serial.println("strJson");
    Serial.println(strJson);
    Serial.println("--------------pubToMQTT結束-----------------");
    Serial.println("-----------------------------迴圈結束---------------------------------");
    prevMillis = millis();
  }

  //要傳到資料庫的資料，設定"interval_"秒傳一次
  if ((millis() - prevMillis_2) > interval_2)
  {
    client.publish(topicPub, charJson);
    prevMillis_2 = millis();
  }

}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
void reconnect()
{

  //當沒連接時，顯示嘗試連接，若成功，就顯示成功，否則顯示失敗
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("arduinoClient"))
    {
      Serial.println("connected");
      client.subscribe(topicSub, 0);
  
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(3000);
    }
  }
}
