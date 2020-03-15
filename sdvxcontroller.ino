// sdvx コントローラ with Teensy 2.0
// (ボタン・ロータリーエンコーダ(A相、B相) それぞれについて、前回と同じ状態が n 回続いたらチャタリングなしと判定するロジック)
// Copyright (C) 2019-2020 KOISHIKAWA Misaki

#include "Keyboard.h"

// 定数の定義

// Teensy 2.0 ピン番号 (Teensy 以外を使用する場合、適宜調整可)
#define PIN_B0  0   // Teensy 2.0 の左下、GND の隣
#define PIN_B1  1   // …以降、左下から右に進む
#define PIN_B2  2
#define PIN_B3  3
#define PIN_B7  4
#define PIN_D0  5
#define PIN_D1  6
#define PIN_D2  7
#define PIN_D3  8
#define PIN_C6  9
#define PIN_C7  10  // Teensy 2.0 の右下角
#define PIN_D6  11  // Teensy 2.0 の右上角
#define PIN_D7  12  // …以降、右上から左に進む
#define PIN_B4  13
#define PIN_B5  14
#define PIN_B6  15
#define PIN_F7  16
#define PIN_F6  17
#define PIN_F5  18
#define PIN_F4  19
#define PIN_F1  20
#define PIN_F0  21  // Teensy 2.0 の左上、VCC の隣
#define PIN_D4  22  // Teensy 2.0 の右上、D6 のすぐ下
#define PIN_D5  23  // Teensy 2.0 の右下、C7 のすぐ上
#define PIN_E6  24  // Teensy 2.0 の中央下内側

// ボタン チャタリング回避用判定閾値 (環境に応じて適宜調整可)
// (頻繁にボタンプッシュ中にリリースされてしまうまたはその逆の場合はこの値を増やす。ボタンプッシュ・リリースに対するレスポンスが遅くなる場合は減らす)
#define BT_SAME_CNT_MIN      (5)     // ボタン状態が切り替わった後の値がこの回数続いたら初めてその状態になったものとする

// ロータリーエンコーダ チャタリング回避用判定閾値 (環境に応じて適宜調整可)
// (頻繁につまみが逆転してしまう場合はこの値を増やす。つまみ操作に対するレスポンスが遅くなる場合は減らす)
#define VOL_SAME_CNT_MIN      (7)     // ロータリーエンコーダ A相/B相それぞれが、値が切り替わった後の値がこの回数続いたら初めてその値になったものとする

// ロータリーエンコーダ連続回転状態判定閾値 (環境に応じて適宜調整可)
// (メニュー・曲選択でのつまみ操作でなかなか思った位置で止まらない場合はこの値を増やす。プレイ中に頻繁にキーリリースされる場合はこの値を減らす)
#define VOL_MOVE_CONTINUE_MIN   (5)   // (同じ方向の回転状態がこの回数以上繰り返されている場合、ぐるぐる回転していると判定=すぐには回転を止めない、それまではチョンと操作されていると判定=すぐに止める)

// ロータリーエンコーダ停止判定閾値 (環境に応じて適宜調整可)
// (頻繁にキーリリースされる場合はこの値を増やす。キーリリースにもたつく場合は減らす)
#define VOL_RELEASE_CNT_MIN_SHORT   (500)      // ロータリーエンコーダ A相/B相それぞれ、チョンと動かした後、値に変化がないままの状態がこの回数続いたら回転を停止したものとする
#define VOL_RELEASE_CNT_MIN_LONG    (2500)     // ロータリーエンコーダ A相/B相それぞれ、ぐるぐる回転させた後、値に変化がないままの状態がこの回数続いたら回転を停止したものとする

