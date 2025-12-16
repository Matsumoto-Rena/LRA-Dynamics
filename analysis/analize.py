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
TARGET_FOLDER = "d_LEFT" 

# 単位変換
REAL_HEIGHT_MM = 93.0   
CANVAS_HEIGHT_PX = 480  
MM_PER_PX = REAL_HEIGHT_MM / CANVAS_HEIGHT_PX 

# マーカーの設定
MARKER_STEP = 15      
STICK_LENGTH_MM = 0.3 # 棒の長さ

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
def calculate_stats_mm(df):
    time_sec = (df['Time(ms)'].iloc[-1] - df['Time(ms)'].iloc[0]) / 1000.0
    
    df['dx_mm'] = df['LocalX_mm'].diff().fillna(0)
    df['dy_mm'] = df['LocalY_mm'].diff().fillna(0)
    df['dist_mm'] = np.sqrt(df['dx_mm']**2 + df['dy_mm']**2)
    total_distance_mm = df['dist_mm'].sum()
    
    avg_speed_mm = total_distance_mm / time_sec if time_sec > 0 else 0
    
    last_locX_mm = df['LocalX_mm'].iloc[-1]
    last_locY_mm = df['LocalY_mm'].iloc[-1]
    
    start_angle = df['Angle(deg)'].iloc[0]
    last_angle = df['Angle(deg)'].iloc[-1]
    deviation = last_angle - start_angle
    while deviation > 180: deviation -= 360
    while deviation < -180: deviation += 360

    return {
        'Time(sec)': round(time_sec, 2),
        'Distance(mm)': round(total_distance_mm, 1),
        'Speed(mm/s)': round(avg_speed_mm, 1),
        'FinalX(mm)': round(last_locX_mm, 1),
        'FinalY(mm)': round(last_locY_mm, 1),
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

    print(f"--- {TARGET_FOLDER} 解析開始 (Start Marker Changed) ---")

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

        color, linestyle, label = STYLES[i]
        
        # px -> mm 変換
        x_raw_px = df['LocalX'] - df['LocalX'].iloc[0]
        y_raw_px = df['LocalY'] - df['LocalY'].iloc[0]
        
        df['LocalX_mm'] = x_raw_px * MM_PER_PX
        df['LocalY_mm'] = y_raw_px * MM_PER_PX
        
        stats = calculate_stats_mm(df)
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # 軌跡の線 (薄く)
        x = df['LocalX_mm']
        y = df['LocalY_mm']
        ax.plot(x, y, label=label, color=color, linestyle=linestyle, linewidth=1.0, alpha=0.4, zorder=1)
        
        # Start/End マーカー
        # ★ここを変更: Startを星形('*')にしてサイズアップ (s=150)
        ax.scatter(0, 0, marker='*', s=150, facecolors='white', edgecolors=color, linewidth=2, zorder=5)
        # Endはダイヤ('D')のまま
        ax.scatter(x.iloc[-1], y.iloc[-1], marker='D', s=80, color=color, zorder=5)

        # 丸と棒 (Heading)
        indices = range(0, len(df), MARKER_STEP)
        sub_df = df.iloc[indices].copy()
        
        arrow_x = sub_df['LocalX_mm'].values
        arrow_y = sub_df['LocalY_mm'].values
        raw_angles = sub_df['Angle(deg)'].values
        start_ang = df['Angle(deg)'].iloc[0]
        
        plot_angles_rad = np.radians((raw_angles - start_ang) + 90)
        
        U = np.cos(plot_angles_rad) * STICK_LENGTH_MM
        V = np.sin(plot_angles_rad) * STICK_LENGTH_MM
        
        # 丸
        ax.scatter(arrow_x, arrow_y, marker='o', s=50, facecolors='white', edgecolors=color, linewidth=1, alpha=0.9, zorder=6)
        
        # 棒
        ax.quiver(arrow_x, arrow_y, U, V, 
                   color=color, angles='xy', scale_units='xy', scale=1, 
                   width=0.004, headwidth=1, headlength=0, headaxislength=0, 
                   alpha=1.0, zorder=7)

        print(f"\n====== 解析結果 ({label}) ======")
        print(f"経過時間  : {stats['Time(sec)']} sec")
        print(f"総移動距離: {stats['Distance(mm)']} mm")
        print(f"最終到達点: 前 {stats['FinalY(mm)']} mm / 横 {stats['FinalX(mm)']} mm")
        print(f"角度ズレ  : {stats['AngleDev(deg)']} 度")

    # --- グラフ設定 ---
    legend_elements = [
        # ★凡例も星形に変更
        Line2D([0], [0], marker='*', color='w', markeredgecolor='black', label='Start', markersize=12),
        Line2D([0], [0], marker='D', color='black', label='End', markersize=8),
        Line2D([0], [0], marker=r'$\circ$', color='black', linestyle='-', label='Heading', markersize=10)
    ]
    for j in range(len(stats_list)):
        c, ls, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, linestyle=ls, label=lbl))

    ax.set_title(f'Local Trajectories: {TARGET_FOLDER}\n(Trimmed to 3.0s, Unit: mm)', fontsize=14)
    ax.set_xlabel('Lateral Deviation (mm)', fontsize=14)
    ax.set_ylabel('Forward Distance (mm)', fontsize=14)
    
    ax.axhline(0, color='gray', linewidth=0.5, zorder=0)
    ax.axvline(0, color='gray', linewidth=0.5, zorder=0)
    ax.grid(True, linestyle=':', alpha=0.6, zorder=0)
    ax.axis('equal')
    
    xlim = ax.get_xlim(); ylim = ax.get_ylim()
    ax.set_xlim(xlim[0]-5, xlim[1]+5)
    ax.set_ylim(ylim[0]-5, ylim[1]+5)
    
    ax.legend(handles=legend_elements, fontsize=12, loc='best')
    plt.tight_layout()
    
    img_name = f"{TARGET_FOLDER}_mm_final.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    if stats_list:
        cols = ['Run', 'Time(sec)', 'Distance(mm)', 'Speed(mm/s)', 'FinalX(mm)', 'FinalY(mm)', 'AngleDev(deg)', 'File']
        summary_df = pd.DataFrame(stats_list)[cols]
        print("\n====== サマリーテーブル (mm) ======")
        print(summary_df.to_string(index=False))
        summary_df.to_csv(f"{TARGET_FOLDER}_mm_summary.csv", index=False)

    plt.show()

if __name__ == "__main__":
    main()