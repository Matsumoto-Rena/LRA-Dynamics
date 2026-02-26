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
# 解析したいCSVが入っているフォルダ名
TARGET_FOLDER = "UP" 

# ★目標とする進行方向 (deg)
# Webツールの座標系ではなく、解析グラフ上の「見た目」で指定してください
# 前(UP)    = 0
# 右(RIGHT) = -90
# 左(LEFT)  = 90
# 後(DOWN)  = 180
TARGET_ROTATION = 0

# 単位変換
REAL_HEIGHT_MM = 163.0   # 撮影範囲の縦の長さ(mm) ※実測値に合わせて調整
CANVAS_HEIGHT_PX = 640   # 解像度(縦)。Webツールの解像度に合わせる(通常480か640)
# ※Webツールで「Resolution: 640 x 480」なら480
# もし正方形(640x640)で処理しているなら640
CANVAS_H_FOR_CALC = 480 
MM_PER_PX = REAL_HEIGHT_MM / CANVAS_H_FOR_CALC

# 解析する時間範囲 (ms)
TIME_LIMIT_MS = 3000

# マーカーの設定
MARKER_STEP = 5       # 何データごとに矢印を描画するか（1だと密集しすぎる場合は増やす）
STICK_LENGTH_MM = 10  # 矢印の長さ(mm)

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
    
    # 座標の差分から移動距離を計算
    df['dx_mm'] = df['X_mm'].diff().fillna(0)
    df['dy_mm'] = df['Y_mm'].diff().fillna(0)
    df['dist_mm'] = np.sqrt(df['dx_mm']**2 + df['dy_mm']**2)
    total_distance_mm = df['dist_mm'].sum()
    
    avg_speed_mm = total_distance_mm / time_sec if time_sec > 0 else 0
    
    # 最終到達地点
    last_x_mm = df['X_mm'].iloc[-1]
    last_y_mm = df['Y_mm'].iloc[-1]
    
    # === 到達地点の角度 (Displacement Angle) ===
    # スタート地点(0,0)からゴール地点への角度
    # atan2は 右=0, 上=90, 左=180, 下=-90
    rad = math.atan2(last_y_mm, last_x_mm)
    deg_standard = math.degrees(rad)
    
    # ロボット工学的に「前(Y軸プラス)」を0度としたい場合の変換
    # 右(0度) → -90度
    # 上(90度) → 0度
    trajectory_angle = deg_standard - 90
    
    # -180～180に正規化
    while trajectory_angle > 180: trajectory_angle -= 360
    while trajectory_angle < -180: trajectory_angle += 360
    
    # 目標との誤差
    error_from_target = trajectory_angle - TARGET_ROTATION
    while error_from_target > 180: error_from_target -= 360
    while error_from_target < -180: error_from_target += 360

    return {
        'Time(sec)': round(time_sec, 3),
        'Distance(mm)': round(total_distance_mm, 3),
        'Speed(mm/s)': round(avg_speed_mm, 3),
        'FinalX(mm)': round(last_x_mm, 3),
        'FinalY(mm)': round(last_y_mm, 3),
        'TrajAngle(deg)': round(trajectory_angle, 3), 
        'Error(deg)': round(error_from_target, 3),
    }

