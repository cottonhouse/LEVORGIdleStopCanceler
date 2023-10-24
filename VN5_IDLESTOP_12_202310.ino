// for Arduino Leonard (ProMicro, CJMCU Beetle) + MCP2515
//
// Copyright (c) 2023 Cottonhouse
// Released under the MIT license
// https://opensource.org/licenses/mit-license.php
//
// Rev.01 実地テスト前
// Rev.01_DEBUG 実地テストでデバッグしてとりあえず動作させた
// Rev.02 ブラッシュアップと機能追加を図るもCANからデータを取れず動作しなくなった
// Rev.03 ACCから電源を取得することを前提としてプログラムの簡素化を図る
// Rev.04 Engine OFFでACC ONの状態になるが、パケットを受信できないのでMode変遷ができない問題をTickerで解決
// Rev.05 Serialを削除
// Rev.07 Rev.06で使った8MHzのCAN Libが正常動作しないため、元に戻す
// Rev.08 Serial.printをDEBUGモード時に有効にする
// Rev.09 CANのライブラリをCAN_BUS_Shieldからcoryjfouler/mcp_canに変更
// Rev.10 注釈にした不要な行の削除
// Rev.11 万が一の危険性を考慮（CAN分析では出なかった値がパケットに出た場合を考えた処理を追加）
// Rev.12 パケットの条件比較をビットで比較に変更（実利用を考慮）
#include <Arduino.h>
#include <Ticker.h>
#include <mcp_can.h>
#include <SPI.h>
#include <EEPROM.h>
//#define DEBUG 1 // シリアルにModeを表示する場合コメントを外す(USBシリアル接続をしていないと動作しなくなる)
#define SS 2 // 2 pin (10pinを使っていたが拡張性を考えて2pinに変更。10pinはTimer1で利用されるため)
MCP_CAN CAN(SS);   // Set CS pin
// for CAN Message
unsigned char len = 0;
unsigned char buf[8];
unsigned long id;

// for EEPROM
unsigned char romstat;  // 前回電源が切れる直前に保存されていた設定データ
const unsigned int  ROMADDR = 0;  // EEPROMのアドレス

// Main Loop
int STAT = 0; // 動作ステータス 0..エンジン起動まで待つ  1..保存された設定の適用  2..変更があったら設定を保存
int pktSending = 0;  // パケット送信フェーズフラグ

// Timer
volatile unsigned int intr = 0;
void intrrup() {
  intr += 1;
}
Ticker blinker(intrrup, 50); // ms指定, TickerはTimer1を使うため9ピン10ピンは使えなくなる。CSのピンに注意。

// EEPROM Reading
void getRomstat() {
  // Get EEPROM Data
  romstat = EEPROM.read(ROMADDR);
  #ifdef DEBUG
  Serial.print("EEPROM Read: ");
  Serial.print(romstat, HEX);  
  #endif
  // 値は00(ON)か40(OFF)しか保存されないはずだが、念の為それ以外だったら0にする（保存途中に電源が切れたりして内容が破壊されることもあり得る)）
  if(romstat != 0x40 && romstat != 0) {
    romstat = 0;
  }
  romstat &= 0x40; // 上から2bit目取り出し
  #ifdef DEBUG
  Serial.print(" -> ");
  Serial.println(romstat, HEX); 
  #endif
}

// パケット送信
void PktSend() {
  unsigned char chksum = 0; // チェックサム計算用
  unsigned char sendbuf[8]; // 送信用バッファ
  static unsigned char SecNo = 0; // シーケンシャルNo用

  sendbuf[1] = SecNo; // シーケンシャルNo
  sendbuf[2] = buf[2];
  sendbuf[3] = buf[3];
  sendbuf[4] = buf[4];
  sendbuf[5] = buf[5];
   sendbuf[6] = 0x40 | buf[6]; // SW ON
  sendbuf[7] = buf[7];
  for(int i = 1; i < len; i++) { // buf[1] から buf[7] を合計してチェックサムを計算
    chksum += sendbuf[i];
  }
  sendbuf[0] = chksum + 0x93;
  CAN.sendMsgBuf(0x390, 0, len, sendbuf);  // 送信！

  SecNo = (++SecNo) & 0x0F;  // シーケンシャルNoの加算(不要と思われるが、念の為変える)
  #ifdef DEBUG
  Serial.println("Send"); 
  #endif
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  while (!Serial) {
  }
  #endif

  // CAN
  #ifdef DEBUG
  Serial.print("CAN initializing");
  #endif
  while (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ)) {  // init can bus : baudrate = 500k // Oscillator 16MHz
    delay(50);
    #ifdef DEBUG
    Serial.print(".");
    #endif
  }
  #ifdef DEBUG
  Serial.println("");
  #endif

  // init Mask & Filter
  CAN.init_Mask(0, 0, 0x7FF);
  CAN.init_Mask(1, 0, 0x7FF);
  CAN.init_Filt(0, 0, 0x174); // アイストCU
  CAN.init_Filt(1, 0, 0x390); // アイストSW
  CAN.setMode(MCP_NORMAL);
  #ifdef DEBUG
  Serial.println("CAN init end");
  #endif

  // Get EEPROM Data
  getRomstat();
  #ifdef DEBUG
  Serial.println("Mode:0");
  #endif

  // Timer
  blinker.start();
}

