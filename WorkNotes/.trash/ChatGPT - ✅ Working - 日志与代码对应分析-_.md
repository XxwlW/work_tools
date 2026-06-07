# ✅ Working - 日志与代码对应分析->

---
## 📤 导出信息
- **会话标题**: ✅ Working - 日志与代码对应分析->
- **导出时间**: 4/8/2026, 2:50:31 PM
- **来源**: ChatGPT
- **链接**: https://chatgpt.com/g/g-p-6965d56ede448191a6aef0d4d8776f30/c/69d4d6f9-0068-83e8-9111-618412c9af52
---

## 🙋 用户

粘贴的文本 (1).txt
文档
根据来源中的outmap.txt ；逐行解释附件log中的内容与outmap.txt代码的对应

---

## 🤖 ChatGPT

下面按**“日志 → 对应代码变量/逻辑 → 含义”**的方式，一步一步对齐说明。先只做**核心主链路（停车位→边界→终点）**，确保你理解正确，再可以继续细化其它模块。




# 一、核心代码（outmap.txt）在做什么

先抽取关键变量（这是后面所有 log 的“源头”）：



```
C++Obj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];Obj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];SlotLen = APAMap_GInfo.SlotPar.SlotLen;EndPos = APAMap_GInfo.SlotPar.EndPos;OrgAng = APAMap_GInfo.NewCordSysAng;OrgPt  = APAMap_GInfo.NewCordSysOPt;bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;BloundaryOffsetY = 0;bSeizeEndCarPosFlag = FALSE;SafeDis = 250;
```

👉 这段的本质是：



| 变量 | 含义 |
| --- | --- |
| Obj1Pt / Obj2Pt | 车位两侧边界点 |
| SlotLen | 车位长度 |
| EndPos | 规划出的停车终点 |
| OrgPt / OrgAng | 新坐标系 |
| bSlotDataAtRigthSide | 车位在车左还是右 |
| CurCarCoordinateX | 当前车位置 |
| SafeDis | 安全距离 |



# 二、日志逐行对齐（第一阶段：输入与车位）


### 1️⃣ 输入数据


```
==INPUTDATA==TOTALMAP==...carpose: 345.757355,-7923.585851,0.153938
```




👉 对应代码：



```
C++CurCarCoordinateX = CarPos.x * 0.001;
```

👉 含义：




- 
车辆当前位姿（x,y,θ）




- 
后续所有计算的基准







### 2️⃣ 车位点（非常关键）


```
==ParkOutSlotInfoFromTotalMap==SlotPt:0(3254,-3629),1(971,-4045),2(1975,-9466),3(4258,-9050)
```




👉 对应代码（间接来源）：



```
C++APAMap_GInfo.SlotPar.SlotBordPt[]
```

👉 含义：


这是一个矩形车位：



```
  0 -------- 1  |          |  |          |  3 -------- 2
```

后面会选：



```
C++Obj2Pt = SlotBordPt[0]Obj1Pt = SlotBordPt[1]
```

👉 log 中后续验证：



```
ObjPt2(-1947.49,-4560.28)=ObjPt1(1091.15,-3894.80)
```




✔ 已映射成功




# 三、第二阶段：选取当前车位（核心逻辑）


```
==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot==Valid==bResult1(1)
```




👉 对应代码函数（你文件中）：



```
C++APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(...)
```

👉 含义：




- 
从多个候选车位中




- 
选出“当前车辆所属车位”







# 四、第三阶段：几何计算（最重要）

代码关键：



```
C++AlgCom_LineParABCbyTwoPoints(...)   // 建直线AlgCom_LineParABCByPerPendLineAndPointOnLine(...) // 垂线AlgCom_CrossPointOfTwoLines(...)   // 求交点
```






### 对应日志


```
==ObjbyFSDAndOD==ObjPt2(-1947.49,-4560.28)ObjPt1(1091.15,-3894.80)EndPos(0.22,-2.11)SlotLen(3109.67)
```




👉 对应关系：



| log字段 | 代码变量 |
| --- | --- |
| ObjPt2 | Obj2Pt |
| ObjPt1 | Obj1Pt |
| SlotLen | SlotLen |
| EndPos | EndPos |


👉 这里做的事：


**用车位边界 → 计算停车终点**




# 五、第四阶段：偏移与安全处理

代码：



```
C++SafeDis = 250;BloundaryOffsetY = 0;bSeizeEndCarPosFlag = FALSE;
```



### 对应日志


```
FSDOffset(…)ODOffset(…)OffsetX(0,0)OffsetY(0,1.39)
```




👉 含义：



| 项 | 含义 |
| --- | --- |
| FSDOffset | 视觉/融合感知偏移 |
| ODOffset | 障碍物检测偏移 |
| OffsetX/Y | 最终修正 |
| 1.39 | Y方向修正（贴边/安全距离） |



# 六、第五阶段：终点是否被“侵占”

代码：



```
C++bSeizeEndCarPosFlag // FSD侵占终点
```






### 对应日志


```
bSeizeEndCarPosFlag(0)
```




👉 含义：




- 
0 → 终点安全




- 
1 → 被占用，需要重新规划







# 七、第六阶段：边界生成（路径约束）


```
=BSegData===NSegData==
```




👉 对应代码（隐式）：




- 
车位边界




- 
可行驶区域





👉 含义：



| 数据 | 含义 |
| --- | --- |
| BSegData | 边界线 |
| NSegData | 可行驶路径线 |



# 八、第七阶段：最终输出


```
==APAMap=Output==CarPos(...)Obj2_x(...)Obj2_y(...)endpos(-7.208,-1.580)
```




👉 对应代码变量：



```
C++EndPosObj2PtObj1Pt
```



# 九、总结（主链路）

整个 log 实际就是执行了这条链：



```
车位点(SlotPt)    ↓选当前车位    ↓提取 Obj1Pt / Obj2Pt    ↓构建几何线（直线/垂线/交点）    ↓计算 EndPos    ↓加偏移(FSD / OD)    ↓安全检查(bSeizeEndCarPosFlag)    ↓生成边界(BSeg / NSeg)    ↓输出路径
```



# 十、关键结论（非常重要）

你现在可以用一个“调试思维”去看 log：



### 只看这5个字段就够定位问题：


```
SlotPt          ← 车位是否正确ObjPt1/ObjPt2   ← 边界是否正确EndPos          ← 终点是否异常OffsetX/Y       ← 偏移是否异常bSeizeEndCarPosFlag ← 是否被占用
```



# 如果下一步需要

可以继续做更深入的三件事（建议按顺序）：




1. 
**逐函数级别（精确到每一行代码）对齐 log**




2. 
**分析为什么出现 -1e18 / nan（你日志里很多）**




3. 
**结合路径规划算法（A*/RRT）解释边界如何影响轨迹**





只要说“继续第几步”。

---

## 🙋 用户

第1步

---

## 🤖 ChatGPT

好的，进入**第1步：函数级逐行对齐（严格对应 log）**。

本步只聚焦一个函数，确保完全理解，再进入下一个。




# 一、目标函数

选择最关键函数：



```
C++APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(...)
```




👉 因为它直接对应日志：



```
==ParkOutGetSlotInfoFromLocVPLSlotByTotalMap==
```






# 二、函数执行链（先建立骨架）

这个函数内部逻辑可以拆成 **5步**：



```
① 获取输入（车位点、车位置）② 判断车位左右③ 选取Obj1/Obj2点④ 构建三条线（Close / Far / 垂线）⑤ 求交点 → 修正车位几何
```

下面逐行对齐。




# 三、逐行对齐（严格对应 log）



## Step 1：输入数据


```
C++CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
```

👉 对应 log：



```
CarPos(345.757721,-7923.586914,0.153938)parkoutmode(0)side(0)
```




👉 含义：



| 变量 | 值 | 说明 |
| --- | --- | --- |
| CurCarPos | (345,-7923) | 当前车 |
| ParkOutMode | 0 | 平行/垂直出库模式 |
| bSlotDataAtRigthSide | 0 | 车位在左 |



## Step 2：构造车辆矩形


```
C++APAMap_GetCarRectArea(..., CurCarPos, &pRectPt[0], &pRectLine[0]);
```

👉 log没有直接打印，但影响后续：



```
CurCarCoordinateX(0.35)
```




👉 含义：




- 
得到车身四个角点




- 
用于判断与车位关系







## Step 3：选择车位角点索引（关键）


```
C++if (bSlotDataAtRigthSide == TRUE){    Obj2PtIndex = 0;    Obj1PtIndex = 1;}else{    Obj2PtIndex = 3;    Obj1PtIndex = 2;}
```

👉 对应 log：



```
SlotPt:0(971,-4045),1(1975,-9466),2(4258,-9050),3(3254,-3629)
```




👉 实际选点：



```
Obj2Pt = SlotPt[3]Obj1Pt = SlotPt[2]
```

👉 对应后续 log：



```
ObjPt2(-1947.49,-4560.28)ObjPt1(1091.15,-3894.80)
```




✔ 完全对应




## Step 4：构建“近边线”（CloseLine）


```
C++AlgCom_LineParABCbyTwoPoints(    pVPLSlotPts[Obj1PtIndex],    pVPLSlotPts[Obj2PtIndex],    &CloseLine);
```

👉 对应 log（隐式）：



```
SlotBordPt(...)
```




👉 含义：




- 
用 Obj1 → Obj2 构建车位入口边







## Step 5：构建“远边线”（FarLine）


```
C++AlgCom_LineParABCbyTwoPoints(    pVPLSlotPts[Obj1InnerIndex],    pVPLSlotPts[Obj2InnerIndex],    &FarLine);
```

👉 对应 log：



```
SlotLen(3109.67)
```




👉 含义：




- 
车位另一侧边界




- 
用于计算深度







## Step 6：构建垂线（核心几何）


```
C++TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(    pVPLSlotPts[Obj2PtIndex],    &CloseLine);
```

👉 含义：




- 
从 Obj2 点做一条 **垂直于入口边的线**





👉 对应 log（间接）：



```
ObjAng(13.79)OrgAng(102.35)
```




👉 本质：


👉 在算车位方向角（法向）




## Step 7：求交点（最关键）


```
C++AlgCom_CrossPointOfTwoLines(    &TempLine,    &FarLine,    &pVPLSlotPts[Obj2InnerIndex]);
```

👉 对应 log：



```
NearByCarPosSlot1(...)NearByCarPosSlot2(...)
```




👉 含义：




- 
修正车位矩形（防止畸形）




- 
得到标准矩形







# 四、函数输出（核心结果）

这个函数最终输出：



```
C++*pObj2Pt*pObj1Pt*pObj2Ang*pObj1Ang*pObj2Dis*pObj1Dis*pCarOffsetX*pMinSlotDpth*pVPLSlotDpth
```



## 对应 log


```
Obj2Pt(-1517,-4466)Obj1Pt(798,-3959)ObjAng(13.79)Dis(-41.94,124.09)CarOffsetX(-56.45)MinSlotDpth(4919.66)VPLSlotDpth(5557)
```






# 五、这一函数本质（总结一句话）


```
把原始车位点 → 转换成“标准矩形 + 几何参数”
```

输出的是：




- 
边界点（Obj1/Obj2）




- 
朝向（角度）




- 
深度（slot depth）




- 
偏移（CarOffset）







# 六、你现在应该能做到

看到任何一行 log：



```
ObjPt2(...)
```

能立即知道：



```
来源 = SlotPt[i]经过 = 索引选择 + 坐标变换 + 直线求交
```



# 七、下一步建议（按优先级）

如果继续深入，建议：



### 👉 第2步（强烈推荐）

**分析异常值来源：**



```
-1e18 / nan / 32767
```

这些在你 log 里大量出现，是关键问题。



如果继续，请直接说：


👉「第2步」

---

## 🙋 用户

可以结束了 接下来需要帮我解析代码 然后协助完成对代码的熟悉和了解 便于后续工作维护

---

## 🤖 ChatGPT

可以。


接下来可以按**“能维护代码”**这个目标来做，不再只盯日志，而是把这份代码体系拆开讲透。建议按下面顺序推进：



## 后续建议路线


### 1. 先建立代码总框架

把 `outmap.txt` 对应模块拆成：




- 
入口函数




- 
核心数据结构




- 
车位计算




- 
边界融合




- 
终点位计算




- 
失败码与调试日志




- 
输入输出链路





目标是先回答这几个问题：




- 
这段代码从哪里进来




- 
主要改哪些全局变量




- 
一次泊出计算的主流程是什么




- 
哪些函数是真正“算结果”的，哪些只是“修正/融合/调试”







### 2. 再逐函数精读

按维护优先级，建议先读这几类函数：



#### 第一批：主流程函数



- 
`APAMap_ParkingOutTask()`




- 
`APAMap_ParkingOutCalMapSlotPar()`




- 
`APAMap_ParkingOutCalSlotInfo()`




- 
`APAMap_ParkingOutCalMapInfo()`




- 
`APAMap_ParkingOutCheckIfCarPosIsValid()`





这批函数解决“流程怎么走”。



#### 第二批：核心几何函数



- 
`APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`




- 
`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`




- 
`APAMap_ParkingOutGetSlotInfoFromVPLSlotPts()`





这批函数解决“车位和终点怎么计算”。



#### 第三批：融合函数



- 
`APAMap_ParkingOutFusBoundaryByFSDMapInfo()`




- 
`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`




- 
后续的 RefercLine / OD / SDG 融合函数





这批函数解决“边界为什么会变”。




### 3. 建立维护视角

后续我会帮你把每个函数都从这 6 个角度解析：




- 
函数作用




- 
输入来源




- 
输出写到哪里




- 
关键分支




- 
依赖的全局变量




- 
最容易出 bug 的点





这样以后看代码不会只是在“读”，而是在判断：




- 
改这里会影响哪些函数




- 
一个异常 log 应该回溯到哪里




- 
一个 failcause 大概率是谁触发的







### 4. 最后做维护资料

等解析到一定程度，可以再帮你整理成便于长期维护的资料，例如：




- 
模块总览图




- 
函数调用链




- 
全局变量说明表




- 
常见日志对照表




- 
常见异常值来源表




- 
维护时改动注意事项





这个阶段会非常适合后续接手、排障和改需求。




## 建议现在的起点

下一步最合适的是：


**从 `APAMap_ParkingOutTask()` 开始，做整条泊出主流程解析。**


这样先把“树干”看懂，再看每个分支，不容易乱。


直接把需要解析的代码段继续发来，或者就从 `APAMap_ParkingOutTask()` 开始，我按维护视角给你逐段拆。

---

## 🙋 用户

void APAMap_ParkingOutTask() {
  BOOLEAN bResult;

  if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 4) &&
      (APAMap_GInputData.ParkReqPar.Request_cmd == 1) &&
      (APAMap_GInputData.ParkReqPar.request_cnt == APAMap_GInfo.lastreqcnt)) {
    APAMap_ParkingOutUpDataMapInfo();
    return;
  }
  {
    if ((APAMap_GInputData.ParkReqPar.Request_cmd == 1) ||
        (APAMap_GInputData.ParkReqPar.Request_cmd == 6)) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==First APAMapParkout Build cmd(%d)===request_cnt(%lld)==",
                 APAMap_GInputData.ParkReqPar.Request_cmd,
                 APAMap_GInputData.ParkReqPar.request_cnt);
        TLOG_INFO << log_string;
      }
      APAMap_GInfo.calcnt++;
      if ((APAMap_GInputData.ParkReqPar.APAstate <= 3) &&
          (APAMap_GInputData.ParkReqPar.APARunningstate >= 1)) {
        APAMap_ParkingOutDebugInit();
      }
      // APAMap_GInfo.status = APAMapStatus_BUSY;
      bResult = APAMap_ParkingOutCalMapSlotPar();
      if (bResult) {
        bResult = APAMap_ParkingOutCalSlotInfo();
        if (bResult) {
          bResult = APAMap_ParkingOutCalMapInfo();
          if (bResult == FALSE) {
            APAMAP_Setfailcause(45);
          } else {
            bResult = APAMap_ParkingOutCheckIfCarPosIsValid();
          }
        }
      }
      if (bResult == FALSE) {
        APAMap_GInfo.failcalcnt++;
      }
      APAMap_GInfo.bCalResult = bResult;
      // APAMap_GInfo.status = APAMapStatus_CALFINISHED;
    } else {
      if ((APAMap_GInfo.OutLine.LeftBoundary.PtNum < 2) ||
          (APAMap_GInfo.OutLine.RightBoundary.PtNum < 2)) {
        TLOG_DEBUG << "start APAMAP_Setfailcause(59)...";
        APAMap_DataInit();
        APAMap_ParkingOutDebugInit();
        APAMAP_Setfailcause(59);
        return;
      }
      APAMap_ParkingOutUpDataMapInfo();
      APAMap_GInfo.bCalResult = TRUE;
    }
  }
  if (APAMap_GInputData.ParkReqPar.APARunningstate >= 7) {
    APAMap_ParkingOutDebugInit();
  }
  return;
  // APAMap_SetOutputData();
}

---

## 🤖 ChatGPT

下面按**维护视角**来拆这个函数：先讲它在整个模块里的定位，再逐段解释，最后给出后续维护时最该盯的点。




# 1. 这个函数的定位

`APAMap_ParkingOutTask()` 是**泊出地图模块的主入口函数**。


它本质上只做三件事：



```
C++1) 判断这次是“重建地图”还是“只更新地图”2) 如果要重建，就按固定流程依次计算3) 记录结果、失败次数，并在特定阶段清理调试状态
```

可以把它看成一个**调度函数**，而不是一个“几何计算函数”。


也就是说：




- 
它自己不直接算车位




- 
不直接算边界




- 
不直接算终点位





它只是决定：




- 
什么时候调 `CalMapSlotPar`




- 
什么时候调 `CalSlotInfo`




- 
什么时候调 `CalMapInfo`




- 
什么时候只做 `UpDataMapInfo`







# 2. 先给出主流程图

这段代码可以抽成下面这个流程：



```
进入 APAMap_ParkingOutTask    │    ├─ 条件1：是否是“相同请求下的后续更新帧”    │      ├─ 是：只调用 APAMap_ParkingOutUpDataMapInfo()，然后 return    │      └─ 否：继续    │    ├─ 条件2：Request_cmd 是否为 1 或 6    │      ├─ 是：执行“完整建图流程”    │      │      1. 打日志    │      │      2. calcnt++    │      │      3. 必要时 DebugInit    │      │      4. CalMapSlotPar    │      │      5. CalSlotInfo    │      │      6. CalMapInfo    │      │      7. CheckIfCarPosIsValid    │      │      8. 失败则 failcalcnt++    │      │      9. 保存 bCalResult    │      │    │      └─ 否：执行“增量更新流程”    │             1. 检查左右边界点数    │             2. 不足则 failcause=59 并返回    │             3. 否则 UpDataMapInfo()    │             4. bCalResult = TRUE    │    ├─ 条件3：APARunningstate >= 7    │      └─ DebugInit    │    └─ return
```



