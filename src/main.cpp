#include <Arduino.h>
#include "esp32_digital_led_lib.h"
#include "esp32-hal-log.h"
static const int nofCh=8;
static const char ledPins[8]={32,33,25,26 ,27,14,13,16  };
static const int nofLed[nofCh]={50*4+1/*天*/,50*5+1/*左*/,50*4+1/*奥*/,50*5+1/*右*/, 50*5+1/*入り口*/,50*2+1/*床*/,2*50+1/*風呂1*/,2*50+1/*風呂2*/};
static  uint8_t vPos[nofCh][251]={0};
static const  int nofLedMax=281;

static strand_t hLed[nofCh];
static int gCount=0;
static const int gCountMax=4000;
void setVPos(){
  //天
  for(int i=0;i<nofLed[0];i++)vPos[0][i]=0;
  //左
  for(int i=0;i<nofLed[1];i++){
    vPos[1][i]=255*i/(nofLed[1]-1);
  }
  //奥
  for(int i=0;i<nofLed[2];i++){
    vPos[2][i]=255*i/(250);
  }
  //右
  for(int i=0;i<nofLed[3];i++){
    vPos[3][i]=255*i/(nofLed[3]-1);
  }
  //入口
  for(int i=0;i<nofLed[4];i++){
    vPos[4][i]=255*i/(nofLed[4]-1);
  }
  //床
  for(int i=0;i<nofLed[5];i++){
    vPos[5][i]=255;
  }
  //風呂1
  for(int i=0;i<nofLed[6];i++){
    vPos[6][i]=200+55*i/(nofLed[6]-1);
  }
  //風呂2
  for(int i=0;i<nofLed[6];i++){
    vPos[7][i]=200+55*i/(nofLed[7]-1);
  }
}
void initLed(void){
  for(int i=0;i<nofCh;i++){
    log_i("ch:%d, gpio:%d,nofLed:%d",i,ledPins[i],nofLed[i]);
    hLed[i].rmtChannel = i; 
    hLed[i].gpioNum = ledPins[i] ;
    hLed[i].ledType = LED_WS2812B_V3; 
    hLed[i].brightLimit = 255; 
    hLed[i].numPixels = nofLed[i];
    hLed[i].pixels = nullptr;
    hLed[i]._stateVars = nullptr;
    pinMode (ledPins[i], OUTPUT);
  }
  digitalLeds_initStrands(hLed,nofCh);
  for(int i=0;i<nofCh;i++){ 
    digitalLeds_resetPixels(&(hLed[i]) );
  }
}
void setup() {
  Serial.begin(115200);
  setVPos();
  initLed();
}
void fillSome(strand_t *led,pixelColor_t col){
  for(int i=0;i< led->numPixels ;i++){
    led->pixels[i]=col;
  }
}
void fade(uint8_t rate,strand_t *led){
  for(int i=0;i< led->numPixels ;i++){
    led->pixels[i].r = (uint16_t)(led->pixels[i].r)*rate/255;
    led->pixels[i].g = (uint16_t)(led->pixels[i].g)*rate/255;
    led->pixels[i].b = (uint16_t)(led->pixels[i].b)*rate/255;    
  }
}
void addDark(strand_t *led){
  for(int i=0;i< led->numPixels ;i++){
    if(led->pixels[i].r+led->pixels[i].g+led->pixels[i].b<3){
      led->pixels[i].r +=1;
      led->pixels[i].g +=1;
      led->pixels[i].b +=1;
    }    
  }

}

unsigned long fadeMs[nofCh]={0};

static const uint8_t fadeTbl[256]={
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,
  7,7,7,7,7,7,
  8,8,8,8,8,8,
  9,9,9,9,
  10,10,10,10,10,
  11,11,11,11,
  12,12,12,12,
  13,13,13,
  14,14,14,
  15,15,15,
  16,16,16,
  17,17,17,
  18,18,
  19,19,
  20,20,20,
  21,21,
  22,22,
  23,23,
  24,24,
  25,
  26,26,
  27,27,
  28,
  29,29,
  30,30,
  31,
  32,33,33,
  34,
  35,
  36,36,
  37,38,39,40,41,41,42,43,44,45,46,47,48,49,51,52,53,54,55,56,58,59,60,62,
  63,64,66,67,69,70,72,73,75,77,78,80,82,84,86,87,89,91,93,95,98,100,102,
  104,106,109,111,114,116,119,121,124,127,130,132,135,138,141,144,148,151,
  154,158,161,165,168,172,176,180,184,188,192,196,200,205,209,214,219,223,228,233,238,244,249,255 
};
int fillModePrev[nofCh]={0};//Previous mode for fade in
int fillMode[nofCh]={0};    // Set this for Immediately switch
int fillModeNext[nofCh]={0};//Set this for Fade out 