// キー配列上の位置と数
#define NO_BUTTON_A           (0)     // キー配列位置 : BT-A
#define NO_BUTTON_B           (1)     // キー配列位置 : BT-B
#define NO_BUTTON_C           (2)     // キー配列位置 : BT-C
#define NO_BUTTON_D           (3)     // キー配列位置 : BT-D
#define NO_FX_LEFT            (4)     // キー配列位置 : FX-L
#define NO_FX_RIGHT           (5)     // キー配列位置 : FX-R
#define NO_START              (6)     // キー配列位置 : START
#define NO_VOL_LEFT_REVERSE   (7)     // キー配列位置 : VOL-L 反時計回り 
#define NO_VOL_LEFT_NORMAL    (8)     // キー配列位置 : VOL-L 時計回り
#define NO_VOL_RIGHT_REVERSE  (9)     // キー配列位置 : VOL-R 反時計回り
#define NO_VOL_RIGHT_NORMAL   (10)    // キー配列位置 : VOL-R 時計回り
#define KEYARRAY_NUM          (11)    // ボタン入力・ロータリーエンコーダ入力に対応するキー配列の数
#define BTARRAY_NUM           (7)     // ボタン入力に対応するピンの数 (BT-A / BT-B / BT-C / BT-D / FX-L / FX-R / START)

// キー押し下げ状態
#define STS_RELEASE           (0)     // キー押し下げ状態 : キーが離された
#define STS_PRESS             (1)     // キー押し下げ状態 : キーが押された

// つまみ回転方向
#define VOL_DIR_STOP          (0)     // つまみ回転方向 : 停止
#define VOL_DIR_REVERSE       (1)     // つまみ回転方向 : 反時計回り
#define VOL_DIR_NORMAL        (2)     // つまみ回転方向 : 時計回り

// つまみの左右
#define VOL_LEFT              (0)     // つまみの左右 : 左
#define VOL_RIGHT             (1)     // つまみの左右 : 右
#define VOLARRAY_NUM          (2)     // つまみの数 (左右で 2 個)

// ロータリーエンコーダ A/B相
#define ENC_A                 (0)     // ロータリーエンコーダ A相
#define ENC_B                 (1)     // ロータリーエンコーダ B相
#define ABARRAY_NUM           (2)     // A相/B相の数 (AB で 2 個)


// グローバル変数

// Teensy ピン位置と、接続されているボタン・ロータリーエンコーダ A/B相との対応 (環境に応じて適宜変更可)
const byte pinBtA = PIN_B6;               // ピン位置 : BT-A
const byte pinBtB = PIN_B4;               // ピン位置 : BT-B
const byte pinBtC = PIN_D1;               // ピン位置 : BT-C
const byte pinBtD = PIN_D0;               // ピン位置 : BT-D
const byte pinBtFxL = PIN_D7;             // ピン位置 : FX-L
const byte pinBtFxR = PIN_C7;             // ピン位置 : FX-R
const byte pinBtStart = PIN_D6;           // ピン位置 : START
const byte pinEncLeftA = PIN_F6;          // ピン位置 : VOL-L A相
const byte pinEncLeftB = PIN_F5;          // ピン位置 : VOL-L B相
const byte pinEncRightA = PIN_B3;         // ピン位置 : VOL-R A相
const byte pinEncRightB = PIN_B7;         // ピン位置 : VOL-R B相

// ボタン・ロータリーエンコーダでまとめたピン入力の配列
const byte btArray[BTARRAY_NUM] = {pinBtA, pinBtB, pinBtC, pinBtD, pinBtFxL, pinBtFxR, pinBtStart};   // ボタン系ピン入力リスト
const byte encArray[VOLARRAY_NUM][ABARRAY_NUM] = {{pinEncLeftA, pinEncLeftB}, {pinEncRightA, pinEncRightB}};  // ロータリーエンコーダ系ピン入力リスト

// ボタン・つまみ回転方向と、送信するキーコードとの対応 (環境に応じて適宜変更可)
// SOUND VOLTEX III / k-shoot mania の SETTING 画面で、以下のキーコードに合わせて設定を行う必要あり)
const byte keyBtA = 'd';                  // 送信するキーコード : BT-A
const byte keyBtB = 'f';                  // 送信するキーコード : BT-B
const byte keyBtC = 'j';                  // 送信するキーコード : BT-C
const byte keyBtD = 'k';                  // 送信するキーコード : BT-D
const byte keyBtFxL = 'c';                // 送信するキーコード : FX-L
const byte keyBtFxR = 'n';                // 送信するキーコード : FX-R
const byte keyBtStart = '\n';              // 送信するキーコード : START
const byte keyVolLeftReverse = 'a';       // 送信するキーコード : VOL-L 反時計回り
const byte keyVolLeftNormal = 's';        // 送信するキーコード : VOL-L 時計回り
const byte keyVolRightReverse = 'l';      // 送信するキーコード : VOL-R 反時計回り
const byte keyVolRightNormal = ';';       // 送信するキーコード : VOL-R 時計回り