# 3. 逐段解释



## 第一段：快速更新分支


```
C++if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 4) &&    (APAMap_GInputData.ParkReqPar.Request_cmd == 1) &&    (APAMap_GInputData.ParkReqPar.request_cnt == APAMap_GInfo.lastreqcnt)) {  APAMap_ParkingOutUpDataMapInfo();  return;}
```


### 作用

这是一个**捷径分支**。


满足这三个条件时，不再重新完整计算，而是直接：



```
C++APAMap_ParkingOutUpDataMapInfo();return;
```


### 条件含义


#### 1）`APARunningstate >= 4`

说明流程已经进入较后阶段，不是刚开始。



#### 2）`Request_cmd == 1`

说明当前请求命令是某种“持续规划/更新”类型，而不是一次性重新构建。



#### 3）`request_cnt == lastreqcnt`

说明**请求计数没有变化**，也就是“还是同一次请求”。



### 维护视角理解

这段逻辑的真实意思是：



>
> 如果已经是同一次泊出任务的后续阶段，而且请求没有变化，就不要每帧都重算整张图，只做增量更新。
>
>
>


### 为什么这样设计

完整建图代价更高，而后续帧通常只需要：




- 
跟随车辆移动更新地图




- 
更新局部边界




- 
保持已有结果连续性






### 维护风险

这段是很容易引起“为什么没重新算”的地方。


后续排障时，如果发现：




- 
新输入来了




- 
但没有触发 `CalMapSlotPar / CalSlotInfo / CalMapInfo`





先检查这三个量：




- 
`APARunningstate`




- 
`Request_cmd`




- 
`request_cnt / lastreqcnt`







## 第二段：是否进入完整建图流程


```
C++if ((APAMap_GInputData.ParkReqPar.Request_cmd == 1) ||    (APAMap_GInputData.ParkReqPar.Request_cmd == 6)) {
```


### 作用

只有 `Request_cmd` 是 `1` 或 `6`，才走完整建图流程。



### 维护结论

`cmd=1` 和 `cmd=6` 是这个模块里最关键的两类命令。

它们会触发：




- 
车位参数计算




- 
车位边界计算




- 
地图边界构建




- 
车辆位置合法性检查





如果不是这两个命令，则默认进入下面那个“更新已有地图”的分支。




## 第三段：打印首次建图日志


```
C++char log_string[512];snprintf(log_string, sizeof(log_string),         "==First APAMapParkout Build cmd(%d)===request_cnt(%lld)==",         APAMap_GInputData.ParkReqPar.Request_cmd,         APAMap_GInputData.ParkReqPar.request_cnt);TLOG_INFO << log_string;
```


### 作用

告诉日志系统：现在进入了一次新的泊出地图构建。



### 对维护特别有用

看到这个 log，说明当前帧不是纯更新，而是进入了完整建图链路。


日志示例你之前已经见到过：



```
==First APAMapParkout Build cmd(6)===request_cnt(1)==
```





### 后续排障经验

如果日志里没有这句，通常说明：




- 
没进完整建图分支




- 
或提前走了前面的快速更新分支







## 第四段：总计算次数累加


```
C++APAMap_GInfo.calcnt++;
```


### 作用

记录泊出地图计算次数。



### 维护意义

这是个统计量，不影响核心几何结果，但很适合用来判断：




- 
模块是否频繁重算




- 
某段时间是否重复进入建图




- 
一个问题是否出现在第几次计算后







## 第五段：调试状态初始化


```
C++if ((APAMap_GInputData.ParkReqPar.APAstate <= 3) &&    (APAMap_GInputData.ParkReqPar.APARunningstate >= 1)) {  APAMap_ParkingOutDebugInit();}
```


### 作用

在特定状态下，把一批全局调试/标志位清零。



### 为什么不是每次都清

因为后续某些阶段需要保留上一帧的状态，例如：




- 
是否已经更新过终点位




- 
是否已经锚点转换




- 
是否已经识别某类边界融合结果





所以这里只在**较早阶段**做初始化。



### 这类初始化一般会清哪些内容

你前面已经看到过，`APAMap_ParkingOutDebugInit()` 里会清掉大量全局标志，例如：




- 
`bCntAddFlag`




- 
`bLaneLineUpdateEndCarPosFlag`




- 
`bRefercLineUpdateEndCarPosFlag`




- 
`bAfterNewAnchorPointFlag`




- 
`bCarryOutSlot`




- 
`bLabelAngledFlag`




- 
`bShortSlotLen`




- 
等等 






### 维护风险

如果后续你改了状态机，最容易出问题的就是这里：




- 
清早了：会把本该保留的中间状态冲掉




- 
清晚了：会把上一轮脏状态带进来





所以改 `APAstate / APARunningstate` 逻辑时，一定要反查这里。




## 第六段：第一步计算 `APAMap_ParkingOutCalMapSlotPar()`


```
C++bResult = APAMap_ParkingOutCalMapSlotPar();
```


### 作用

这是完整流程的第一个实质计算函数。



### 它负责什么

从你前面给的代码看，它主要负责：




- 
解析输入请求参数




- 
识别车位左右侧关系




- 
选定当前 `SlotID / SlotIndex / FusSlotIndex`




- 
初始化 `SlotPar` 内的一些成员




- 
检查请求是否有效






### 直白理解

这一层是在回答：



>
> “这次到底要算哪个车位，车位在车哪边，基础参数是否合法？”
>
>
>


### 如果这里失败

后面的：




- 
`CalSlotInfo`




- 
`CalMapInfo`




- 
`CheckIfCarPosIsValid`





都不会执行。




## 第七段：第二步计算 `APAMap_ParkingOutCalSlotInfo()`


```
C++if (bResult) {  bResult = APAMap_ParkingOutCalSlotInfo();
```


### 作用

只有第一步成功，才继续算车位信息。



### 它负责什么

这个函数的职责是：




- 
计算车位边界点




- 
计算 Obj1 / Obj2




- 
计算车位长度、深度




- 
计算终点位的初始几何信息




- 
融合 FSD/OD/传感器偏移修正






### 直白理解

这一层回答的是：



>
> “这个车位长什么样，边界在哪，车位终点大概放哪。”
>
>
>



## 第八段：第三步计算 `APAMap_ParkingOutCalMapInfo()`


```
C++if (bResult) {  bResult = APAMap_ParkingOutCalMapInfo();  if (bResult == FALSE) {    APAMAP_Setfailcause(45);  } else {    bResult = APAMap_ParkingOutCheckIfCarPosIsValid();  }}
```


### 作用

这是完整建图链路中最重的一步。



### 它负责什么

从你前面的代码可见，它会做：




- 
主边界构建




- 
FSD融合




- 
LaneLine融合




- 
RefercLine融合




- 
OD融合




- 
平滑处理




- 
终点位是否被边界侵占检查




- 
必要时修正终点位






### 直白理解

这一层回答的是：



>
> “在车位、道路、障碍物、车道线等综合信息下，最终可用地图边界是什么样。”
>
>
>


### 为什么失败后设置 `failcause(45)`

因为这里是“地图构建”阶段。

如果前面车位算出来了，但地图边界没构出来，就用 45 表示这一层失败。



### 维护建议

以后看到 `failcause = 45`，不要去先看车位输入，先优先查：




- 
`APAMap_ParkingOutCalBoundaryByParkOutInfo()`




- 
FSD/LaneLine/RefercLine/OD 融合链




- 
边界点数量是否合法







## 第九段：车辆位置合法性检查


```
C++bResult = APAMap_ParkingOutCheckIfCarPosIsValid();
```


### 作用

在车位、边界、终点都算出来以后，最后做一次**车辆当前位置是否合法**的检查。



### 为什么放最后

因为它依赖前面建立好的：




- 
新坐标系




- 
车位边界




- 
车辆姿态变换结果






### 它典型检查什么

根据你给过的代码，它会检查：




- 
车身角度是否过大




- 
车角点是否超出允许边界




- 
是否明显不在合理泊出位置






### 维护结论

前面步骤成功，不代表最终一定成功。

这一层相当于是最终的“合理性门禁”。




## 第十段：失败计数


```
C++if (bResult == FALSE) {  APAMap_GInfo.failcalcnt++;}
```


### 作用

记录失败次数。



### 维护意义

用于统计，不直接控制逻辑，但对排障很有用：




- 
是否持续失败




- 
某版本改动后失败率是否升高







## 第十一段：保存本次计算结果


```
C++APAMap_GInfo.bCalResult = bResult;
```


### 作用

把本次任务是否成功写到全局结果里。



### 维护意义

后面输出层、状态机或者其它模块，通常会依赖它判断：




- 
本次地图是否可用




- 
是否需要 fallback




- 
是否继续沿用旧地图







# 4. 非 1/6 命令时的分支


```
C++} else {  if ((APAMap_GInfo.OutLine.LeftBoundary.PtNum < 2) ||      (APAMap_GInfo.OutLine.RightBoundary.PtNum < 2)) {    TLOG_DEBUG << "start APAMAP_Setfailcause(59)...";    APAMap_DataInit();    APAMap_ParkingOutDebugInit();    APAMAP_Setfailcause(59);    return;  }  APAMap_ParkingOutUpDataMapInfo();  APAMap_GInfo.bCalResult = TRUE;}
```

这个分支非常重要，容易被忽略。




## 先看判断条件

如果 `Request_cmd` 不是 1 也不是 6，就进这里。



### 这说明什么

说明系统认为：



>
> 当前不是一次“新建地图”的命令，而是基于已有地图继续更新。
>
>
>



## 边界点数检查


```
C++if ((LeftBoundary.PtNum < 2) || (RightBoundary.PtNum < 2))
```


### 为什么要求至少 2 个点

因为边界线至少要两个点才能构成线段。

少于 2 个点，就说明当前已有地图无效，无法更新。



### 失败处理


```
C++APAMap_DataInit();APAMap_ParkingOutDebugInit();APAMAP_Setfailcause(59);return;
```


### 含义



- 
地图清空




- 
调试状态清空




- 
设置失败原因 59




- 
直接返回






### 维护结论

`failcause = 59` 一般表示：



>
> 想走增量更新，但当前没有一张合法的旧地图可供更新。
>
>
>

这通常不是“几何计算失败”，而是“更新前提不存在”。




## 如果边界合法


```
C++APAMap_ParkingOutUpDataMapInfo();APAMap_GInfo.bCalResult = TRUE;
```


### 含义

用已有边界做增量更新，并认为这次更新成功。



### 维护意义

这说明 `UpDataMapInfo()` 在这里被当成一种“轻量刷新”逻辑，而不是完全建图逻辑。




# 5. 最后一段：结束阶段再清理一次调试状态


```
C++if (APAMap_GInputData.ParkReqPar.APARunningstate >= 7) {  APAMap_ParkingOutDebugInit();}
```


### 作用

当运行状态进入较晚阶段，再次做调试状态清理。



### 为什么要再清一次

说明作者认为：




- 
早期阶段要清一次，避免旧状态污染新任务




- 
晚期阶段要再清一次，避免任务结束后状态残留到下一轮






### 维护风险

如果后续要在 late stage 观察某些状态，这里可能把你想保留的信息清掉。


所以以后如果出现：




- 
日志里中间某个 flag 明明置过 1




- 
但到结束输出时又变成 0





要回来查这一句。




# 6. 这个函数依赖的关键全局变量

维护时最先要认识这些：



## 输入侧



- 
`APAMap_GInputData.ParkReqPar.APARunningstate`




- 
`APAMap_GInputData.ParkReqPar.Request_cmd`




- 
`APAMap_GInputData.ParkReqPar.request_cnt`




- 
`APAMap_GInputData.ParkReqPar.APAstate`






## 状态侧



- 
`APAMap_GInfo.lastreqcnt`




- 
`APAMap_GInfo.calcnt`




- 
`APAMap_GInfo.failcalcnt`




- 
`APAMap_GInfo.bCalResult`






## 地图侧



- 
`APAMap_GInfo.OutLine.LeftBoundary.PtNum`




- 
`APAMap_GInfo.OutLine.RightBoundary.PtNum`





这些变量基本决定了这函数的所有分支。




# 7. 这个函数的“维护版一句话总结”

`APAMap_ParkingOutTask()` 不是算几何的，它是**泊出地图模块的总调度器**：




- 
负责决定“重建还是更新”




- 
负责串联“车位参数 → 车位信息 → 地图信息 → 合法性检查”




- 
负责记录成功失败




- 
负责在特定阶段清理调试状态







# 8. 后续维护时最常见的 5 个排查入口


### 1）为什么没进入完整建图？

先看：




- 
`Request_cmd`




- 
`APARunningstate`




- 
`request_cnt == lastreqcnt`






### 2）为什么直接只更新不重算？

看第一段快速返回分支。



### 3）为什么 failcause=45？

优先查 `APAMap_ParkingOutCalMapInfo()` 里面的边界构建与融合。



### 4）为什么 failcause=59？

说明进入了更新分支，但左右边界点数不足，旧地图不可用。



### 5）为什么某些中间标志突然没了？

查两处 `APAMap_ParkingOutDebugInit()`。




# 9. 给后续熟悉代码的建议顺序

既然这个函数已经看懂，下一步最合理的是按它的调用顺序继续：



```
C++APAMap_ParkingOutCalMapSlotPar()→ APAMap_ParkingOutCalSlotInfo()→ APAMap_ParkingOutCalMapInfo()→ APAMap_ParkingOutCheckIfCarPosIsValid()
```

这样最符合维护视角，因为这就是主干调用链。


下一段最建议发 `APAMap_ParkingOutCalMapSlotPar()`。

---

## 🙋 用户

BOOLEAN APAMap_ParkingOutCalMapSlotPar() {
  APA_ENUM_TYPE ParkSide;
  uint16_t_INF SlotID;
  APA_ENUM_TYPE SlotSide;
  // uint8_t_INF ParkMode;
  uint8_t_INF FusionMode;
  uint8_t_INF SlotIndex;
  uint8_t_INF FusSlotIndex;
  uint8_t_INF i;
  BOOLEAN bResult;
  BOOLEAN bSlotDataAtRigthSide;
  uint8_t_INF ParkOutMode;
  ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
  SlotID = APAMap_GInputData.ParkReqPar.Request_SlotId;
  ParkSide = APAMap_GInputData.ParkReqPar.parkside;
  FusionMode = APAMap_GInputData.ParkReqPar.Request_SlotFusionMode;
  FusSlotIndex = MAP_SLOT_INVALID_INDEX;
  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
      if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
        bSlotDataAtRigthSide = TRUE;
      } else {
        bSlotDataAtRigthSide = FALSE;
      }
    } else {
      if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
        bSlotDataAtRigthSide = FALSE;
      } else {
        bSlotDataAtRigthSide = TRUE;
      }
    }
  } else {
    if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
      if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
        bSlotDataAtRigthSide = FALSE;
      } else {
        bSlotDataAtRigthSide = TRUE;
      }
    } else {
      if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
        bSlotDataAtRigthSide = TRUE;
      } else {
        bSlotDataAtRigthSide = FALSE;
      }
    }
  }
  for (i = 0; i < 2; i++) {
    APAMap_GInfo.SlotPar.VplPt[i].x = NO_OBJ_DISTANCE;
    APAMap_GInfo.SlotPar.VplPt[i].y = NO_OBJ_DISTANCE;

    APAMap_GInfo.SlotPar.UsPt[i].x = NO_OBJ_DISTANCE;
    APAMap_GInfo.SlotPar.UsPt[i].y = NO_OBJ_DISTANCE;

    APAMap_GInfo.SlotPar.ODPt[i].x = NO_OBJ_DISTANCE;
    APAMap_GInfo.SlotPar.ODPt[i].y = NO_OBJ_DISTANCE;

    APAMap_GInfo.SlotPar.FSDPt[i].x = NO_OBJ_DISTANCE;
    APAMap_GInfo.SlotPar.FSDPt[i].y = NO_OBJ_DISTANCE;

    APAMap_GInfo.SlotPar.PAPt[i].x = NO_OBJ_DISTANCE;
    APAMap_GInfo.SlotPar.PAPt[i].y = NO_OBJ_DISTANCE;
  }

  APAMap_GInfo.SlotPar.bSlotDataAtRigthSide = bSlotDataAtRigthSide;
  if ((APAMap_GInputData.ParkReqPar.parkmode ==
       APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
      (APAMap_GInputData.ParkReqPar.parkmode ==
       APA_PARKPROC_PARKING_MODE_PARKEXIT)) {
    APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
    APAMap_GInfo.SlotPar.SlotID = SlotID;
    APAMap_GInfo.SlotPar.SlotIndex = 0;
    APAMap_GInfo.SlotPar.FusSlotIndex = 0;
    return TRUE;
  }
  if (SlotID == APA_VPL_SLOT_PROC_INVALID_SLOT_ID) {
    APAMAP_Setfailcause(1);
    return FALSE;
  }
  if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
    SlotSide = 0;
  } else {
    SlotSide = 1;
  }

  bResult = FALSE;
  if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {
    for (SlotIndex = 0; SlotIndex < APA_SLOT_SUPPORT_MAX_SLOT_NUM;
         SlotIndex++) {
      if (APAMap_GInputData.Usslot.USSlot[SlotSide].SlotPar[SlotIndex].SlotID ==
          SlotID) {
        bResult = TRUE;
        break;
      }
    }
  } else {
    for (SlotIndex = 0; SlotIndex < APA_VPL_SLOT_PROC_MAX_VPL_SLOT_NUM;
         SlotIndex++) {
      if (APAMap_GInputData.Vplslot.VPLSlot[SlotSide].Slot[SlotIndex].SlotID ==
          SlotID) {
        bResult = TRUE;
        break;
      }
    }
    if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_VPLSLOT) {
    } else {
      bResult = FALSE;
      for (FusSlotIndex = 0;
           FusSlotIndex <
           APAMap_GInputData.FusSlot.FusionSlot[SlotSide].SlotNum;
           FusSlotIndex++) {
        if ((APAMap_GInputData.FusSlot.FusionSlot[SlotSide]
                 .Slot[FusSlotIndex]
                 .FusedByVPLSlotID == SlotID) &&
            (APAMap_GInputData.FusSlot.FusionSlot[SlotSide]
                 .Slot[FusSlotIndex]
                 .FusionMode == FusionMode)) {
          bResult = TRUE;
          break;
        }
      }
    }
  }
  if (bResult == FALSE) {
    APAMAP_Setfailcause(2);
    return FALSE;
  }
  if (APAMap_GInputData.ParkReqPar.APARunningstate == 0) {
    APAMAP_Resetlastreqcnt();
  }
  APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
  APAMap_GInfo.SlotPar.SlotID = SlotID;
  APAMap_GInfo.SlotPar.SlotSide = SlotSide;
  APAMap_GInfo.SlotPar.SlotIndex = SlotIndex;
  APAMap_GInfo.SlotPar.FusSlotIndex = FusSlotIndex;
  return TRUE;
}