void fadeOutIn(int ch,unsigned long diffMs,strand_t *led){
  const long fadeTime=2000;
  uint8_t rate=255;
  if(fillModeNext[ch]!=fillMode[ch]){ //Do fade Out
    fadeMs[ch]+=diffMs;
    if(fadeMs[ch]>fadeTime){
      fillModePrev[ch]=fillMode[ch];
      fillMode[ch]=fillModeNext[ch];
      
      fadeMs[ch]=0;
      rate=0;
    }else{
      uint8_t a=255.0f*(fadeTime-fadeMs[ch])/fadeTime;
      rate=fadeTbl[a];
    }
  }else if(fillModePrev[ch]!=fillMode[ch]){ //Do fade in
    fadeMs[ch]+=diffMs;
    if(fadeMs[ch]>fadeTime){
      fillModePrev[ch]=fillMode[ch];
      fadeMs[ch]=0;
      rate=255;
    }else{
      uint16_t a=255.0f*fadeMs[ch]/fadeTime;
      rate=fadeTbl[a];
    }  
  }
  if(rate!=255){
    fade(rate,led);
  }
}










void fillBlack(int ch,unsigned long diffMs, strand_t *led){
  for(int i=0;i< led->numPixels ;i++){
    led->pixels[i].num=0;
  }
}

void fillBaseBlink(unsigned char h,unsigned char s,unsigned char max_v,unsigned char min_v,float pahse,strand_t *led){
    float p=pahse;
    const float Lp1=0.40;//上がりの間のタイミング
    const float Lp2=0.75;//下がりまでのタイミング
    if(p<Lp1){
      p/=Lp1;
    }else if(p<Lp2){
     p=(Lp2-p)/(Lp2-Lp1);
    }else{//暗いままの時間
     p=0;
    }
    float fv= p*max_v+min_v;
    uint8_t v=fv;
    pixelColor_t colL=pixelFromHSB(h,s,v);
    pixelColor_t colH=pixelFromHSB(h,s,v+1);
    float pp=fv-v;
    char ipp=5*pp;
    for(int i=0;i< led->numPixels ;i++){
      led->pixels[i]=(i%5 < ipp)?colL:colH;

    }

    
}
void addWave(int ch,unsigned long diffMs,strand_t *led){
  int nofLed=led->numPixels;
 
  const int span=4000;//speed of lightning (msec)
  const int offsetMax=1000;
  
  const int nofFire=1;
  const int nofShape=9;
  const uint8_t shape[nofShape]={150,112,84,63,47,35,26 };
  const uint8_t satu[nofShape] = {250,250,250,204,163,130,104,83,66 };
  static uint8_t color[nofCh][nofFire];

  static long mss[nofCh][nofFire]={{0},};
  for (int i=0;i<nofFire;i++)mss[ch][i]+=diffMs;

  

  ////write each color
  for(int f=0;f<nofFire;f++){
    float phase=(float)( mss[ch][f] )/span;
    if(phase<0)continue;
    if(phase>1){
      mss[ch][f]=-1*random(0,offsetMax);
      color[ch][f]=random(0,252);
      continue;
    }
    if(color[ch][f]&1)phase=1-phase; //reverse 
    int pos= (nofLed+2*nofShape)*phase-nofShape;
    for(int i=0;i<nofShape;i++){
      int p=pos+i;
      if(p<0 || nofLed<=p)continue;
      led->pixels[p]=pixelFromHSB(color[ch][f],satu[nofShape-1-i],shape[nofShape-1-i]);
    }
  }
  
}

void fillLoop(int ch,unsigned long diffMs,strand_t *led){
  float p= ((gCount+ch*150)%gCountMax)/(float)gCountMax;
  fillBaseBlink(20,150,22,2,p,led);
  addWave(ch,diffMs,led); 
}
bool isBright(const pixelColor_t &c){
  return 12 < (uint16_t)c.r +(uint16_t)c.g +(uint16_t)c.b;
}
void fillDot(int ch,unsigned long diffMs,strand_t *led){
  int nofLed=led->numPixels;
  static long ms=0;
  int targetCount=0;
  if(ch==0){
    ms+=diffMs;
    ms%=20000;
  }
  if(ms<6000){
    targetCount=nofLed*ms/6000;
  }else if(ms<12000){
    targetCount=nofLed;
  }else if(ms<18000){
    targetCount=nofLed- nofLed*(ms-12000)/6000;
  }else{
    targetCount=0;
  }

  int brightCount=0;
  for(int i=0;i<nofLed;i++){
    if( isBright(led->pixels[i]) )brightCount++;
  }

  if(targetCount<brightCount){
    int p=random(brightCount);
    int i;
    for(i=0;i<nofLed;i++){
      if(isBright(led->pixels[i])){
        p--;
        if(p==0)break;
      }
    }
    led->pixels[i].num=0;
  }
  if(brightCount<targetCount){
    uint8_t c=random(255);
    int p=random(nofLed-brightCount);
    int i;
    for(i=0;i<nofLed;i++){
      if(!isBright(led->pixels[i])){
        p--;
        if(p==0)break;
      }
    }
    led->pixels[i]=pixelFromHSB(c,200,100);
  }
  if(ch==6 || ch==7)addWave(ch,diffMs,led);
}

