# Sample Program using Vulkan for Japanese

## What is this?


## Contents

このサンプルプログラムに含まれる技術は次の通り。

- デバイスに対してコマンドを送り計算させる
- プッシュ定数を用いてシェーダにデータを送る
- ディスクリプタセットを用いてシェーダにデータを送る
- オフスクリーンレンダリングで描画する
- Win32APIでウィンドウを作成・描画する (Windows限定)
- GLSL 4.50でシェーディングする
- 外部のSPIR-Vデータからシェーダオブジェクトを作成する
- 外部の3Dモデルデータから3Dモデルオブジェクトを作成する


## Build

ビルド結果はbinディレクトリ直下に生成される。

### Windows

次をインストールする。

- MSVC (cl)
- Node.js (node)
- Vulkan SDK (glslc)

次をMSVC環境コンソールで行う。
ただしMSVC環境は、`x64 Native Tools Command Prompt`を用いるか、`vcvars64.bat`を実行することで簡単に用意できる。

```
git clone https://github.com/Tengu712/sample-vulkan-jp.git
cd sample-vulkan-jp
.\build.bat
```


## Usage

日本語の標準出力を行う都合上、特にWindowsでは文字コード設定を「UTF-8」にしておくこと。

コマンドライン引数を与えることでプログラムを選択する。

- (なし): オフスクリーンレンダリング
- `offscreen`: オフスクリーンレンダリング
- `windows`: Win32APIで作成したウィンドウへの描画

オフスクリーンレンダリングの結果は`rendering-result.png`として実行ファイルと同一ディレクトリに生成される。