// 送信キーコードをまとめた配列
const byte keyCodeArray[KEYARRAY_NUM] = {      // 送信キーコードリスト
  keyBtA, keyBtB, keyBtC, keyBtD,         // BT-A / BT-B / BT-C / BT-D
  keyBtFxL, keyBtFxR,                     // FX-L / FX-R
  keyBtStart,                             // START
  keyVolLeftReverse, keyVolLeftNormal,    // VOL-L (反時計回り / 時計回り)
  keyVolRightReverse, keyVolRightNormal   // VOL-R (反時計回り / 時計回り)
};

// キーコード送信中かどうかのフラグの配列
byte keyStatusArray[KEYARRAY_NUM];        // キー押し下げ状態 (STS_RELEASE : 離されている、STS_PRESS : 押されている)

// ボタンの状態
byte valueOldBtArray[BTARRAY_NUM] = {0, 0, 0, 0, 0, 0, 0};        // BT-A ～ START 前回のボタン押し下げ状態 (0/1)
int cntBtSameArray[BTARRAY_NUM] = {0, 0, 0, 0, 0, 0, 0};          // BT-A ～ START 値が変わらなかった連続回数

// ロータリーエンコーダの状態
byte valueOldABArray[VOLARRAY_NUM][ABARRAY_NUM] = {0, 0, 0, 0};   // VOL-L/R 前回のロータリーエンコーダ A相/B相値 (0/1)
int cntABSameArray[VOLARRAY_NUM][ABARRAY_NUM] = {0, 0, 0, 0};     // VOL-L/R A相/B相 値が変わらなかった連続回数
byte posOldArray[VOLARRAY_NUM] = {0, 0};                          // VOL-L/R 前回のロータリーエンコーダ値 (0～3)
byte dirOldArray[VOLARRAY_NUM] = {VOL_DIR_STOP, VOL_DIR_STOP};    // VOL-L/R 前回の回転方向 (VOL_DIR_STOP : 停止、VOL_DIR_REVERSE : 反時計回り、VOL_DIR_NORMAL : 時計回り)
int cntStopArray[VOLARRAY_NUM] = {0, 0};                          // VOL-L/R 回転が停止してからのループ数
int cntMoveArray[VOLARRAY_NUM] = {0, 0};                          // VOL-L/R 回転中のループ数


// プロトタイプ宣言

static void readBt(byte btType);
static void readVol(byte volType);


// 初期処理
void setup()
{
  byte i;
  byte j;

  // それぞれのピンからの入力を受け付ける (プルアップ抵抗は内部プルアップを使用)
  for (i = 0; i < BTARRAY_NUM; i++)
  {
    pinMode(btArray[i], INPUT_PULLUP);    // ボタン系入力の受け付け (BT-A ～ START)
  }
  for (i = 0; i < VOLARRAY_NUM; i++)
  {
    for (j = 0; j < ABARRAY_NUM; j++)
    {
      pinMode(encArray[i][j], INPUT_PULLUP);   // ロータリーエンコーダ系入力の受け付け (左/右それぞれ A/B)
    }
  }

  // キーの押し下げ状態を初期化
  for (i = 0; i < KEYARRAY_NUM; i++)
  {
    keyStatusArray[i] = STS_RELEASE;    // キーはどれも押されていない (= Keyboard.press() は送信されていない)
  }

  // ボタンの現在の値を取得し、グローバル変数に保存
  for (i = 0; i < BTARRAY_NUM; i++)
  {
    valueOldBtArray[i] = !digitalRead(btArray[i]);    // BT-A ～ START の状態取得
  }
  
  // ロータリーエンコーダの現在の値 (0～3) を取得し、グローバル変数に保存
  valueOldABArray[VOL_LEFT][ENC_A] = !digitalRead(pinEncLeftA);     // 左 A相
  valueOldABArray[VOL_LEFT][ENC_B] = !digitalRead(pinEncLeftB);     // 左 B相
  valueOldABArray[VOL_RIGHT][ENC_A] = !digitalRead(pinEncRightA);   // 右 A相
  valueOldABArray[VOL_RIGHT][ENC_B] = !digitalRead(pinEncRightB);   // 右 B相
  posOldArray[VOL_LEFT] = (valueOldABArray[VOL_LEFT][ENC_A]) + (valueOldABArray[VOL_LEFT][ENC_B] << 1);     // 左 (最下部ビットが A相、その上が B相)
  posOldArray[VOL_RIGHT] = (valueOldABArray[VOL_RIGHT][ENC_A]) + (valueOldABArray[VOL_RIGHT][ENC_B] << 1);  // 右 (同上)
  cntStopArray[VOL_LEFT] = 0;                     // 左 停止中カウントをリセット
  cntStopArray[VOL_RIGHT] = 0;                    // 右 停止中カウントをリセット
  cntMoveArray[VOL_LEFT] = 0;                     // 左 動作中カウントをリセット
  cntMoveArray[VOL_RIGHT] = 0;                    // 右 動作中カウントをリセット

  // キーボードライブラリの開始
  Keyboard.begin();
  Keyboard.releaseAll();  // キーはどれも押されていない

  // シリアル出力の開始 (デバッグ用)
  //Serial.begin(38400);
}


