#include "qqqDALI.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_attr.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/uart.h"

#include "esp_task_wdt.h"

Dali dali;

//esp32 specific
#define VR_pin GPIO_NUM_34
#define BUTTON_pin GPIO_NUM_26
#define TX_PIN GPIO_NUM_16
#define RX_PIN GPIO_NUM_4
#define TIMER_UPDATES_PER_SECOND 9600

volatile int lux;
int buttonPushCounter = 0;
volatile bool buttonState = LOW;

union{
  int Data9bytes;
  struct{
    char byte1;
    char byte2;
    char byte3;
    char byte4;
    char byte5;
    char byte6;
    char byte7;
    char byte8;
    char byte9;
    // char byte10;
  }Byte;
}Data;

uint8_t bus_is_high() {  //is bus asserted
  return digitalRead(RX_PIN);  
}

//assert bus
void bus_set_low() {
  digitalWrite(TX_PIN,LOW); //diy slow version (DIY: do it yourself)
}

//release bus
void bus_set_high() {
  digitalWrite(TX_PIN,HIGH); //diy slow version
}

void IRAM_ATTR dali_timer_callback(void* arg)
{
  dali.timer();
}

void IRAM_ATTR isr(){
  buttonState = !buttonState;
}

void menu_blink() {
  Serial.println("Running: Blinking all lamps");
  for(uint8_t i=0;i<5;i++) {
    dali.set_level(254);
    Serial.print(".");
    delay(500);
    dali.set_level(0);
    Serial.print(".");
    delay(500);
  }
  Serial.println();
}

void bus_init() {
  //setup rx pin
  pinMode(RX_PIN, INPUT);

  //setup tx pin
  pinMode(TX_PIN, OUTPUT);
  
//   //setup tx timer interrupt
  const esp_timer_create_args_t my_timer_args = {
        .callback = &dali_timer_callback, 
        .arg = nullptr,
        .dispatch_method = esp_timer_dispatch_t::ESP_TIMER_TASK, 
        .name = "dali_timer_callback", 
        .skip_unhandled_events = false 
    };

    esp_timer_handle_t timer_handler;
    ESP_ERROR_CHECK(esp_timer_create(&my_timer_args, &timer_handler));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, 1000000 / TIMER_UPDATES_PER_SECOND));  //interrupt every 1 000 000 / 9600 = 104,... (us)
}

void menu() {
  Serial.println("----------------------------");
  Serial.println("1 Blink all lamps");
  Serial.println("2 Scan short addresses");
  // Serial.println("3 Commission short addresses");
  Serial.println("3 Commission short addresses (VERBOSE)");
  Serial.println("4 Delete short addresses");
  Serial.println("5 Read memory bank");
  Serial.println("6 Set group (open README.txt for more details)");
  Serial.println("----------------------------");  
}

void menu_scan_short_addr() {
  Serial.println("Running: Scan all short addresses");
  uint8_t sa;
  uint8_t cnt = 0;
  for(sa = 0; sa<64; sa++) {
    int16_t rv = dali.cmd(DALI_QUERY_STATUS,sa);
    if(rv>=0) {
      cnt++;
      Serial.print("short address=");
      Serial.print(sa);
      Serial.print(" status=0x");
      Serial.print(rv,HEX);
      Serial.print(" minLevel=");
      Serial.print(dali.cmd(DALI_QUERY_MIN_LEVEL,sa));
      Serial.print("   flashing");
      for(uint8_t i=0;i<5;i++) {
        dali.set_level(254,sa);
        Serial.print(".");
        delay(500);
        dali.set_level(0,sa);
        Serial.print(".");
        delay(500);
      }
      Serial.println();
    }else if (-rv != DALI_RESULT_NO_REPLY) {
      Serial.print("short address=");
      Serial.print(sa);
      Serial.print(" ERROR=");
      Serial.println(-rv);     
    }
  }  
  Serial.print("DONE, found ");Serial.print(cnt);Serial.println(" short addresses");
}

void menu_commission_debug(){
  Serial.println("Running: Commission (VERBOSE)");
  Serial.println("Might need a couple of runs to find all lamps ...");
  Serial.println("Be patient, this takes a while ...");
  uint8_t cnt = debug_commission(0xff); //init_arg=0b11111111 : all without short address  
  Serial.print("DONE, assigned ");Serial.print(cnt);Serial.println(" new short addresses");
}