---

## 🤖 ChatGPT

下面继续按**维护视角**来拆 `APAMap_ParkingOutCalMapSlotPar()`。


这个函数是主流程里的**第一个实质计算函数**，它不做复杂几何，核心职责只有一句话：



>
> **确定“这次泊出到底针对哪个车位”，并把后续计算要用的车位基础索引和侧别信息写入 `APAMap_GInfo.SlotPar`。**
>
>
>



# 1. 先说这个函数在主链路中的位置

在 `APAMap_ParkingOutTask()` 里，它是第一步：



```
C++bResult = APAMap_ParkingOutCalMapSlotPar();
```

它成功之后，后面函数才有意义：




- 
`APAMap_ParkingOutCalSlotInfo()`：算车位边界、Obj1/Obj2、终点位




- 
`APAMap_ParkingOutCalMapInfo()`：算地图边界、融合 FSD/OD/LaneLine




- 
`APAMap_ParkingOutCheckIfCarPosIsValid()`：做最终合法性检查





所以这个函数本质是在回答 4 个问题：



```
1) 当前是哪个泊出模式2) 车位在车哪一侧3) 请求的 SlotID 在输入数据里能不能找到4) 找到后，索引是多少
```



# 2. 函数主逻辑总览

这段代码可以拆成 7 步：



```
① 读取请求参数② 计算 bSlotDataAtRigthSide③ 清空 SlotPar 里的点位缓存④ 特殊模式快速返回（PARKING_OUT / PARKEXIT）⑤ 校验 SlotID 是否合法⑥ 按 FusionMode 在 US/VPL/FusionSlot 中查找 SlotID⑦ 成功后写回全局状态
```



# 3. 逐段解释



## Step 1：读取输入参数


```
C++APA_ENUM_TYPE ParkSide;uint16_t_INF SlotID;APA_ENUM_TYPE SlotSide;uint8_t_INF FusionMode;uint8_t_INF SlotIndex;uint8_t_INF FusSlotIndex;BOOLEAN bSlotDataAtRigthSide;uint8_t_INF ParkOutMode;ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;SlotID = APAMap_GInputData.ParkReqPar.Request_SlotId;ParkSide = APAMap_GInputData.ParkReqPar.parkside;FusionMode = APAMap_GInputData.ParkReqPar.Request_SlotFusionMode;FusSlotIndex = MAP_SLOT_INVALID_INDEX;
```


### 作用

从请求里取出后续需要的基础控制量。



### 各变量意义


#### `ParkOutMode`

泊出模式。比如：




- 
平行泊出




- 
头出




- 
倒出




- 
转出





它会影响后面“左右含义”的解释。



#### `SlotID`

用户/上游指定的目标车位 ID。

这是后面查找的主键。



#### `ParkSide`

车停在左侧还是右侧。

这和“车位数据是否在右侧”不是一回事，后面会转成另一个布尔量。



#### `FusionMode`

请求使用哪一种车位源：




- 
USSlot




- 
VPLSlot




- 
FusionSlot 中的某一种模式






#### `FusSlotIndex`

融合车位索引，先初始化为无效值。




# 4. Step 2：计算 `bSlotDataAtRigthSide`

这是本函数最容易读乱的一段，也是维护时最值得单独记住的一段。



```
C++if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {  if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {    if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {      bSlotDataAtRigthSide = TRUE;    } else {      bSlotDataAtRigthSide = FALSE;    }  } else {    if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {      bSlotDataAtRigthSide = FALSE;    } else {      bSlotDataAtRigthSide = TRUE;    }  }} else {  if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {    if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {      bSlotDataAtRigthSide = FALSE;    } else {      bSlotDataAtRigthSide = TRUE;    }  } else {    if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {      bSlotDataAtRigthSide = TRUE;    } else {      bSlotDataAtRigthSide = FALSE;    }  }}
```



## 4.1 这段到底在做什么

它在计算一个后续几何模块特别关键的布尔量：



```
C++APAMap_GInfo.SlotPar.bSlotDataAtRigthSide
```

这个量不是简单的“停车在左还是右”，而是：



>
> **在当前坐标约定下，车位数据是否应按“位于右侧”来解释。**
>
>
>

也就是说，它是一个**几何解释标志**。




## 4.2 为什么会这么绕

因为这里同时受 3 个因素影响：




- 
`ParkOutMode`




- 
`ParkSide`




- 
`SlotDataIsNotMirrored`





其中最麻烦的是：



### `SlotDataIsNotMirrored`

它表示输入车位数据有没有被镜像过。


如果数据没镜像，左右关系按一种方式解释。

如果数据镜像了，左右关系就要反过来。



### `ParkOutMode == PARALLEL`

平行泊出时，车位的坐标语义和垂直/斜列泊出不一样，所以这里单独分支处理。




## 4.3 维护时怎么理解最稳

不要背 if-else，记住这个结论：



### 结论

`bSlotDataAtRigthSide` 是后续几何计算统一使用的“右侧语义开关”。


它的目的就是把：




- 
平行/非平行




- 
左停车/右停车




- 
镜像/非镜像





这三类差异，统一折叠成一个布尔量。


后面几乎所有关键几何函数都依赖它，例如：




- 
选 Obj1/Obj2 点




- 
坐标翻转




- 
偏移方向




- 
终点位方向




- 
主/子边界左右归属





所以以后如果看到：




- 
Obj1/Obj2 选反了




- 
边界跑到另一侧




- 
终点位左右颠倒





第一检查点就是这里。




# 5. Step 3：清空 SlotPar 中的点位缓存


```
C++for (i = 0; i < 2; i++) {  APAMap_GInfo.SlotPar.VplPt[i].x = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.VplPt[i].y = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.UsPt[i].x = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.UsPt[i].y = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.ODPt[i].x = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.ODPt[i].y = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.FSDPt[i].x = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.FSDPt[i].y = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.PAPt[i].x = NO_OBJ_DISTANCE;  APAMap_GInfo.SlotPar.PAPt[i].y = NO_OBJ_DISTANCE;}
```


### 作用

把各种来源的点缓存清成无效值。



### 涉及的点类型



- 
`VplPt`：视觉车位点




- 
`UsPt`：超声车位点




- 
`ODPt`：障碍物相关点




- 
`FSDPt`：FSD 相关点




- 
`PAPt`：PA 传感器点






### 为什么这里要清

因为这个函数是新一轮车位参数准备阶段，之前保留的点位不一定还对应当前车位。


如果不清，后面函数很容易误把旧车位的点当成当前车位的数据。



### 维护意义

这是标准的“防脏数据污染”代码。

以后如果你新增一种点源，也应该考虑在这里初始化。




# 6. Step 4：写入 `bSlotDataAtRigthSide`


```
C++APAMap_GInfo.SlotPar.bSlotDataAtRigthSide = bSlotDataAtRigthSide;
```


### 作用

把刚刚统一出来的“右侧语义开关”写入全局状态。



### 后续谁会用

几乎所有关键几何函数都会用它，包括：




- 
车位点索引选择




- 
坐标镜像/翻转




- 
边界线左右归属




- 
终点位修正方向






### 维护结论

这一句虽然简单，但它是后续大半个泊出几何链的基础输入。




# 7. Step 5：特殊模式快速返回


```
C++if ((APAMap_GInputData.ParkReqPar.parkmode ==     APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||    (APAMap_GInputData.ParkReqPar.parkmode ==     APA_PARKPROC_PARKING_MODE_PARKEXIT)) {  APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;  APAMap_GInfo.SlotPar.SlotID = SlotID;  APAMap_GInfo.SlotPar.SlotIndex = 0;  APAMap_GInfo.SlotPar.FusSlotIndex = 0;  return TRUE;}
```


### 作用

对于两种特定 `parkmode`，直接不做后续 SlotID 查找，快速返回成功。



### 说明了什么

说明在这两种模式下：




- 
这个函数不需要真的去 US/VPL/Fusion 数据结构里找车位




- 
后续逻辑可能主要依赖当前位置和简化车位信息，而不是强依赖完整车位索引






### 写回内容

这里只写了：




- 
`CarPos`




- 
`SlotID`




- 
`SlotIndex = 0`




- 
`FusSlotIndex = 0`






### 维护风险

这是一段很典型的“特殊模式绕过常规查找流程”的代码。


以后如果发现：




- 
某些模式下 `SlotIndex` 总是 0




- 
没有进入 US/VPL/Fusion 查找逻辑





先看是不是走了这个分支。




# 8. Step 6：校验 SlotID 是否非法


```
C++if (SlotID == APA_VPL_SLOT_PROC_INVALID_SLOT_ID) {  APAMAP_Setfailcause(1);  return FALSE;}
```


### 作用

如果请求里带的车位 ID 本身就是无效值，立即失败。



### 失败原因



- 
`failcause = 1`






### 维护结论

`failcause 1` 代表的是：



>
> 输入请求本身无效，连查找都不用查。
>
>
>

所以看到 failcause=1，不要去查复杂几何，先回头看上游请求。




# 9. Step 7：把 `ParkSide` 转成 `SlotSide`


```
C++if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {  SlotSide = 0;} else {  SlotSide = 1;}
```


### 作用

把“停车在左/右”的枚举，转成内部数组索引。



### 说明

后面的输入数据通常是按左右槽分开的，例如：




- 
`USSlot[SlotSide]`




- 
`VPLSlot[SlotSide]`




- 
`FusionSlot[SlotSide]`





这里做的是一个索引映射。



### 维护建议

这里的 `0/1` 含义要记牢。

很多后续数组访问都依赖它。


如果以后发现：




- 
明明右侧车位却查到左侧数组




- 
SlotID 找不到但日志显示其实存在





先查这里和上游 `ParkSide` 是否一致。




# 10. Step 8：按 FusionMode 查找车位

这是整个函数的核心主体。




## 10.1 先初始化查找结果


```
C++bResult = FALSE;
```

默认没找到。




## 10.2 如果请求的是 USSlot


```
C++if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {  for (SlotIndex = 0; SlotIndex < APA_SLOT_SUPPORT_MAX_SLOT_NUM;       SlotIndex++) {    if (APAMap_GInputData.Usslot.USSlot[SlotSide].SlotPar[SlotIndex].SlotID ==        SlotID) {      bResult = TRUE;      break;    }  }}
```


### 作用

在超声车位数组里查指定 `SlotID`。



### 结果

找到后：




- 
`bResult = TRUE`




- 
`SlotIndex` 保留找到的位置






### 维护结论

如果 `FusionMode=USSLOT` 时查找失败：




- 
先看 `SlotSide` 是否错




- 
再看 `SlotID` 是否真在 `USSlot` 里




- 
再看数组最大长度是否足够







## 10.3 否则先在 VPLSlot 里查


```
C++else {  for (SlotIndex = 0; SlotIndex < APA_VPL_SLOT_PROC_MAX_VPL_SLOT_NUM;       SlotIndex++) {    if (APAMap_GInputData.Vplslot.VPLSlot[SlotSide].Slot[SlotIndex].SlotID ==        SlotID) {      bResult = TRUE;      break;    }  }
```


### 作用

不是 USSlot 的话，先默认去视觉车位里找。



### 说明

这说明视觉车位是很多融合模式的基础主键来源。




## 10.4 如果 FusionMode 恰好就是 VPLSLOT


```
C++if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_VPLSLOT) {}
```


### 作用

这里什么都没做。



### 为什么留空

因为前面已经在 `VPLSlot` 里查过了。

如果模式就是 `VPLSLOT`，那么找到就够了，不需要再查融合槽。



### 维护角度

这类空分支常见于“占位式结构”。

不要误删，它表达的是一种逻辑分流。




## 10.5 如果不是 VPLSLOT，则去 FusionSlot 再查


```
C++else {  bResult = FALSE;  for (FusSlotIndex = 0;       FusSlotIndex <       APAMap_GInputData.FusSlot.FusionSlot[SlotSide].SlotNum;       FusSlotIndex++) {    if ((APAMap_GInputData.FusSlot.FusionSlot[SlotSide]             .Slot[FusSlotIndex]             .FusedByVPLSlotID == SlotID) &&        (APAMap_GInputData.FusSlot.FusionSlot[SlotSide]             .Slot[FusSlotIndex]             .FusionMode == FusionMode)) {      bResult = TRUE;      break;    }  }}
```


### 作用

在融合车位里查找：




- 
该融合槽是由当前 `SlotID` 的 VPL 槽融合而来




- 
且融合模式与请求一致






### 这里很关键

查的不是：



```
C++FusionSlot.Slot[FusSlotIndex].SlotID == SlotID
```

而是：



```
C++FusedByVPLSlotID == SlotID
```


### 这说明什么

说明融合槽的主索引体系并不完全等于原始 `SlotID`，而是通过 “由哪个 VPL 车位融合而来” 建立关联。



### 维护风险

如果后续融合模块换了 ID 规则，这里极易失配。


看到 `failcause=2` 且怀疑是融合问题时，重点检查：




- 
`FusedByVPLSlotID`




- 
`FusionMode`




- 
`SlotSide`




- 
`FusionSlot[SlotSide].SlotNum`







# 11. Step 9：查找失败处理


```
C++if (bResult == FALSE) {  APAMAP_Setfailcause(2);  return FALSE;}
```


### 含义

车位 ID 不是非法值，但在指定数据源里没找到。



### 与 failcause=1 的区别



- 
`1`：输入 SlotID 本身就是无效值




- 
`2`：输入 SlotID 看起来合法，但在目标数据集中找不到






### 维护结论

`failcause=2` 是典型的“请求和数据不同步”问题。


常见原因：




- 
上游传错 SlotID




- 
SlotSide 算错




- 
FusionMode 不匹配




- 
对应数组尚未刷新到最新数据







# 12. Step 10：运行状态为 0 时重置 lastreqcnt


```
C++if (APAMap_GInputData.ParkReqPar.APARunningstate == 0) {  APAMAP_Resetlastreqcnt();}
```


### 作用

在运行起始态时，重置“上一次请求计数”。



### 为什么要在这里做

因为上层 `APAMap_ParkingOutTask()` 有一个快速更新分支依赖：



```
C++request_cnt == lastreqcnt
```

如果新任务开始时不清掉旧的 `lastreqcnt`，后面可能会误判成“还是上一次请求”，从而跳进快速更新分支，导致不重建地图。



### 维护结论

这是和主入口函数联动的一句关键代码。

如果你以后改状态机，千万别忽略它。




# 13. Step 11：把结果写回全局


```
C++APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;APAMap_GInfo.SlotPar.SlotID = SlotID;APAMap_GInfo.SlotPar.SlotSide = SlotSide;APAMap_GInfo.SlotPar.SlotIndex = SlotIndex;APAMap_GInfo.SlotPar.FusSlotIndex = FusSlotIndex;return TRUE;
```


### 作用

把后续几何计算需要的基础索引和状态写回去。



### 这些量后面怎么用


#### `CarPos`

后续几何函数都要用当前车辆位姿。



#### `SlotID`

用于标识当前处理的是哪个车位。



#### `SlotSide`

后续继续访问左右分离数组时要用。



#### `SlotIndex`

US/VPL 车位数组中的索引。

后续取具体车位点时会用到。



#### `FusSlotIndex`

融合车位索引。

如果后面要从 FusionSlot 里拿更丰富信息，就靠它。




# 14. 这个函数的“维护版一句话总结”

`APAMap_ParkingOutCalMapSlotPar()` 的本质不是“算车位几何”，而是：



>
> **根据请求参数，确定当前目标车位在什么侧、属于哪个数据源、在数组里的哪个索引，并把这些基础信息写入 `APAMap_GInfo.SlotPar`。**
>
>
>



# 15. 这个函数最重要的维护关注点



## 关注点 1：`bSlotDataAtRigthSide`

这是最关键的几何语义开关。


一旦它错，后面会出现：




- 
Obj1/Obj2 反了




- 
坐标翻转错




- 
主/子边界左右颠倒




- 
终点位方向错







## 关注点 2：`SlotSide`

这是数组访问方向。


一旦错，后面会出现：




- 
查不到 SlotID




- 
查到错误侧车位




- 
明明车位存在却 failcause=2







## 关注点 3：`FusionMode`

这决定查哪个源：




- 
USSlot




- 
VPLSlot




- 
FusionSlot





很多“找不到车位”的问题，本质不是车位不存在，而是模式不匹配。




## 关注点 4：特殊模式快速返回

`PARKING_OUT / PARKEXIT` 会绕过常规查找。


如果你调试时发现：




- 
`SlotIndex` 总是 0




- 
没进查找循环





优先看是不是走了这个分支。




## 关注点 5：`APARunningstate == 0` 时重置 `lastreqcnt`

这是和主入口更新/重建逻辑联动的关键点。




# 16. 后续排障时的快速判断表


| 现象 | 优先检查 |
| --- | --- |
| failcause=1 | Request_SlotId 是否为非法值 |
| failcause=2 | SlotSide、FusionMode、SlotID 是否与输入数组匹配 |
| 车位左右反了 | bSlotDataAtRigthSide 计算是否正确 |
| 后面几何全错但没报错 | SlotIndex / FusSlotIndex 是否指到了错误槽 |
| 新任务没重建 | APARunningstate==0 时是否正确 reset lastreqcnt |



# 17. 建议下一步

按主链路继续，最合理的是看：



```
C++BOOLEAN APAMap_ParkingOutCalSlotInfo()
```

因为它就是把这里准备好的：




- 
`SlotID`




- 
`SlotSide`




- 
`SlotIndex`




- 
`FusSlotIndex`




- 
`bSlotDataAtRigthSide`





真正变成：




- 
Obj1 / Obj2




- 
SlotLen / SlotDepth




- 
EndPos





这一步开始进入真正的几何核心。

---

## 🙋 用户

BOOLEAN APAMap_ParkingOutCalSlotInfo() {
  BOOLEAN bResult;
#ifdef APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO
  bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();
#else
  bResult = FALSE;
#endif
  if (FALSE == bResult) {
    bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();
  }
#if 1
  if (bResult) {
    bResult = APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
    debug1++;
  }
#endif
  return bResult;
}

---

## 🤖 ChatGPT

这个函数很短，但它在主流程里非常关键。

它的作用不是“完成一种固定算法”，而是：



>
> **按优先级选择车位边界的来源，并在已有边界基础上继续用 FSD/OD 做二次修正。**
>
>
>

可以把它理解成一个**车位边界求解调度器**。




# 1. 先看它在主流程里的位置

在 `APAMap_ParkingOutTask()` 里：



```
C++bResult = APAMap_ParkingOutCalMapSlotPar();if (bResult) {  bResult = APAMap_ParkingOutCalSlotInfo();  if (bResult) {    bResult = APAMap_ParkingOutCalMapInfo();    ...  }}
```

所以它处于：