void loop() {
  static unsigned char D4_174;  // D4の直前の値保存用
  static unsigned int refTimes = 0; // 設定反映の試行回数
  blinker.update();

  if(CAN_MSGAVAIL == CAN.checkReceive()) {
    CAN.readMsgBuf(&id, &len, buf); 
    switch (id) {
      case 0x174: // アイストCU
        if((buf[2] & 0x08) == 0) {
          // エンジンがOFFだったら強制的にステータス0へ遷移(ACCから電源をとる前提。エンジンOFFでパケットが来なくなるようなので、処理的には不要)
          if(STAT) {  // STATが0でない時STATを0にする
            getRomstat(); // EEPROM読み込み
            STAT = 0;
            pktSending = 0;  // 送信中フラグOFF
            #ifdef DEBUG
            Serial.println("Mode:0");
            #endif
          }
          break; // switch(id)抜ける
        } else { // エンジンがONになった
          if(STAT == 0) { // エンジンがONでステータスが0だったらステータス1に変遷
            STAT = 1; // EEPROMに保存された値の復旧処理ステータスへ変遷
            refTimes = 0; // 送信試行回数クリア
            #ifdef DEBUG
            Serial.println("Mode:1");
            #endif
          }
        }
        switch (STAT) { // ステータスによって違う処理を実行
          case 1: // 保存された設定の適用
            if(romstat != (buf[4] & 0x40)) { // 保存された設定状態と現在の設定状態が異なっていたら、SW ONのパケットを送信する
              if(!pktSending) {
                pktSending = 1;  // 送信中フラグON
                #ifdef DEBUG
                Serial.println("Mode:1Send");
                Serial.print("  ROM: "); Serial.println(romstat, HEX);
                Serial.print("  D4 : ");  Serial.println(buf[4], HEX);
                #endif
              } else {
                // 念の為のルーチン（送信して設定が変更されても保存された前回値にならなかった場合（CAN解析では出てこなかった値になった場合）
                // 無限に送信してしまう事になるのでそれを防止）
                if(refTimes > 4) { // 送信試行は5回まで
                  STAT = 2;
                  pktSending = 0;  // 送信中フラグOFF
                  D4_174 = romstat; // EEPROM保存データを格納しておくと、STAT2で現在設定がEEPROMに保存される
                  refTimes = 0; // 要らないけど気持ち悪いのでクリアしておく
                  #ifdef DEBUG
                  Serial.println("Mode:2"); 
                  #endif
                }
              }
            } else {
              STAT = 2; // 保存された設定と同じだった or Sendの結果同じになったら、次のステータスへ（2=設定値の保存ステータス）
              pktSending = 0;  // 送信中フラグOFF
              D4_174 = buf[4] & 0x40; // 前回受信データの保存
              #ifdef DEBUG
              Serial.println("Mode:2"); 
              #endif
            }
            break;
          case 2: // 設定に変更があったら保存する
            if((buf[4] & 0x40) != D4_174) {  // 保存しておいた前回データと違う場合
              D4_174 = buf[4] & 0x40; // 前回データの保存
              EEPROM.write(ROMADDR, D4_174); // EEPROMへ保存
              #ifdef DEBUG
              Serial.print("Write EEPROM: ");
              Serial.println(D4_174, HEX);
              #endif
            }
            break;
          default:  // メモリリークとか無いように念の為
            break;
        }
        break;
      case 0x390: // アイストSW
        // パケット送信
        if(pktSending) { // パケット送信中フラグがONの時
          if((buf[6] & 0x40) != 0x40) { // 自分が送ったデータ含む SW ONのデータを受信した時は、パケットは送らない
            PktSend(); // パケット送信
            refTimes++; // 送信回数カウント
          }
        }
        break;
      default:  // メモリリークとか無いように念の為
        break;
    }
    // エンジンOFFになった場合、モード変遷できない（174パケットが無いとエンジンOFFが判断できない）ので、タイマ利用
    intr = 0;  // タイマクリア（CANパケット受信時のみクリアされるので、受信できないときはタイマカウンタが進む)
  }
  if((intr >= 2)) { // 0.05-0.1秒程度受信がないとき、電源OFFとみなす
    if(STAT != 0) {
      getRomstat();
      STAT = 0; // エンジン掛かるまで待つモード
      pktSending = 0;  // 送信中フラグOFF
      #ifdef DEBUG
      Serial.println("Mode:0"); 
      #endif
    }
    intr = 0; // Clear Timer
  }
  delay(1); // 無くても良いと思うけど念の為
}