void menu_delete_short_addr() {
  Serial.println("Running: Delete all short addresses");
  //remove all short addresses
  dali.cmd(DALI_DATA_TRANSFER_REGISTER0,0xFF);
  dali.cmd(DALI_SET_SHORT_ADDRESS, 0xFF);
  Serial.println("DONE delete");
}

//init_arg=11111111 : all without short address
//init_arg=00000000 : all 
//init_arg=0AAAAAA1 : only for this shortadr
//returns number of new short addresses assigned
uint8_t debug_commission(uint8_t init_arg) {
  uint8_t cnt = 0;
  uint8_t arr[64];
  uint8_t sa;
  for(sa=0; sa<64; sa++) arr[sa]=0;

  dali.cmd(DALI_INITIALISE,init_arg);
  dali.cmd(DALI_RANDOMISE,0x00);
  //need 100ms pause after RANDOMISE, scan takes care of this...
  
  //find used short addresses (run always, seems to work better than without...)
  Serial.println("Find existing short adr");
  for(sa = 0; sa<64; sa++) {
    int16_t rv = dali.cmd(DALI_QUERY_STATUS,sa);
    if(rv>=0) {
      if(init_arg!=0b00000000) arr[sa]=1; //remove address from list if not in "all" mode
      Serial.print("sortadr=");
      Serial.print(sa);
      Serial.print(" status=0x");
      Serial.print(rv,HEX);
      Serial.print(" minLevel=");
      Serial.println(dali.cmd(DALI_QUERY_MIN_LEVEL,sa));
    }
  }
  Serial.println("Find random adr");
  while(1) {
    uint32_t adr = dali.find_addr();
    if(adr>0xffffff) break;
    Serial.print("found=");
    Serial.println(adr,HEX);
  
    //find available address
    for(sa=0; sa<64; sa++) {
      if(arr[sa]==0) break;
    }
    if(sa>=64) break;
    arr[sa] = 1;
    cnt++;
 
    Serial.print("program short adr=");
    Serial.println(sa);
    dali.program_short_address(sa);  
    Serial.print("read short adr=");
    Serial.println(dali.query_short_address());

    dali.cmd(DALI_WITHDRAW,0x00);
  }
  
  dali.cmd(DALI_TERMINATE,0x00);
  return cnt;
}

void menu_read_memory() {
  Serial.println("Running: Scan all short addresses");
  uint8_t sa;
  uint8_t cnt = 0;
  for(sa = 0; sa<64; sa++) {
    int16_t rv = dali.cmd(DALI_QUERY_STATUS,sa);
    if(rv>=0) {
      cnt++;
      Serial.print("\nshort address ");
      Serial.println(sa);
      Serial.print("status=0x");
      Serial.println(rv,HEX);
      Serial.print("minLevel=");
      Serial.println(dali.cmd(DALI_QUERY_MIN_LEVEL,sa));

      dali_read_memory_bank_verbose(0,sa);
      
    }else if (-rv != DALI_RESULT_NO_REPLY) {
      Serial.print("short address=");
      Serial.print(sa);
      Serial.print(" ERROR=");
      Serial.println(-rv);     
    }
  }  
  Serial.print("DONE, found ");Serial.print(cnt);Serial.println(" short addresses"); 
}

uint8_t dali_read_memory_bank_verbose(uint8_t bank, uint8_t adr) {
  uint16_t rv;

  if(dali.set_dtr0(0, adr)) return 1;
  if(dali.set_dtr1(bank, adr)) return 2;
    
  //uint8_t data[255];
  uint16_t len = dali.cmd(DALI_READ_MEMORY_LOCATION, adr);
  Serial.print("memlen=");
  Serial.println(len);
  for(uint8_t i=0;i<len;i++) {
    int16_t mem = dali.cmd(DALI_READ_MEMORY_LOCATION, adr);
    if(mem>=0) {
      //data[i] = mem;
      Serial.print(i,HEX);
      Serial.print(":");
      Serial.print(mem);
      Serial.print(" 0x");
      Serial.print(mem,HEX);
      Serial.print(" ");
      if(mem>=32 && mem <127) Serial.print((char)mem);   
      Serial.println(); 
    }else if(mem!=-DALI_RESULT_NO_REPLY) {
      Serial.print(i,HEX);
      Serial.print(":err=");
      Serial.println(mem);   
    }
  }

  uint16_t dtr0 = dali.cmd(DALI_QUERY_CONTENT_DTR0,adr); //get DTR value
  if(dtr0 != 255) return 4;
}