```
车位索引确定    ↓车位边界/终点位初算   ← 当前函数    ↓地图边界构建与融合
```

也就是说，`APAMap_ParkingOutCalSlotInfo()` 的职责是先把“车位自身几何”算出来，给后面的地图边界构建打基础。




# 2. 这个函数只做三步

代码如下：



```
C++BOOLEAN APAMap_ParkingOutCalSlotInfo() {  BOOLEAN bResult;#ifdef APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO  bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();#else  bResult = FALSE;#endif  if (FALSE == bResult) {    bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();  }#if 1  if (bResult) {    bResult = APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();    debug1++;  }#endif  return bResult;}
```

它可以拆成：



```
① 优先尝试：从 TotalMap / VPL 车位信息直接构造车位边界② 如果失败：退回到基于泊出模式的默认车位边界构造③ 如果前面成功：再用 FSD/OD 对车位边界做修正
```



# 3. 第一步：优先走 `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`


```
C++#ifdef APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO  bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();#else  bResult = FALSE;#endif
```


## 作用

如果编译宏打开，就优先用**TotalMap / VPLSlot 实际车位点**来构造车位边界。



## 为什么这是优先路径

因为这种路径用的是“真实检测到的车位点”，通常比默认模板更准确。


它一般会得到：




- 
更真实的 `Obj1Pt / Obj2Pt`




- 
更真实的 `SlotLen / SlotDepth`




- 
更真实的 `Obj1Ang / Obj2Ang / OrgAng`





你前面的日志已经对应过这一类结果，例如：



```
==ParkOutGetSlotInfoFromLocVPLSlotByTotalMap===SlotPt:0(-1517.00,-4466.00),1(-192.00,-9864.00),2(2123.23,-9357.95),3(798.00,-3959.00),Obj2Pt(-1517.00,-4466.00),Obj1Pt(798.00,-3959.00),ObjAng(13.79),OrgAng(102.35),Dis(-41.94,124.09),CarOffsetX(-56.45),MinSlotDpth(4919.66),VPLSlotDpth(5557.47)
```




这类 log 本质就属于这一条路径的产物。



## 维护结论

这一步代表“高置信度车位边界”。


如果它成功，说明：




- 
当前帧能从地图/视觉车位数据中拿到足够可靠的车位几何信息





如果它失败，不一定说明模块整体失败，只说明：




- 
**真实车位点路径不可用**




- 
要退回默认几何推断







# 4. 第二步：失败时回退到 `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`


```
C++if (FALSE == bResult) {  bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();}
```


## 作用

如果第一种“真实车位点法”失败，就退回到一种更保守、更模板化的方法。



## 这条路径的本质

它不是直接依赖完整车位点，而是依赖：




- 
当前车姿




- 
泊出模式




- 
车宽车长等车辆参数




- 
左右侧关系




- 
默认安全边界





来“构造一个可用车位”。



## 直白理解

可以理解成：



>
> “如果当前拿不到高质量车位框，那就按照车辆和泊出模式，先估一个合理车位出来，保证流程不断。”
>
>
>


## 你前面贴过这个函数的日志：

例如：



```
==FirstBuildMapObjAndEndCarPos==Obj2Pt(-1948.84,-4560.58,0.24)Obj1Pt(1091.15,-3894.80,0.24)NewCordSysOPt(-1948.84,-4560.58,1.79)SlotBordPt[0](-1948.84,-4560.58)SlotBordPt[1](1091.15,-3894.80)EndPos(-7208.11,-1580.73,1.79)SlotLen(3106)SlotDepth(5219)
```




这类结果就更接近这种“默认构造法”。



## 维护结论

这个分支是**容错分支**，非常重要。


后续维护时不要简单认为：




- 
第一条路径失败就是坏事





因为在工程上，第二条路径往往就是为了兜底。




# 5. 第三步：成功后统一用 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 修正


```
C++if (bResult) {  bResult = APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();  debug1++;}
```

这一步最关键。



## 它不是重新建车位

而是在前两步已经得到的车位边界基础上，再结合：




- 
FSD




- 
OD




- 
传感器偏移




- 
安全距离




- 
终点位侵占情况





去做二次修正。



## 直白理解

前两步解决的是：



>
> “车位大概在哪，边界点大概在哪。”
>
>
>

这一步解决的是：



>
> “这个边界还能不能更贴近真实环境，并且终点位是否安全。”
>
>
>


## 典型修正内容

从你之前给的代码和日志看，这一步会处理：




- 
`FSDOffset`




- 
`ODOffset`




- 
`SensorOffset`




- 
`OffsetX / OffsetY`




- 
`bSeizeEndCarPosFlag`




- 
`SlotLen` 更新




- 
`EndPos` 更新




- 
`SlotBordPt[0/1]` 更新





对应日志非常典型：



```
==FSDOffset(0)(-3998.739990,-33.033875)...==FSDOffset(1)(-1348.176147,1.390734)...==ODOffset(0)(-2800.000000,-1000.000000)...==ODOffset(1)(-2800.000000,-1000.000000)...==app==ObjbyFSDAndOD==ObjPt2(-1947.49,-4560.28)=ObjPt1(1091.15,-3894.80)==EndPos(0.22,-2.11)==SlotLen(3109.67)...
```


## 维护结论

这一步是“车位边界从理论值变成工程可用值”的关键步骤。


前面两步给的是基础轮廓，

这一步给的是**环境修正版**。




# 6. `debug1++` 的意义


```
C++debug1++;
```

这只是一个调试计数器。



## 用途

通常用于确认：




- 
当前帧是否执行到了 FSD/OD 修正阶段




- 
某类路径被调用了多少次






## 维护意义

不直接影响几何结果，但在排障时可以作为辅助统计量。




# 7. 这个函数的输入和输出，维护时要怎么理解



## 输入

它不显式传参，全部依赖全局：



### 来自上一步 `CalMapSlotPar()`



- 
`APAMap_GInfo.SlotPar.SlotID`




- 
`APAMap_GInfo.SlotPar.SlotIndex`




- 
`APAMap_GInfo.SlotPar.FusSlotIndex`




- 
`APAMap_GInfo.SlotPar.bSlotDataAtRigthSide`




- 
`APAMap_GInfo.CarPos`






### 来自输入总线



- 
`VPLSlot / USSlot / FusSlot`




- 
`TotalMapInfo`




- 
`FSDInfo`




- 
`ODInfo`




- 
`CarLocInfo`




- 
`ParkReqPar`







## 输出

这个函数成功后，通常会把这些关键量写入 `APAMap_GInfo.SlotPar`：




- 
`Obj1Pt / Obj2Pt`




- 
`Obj1Ang / Obj2Ang`




- 
`SlotBordPt[0/1/2/3]`




- 
`SlotLen / SlotDepth`




- 
`EndPos`




- 
`EndPosLine`




- 
`NewCordSysOPt / NewCordSysAng`





后面的 `APAMap_ParkingOutCalMapInfo()` 全都依赖这些结果。




# 8. 这个函数的维护版一句话总结

`APAMap_ParkingOutCalSlotInfo()` 的本质是：



>
> **先确定车位边界的初值来源（优先真实车位框，失败则模板构造），再用 FSD/OD 等感知信息把车位边界和终点位修正成工程可用状态。**
>
>
>



# 9. 维护时最该盯的 4 个问题



## 问题 1：为什么没有走真实车位框路径？

看：




- 
编译宏 `APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO` 是否打开




- 
`APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()` 是否返回 `FALSE`







## 问题 2：为什么走了回退模板路径？

说明真实车位框不可用，可能原因有：




- 
车位点不完整




- 
车位点几何关系不合法




- 
目标 `SlotID` 虽找到，但提取车位框失败







## 问题 3：为什么前面成功了，后面还是失败？

重点看第三步：




- 
`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`





因为它会做更严格的安全和环境修正，可能在这里因为终点位/边界/安全距离不满足而失败。




## 问题 4：为什么 Obj1/Obj2 看起来差不多，但结果还是不同？

因为第三步可能改变：




- 
`Obj1Pt / Obj2Pt`




- 
`SlotLen`




- 
`EndPos`




- 
`SlotBordPt`





所以不要只看初值，要看修正后结果。




# 10. 与日志排障的直接对应关系

后续看日志时，可以这么定位：



### 如果看见这类日志


```
==ParkOutGetSlotInfoFromLocVPLSlotByTotalMap===
```

说明更偏向第一条路径。 



### 如果看见这类日志


```
==FirstBuildMapObjAndEndCarPos==
```

说明更偏向第二条路径。 



### 如果看见这类日志


```
==FSDOffset(...)==ODOffset(...)==app==ObjbyFSDAndOD==
```

说明已经进入第三条路径修正。




# 11. 下一步建议

现在最合适继续解析的是这三个函数中的一个，按优先级建议：




1. 
`APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`




2. 
`APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`




3. 
`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`





如果目标是**尽快具备维护能力**，建议先看：


**`APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`**


因为它是最稳的兜底路径，也是最容易建立整体几何理解的函数。

---

## 🙋 用户

BOOLEAN APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo() {
  APA_ENUM_TYPE SlotType;
  uint8_t_INF ParkOutMode;
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
  BOOLEAN bSlotDataAtRigthSide;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType TempPt1, TempPt2, TempPt3;
  APA_DISTANCE_TYPE SlotLength;
  APA_DISTANCE_TYPE SlotDepth;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
  APALineParameterABCType TempLine;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
  APA_ENUM_TYPE ParkSide;
  BOOLEAN bResult;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSubLaneX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotInnerX;
  BOOLEAN bSeizeEndCarPosFlag; // fsd侵占终点位置标志位

  bSeizeEndCarPosFlag = FALSE;
  ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  ParkOutEightMode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
  // zqf-GetParkOutEightMode
  ParkSide = APAMap_GInputData.ParkReqPar.parkside;
  ParkOutEightMode = AlgCom_GetParkOutEightMode(ParkOutMode, ParkSide);
  // zqf-SetParkOutObjPt
  bResult = APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(
      &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &OrgAng, &MaxSubLaneX,
      &MaxSlotInnerX);
  if (FALSE == bResult) {
    {
      char log_string[512];
      snprintf(
          log_string, sizeof(log_string),
          "==Carry out 1.APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()==");
      TLOG_INFO << log_string;
    }
    return FALSE;
  }
  {
    char log_string[512];
    snprintf(
        log_string, sizeof(log_string),
        "==Carry out 2.APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()==\n"
        "==Obj2Pt(%.2f,%.2f)==Obj1Pt(%.2f,%.2f)==Obj2Ang(%.2f)==Obj1Ang(%.2f)=="
        "OrgAng(%.2f)==MaxSubLaneX(%.2f)==MaxSlotInnerX(%.2f)",
        Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, Obj2Ang, Obj1Ang, OrgAng,
        MaxSubLaneX, MaxSlotInnerX);
    TLOG_INFO << log_string;
  }

  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);
  } else {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);
  }
  fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);
  fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);
  SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
  if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - 150.0)) ||
      ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - 150.0))) {
    APAMAP_Setfailcause(100);
    return FALSE;
  }

  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    SlotType = 0;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
             (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
    SlotType = 1;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  } else {
    SlotType = 1;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  }
  OrgPt = Obj2Pt;
  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (bSlotDataAtRigthSide == FALSE) {
      TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
    } else {
      TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.HalfWidthOfCar);
    }
    TempPt1.y =
        (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
    TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
                                                CurCarPos.Coordinate);
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        TempPt2, 0, OrgAng, OrgPt);
    TempPt1.y =
        (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
    TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
                                                CurCarPos.Coordinate);
    TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        TempPt3, 0, OrgAng, OrgPt);

  } else {
    if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
        (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
      TempPt1.y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
    } else {
      TempPt1.y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
    }
    TempPt1.x = APAMap_ComCfg.HalfWidthOfCar;
    TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
                                                CurCarPos.Coordinate);
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        TempPt2, 0, OrgAng, OrgPt);
    TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
    TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
                                                CurCarPos.Coordinate);
    TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        TempPt3, 0, OrgAng, OrgPt);
  }
  if (bSlotDataAtRigthSide == FALSE) {
    TempPt2.x = -TempPt2.x;
    TempPt3.x = -TempPt3.x;
  }
  if (TempPt2.x > TempPt3.x) {
    MaxSlotPtX = TempPt2.x;
  } else {
    MaxSlotPtX = TempPt3.x;
  }
  if (MaxSlotInnerX > MaxSlotPtX) {
    MaxSlotPtX = MaxSlotInnerX;
  }
  APAMap_GInfo.SlotPar.slotCarEndPosXBackUp = MaxSlotPtX;
  APAMap_GInfo.SlotPar.SlotDepth = SlotDepth;
  APAMap_GInfo.SlotPar.SlotLen = SlotLength;
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
        ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
  } else {
    TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
  }
#else
  TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
      ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
#endif
  if (TempPt3.x == 0xff) {
    APAMAP_Setfailcause(101);
    return FALSE;
  }
  APAMap_GInfo.SlotPar.EndPos.Coordinate = TempPt3;
  APAMap_GInfo.SlotPar.EndPosLine =
      AlgCom_LineParABCByCurrentCarPosition(&APAMap_GInfo.SlotPar.EndPos, 0);
  APAMap_GInfo.bDataMirrored = FALSE;
  APAMap_GInfo.bCordSysReSet = FALSE;
  APAMap_GInfo.SlotPar.SlotType = SlotType;
  APAMap_GInfo.SlotPar.bObj2Exist = TRUE;
  APAMap_GInfo.SlotPar.bObj1Exist = TRUE;
  APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
  APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
  APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
  APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;
  APAMap_GInfo.NewCordSysOPt = OrgPt;
  APAMap_GInfo.NewCordSysAng = OrgAng;
  APAMap_GInfo.SlotPar.Obj2Ang = Obj2Ang;
  APAMap_GInfo.SlotPar.Obj1Ang = Obj1Ang;
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FirstBuildMapObjAndEndCarPos==Obj2Pt(%.2f,%.2f,%.2f)==Obj1Pt(%."
             "2f,%.2f,%.2f)==NewCordSysOPt(%.2f,%.2f,%.2f)"
             "==SlotBordPt[0](%.2f,%.2f)====SlotBordPt[1](%.2f,%.2f)==EndPos(%."
             "2f,%.2f,%.2f)==SlotLen(%d)==SlotDepth(%d)==APAstate(%d)=="
             "APARunningstate(%d)",
             APAMap_GInfo.SlotPar.Obj2Pt.x, APAMap_GInfo.SlotPar.Obj2Pt.y,
             APAMap_GInfo.SlotPar.Obj2Ang, APAMap_GInfo.SlotPar.Obj1Pt.x,
             APAMap_GInfo.SlotPar.Obj1Pt.y, APAMap_GInfo.SlotPar.Obj1Ang,
             APAMap_GInfo.NewCordSysOPt.x, APAMap_GInfo.NewCordSysOPt.y,
             APAMap_GInfo.NewCordSysAng, APAMap_GInfo.SlotPar.SlotBordPt[0].x,
             APAMap_GInfo.SlotPar.SlotBordPt[0].y,
             APAMap_GInfo.SlotPar.SlotBordPt[1].x,
             APAMap_GInfo.SlotPar.SlotBordPt[1].y,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.y,
             APAMap_GInfo.SlotPar.EndPos.CarAng, APAMap_GInfo.SlotPar.SlotLen,
             APAMap_GInfo.SlotPar.SlotDepth,
             APAMap_GInputData.ParkReqPar.APAstate,
             APAMap_GInputData.ParkReqPar.APARunningstate);
    TLOG_INFO << log_string;
  }
  return TRUE;
}

---

## 🤖 ChatGPT

下面继续按**维护视角**解析 `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`。


这个函数可以概括成一句话：



>
> **基于 TotalMap / VPL 车位信息，直接生成“当前车位的几何骨架”，并写入后续地图构建所需的核心状态。**
>
>
>

相比前面那个兜底函数 `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`，这个函数更偏向：




- 
用真实车位框




- 
用真实几何方向




- 
用真实车位内边界深度





所以它是**高优先级、精度更高**的车位初始化路径。




# 1. 它在整条链路中的作用

它是 `APAMap_ParkingOutCalSlotInfo()` 的第一优先分支：



```
C++bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();if (FALSE == bResult) {  bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();}
```

所以它的职责是：



```
输入：当前车位索引 + 当前车姿 + 车位侧别输出：Obj1/Obj2、OrgPt/OrgAng、SlotLen/SlotDepth、EndPos
```

只要它成功，后面的 `FSDAndODMapInfo()` 和 `CalMapInfo()` 就会沿用它生成的结果。




# 2. 函数主流程先看骨架

这个函数可以拆成 8 步：



```
① 读取模式、车姿、左右语义② 调用 APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap 取真实车位几何③ 用当前车姿计算车位长度 SlotLength④ 校验车位长度是否过小⑤ 计算车位深度 SlotDepth 和最大可用 X 范围⑥ 计算终点位 EndPos⑦ 把结果写回 APAMap_GInfo / SlotPar⑧ 打日志并返回成功
```



# 3. Step 1：读取基础输入


```
C++bSeizeEndCarPosFlag = FALSE;ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;ParkOutEightMode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;ParkSide = APAMap_GInputData.ParkReqPar.parkside;ParkOutEightMode = AlgCom_GetParkOutEightMode(ParkOutMode, ParkSide);
```


## 作用

把后续几何计算所需的模式和车辆状态准备好。



## 关键变量


### `ParkOutMode`

泊出大模式：




- 
平行泊出




- 
车头直出




- 
车头转出




- 
车尾转出






### `CurCarPos`

当前车辆姿态，是后面算长度、投影和终点位的基准。



### `bSlotDataAtRigthSide`

这是前一个函数已经准备好的“几何右侧语义开关”。



### `ParkOutEightMode`

把“大模式 + 左右侧”进一步细分成更具体的八种子模式。

这在终点位计算时很常见。



### `bSeizeEndCarPosFlag = FALSE`

先假定终点位没有被侵占，后面如果需要再更新。




# 4. Step 2：调用核心函数拿真实车位几何


```
C++bResult = APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(    &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &OrgAng, &MaxSubLaneX,    &MaxSlotInnerX);
```

这是这个函数最核心的一步。



## 作用

从 TotalMap / VPL 车位信息里，直接解析出：




- 
`Obj2Pt`




- 
`Obj1Pt`




- 
`Obj2Ang`




- 
`Obj1Ang`




- 
`OrgAng`




- 
`MaxSubLaneX`




- 
`MaxSlotInnerX`






## 这几个量各自代表什么


### `Obj2Pt / Obj1Pt`

当前车位的两条关键边界点。

后续整个地图主边界、终点位、车位口方向都围绕这两个点。



### `Obj2Ang / Obj1Ang`

对应两条边界的方向角。

后面偏移、平移、构边界线都会用到。



### `OrgAng`

新的局部车位坐标系角度。

非常关键，后面会写到：



