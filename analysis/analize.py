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
TARGET_FOLDER = "UP" 

# 目標進行方向 (0:上, -90:右, 90:左, 180:下)
TARGET_ROTATION = 0

# 単位変換
REAL_HEIGHT_MM = 163.0   
CANVAS_H_FOR_CALC = 480 
MM_PER_PX = REAL_HEIGHT_MM / CANVAS_H_FOR_CALC

# マーカー設定
MARKER_STEP = 8       # 間引き間隔
STICK_LENGTH_MM = 0.3 # ★棒の長さ（少し長めにすると向きが見やすいです）

STYLES = [
    ('red',    'o', 'Run 1'),
    ('blue',   '^', 'Run 2'),
    ('green',  's', 'Run 3'),
    ('orange', 'D', 'Run 4'),
    ('purple', '*', 'Run 5'),
]

# ==========================================
# 関数定義
# ==========================================
def calculate_stats_mm(df):
    time_sec = (df['Time(ms)'].iloc[-1] - df['Time(ms)'].iloc[0]) / 1000.0
    
    df['dx_mm'] = df['X_mm'].diff().fillna(0)
    df['dy_mm'] = df['Y_mm'].diff().fillna(0)
    df['dist_mm'] = np.sqrt(df['dx_mm']**2 + df['dy_mm']**2)
    total_distance_mm = df['dist_mm'].sum()
    
    last_x_mm = df['X_mm'].iloc[-1]
    last_y_mm = df['Y_mm'].iloc[-1]
    
    rad = math.atan2(last_y_mm, last_x_mm)
    deg_standard = math.degrees(rad)
    trajectory_angle = deg_standard - 90
    
    while trajectory_angle > 180: trajectory_angle -= 360
    while trajectory_angle < -180: trajectory_angle += 360
    
    error_from_target = trajectory_angle - TARGET_ROTATION
    while error_from_target > 180: error_from_target -= 360
    while error_from_target < -180: error_from_target += 360

    return {
        'Time(sec)': round(time_sec, 3),
        'Distance(mm)': round(total_distance_mm, 3),
        'FinalX(mm)': round(last_x_mm, 3),
        'FinalY(mm)': round(last_y_mm, 3),
        'TrajAngle(deg)': round(trajectory_angle, 3), 
        'Error(deg)': round(error_from_target, 3),
    }

def main():
    if not os.path.exists(TARGET_FOLDER):
        print(f"エラー: フォルダ '{TARGET_FOLDER}' がありません。")
        return

    csv_files = sorted(glob.glob(os.path.join(TARGET_FOLDER, "*.csv")))
    if not csv_files:
        print("エラー: CSVファイルが見つかりません。")
        return

    print(f"--- 解析開始 (丸＋棒スタイル) ---")

    fig, ax = plt.subplots(figsize=(8, 10))
    stats_list = []

    for i, file_path in enumerate(csv_files):
        if i >= len(STYLES): break
        
        try:
            df = pd.read_csv(file_path)
            if len(df) < 5: continue

            if 'PixelX' not in df.columns:
                if 'LocalX' in df.columns:
                    df = df.rename(columns={'LocalX': 'PixelX', 'LocalY': 'PixelY'})
                else:
                    continue

            # 座標変換
            x_raw_px = df['PixelX'] - df['PixelX'].iloc[0]
            y_raw_px = -(df['PixelY'] - df['PixelY'].iloc[0]) 
            
            df['X_mm'] = x_raw_px * MM_PER_PX
            df['Y_mm'] = y_raw_px * MM_PER_PX
            
            # 角度計算
            window = 5
            df['dx_smooth'] = df['X_mm'].diff(window).fillna(0)
            df['dy_smooth'] = df['Y_mm'].diff(window).fillna(0)
            df['CalcRad'] = np.arctan2(df['dy_smooth'], df['dx_smooth'])
            df['is_moving'] = (df['dx_smooth']**2 + df['dy_smooth']**2) > 0.5 

        except Exception as e:
            print(f"Skip: {file_path} ({e})")
            continue

        stats = calculate_stats_mm(df)
        color, base_marker, label = STYLES[i]
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # 1. 軌跡の線
        ax.plot(df['X_mm'], df['Y_mm'], label=label, color=color, linewidth=1.0, alpha=0.5)
        
        # 2. 終了点
        ax.scatter(df['X_mm'].iloc[-1], df['Y_mm'].iloc[-1], marker='D', s=60, color=color)

        # 3. マーカーと棒 (Stick)
        sub_df = df.iloc[::MARKER_STEP]
        sub_df = sub_df[sub_df['is_moving']]
        
        if not sub_df.empty:
            arrow_x = sub_df['X_mm'].values
            arrow_y = sub_df['Y_mm'].values
            angles_rad = sub_df['CalcRad'].values
            
            U = np.cos(angles_rad) * STICK_LENGTH_MM
            V = np.sin(angles_rad) * STICK_LENGTH_MM
            
            # (A) マーカー（丸や四角）を描画
            # facecolors='none' にすると白抜きになります。お好みで 'white' や color に変更可
            ax.scatter(arrow_x, arrow_y, marker=base_marker, s=30, 
                       edgecolors=color, facecolors='white', linewidth=1.0, alpha=0.9, zorder=5)

            # (B) 棒（矢尻のない矢印）を描画
            # headlength=0, headaxislength=0 で矢尻を消滅させます
            ax.quiver(arrow_x, arrow_y, U, V, color=color, angles='xy', scale_units='xy', scale=1, 
                      width=0.003, headwidth=1, headlength=0, headaxislength=0, alpha=1.0, zorder=4)

        print(f"[{label}] Dist: {stats['Distance(mm)']}mm")

    # グラフ設定
    ax.set_xlabel('x (mm)', fontsize=14)
    ax.set_ylabel('y (mm)', fontsize=14)
    ax.set_title(f"Trajectory: {TARGET_FOLDER}", fontsize=16)
    ax.axhline(0, color='black', linewidth=0.8)
    ax.axvline(0, color='black', linewidth=0.8)
    ax.grid(True, linestyle='--', alpha=0.6)
    ax.axis('equal')
    
    if stats_list:
        ax.legend()
        plt.savefig(f"{TARGET_FOLDER}_stick_style.png", dpi=300)
        pd.DataFrame(stats_list).to_csv(f"{TARGET_FOLDER}_summary.csv", index=False)
        plt.show()

if __name__ == "__main__":
    main()