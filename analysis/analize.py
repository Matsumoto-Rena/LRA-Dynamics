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
TARGET_FOLDER = "1218_LEFT" 

# ★目標とする進行方向 (deg)
# 前(UP)    = 0
# 右(RIGHT) = -90
# 左(LEFT)  = 90
# 後(DOWN)  = 180
TARGET_ROTATION = 90

# 単位変換
REAL_HEIGHT_MM = 163.0
CANVAS_HEIGHT_PX = 480  
MM_PER_PX = REAL_HEIGHT_MM / CANVAS_HEIGHT_PX 

# マーカーの設定
MARKER_STEP = 1       # 全点描写
STICK_LENGTH_MM = 0.3 # 棒の長さ

# 色とマーカー形状
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
    
    df['dx_mm'] = df['LocalX_mm'].diff().fillna(0)
    df['dy_mm'] = df['LocalY_mm'].diff().fillna(0)
    df['dist_mm'] = np.sqrt(df['dx_mm']**2 + df['dy_mm']**2)
    total_distance_mm = df['dist_mm'].sum()
    
    avg_speed_mm = total_distance_mm / time_sec if time_sec > 0 else 0
    
    last_locX_mm = df['LocalX_mm'].iloc[-1]
    last_locY_mm = df['LocalY_mm'].iloc[-1]
    
    # === ★変更点: 到達地点の角度 (Displacement Angle) を計算 ===
    # atan2(y, x) は標準で「右(X軸正)が0度、上(Y軸正)が90度」
    # ロボット座標系(上が0度)に合わせるため、(angle - 90) を行います。
    # 例: 上(90) -> 0度, 右(0) -> -90度
    
    rad = math.atan2(last_locY_mm, last_locX_mm)
    deg_standard = math.degrees(rad)
    
    # 上(Y軸)を0度とする変換
    trajectory_angle = deg_standard - 90
    
    # -180～180に正規化
    while trajectory_angle > 180: trajectory_angle -= 360
    while trajectory_angle < -180: trajectory_angle += 360
    
    # 目標との誤差 (Error)
    error_from_target = trajectory_angle - TARGET_ROTATION
    while error_from_target > 180: error_from_target -= 360
    while error_from_target < -180: error_from_target += 360

    return {
        'Time(sec)': round(time_sec, 3),
        'Distance(mm)': round(total_distance_mm, 3),
        'Speed(mm/s)': round(avg_speed_mm, 3),
        'FinalX(mm)': round(last_locX_mm, 3),
        'FinalY(mm)': round(last_locY_mm, 3),
        'TrajAngle(deg)': round(trajectory_angle, 3), # 実際の進行方向
        'Error(deg)': round(error_from_target, 3),    # 目標からのズレ
    }

def main():
    if not os.path.exists(TARGET_FOLDER):
        print(f"エラー: フォルダ '{TARGET_FOLDER}' が見つかりません。")
        return

    csv_files = sorted(glob.glob(os.path.join(TARGET_FOLDER, "*.csv")))
    if len(csv_files) == 0:
        print("エラー: CSVファイルがありません。")
        return

    print(f"--- {TARGET_FOLDER} 解析開始 (Target: {TARGET_ROTATION} deg, Displacement Error) ---")

    fig, ax = plt.subplots(figsize=(8, 10))
    stats_list = []

    for i, file_path in enumerate(csv_files):
        if i >= len(STYLES): break
        
        try:
            df = pd.read_csv(file_path)
            df = df[df['Time(ms)'] <= 3000].copy()
            if len(df) < 2: continue
        except Exception as e:
            print(f"Skip: {file_path} ({e})")
            continue

        color, base_marker, label = STYLES[i]
        
        x_raw_px = df['LocalX'] - df['LocalX'].iloc[0]
        y_raw_px = df['LocalY'] - df['LocalY'].iloc[0]
        
        df['LocalX_mm'] = x_raw_px * MM_PER_PX
        df['LocalY_mm'] = y_raw_px * MM_PER_PX
        
        stats = calculate_stats_mm(df)
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # 1. 軌跡の線 (線あり)
        x = df['LocalX_mm']
        y = df['LocalY_mm']
        ax.plot(x, y, label=label, color=color, linestyle='-', linewidth=1.0, alpha=0.4, zorder=1)
        
        # End マーカー
        ax.scatter(df['LocalX_mm'].iloc[-1], df['LocalY_mm'].iloc[-1], marker='D', s=80, color=color, zorder=5)

        # 2. マーカー＋棒 (全点)
        indices = range(0, len(df), MARKER_STEP)
        sub_df = df.iloc[indices].copy()
        
        arrow_x = sub_df['LocalX_mm'].values
        arrow_y = sub_df['LocalY_mm'].values
        raw_angles = sub_df['Angle(deg)'].values
        start_ang = df['Angle(deg)'].iloc[0]
        
        plot_angles_rad = np.radians((raw_angles - start_ang) + 90)
        
        U = np.cos(plot_angles_rad) * STICK_LENGTH_MM
        V = np.sin(plot_angles_rad) * STICK_LENGTH_MM
        
        ax.scatter(arrow_x, arrow_y, marker=base_marker, s=25, facecolors='white', edgecolors=color, linewidth=1, alpha=0.8, zorder=6)
        ax.quiver(arrow_x, arrow_y, U, V, 
                   color=color, angles='xy', scale_units='xy', scale=1, 
                   width=0.003, headwidth=1, headlength=0, headaxislength=0, 
                   alpha=1.0, zorder=7)

        print(f"\n====== 解析結果 ({label}) ======")
        print(f"経過時間  : {stats['Time(sec)']} sec")
        print(f"総移動距離: {stats['Distance(mm)']} mm")
        print(f"到達座標  : ({stats['FinalX(mm)']}, {stats['FinalY(mm)']})")
        print(f"進行角度  : {stats['TrajAngle(deg)']} 度 (Forward=0)")
        print(f"角度誤差  : {stats['Error(deg)']} 度 (Target={TARGET_ROTATION})")

    # --- グラフ設定 ---
    legend_elements = [
        Line2D([0], [0], marker='D', color='black', label='End', markersize=8),
    ]
    for j in range(len(stats_list)):
        c, m, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, linestyle='None', marker=m, markerfacecolor='white', markeredgecolor=c, label=lbl, markersize=8))

    ax.set_xlabel(r'$x\ \mathrm{(mm)}$', fontsize=21)
    ax.set_ylabel(r'$y\ \mathrm{(mm)}$', fontsize=21)
    
    ax.axhline(0, color='gray', linewidth=0.5, zorder=0)
    ax.axvline(0, color='gray', linewidth=0.5, zorder=0)
    ax.grid(True, linestyle=':', alpha=0.6, zorder=0)
    
    ax.axis('equal')
    ax.margins(0.05)
    
    ax.legend(handles=legend_elements, fontsize=21, loc='best')
    plt.tight_layout()
    
    img_name = f"{TARGET_FOLDER}_mm_traj_error.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    if stats_list:
        cols = ['Run', 'Time(sec)', 'Distance(mm)', 'Speed(mm/s)', 'FinalX(mm)', 'FinalY(mm)', 'TrajAngle(deg)', 'Error(deg)', 'File']
        summary_df = pd.DataFrame(stats_list)[cols]
        print("\n====== サマリーテーブル (mm) ======")
        print(summary_df.to_string(index=False))
        summary_df.to_csv(f"{TARGET_FOLDER}_mm_summary.csv", index=False)

    plt.show()

if __name__ == "__main__":
    main()