```
C++APAMap_GInfo.NewCordSysAng = OrgAng;
```


### `MaxSubLaneX`

对子边界/对向车道的一种几何限制量。



### `MaxSlotInnerX`

车位内部可用最大 X 范围，用来估深度、估安全边界。




## 为什么这一步最关键

因为如果这里成功，后面生成的车位几何就是**基于真实车位框**的，而不是模板估计的。


换句话说，这一步是本函数“高精度”的根源。




## 失败处理


```
C++if (FALSE == bResult) {  ...  return FALSE;}
```


### 含义

只要真实车位几何提取失败，本函数立刻返回失败，让上层去走兜底函数 `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`。



### 维护结论

这个函数本身不负责兜底，它只负责“真实车位框路径”。

失败时要**尽快退出，让上层切换策略**。




# 5. Step 3：打印真实车位几何结果


```
C++"==Carry out 2.APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()====Obj2Pt(...)==Obj1Pt(...)==Obj2Ang(...)==Obj1Ang(...)==OrgAng(...)==MaxSubLaneX(...)==MaxSlotInnerX(...)"
```


## 作用

把这一步解析出的核心几何量打印出来。



## 维护时非常重要

以后只要看到这条日志，就说明：




- 
没走兜底模板路径




- 
已经成功从真实车位框提取出关键几何





排障时，这条日志能直接回答：




- 
Obj1/Obj2 对不对




- 
OrgAng 对不对




- 
MaxSlotInnerX 是否异常







# 6. Step 4：按车辆当前姿态计算车位长度


```
C++if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {  TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);} else {  TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);}fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
```

这一步特别值得理解。



## 它不是直接“点到点距离”

而是：




- 
先根据车辆当前位置构造一条参考线 `TempLine`




- 
再求 `Obj1Pt`、`Obj2Pt` 到这条线的距离




- 
最后把两个距离加起来，得到 `SlotLength`






## 为什么这么算

因为这里的长度语义不是简单的欧氏距离，而更接近：



>
> 当前泊出方向下，车位在车辆参考轴上的有效长度
>
>
>

这和后续路径规划、终点位是否足够、安全余量等更匹配。



## 平行泊出和非平行泊出为何不同

这里传给 `AlgCom_LineParABCByCurrentCarPosition` 的第二个参数不同：




- 
平行泊出用 `1`




- 
其它泊出用 `0`





说明不同泊出模式下，长度参考轴不同。




# 7. Step 5：校验车位长度是否合法


```
C++if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&     (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - 150.0)) ||    ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&     (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - 150.0))) {  APAMAP_Setfailcause(100);  return FALSE;}
```


## 作用

检查车位长度是否太小。



## 含义



- 
平行泊出：用平行车位的最小长度阈值




- 
非平行泊出：用垂直/其它车位的最小长度阈值





并且都带一个 `-150mm` 容差。



## 失败码



- 
`failcause = 100`






## 维护结论

`100` 表示：



>
> 真实车位框虽然提取出来了，但长度太小，不足以作为有效泊出车位。
>
>
>

这类问题一般不是代码逻辑错，而是：




- 
车位检测不完整




- 
车位框畸形




- 
目标车位本身就不满足条件







# 8. Step 6：计算 SlotType 和 SlotDepth


```
C++if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {  SlotType = 0;  SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);} else if (...) {  SlotType = 1;  SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);} else {  SlotType = 1;  SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);}
```


## 作用

确定车位类型和车位深度。



## 观察

这里无论哪种模式，`SlotDepth` 都是：



```
C++MaxSlotInnerX + 300
```

说明深度的核心来源就是 `MaxSlotInnerX`，只是在工程上统一额外补了 `300mm` 安全余量。



## `SlotType`



- 
平行车位：`0`




- 
非平行：`1`





这会影响后面边界构造和路径策略。



## 维护结论

这个函数里的 `SlotDepth` 更偏真实车位框的“内边界深度 + 冗余量”，不是简单车辆模板深度。




# 9. Step 7：设置新的局部坐标原点


```
C++OrgPt = Obj2Pt;
```

这是一个非常重要的设计选择。



## 含义

新的局部坐标系原点直接取 `Obj2Pt`。


后面会写回：



```
C++APAMap_GInfo.NewCordSysOPt = OrgPt;APAMap_GInfo.NewCordSysAng = OrgAng;
```

于是后续所有边界、终点位、坐标变换都围绕：



```
原点 = Obj2Pt角度 = OrgAng
```

这个坐标系展开。



## 维护结论

以后如果发现：




- 
边界整体平移不对




- 
终点位相对车位口偏移异常





要优先看 `Obj2Pt / OrgAng`，因为它们定义了新的基础坐标系。




# 10. Step 8：基于车身外廓，估计车位最大可用 X

这一段很重要，但容易看乱。




## 10.1 平行泊出分支


```
C++if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {  if (bSlotDataAtRigthSide == FALSE) {    TempPt1.x = -HalfWidthOfCar;  } else {    TempPt1.x = HalfWidthOfCar;  }  TempPt1.y = -LenBetweenRAxisAndRBumper;  TempPt2 = ... 当前车后角点 ...  TempPt2 = ... 转到以 OrgPt/OrgAng 为基准的旧坐标系 ...  TempPt1.y = LenBetweenRAxisAndFBumper;  TempPt3 = ... 当前车前角点 ...  TempPt3 = ... 转到同一坐标系 ...}
```


## 10.2 非平行泊出分支


```
C++else {  if (HEAD_GO_STRAIGHT 或 HEAD_TURN_ROUND) {    TempPt1.y = -LenBetweenRAxisAndRBumper;  } else {    TempPt1.y = LenBetweenRAxisAndFBumper;  }  TempPt1.x = HalfWidthOfCar;  TempPt2 = ...  TempPt1.x = -HalfWidthOfCar;  TempPt3 = ...}
```


## 这段在做什么

它不是在算车位点，而是在算：



>
> 当前车辆外廓在新车位坐标系下，最远能占到哪里。
>
>
>

得到两个点：




- 
`TempPt2`




- 
`TempPt3`





然后取它们的更大 X 作为：



```
C++MaxSlotPtX
```

再与 `MaxSlotInnerX` 比较：



```
C++if (MaxSlotInnerX > MaxSlotPtX) {  MaxSlotPtX = MaxSlotInnerX;}
```

最终写入：



```
C++APAMap_GInfo.SlotPar.slotCarEndPosXBackUp = MaxSlotPtX;
```


## 它的工程意义

这是在保存一个“车位口方向上，可用于后续终点位/边界构造的最大安全 X”。


可以理解成：



>
> 当前车身外廓和车位内部边界共同决定的一个保守边界值。
>
>
>



# 11. Step 9：保存 SlotLen / SlotDepth


```
C++APAMap_GInfo.SlotPar.SlotDepth = SlotDepth;APAMap_GInfo.SlotPar.SlotLen = SlotLength;
```


## 含义

到这里，车位自身几何的两个核心标量已经有了：




- 
长度 `SlotLen`




- 
深度 `SlotDepth`





后面很多函数都依赖它们：




- 
边界偏移




- 
安全距离计算




- 
小车位/短车位判定




- 
终点位策略调整







# 12. Step 10：计算终点位 EndPos


```
C++TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(    ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
```

如果支持 UWB，会优先用 UWB 版本；否则走默认版本。



## 作用

根据：




- 
泊出模式




- 
新坐标系原点 `OrgPt`




- 
新坐标系角度 `OrgAng`




- 
`Obj2Pt`




- 
是否终点位侵占





计算终点位。



## 为什么放在这里

因为到这里才有足够信息：




- 
车位几何已建立




- 
局部坐标系已建立




- 
车位长度深度已确定






## 错误处理


```
C++if (TempPt3.x == 0xff) {  APAMAP_Setfailcause(101);  return FALSE;}
```


### 含义

终点位计算失败。



### 失败码



- 
`101`






### 维护结论

`101` 一般代表：



>
> 车位边界已有，但终点位无法合理生成。
>
>
>

重点查：




- 
`ParkOutMode`




- 
`OrgPt / OrgAng`




- 
`Obj2Pt`




- 
`APAMap_ParkingOutSetEndCarPosInOldCorSys()`







# 13. Step 11：写回 EndPos 与 EndPosLine


```
C++APAMap_GInfo.SlotPar.EndPos.Coordinate = TempPt3;APAMap_GInfo.SlotPar.EndPosLine =    AlgCom_LineParABCByCurrentCarPosition(&APAMap_GInfo.SlotPar.EndPos, 0);
```


## 作用

不仅保存终点位坐标，还立即构造终点位对应的参考线 `EndPosLine`。


这说明后续很多逻辑不是只看一个终点点，而是把终点位当成一条方向线来使用。




# 14. Step 12：把所有结果写入全局状态

这一段是整个函数最重要的“落盘”部分。



```
C++APAMap_GInfo.bDataMirrored = FALSE;APAMap_GInfo.bCordSysReSet = FALSE;APAMap_GInfo.SlotPar.SlotType = SlotType;APAMap_GInfo.SlotPar.bObj2Exist = TRUE;APAMap_GInfo.SlotPar.bObj1Exist = TRUE;APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;APAMap_GInfo.NewCordSysOPt = OrgPt;APAMap_GInfo.NewCordSysAng = OrgAng;APAMap_GInfo.SlotPar.Obj2Ang = Obj2Ang;APAMap_GInfo.SlotPar.Obj1Ang = Obj1Ang;
```


## 作用

把这个函数解析出来的“车位骨架”全部写入后续全局状态。



## 这意味着什么

从这里开始，后面的函数默认认为：




- 
当前车位边界已经建立




- 
新坐标系已经建立




- 
Obj1/Obj2 是可信的




- 
SlotLen/SlotDepth 是可信的




- 
EndPos 是第一版可信终点位





所以后面的 `FSDAndODMapInfo()` 更像是在这个基础上修正，而不是重新从零构造。




# 15. Step 13：最终日志


```
C++==FirstBuildMapObjAndEndCarPos==...
```

这条日志非常关键。



## 它说明什么

只要你看到这条日志，就可以确认：




- 
当前走的是“真实车位框路径”




- 
车位边界已建成




- 
新坐标系已建成




- 
初始终点位已建成





这在维护中非常好用，因为它是“车位骨架生成完成”的明确信号。




# 16. 这个函数的维护版一句话总结

`APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()` 的核心是：



>
> **从真实车位框中提取 Obj1/Obj2、角度和车位内部边界，建立新的局部车位坐标系，并据此生成第一版终点位与车位几何骨架。**
>
>
>



# 17. 这个函数最容易出问题的点



## 问题 1：真实车位框提取失败

根源通常在：



```
C++APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(...)
```

这一步失败，本函数直接退出。




## 问题 2：SlotLength 太小导致 failcause=100

说明真实车位框有了，但长度不够。

这往往是检测车位框畸形或不完整，不一定是后面边界构造问题。




## 问题 3：EndPos 计算失败导致 failcause=101

说明车位骨架建立了，但终点位无法合理生成。

重点看终点位函数，不要只盯 Obj1/Obj2。




## 问题 4：左右翻转异常

这里虽然没有直接生成全部边界，但已经用了：




- 
`bSlotDataAtRigthSide`




- 
`OrgPt = Obj2Pt`




- 
`OrgAng`





如果这三个语义不对，后面边界和终点位都会偏。




## 问题 5：`slotCarEndPosXBackUp` 异常

这个量后续会影响默认边界和终点位安全范围。

如果这里算得过小或过大，后面很多保守边界都会出偏差。




# 18. 现在对后续维护最重要的认知

到这里可以把车位初始化路径理解成两层：



### 第一层：真实车位骨架

由本函数建立：




- 
Obj1 / Obj2




- 
Obj1Ang / Obj2Ang




- 
OrgPt / OrgAng




- 
SlotLen / SlotDepth




- 
EndPos






### 第二层：环境修正

由后面的 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 继续修正：




- 
OffsetX / OffsetY




- 
FSDOffset / ODOffset




- 
EndPos 是否侵占




- 
SlotBordPt 最终修正





所以本函数是“骨架层”，不是“最终层”。




# 19. 下一步最建议解析哪个函数

现在最自然的下一步是：


**`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`**


因为这样正好把刚刚建立的“骨架车位”继续推进到“最终可用车位”。


如果从维护效率看，这是最有价值的一步。

---

## 🙋 用户

粘贴的文本 (1).txt
文档

---

## 🤖 ChatGPT

