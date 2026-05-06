# 金价监控通知系统

监控金价并在达到阈值时通过微信推送通知。

## 配置

1. 编辑 `config.json`:
   - `thresholds.high_price`: 高价阈值（金价 >= 此值时通知）
   - `thresholds.low_price`: 低价阈值（金价 <= 此值时通知）
   - `notify.sc_key`: 在 [Server酱](https://scf.serc.live) 注册获取的 SC_KEY
   - `check_interval_minutes`: 检查间隔（分钟）

2. 安装依赖:
   ```bash
   pip install -r requirements.txt
   ```

## 运行

```bash
python monitor.py
```

## Linux 服务器部署

1. 创建日志目录:
   ```bash
   sudo mkdir -p /var/log
   sudo touch /var/log/gold_monitor.log
   sudo chmod 666 /var/log/gold_monitor.log
   ```

2. 复制服务文件并注册:
   ```bash
   sudo cp gold-monitor.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable gold-monitor
   sudo systemctl start gold-monitor
   ```

3. 检查状态:
   ```bash
   sudo systemctl status gold-monitor
   ```

4. 查看日志:
   ```bash
   tail -f /var/log/gold_monitor.log
   ```

## 重置通知状态

当金价回到正常区间后，程序会自动重置通知状态。如需手动重置，删除 `.monitor_state.json` 文件。