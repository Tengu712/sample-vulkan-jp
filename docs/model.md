# モデル

## Models

用意しているモデルは次の通り。

- 正方形
  - square.json
  - position, uv
  - ui.vert
- ユタ・ティーポット
  - utah.json
  - position, normal

## Vertex Data Contents

一頂点に含まれうるデータは以下の通り。

| name | meaning | size | mask |
| ---- | ------- | ---- | ---- |
| position | ローカル座標 | 32bit * 3| なし |
| normal | 法線ベクトル | 32bit * 3 | `0b01` |
| uv | UV座標 | 32bit * 2 | `0b10` |

## Raw Data Format

converter.jsによって変換された後のデータのフォーマットは次の通り。

1. 一頂点にどのデータが含まれるかビットフラグ (32bit integer)
2. 頂点数 (32bit integer)
3. 頂点データ (32bit * 頂点数 * 一頂点サイズ float)
4. インデックス数 (32bit integer)
5. インデックスデータ (32bit * インデックス数 integer)