void setup() {
  Serial.begin(115200);
  Serial.println("DALI Commissioning Demo");
  pinMode(VR_pin, INPUT);
  pinMode(BUTTON_pin, INPUT);

  dali.begin(bus_is_high, bus_set_high, bus_set_low);
  bus_init();  

  attachInterrupt(BUTTON_pin, isr, FALLING);
  menu();
}

void loop() {
  if (buttonState == LOW) {
    lux = analogRead(VR_pin);
    delay(2);
    int vla = map(lux, 0, 4095, 8, 254); 
    dali.set_level(vla);
    Serial.print(".");
    delay(500);
    Serial.print("lux = ");Serial.println(vla);
  }
  else{
    dali.cmd(DALI_OFF, 0xFF);
    Serial.print(".");
    delay(500);
    Serial.println("I'M OFF");
    while (Serial.available() > 0) {   
      menu();
      int incomingByte = Serial.read(); 
      switch(incomingByte) {
        case '1': menu_blink(); menu(); break;    
        case '2': menu_scan_short_addr(); menu(); break;
        case '3': menu_commission_debug(); menu(); break;
        case '4': menu_delete_short_addr(); menu(); break;
        case '5': menu_read_memory(); menu(); break;
        case '6': set_group(); menu(); break;
      }
    }
  }
}

void set_group(){
  int lamp_addr;
  int gr;
  int number_of_lamp;

  Data.Byte.byte1 = Serial.read();
  Data.Byte.byte2 = Serial.read();
  Data.Byte.byte3 = Serial.read();
  Data.Byte.byte4 = Serial.read();
  Data.Byte.byte5 = Serial.read();
  Data.Byte.byte6 = Serial.read();
  Data.Byte.byte7 = Serial.read();
  Data.Byte.byte8 = Serial.read();
  Data.Byte.byte9 = Serial.read();

  // //choose lamp address first
  Serial.print("the short address for the first lamp in the series of lamps [0..63]: ");
    switch (Data.Byte.byte2){
      case '0': lamp_addr = Data.Byte.byte3 - 48; break;
      case '1': lamp_addr = Data.Byte.byte3 - 48 + 10; break;
      case '2': lamp_addr = Data.Byte.byte3 - 48 + 20; break;
      case '3': lamp_addr = Data.Byte.byte3 - 48 + 30; break;
      case '4': lamp_addr = Data.Byte.byte3 - 48 + 40; break;
      case '5': lamp_addr = Data.Byte.byte3 - 48 + 50; break;
      case '6': lamp_addr = Data.Byte.byte3 - 48 + 60; break;
     }
  Serial.println(lamp_addr);

  //number of lamps
  Serial.print("The number of series of lamps [1..63]: ");
  switch (Data.Byte.byte5){
      case '0': number_of_lamp = Data.Byte.byte6 - 48; break;
      case '1': number_of_lamp = Data.Byte.byte6 - 48 + 10; break;
      case '2': number_of_lamp = Data.Byte.byte6 - 48 + 20; break;
      case '3': number_of_lamp = Data.Byte.byte6 - 48 + 30; break;
      case '4': number_of_lamp = Data.Byte.byte6 - 48 + 40; break;
      case '5': number_of_lamp = Data.Byte.byte6 - 48 + 50; break;
      case '6': number_of_lamp = Data.Byte.byte6 - 48 + 60; break;
     }
  Serial.println(number_of_lamp);

  // //now we choose group
  Serial.print("Choose group [0..15]: ");
  if (Data.Byte.byte8 == '0'){gr = Data.Byte.byte9 - 48;}else{gr = Data.Byte.byte9 - 48 + 10;}
  Serial.println(gr);

   //add each lamp to the group
  Serial.println("is adding . . .  ");
  for (int i=lamp_addr; i< (number_of_lamp + lamp_addr); i++){
    dali.cmd(DALI_ADD_TO_GROUP0 + gr, i);
  }

    //blink that group for testing
  Serial.println("Blinking group ");
  gr = gr + 64; // 64 = 0b 0100 0000 (group address)
  for(uint8_t i=0;i<5;i++) {
    dali.set_level(254,gr);
    Serial.print(".");
    delay(500);
    dali.set_level(0,gr);
    Serial.print(".");
    delay(500);
  }
}