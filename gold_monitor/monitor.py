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
    # AU0 实时行情（沪金期货主力，元/克）
    realtime_price = None
    try:
        df = ak.futures_zh_minute_sina(symbol="AU0", period="1")
        latest = df.iloc[-1]
        realtime_price = float(latest["close"])
        logger.info(f"实时金价(AU0): {realtime_price} 元/克")
    except Exception as e:
        logger.warning(f"AU0 实时行情获取失败: {e}")

    # USD/CNY 汇率（frankfurter 免费接口）
    usd_cny = None
    try:
        r = requests.get("https://api.frankfurter.app/latest?from=USD&to=CNY", timeout=5)
        usd_cny = r.json()["rates"]["CNY"]
        logger.info(f"USD/CNY 汇率: {usd_cny}")
    except Exception as e:
        logger.warning(f"汇率获取失败: {e}")

    # 国际金价（美元/盎司）
    international_price = None
    if realtime_price and usd_cny:
        # 1 金衡盎司 = 31.1035 克
        international_price = (realtime_price / usd_cny) * 31.1035
        logger.info(f"国际金价: {international_price:.2f} 美元/盎司")

    # 备用基准价
    df_bench = ak.spot_golden_benchmark_sge()
    bench_latest = df_bench.iloc[-1]
    current_price = float(bench_latest["晚盘价"])
    base_price = float(bench_latest["早盘价"])
    logger.info(
        f"基准价: 时间={bench_latest['交易时间']}, 晚盘价={bench_latest['晚盘价']}, 早盘价={bench_latest['早盘价']}"
    )

    price_to_use = realtime_price if realtime_price is not None else current_price
    return price_to_use, base_price, international_price


def send_notification(token, title, content, msg_type="text", topic=None):
    url = "http://www.pushplus.plus/send"
    data = {
        "token": token,
        "title": title,
        "content": content,
        "type": msg_type,
    }
    if topic:
        data["topic"] = topic
    try:
        response = requests.post(url, json=data, timeout=10)
        result = response.json()
        if result.get("code") == 200:
            logger.info(f"通知发送成功: {title} -> topic={topic}")
            return True
        else:
            logger.error(f"通知发送失败: {result}")
            return False
    except Exception as e:
        logger.error(f"通知请求异常: {e}")
        return False


def periodic_report():
    config = load_config()
    notify_config = config["notify"]
    if not notify_config.get("enabled", True):
        return
    token = notify_config.get("pushplus_token")
    topic = notify_config.get("pushplus_topic")
    if not token:
        logger.warning("没有配置 pushplus_token")
        return
    try:
        price, base_price, intl_price = get_gold_price(config)
        change_percent = ((price - base_price) / base_price) * 100
        trend = "↑" if change_percent > 0 else "↓" if change_percent < 0 else "→"
        intl_line = f"\n国际金价: {intl_price:.2f} 美元/盎司" if intl_price else ""
        send_notification(
            token,
            f"金价定时推送 {trend} {abs(change_percent):.2f}%",
            f"当前金价(AU0) {price} 元/克\n基准价(早盘) {base_price} 元/克\n涨跌 {change_percent:+.2f}%{intl_line}\n时间: {datetime.now()}",
            topic=topic,
        )
    except Exception as e:
        logger.error(f"定时推送异常: {e}")


def monitor():
    config = load_config()
    state = load_state()
    last_price_from_state = state.get("last_price")

    try:
        price, base_price, intl_price = get_gold_price(config)
        thresholds = config["thresholds"]
        rise_percent = thresholds.get("rise_percent", 0.1)
        fall_percent = thresholds.get("fall_percent", 0.1)
        rise_absolute = thresholds.get("rise_absolute", 5)
        fall_absolute = thresholds.get("fall_absolute", 5)

        change_percent = ((price - base_price) / base_price) * 100
        logger.info(
            f"当前金价: {price}元, 基准价: {base_price}元, 涨跌: {change_percent:.2f}%"
        )

        notify_config = config["notify"]
        token = notify_config.get("pushplus_token")
        topic = notify_config.get("pushplus_topic")
        intl_line = f"国际金价: {intl_price:.2f} 美元/盎司\n" if intl_price else ""

        if notify_config.get("enabled", True) and token:
            if last_price_from_state is not None:
                abs_change = price - last_price_from_state
                if abs_change >= rise_absolute and not state.get("notified_rise_absolute"):
                    send_notification(
                        token,
                        f"金价飙升 +{abs_change:.2f}元/克！",
                        f"{intl_line}当前金价(AU0) {price} 元/克\n上次数值 {last_price_from_state} 元/克\n单次变动 +{abs_change:.2f} 元/克\n超过阈值 {rise_absolute} 元\n时间: {datetime.now()}",
                        topic=topic,
                    )
                    state["notified_rise_absolute"] = True
                elif abs_change <= -fall_absolute and not state.get("notified_fall_absolute"):
                    send_notification(
                        token,
                        f"金价暴跌 {abs_change:.2f}元/克！",
                        f"{intl_line}当前金价(AU0) {price} 元/克\n上次数值 {last_price_from_state} 元/克\n单次变动 {abs_change:.2f} 元/克\n超过阈值 {fall_absolute} 元\n时间: {datetime.now()}",
                        topic=topic,
                    )
                    state["notified_fall_absolute"] = True
                if abs(abs_change) < rise_absolute * 0.5:
                    state.pop("notified_rise_absolute", None)
                    state.pop("notified_fall_absolute", None)

            if change_percent >= rise_percent and not state["notified_high"]:
                send_notification(
                    token,
                    f"金价上涨 {change_percent:.2f}%（警报）",
                    f"{intl_line}当前金价(AU0) {price} 元/克\n基准价(早盘) {base_price} 元/克\n上涨 {change_percent:.2f}%\n超过阈值 {rise_percent}%\n时间: {datetime.now()}",
                    topic=topic,
                )
                state["notified_high"] = True

            elif change_percent <= -fall_percent and not state["notified_low"]:
                send_notification(
                    token,
                    f"金价下跌 {abs(change_percent):.2f}%（警报）",
                    f"{intl_line}当前金价(AU0) {price} 元/克\n基准价(早盘) {base_price} 元/克\n下跌 {abs(change_percent):.2f}%\n超过阈值 {fall_percent}%\n时间: {datetime.now()}",
                    topic=topic,
                )
                state["notified_low"] = True

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
    token = config["notify"].get("pushplus_token")
    topic = config["notify"].get("pushplus_topic")
    logger.info(f"金价监控启动，检查间隔 {interval} 分钟，topic={topic}")

    last_price = None

    monitor()
    schedule.every(interval).minutes.do(monitor)
    schedule.every(PERIODIC_REPORT_MINUTES).minutes.do(periodic_report)

    while True:
        schedule.run_pending()
        time.sleep(30)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        logger.info("监控已停止")
        sys.exit(0)