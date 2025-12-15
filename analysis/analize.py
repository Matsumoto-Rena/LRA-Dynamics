import pandas as pd
import matplotlib.pyplot as plt
import glob
import os
import numpy as np
from matplotlib.lines import Line2D
import math

# ==========================================
# ★設定エリア
# ==========================================
TARGET_FOLDER = "test"  # 解析したいフォルダ名

# グラフの線種設定
STYLES = [
    ('red',         '-',       'Run 1'),
    ('blue',        '--',      'Run 2'),
    ('green',       '-.',      'Run 3'),
    ('orange',      ':',       'Run 4'),
    ('purple',      (0, (3, 1, 1, 1)), 'Run 5'),
]

# ==========================================
# 関数定義
# ==========================================
def calculate_stats(df):
    time_sec = (df['Time(ms)'].iloc[-1] - df['Time(ms)'].iloc[0]) / 1000.0
    
    # 距離計算
    df['dx'] = df['RelX'].diff().fillna(0)
    df['dy'] = df['RelY'].diff().fillna(0)
    df['dist'] = np.sqrt(df['dx']**2 + df['dy']**2)
    total_distance = df['dist'].sum()
    
    avg_speed = total_distance / time_sec if time_sec > 0 else 0
    
    # ★重要: 解析結果もローカル座標基準で出す
    last_locX = df['LocalX'].iloc[-1]
    last_locY = df['LocalY'].iloc[-1]
    
    # 角度ズレ計算 (今回はすでにロボット基準のローカル座標なので計算が楽)
    # LocalYが進行方向(目標)なので、そこからどれだけズレたか
    # atan2(LocalX, LocalY) で、進行方向(Y軸)からの角度が出る
    # 通常atan2(y, x)だが、ここでは進行方向Yを基準にしたいので atan2(x, y) を使うと
    # 正面(Y)が0度、右(X)が90度になる。
    angle_rad = math.atan2(last_locX, last_locY)
    deviation = math.degrees(angle_rad)
    
    return {
        'Time(sec)': round(time_sec, 2),
        'Distance(px)': round(total_distance, 1),
        'Speed(px/s)': round(avg_speed, 2),
        'FinalLocX': round(last_locX, 1), # 横ズレ
        'FinalLocY': round(last_locY, 1), # 進行距離
        'AngleDev(deg)': round(deviation, 2),
    }

def main():
    if not os.path.exists(TARGET_FOLDER):
        print(f"エラー: フォルダ '{TARGET_FOLDER}' が見つかりません。")
        return

    csv_files = sorted(glob.glob(os.path.join(TARGET_FOLDER, "*.csv")))
    if len(csv_files) == 0:
        print("エラー: CSVファイルがありません。")
        return

    print(f"--- {TARGET_FOLDER} 解析開始 (Local Coordinates) ---")

    plt.figure(figsize=(8, 10)) # 縦長の方が見やすいかも
    
    stats_list = []

    for i, file_path in enumerate(csv_files):
        if i >= len(STYLES): break
        
        try:
            df = pd.read_csv(file_path)
            if len(df) < 2: continue
        except Exception as e:
            print(f"Skip: {file_path} ({e})")
            continue

        color, linestyle, label = STYLES[i]
        
        stats = calculate_stats(df)
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # ★重要: ローカル座標 (LocalX, LocalY) をプロット
        # LocalX: 横方向 (Right)
        # LocalY: 進行方向 (Forward)
        x, y = df['LocalX'], df['LocalY']
        
        plt.plot(x, y, label=label, color=color, linestyle=linestyle, linewidth=2, alpha=0.8)
        plt.scatter(x.iloc[0], y.iloc[0], marker='o', s=120, facecolors='white', edgecolors=color, linewidth=2, zorder=5)
        plt.scatter(x.iloc[-1], y.iloc[-1], marker='D', s=120, color=color, zorder=5)
        
        print(f"\n====== 解析結果 ({label}) ======")
        print(f"総移動距離: {stats['Distance(px)']} px")
        print(f"最終到達点: 前 {stats['FinalLocY']} px / 横 {stats['FinalLocX']} px")
        print(f"角度ズレ  : {stats['AngleDev(deg)']} 度")

    # --- グラフ設定 ---
    legend_elements = [
        Line2D([0], [0], marker='o', color='w', markeredgecolor='black', label='Start', markersize=10),
        Line2D([0], [0], marker='D', color='black', label='End', markersize=10),
    ]
    for j in range(len(stats_list)):
        c, ls, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, linestyle=ls, label=lbl))

    plt.title(f'Local Trajectories: {TARGET_FOLDER}\n(Up = Initial Heading)', fontsize=14)
    plt.xlabel('Lateral Deviation (px)', fontsize=14) # 横ズレ
    plt.ylabel('Forward Distance (px)', fontsize=14)  # 進行距離
    
    plt.axhline(0, color='gray', linewidth=0.5)
    plt.axvline(0, color='gray', linewidth=0.5)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.axis('equal')
    
    # ローカル座標なら、進行方向は「上」にしたいので、Y軸反転は不要（むしろ通常通り上がプラスでOK）
    # plt.gca().invert_yaxis() 
    
    plt.legend(handles=legend_elements, fontsize=12, loc='best')
    plt.tight_layout()
    
    img_name = f"{TARGET_FOLDER}_local.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    if stats_list:
        cols = ['Run', 'Time(sec)', 'Distance(px)', 'Speed(px/s)', 'FinalLocX', 'FinalLocY', 'AngleDev(deg)', 'File']
        summary_df = pd.DataFrame(stats_list)[cols]
        print("\n====== サマリーテーブル (Local Coords) ======")
        print(summary_df.to_string(index=False))
        summary_df.to_csv(f"{TARGET_FOLDER}_local_summary.csv", index=False)

    plt.show()

if __name__ == "__main__":
    main()