#!/usr/bin/env python3
import json
import logging
import sys
import time
from datetime import datetime

import akshare as ak
import requests
import schedule

CONFIG_FILE = "/home/admin/gold_monitor/gold_monitor/config.json"
STATE_FILE = ".monitor_state.json"
TIMEOUT_MINUTES = 30
PERIODIC_REPORT_MINUTES = 15

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.FileHandler("monitor.log", encoding="utf-8"),
        logging.StreamHandler(),
    ],
)
logger = logging.getLogger(__name__)


def load_config():
    with open(CONFIG_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def load_state():
    try:
        with open(STATE_FILE, "r") as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return {"notified_high": False, "notified_low": False, "last_price": None}


def save_state(state):
    with open(STATE_FILE, "w") as f:
        json.dump(state, f)


def get_gold_price(config):
    df = ak.spot_golden_benchmark_sge()
    latest = df.iloc[-1]
    current_price = float(latest["晚盘价"])
    base_price = float(latest["早盘价"])
    logger.info(
        f"黄金数据: 时间={latest['交易时间']}, 晚盘价={latest['晚盘价']}, 早盘价={latest['早盘价']}"
    )
    return current_price, base_price


def send_notification(pushplus_token, title, content):
    url = "http://www.pushplus.plus/send"
    data = {
        "token": pushplus_token,
        "title": title,
        "content": content,
        "type": "text",
    }
    response = requests.post(url, json=data, timeout=10)
    result = response.json()
    if result.get("code") == 200:
        logger.info(f"通知发送成功: {title}")
        return True
    else:
        logger.error(f"通知发送失败: {result}")
        return False


def periodic_report():
    config = load_config()
    notify_config = config["notify"]
    if not notify_config.get("enabled", True):
        return
    try:
        price, base_price = get_gold_price(config)
        change_percent = ((price - base_price) / base_price) * 100
        trend = "↑" if change_percent > 0 else "↓" if change_percent < 0 else "→"
        send_notification(
            notify_config["pushplus_token"],
            f"金价定时推送 {trend} {abs(change_percent):.2f}%",
            f"当前金价(晚盘) {price} 元\n基准价(早盘) {base_price} 元\n涨跌 {change_percent:+.2f}%\n时间: {datetime.now()}",
        )
    except Exception as e:
        logger.error(f"定时推送异常: {e}")


def monitor():
    config = load_config()
    state = load_state()

    try:
        price, base_price = get_gold_price(config)
        thresholds = config["thresholds"]
        rise_percent = thresholds.get("rise_percent", 0.31)  # 默认5%涨幅
        fall_percent = thresholds.get("fall_percent", 0.31)  # 默认5%跌幅

        change_percent = ((price - base_price) / base_price) * 100
        logger.info(
            f"当前金价: {price}元, 基准价(早盘价): {base_price}元, 涨跌: {change_percent:.2f}%"
        )

        notify_config = config["notify"]

        if notify_config.get("enabled", True):
            # 涨幅超过阈值
            if change_percent >= rise_percent and not state["notified_high"]:
                send_notification(
                    notify_config["pushplus_token"],
                    f"金价上涨 {change_percent:.2f}%（警报）",
                    f"当前金价(晚盘) {price} 元\n基准价(早盘) {base_price} 元\n上涨 {change_percent:.2f}%\n超过阈值 {rise_percent}%\n时间: {datetime.now()}",
                )
                state["notified_high"] = True

            # 跌幅超过阈值
            elif change_percent <= -fall_percent and not state["notified_low"]:
                send_notification(
                    notify_config["pushplus_token"],
                    f"金价下跌 {change_percent:.2f}%（警报）",
                    f"当前金价(晚盘) {price} 元\n基准价(早盘) {base_price} 元\n下跌 {abs(change_percent):.2f}%\n超过阈值 {fall_percent}%\n时间: {datetime.now()}",
                )
                state["notified_low"] = True

            # 回到正常区间，重置状态
            if -fall_percent < change_percent < rise_percent:
                state["notified_high"] = False
                state["notified_low"] = False

        state["last_price"] = price
        save_state(state)

    except Exception as e:
        logger.error(f"监控异常: {e}")


def main():
    config = load_config()
    interval = config.get("check_interval_minutes", 5)

    logger.info(f"金价监控启动，检查间隔 {interval} 分钟，超时 {TIMEOUT_MINUTES} 分钟")

    last_price = None
    last_change_time = time.time()

    monitor()
    schedule.every(interval).minutes.do(monitor)
    schedule.every(PERIODIC_REPORT_MINUTES).minutes.do(periodic_report)

    while True:
        schedule.run_pending()

        current_price = None
        try:
            _, base_price = get_gold_price(config)
            current_price = base_price
        except:
            pass

        if current_price is not None and last_price is not None:
            if current_price != last_price:
                last_price = current_price
                last_change_time = time.time()
                logger.info(f"价格已更新: {current_price}")
        elif current_price is not None:
            last_price = current_price
            last_change_time = time.time()

        time.sleep(30)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        logger.info("监控已停止")
        sys.exit(0)