下面继续按**维护视角**解析 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`。这一个函数非常重要，因为它不是“从零建车位”，而是：



>
> **在前一步已经得到的车位骨架基础上，用传感器/FSD/OD信息把 Obj1、Obj2、SlotLen、EndPos 修正成更接近真实环境的最终结果。** 
>
>
>

它可以理解成“**车位骨架二次精修函数**”。




# 1. 这个函数在整条链路里的定位

前面已经有了：




- 
`Obj2Pt / Obj1Pt`




- 
`Obj2Ang / Obj1Ang`




- 
`OrgPt / OrgAng`




- 
`SlotLen / SlotDepth`




- 
`EndPos`





这些是“第一版骨架”。


这个函数做的事是：



```
骨架车位  ↓取 Sensor/FSD/OD 偏移  ↓计算 Obj1/Obj2 各自应该内缩/平移多少  ↓重新计算车位长度  ↓必要时重新计算终点位 EndPos  ↓把修正后的结果写回全局
```

所以后面看到的日志：



```
==FSDOffset(...)==ODOffset(...)==app==ObjbyFSDAndOD====SlotBordPt(...)
```

基本都来自这个函数。




# 2. 先给出主流程骨架

这个函数可以拆成 10 步：



```
① 读取当前车位骨架② 计算 Sensor / FSD / OD 三类偏移③ 取三类偏移的“最大值”作为综合偏移④ 计算 Obj1 / Obj2 距离车位参考线的剩余安全距离 fDis1/fDis2⑤ 判断是否要禁用默认边界修正⑥ 做安全距离校验（failcause 58）⑦ 按 OffsetX/OffsetY 分别修正 Obj1 和 Obj2⑧ 重新计算 SlotLen，并标记长/短车位⑨ 视条件重新计算 EndPos⑩ 把修正结果写回全局
```



# 3. Step 1：读取“骨架车位”作为输入

一开始先把前一步生成的骨架读出来：



```
C++Obj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];Obj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];SlotLen = APAMap_GInfo.SlotPar.SlotLen;EndPos = APAMap_GInfo.SlotPar.EndPos;EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;OrgAng = APAMap_GInfo.NewCordSysAng;OrgPt = APAMap_GInfo.NewCordSysOPt;bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
```


### 维护理解

这说明它不是独立函数，而是强依赖前一步结果。

如果前面的 `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()` 或兜底函数出了问题，这里只会在错误骨架上继续“精修”，不会自动纠正源头。




# 4. Step 2：分别计算三类偏移


## 4.1 传感器偏移


```
C++APAMap_CalSlotBorderPtOffsetBySensorMapInfo(&SensorOffsetX1, &SensorOffsetY1,                                            &SensorOffsetX2, &SensorOffsetY2);
```


### 含义

从传感器地图信息出发，估计 Obj1、Obj2 两个边界点各自应偏移多少。


这类偏移更偏近距离安全信息，通常保守。




## 4.2 FSD 偏移


```
C++APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(&FSDOffsetX1, &FSDOffsetY1,                                                &FSDOffsetX2, &FSDOffsetY2);
```


### 含义

从顶视 FSD 地图估计边界点偏移。


这类偏移更偏视觉/语义几何。




## 4.3 OD 偏移


```
C++APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(...)或APAMap_CalSlotBorderPtOffsetByODMapInfo(...)
```


### 含义

从障碍物检测出发估计偏移。


工程上这里又做了一层阈值过滤：



```
C++if (ODOffsetX1 < 50) ODOffsetX1 = 0;...
```

也就是 **小于 50mm 的 OD 修正直接忽略**。



### 维护结论

OD 偏移在这里被当成“够明显才启用”的修正源，不是轻微波动都用。




# 5. Step 3：综合偏移的策略不是相加，而是“逐维取最大”

先初始化为 FSD：



```
C++OffsetX1 = FSDOffsetX1;OffsetY1 = FSDOffsetY1;OffsetX2 = FSDOffsetX2;OffsetY2 = FSDOffsetY2;
```

然后逐项和 Sensor 比：



```
C++if (OffsetX1 < SensorOffsetX1) OffsetX1 = SensorOffsetX1;...
```

再逐项和 OD 比：



```
C++if (OffsetX1 < ODOffsetX1) OffsetX1 = ODOffsetX1;...
```



## 核心结论

这个函数的综合偏移不是：



```
Sensor + FSD + OD
```

而是：



```
综合偏移 = 三者中最保守、最大的那个
```


### 维护意义

这是典型的**安全优先策略**。

它避免多个源叠加导致过度收缩，同时又保证不会比任一风险源更乐观。


所以以后看日志时：



```
OffsetX / OffsetY
```

不等于单一源，而是三类来源竞争后的最大值。




# 6. Step 4：计算 `fDis1 / fDis2`

这两个量很关键，它们表示 Obj1 / Obj2 到“车位参考线”的可用距离。




## 4.1 有车位框时


```
C++if (TRUE == bCarryOutSlot) {  fDis2 = APAMap_GetSearchMaxInnerY(... Obj2Pt ...);  fDis1 = APAMap_GetSearchMaxInnerY(... Obj1Pt ...);}
```


### 含义

如果当前是“采用车位框”的状态，就按车位框方向去搜索最大内部距离。




## 4.2 没有采用车位框时


```
C++Line = APAMAP_GetSlotLineByCarPos();TempDis1 = APAMap_GetDisByCarPosToBumper(1);fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, Line) - TempDis1;...
```


### 含义

否则就退回到“车位点到车辆参考线的距离 - 车身到保险杠距离”。




## 维护结论

`fDis1 / fDis2` 是后面判断：




- 
偏移还能不能施加




- 
剩余安全空间够不够




- 
是否会触发 failcause 58





的核心变量。




# 7. Step 5：是否启用默认边界修正 `bUpdataDefaulBordenFlag`

这部分逻辑比较工程化。



```
C++bUpdataDefaulBordenFlag = FALSE;if (TRUE == bAfterNewAnchorPointFlag) {  ...  if (CurCarCoordinateX > 某阈值) {    bUpdataDefaulBordenFlag = TRUE;  }}
```


### 直白理解

如果已经发生锚点转换，且车辆已经“明显驶出车位”，那么就不再继续使用之前那种保守默认边界修正，而是把 Offset 归零：



```
C++OffsetX1 = 0;OffsetX2 = 0;OffsetY1 = 0;OffsetY2 = 0;
```


### 为什么

因为车已经出来了，再继续按车位内逻辑去收缩 Obj1/Obj2，反而会错误约束。




## 与斜列车位相关的特殊处理


```
C++if (TRUE == bLabelAngledFlag) {  if (TRUE == bUpdataDefaulBordenFlag) {    bLabelAngledParkingOutSlotFlag = TRUE;  }  if (TRUE == bLabelAngledParkingOutSlotFlag) {    bUpdataDefaulBordenFlag = TRUE;  }}
```


### 含义

斜列车位一旦进入某个“已泊出”状态，这个标志会持续保持，后面继续禁用默认边界收缩。



### 维护结论

这是一种“场景锁存”机制，防止状态抖动。




# 8. Step 6：默认 Y 偏移和安全距离校验

如果没有禁用默认边界修正：



```
C++DefaultOffsetY1 = 0;DefaultOffsetY2 = 0;if ((ParkOutMode != PARALLEL) && (FALSE == bCarryOutSlot)) {  DefaultOffsetY1 = fDis1 - 600;  DefaultOffsetY2 = fDis2 - 600;}if (OffsetY1 < DefaultOffsetY1) OffsetY1 = DefaultOffsetY1;if (OffsetY2 < DefaultOffsetY2) OffsetY2 = DefaultOffsetY2;
```


### 含义

对于非平行泊出且不是“采用车位框”的场景，会额外要求留出大约 `600mm` 的默认剩余空间。




## 安全校验


```
C++NewDis1 = fDis1 - OffsetY1;NewDis2 = fDis2 - OffsetY2;NewDis = NewDis1 + NewDis2;if (NewDis < 2 * SafeDis) {  APAMAP_Setfailcause(58);  return FALSE;}
```

这里 `SafeDis = 250`。



### 含义

两边修正完之后，车位总剩余空间必须至少大于 `500mm`。否则判定风险太高，直接失败。



### 失败码



- 
`58`






### 维护结论

`failcause=58` 不是“没检测到车位”，而是：



>
> **检测到了车位，但按 FSD/OD/Sensor 修正后，剩余安全空间太小，不允许继续。**
>
>
>

这在排障时很重要。




# 9. Step 7：对短车位的保守处理


```
C++if (ParkOutMode == PARALLEL) {  if (SlotLen < (LengthOfCar + 700)) {    OffsetY1 = 0;    OffsetY2 = 0;  }} else {  if (SlotLen < (WidthOfCar + 500)) {    OffsetY1 = 0;    OffsetY2 = 0;  }}
```


### 含义

如果车位已经比较短了，就不要再沿 Y 方向继续收缩，否则可能把本来勉强可用的车位直接收没了。



### 维护结论

这里体现的是“短车位弱化 Y 修正”的策略。




# 10. Step 8：分别修正 Obj1 和 Obj2

这是整个函数最核心的几何操作。




## 10.1 修正 Obj1

思路是：




- 
先取 Obj1 的边界方向线 `TempLine2`




- 
按 `OffsetY1` 沿 Obj1 自身方向做平移




- 
再取车位坐标系方向线 `TempLine1`




- 
按 `OffsetX1` 沿 X 方向平移




- 
最后求两条线交点，得到新的 `Obj1Pt`






```
C++if ((OffsetY1 != 0) || (OffsetX1 != 0)) {  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &Obj1Pt);}
```


### 维护理解

Obj1 的修正不是直接 `x+=... y+=...`，而是**重新构造两条偏移后的几何线，再取交点**。

所以它能保持边界方向语义，而不是简单平移点。




## 10.2 修正 Obj2

Obj2 逻辑类似，但更复杂一些，多了几个工程分支：




- 
`BloundaryOffsetY`




- 
`bCntAddFlag`




- 
`bShortSlotLen`




- 
平行小车位时 `OffsetX2 = 0`





最终也是通过两条偏移线求交点得到新的 `Obj2Pt`。



### 为什么 Obj2 更复杂

因为 Obj2 通常既是：




- 
车位口关键点




- 
新坐标系原点候选




- 
终点位计算基准





所以工程上对它更谨慎。




# 11. Step 9：车身侵占车位边界修正


```
C++APAMap_ParkingOutCarPosInvadeSlotBorderInfo(&Obj2Pt, &Obj1Pt,                                            bUpdataDefaulBordenFlag);
```


### 含义

在前面偏移修正之后，再检查当前车辆是否侵占了车位边界，并对 Obj1/Obj2 做补偿。



### 维护理解

这是最后一道“车身实际占位”修正，不是纯感知驱动，而是和当前车姿直接相关。




# 12. Step 10：重新计算 SlotLen，并判断长/短车位类型


```
C++Line = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);SlotLen = AlgCom_GetPointToLineDis(Obj1Pt, Line);
```


### 含义

用修正后的 Obj2 方向重新算车位长度。




## 平行泊出时还会标记三种状态


```
C++bLonggestSlotLenbShortSlotLenbShortestSlotLen
```

判定规则大意是：




- 
`> LengthOfCar + 2000`：超长车位




- 
`<= LengthOfCar + 1500`：短车位




- 
`<= LengthOfCar + 1100`：极短车位






### 维护意义

这几个标志后面会影响：




- 
边界收缩策略




- 
Obj2 X 修正策略




- 
终点位策略




- 
平行泊出路径保守程度







# 13. Step 11：是否重新计算 EndPos

这部分非常关键。




## 11.1 某些情况下不更新终点位


```
C++if ((TRUE == bAfterNewAnchorPointFlag) ||    (TRUE == bLaneLineUpdateEndCarPosFlag) ||    (TRUE == bRefercLineUpdateEndCarPosFlag)){  EndPos = APAMap_GInfo.SlotPar.EndPos;  EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;}
```


### 含义

如果已经发生：




- 
锚点转换




- 
车道线更新终点位




- 
参考线更新终点位





那这里就不要再改终点位，避免互相覆盖。




## 11.2 否则重新根据新 Obj2 计算终点位


```
C++EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(    ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
```

然后重新生成：



```
C++EndPosLine = AlgCom_LineParABCByCurrentCarPosition(&EndPos, 0);
```


### 错误处理

如果 `EndPos.Coordinate.x == 0xff`，设：




- 
`failcause=101`






### 维护结论

终点位不是固定值，它会随 Obj2 的修正而重算。

所以后面如果出现“边界修得对但终点位奇怪”，重点看这里。




# 14. Step 12：打印核心调试日志


```
C++==app==ObjbyFSDAndOD==ObjPt2(...)ObjPt1(...)EndPos(...)SlotLen(...)==Dis(...)==OffsetX(...)==OffsetY(...)==FSDOffset(...)==ODOffset(...)==SensorOffset(...)==bLonggestSlotLen(...)==bShortSlotLen(...)==bShortestSlotLen(...)
```


### 这是最值得保留的一条日志

因为它把这个函数内部所有关键结果一次性打全了。


以后排障这个函数，最先看这条就够了。




# 15. Step 13：写回全局状态

最后把修正结果落回去：



```
C++APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;...APAMap_GInfo.NewCordSysOPt = APAMap_GInfo.SlotPar.SlotBordPt[0];...APAMap_GInfo.SlotPar.SlotLen = (APA_DISTANCE_TYPE)SlotLen;APAMap_GInfo.SlotPar.EndPos = EndPos;APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
```


### 有一个特别重要的点

这里把：



```
C++APAMap_GInfo.NewCordSysOPt = APAMap_GInfo.SlotPar.SlotBordPt[0];
```

也就是**新坐标系原点更新成修正后的 Obj2 点**。



### 维护意义

这说明前面真实车位框建立的坐标系原点并不是最终版；经过 FSD/OD 修正后，原点会再次更新。


这会影响后续：




- 
主边界构造




- 
子边界构造




- 
LaneLine/RefercLine 融合




- 
终点位坐标系解释







# 16. 这个函数的维护版一句话总结

`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 的本质是：



>
> **把前一步“基于车位框建立的骨架车位”，用 Sensor/FSD/OD 的最保守偏移进行几何修正，并重算车位长度、终点位和最终坐标系原点。** 
>
>
>



# 17. 后续维护最该盯的 6 个问题


## 1）为什么 `failcause=58`

修正后剩余安全空间太小，不满足 `2 * SafeDis`。



## 2）为什么 `Obj1/Obj2` 变了很多

看：




- 
`OffsetX/OffsetY`




- 
`FSDOffset`




- 
`ODOffset`




- 
`SensorOffset`






## 3）为什么终点位突然变了

因为 Obj2 修正后重新计算了 `EndPos`。



## 4）为什么坐标系整体漂了

因为 `NewCordSysOPt` 被更新成了新的 `SlotBordPt[0]`。



## 5）为什么某些帧不更新 EndPos

可能触发了：




- 
`bAfterNewAnchorPointFlag`




- 
`bLaneLineUpdateEndCarPosFlag`




- 
`bRefercLineUpdateEndCarPosFlag`






## 6）为什么平行短车位表现很奇怪

看：




- 
`bShortSlotLen`




- 
`bShortestSlotLen`




- 
`OffsetY` 是否被清零




- 
`OffsetX2` 是否被限制







# 18. 到这里，主线已经比较完整了

现在你已经基本有了这条主线：



```
APAMap_ParkingOutTask  → APAMap_ParkingOutCalMapSlotPar  → APAMap_ParkingOutCalSlotInfo      → APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo / ByParkOutInfo      → APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo
```

也就是：



```
先找车位  ↓建车位骨架  ↓感知修正车位
```

下一步最自然的是继续看：


**`APAMap_ParkingOutCalMapInfo()`**


因为它就是在这个“最终车位”基础上，开始真正构建：




- 
主边界




- 
子边界




- 
FSD/LaneLine/RefercLine/OD 融合边界





这一步进入“地图维护”的核心。

---

## 🙋 用户

BOOLEAN APAMap_ParkingOutCalMapInfo() {
  BOOLEAN bResult;
  uint8_t_INF ParkOutMode;
  APACoordinateDataCalFloatType Obj2Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType OrgPt;
  APACarCoordinateDataCalFloatType EndPos;
  APA_ENUM_TYPE i;
  BOOLEAN bCenterEndCarPosFlag = FALSE; //采用终点位置居中标志位
  BOOLEAN bSeizeEndCarPosFlag;          // fsd侵占终点位置标志位

  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
  bResult = APAMap_ParkingOutCalBoundaryByParkOutInfo();
  BoudaryNum[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
#ifdef APAMAP_PARKOUT_FUS_PDC
  if (bResult) {
    APAMap_CalMapSubBoundaryByPDCInfo();
  }
#endif
  if (bResult) {
    bResult = APAMap_ParkingOutFusBoundaryByFSDMapInfo();
    BoudaryNum[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
    debug3++;
  }
  if (bResult) {
    bResult = APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
    BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }
  if (bResult) {
    bResult = APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
    BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }
  if (bResult) {
    bResult = APAMap_FusBoundaryByODMapInfo();
    BoudaryNum[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
    debug4++;
  }
  if (bResult) {
#ifdef APAMAP_PARKOUT_FUS_SDG
    APAMap_ParkingOutUpDataMapBoundaryBySDGInfo();
    APAMap_ParkingOutDeleteMainSlotBord();
#endif
#ifdef APAMAP_PARKOUT_FUS_PDC
    APAMap_UpDataMapBoundaryByPDCInfo();
    APAMap_ParkingOutDeleteMainSlotBord();
#endif
    APAMap_SmoothMapBoundary(0);
    BoudaryNum[3][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum[3][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
    debug2++;
  }
  // zqf: add EndCarPos update
  i = 0;
  bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
  if (TRUE == bSeizeEndCarPosFlag) {
    bCenterEndCarPosFlag = APAMap_ParkingOutCenterEndCarPosInfo();
    if (TRUE == bCenterEndCarPosFlag) {
      bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
    }
    {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==CenterEndCarPosUpdata===bCenterEndCarPosFlag(%d)"
               "==bSeizeEndCarPosFlag(%d)==EndPos_Coordinate(%.2f,%.2f)",
               bCenterEndCarPosFlag, bSeizeEndCarPosFlag,
               APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
               APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
      TLOG_INFO << log_string;
    }
  }
  while (TRUE == bSeizeEndCarPosFlag) {
#ifdef SUPPORT_PARKING_OUT_UWB
    if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
          ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    } else {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
          ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    }
#else
    EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
#endif
    APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
    bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
    i++;
    if (i > 9) {
      bSeizeEndCarPosFlag = FALSE;
    }
  }
  return bResult;
}

---

## 🤖 ChatGPT

下面继续按**维护视角**解析 `APAMap_ParkingOutCalMapInfo()`。


这个函数是主流程里真正的**地图构建总控函数**。

前面的函数主要解决的是：




- 
目标车位是谁




- 
车位边界点在哪里




- 
初始终点位在哪里





而这个函数解决的是：



>
> **在车位骨架已经确定后，如何构造左右边界，并逐步融合 FSD、LaneLine、RefercLine、OD 等信息，最终得到一张可用于泊出的地图。**
>
>
>



# 1. 这个函数的核心定位

可以把它理解成：



```
车位级几何   ↓道路/边界级地图
```

也就是说：




- 
`CalSlotInfo()` 更偏“车位内部”




- 
`CalMapInfo()` 更偏“车位外部通道与边界”





它做完之后，`APAMap_GInfo.OutLine.LeftBoundary / RightBoundary` 才算真正成型。




# 2. 主流程骨架

这个函数可以拆成 8 个阶段：



```
① 读取当前坐标系与关键点② 用车位几何生成初始边界③ 融合 FSD 边界④ 融合 LaneLine 边界⑤ 融合 RefercLine 边界⑥ 融合 OD 边界⑦ 平滑边界，并做 SDG/PDC 后处理⑧ 检查终点位是否被边界侵占，必要时反复重算终点位
```

所以它本质不是“一个算法”，而是一个**边界融合流水线**。




# 3. Step 1：读取基础输入


```
C++OrgAng = APAMap_GInfo.NewCordSysAng;OrgPt = APAMap_GInfo.NewCordSysOPt;Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
```


## 含义

这里读的是前面 `CalSlotInfo()` 已经修正好的最终车位骨架：




- 
`OrgAng`：新局部坐标系角度




- 
`OrgPt`：新局部坐标系原点




- 
`Obj2Pt`：关键边界点




- 
`ParkOutMode`：泊出模式






## 维护理解

这说明当前函数强依赖前一阶段的结果。

如果这里的 `OrgPt / OrgAng / Obj2Pt` 已经偏了，后续整个左右边界都会跟着偏。




# 4. Step 2：先用车位信息生成“初始主边界”


```
C++bResult = APAMap_ParkingOutCalBoundaryByParkOutInfo();BoudaryNum[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;BoudaryNum[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
```


## 作用

这是地图构建的起点。


它会根据：




- 
`Obj1Pt / Obj2Pt`




- 
`SlotLen / SlotDepth`




- 
`OrgPt / OrgAng`




- 
`ParkOutMode`




- 
左右侧关系





先生成一版默认的：




- 
左边界




- 
右边界






## 这一步的重要性

后面所有融合，都是在这版初始边界基础上进行的。

所以可以把它理解为：



>
> **地图边界的初稿。**
>
>
>


## `BoudaryNum[0]`

这是调试统计数组，用来记录“初始边界”的点数。



### 维护意义

排障时如果发现：




- 
后面融合失败




- 
或边界越来越怪





先看初始边界点数是不是就异常。




# 5. Step 3：可选的 PDC 子边界


```
C++#ifdef APAMAP_PARKOUT_FUS_PDCif (bResult) {  APAMap_CalMapSubBoundaryByPDCInfo();}#endif
```


## 含义

如果编译宏打开，会先补充 PDC 的子边界信息。



## 维护理解

这一步不改变主流程结构，但说明边界体系不只是一条主边界，还有可能有“子边界”。


后面如果遇到：




- 
同样的代码在不同项目/车型表现不同





先确认是不是这些编译宏不同。




# 6. Step 4：融合 FSD 边界


```
C++if (bResult) {  bResult = APAMap_ParkingOutFusBoundaryByFSDMapInfo();  BoudaryNum[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;  BoudaryNum[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;  debug3++;}
```


## 作用

把 FSD 识别到的边界/线段融合到当前地图边界上。



## 为什么 FSD 放这么前

因为 FSD 通常能提供比较强的几何边缘信息，对边界修正价值很高。



## 维护理解

这一步之后，边界会从“车位模板边界”变成“带视觉感知修正的边界”。



## `BoudaryNum[1]`

记录 FSD 融合后的边界点数。



### 维护用法

如果你发现初始边界正常，但 FSD 融合后点数异常暴涨、骤减，问题大概率在 `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`。




# 7. Step 5：融合 LaneLine


```
C++if (bResult) {  bResult = APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();  BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;  BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;}
```


## 作用

把车道线信息融进当前边界。



## 和 FSD 的区别



- 
FSD 更像“感知到的障碍/线段/边缘”




- 
LaneLine 更像“车道约束”





所以这一步更偏通道边界修正，而不是车位边缘修正。



## 维护理解

这一步常常会影响：




- 
终点位方向




- 
通道中线




- 
一侧边界是否更平滑/更接近真实车道







# 8. Step 6：融合 RefercLine


```
C++if (bResult) {  bResult = APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();  BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;  BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;}
```


## 作用

融合车位参考线相关信息。



## 维护理解

这一层一般对：




- 
终点位朝向




- 
边界方向一致性




- 
车位出口方向约束





比较重要。


如果以后碰到：




- 
边界有了，但方向很怪




- 
或终点位角度不稳定





要重点看这一层和 LaneLine 层之间有没有冲突。




# 9. Step 7：融合 OD


```
C++if (bResult) {  bResult = APAMap_FusBoundaryByODMapInfo();  BoudaryNum[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;  BoudaryNum[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;  debug4++;}
```


## 作用

把障碍物检测结果融到地图边界里。



## 和前几层的关系

前几层更像：




- 
车位/道路/线条信息





这一层更像：




- 
实际障碍约束





所以它通常让边界更保守。



## 维护理解

如果边界在这一层之后突然向内收缩很多，常见原因有：




- 
OD 数据太保守




- 
障碍物误检




- 
某些点没做过滤就被融合进来







# 10. Step 8：后处理和平滑


```
C++if (bResult) {#ifdef APAMAP_PARKOUT_FUS_SDG  APAMap_ParkingOutUpDataMapBoundaryBySDGInfo();  APAMap_ParkingOutDeleteMainSlotBord();#endif#ifdef APAMAP_PARKOUT_FUS_PDC  APAMap_UpDataMapBoundaryByPDCInfo();  APAMap_ParkingOutDeleteMainSlotBord();#endif  APAMap_SmoothMapBoundary(0);  BoudaryNum[3][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;  BoudaryNum[3][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;  debug2++;}
```

这一段很工程化。




## 10.1 SDG / PDC 更新

如果开启宏，会进一步：




- 
用 SDG 信息更新边界




- 
用 PDC 信息更新边界




- 
删除主车位边框中不再需要的部分






### 含义

说明最终地图边界并不只是“简单融合”，还会做一些结构性裁剪。




## 10.2 平滑边界


```
C++APAMap_SmoothMapBoundary(0);
```


### 作用

把前面多源融合后的边界做平滑处理。



### 为什么必须有这一步

前面每一层融合都可能带来：




- 
点数不均匀




- 
折线突变




- 
局部锯齿




- 
不连续边界





所以必须在输出前统一整理。




## `BoudaryNum[3]`

记录平滑后的最终边界点数。



### 维护用法

如果前面都正常，最终边界却异常，重点看：




- 
`DeleteMainSlotBord`




- 
`SmoothMapBoundary`







# 11. Step 9：检查终点位是否被边界侵占

这是这个函数后半段最关键的逻辑。



```
C++i = 0;bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
```


## 含义

检查当前边界是否“侵占了终点位”。


也就是：



>
> 地图边界构建完以后，看看原先算出来的终点位是不是已经不安全了。
>
>
>

这很合理，因为：




- 
终点位是之前按车位骨架算的




- 
现在边界被 FSD/LaneLine/OD 等修过了




- 
原来的终点位可能已经落到障碍或过窄区域里







# 12. Step 10：先尝试“终点位居中”修正


```
C++if (TRUE == bSeizeEndCarPosFlag) {  bCenterEndCarPosFlag = APAMap_ParkingOutCenterEndCarPosInfo();  if (TRUE == bCenterEndCarPosFlag) {    bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();  }  ...}
```


## 作用

如果终点位被侵占，先尝试一种温和修正：



>
> 把终点位居中。
>
>
>


## 逻辑



- 
先居中修正




- 
再重新检查是否还被侵占






## 维护理解

这一步说明作者希望先用“轻量调整”修复，而不是立刻大幅重算终点位。




# 13. Step 11：若仍被侵占，循环重算终点位


```
C++while (TRUE == bSeizeEndCarPosFlag) {  EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(...);  APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;  bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();  i++;  if (i > 9) {    bSeizeEndCarPosFlag = FALSE;  }}
```


## 含义

如果边界还是侵占终点位，那就进入迭代：



```
重算终点位  ↓检查是否仍被侵占  ↓若仍侵占，再重算
```

最多重试 10 次。




## 为什么要循环

因为终点位和边界之间是耦合的：




- 
边界决定终点位是否安全




- 
终点位位置变化又会影响下一次安全判定





所以单次修正不一定够。




## 为什么限制 10 次

防止死循环。



### 维护结论

如果这里经常跑满 10 次，通常说明：




- 
当前边界太紧




- 
终点位策略和边界策略冲突




- 
或某个输入场景已经接近不可解







# 14. `bResult` 的语义

注意，这个函数最终：



```
C++return bResult;
```

但是终点位侵占后的 while 调整，并不会再把 `bResult` 置成 `FALSE`。



## 这说明什么

只要边界构建链成功了，这个函数一般就返回成功。

终点位侵占更多是“尽量修复”，而不是一定当作构图失败。



## 维护理解

所以：




- 
`bResult=FALSE` 更代表边界构建链失败




- 
终点位侵占更多要靠日志和终点位结果看，不一定会直接通过返回值体现







# 15. 这个函数的维护版一句话总结

`APAMap_ParkingOutCalMapInfo()` 的本质是：



>
> **先用车位信息生成初始左右边界，再按 FSD、LaneLine、RefercLine、OD 等多源信息逐层融合并平滑，最后检查并修正终点位是否被最终边界侵占。**
>
>
>



# 16. 后续维护最该盯的 7 个问题


## 1）为什么 failcause=45

这不是在这个函数内部直接设的，而是上层 `APAMap_ParkingOutTask()` 在它返回 `FALSE` 时设的。

所以 45 本质表示：



>
> `APAMap_ParkingOutCalMapInfo()` 某一步边界构建失败了。
>
>
>



## 2）初始边界就不对

看：




- 
`APAMap_ParkingOutCalBoundaryByParkOutInfo()`







## 3）FSD 后边界点数异常

看：




- 
`BoudaryNum[1]`




- 
`APAMap_ParkingOutFusBoundaryByFSDMapInfo()`







## 4）LaneLine/RefercLine 融合后方向异常

看：




- 
`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`




- 
`APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`







## 5）OD 后边界过于保守

看：




- 
`APAMap_FusBoundaryByODMapInfo()`







## 6）最终边界点数正常，但形状很差

看：




- 
`APAMap_ParkingOutDeleteMainSlotBord()`




- 
`APAMap_SmoothMapBoundary(0)`







## 7）终点位总是漂、总是变化

看：




- 
`APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`




- 
`APAMap_ParkingOutCenterEndCarPosInfo()`




- 
`APAMap_ParkingOutSetEndCarPosInOldCorSys()`







# 17. 到这里，主干调用链已经完整了

现在主干已经清楚：



```
APAMap_ParkingOutTask  → APAMap_ParkingOutCalMapSlotPar  → APAMap_ParkingOutCalSlotInfo      → 建车位骨架      → FSD/OD修车位  → APAMap_ParkingOutCalMapInfo      → 建初始边界      → FSD/LaneLine/RefercLine/OD融合边界      → 平滑边界      → 修终点位  → APAMap_ParkingOutCheckIfCarPosIsValid
```

这已经足够支撑后续维护排障。




# 18. 下一步最有价值的两种方向

现在后续最值得做的是二选一：



### 方向 A：继续拆关键子函数

优先建议：




- 
`APAMap_ParkingOutCalBoundaryByParkOutInfo()`




- 
`APAMap_ParkingOutFusBoundaryByFSDMapInfo()`




- 
`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`





这适合继续深入代码实现。



### 方向 B：开始整理“维护手册”

把目前已经讲过的内容整理成：




- 
主流程图




- 
全局变量表




- 
failcause 对照表




- 
关键日志对照表




- 
排障顺序表





这会更适合后续真正接手维护。


如果从工作效率看，下一步我更建议直接开始帮你做一版**维护手册骨架**。

---

## 🙋 用户

bool_t_INF APAMap_ParkingOutCalBoundaryByParkOutInfo() {
  APA_DISTANCE_CAL_FLOAT_TYPE MaxY, MinY;
  BOOLEAN bSlotDataAtRigthSide;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxDefaultRoadWith;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultObj2PtX;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultObj1PtX;
  BOOLEAN bObj2Exist, bObj1Exist;
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1X, Obj2X;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang, Obj1Ang;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APALineParameterABCType TempLine;
  APACarCoordinateDataCalFloatType TempCarPos;
  tMap_BoundPt_t MainBoudary;
  tMap_BoundPt_t SubBoundary;
  APA_ENUM_TYPE i;
  APACoordinateDataCalFloatType TempPt;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj1;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj2;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj3;
  uint8_t_INF ParkOutMode;
  BOOLEAN bUpdataDefaulBordenFlag;
  BOOLEAN bUpdataSubBoundaryFlag;
  BOOLEAN bWideChannelforParallelFlag;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  APACarCoordinateDataCalFloatType EndPos;
  APA_DISTANCE_TYPE LabelAngledDis;
#ifdef SUPPORT_PARKING_OUT_UWB
  APACoordinateDataCalFloatType RemoContPos;
#endif
  EndPos = APAMap_GInfo.SlotPar.EndPos;
  DefaultObj2PtX = 3000;
  DefaultObj1PtX = 3000;
  ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    DefaulBordenObj1 = 1000;
    DefaulBordenObj2 = 1000;
    if (TRUE == bLonggestSlotLen) {
      DefaulBordenObj3 = 200;
    } else {
      DefaulBordenObj3 = 250;
      if (TRUE == bShortSlotLen) {
        DefaulBordenObj3 = 300;
      }
      if (TRUE == bShortestSlotLen) {
        DefaulBordenObj3 = 400;
      }
    }
  } else {
    if (TRUE == bLabelAngledFlag) {
      if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
        DefaulBordenObj1 = 4000;
        DefaulBordenObj2 = 4000;
      } else {
        DefaulBordenObj1 = 1000;
        DefaulBordenObj2 = 1000;
      }
    } else {
      DefaulBordenObj1 = 2000;
      DefaulBordenObj2 = 2000;
    }
    DefaulBordenObj3 = 300;
  }
  bUpdataDefaulBordenFlag = FALSE;
  bUpdataSubBoundaryFlag = FALSE;
  bWideChannelforParallelFlag = FALSE;
  if (TRUE == bAfterNewAnchorPointFlag) //判断在锚点转换之后，且车辆已开出车位
  {
    if (bSlotDataAtRigthSide) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if (CurCarCoordinateX > -1) {
        bUpdataDefaulBordenFlag = TRUE;
      }
    } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
      if (CurCarCoordinateX > 0) {
        bUpdataDefaulBordenFlag = TRUE;
      }
    } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      if (CurCarCoordinateX > 0) {
        bUpdataDefaulBordenFlag = TRUE;
      } else {
        bUpdataSubBoundaryFlag = TRUE;
      }
    } else {
      if (CurCarCoordinateX > 0) {
        bUpdataDefaulBordenFlag = TRUE;
      }
    }
    //判断对向通道是否太窄，判断标准采用车辆到锚点的距离Cal值大小
    if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
        (MATH_FABS(EndPos.Coordinate.x) >
         (APAMap_ComCfg.HalfWidthOfCar + 900))) {
      bWideChannelforParallelFlag = TRUE;
    }
  }
  if (FALSE == bUpdataDefaulBordenFlag) //车在车位内
  {
    if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      DefaulBordenObj1 = 1000;
      if (TRUE == bLonggestSlotLen) {
        DefaulBordenObj2 = 0;
      } else {
        //大于0.9m（通道宽）则允许用保守内缩策略；小于0.9米（通道窄）则用激进的内缩策略。
        if (TRUE == bWideChannelforParallelFlag) {
          DefaulBordenObj2 = 0;
          if (TRUE == bShortSlotLen) {
            DefaulBordenObj2 = 100; // 50;
          }
          if (TRUE == bShortestSlotLen) {
            DefaulBordenObj2 = 200; // 100;
          }
        } else {
          if (FALSE == bShortestSlotLen) {
            DefaulBordenObj2 = 500;
          } else {
            DefaulBordenObj2 = 1000;
          }
        }
      }
    } else {
      if ((TRUE == bLabelAngledFlag) &&
          (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
        DefaulBordenObj1 = 2000;
        DefaulBordenObj2 = 2000;
      }
#if 0
      else if ((FALSE == bLabelAngledFlag)
        && (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND))
      {
        DefaulBordenObj1 = 0;
        DefaulBordenObj2 = 0;
      }
#endif
      else {
        DefaulBordenObj1 = 1000;
        DefaulBordenObj2 = 1000;
      }
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutCalBoundaryByParkOutInfo==DefaulBordenObj1(%."
             "2f),DefaulBordenObj2(%.2f)==bUpdataDefaulBordenFlag(%d)=="
             "CurCarCoordinateX(%.2f)",
             DefaulBordenObj1, DefaulBordenObj2, bUpdataDefaulBordenFlag,
             CurCarCoordinateX);
    TLOG_INFO << log_string;
  }
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt; // APA CorSys
  Obj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
  Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;
  Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;

  bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
  bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  MaxSlotPtX = APAMap_GInfo.SlotPar.slotCarEndPosXBackUp + DefaulBordenObj3;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  if (TRUE == bLabelAngledFlag) {
    LabelAngledDis = 1000;
  } else {
    LabelAngledDis = 0;
  }
  MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(5000 + APAMap_ComCfg.LengthOfCar +
                                       LabelAngledDis);
  MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
                                       APAMap_GInfo.SlotPar.SlotLen - 1000);
  if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    MaxDefaultRoadWith = 5000;
  } else {
    MaxDefaultRoadWith = 7000;
  }
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(12000 + APAMap_ComCfg.LengthOfCar +
                                         APAMap_ComCfg.LengthOfCar);
    MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
                                         APAMap_GInfo.SlotPar.SlotLen -
                                         APAMap_GInfo.SlotPar.SlotLen - 10000);
    if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
        (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
      RemoContPos.x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)
              APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x; // APA坐标系下
      RemoContPos.y = (APA_DISTANCE_CAL_FLOAT_TYPE)
                          APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y;
      RemoContPos = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          RemoContPos, 0, OrgAng, OrgPt); // APA转锚点坐标系下
      if (RemoContPos.x < -4000) {
        MaxDefaultRoadWith = (MATH_FABS(RemoContPos.x)) + 800;
      } else {
        MaxDefaultRoadWith = 12000;
      }
    }
  }