// ループ処理
void loop()
{
  byte i;

  // ボタン系処理
  for (i = 0; i < BTARRAY_NUM; i++)
  {
    readBt(i);    // ボタン (BT-A～START) の押し下げ状態に合わせてキー送信・停止する
  }

  // ロータリーエンコーダ系処理
  for (i = 0; i < VOLARRAY_NUM; i++)
  {
    readVol(i);   // つまみ (左/右) の回転に合わせてキー送信・停止する
  }
}


// ボタン系処理
static void readBt(byte btType)
{
  byte btCur;         // 現在のボタンの値

  // 現在のボタンの押し下げ状態を取得
  btCur = !digitalRead(btArray[btType]);

  // チャタリング除去
  // 前回の値と比較して、同じ場合は連続回数をインクリメント、違う場合はリセット
  if (valueOldBtArray[btType] == btCur)
  {
    cntBtSameArray[btType] ++;      // 同じ値が連続した = 連続回数インクリメント
  }
  else
  {
    valueOldBtArray[btType] = btCur;      // 違う値になった = 次回はこの値と比較
    cntBtSameArray[btType] = 0;     // 違う値になった = 連続回数リセット
  }
  
  // 同じ値が続いた連続回数が必要以上に上がらないようにする
  if (cntBtSameArray[btType] > BT_SAME_CNT_MIN)
  {
    cntBtSameArray[btType] = BT_SAME_CNT_MIN;
  }
  
  // 同じ状態が規定回数連続していない場合は、チャタリングかもしれないのでまだ入力と見なさない
  if (cntBtSameArray[btType] < BT_SAME_CNT_MIN)
  {
    return;
  }
  
  // btCur が現在の (チャタリングしていない) ボタンの値になる

  if (btCur != 0)     // BT-A ～ START のボタンが押されている場合、
  {
    if (keyStatusArray[btType] == STS_RELEASE)    // もしそのボタンに対するキーが押されていないと送信されている場合は
    {
      Keyboard.press(keyCodeArray[btType]);         // 「対応するキーが押された」旨を送信する
      keyStatusArray[btType] = STS_PRESS;           // 現在のキーコード状態 (押されている) をグローバル変数に保存
    }
  }
  else                                          // BT-A ～ START のボタンが押されていない場合、
  {
    if (keyStatusArray[btType] == STS_PRESS)      // もしそのボタンに対するキーが押されていると送信されている場合は
    {
      Keyboard.release(keyCodeArray[btType]);        // 「対応するキーが離された」旨を送信する
      keyStatusArray[btType] = STS_RELEASE;          // 現在のキーコード状態 (押されていない) をグローバル変数に保存
    }
  }
}