def main():
    if not os.path.exists(TARGET_FOLDER):
        print(f"エラー: フォルダ '{TARGET_FOLDER}' が見つかりません。")
        return

    csv_files = sorted(glob.glob(os.path.join(TARGET_FOLDER, "*.csv")))
    if len(csv_files) == 0:
        print("エラー: CSVファイルがありません。")
        return

    print(f"--- {TARGET_FOLDER} 解析開始 (Target: {TARGET_ROTATION} deg) ---")

    fig, ax = plt.subplots(figsize=(8, 10))
    stats_list = []

    for i, file_path in enumerate(csv_files):
        if i >= len(STYLES): break
        
        try:
            df = pd.read_csv(file_path)
            
            # --- ★データ前処理（ここが重要）---
            
            # 1. 時間フィルタ
            df = df[df['Time(ms)'] <= TIME_LIMIT_MS].copy()
            if len(df) < 2: continue

            # 2. 列名の対応 (PixelX, PixelY がWebツールからの出力)
            # 初期位置を引いて (0,0) スタートにする
            x_raw_px = df['PixelX'] - df['PixelX'].iloc[0]
            
            # ★Y軸の反転処理
            # 画像座標(下プラス) → グラフ座標(上プラス) にするためマイナスをかける
            y_raw_px = -(df['PixelY'] - df['PixelY'].iloc[0])
            
            # mmに変換
            df['X_mm'] = x_raw_px * MM_PER_PX
            df['Y_mm'] = y_raw_px * MM_PER_PX
            
            # 3. 角度の処理
            # 画像上の角度(時計回り)をグラフ上の角度(反時計回り)に変換
            # かつ、初期角度からの相対変化を見る場合
            # ※絶対角度を使いたい場合は start_ang を引かずに計算してください
            current_angles = -df['Angle(deg)'] # 符号反転
            start_ang = current_angles.iloc[0]
            
            # ロボットが最初に「前（画面上）」を向いていたとして補正
            # グラフ上では「上」は90度なので、それに合わせるオフセット
            df['GraphAngle'] = (current_angles - start_ang) + 90

        except Exception as e:
            print(f"Skip: {file_path} (Format Error? {e})")
            continue

        # 統計計算
        stats = calculate_stats_mm(df)
        
        color, base_marker, label = STYLES[i]
        stats['Run'] = label
        stats['File'] = os.path.basename(file_path)
        stats_list.append(stats)

        # === グラフ描画 ===
        
        # 1. 軌跡の線
        ax.plot(df['X_mm'], df['Y_mm'], label=label, color=color, 
                linestyle='-', linewidth=1.5, alpha=0.6, zorder=1)
        
        # 2. End地点のマーカー
        ax.scatter(df['X_mm'].iloc[-1], df['Y_mm'].iloc[-1], 
                   marker='D', s=80, color=color, zorder=5)

        # 3. 姿勢を表す矢印 (Quiver)
        # 間引きして表示
        sub_df = df.iloc[::MARKER_STEP]
        
        arrow_x = sub_df['X_mm'].values
        arrow_y = sub_df['Y_mm'].values
        angles_rad = np.radians(sub_df['GraphAngle'].values)
        
        # 矢印の成分計算
        U = np.cos(angles_rad) * STICK_LENGTH_MM
        V = np.sin(angles_rad) * STICK_LENGTH_MM
        
        ax.quiver(arrow_x, arrow_y, U, V, 
                  color=color, angles='xy', scale_units='xy', scale=1, 
                  width=0.005, headwidth=3, headlength=4, 
                  alpha=0.8, zorder=6)

        # ログ出力
        print(f"\n[{label}] {os.path.basename(file_path)}")
        print(f"  Final Pos : ({stats['FinalX(mm)']}, {stats['FinalY(mm)']})")
        print(f"  Angle Err : {stats['Error(deg)']} deg")

    # --- グラフの体裁 ---
    legend_elements = [Line2D([0], [0], marker='D', color='black', label='End', markersize=8)]
    for j in range(len(stats_list)):
        c, m, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, lw=2, label=lbl))

    ax.set_xlabel(r'$x\ \mathrm{(mm)}$', fontsize=18)
    ax.set_ylabel(r'$y\ \mathrm{(mm)}$', fontsize=18)
    ax.set_title(f"Trajectory Analysis: {TARGET_FOLDER}", fontsize=16)
    
    ax.axhline(0, color='black', linewidth=1, zorder=0)
    ax.axvline(0, color='black', linewidth=1, zorder=0)
    ax.grid(True, linestyle='--', alpha=0.7)
    
    # 軸のスケールを揃える（重要）
    ax.axis('equal')
    
    ax.legend(handles=legend_elements, loc='best', fontsize=12)
    plt.tight_layout()
    
    # 保存
    img_name = f"{TARGET_FOLDER}_analysis.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    # CSV保存
    if stats_list:
        summary_df = pd.DataFrame(stats_list)
        # 列の並び替え
        cols = ['Run', 'Time(sec)', 'Distance(mm)', 'Speed(mm/s)', 
                'FinalX(mm)', 'FinalY(mm)', 'TrajAngle(deg)', 'Error(deg)', 'File']
        summary_df = summary_df[cols]
        
        print("\n====== Summary ======")
        print(summary_df.to_string(index=False))
        summary_df.to_csv(f"{TARGET_FOLDER}_summary.csv", index=False)

    plt.show()

if __name__ == "__main__":
    main()