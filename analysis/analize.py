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
TARGET_FOLDER = "d_UP"  
ARROW_STEP = 8  
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
    
    df['dx'] = df['RelX'].diff().fillna(0)
    df['dy'] = df['RelY'].diff().fillna(0)
    df['dist'] = np.sqrt(df['dx']**2 + df['dy']**2)
    total_distance = df['dist'].sum()
    
    avg_speed = total_distance / time_sec if time_sec > 0 else 0
    
    last_locX = df['LocalX'].iloc[-1]
    last_locY = df['LocalY'].iloc[-1]
    
    start_angle = df['Angle(deg)'].iloc[0]
    last_angle = df['Angle(deg)'].iloc[-1]
    
    deviation = last_angle - start_angle
    while deviation > 180: deviation -= 360
    while deviation < -180: deviation += 360

    return {
        'Time(sec)': round(time_sec, 2),
        'Distance(px)': round(total_distance, 1),
        'Speed(px/s)': round(avg_speed, 2),
        'FinalLocX': round(last_locX, 1),
        'FinalLocY': round(last_locY, 1),
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

    print(f"--- {TARGET_FOLDER} 解析開始 (3.0s Trimmed) ---")

    plt.figure(figsize=(8, 10))
    stats_list = []

    for i, file_path in enumerate(csv_files):
        if i >= len(STYLES): break
        
        try:
            df = pd.read_csv(file_path)
            
            # ★★★ ここが重要！ 3秒 (3000ms) 以降のデータを切り捨てる ★★★
            df = df[df['Time(ms)'] <= 3000].copy()

            if len(df) < 2: continue
        except Exception as e:
            print(f"Skip: {file_path} ({e})")
            continue

        color, linestyle, label = STYLES[i]
        stats = calculate_stats(df)
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # Web側で (0,0) スタートを保証したので、そのままプロットしてもOKですが、
        # 念のためここでもゼロ補正しておくと最強です
        x_raw, y_raw = df['LocalX'], df['LocalY']
        x = x_raw - x_raw.iloc[0]
        y = y_raw - y_raw.iloc[0]
        
        plt.plot(x, y, label=label, color=color, linestyle=linestyle, linewidth=2, alpha=0.8)
        
        plt.scatter(0, 0, marker='o', s=120, facecolors='white', edgecolors=color, linewidth=2, zorder=5)
        plt.scatter(x.iloc[-1], y.iloc[-1], marker='D', s=120, color=color, zorder=5)

        # 矢印プロット
        indices = range(0, len(df), ARROW_STEP)
        sub_df = df.iloc[indices].copy()
        
        arrow_x = sub_df['LocalX'].values - x_raw.iloc[0]
        arrow_y = sub_df['LocalY'].values - y_raw.iloc[0]
        raw_angles = sub_df['Angle(deg)'].values
        start_ang = df['Angle(deg)'].iloc[0]
        
        plot_angles_rad = np.radians((raw_angles - start_ang) + 90)
        U = np.cos(plot_angles_rad)
        V = np.sin(plot_angles_rad)
        
        plt.quiver(arrow_x, arrow_y, U, V, 
                   color=color, angles='xy', scale_units='xy', scale=1.5, 
                   width=0.002, headwidth=3, headlength=4, alpha=0.6, zorder=4)

        print(f"\n====== 解析結果 ({label}) ======")
        print(f"経過時間  : {stats['Time(sec)']} sec")
        print(f"総移動距離: {stats['Distance(px)']} px")
        print(f"最終到達点: 前 {stats['FinalLocY']} px / 横 {stats['FinalLocX']} px")
        print(f"角度ズレ  : {stats['AngleDev(deg)']} 度")

    legend_elements = [
        Line2D([0], [0], marker='o', color='w', markeredgecolor='black', label='Start', markersize=10),
        Line2D([0], [0], marker='D', color='black', label='End (3.0s)', markersize=10),
        Line2D([0], [0], color='black', marker=r'$\rightarrow$', linestyle='None', label='Orientation', markersize=15)
    ]
    for j in range(len(stats_list)):
        c, ls, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, linestyle=ls, label=lbl))

    plt.title(f'Local Trajectories: {TARGET_FOLDER}\n(Trimmed to 3.0 sec)', fontsize=14)
    plt.xlabel('Lateral Deviation (px)', fontsize=14)
    plt.ylabel('Forward Distance (px)', fontsize=14)
    
    plt.axhline(0, color='gray', linewidth=0.5, zorder=1)
    plt.axvline(0, color='gray', linewidth=0.5, zorder=1)
    plt.grid(True, linestyle=':', alpha=0.6, zorder=0)
    plt.axis('equal')
    
    # マージン調整
    xlim = plt.xlim(); ylim = plt.ylim()
    plt.xlim(xlim[0]-2, xlim[1]+2); plt.ylim(ylim[0]-2, ylim[1]+2)
    
    plt.legend(handles=legend_elements, fontsize=12, loc='best')
    plt.tight_layout()
    
    img_name = f"{TARGET_FOLDER}_local_final_trimmed.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    if stats_list:
        cols = ['Run', 'Time(sec)', 'Distance(px)', 'Speed(px/s)', 'FinalLocX', 'FinalLocY', 'AngleDev(deg)', 'File']
        summary_df = pd.DataFrame(stats_list)[cols]
        print("\n====== サマリーテーブル (3.0s Trimmed) ======")
        print(summary_df.to_string(index=False))
        summary_df.to_csv(f"{TARGET_FOLDER}_local_summary.csv", index=False)

    plt.show()

if __name__ == "__main__":
    main()