import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import math

# ---------------------------------------------------------
# 1. データの読み込み
# ---------------------------------------------------------
# ★ここに保存したCSVファイル名を指定してください
csv_file = 'felt_right.csv' 

# データを読み込む
try:
    df = pd.read_csv(csv_file)
except FileNotFoundError:
    print(f"エラー: '{csv_file}' が見つかりません。ファイル名をアップロードして確認してください。")
    # テスト用ダミーデータ（ファイルがない場合用）
    from io import StringIO
    csv_content = """Time(ms),Input,RawX,RawY,RelX,RelY
    0,UP,100,100,0,0
    100,UP,100,95,0,-5
    200,UP,102,90,2,-10
    300,UP,105,80,5,-20
    400,UP,108,70,8,-30
    """
    df = pd.read_csv(StringIO(csv_content))

# ---------------------------------------------------------
# 2. データの解析（距離と速度）
# ---------------------------------------------------------
# 相対座標（原点からの移動量）を使用
x = df['RelX'].values
y = df['RelY'].values
time = df['Time(ms)'].values
input_cmd = df['Input'].iloc[0] # 入力方向 (UP, DOWN, LEFT, RIGHT)

# --- A. 軌跡の長さ (総移動距離) ---
# 各ポイント間の距離を計算して合計する
dx = np.diff(x)
dy = np.diff(y)
distances = np.sqrt(dx**2 + dy**2)
total_distance = np.sum(distances)

# --- B. 移動速度 ---
# 総時間 (秒)
total_time_sec = (time[-1] - time[0]) / 1000.0

# 平均速度 (ピクセル/秒)
if total_time_sec > 0:
    average_speed = total_distance / total_time_sec
else:
    average_speed = 0

# --- C. 意図した方向とのズレ (角度) ---
# 最終地点の座標
last_x = x[-1]
last_y = y[-1]

# 実際の移動角度 (ラジアン -> 度)
# atan2は(y, x)の順で引数を取る。画面座標系(Y下向き)に注意
actual_angle_rad = math.atan2(last_y, last_x)
actual_angle_deg = math.degrees(actual_angle_rad)

# 理想の角度
ideal_angle_deg = 0
if input_cmd == "RIGHT": ideal_angle_deg = 0
elif input_cmd == "DOWN": ideal_angle_deg = 90
elif input_cmd == "LEFT": ideal_angle_deg = 180
elif input_cmd == "UP": ideal_angle_deg = -90

# ズレの計算
deviation = actual_angle_deg - ideal_angle_deg
# -180~180の範囲に正規化
while deviation > 180: deviation -= 360
while deviation < -180: deviation += 360

print(f"====== 解析結果 ({input_cmd}) ======")
print(f"総移動距離: {total_distance:.2f} px")
print(f"経過時間: {total_time_sec:.2f} sec")
print(f"平均速度: {average_speed:.2f} px/sec")
print(f"最終地点: ({last_x}, {last_y})")
print(f"角度のズレ: {deviation:.2f} 度 (プラスなら時計回り寄り)")

# ---------------------------------------------------------
# 3. グラフ描画（軌跡とズレの可視化）
# ---------------------------------------------------------
plt.figure(figsize=(8, 8))

# カメラ座標系に合わせるためY軸を反転（上がマイナス、下がプラス）
plt.gca().invert_yaxis()

# --- 1. 実際の軌跡 (青線) ---
plt.plot(x, y, marker='o', markersize=4, label='Actual Path', color='blue', alpha=0.7)
plt.scatter(0, 0, color='green', s=150, label='Start (0,0)', zorder=5) # スタート
plt.scatter(x[-1], y[-1], color='red', s=150, label='End', zorder=5)   # ゴール

# --- 2. 意図した方向 (オレンジ点線) ---
# グラフを見やすくするために、実際の移動距離と同じくらいの長さで線を引く
scale = max(abs(last_x), abs(last_y), 50) 

target_x, target_y = 0, 0
if input_cmd == "UP": target_x, target_y = 0, -scale
elif input_cmd == "DOWN": target_x, target_y = 0, scale
elif input_cmd == "LEFT": target_x, target_y = -scale, 0
elif input_cmd == "RIGHT": target_x, target_y = scale, 0

plt.plot([0, target_x], [0, target_y], linestyle='--', color='orange', linewidth=2, label=f'Intended ({input_cmd})')

# --- 3. グラフの装飾 ---
plt.title(f"Trajectory Analysis: {input_cmd}\n(Speed: {average_speed:.1f} px/s, Dev: {deviation:.1f} deg)")
plt.xlabel("Relative X (px)")
plt.ylabel("Relative Y (px)")
plt.axhline(0, color='black', linewidth=0.5)
plt.axvline(0, color='black', linewidth=0.5)
plt.grid(True, linestyle=':', alpha=0.6)
plt.legend()
plt.axis('equal') # 縦横の比率を揃える（歪み防止）

plt.show()