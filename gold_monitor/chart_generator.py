#!/usr/bin/env python3
"""Chart generator using system python3 (has matplotlib)"""
import sys
import json
import os

# Use non-interactive backend before any matplotlib import
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta

def generate_price_chart(price_history, output_path, base_price):
    if not price_history:
        print("No price data to plot")
        return False

    times = [datetime.strptime(p["timestamp"], "%Y-%m-%d %H:%M:%S") for p in price_history]
    prices = [p["price"] for p in price_history]

    fig, ax = plt.subplots(figsize=(10, 5))

    ax.plot(times, prices, color='#FFD700', linewidth=2, label='AU0 Price', zorder=3)
    ax.fill_between(times, prices, alpha=0.3, color='#FFD700')

    #基准价水平线
    ax.axhline(y=base_price, color='red', linestyle='--', linewidth=1.2, alpha=0.8, label=f'Base Price {base_price}')

    latest = prices[-1]
    change = latest - base_price
    change_pct = (change / base_price) * 100

    if change >= 0:
        ax.set_facecolor('#1a1a2e')
        fig.patch.set_facecolor('#0f0f1a')
        ax.tick_params(colors='white')
        ax.xaxis.label.set_color('white')
        ax.yaxis.label.set_color('white')
        ax.title.set_color('white')
        for spine in ax.spines.values():
            spine.set_color('#444')
        color = '#00e676'
    else:
        ax.set_facecolor('#f8f8f8')
        fig.patch.set_facecolor('white')
        color = '#ff5252'

    sign = '+' if change >= 0 else ''

    ax.set_title('Gold Price (AU0) Real-time Trend', fontsize=14, fontweight='bold', pad=10)
    ax.set_xlabel('Time', fontsize=11)
    ax.set_ylabel('Price (CNY/g)', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.4)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
    fig.autofmt_xdate(rotation=45)

    ax.annotate(
        f'{latest}\n({sign}{change:.2f} / {sign}{change_pct:.2f}%)',
        xy=(times[-1], latest), xytext=(10, 0),
        textcoords='offset points',
        fontsize=9, color=color,
        fontweight='bold',
        va='center'
    )

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc='upper left', fontsize=9)

    plt.tight_layout()
    plt.savefig(output_path, dpi=120, bbox_inches='tight')
    plt.close()
    print(f"Chart saved to {output_path}")
    return True


if __name__ == "__main__":
    # Standalone test
    now = datetime.now()
    fake_history = [
        {"price": 990 + i * 0.5, "timestamp": (now - timedelta(minutes=30-i)).strftime("%Y-%m-%d %H:%M:%S")}
        for i in range(30, 0, -1)
    ]
    output = "/tmp/gold_chart.png"
    generate_price_chart(fake_history, output, base_price=995)