#endif
  MainBoudary.PtNum = 6;
  SubBoundary.PtNum = 2;
  Obj2Ang -= OrgAng;
  Obj1Ang -= OrgAng;
  AlgCom_AngNormalized(&Obj2Ang);
  AlgCom_AngNormalized(&Obj1Ang);
  if (bSlotDataAtRigthSide == FALSE) {
    Obj2Ang = -Obj2Ang;
    Obj1Ang = -Obj1Ang;
  }
  Obj2Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj2Pt, 0,
                                                               OrgAng, OrgPt);
  Obj1Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
                                                               OrgAng, OrgPt);
  if (bSlotDataAtRigthSide == FALSE) {
    Obj2Pt.x = -Obj2Pt.x;
    Obj1Pt.x = -Obj1Pt.x;
  }
  Obj2X = Obj2Pt.x;
  Obj1X = Obj1Pt.x;
  if (bObj2Exist == FALSE) {
    if (Obj2X > DefaultObj2PtX) {
      Obj2X = DefaultObj2PtX;
    }
  }
  if (bObj1Exist == FALSE) {
    if (Obj1X > DefaultObj1PtX) {
      Obj1X = DefaultObj1PtX;
    }
  }
  // 0
  MainBoudary.Points[0].x = Obj1X;
  MainBoudary.Points[0].y = MinY;
  MainBoudary.Points[0].x += DefaulBordenObj1 * MATH_SIN(Obj1Ang);
  MainBoudary.Property[0] = 0;

  // 1 2
  TempCarPos.Coordinate.x = Obj1Pt.x;
  TempCarPos.Coordinate.y = Obj1Pt.y;
  TempCarPos.CarAng = Obj1Ang;
  TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  if (TempLine.LineType == APALineIsHorizontal) {
    MainBoudary.Points[1].y = Obj1Pt.y;
    MainBoudary.Points[2].y = Obj1Pt.y;
  } else {
    MainBoudary.Points[1].y = Obj1X * TempLine.A + TempLine.C;
    MainBoudary.Points[2].y = MaxSlotPtX * TempLine.A + TempLine.C;
  }
  MainBoudary.Points[1].x = Obj1X;
  MainBoudary.Points[2].x = MaxSlotPtX;
  MainBoudary.Points[1].x += DefaulBordenObj1 * MATH_SIN(Obj1Ang);
  MainBoudary.Points[1].y -= DefaulBordenObj1 * MATH_COS(Obj1Ang);
  MainBoudary.Property[1] = 0;
  MainBoudary.Property[2] = 0;

  // 3 4
  TempCarPos.Coordinate.x = Obj2Pt.x;
  TempCarPos.Coordinate.y = Obj2Pt.y;
  TempCarPos.CarAng = Obj2Ang;
  TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  if (TempLine.LineType == APALineIsHorizontal) {
    MainBoudary.Points[3].y = Obj2Pt.y;
    MainBoudary.Points[4].y = Obj2Pt.y;
  } else {
    MainBoudary.Points[3].y = MaxSlotPtX * TempLine.A + TempLine.C;
    MainBoudary.Points[4].y = Obj2X * TempLine.A + TempLine.C;
  }
  MainBoudary.Points[3].x = MaxSlotPtX;
  MainBoudary.Points[4].x = Obj2X;
  MainBoudary.Points[4].x += DefaulBordenObj2 * MATH_SIN(Obj2Ang);
  MainBoudary.Points[4].y -= DefaulBordenObj2 * MATH_COS(Obj2Ang);
  MainBoudary.Property[3] = 0;
  MainBoudary.Property[4] = 0;

  // 5
  MainBoudary.Points[5].x = Obj2X;
  MainBoudary.Points[5].y = MaxY;
  MainBoudary.Points[5].x += DefaulBordenObj2 * MATH_SIN(Obj2Ang);
  MainBoudary.Property[5] = 0;

  for (i = 0; i < MainBoudary.PtNum; i++) {
    TempPt = MainBoudary.Points[i];
    if (bSlotDataAtRigthSide == FALSE) {
      TempPt.x = -TempPt.x;
    }
    MainBoudary.Points[i] =
        AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);
  }

  // 0 1
  SubBoundary.Points[0].x = -MaxDefaultRoadWith;
  SubBoundary.Points[0].y = MinY;
  SubBoundary.Property[0] = 0;
  SubBoundary.Points[1].x = -MaxDefaultRoadWith;
  SubBoundary.Points[1].y = MaxY;
  SubBoundary.Property[1] = 0;
  for (i = 0; i < SubBoundary.PtNum; i++) {
    TempPt = SubBoundary.Points[i];
    if (bSlotDataAtRigthSide == FALSE) {
      TempPt.x = -TempPt.x;
    }
    SubBoundary.Points[i] =
        AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);
  }

  if ((FALSE == bAfterNewAnchorPointFlag) || (TRUE == bUpdataSubBoundaryFlag)) {
    if (bSlotDataAtRigthSide == TRUE) {
      APAMap_GInfo.OutLine.LeftBoundary = SubBoundary;
      APAMap_GInfo.OutLine.RightBoundary = MainBoudary;
    } else {
      APAMap_GInfo.OutLine.LeftBoundary = MainBoudary;
      APAMap_GInfo.OutLine.RightBoundary = SubBoundary;
    }
  } else //锚点转换之后不再重置子边界，但需要重置主边界
  {
    if (bSlotDataAtRigthSide == TRUE) {
      APAMap_GInfo.OutLine.RightBoundary = MainBoudary;
    } else {
      APAMap_GInfo.OutLine.LeftBoundary = MainBoudary;
    }
  }
  APAMap_GInfo.SlotPar.SlotStrIndex = 2;
  APAMap_GInfo.SlotPar.SlotEndIndex = 3;
  APAMap_GInfo.SlotPar.Obj1PtIndex = 1;
  APAMap_GInfo.SlotPar.Obj2PtIndex = 4;
  APAMap_GInfo.SlotPar.SlotBordPt[2] = MainBoudary.Points[2];
  APAMap_GInfo.SlotPar.SlotBordPt[3] = MainBoudary.Points[3];
  return TRUE;
}

---

## 🤖 ChatGPT

下面继续按**维护视角**拆 `APAMap_ParkingOutCalBoundaryByParkOutInfo()`。


这个函数非常关键，因为它是 `APAMap_ParkingOutCalMapInfo()` 的第一步，职责可以概括成一句话：



>
> **根据当前车位骨架，先构造一版“主边界 + 子边界”的默认地图。**
>
>
>

