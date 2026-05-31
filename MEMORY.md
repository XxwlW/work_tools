---
name: work_tools_deployment
description: 工作工具项目代码已克隆，部署方案待确认
type: project
---

## 项目概述
- **仓库**: https://github.com/XxwlW/work_tools
- **内容**: 黄金价格监控工具（gold_monitor）+ 可视化工具（tools）
- **语言**: Python 100%
- **克隆位置**: E:\Wxj\LearnForPnc\Ubuntu_Share\myproject

## gold_monitor 模块
- `monitor.py` - 主脚本，使用 akshare 获取金价，通过 PushPlus 推送通知
- `config.json` - 配置文件
- `gold-monitor.service` - systemd 服务配置
- `requirements.txt` - 依赖：akshare、requests、schedule

## 部署方案（待确认）
- **目标**: 阿里云服务器（Linux，有 SSH 权限）
- **路径**: /media/disk1/glod_project/gold_monitor
- **用户**: wang
- **服务**: systemd gold-monitor.service

## 待确认事项
1. 服务器用户是否为 wang
2. /media/disk1/ 路径是否存在
3. pushplus_token 是否用环境变量