void fillHeartBeat(int ch,unsigned long diffMs,strand_t *led){
  static long msb=0;
  const int p1=80;
  const int p2=160;
  const int p3=300;
  const int p4=800;
  const int pe=1200;
  if(ch==0){
    msb+=diffMs;
    msb%=pe;
  }
  for(int i=0;i<nofLed[ch];i++){
    long ms=msb+vPos[ch][i]*2/3;
    ms%=pe;
    uint8_t v=0;
    if(ms<p1){
      float phase=(float)ms/p1;
      v= phase * 100;
    }else if(ms<p2){
      float phase=(float)(ms-p1)/(p2-p1);    
      v= (1.0-phase) * 90 +10;
    }else if(ms<p3){
      float phase=(float)(ms-p2)/(p3-p2);    
      v= phase * 90+10;

    }else if(ms<p4){
      float phase=(float)(ms-p3)/(p4-p3);    
      v= (1.0-phase) * 100;
    }else{
      v=0;
    }
    v+=2;
    led->pixels[i]=pixelFromHSB(4,230,v);
    //fillSome(led,pixelFromHSB(4,230,v)); 
  }
  if(ch==6 || ch==7)addWave(ch,diffMs,led);
}
void fillWave(int ch,unsigned long diffMs,strand_t *led){
  static long msb=0;
  const int p1=3000;
  const int p2=4000;
  const int pe=4200;
  
  if(ch==0){
    msb+=diffMs;
    msb%=pe;
  }
  for(int i=0;i<nofLed[ch];i++){
    int ms=msb+vPos[ch][i]*4;
    ms%=pe;
    uint8_t v;
    if(ms<p1){
      float phase=(float)ms/p1;
      v=phase*60;
    }else if(ms<p2){
      v=60;
    }else{
      float phase=(float)(ms-p2)/(pe-p2);
      v=(1-phase)*60;
    }
    //v=60-v;
    v+=2;
    led->pixels[i]=pixelFromHSB(vPos[ch][i]/2+127,200,v);
  } 
  if(ch==6 || ch==7)addWave(ch,diffMs,led);
}




void fillSeesaaw(int ch,unsigned long diffMs,strand_t *led){
  static long msb=0;
  const int pe=8000;
  if(ch==0){
    msb+=diffMs;
    msb%=pe;
  }
  uint8_t c=(ch+1)*31;
  for(int i=0;i<nofLed[ch];i++){
    float y=vPos[ch][i]/255.0f;//0..1    
    uint8_t v=0;
    float phase=(float)msb/pe;
    float p=0.5f+0.5f*sin(PI*2*phase);
    float yn=1.0-y;
    v= 200*( p*(y*y*y*y) + (1.0-p)*(yn*yn*yn*yn)   );
    
    c= (uint16_t)c * 31;
    v+=2;
    led->pixels[i]=pixelFromHSB( c  ,150,v);
    //fillSome(led,pixelFromHSB(4,230,v)); 
  }
  if(ch==6 || ch==7)addWave(ch,diffMs,led);
}

const int nofFuncs=5;
typedef void (* T_fillFunc)(int ,unsigned long ,strand_t *);
T_fillFunc funcs[nofFuncs]={fillSeesaaw,fillDot,fillLoop,fillHeartBeat,fillWave };


static unsigned long wdt=0;
void loop() {
  static int nowMode=0;
  static unsigned long oldMillis=millis();
  unsigned long now=millis();
  unsigned long diffMs=now-oldMillis;
  oldMillis=now;
  wdt+=diffMs;
  gCount+=diffMs;
  gCount%=gCountMax;


  if(wdt>60*1000){
    log_i("WDT arrive do random change");
    wdt=0;
    nowMode++;
    nowMode%=nofFuncs;
    for(int c=0;c<nofCh;c++)fillModeNext[c]=nowMode;
  }
  // put your main code here, to run repeatedly:
  for(byte ch=0;ch<nofCh;ch++){
    if(fillMode[ch]<nofFuncs)funcs[fillMode[ch]](ch,diffMs,hLed+ch);
    fadeOutIn(ch,diffMs,hLed+ch);
    addDark(hLed+ch);
    digitalLeds_updatePixels(hLed+ch);
  }
  
}
