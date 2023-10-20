# LEVORGIdleStop
　Idling Stop Setting Value Reserver
　for SUBARU LEVORG NV5(2020-)

# Name
　SUBARU LEVORG NV5 Idling Stop Setting Value Reserver 

　LEVORGのアイドリングストップ設定を保存し、次回のエンジン起動時に復元する実験をしました。
 
# DEMO
　[動作させたところです。](https://youtube.com/shorts/Mov6Zhl9ecg)
　エンジンを起動し直すと設定を反映します。
　
# Features
　スバルのLEVORG(NV5 2020-)は、エンジン起動時に必ずアイドリングストップがONになってしまいます。
　アイドリングストップが不要なユーザーにとっては、エンジンを掛ける度にOFFにしなければならず、
　いささか面倒です。
　この実験により、ON/OFFの状態を保存しておき、エンジン始動時に設定を復元させることができました。

# Requirement
**ハードウェア
-Arduino Leonard または互換品
 　Arduino Leonard (Arduino ProMicro, CJMCU Beetle) 用にプログラミングしています。
 　また、動作確認もLeonard互換のArduinoを用いています。
-MCP2515 CANコントローラ＋CANレシーバ（「NiRen MCP2515_CANモジュール」の水晶発振器が16Mhzのものを利用しました。）
**ソフトウェア
-Arduino IDE
-[coryjfouler/MCP_CAN_lib ライブラリ(GitHub)](https://github.com/coryjfowler/MCP_CAN_lib)

# Installation
ハードウェア
　ArduinoとCANコントローラ(MCP2515)はSPI接続で、Arduinoの2pinをCSにしています。SPIの他の3本は規定のピンです。
　（Beetleで動作させたかった関係で、Beetleで外に出ているピンの中から選択しました。）
　Ticker(Timer1)を使っている関係で、10pinと9pinは使えません。（Arduino Leonardの仕様）
　10pin 9pinを使わなければ、空いているピンをCSに使っても動作すると思います。（プログラムの編集が必要です）
　CANモジュールの水晶発振器の発振周波数は16MHzとしてプログラムを記載しています。
　今回使用したCANモジュールは、過去には16MHzの発信機が搭載されていましたが、2023年の現状、8MHzが搭載されて
　いるものが大半のようですので、8MHzのモジュールを利用する場合はプログラムの MCP_16MHZ の部分を MCP_8MHZ に
　書き換えてください。
　発振周波数はCANモジュール上に搭載されている水晶発振器の表面に記載されていますので、ご確認ください。

# Usage
　電源は車体のアクセサリ電源から取得してください。
　（プログラムコード上は他の電源でも動作すると思いますが、アクセサリ電源でしか動作を確認していません。）
 
　車体のCANの線はドライバーズモニタリングシステムのコネクタ辺りから取るのが良いようです。
　セキュリティ面を考えてのことだと思いますが、OBDのコネクタにはCANのデータは出ていないそうです。
　また、車両に結線する際はCANレシーバの終端抵抗120Ωは不要です。（車体のCAN幹線に設置されているため。）
　NiRen MCP2515_CANモジュールですと、ジャンパJ1をオープンにした状態が終端抵抗が接続されていない状態です。
　市販されているCANトランシーバのモジュール基板は終端抵抗がハンダ付けされているものがほとんどです。
　これらのモジュールを使う場合は、終端抵抗を取り除く必要があります。
 
　なお、CANの一般的な注意事項ですが、CANの幹線、枝線は、ノイズ対策として２本よりにするのが通例です。
　CANの幹線、枝線は間に増幅器等がなく、ノイズが乗った場合にCANの線全体に波及します。
　これを避けるため２本よりにします。
　車体本体のハーネスのCANの線も、テープを剥がすと２本よりになっています。
　（このことを利用して、２本よりになった線を探すことでCANの配線を探すこともできます。）

# Note
　実験結果として、アイドリングストップの設定値の保存と復元ができました。
　本プログラムはあくまで実験用です。日常用途などで継続的に利用することを想定していませんので、
　皆さんが追実験する場合も動作確認や実験までに留め、日常的に利用するといったことはおやめください。
　（自動車に関することですので、想定されていない日常的な利用は危険が伴う可能性があります。）
　なお、VN5A(A型)でしか動作確認をしていません。A型以外では不具合が生じる可能性があります。

　本プログラムはMITライセンスに準拠します。
　下記のライセンスに記載されているように、本プログラムを利用した場合の損害等を含めた一切について、
　作成者である私Cottonhouseは責を負いません。
　本プログラムを利用するということは、ライセンスに同意したということですので、ご自分の責任で利用
　なさってください。
　また、本プログラムはAS ISです。あるがままで、不具合等ある可能性があります。
　作者は動作の保証をしませんし、不具合の改修の責も負いません。

# Author
* 作成者：Cottonhouse
* as [にゃんカラ](https://minkara.carview.co.jp/userid/2407630/profile/)

# License
"Idling Stop Setting Value Reserver" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).

Copyright (c) 2023 Cottonhouse
Released under the MIT license
https://opensource.org/licenses/mit-license.php

MITライセンスですので商用利用可ですが、ライセンスに則ってCopyright表示とMITライセンスの全文URL表示をお願いします。