也就是说，它不是做多源融合，而是先把**基础通道形状**搭出来，后面的 FSD / LaneLine / RefercLine / OD 都是在这版基础边界上继续修。




# 1. 这个函数在整条链路中的定位

主链路里它的位置是：



```
C++APAMap_ParkingOutCalMapInfo()    -> APAMap_ParkingOutCalBoundaryByParkOutInfo()    -> APAMap_ParkingOutFusBoundaryByFSDMapInfo()    -> APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()    -> APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()    -> APAMap_FusBoundaryByODMapInfo()
```

所以这个函数生成的是：



```
边界初稿
```

后面函数做的是：



```
边界修稿
```



# 2. 先说它最终产出什么

这个函数最重要的输出是：



```
C++APAMap_GInfo.OutLine.LeftBoundaryAPAMap_GInfo.OutLine.RightBoundary
```

以及 4 个索引：



```
C++APAMap_GInfo.SlotPar.SlotStrIndex = 2;APAMap_GInfo.SlotPar.SlotEndIndex = 3;APAMap_GInfo.SlotPar.Obj1PtIndex = 1;APAMap_GInfo.SlotPar.Obj2PtIndex = 4;
```

还有两点：



```
C++APAMap_GInfo.SlotPar.SlotBordPt[2]APAMap_GInfo.SlotPar.SlotBordPt[3]
```

所以它的本质是：



>
> **把车位两条边界点 Obj1/Obj2，扩展成一条完整主边界折线和一条对侧子边界直线。**
>
>
>



# 3. 主流程骨架

这个函数可以拆成 7 步：



```
① 根据车位类型/长短/斜列等状态，确定默认边界收缩量② 判断当前车是否已驶出车位，决定是否更新默认边界③ 读取 Obj1/Obj2、角度和新坐标系④ 把 Obj1/Obj2 转到锚点局部坐标系并统一左右语义⑤ 构造主边界 6 个点⑥ 构造子边界 2 个点⑦ 再转回全局坐标，并写入 LeftBoundary / RightBoundary
```



# 4. Step 1：确定默认边界参数

这部分最先出现的是：



```
C++DefaulBordenObj1DefaulBordenObj2DefaulBordenObj3
```

这 3 个量是整个函数的核心调节参数。




## 4.1 它们分别表示什么

从代码用途看：



### `DefaulBordenObj1`

主边界靠近 Obj1 那一侧的默认外扩/内缩量。



### `DefaulBordenObj2`

主边界靠近 Obj2 那一侧的默认外扩/内缩量。



### `DefaulBordenObj3`

沿车位开口方向（X 方向）的额外延展量，用来确定边界中间上方那两个点能延展到多远。




## 4.2 平行泊出时的默认值


```
C++if (ParkOutMode == PARALLEL) {  DefaulBordenObj1 = 1000;  DefaulBordenObj2 = 1000;  if (TRUE == bLonggestSlotLen) {    DefaulBordenObj3 = 200;  } else {    DefaulBordenObj3 = 250;    if (TRUE == bShortSlotLen) {      DefaulBordenObj3 = 300;    }    if (TRUE == bShortestSlotLen) {      DefaulBordenObj3 = 400;    }  }}
```


### 维护理解

平行车位里：




- 
左右两侧初始边界默认都收 `1000`




- 
但沿出口方向的延展量 `Obj3` 会随着车位变短而变大





也就是：



>
> 车位越短，边界越保守，越要给出口方向多留一点冗余。
>
>
>



## 4.3 非平行泊出时的默认值


```
C++else {  if (TRUE == bLabelAngledFlag) {    if (ParkOutMode == REAR_TURN_ROUND) {      DefaulBordenObj1 = 4000;      DefaulBordenObj2 = 4000;    } else {      DefaulBordenObj1 = 1000;      DefaulBordenObj2 = 1000;    }  } else {    DefaulBordenObj1 = 2000;    DefaulBordenObj2 = 2000;  }  DefaulBordenObj3 = 300;}
```


### 维护理解

非平行时整体更保守：




- 
普通非平行：两侧默认 `2000`




- 
斜列倒车转出：甚至给到 `4000`





说明作者认为这些场景的通道风险更大，需要更宽松的默认边界。




# 5. Step 2：判断车辆是否已驶出车位


```
C++bUpdataDefaulBordenFlag = FALSE;bUpdataSubBoundaryFlag = FALSE;bWideChannelforParallelFlag = FALSE;if (TRUE == bAfterNewAnchorPointFlag) {    ...}
```

这段逻辑非常工程化，但非常重要。




## 5.1 `bAfterNewAnchorPointFlag`

这代表：



>
> 已经发生锚点转换，而且车辆处于“车位外”阶段。
>
>
>

一旦进入这个阶段，边界策略会发生变化。




## 5.2 `bUpdataDefaulBordenFlag`

这个标志控制：



>
> 是否要更新默认主边界策略。
>
>
>

比如：




- 
平行泊出且 `CurCarCoordinateX > 0` 时置真




- 
头出/尾出模式也有各自阈值






### 直白理解

如果车已经开出来了，就不能再按“车还在车位里”的默认边界来画主边界了。




## 5.3 `bUpdataSubBoundaryFlag`

仅在平行场景下特殊处理：



```
C++else if (ParkOutMode == PARALLEL) {  if (CurCarCoordinateX > 0) {    bUpdataDefaulBordenFlag = TRUE;  } else {    bUpdataSubBoundaryFlag = TRUE;  }}
```


### 含义

平行车位里，如果锚点转换后车还没明显越过某阈值，就先只更新子边界，不急着重画主边界。



### 维护理解

这是一种“渐进式边界更新”策略，避免锚点切换瞬间边界剧烈跳变。




## 5.4 `bWideChannelforParallelFlag`


```
C++if ((ParkOutMode == PARALLEL) &&    (fabs(EndPos.Coordinate.x) > (HalfWidthOfCar + 900))) {  bWideChannelforParallelFlag = TRUE;}
```


### 含义

判断对向通道是否比较宽。



### 工程意义

后面在平行泊出里会根据这个标志决定：




- 
用更保守的内缩




- 
还是用更激进的内缩







# 6. Step 3：根据车辆是否仍在车位内，二次调整默认边界参数

这是第二轮调参。



```
C++if (FALSE == bUpdataDefaulBordenFlag) {   ...}
```

也就是：



>
> 如果车还在车位内，重新收紧默认边界策略。
>
>
>



## 6.1 平行车位里的策略最复杂


```
C++if (ParkOutMode == PARALLEL) {  DefaulBordenObj1 = 1000;  if (TRUE == bLonggestSlotLen) {    DefaulBordenObj2 = 0;  } else {    if (TRUE == bWideChannelforParallelFlag) {      DefaulBordenObj2 = 0;      if (TRUE == bShortSlotLen) {        DefaulBordenObj2 = 100;      }      if (TRUE == bShortestSlotLen) {        DefaulBordenObj2 = 200;      }    } else {      if (FALSE == bShortestSlotLen) {        DefaulBordenObj2 = 500;      } else {        DefaulBordenObj2 = 1000;      }    }  }}
```


### 维护理解

平行泊出的边界策略受 3 个因素影响：




- 
车位长短




- 
通道宽不宽




- 
车是否还在车位内





这说明平行场景是作者重点做过工程调优的场景。




## 6.2 非平行车位

非平行场景下规则简单很多，通常就是：




- 
普通：`1000`




- 
斜列倒车转出：`2000`







## 6.3 日志

这里有一条很关键的日志：



```
C++==APAMap_ParkingOutCalBoundaryByParkOutInfo==DefaulBordenObj1(...)DefaulBordenObj2(...)bUpdataDefaulBordenFlag(...)CurCarCoordinateX(...)
```


### 维护价值很高

以后如果感觉“初始边界画得太宽/太窄”，先看这条日志就知道：




- 
当前模式下边界策略用了哪组参数




- 
是否已经进入车位外阶段







# 7. Step 4：读取当前车位骨架


```
C++Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;Obj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;MaxSlotPtX = APAMap_GInfo.SlotPar.slotCarEndPosXBackUp + DefaulBordenObj3;OrgPt = APAMap_GInfo.NewCordSysOPt;OrgAng = APAMap_GInfo.NewCordSysAng;
```


### 作用

把前面步骤算好的车位关键点、角度、坐标系原点、边界延展量拿出来。



### `MaxSlotPtX`

非常关键：



```
C++slotCarEndPosXBackUp + DefaulBordenObj3
```

也就是：




- 
前面车位几何里算出来的最大可用 X




- 
再加一个默认冗余量





这个值后面决定主边界中部两点能延到哪里。




# 8. Step 5：确定边界整体上下范围


```
C++if (TRUE == bLabelAngledFlag) {  LabelAngledDis = 1000;} else {  LabelAngledDis = 0;}MaxY = 5000 + LengthOfCar + LabelAngledDis;MinY = -LengthOfCar - SlotLen - 1000;
```


### 含义

在局部坐标系里，先给边界确定一个纵向范围：




- 
`MaxY`：车前方通道延展




- 
`MinY`：车位后方/车位内部延展






### 维护理解

这不是边界精确位置，而是一个“搜索盒子/边界包络框”的上下边界。




## 8.1 `MaxDefaultRoadWith`


```
C++if (ParkOutMode == PARALLEL) {  MaxDefaultRoadWith = 5000;} else {  MaxDefaultRoadWith = 7000;}
```


### 含义

对子边界那条直线的默认横向宽度。



### 工程意义

非平行场景默认给更宽的对向通道。




## 8.2 UWB 场景下会大幅放宽范围

如果启用 UWB，`MaxY / MinY / MaxDefaultRoadWith` 都可能被放大。



### 维护理解

这意味着带远程控制/UWB场景，边界不再是“近距离泊出边界”，而是更大范围的出库通道。




# 9. Step 6：把 Obj1/Obj2 转到局部坐标系，并统一左右语义


```
C++Obj2Ang -= OrgAng;Obj1Ang -= OrgAng;AlgCom_AngNormalized(&Obj2Ang);AlgCom_AngNormalized(&Obj1Ang);if (bSlotDataAtRigthSide == FALSE) {  Obj2Ang = -Obj2Ang;  Obj1Ang = -Obj1Ang;}Obj2Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj2Pt, 0, OrgAng, OrgPt);Obj1Pt = ...if (bSlotDataAtRigthSide == FALSE) {  Obj2Pt.x = -Obj2Pt.x;  Obj1Pt.x = -Obj1Pt.x;}
```


### 作用

这一步非常关键：



>
> 把原本在 APA 坐标系中的 Obj1/Obj2，转换到“以当前车位为原点、并统一右侧语义”的局部坐标系。
>
>
>


### 维护理解

后面所有边界点计算都在这个统一后的坐标系里做。


这样做的好处是：




- 
左右停车用同一套公式




- 
只要最后再映射回全局就行







# 10. Step 7：如果 Obj1/Obj2 不存在，就做默认截断


```
C++Obj2X = Obj2Pt.x;Obj1X = Obj1Pt.x;if (bObj2Exist == FALSE) {  if (Obj2X > DefaultObj2PtX) {    Obj2X = DefaultObj2PtX;  }}if (bObj1Exist == FALSE) {  if (Obj1X > DefaultObj1PtX) {    Obj1X = DefaultObj1PtX;  }}
```


### 含义

如果某侧边界点并不真实存在，就不要让它离得太远，限制到默认值 `3000`。



### 维护理解

这是一种缺失信息时的兜底：




- 
有真实 Obj 点 → 用真实值




- 
没有 → 裁到保守默认值







# 11. Step 8：构造主边界 `MainBoudary`

这部分是函数最核心的几何构造。




## 11.1 主边界总共有 6 个点


```
C++MainBoudary.PtNum = 6;
```

含义上可以这样理解：



```
0 —— Obj1下延点1 —— Obj1点附近2 —— 主边界中部延展点3 —— 主边界中部另一点4 —— Obj2点附近5 —— Obj2上延点
```

也就是一条从车位一侧边界延到通道上方的折线。




## 11.2 点 0


```
C++MainBoudary.Points[0].x = Obj1X;MainBoudary.Points[0].y = MinY;MainBoudary.Points[0].x += DefaulBordenObj1 * sin(Obj1Ang);
```


### 含义

这是主边界最底部的起点，沿 Obj1 一侧向后延。




## 11.3 点 1、2

通过 Obj1 方向线来构造：



```
C++TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);...MainBoudary.Points[1]MainBoudary.Points[2]
```


### 含义



- 
点1：靠近 Obj1 的边界点




- 
点2：向车位出口方向延展后的边界点







## 11.4 点 3、4

同理，用 Obj2 的方向线构造：




- 
点3：中部延展点




- 
点4：靠近 Obj2 的边界点







## 11.5 点 5


```
C++MainBoudary.Points[5].x = Obj2X;MainBoudary.Points[5].y = MaxY;
```


### 含义

主边界最上方的终点，沿 Obj2 一侧向前延。




## 11.6 这 6 个点的本质

主边界不是简单矩形，而是一条：



>
> **沿 Obj1 侧线 → 向车位出口方向延展 → 沿 Obj2 侧线回收** 的折线。
>
>
>

这很适合描述泊出通道的一侧边缘。




# 12. Step 9：把主边界点从局部坐标系转回全局


```
C++for (i = 0; i < MainBoudary.PtNum; i++) {  TempPt = MainBoudary.Points[i];  if (bSlotDataAtRigthSide == FALSE) {    TempPt.x = -TempPt.x;  }  MainBoudary.Points[i] =      AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);}
```


### 作用

边界构造时在统一局部坐标系里做，最后再变回全局。



### 维护理解

如果边界形状本身合理，但投回全局后方向反了，重点看这里：




- 
`bSlotDataAtRigthSide`




- 
`OrgAng`




- 
`OrgPt`







# 13. Step 10：构造子边界 `SubBoundary`


```
C++SubBoundary.PtNum = 2;SubBoundary.Points[0].x = -MaxDefaultRoadWith;SubBoundary.Points[0].y = MinY;SubBoundary.Points[1].x = -MaxDefaultRoadWith;SubBoundary.Points[1].y = MaxY;
```


### 含义

子边界非常简单，就是局部坐标系中的一条竖直直线。



### 它代表什么

通常代表：




- 
对向通道边界




- 
与主边界相对的另一侧保守边界






### 为什么只要两个点

因为它是一条简单直线，作为默认通道另一侧边界已经够用。




# 14. Step 11：决定 LeftBoundary 和 RightBoundary 分配给谁

这是输出阶段最重要的一步。




## 11.1 正常情况


```
C++if ((FALSE == bAfterNewAnchorPointFlag) || (TRUE == bUpdataSubBoundaryFlag)) {  if (bSlotDataAtRigthSide == TRUE) {    LeftBoundary = SubBoundary;    RightBoundary = MainBoudary;  } else {    LeftBoundary = MainBoudary;    RightBoundary = SubBoundary;  }}
```


### 含义

根据车位在左还是右，把主边界和子边界分别赋给左右边界。



### 维护理解

主边界永远是“贴着车位那一侧”的边界，子边界永远是“对向通道那一侧”的边界。




## 11.2 锚点转换后特殊情况


```
C++else {  if (bSlotDataAtRigthSide == TRUE) {    RightBoundary = MainBoudary;  } else {    LeftBoundary = MainBoudary;  }}
```


### 含义

锚点转换之后，不再重置子边界，只更新主边界。



### 维护理解

这是为了让对向通道那一侧边界保持连续，避免锚点切换导致两侧都跳变。




# 15. Step 12：设置关键索引


```
C++APAMap_GInfo.SlotPar.SlotStrIndex = 2;APAMap_GInfo.SlotPar.SlotEndIndex = 3;APAMap_GInfo.SlotPar.Obj1PtIndex = 1;APAMap_GInfo.SlotPar.Obj2PtIndex = 4;APAMap_GInfo.SlotPar.SlotBordPt[2] = MainBoudary.Points[2];APAMap_GInfo.SlotPar.SlotBordPt[3] = MainBoudary.Points[3];
```


### 含义

这里固定了主边界中几个特殊点的语义：




- 
`Obj1PtIndex = 1`




- 
`SlotStrIndex = 2`




- 
`SlotEndIndex = 3`




- 
`Obj2PtIndex = 4`





也就是说，后续融合函数默认知道：




- 
点1是 Obj1 对应点




- 
点4是 Obj2 对应点




- 
点2/3是车位出口边界段






### 维护理解

这些索引非常关键。

后面的：




- 
FSD 融合




- 
LaneLine 融合




- 
RefercLine 融合





很多都默认依赖这几个索引。


如果以后改了主边界点定义，这里必须同步改，不然整个边界融合链会错位。




# 16. 这个函数的维护版一句话总结

`APAMap_ParkingOutCalBoundaryByParkOutInfo()` 的本质是：



>
> **把当前车位的 Obj1/Obj2 和局部坐标系扩展成一条 6 点主边界和一条 2 点子边界，作为后续所有边界融合的初始地图。**
>
>
>



# 17. 维护时最该盯的 6 个问题


## 1）为什么初始边界太宽/太窄

先看：




- 
`DefaulBordenObj1`




- 
`DefaulBordenObj2`




- 
`DefaulBordenObj3`






## 2）为什么锚点切换后边界跳变

看：




- 
`bAfterNewAnchorPointFlag`




- 
`bUpdataDefaulBordenFlag`




- 
`bUpdataSubBoundaryFlag`






## 3）为什么左右边界放反了

看：




- 
`bSlotDataAtRigthSide`




- 
最后 Left/Right 的赋值逻辑






## 4）为什么边界方向是对的，但位置整体偏了

看：




- 
`OrgPt`




- 
`OrgAng`




- 
局部坐标系转回全局那段






## 5）为什么某些融合函数后面报索引相关问题

看：




- 
`SlotStrIndex`




- 
`SlotEndIndex`




- 
`Obj1PtIndex`




- 
`Obj2PtIndex`






## 6）为什么 Obj 点不存在时边界怪异

看：




- 
`bObj1Exist / bObj2Exist`




- 
`DefaultObj1PtX / DefaultObj2PtX`







# 18. 到这里，已经能做什么

现在已经可以比较系统地理解：



```
车位骨架怎么来车位如何被 FSD/OD 修正默认地图边界怎么搭后面融合为什么依赖这些索引
```

也就是说，主干已经够支撑后续维护和排障。




# 19. 下一步建议

接下来有两种最有价值的方向：



### 方向 A：继续拆“边界融合函数”

优先建议下一个看：




- 
`APAMap_ParkingOutFusBoundaryByFSDMapInfo()`





因为它是初始边界之后的第一层融合，也是最重要的一层。



### 方向 B：我直接开始帮你整理“维护手册”

把到目前为止的内容整理成：




- 
主流程图




- 
全局关键变量表




- 
关键函数职责表




- 
failcause 对照表




- 
关键日志对照表




- 
排障顺序建议





如果目标是后续实际维护，我更建议现在开始做这份手册。

---