// ロータリーエンコーダ系処理
static void readVol(byte volType)
{
  // posOld : 前回のロータリーエンコーダ (左/右) の値 (0～3) = グローバル変数に保持している値を取得
  byte posOld = posOldArray[volType];
  // dirOld : 前回の回転方向 (VOL_DIR_STOP:停止 VOL_DIR_NORMAL:正回転中、VOL_DIR_REVERSE:逆回転中) = グローバル変数に保持している値を取得
  byte dirOld = dirOldArray[volType];

  byte posCurA;         // 現在の A相の値
  byte posCurB;         // 現在の B相の値
  byte posCur;          // 現在のロータリーエンコーダの値 (最下部ビットが A相、その上が B相)
  byte reversePos;      // キーコードリスト・キー押し下げ状態配列上の位置 (反時計回り)
  byte normalPos;       // キーコードリスト・キー押し下げ状態配列上の位置 (時計回り)
  int cntReleaseMin;    // 回転停止の閾値 (チョンと動かした場合は VOL_RELEASE_CNT_MIN_SHORT、ぐるぐる回転中の場合は VOL_RELEASE_CNT_MIN_LONG)

  // ロータリーエンコーダ状態取得
  if (volType == VOL_LEFT)
  {
    posCurA = (!digitalRead(pinEncLeftA));    // VOL-L A相の値を取得
    posCurB = (!digitalRead(pinEncLeftB));    // VOL-L B相の値を取得
    reversePos = NO_VOL_LEFT_REVERSE;         // VOL-L 逆回転した場合、このキーコードを送信する
    normalPos = NO_VOL_LEFT_NORMAL;           // VOL-L 正回転した場合、このキーコードを送信する
  }
  else
  {
    posCurA = (!digitalRead(pinEncRightA));   // VOL-R A相の値を取得
    posCurB = (!digitalRead(pinEncRightB));   // VOL-R B相の値を取得
    reversePos = NO_VOL_RIGHT_REVERSE;        // VOL-R 逆回転した場合、このキーコードを送信する
    normalPos = NO_VOL_RIGHT_NORMAL;          // VOL-R 正回転した場合、このキーコードを送信する
  }

  // チャタリング除去
  // A相・B相それぞれについて、前回の値と比較して、同じ場合は連続回数をインクリメント、違う場合はリセット
  if (valueOldABArray[volType][ENC_A] == posCurA)
  {
    cntABSameArray[volType][ENC_A] ++;      // 同じ値が連続した = 連続回数インクリメント
  }
  else
  {
    valueOldABArray[volType][ENC_A] = posCurA;      // 違う値になった = 次回はこの値と比較
    cntABSameArray[volType][ENC_A] = 0;     // 違う値になった = 連続回数リセット
  }
  
  if (valueOldABArray[volType][ENC_B] == posCurB)
  {
    cntABSameArray[volType][ENC_B] ++;      // 同じ値が連続した = 連続回数インクリメント
  }
  else
  {
    valueOldABArray[volType][ENC_B] = posCurB;      // 違う値になった = 次回はこの値と比較
    cntABSameArray[volType][ENC_B] = 0;     // 違う値になった = 連続回数リセット
  }

  // A相・B相それぞれについて、同じ値が続いた連続回数が必要以上に上がらないようにする
  if (cntABSameArray[volType][ENC_A] > VOL_SAME_CNT_MIN)
  {
    cntABSameArray[volType][ENC_A] = VOL_SAME_CNT_MIN;
  }
  if (cntABSameArray[volType][ENC_B] > VOL_SAME_CNT_MIN)
  {
    cntABSameArray[volType][ENC_B] = VOL_SAME_CNT_MIN;
  }

  // A相・B相どちらか片方でも、同じ状態が規定回数連続していない場合は、チャタリングかもしれないので入力と見なさない
  if (cntABSameArray[volType][ENC_A] < VOL_SAME_CNT_MIN)
  {
    return;
  }
  if (cntABSameArray[volType][ENC_B] < VOL_SAME_CNT_MIN)
  {
    return;
  }
  
  posCur = (posCurA) + ((posCurB) << 1);  // 現在の (チャタリングしていない) VOL-L/R の値決定 (最下部ビットが A相、その上が B相)
  
  // 00→01→11→10→00… の順に進む値を 0→1→2→3→0… の順に進むように置換
  // (その方がわかりやすいから)
  if (posCur == 3)
  {
    posCur = 2;
  }
  else if (posCur == 2)
  {
    posCur = 3;
  }

  // 動きを検出した場合、
  if (posCur != posOld)
  {
    // dirCur : 回転方向 (0:停止 1:正回転 3:逆回転) = 未決定
    byte dirCur = VOL_DIR_STOP;

    // 前回の位置と比較して、回転方向を取得
    switch (posOld)
    {
      case 0:   // 前回の値が 0
        switch (posCur)
        {
          case 1:   // 現在の値が 1
            dirCur = VOL_DIR_NORMAL;    // 0->1 : 正回転
            //Serial.println("0->1 : 正回転");
            break;
          case 2:
            dirCur = dirOld;            // 0->2 : どっちかわからないので、直前の回転が続いているものとする
            //Serial.println("不明:0->2");
            break;
          case 3:
            dirCur = VOL_DIR_REVERSE;   // 0->3 : 逆回転
            //Serial.println("0->3 : 逆回転");
            break;
          default:                      // あり得ない
            break;
        }
        break;
      case 1:   // 前回の値が 1
        switch (posCur)
        {
          case 0:
            dirCur = VOL_DIR_REVERSE;   // 1->0 : 逆回転
            //Serial.println("1->0 : 逆回転");
            break;
          case 2:
            dirCur = VOL_DIR_NORMAL;    // 1->2 : 正回転
            //Serial.println("1->2 : 正回転");
            break;
          case 3:
            dirCur = dirOld;            // 1->3 : どっちかわからないので、直前の回転が続いているものとする
            //Serial.println("不明:1->3");
            break;
          default:                      // あり得ない
            break;
        }
        break;
      case 2:   // 前回の値が 2
        switch (posCur)
        {
          case 0:
            dirCur = dirOld;            // 2->0 : どっちかわからないので、直前の回転が続いているものとする
            //Serial.println("不明:2->0");
            break;
          case 1:
            dirCur = VOL_DIR_REVERSE;   // 2->1 : 逆回転
            //Serial.println("2->1 : 逆回転");
            break;
          case 3:
            dirCur = VOL_DIR_NORMAL;    // 2->3 : 正回転
            //Serial.println("2->3 : 正回転");
            break;
          default:                      // あり得ない
            break;
        }
        break;
      case 3:   // 前回の値が 3
        switch (posCur)
        {
          case 0:
            dirCur = VOL_DIR_NORMAL;    // 2->3 : 正回転
            //Serial.println("3->0 : 正回転");
            break;
          case 1:
            dirCur = dirOld;            // 3->1 : どっちかわからないので、直前の回転が続いているものとする
            //Serial.println("不明:3->1");
            break;
          case 2:
            dirCur = VOL_DIR_REVERSE;   // 3->2 : 逆回転
            //Serial.println("3->2 : 逆回転");
            break;
          default:                      // あり得ない
            break;
        }
        break;
      default:                  // 0～3 以外はあり得ない
        break;
    }

    // グローバル変数に前回←今回 の値を保存
    posOldArray[volType] = posCur;    // (次回はこの値と、ロータリーエンコーダから取得した値を比較する)
    dirOldArray[volType] = dirCur;    // (次回はこの回転方向と、値の変化から決定された回転方向を比較する)

    // デバッグ用出力
    //Serial.print(" POS=[");
    //Serial.print(posOld);
    //Serial.print(" -> ");
    //Serial.print(posCur);
    //Serial.print("] dir=[");
    //Serial.print(dirOld);
    //Serial.print(" ->  ");
    //Serial.print(dirCur);
    //Serial.println("]");

    // 「今回の値」で、送信するキー状態を決定する。
    // 正回転を送信する場合、
    if (dirCur == VOL_DIR_NORMAL)
    {
      // 今まで逆回転していた場合は逆回転に対応するキーをリリース
      if (keyStatusArray[reversePos] == STS_PRESS)
      {
        keyStatusArray[reversePos] = STS_RELEASE;
        Keyboard.release(keyCodeArray[reversePos]);
        cntMoveArray[volType] = 0;                  // 動作中カウントをリセット
      }
      // まだ正回転に対応するキープッシュが行われていない場合は、正回転に対応するキーをプッシュ
      if (keyStatusArray[normalPos] == STS_RELEASE)
      {
        keyStatusArray[normalPos] = STS_PRESS;
        Keyboard.press(keyCodeArray[normalPos]);
      }
    }
    // 逆回転を送信する場合、
    else if (dirCur == VOL_DIR_REVERSE)
    {
      // 今まで正回転していた場合は正回転に対応するキーをリリース
      if (keyStatusArray[normalPos] == STS_PRESS)
      {
        keyStatusArray[normalPos] = STS_RELEASE;
        Keyboard.release(keyCodeArray[normalPos]);
      }
      // まだ逆回転に対応するキープッシュが行われていない場合は、逆回転に対応するキーをプッシュ
      if (keyStatusArray[reversePos] == STS_RELEASE)
      {
        keyStatusArray[reversePos] = STS_PRESS;
        Keyboard.press(keyCodeArray[reversePos]);
      }
    }
    else
    {
      ;   // あり得ない
    }
    cntStopArray[volType] = 0;                     // 停止中カウントをリセット
    cntMoveArray[volType] ++;                     // 動作中カウントをインクリメント
    if (cntMoveArray[volType] > VOL_MOVE_CONTINUE_MIN)
    {
      cntMoveArray[volType] = VOL_MOVE_CONTINUE_MIN;    // 動作中カウントが必要以上に上がらないようにする
    }
  }
  // 動きがなくなったら、キーリリース
  else
  {
    // 動きが止まったのがしばらく続いたらキーリリース
    // (ちょっと止まっただけでは反応しない)
    if ((keyStatusArray[reversePos] == STS_PRESS) || (keyStatusArray[normalPos] == STS_PRESS))
    {
      cntStopArray[volType] ++;   // 停止中のカウントをインクリメント (閾値以上になった場合は停止と見なす)
    }

    // つまみの操作状態によって閾値を加減する
    if (cntMoveArray[volType] >= VOL_MOVE_CONTINUE_MIN)
    {
      cntReleaseMin = VOL_RELEASE_CNT_MIN_LONG;   // プレイ中など、ぐるぐる回転させている場合、回転停止の閾値は多め
    }
    else
    {
      cntReleaseMin = VOL_RELEASE_CNT_MIN_SHORT;  // メニュー操作時など、チョンと回転させている場合、回転停止の閾値は少なめ
    }

    if (cntStopArray[volType] >= cntReleaseMin)   // 回転停止が閾値以上の時間続いた場合
    {
      cntABSameArray[volType][ENC_A] = 0;               // A相の同じ状態の連続回数リセット
      cntABSameArray[volType][ENC_B] = 0;               // B相も同様
      // 現在、逆回転に対応するキープッシュ中の場合、
      if (keyStatusArray[reversePos] == STS_PRESS)
      {
        // グローバル変数に前回←今回(停止) の値を保存
        posOldArray[volType] = posCur;                  // (次回はこの値と、ロータリーエンコーダから取得した値を比較する)
        dirOldArray[volType] = VOL_DIR_STOP;            // 回転方向は「停止」

        keyStatusArray[reversePos] = STS_RELEASE;
        Keyboard.release(keyCodeArray[reversePos]);     // 逆回転に対応するキーリリース
        cntStopArray[volType] = 0;                      // 停止中カウントをリセット
        cntMoveArray[volType] = 0;                      // 動作中カウントをリセット
      }
      // 現在、正回転に対応するキープッシュ中の場合、
      else if (keyStatusArray[normalPos] == STS_PRESS)
      {
        // グローバル変数に前回←今回(停止) の値を保存
        posOldArray[volType] = posCur;
        dirOldArray[volType] = VOL_DIR_STOP;

        keyStatusArray[normalPos] = STS_RELEASE;
        Keyboard.release(keyCodeArray[normalPos]);      // 正回転に対応するキーリリース
        cntStopArray[volType] = 0;                      // 停止中カウントをリセット
        cntMoveArray[volType] = 0;                      // 動作中カウントをリセット
      }
      else {
        ;   // あり得ない
      }
    }
  }
}
