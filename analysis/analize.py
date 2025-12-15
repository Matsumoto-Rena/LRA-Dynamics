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
TARGET_FOLDER = "DOWN2"  # 解析したいフォルダ名

# グラフの線種設定 (カラー + 線種)
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
    """データフレームから移動統計量を計算する"""
    # 1. 経過時間 (秒)
    time_sec = (df['Time(ms)'].iloc[-1] - df['Time(ms)'].iloc[0]) / 1000.0
    
    # 2. 総移動距離 (px) - 各点間の距離の合計
    # dx, dy を計算
    df['dx'] = df['RelX'].diff().fillna(0)
    df['dy'] = df['RelY'].diff().fillna(0)
    # 距離 = sqrt(dx^2 + dy^2)
    df['dist'] = np.sqrt(df['dx']**2 + df['dy']**2)
    total_distance = df['dist'].sum()
    
    # 3. 平均速度 (px/sec)
    avg_speed = total_distance / time_sec if time_sec > 0 else 0
    
    # 4. 最終地点 (RelX, RelY)
    last_x = df['RelX'].iloc[-1]
    last_y = df['RelY'].iloc[-1]
    
    # 5. 角度のズレ (度)
    # スタート(0,0)からゴール(last_x, last_y)への角度
    # 画像座標系(Yが下プラス)でのatan2: 右=0度, 下=90度, 左=180度, 上=-90度
    angle_rad = math.atan2(last_y, last_x)
    angle_deg = math.degrees(angle_rad)
    
    # ターゲット方向の自動推定 (絶対値が大きい方が進行方向とみなす)
    # 0:Right, 90:Down, 180:Left, -90:Up
    target_deg = 0
    if abs(last_x) >= abs(last_y):
        # 横移動
        target_deg = 0 if last_x > 0 else 180
    else:
        # 縦移動
        target_deg = 90 if last_y > 0 else -90
        
    # ズレを計算 (-180 ~ 180 の範囲に正規化)
    deviation = angle_deg - target_deg
    # 補正 (例: ターゲット180で実際-170の場合、差は-350ではなく+10にしたい)
    while deviation > 180:  deviation -= 360
    while deviation < -180: deviation += 360
    
    return {
        'Time(sec)': round(time_sec, 2),
        'Distance(px)': round(total_distance, 1),
        'Speed(px/s)': round(avg_speed, 2),
        'FinalX': round(last_x, 1),
        'FinalY': round(last_y, 1),
        'AngleDev(deg)': round(deviation, 2),
        'TargetDir': target_deg # 参考用
    }

def main():
    if not os.path.exists(TARGET_FOLDER):
        print(f"エラー: フォルダ '{TARGET_FOLDER}' が見つかりません。")
        return

    csv_files = sorted(glob.glob(os.path.join(TARGET_FOLDER, "*.csv")))
    if len(csv_files) == 0:
        print("エラー: CSVファイルがありません。")
        return

    print(f"--- {TARGET_FOLDER} 解析開始 ({len(csv_files)} runs) ---")

    # グラフ準備
    plt.figure(figsize=(10, 8))
    
    # 解析結果を保存するリスト
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
        
        # 統計計算
        stats = calculate_stats(df)
        stats['Run'] = label # Run名を追加
        stats['File'] = os.path.basename(file_path) # ファイル名も追加
        stats_list.append(stats)

        # 描画
        x, y = df['RelX'], df['RelY']
        plt.plot(x, y, label=label, color=color, linestyle=linestyle, linewidth=2, alpha=0.8)
        plt.scatter(x.iloc[0], y.iloc[0], marker='o', s=120, facecolors='white', edgecolors=color, linewidth=2, zorder=5)
        plt.scatter(x.iloc[-1], y.iloc[-1], marker='D', s=120, color=color, zorder=5)
        
        # コンソールに詳細を表示（ユーザー好みの形式）
        print(f"\n====== 解析結果 ({label}) ======")
        print(f"ファイル名: {stats['File']}")
        print(f"総移動距離: {stats['Distance(px)']} px")
        print(f"経過時間  : {stats['Time(sec)']} sec")
        print(f"平均速度  : {stats['Speed(px/s)']} px/sec")
        print(f"最終地点  : ({stats['FinalX']}, {stats['FinalY']})")
        print(f"角度ズレ  : {stats['AngleDev(deg)']} 度 (推定ターゲット: {stats['TargetDir']}度)")

    # --- グラフの仕上げ ---
    legend_elements = [
        Line2D([0], [0], marker='o', color='w', markeredgecolor='black', label='Start', markersize=10),
        Line2D([0], [0], marker='D', color='black', label='End', markersize=10),
    ]
    for j in range(len(stats_list)):
        c, ls, lbl = STYLES[j]
        legend_elements.append(Line2D([0], [0], color=c, linestyle=ls, label=lbl))

    plt.title(f'Robot Trajectories: {TARGET_FOLDER}', fontsize=16)
    plt.xlabel('X Position (px)', fontsize=14)
    plt.ylabel('Y Position (px)', fontsize=14)
    plt.axhline(0, color='gray', linewidth=0.5)
    plt.axvline(0, color='gray', linewidth=0.5)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.axis('equal')
    # 画像座標系(Y下向き)に合わせるならコメントアウトを外す
    plt.gca().invert_yaxis() 
    plt.legend(handles=legend_elements, fontsize=12, loc='best')
    plt.tight_layout()
    
    # 画像保存
    img_name = f"{TARGET_FOLDER}.png"
    plt.savefig(img_name, dpi=300)
    print(f"\n[画像保存] {img_name}")
    
    # --- 表（DataFrame）の作成と保存 ---
    if stats_list:
        # 表にしたいカラムの順番を整理
        cols = ['Run', 'Time(sec)', 'Distance(px)', 'Speed(px/s)', 'FinalX', 'FinalY', 'AngleDev(deg)', 'File']
        summary_df = pd.DataFrame(stats_list)[cols]
        
        # コンソールに表を表示
        print("\n====== サマリーテーブル ======")
        print(summary_df.to_string(index=False))
        
        # CSVとして保存
        csv_name = f"{TARGET_FOLDER}_summary.csv"
        summary_df.to_csv(csv_name, index=False)
        print(f"[CSV保存] {csv_name}")

    plt.show()

if __name__ == "__main__":
    main()