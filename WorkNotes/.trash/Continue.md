### [Continue](https://continue.dev) session transcript
 Exported: 4/28/2026, 9:45:55 AM

#### _User_

> 你好

#### _Assistant_

> 你好！有什么我可以帮你的吗？

#### _User_

> 解析这个代码：#include "MapParkingOut.h"
> #include "APAMapCfg.h"
> #include "AlgCom.h"
> #include "Map.h"
> #include "MapType.h"
> #include "Map_DeadendScenario_Decider.h"
> #include "common/log_wrap.h"
> #include "data_exchange/someip/planning_data_interface.h"
> #include "stdio.h"
> #ifdef SUPPORT_PARKING_OUT_SYSTEM
> BOOLEAN bCntAddFlag = FALSE;  // cnt+1轨迹重算标志位，下一帧会清空FALSE
> BOOLEAN bLaneLineUpdateEndCarPosFlag = FALSE;  // 车道线更新终点位置标志位
> BOOLEAN bRefercLineUpdateEndCarPosFlag = FALSE;  // 车位参考线更新终点位置标志位
> BOOLEAN bAfterNewAnchorPointFlag = FALSE;    // 锚点转换后标志位
> BOOLEAN bFsdInRightOfEndCarPosFlag = FALSE;  // FSD点位入侵终点位置右边标志位
> BOOLEAN bFsdFromMapMainSlotBordFlag = FALSE;  // 入侵的边界点是否来自主边界标志位
> BOOLEAN bFsdFromMapSubSlotBordFlag = FALSE;  // 入侵的边界点是否来自子边界标志位
> BOOLEAN bFsdFromMapMainAndSubSlotBordFlag =
> FALSE;  // 入侵的边界点是否来自主子边界标志位
> BOOLEAN bPreventStepNRedundantFlag = FALSE;  // 防多走标志位
> BOOLEAN bShortestSlotLen = FALSE;            // 水平极小车位标志位
> BOOLEAN bShortSlotLen = FALSE;               // 水平小车位标志位
> BOOLEAN bLonggestSlotLen = FALSE;            // 水平极大车位标志位
> BOOLEAN bCarryOutSlot = FALSE;               // 采用车位框标志位
> BOOLEAN bLabelAngledFlag = FALSE;            // 斜列车位框标志位
> BOOLEAN bObjLabelLadderFlag = FALSE;         // 斜列阶梯车位框标志位
> BOOLEAN bLabelAngledParkingOutSlotFlag = FALSE;  // 斜列车位泊出车位后标志位
> BOOLEAN bODWheelChockFlag = FALSE;  // 水平泊出车位内有轮挡标志位
> tMap_MapBkInfo_BeForeFusSDG_t APAMap_BkDataBfSDGFus;
> tMap_MapBkInfo_SDGBkOutPutData_t APAMap_BkSDGOutPutData;
> tAPAParkProcEightParkingOutModeType ParkOutEightMode;
> void APAMap_ParkingOutTask() {
> BOOLEAN bResult;
> if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 4) &&
> (APAMap_GInputData.ParkReqPar.Request_cmd == 1) &&
> (APAMap_GInputData.ParkReqPar.request_cnt == APAMap_GInfo.lastreqcnt)) {
> APAMap_ParkingOutUpDataMapInfo();
> return;
> }
> {
> if ((APAMap_GInputData.ParkReqPar.Request_cmd == 1) ||
> (APAMap_GInputData.ParkReqPar.Request_cmd == 6)) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==First APAMapParkout Build cmd(%d)===request_cnt(%lld)==",
> APAMap_GInputData.ParkReqPar.Request_cmd,
> APAMap_GInputData.ParkReqPar.request_cnt);
> TLOG_INFO << log_string;
> }
> APAMap_GInfo.calcnt++;
> if ((APAMap_GInputData.ParkReqPar.APAstate <= 3) &&
> (APAMap_GInputData.ParkReqPar.APARunningstate >= 1)) {
> APAMap_ParkingOutDebugInit();
> }
> // APAMap_GInfo.status = APAMapStatus_BUSY;
> bResult = APAMap_ParkingOutCalMapSlotPar();
> if (bResult) {
> bResult = APAMap_ParkingOutCalSlotInfo();
> if (bResult) {
> bResult = APAMap_ParkingOutCalMapInfo();
> if (bResult == FALSE) {
> APAMAP_Setfailcause(45);
> } else {
> bResult = APAMap_ParkingOutCheckIfCarPosIsValid();
> }
> }
> }
> if (bResult == FALSE) {
> APAMap_GInfo.failcalcnt++;
> }
> APAMap_GInfo.bCalResult = bResult;
> // APAMap_GInfo.status = APAMapStatus_CALFINISHED;
> } else {
> if ((APAMap_GInfo.OutLine.LeftBoundary.PtNum < 2) ||
> (APAMap_GInfo.OutLine.RightBoundary.PtNum < 2)) {
> TLOG_DEBUG << "start APAMAP_Setfailcause(59)...";
> APAMap_DataInit();
> APAMap_ParkingOutDebugInit();
> APAMAP_Setfailcause(59);
> return;
> }
> APAMap_ParkingOutUpDataMapInfo();
> APAMap_GInfo.bCalResult = TRUE;
> }
> }
> if (APAMap_GInputData.ParkReqPar.APARunningstate >= 7) {
> APAMap_ParkingOutDebugInit();
> }
> return;
> // APAMap_SetOutputData();
> }
> BOOLEAN APAMap_ParkingOutCalMapSlotPar() {
> APA_ENUM_TYPE ParkSide;
> uint16_t_INF SlotID;
> APA_ENUM_TYPE SlotSide;
> // uint8_t_INF ParkMode;
> uint8_t_INF FusionMode;
> uint8_t_INF SlotIndex;
> uint8_t_INF FusSlotIndex;
> uint8_t_INF i;
> BOOLEAN bResult;
> BOOLEAN bSlotDataAtRigthSide;
> uint8_t_INF ParkOutMode;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> SlotID = APAMap_GInputData.ParkReqPar.Request_SlotId;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> FusionMode = APAMap_GInputData.ParkReqPar.Request_SlotFusionMode;
> FusSlotIndex = MAP_SLOT_INVALID_INDEX;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
> bSlotDataAtRigthSide = TRUE;
> } else {
> bSlotDataAtRigthSide = FALSE;
> }
> } else {
> if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
> bSlotDataAtRigthSide = FALSE;
> } else {
> bSlotDataAtRigthSide = TRUE;
> }
> }
> } else {
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
> bSlotDataAtRigthSide = FALSE;
> } else {
> bSlotDataAtRigthSide = TRUE;
> }
> } else {
> if (APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored == TRUE) {
> bSlotDataAtRigthSide = TRUE;
> } else {
> bSlotDataAtRigthSide = FALSE;
> }
> }
> }
> for (i = 0; i < 2; i++) {
> APAMap_GInfo.SlotPar.VplPt[i].x = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.VplPt[i].y = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.UsPt[i].x = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.UsPt[i].y = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.ODPt[i].x = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.ODPt[i].y = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.FSDPt[i].x = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.FSDPt[i].y = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.PAPt[i].x = NO_OBJ_DISTANCE;
> APAMap_GInfo.SlotPar.PAPt[i].y = NO_OBJ_DISTANCE;
> }
> APAMap_GInfo.SlotPar.bSlotDataAtRigthSide = bSlotDataAtRigthSide;
> if ((APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
> (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKEXIT)) {
> APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
> APAMap_GInfo.SlotPar.SlotID = SlotID;
> APAMap_GInfo.SlotPar.SlotIndex = 0;
> APAMap_GInfo.SlotPar.FusSlotIndex = 0;
> return TRUE;
> }
> if (SlotID == APA_VPL_SLOT_PROC_INVALID_SLOT_ID) {
> APAMAP_Setfailcause(1);
> return FALSE;
> }
> if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
> SlotSide = 0;
> } else {
> SlotSide = 1;
> }
> bResult = FALSE;
> if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {
> for (SlotIndex = 0; SlotIndex < APA_SLOT_SUPPORT_MAX_SLOT_NUM;
> SlotIndex++) {
> if (APAMap_GInputData.Usslot.USSlot[SlotSide].SlotPar[SlotIndex].SlotID ==
> SlotID) {
> bResult = TRUE;
> break;
> }
> }
> } else {
> for (SlotIndex = 0; SlotIndex < APA_VPL_SLOT_PROC_MAX_VPL_SLOT_NUM;
> SlotIndex++) {
> if (APAMap_GInputData.Vplslot.VPLSlot[SlotSide].Slot[SlotIndex].SlotID ==
> SlotID) {
> bResult = TRUE;
> break;
> }
> }
> if (FusionMode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_VPLSLOT) {
> } else {
> bResult = FALSE;
> for (FusSlotIndex = 0;
> FusSlotIndex <
> APAMap_GInputData.FusSlot.FusionSlot[SlotSide].SlotNum;
> FusSlotIndex++) {
> if ((APAMap_GInputData.FusSlot.FusionSlot[SlotSide]
> .Slot[FusSlotIndex]
> .FusedByVPLSlotID == SlotID) &&
> (APAMap_GInputData.FusSlot.FusionSlot[SlotSide]
> .Slot[FusSlotIndex]
> .FusionMode == FusionMode)) {
> bResult = TRUE;
> break;
> }
> }
> }
> }
> if (bResult == FALSE) {
> APAMAP_Setfailcause(2);
> return FALSE;
> }
> if (APAMap_GInputData.ParkReqPar.APARunningstate == 0) {
> APAMAP_Resetlastreqcnt();
> }
> APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
> APAMap_GInfo.SlotPar.SlotID = SlotID;
> APAMap_GInfo.SlotPar.SlotSide = SlotSide;
> APAMap_GInfo.SlotPar.SlotIndex = SlotIndex;
> APAMap_GInfo.SlotPar.FusSlotIndex = FusSlotIndex;
> return TRUE;
> }
> BOOLEAN APAMap_ParkingOutCalSlotInfo() {
> BOOLEAN bResult;
> #ifdef APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO
> bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();
> #else
> bResult = FALSE;
> #endif
> if (FALSE == bResult) {
> bResult = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();
> }
> #if 1
> if (bResult) {
> bResult = APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
> debug1++;
> }
> #endif
> return bResult;
> }
> void APAMap_CalSlotBorderPtOffsetBySensorMapInfo(
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
> APAMAP_GetSlotBdPtBySensorObjs(0, pOffsetX1, pOffsetY1);
> APAMAP_GetSlotBdPtBySensorObjs(1, pOffsetX2, pOffsetY2);
> return;
> }
> /**
> * @brief 根据FSD和OD地图信息计算停车位边界点
> * @return BOOLEAN 计算成功返回TRUE，失败返回FALSE
> */
> BOOLEAN APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo(void) {
> APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;  // 停车位边界点坐标
> // 声明变量：停车位长度
> APA_DISTANCE_CAL_FLOAT_TYPE SlotLen;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1, fDis2;     // 距离参数
> APALineParameterABCType EndPosLine;           // 停车位终点线参数
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultOffsetY1;  // 默认Y方向偏移量1
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultOffsetY2;  // 默认Y方向偏移量2
> APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetX1;      // FSD X方向偏移量1
> APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetY1;
> APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetX2;  // FSD X方向偏移量2
> APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetY2;  // FSD Y方向偏移量2
> APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetX1;   // OD X方向偏移量1
> APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetY1;   // OD Y方向偏移量1
> APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetX2;   // OD X方向偏移量2
> APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetY2;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetX1;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetY1;
> // 声明变量：综合偏移量X1和Y1
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetX2;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetY2;
> APA_DISTANCE_CAL_FLOAT_TYPE NewDis1, NewDis2;
> APA_DISTANCE_CAL_FLOAT_TYPE NewDis;
> APALineParameterABCType Line;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDis;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> APACarCoordinateDataCalFloatType TempCarPos1, TempCarPos2;
> APALineParameterABCType TempLine1, TempLine2;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType OrgPt;
> APACarCoordinateDataCalFloatType EndPos;
> BOOLEAN bSlotDataAtRigthSide;
> uint8_t_INF ParkOutMode;
> APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetX1;
> APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetY1;
> APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetX2;
> APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetY2;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
> APACoordinateDataCalFloatType TempPt;
> BOOLEAN bUpdataDefaulBordenFlag;
> APA_DISTANCE_CAL_FLOAT_TYPE BloundaryOffsetY;
> BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位
> // 检查是否为停车出库模式
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> // return TRUE;
> }
> bSeizeEndCarPosFlag = FALSE;  // 初始化FSD侵占终点位置标志位为FALSE
> SafeDis = 250;                // 设置安全距离为250mm
> // 获取停车位边界点长度
> Obj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];  // APAMap_GInfo.SlotPar.Obj2Pt;
> Obj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];  // APAMap_GInfo.SlotPar.Obj1Pt;
> SlotLen = APAMap_GInfo.SlotPar.SlotLen;
> EndPos = APAMap_GInfo.SlotPar.EndPos;
> EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> BloundaryOffsetY = 0;
> // 根据传感器地图信息计算停车位边界点偏移量
> APAMap_CalSlotBorderPtOffsetBySensorMapInfo(&SensorOffsetX1, &SensorOffsetY1,
> &SensorOffsetX2, &SensorOffsetY2);
> #if 1
> // 根据顶部视角FSD地图信息计算停车位边界点偏移量
> APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(&FSDOffsetX1, &FSDOffsetY1,
> &FSDOffsetX2, &FSDOffsetY2);
> #else
> FSDOffsetX1 = 0;
> FSDOffsetY1 = 0;
> FSDOffsetX2 = 0;
> FSDOffsetY2 = 0;
> #endif
> // 初始化综合偏移量为FSD偏移量
> OffsetX1 = FSDOffsetX1;
> OffsetY1 = FSDOffsetY1;
> OffsetX2 = FSDOffsetX2;
> OffsetY2 = FSDOffsetY2;
> // 确保偏移量不小于传感器偏移量
> if (OffsetX1 < SensorOffsetX1) {
> OffsetX1 = SensorOffsetX1;
> }
> if (OffsetX2 < SensorOffsetX2) {
> OffsetX2 = SensorOffsetX2;
> }
> if (OffsetY1 < SensorOffsetY1) {
> OffsetY1 = SensorOffsetY1;
> }
> if (OffsetY2 < SensorOffsetY2) {
> OffsetY2 = SensorOffsetY2;
> }
> #if 1
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> // 使用总地图目标计算OD偏移量
> APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(&ODOffsetX1, &ODOffsetY1,
> &ODOffsetX2, &ODOffsetY2);
> #else
> // 根据OD地图信息计算停车位边界点偏移量
> APAMap_CalSlotBorderPtOffsetByODMapInfo(&ODOffsetX1, &ODOffsetY1, &ODOffsetX2,
> &ODOffsetY2);
> #endif
> // 确保OD偏移量不小于50mm
> if (ODOffsetX1 < 50) {
> ODOffsetX1 = 0;
> }
> if (ODOffsetY1 < 50) {
> ODOffsetY1 = 0;
> }
> if (ODOffsetX2 < 50) {
> ODOffsetX2 = 0;
> }
> if (ODOffsetY2 < 50) {
> ODOffsetY2 = 0;
> }
> #else
> ODOffsetX1 = 0;
> ODOffsetY1 = 0;
> ODOffsetX2 = 0;
> ODOffsetY2 = 0;
> #endif
> // 更新综合偏移量，确保不小于OD偏移量
> if (OffsetX1 < ODOffsetX1) {
> OffsetX1 = ODOffsetX1;
> }
> if (OffsetX2 < ODOffsetX2) {
> OffsetX2 = ODOffsetX2;
> }
> if (OffsetY1 < ODOffsetY1) {
> OffsetY1 = ODOffsetY1;
> }
> if (OffsetY2 < ODOffsetY2) {
> OffsetY2 = ODOffsetY2;
> }
> // zqf:Mix FSD && OD Data Obj1/2
> if (TRUE == bCarryOutSlot) {
> fDis2 = APAMap_GetSearchMaxInnerY(1, bSlotDataAtRigthSide, Obj2Pt,
> APAMap_GInfo.SlotPar.Obj2Ang);
> fDis1 = APAMap_GetSearchMaxInnerY(0, bSlotDataAtRigthSide, Obj1Pt,
> APAMap_GInfo.SlotPar.Obj1Ang);
> } else {
> Line = APAMAP_GetSlotLineByCarPos();
> TempDis1 = APAMap_GetDisByCarPosToBumper(1);
> fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, Line) - TempDis1;
> TempDis1 = APAMap_GetDisByCarPosToBumper(0);
> fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, Line) - TempDis1;
> }
> bUpdataDefaulBordenFlag = FALSE;
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -1) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (CurCarCoordinateX > 2) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> } else {
> if (CurCarCoordinateX > 0) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> }
> }
> if (TRUE == bLabelAngledFlag) {
> if (TRUE == bUpdataDefaulBordenFlag) {
> bLabelAngledParkingOutSlotFlag = TRUE;
> }
> if (TRUE == bLabelAngledParkingOutSlotFlag) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> }
> if (FALSE == bUpdataDefaulBordenFlag) {
> DefaultOffsetY1 = 0;
> DefaultOffsetY2 = 0;
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (FALSE == bCarryOutSlot)) {
> DefaultOffsetY1 = fDis1 - 600;
> DefaultOffsetY2 = fDis2 - 600;
> }
> if (OffsetY1 < DefaultOffsetY1) {
> OffsetY1 = DefaultOffsetY1;
> }
> if (OffsetY2 < DefaultOffsetY2) {
> OffsetY2 = DefaultOffsetY2;
> }
> } else {
> OffsetX1 = 0;
> OffsetX2 = 0;
> OffsetY1 = 0;
> OffsetY2 = 0;
> }
> if (APAMap_GInputData.ParkReqPar.Request_SlotFusionMode ==
> APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {
> // movedis < 0  move upward;
> NewDis1 = fDis1 - OffsetY1;
> NewDis2 = fDis2 - OffsetY2;
> NewDis = NewDis1 + NewDis2;
> // if (NewDis < 2 * SafeDis) {
> //   APAMAP_Setfailcause(57);
> //   return FALSE;
> // }
> } else {
> NewDis1 = fDis1 - OffsetY1;
> NewDis2 = fDis2 - OffsetY2;
> NewDis = NewDis1 + NewDis2;
> if (NewDis < 2 * SafeDis) {
> APAMAP_Setfailcause(58);
> #ifdef DEBUG_PRINT_SLOTOBJ
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==app==FailCause58Debug==ObjPt2(%.2f,%.2f)=ObjPt1(%.2f,%.2f)=="
> "EndPos(%.2f,%.2f)=="
> "Dis(%.2f,%.2f)==Offset(%.2f,%.2f)==FSDOffset(%.2f,%.2f)=="
> "ODOffset(%.2f,%.2f)=\n==HalfWideCar(%u)=="
> "SensorOffset1(%.2f,%.2f)==SensorOffset2(%.2f,%.2f)==",
> Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, EndPosLine.A,
> EndPosLine.C, fDis1, fDis2, OffsetY1, OffsetY2, FSDOffsetY1,
> FSDOffsetY2, ODOffsetY1, ODOffsetY2,
> APAMap_ComCfg.HalfWidthOfCar, SensorOffsetX1, SensorOffsetY1,
> SensorOffsetX2, SensorOffsetY2);
> TLOG_INFO << log_string;
> #endif
> return FALSE;
> }
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (APAMap_GInfo.SlotPar.SlotLen < (APAMap_ComCfg.LengthOfCar + 700)) {
> OffsetY1 = 0;
> OffsetY2 = 0;
> }
> } else {
> if (APAMap_GInfo.SlotPar.SlotLen < (APAMap_ComCfg.WidthOfCar + 500)) {
> OffsetY1 = 0;
> OffsetY2 = 0;
> }
> }
> // obj1
> TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj1Ang;
> TempCarPos2.Coordinate = Obj1Pt;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
> if (OffsetY1 != 0) {
> TempDis = OffsetY1;
> if ((fDis1 - OffsetY1) < 200) {
> TempDis = OffsetY1 - 100;
> }
> if (bSlotDataAtRigthSide == FALSE) {
> TempDis = -TempDis;
> }
> TempLine2 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &TempLine2, TempCarPos2.CarAng, TempDis);
> }
> TempCarPos1.CarAng = OrgAng;
> TempCarPos1.Coordinate = Obj1Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
> if (OffsetX1 != 0) {
> TempDis = OffsetX1;
> if (bSlotDataAtRigthSide) {
> TempDis = -TempDis;
> }
> TempLine1 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &TempLine1, TempCarPos1.CarAng, TempDis);
> }
> if ((OffsetY1 != 0) || (OffsetX1 != 0)) {
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &Obj1Pt);
> }
> // obj2
> TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> TempCarPos2.Coordinate = Obj2Pt;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
> if (OffsetY2 != 0) {
> TempDis = OffsetY2 + BloundaryOffsetY;
> if ((fDis2 - OffsetY2) < 200) {
> BloundaryOffsetY = -100;
> if (TRUE == bShortestSlotLen) {
> TempDis = OffsetY2 - 150;
> }
> }
> if (bSlotDataAtRigthSide == TRUE) {
> TempDis = -TempDis;
> }
> TempLine2 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &TempLine2, TempCarPos2.CarAng, TempDis);
> }
> TempCarPos1.CarAng = OrgAng;
> TempCarPos1.Coordinate = Obj2Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
> if (TRUE == bCntAddFlag)  // 判断是子边界导致的轨迹重算，不更新Obj2的x坐标
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (CurCarCoordinateX > 1) {
> OffsetX2 = 0;
> }
> }
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (FALSE == bAfterNewAnchorPointFlag) && (TRUE == bShortSlotLen)) {
> OffsetX2 = 0;
> }
> if ((OffsetX2 != 0) || (TRUE == bShortSlotLen)) {
> TempDis = OffsetX2;
> if (TempDis > 1000) {
> TempDis = 1000;
> }
> if (bSlotDataAtRigthSide) {
> TempDis = -TempDis;
> }
> TempLine1 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &TempLine1, TempCarPos1.CarAng, TempDis);
> }
> if ((OffsetY2 != 0) || (OffsetX2 != 0)) {
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &Obj2Pt);
> }
> APAMap_ParkingOutCarPosInvadeSlotBorderInfo(&Obj2Pt, &Obj1Pt,
> bUpdataDefaulBordenFlag);
> TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> TempCarPos2.Coordinate = Obj2Pt;
> Line = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
> SlotLen = AlgCom_GetPointToLineDis(Obj1Pt, Line);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> bLonggestSlotLen = FALSE;
> bShortSlotLen = FALSE;
> bShortestSlotLen = FALSE;
> if (SlotLen > (APAMap_ComCfg.LengthOfCar + 2000)) {
> bLonggestSlotLen = TRUE;
> } else {
> if (SlotLen <= (APAMap_ComCfg.LengthOfCar + 1500)) {
> bShortSlotLen = TRUE;
> if (SlotLen <= (APAMap_ComCfg.LengthOfCar + 1100)) {
> bShortestSlotLen = TRUE;
> }
> }
> }
> } else {
> bShortSlotLen = FALSE;
> bShortestSlotLen = FALSE;
> }
> // Recal endpos;
> if ((TRUE == bAfterNewAnchorPointFlag)  // 满足锚点转换后，不更新终点位置
> || (TRUE == bLaneLineUpdateEndCarPosFlag) ||
> (TRUE ==
> bRefercLineUpdateEndCarPosFlag))  // 满足车道线条件，不更新终点位置
> {
> EndPos = APAMap_GInfo.SlotPar.EndPos;
> EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;
> } else {
> // zqf: add EndCarPos update
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==1.ParkoutUWBPos==Parkout_UWBPos(%d,%d)==EndPos.Coordinate(%"
> ".2f,%.2f)==",
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x,
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y,
> EndPos.Coordinate.x, EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> } else {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==2.Parkout==EndPos.Coordinate(%.2f,%.2f)==",
> EndPos.Coordinate.x, EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> }
> #else
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> #endif
> EndPosLine = AlgCom_LineParABCByCurrentCarPosition(&EndPos, 0);
> if (EndPos.Coordinate.x == 0xff) {
> APAMAP_Setfailcause(101);
> return FALSE;
> }
> }
> {
> char log_string[1024];
> snprintf(
> log_string, sizeof(log_string),
> "==app==ObjbyFSDAndOD==ObjPt2(%.2f,%.2f)=ObjPt1(%.2f,%.2f)==EndPos(%."
> "2f,%.2f)==SlotLen(%.2f)==LengthOfCar(%d)==WidthOfCar(%d)"
> "==Dis(%.2f,%.2f)==OffsetX(%.2f,%.2f)==OffsetY(%.2f,%.2f)==FSDOffset(2("
> "%.2f,%.2f),1(%.2f,%.2f))==ODOffset(2(%f,%f),1(%.2f,%.2f))==\n"
> "==SensorOffset1(%.2f,%.2f)==SensorOffset2(%.2f,%.2f)=="
> "bLonggestSlotLen(%d)==bShortSlotLen(%d)==bShortestSlotLen(%d)=="
> "BloundaryOffsetY(%.2f)",
> Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, EndPosLine.A, EndPosLine.C,
> SlotLen, APAMap_ComCfg.LengthOfCar, APAMap_ComCfg.WidthOfCar, fDis1,
> fDis2, OffsetX1, OffsetX2, OffsetY1, OffsetY2, FSDOffsetX2, FSDOffsetY2,
> FSDOffsetX1, FSDOffsetY1, ODOffsetX2, ODOffsetY2, ODOffsetX1,
> ODOffsetY1, SensorOffsetX1, SensorOffsetY1, SensorOffsetX2,
> SensorOffsetY2, bLonggestSlotLen, bShortSlotLen, bShortestSlotLen,
> BloundaryOffsetY);
> TLOG_INFO << log_string;
> }
> if ((OffsetY2 != 0) || (OffsetX2 != 0)) {
> APAMap_GInfo.SlotPar.bObj2Exist = TRUE;
> }
> if ((OffsetY1 != 0) || (OffsetX1 != 0)) {
> APAMap_GInfo.SlotPar.bObj1Exist = TRUE;
> }
> APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
> APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;
> APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
> APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
> TempCarPos1.CarAng = OrgAng;
> TempCarPos1.Coordinate = OrgPt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
> TempCarPos1.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
> if (OffsetX2 != 0) {
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, Obj2Pt,
> &TempLine2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt);
> APAMap_GInfo.SlotPar.SlotBordPt[0] = TempPt;
> }
> if (OffsetX1 != 0) {
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, Obj1Pt,
> &TempLine2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt);
> APAMap_GInfo.SlotPar.SlotBordPt[1] = TempPt;
> }
> APAMap_GInfo.NewCordSysOPt = APAMap_GInfo.SlotPar.SlotBordPt[0];
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SlotBordPt(2(%f,%f),1(%.2f,%.2f))==",
> APAMap_GInfo.SlotPar.SlotBordPt[0].x,
> APAMap_GInfo.SlotPar.SlotBordPt[0].y,
> APAMap_GInfo.SlotPar.SlotBordPt[1].x,
> APAMap_GInfo.SlotPar.SlotBordPt[1].y);
> TLOG_INFO << log_string;
> }
> APAMap_GInfo.SlotPar.SlotLen = (APA_DISTANCE_TYPE)SlotLen;
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x == NO_OBJ_DISTANCE) {
> APAMap_GInfo.SlotPar.EndPos = EndPos;
> }
> #else
> APAMap_GInfo.SlotPar.EndPos = EndPos;
> #endif
> APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
> return TRUE;
> }
> APACoordinateDataCalFloatType AlgCom_SetParkOutObj1Pt(
> uint8_t_INF ParkOutEightMode, APACarCoordinateDataCalFloatType CurCarPos) {
> APACoordinateDataCalFloatType Obj1Pt;
> APACoordinateDataCalFloatType TempPt1;
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1, SafeDisCal2;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPARALLELSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPRearSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToDefaultBoundary;
> CarWidth = APAMap_ComCfg.WidthOfCar;                 // mm
> SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0];  // 250mm, 0 paralIn;
> SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 400mm, 1 PerpIn;
> CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
> CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
> SafeDisToPARALLELSlotPt = 2000;                      // mm
> SafeDisToPERPSlotPt = 500;                           // mm
> SafeDisToPERPRearSlotPt = 300;                       // mm
> SafeDisToDefaultBoundary = 0;                        // mm
> if ((ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)) {
> TempPt1.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary);
> TempPt1.y = CarLFCal + SafeDisCal2;  // - 100;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
> TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary;
> TempPt1.y = CarLFCal + SafeDisCal2;  // - 100;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
> TempPt1.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
> SafeDisToDefaultBoundary);
> TempPt1.y = -(CarLRCal + SafeDisCal2);  // - 100);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) {
> TempPt1.x = -((CarWidth / 2) + SafeDisCal1);
> TempPt1.y = -(CarLRCal + SafeDisCal1 + SafeDisToPARALLELSlotPt);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary;
> TempPt1.y = -(CarLRCal + SafeDisCal2);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
> TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
> SafeDisToDefaultBoundary;
> TempPt1.y = -(CarLRCal + SafeDisCal2);  // - 100);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT) {
> TempPt1.x = (CarWidth / 2) + SafeDisCal1;
> TempPt1.y = -(CarLRCal + SafeDisCal1 + SafeDisToPARALLELSlotPt);
> } else {
> }
> Obj1Pt = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> return Obj1Pt;
> }
> APACoordinateDataCalFloatType AlgCom_SetParkOutObj2Pt(
> uint8_t_INF ParkOutEightMode, APACarCoordinateDataCalFloatType CurCarPos) {
> APACoordinateDataCalFloatType Obj2Pt;
> APACoordinateDataCalFloatType TempPt2;
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1, SafeDisCal2;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPARALLELSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPRearSlotPt;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToDefaultBoundary;
> CarWidth = APAMap_ComCfg.WidthOfCar;                 // mm
> SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0];  // 250mm, 0 paralIn;
> SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 400mm, 1 PerpIn;
> CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
> CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
> SafeDisToPARALLELSlotPt = 1500;                      // mm
> SafeDisToPERPSlotPt = 500;                           // mm
> SafeDisToPERPRearSlotPt = 300;                       // mm
> SafeDisToDefaultBoundary = 0;                        // mm
> if ((ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)) {
> TempPt2.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary;
> TempPt2.y = CarLFCal + SafeDisCal2;  // - 100;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT) {
> TempPt2.x = (CarWidth / 2) + SafeDisCal1;
> TempPt2.y = CarLFCal + SafeDisCal1 + SafeDisToPARALLELSlotPt;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
> TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary);
> TempPt2.y = CarLFCal + SafeDisCal2;  // - 100;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) {
> TempPt2.x = -((CarWidth / 2) + SafeDisCal1);
> TempPt2.y = CarLFCal + SafeDisCal1 + SafeDisToPARALLELSlotPt;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
> TempPt2.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
> SafeDisToDefaultBoundary;
> TempPt2.y = -(CarLRCal + SafeDisCal2);  // - 100);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
> SafeDisToDefaultBoundary);
> TempPt2.y = -(CarLRCal + SafeDisCal2);
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
> TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
> SafeDisToDefaultBoundary);
> TempPt2.y = -(CarLRCal + SafeDisCal2);  // - 100);
> } else {
> }
> Obj2Pt = AlgCom_PointPosWithAngAndCenterPt(TempPt2, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> return Obj2Pt;
> }
> APA_DISTANCE_CAL_FLOAT_TYPE
> AlgCom_SetParkingOutObjAng(uint8_t_INF ParkOutEightMode,
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng) {
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
> Obj2Ang = 0;
> if ((ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT)) {
> Obj2Ang = (APA_DISTANCE_CAL_FLOAT_TYPE)(OrgAng + (PI / 2.0));
> } else if ((ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT)) {
> Obj2Ang = (APA_DISTANCE_CAL_FLOAT_TYPE)(OrgAng - (PI / 2.0));
> } else {
> }
> return Obj2Ang;
> }
> void APAMap_ParkingOutDebugInit(void) {
> bCntAddFlag = FALSE;
> bLaneLineUpdateEndCarPosFlag = FALSE;
> bRefercLineUpdateEndCarPosFlag = FALSE;
> bAfterNewAnchorPointFlag = FALSE;
> bFsdInRightOfEndCarPosFlag = FALSE;
> bFsdFromMapMainSlotBordFlag = FALSE;
> bFsdFromMapSubSlotBordFlag = FALSE;
> bFsdFromMapMainAndSubSlotBordFlag = FALSE;
> bPreventStepNRedundantFlag = FALSE;
> bShortestSlotLen = FALSE;
> bShortSlotLen = FALSE;
> bLonggestSlotLen = FALSE;
> bCarryOutSlot = FALSE;
> bLabelAngledFlag = FALSE;
> bObjLabelLadderFlag = FALSE;
> bLabelAngledParkingOutSlotFlag = FALSE;
> bODWheelChockFlag = FALSE;
> ParkOutEightMode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
> for (uint16_t i = 0; i < ElectrFencePtNum; i++) {
> APAMapEFOutputData.ElectronicFencePt[i].x = 0.0;
> APAMapEFOutputData.ElectronicFencePt[i].y = 0.0;
> }
> APAMapEFOutputData.CarPos.CarAng = 0.0;
> APAMapEFOutputData.CarPos.Coordinate.x = 0.0;
> APAMapEFOutputData.CarPos.Coordinate.y = 0.0;
> APAMapEFOutputData.timeStamp_ms = 0.0;
> APAMap_ParkingOutBkSDGOutPutDataInit();
> APAMap_ParkingOutBkDataBfSDGFusInit();
> }
> BOOLEAN APAMap_ParkingOutCalSlotBorderPtByParkOutInfo() {
> APA_ENUM_TYPE SlotType;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
> BOOLEAN bObj2Exist, bObj1Exist;
> BOOLEAN bSlotDataAtRigthSide;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
> APACoordinateDataCalFloatType OrgPt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType TempPt1, TempPt2, TempPt3;
> APA_DISTANCE_TYPE SlotLength;
> APA_DISTANCE_TYPE SlotDepth;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
> APALineParameterABCType TempLine;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToSlotInnerPt;
> APA_ENUM_TYPE ParkSide;
> BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位
> bSeizeEndCarPosFlag = FALSE;
> OffsetX = 100;
> SafeDisToSlotInnerPt = 100;  // 300;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> ParkOutEightMode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
> // zqf-GetParkOutEightMode
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> ParkOutEightMode = AlgCom_GetParkOutEightMode(ParkOutMode, ParkSide);
> // zqf-SetParkOutObjPt
> TLOG_DEBUG << "start AlgCom_SetParkOutObj1Pt...";
> Obj1Pt = AlgCom_SetParkOutObj1Pt(ParkOutEightMode, CurCarPos);
> Obj2Pt = AlgCom_SetParkOutObj2Pt(ParkOutEightMode, CurCarPos);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);
> } else {
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);
> }
> fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);
> fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);
> SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - 150.0)) ||
> ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - 150.0))) {
> APAMAP_Setfailcause(100);
> return FALSE;
> }
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Obj2Pt, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> // OrgPt.y = APAMap_ComCfg.LenBetweenRAxisAndFBumper + 300.0;
> // TempPt3.y = TempPt2.y;
> // if (bSlotDataAtRigthSide == TRUE) {
> //   TempPt3.x = -(APAMap_ComCfg.HalfWidthOfCar +
> //                 OffsetX);  // slot at right,left paral park out
> // } else {
> //   TempPt3.x = (APAMap_ComCfg.HalfWidthOfCar + OffsetX);
> // }
> OrgAng = 0;
> SlotType = 0;
> SlotDepth = (APA_DISTANCE_TYPE)(APAMap_ComCfg.WidthOfCar + OffsetX + 300);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> // TempPt3.y = (APAMap_ComCfg.LenBetweenRAxisAndFBumper + OffsetX);
> // TempPt3.x = TempPt2.x;
> if (bSlotDataAtRigthSide == FALSE) {
> OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(PI / 2.0);
> } else {
> OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(-PI / 2.0);
> }
> SlotType = 1;
> SlotDepth = (APA_DISTANCE_TYPE)(APAMap_ComCfg.LengthOfCar + OffsetX + 300);
> } else {
> // TempPt3.y = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + OffsetX);
> // TempPt3.x = TempPt2.x;
> if (bSlotDataAtRigthSide == FALSE) {
> OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(-PI / 2.0);
> } else {
> OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(PI / 2.0);
> }
> SlotType = 1;
> SlotDepth = APAMap_ComCfg.LengthOfCar + (APA_DISTANCE_TYPE)OffsetX + 300;
> }
> OrgAng = OrgAng + CurCarPos.CarAng;
> // OrgPt = AlgCom_PointPosWithAngAndCenterPt(TempPt3, CurCarPos.CarAng,
> //                                           CurCarPos.Coordinate);
> OrgPt = Obj2Pt;
> bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
> bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
> } else {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.HalfWidthOfCar);
> }
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt2, 0, OrgAng, OrgPt);
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt3, 0, OrgAng, OrgPt);
> } else {
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> } else {
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> }
> TempPt1.x = APAMap_ComCfg.HalfWidthOfCar;
> TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt2, 0, OrgAng, OrgPt);
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt3, 0, OrgAng, OrgPt);
> }
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt2.x = -TempPt2.x;
> TempPt3.x = -TempPt3.x;
> }
> if (TempPt2.x > TempPt3.x) {
> MaxSlotPtX = TempPt2.x;
> } else {
> MaxSlotPtX = TempPt3.x;
> }
> MaxSlotPtX += SafeDisToSlotInnerPt;
> APAMap_GInfo.SlotPar.slotCarEndPosXBackUp = MaxSlotPtX;
> APAMap_GInfo.SlotPar.SlotDepth = SlotDepth;
> APAMap_GInfo.SlotPar.SlotLen = SlotLength;
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==1.ParkoutUWBPos==Parkout_UWBPos(%d,%d)==TempPt3(%.2f,%.2f)==",
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x,
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y, TempPt3.x,
> TempPt3.y);
> TLOG_INFO << log_string;
> }
> } else {
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==2.Parkout==EndPos.Coordinate(%.2f,%.2f)==", TempPt3.x,
> TempPt3.y);
> TLOG_INFO << log_string;
> }
> }
> #else
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> #endif
> if (TempPt3.x == 0xff) {
> APAMAP_Setfailcause(101);
> return FALSE;
> }
> APAMap_GInfo.SlotPar.EndPos.Coordinate = TempPt3;
> APAMap_GInfo.SlotPar.EndPosLine =
> AlgCom_LineParABCByCurrentCarPosition(&APAMap_GInfo.SlotPar.EndPos, 0);
> APAMap_GInfo.bDataMirrored = FALSE;
> APAMap_GInfo.bCordSysReSet = FALSE;
> APAMap_GInfo.SlotPar.SlotType = SlotType;
> APAMap_GInfo.SlotPar.bObj2Exist = bObj2Exist;
> APAMap_GInfo.SlotPar.bObj1Exist = bObj1Exist;
> APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
> APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
> APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
> APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;
> APAMap_GInfo.NewCordSysOPt = OrgPt;
> APAMap_GInfo.NewCordSysAng = OrgAng;
> APAMap_GInfo.SlotPar.Obj2Ang =
> AlgCom_SetParkingOutObjAng(ParkOutEightMode, OrgAng);
> APAMap_GInfo.SlotPar.Obj1Ang = APAMap_GInfo.SlotPar.Obj2Ang;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FirstBuildMapObjAndEndCarPos==Obj2Pt(%.2f,%.2f,%.2f)==Obj1Pt(%."
> "2f,%.2f,%.2f)==NewCordSysOPt(%.2f,%.2f,%.2f)"
> "==SlotBordPt[0](%.2f,%.2f)====SlotBordPt[1](%.2f,%.2f)==EndPos(%."
> "2f,%.2f,%.2f)==SlotLen(%d)==SlotDepth(%d)==APAstate(%d)=="
> "APARunningstate(%d)",
> APAMap_GInfo.SlotPar.Obj2Pt.x, APAMap_GInfo.SlotPar.Obj2Pt.y,
> APAMap_GInfo.SlotPar.Obj2Ang, APAMap_GInfo.SlotPar.Obj1Pt.x,
> APAMap_GInfo.SlotPar.Obj1Pt.y, APAMap_GInfo.SlotPar.Obj1Ang,
> APAMap_GInfo.NewCordSysOPt.x, APAMap_GInfo.NewCordSysOPt.y,
> APAMap_GInfo.NewCordSysAng, APAMap_GInfo.SlotPar.SlotBordPt[0].x,
> APAMap_GInfo.SlotPar.SlotBordPt[0].y,
> APAMap_GInfo.SlotPar.SlotBordPt[1].x,
> APAMap_GInfo.SlotPar.SlotBordPt[1].y,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y,
> APAMap_GInfo.SlotPar.EndPos.CarAng, APAMap_GInfo.SlotPar.SlotLen,
> APAMap_GInfo.SlotPar.SlotDepth,
> APAMap_GInputData.ParkReqPar.APAstate,
> APAMap_GInputData.ParkReqPar.APARunningstate);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> // zqf: add PAObj for update Obj
> void APAMAP_GetSlotBdPtBySensorObjs(APA_ENUM_TYPE Bordpttype,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY) {
> APA_DISTANCE_TYPE i;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetY;
> APACoordinateDataCalFloatType LineXStrPt;
> APACoordinateDataCalFloatType Data[UPA_APA_SNS_DT_NON_TRIANGLE_OBJ_ARRAY_NUM];
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;
> tMap_PAObjInfo_t* pPAobjInfo;
> BOOLEAN bSearch;
> APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
> APALineParameterABCType TempLine;
> APA_DISTANCE_TYPE FrontMidSnsDis;
> APA_DISTANCE_TYPE RearMidSnsDis;
> uint8_t_INF ParkOutMode;
> BOOLEAN bSlotDataAtRigthSide;
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> *pOffsetX = 0;
> *pOffsetY = 0;
> return;
> }
> pPAobjInfo = &APAMap_GInputData.PAobjInfo;
> MaxOutOffsetY = 1000;
> MaxInnerOffsetX = 2000;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (FALSE == bLabelAngledFlag) {
> MaxInnerOffsetX = 4000;
> }
> } else {
> MaxInnerOffsetX = 1500;
> }
> MaxInnerOffsetY = APAMap_GInfo.SlotPar.SlotLen;
> if (TRUE == bCarryOutSlot) {
> if (Bordpttype == 0) {
> TempDis = APAMap_GetSearchMaxInnerY(0, bSlotDataAtRigthSide,
> APAMap_GInfo.SlotPar.SlotBordPt[1],
> APAMap_GInfo.SlotPar.Obj1Ang);
> } else {
> TempDis = APAMap_GetSearchMaxInnerY(1, bSlotDataAtRigthSide,
> APAMap_GInfo.SlotPar.SlotBordPt[0],
> APAMap_GInfo.SlotPar.Obj2Ang);
> }
> } else {
> TempDis1 = APAMap_GetDisByCarPosToBumper(Bordpttype);
> TempLine = APAMAP_GetSlotLineByCarPos();
> if (Bordpttype == 0) {
> LineXStrPt = APAMap_GInfo.SlotPar.SlotBordPt[1];  // Obj1
> } else {
> LineXStrPt = APAMap_GInfo.SlotPar.SlotBordPt[0];  // Obj2
> }
> TempDis = AlgCom_GetPointToLineDis(LineXStrPt, TempLine);
> TempDis -= TempDis1;
> }
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> TempDis -= 300;
> } else {
> TempDis -= 100;
> }
> if (TempDis < 0) {
> TempDis = 0;
> }
> MaxInnerOffsetY = TempDis;
> bSearch = TRUE;
> i = 0;
> OffsetX = -MaxInnerOffsetX;
> OffsetY = -MaxOutOffsetY;
> PreOffsetY = 0;
> PreOffsetX = 0;
> FrontMidSnsDis = NO_OBJ_DISTANCE;
> RearMidSnsDis = NO_OBJ_DISTANCE;
> for (i = 0; i < UPA_APA_SNS_DT_NON_TRIANGLE_OBJ_ARRAY_NUM; i++) {
> if (i < UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM) {
> Data[i].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PARearSys].wX[i];
> Data[i].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PARearSys].wY[i];
> if (RearMidSnsDis > Data[i].y) {
> RearMidSnsDis = Data[i].y;
> }
> } else {
> Data[i].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PAFrontSys]
> .wX[i - UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM];
> Data[i].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PAFrontSys]
> .wY[i - UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM];
> if (FrontMidSnsDis > Data[i].y) {
> FrontMidSnsDis = Data[i].y;
> }
> }
> }
> if (bSearch) {
> PreOffsetY = OffsetY;
> PreOffsetX = OffsetX;
> if (Bordpttype == 1) {
> if ((TRUE == APAMap_CheckInputDataIsValidByTimeStamp(
> APAMap_GInputData.CarLocInfo.timeStamp_ms,
> pPAobjInfo->timeStamp_ms, 5000)) &&
> (pPAobjInfo->timeStamp_ms != 0)) {
> if (FrontMidSnsDis > pPAobjInfo->SnsNearDis[PAFrontSys]
> .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX]) {
> FrontMidSnsDis = pPAobjInfo->SnsNearDis[PAFrontSys]
> .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX];
> }
> if (FrontMidSnsDis > pPAobjInfo->SnsNearDis[PAFrontSys]
> .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX]) {
> FrontMidSnsDis = pPAobjInfo->SnsNearDis[PAFrontSys]
> .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX];
> }
> if (FrontMidSnsDis >
> pPAobjInfo->SnsNearDis[PAFrontSys].wY[LM_RM_TRIANGLE_OBJ_INDEX]) {
> FrontMidSnsDis =
> pPAobjInfo->SnsNearDis[PAFrontSys].wDis[LM_RM_TRIANGLE_OBJ_INDEX];
> }
> if (FrontMidSnsDis >
> pPAobjInfo->SnsNearDis[PAFrontSys].wY[RM_LM_TRIANGLE_OBJ_INDEX]) {
> FrontMidSnsDis =
> pPAobjInfo->SnsNearDis[PAFrontSys].wDis[RM_LM_TRIANGLE_OBJ_INDEX];
> }
> OffsetY = MaxInnerOffsetY - FrontMidSnsDis;
> } else {
> FrontMidSnsDis = NO_OBJ_DISTANCE;
> OffsetY = 0;
> }
> } else {
> if ((TRUE == APAMap_CheckInputDataIsValidByTimeStamp(
> APAMap_GInputData.CarLocInfo.timeStamp_ms,
> pPAobjInfo->timeStamp_ms, 5000)) &&
> (pPAobjInfo->timeStamp_ms != 0)) {
> if (RearMidSnsDis > pPAobjInfo->SnsNearDis[PARearSys]
> .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX]) {
> RearMidSnsDis = pPAobjInfo->SnsNearDis[PARearSys]
> .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX];
> }
> if (RearMidSnsDis > pPAobjInfo->SnsNearDis[PARearSys]
> .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX]) {
> RearMidSnsDis = pPAobjInfo->SnsNearDis[PARearSys]
> .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX];
> }
> if (RearMidSnsDis >
> pPAobjInfo->SnsNearDis[PARearSys].wY[LM_RM_TRIANGLE_OBJ_INDEX]) {
> RearMidSnsDis =
> pPAobjInfo->SnsNearDis[PARearSys].wDis[LM_RM_TRIANGLE_OBJ_INDEX];
> }
> if (RearMidSnsDis >
> pPAobjInfo->SnsNearDis[PARearSys].wY[RM_LM_TRIANGLE_OBJ_INDEX]) {
> RearMidSnsDis =
> pPAobjInfo->SnsNearDis[PARearSys].wDis[RM_LM_TRIANGLE_OBJ_INDEX];
> }
> OffsetY = MaxInnerOffsetY - RearMidSnsDis;
> } else {
> RearMidSnsDis = NO_OBJ_DISTANCE;
> OffsetY = 0;
> }
> }
> if (OffsetY > MaxInnerOffsetY) {
> OffsetY = MaxInnerOffsetY;
> }
> }
> bSearch = FALSE;
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PAOffset(%u)(%f,%f),Max(%f),PreOffsetY(%f,%f),FrontMidSnsDis(%d),"
> "RearMidSnsDis(%d)))",
> Bordpttype, OffsetX, OffsetY, MaxInnerOffsetY, PreOffsetX,
> PreOffsetY, FrontMidSnsDis, RearMidSnsDis);
> TLOG_INFO << log_string;
> if (OffsetY > 50) {
> *pOffsetX = 0;
> *pOffsetY = OffsetY;
> } else {
> *pOffsetX = 0;
> *pOffsetY = 0;
> }
> return;
> }
> BOOLEAN APAMap_ParkingOutCalMapInfo() {
> BOOLEAN bResult;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType Obj2Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType OrgPt;
> APACarCoordinateDataCalFloatType EndPos;
> APA_ENUM_TYPE i;
> BOOLEAN bCenterEndCarPosFlag = FALSE;  // 采用终点位置居中标志位
> BOOLEAN bSeizeEndCarPosFlag;           // fsd侵占终点位置标志位
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bResult = APAMap_ParkingOutCalBoundaryByParkOutInfo();
> BoudaryNum[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> if (bResult) {
> bResult = APAMap_ParkingOutFusBoundaryByFSDMapInfo();
> BoudaryNum[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> debug3++;
> }
> if (bResult) {
> bResult = APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
> BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> }
> if (bResult) {
> bResult = APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
> BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> }
> if (bResult) {
> bResult = APAMap_FusBoundaryByODMapInfo();
> BoudaryNum[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> debug4++;
> }
> if (bResult) {
> #ifdef APAMAP_PARKOUT_FUS_SDG
> APAMap_ParkingOutUpDataMapBoundaryBySDGInfo();
> APAMap_ParkingOutDeleteMainSlotBord();
> #endif
> APAMap_SmoothMapBoundary(0);
> BoudaryNum[3][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[3][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> debug2++;
> }
> // zqf: add EndCarPos update
> i = 0;
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> if (TRUE == bSeizeEndCarPosFlag) {
> bCenterEndCarPosFlag = APAMap_ParkingOutCenterEndCarPosInfo();
> if (TRUE == bCenterEndCarPosFlag) {
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==CenterEndCarPosUpdata===bCenterEndCarPosFlag(%d)"
> "==bSeizeEndCarPosFlag(%d)==EndPos_Coordinate(%.2f,%.2f)",
> bCenterEndCarPosFlag, bSeizeEndCarPosFlag,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> }
> while (TRUE == bSeizeEndCarPosFlag) {
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> } else {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> }
> #else
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> #endif
> APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> i++;
> if (i > 9) {
> bSeizeEndCarPosFlag = FALSE;
> }
> }
> return bResult;
> }
> BOOLEAN APAMap_ParkingOutCheckIfCarPosIsValid() {
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Ptcc[4];
> if ((APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
> (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKEXIT)) {
> return TRUE;
> }
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> CurCarPos = APAMap_GInfo.CarPos;
> CurCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarPos.Coordinate, 0, Angle, Pto);
> CurCarPos.CarAng -= Angle;
> if (bSlotDataAtRigthSide == FALSE) {
> CurCarPos.Coordinate.x = -CurCarPos.Coordinate.x;
> CurCarPos.CarAng = -CurCarPos.CarAng;
> }
> AlgCom_AngNormalized(&CurCarPos.CarAng);
> if (MATH_FABS(CurCarPos.CarAng) > 90.0 * PI / 180.0) {
> APAMAP_Setfailcause(47);
> return FALSE;
> }
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> Ptcc[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> Ptcc[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> Ptcc[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> Ptcc[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> if ((Ptcc[0].x > APAMap_ComCfg.HalfWidthOfCar) ||
> (Ptcc[1].x > APAMap_ComCfg.HalfWidthOfCar)) {
> APAMAP_Setfailcause(48);
> return FALSE;
> }
> if ((Ptcc[2].x < -7000.0) || (Ptcc[3].x < -7000.0)) {
> APAMAP_Setfailcause(49);
> return FALSE;
> }
> return TRUE;
> }
> bool_t_INF APAMap_ParkingOutCalBoundaryByParkOutInfo() {
> APA_DISTANCE_CAL_FLOAT_TYPE MaxY, MinY;
> BOOLEAN bSlotDataAtRigthSide;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxDefaultRoadWith;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultObj2PtX;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultObj1PtX;
> BOOLEAN bObj2Exist, bObj1Exist;
> APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1X, Obj2X;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang, Obj1Ang;
> APACoordinateDataCalFloatType OrgPt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APALineParameterABCType TempLine;
> APACarCoordinateDataCalFloatType TempCarPos;
> tMap_BoundPt_t MainBoudary;
> tMap_BoundPt_t SubBoundary;
> APA_ENUM_TYPE i;
> APACoordinateDataCalFloatType TempPt;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj1;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj2;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaulBordenObj3;
> uint8_t_INF ParkOutMode;
> BOOLEAN bUpdataDefaulBordenFlag;
> BOOLEAN bUpdataSubBoundaryFlag;
> BOOLEAN bWideChannelforParallelFlag;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> APACarCoordinateDataCalFloatType EndPos;
> APA_DISTANCE_TYPE LabelAngledDis;
> #ifdef SUPPORT_PARKING_OUT_UWB
> APACoordinateDataCalFloatType RemoContPos;
> #endif
> EndPos = APAMap_GInfo.SlotPar.EndPos;
> DefaultObj2PtX = 3000;
> DefaultObj1PtX = 3000;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> DefaulBordenObj1 = 1000;
> DefaulBordenObj2 = 1000;
> if (TRUE == bLonggestSlotLen) {
> DefaulBordenObj3 = 200;
> } else {
> DefaulBordenObj3 = 250;
> if (TRUE == bShortSlotLen) {
> DefaulBordenObj3 = 300;
> }
> if (TRUE == bShortestSlotLen) {
> DefaulBordenObj3 = 400;
> }
> }
> } else {
> if (TRUE == bLabelAngledFlag) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> DefaulBordenObj1 = 4000;
> DefaulBordenObj2 = 4000;
> } else {
> DefaulBordenObj1 = 1000;
> DefaulBordenObj2 = 1000;
> }
> } else {
> DefaulBordenObj1 = 2000;
> DefaulBordenObj2 = 2000;
> }
> DefaulBordenObj3 = 300;
> }
> bUpdataDefaulBordenFlag = FALSE;
> bUpdataSubBoundaryFlag = FALSE;
> bWideChannelforParallelFlag = FALSE;
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -1) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (CurCarCoordinateX > 0) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (CurCarCoordinateX > 0) {
> bUpdataDefaulBordenFlag = TRUE;
> } else {
> bUpdataSubBoundaryFlag = TRUE;
> }
> } else {
> if (CurCarCoordinateX > 0) {
> bUpdataDefaulBordenFlag = TRUE;
> }
> }
> // 判断对向通道是否太窄，判断标准采用车辆到锚点的距离Cal值大小
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (MATH_FABS(EndPos.Coordinate.x) >
> (APAMap_ComCfg.HalfWidthOfCar + 900))) {
> bWideChannelforParallelFlag = TRUE;
> }
> }
> if (FALSE == bUpdataDefaulBordenFlag)  // 车在车位内
> {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> DefaulBordenObj1 = 1000;
> if (TRUE == bLonggestSlotLen) {
> DefaulBordenObj2 = 0;
> } else {
> // 大于0.9m（通道宽）则允许用保守内缩策略；小于0.9米（通道窄）则用激进的内缩策略。
> if (TRUE == bWideChannelforParallelFlag) {
> DefaulBordenObj2 = 0;
> if (TRUE == bShortSlotLen) {
> DefaulBordenObj2 = 100;  // 50;
> }
> if (TRUE == bShortestSlotLen) {
> DefaulBordenObj2 = 200;  // 100;
> }
> } else {
> if (FALSE == bShortestSlotLen) {
> DefaulBordenObj2 = 500;
> } else {
> DefaulBordenObj2 = 1000;
> }
> }
> }
> } else {
> if ((TRUE == bLabelAngledFlag) &&
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
> DefaulBordenObj1 = 2000;
> DefaulBordenObj2 = 2000;
> }
> #if 0
> else if ((FALSE == bLabelAngledFlag)
> && (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND))
> {
> DefaulBordenObj1 = 0;
> DefaulBordenObj2 = 0;
> }
> #endif
> else {
> DefaulBordenObj1 = 1000;
> DefaulBordenObj2 = 1000;
> }
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutCalBoundaryByParkOutInfo==DefaulBordenObj1(%."
> "2f),DefaulBordenObj2(%.2f)==bUpdataDefaulBordenFlag(%d)=="
> "CurCarCoordinateX(%.2f)",
> DefaulBordenObj1, DefaulBordenObj2, bUpdataDefaulBordenFlag,
> CurCarCoordinateX);
> TLOG_INFO << log_string;
> }
> Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;  // APA CorSys
> Obj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
> Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;
> Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;
> bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
> bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> MaxSlotPtX = APAMap_GInfo.SlotPar.slotCarEndPosXBackUp + DefaulBordenObj3;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> if (TRUE == bLabelAngledFlag) {
> LabelAngledDis = 1000;
> } else {
> LabelAngledDis = 0;
> }
> MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(5000 + APAMap_ComCfg.LengthOfCar +
> LabelAngledDis);
> MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
> APAMap_GInfo.SlotPar.SlotLen - 1000);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> MaxDefaultRoadWith = 5000;
> } else {
> MaxDefaultRoadWith = 7000;
> }
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(12000 + APAMap_ComCfg.LengthOfCar +
> APAMap_ComCfg.LengthOfCar);
> MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
> APAMap_GInfo.SlotPar.SlotLen -
> APAMap_GInfo.SlotPar.SlotLen - 10000);
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> RemoContPos.x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x;  // APA坐标系下
> RemoContPos.y = (APA_DISTANCE_CAL_FLOAT_TYPE)
> APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y;
> RemoContPos = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> RemoContPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (RemoContPos.x < -4000) {
> MaxDefaultRoadWith = (MATH_FABS(RemoContPos.x)) + 800;
> } else {
> MaxDefaultRoadWith = 12000;
> }
> }
> }
> #endif
> MainBoudary.PtNum = 6;
> SubBoundary.PtNum = 2;
> Obj2Ang -= OrgAng;
> Obj1Ang -= OrgAng;
> AlgCom_AngNormalized(&Obj2Ang);
> AlgCom_AngNormalized(&Obj1Ang);
> if (bSlotDataAtRigthSide == FALSE) {
> Obj2Ang = -Obj2Ang;
> Obj1Ang = -Obj1Ang;
> }
> Obj2Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj2Pt, 0,
> OrgAng, OrgPt);
> Obj1Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
> OrgAng, OrgPt);
> if (bSlotDataAtRigthSide == FALSE) {
> Obj2Pt.x = -Obj2Pt.x;
> Obj1Pt.x = -Obj1Pt.x;
> }
> Obj2X = Obj2Pt.x;
> Obj1X = Obj1Pt.x;
> if (bObj2Exist == FALSE) {
> if (Obj2X > DefaultObj2PtX) {
> Obj2X = DefaultObj2PtX;
> }
> }
> if (bObj1Exist == FALSE) {
> if (Obj1X > DefaultObj1PtX) {
> Obj1X = DefaultObj1PtX;
> }
> }
> // 0
> MainBoudary.Points[0].x = Obj1X;
> MainBoudary.Points[0].y = MinY;
> MainBoudary.Points[0].x += DefaulBordenObj1 * MATH_SIN(Obj1Ang);
> MainBoudary.Property[0] = 0;
> // 1 2
> TempCarPos.Coordinate.x = Obj1Pt.x;
> TempCarPos.Coordinate.y = Obj1Pt.y;
> TempCarPos.CarAng = Obj1Ang;
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TempLine.LineType == APALineIsHorizontal) {
> MainBoudary.Points[1].y = Obj1Pt.y;
> MainBoudary.Points[2].y = Obj1Pt.y;
> } else {
> MainBoudary.Points[1].y = Obj1X * TempLine.A + TempLine.C;
> MainBoudary.Points[2].y = MaxSlotPtX * TempLine.A + TempLine.C;
> }
> MainBoudary.Points[1].x = Obj1X;
> MainBoudary.Points[2].x = MaxSlotPtX;
> MainBoudary.Points[1].x += DefaulBordenObj1 * MATH_SIN(Obj1Ang);
> MainBoudary.Points[1].y -= DefaulBordenObj1 * MATH_COS(Obj1Ang);
> MainBoudary.Property[1] = 0;
> MainBoudary.Property[2] = 0;
> // 3 4
> TempCarPos.Coordinate.x = Obj2Pt.x;
> TempCarPos.Coordinate.y = Obj2Pt.y;
> TempCarPos.CarAng = Obj2Ang;
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TempLine.LineType == APALineIsHorizontal) {
> MainBoudary.Points[3].y = Obj2Pt.y;
> MainBoudary.Points[4].y = Obj2Pt.y;
> } else {
> MainBoudary.Points[3].y = MaxSlotPtX * TempLine.A + TempLine.C;
> MainBoudary.Points[4].y = Obj2X * TempLine.A + TempLine.C;
> }
> MainBoudary.Points[3].x = MaxSlotPtX;
> MainBoudary.Points[4].x = Obj2X;
> MainBoudary.Points[4].x += DefaulBordenObj2 * MATH_SIN(Obj2Ang);
> MainBoudary.Points[4].y -= DefaulBordenObj2 * MATH_COS(Obj2Ang);
> MainBoudary.Property[3] = 0;
> MainBoudary.Property[4] = 0;
> // 5
> MainBoudary.Points[5].x = Obj2X;
> MainBoudary.Points[5].y = MaxY;
> MainBoudary.Points[5].x += DefaulBordenObj2 * MATH_SIN(Obj2Ang);
> MainBoudary.Property[5] = 0;
> for (i = 0; i < MainBoudary.PtNum; i++) {
> TempPt = MainBoudary.Points[i];
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt.x = -TempPt.x;
> }
> MainBoudary.Points[i] =
> AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);
> }
> // 0 1
> SubBoundary.Points[0].x = -MaxDefaultRoadWith;
> SubBoundary.Points[0].y = MinY;
> SubBoundary.Property[0] = 0;
> SubBoundary.Points[1].x = -MaxDefaultRoadWith;
> SubBoundary.Points[1].y = MaxY;
> SubBoundary.Property[1] = 0;
> for (i = 0; i < SubBoundary.PtNum; i++) {
> TempPt = SubBoundary.Points[i];
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt.x = -TempPt.x;
> }
> SubBoundary.Points[i] =
> AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);
> }
> if ((FALSE == bAfterNewAnchorPointFlag) || (TRUE == bUpdataSubBoundaryFlag)) {
> if (bSlotDataAtRigthSide == TRUE) {
> APAMap_GInfo.OutLine.LeftBoundary = SubBoundary;
> APAMap_GInfo.OutLine.RightBoundary = MainBoudary;
> } else {
> APAMap_GInfo.OutLine.LeftBoundary = MainBoudary;
> APAMap_GInfo.OutLine.RightBoundary = SubBoundary;
> }
> } else  // 锚点转换之后不再重置子边界，但需要重置主边界
> {
> if (bSlotDataAtRigthSide == TRUE) {
> APAMap_GInfo.OutLine.RightBoundary = MainBoudary;
> } else {
> APAMap_GInfo.OutLine.LeftBoundary = MainBoudary;
> }
> }
> APAMap_GInfo.SlotPar.SlotStrIndex = 2;
> APAMap_GInfo.SlotPar.SlotEndIndex = 3;
> APAMap_GInfo.SlotPar.Obj1PtIndex = 1;
> APAMap_GInfo.SlotPar.Obj2PtIndex = 4;
> APAMap_GInfo.SlotPar.SlotBordPt[2] = MainBoudary.Points[2];
> APAMap_GInfo.SlotPar.SlotBordPt[3] = MainBoudary.Points[3];
> return TRUE;
> }
> APALineParameterABCType APAMap_ParkingOutLineParABCByMainSlotBord(
> APACoordinateDataCalFloatType* MainSlotBordTemp1,
> APACoordinateDataCalFloatType* MainSlotBordTemp2) {
> APALineParameterABCType LineofPar11;  // change LinePar for LineofPar11 //QA.C
> APA_DISTANCE_CAL_FLOAT_TYPE DeltaConstant;
> float k, b;
> LineofPar11 = APAMap_GInfo.SlotPar.EndPosLine;
> DeltaConstant =
> MainSlotBordTemp1->y - APAMap_GInfo.SlotPar.EndPos.Coordinate.y;
> if (MATH_FABS(MainSlotBordTemp1->x - MainSlotBordTemp2->x) <
> __FLT_EPSILON__) {
> return LineofPar11;  // x=C;
> }
> if (MATH_FABS(MainSlotBordTemp1->y - MainSlotBordTemp2->y) <
> __FLT_EPSILON__) {
> return LineofPar11;  // y=C;
> }
> LineofPar11.LineType = APALineIsIncline;
> k = (MainSlotBordTemp2->y - MainSlotBordTemp1->y) /
> (MainSlotBordTemp2->x - MainSlotBordTemp1->x);
> b = (MainSlotBordTemp1->y - k * MainSlotBordTemp1->x) - DeltaConstant;
> if (MATH_FABS(k) < __FLT_EPSILON__) {
> LineofPar11.A = (APA_DISTANCE_CAL_FLOAT_TYPE)0;
> LineofPar11.B = (APA_DISTANCE_CAL_FLOAT_TYPE)-1;
> LineofPar11.C = APAMap_GInfo.SlotPar.EndPos.Coordinate.y;
> return LineofPar11;  // y=C;
> }
> LineofPar11.A = (APA_DISTANCE_CAL_FLOAT_TYPE)k;
> LineofPar11.B = (APA_DISTANCE_CAL_FLOAT_TYPE)-1;
> LineofPar11.C = (APA_DISTANCE_CAL_FLOAT_TYPE)b;
> return LineofPar11;
> }
> BOOLEAN APAMap_ParkingOutFusBoundaryByFSDMapInfo() {
> APA_ENUM_TYPE ParkSide;
> APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_ENUM_TYPE Index;
> APA_DISTANCE_TYPE i;
> APA_ENUM_TYPE k;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType pData1[127];
> APACoordinateDataCalFloatType pData2[127];
> APACoordinateDataCalFloatType pData3[127];
> APACoordinateDataCalFloatType pData4[127];
> APACoordinateDataCalFloatType NSegment[127];
> uint8_t_INF pPtStyle[127];
> uint8_t_INF NewProperty1[127];
> uint8_t_INF NewProperty2[127];
> uint8_t_INF NewProperty3[127];
> uint8_t_INF NewProperty4[127];
> uint8_t_INF NSegProperty[127];
> APA_ENUM_TYPE NSegNum;
> uint16_t_INF DataNum;
> APA_ENUM_TYPE Data1Num;
> APA_ENUM_TYPE Data2Num;
> APA_ENUM_TYPE Data3Num;
> APA_ENUM_TYPE Data4Num;
> st_MapTopViewFSD* pTopViewInfo;
> APA_DISTANCE_TYPE TopViewPtNum;
> APA_ENUM_TYPE LocStyle;
> UCHAR CurID;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
> APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
> APACoordinateDataCalFloatType MainLinXStrPt3, MainLinXEndPt3;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
> APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
> APACoordinateDataCalFloatType SubLinXStrPt1, SubLinXEndPt1;
> APALineParameterABCType TempLine1;
> APALineParameterABCType TempLine2;
> APACarCoordinateDataCalFloatType TempCarPos;
> APACoordinateDataCalFloatType TempPt1, TempPt2;
> APA_ENUM_TYPE OffsetIndex2, OffsetIndex1;
> APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
> APACoordinateDataCalFloatType pDataBk[127];
> APA_ENUM_TYPE DataNumBk;
> BOOLEAN bCheckSubLane;
> BOOLEAN bFusvalid;
> APACoordinateDataCalFloatType CurCarCoordinate;
> BOOLEAN bUpdataFsdObj2CalBoundaryFlag;
> BOOLEAN bObliqueRowStairsFlag;  // 阶梯斜列式场景标志位
> APA_DISTANCE_CAL_FLOAT_TYPE SubfDis;
> #if 0
> APALineParameterABCType EndPosLine;
> APACoordinateDataCalFloatType MainSlotBordTemp1;
> APACoordinateDataCalFloatType MainSlotBordTemp2;
> #endif
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> // return TRUE;
> }
> #if 1
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return TRUE;
> }
> pTopViewInfo = &APAMap_GInputData.TotalMapInfo.mapData.FSDInfo.TopView;
> #else
> if (APAMap_GInputData.VisObjsInfo.timestamp_ms == 0) {
> return TRUE;
> }
> pTopViewInfo = &APAMap_GInputData.VisObjsInfo.FSDInfo.TopView;
> #endif
> APAMap_CheckIfIgnoreFSDPtAtMainBoundary();
> APAMap_CheckIfIgnoreFSDPtAtSubBoundary();
> bObliqueRowStairsFlag = APAMap_ParkingOutObliqueRowStairsInfo();
> if (TRUE == bObliqueRowStairsFlag) {
> MaxOffsetX = 2000;
> } else {
> MaxOffsetX = 1000;  // 600;//700;//2000;
> }
> MaxOffsetY = 1000;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> // zqf:PARALLEL_SIDE
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> } else {
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> }
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
> CurCarPos = APAMap_GInfo.CarPos;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&TempAng);
> TempAng1 = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
> AlgCom_AngNormalized(&TempAng1);
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> fDis = TempAng;
> TempAng = TempAng1;
> TempAng1 = fDis;
> }
> SubfDis = 0;
> if (TRUE == bLabelAngledFlag) {
> SubfDis = 3000;
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> SubfDis = 1500;
> } else {
> SubfDis = 2000;
> }
> SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
> OffsetIndex2 = Obj2PtIndex - SlotEndIndex;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // LineYAngle = Angle;
> // obj1 border line
> MainLinXStrPt1 = OrgObj1Pt;
> MainLinXEndPt1 = MainLinXStrPt1;
> LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
> MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
> Data1Num = 0;
> for (Index = 0; Index <= Obj1PtIndex; Index++) {
> pData1[Data1Num] = pMapMainSlotBord->Points[Index];
> NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
> Data1Num++;
> if (Data1Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSD Buffer Not enough==1==Data1Num:(%d)", Data1Num);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> }
> // obj2 borderline;
> MainLinXStrPt2 = OrgObj2Pt;
> MainLinXEndPt2 = MainLinXStrPt2;
> LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
> MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
> Data2Num = 0;
> for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
> pData2[Data2Num] = pMapMainSlotBord->Points[Index];
> NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
> Data2Num++;
> if (Data2Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSD Buffer Not enough==2==Data2Num:(%d)", Data2Num);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> }
> // obj2 borderline2;
> MainLinXStrPt3.x = 0;
> MainLinXStrPt3.y = MaxOffsetY;
> MainLinXEndPt3.x = 1000;
> MainLinXEndPt3.y = MainLinXStrPt3.y;
> MainLinXStrPt3 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt3, Angle, Pto);
> MainLinXEndPt3 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt3, Angle, Pto);
> CurCarCoordinate = APAMap_GInputData.CarLocInfo.CarPos.Coordinate;
> CurCarCoordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarCoordinate, 0, Angle, Pto);
> bUpdataFsdObj2CalBoundaryFlag = FALSE;
> if (bSlotDataAtRigthSide) {
> CurCarCoordinate.x = -CurCarCoordinate.x;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinate.x < -1) {
> bUpdataFsdObj2CalBoundaryFlag = TRUE;
> }
> } else {
> if (CurCarCoordinate.x < 0) {
> bUpdataFsdObj2CalBoundaryFlag = TRUE;
> }
> }
> // data pt in slot;
> Data3Num = 0;
> for (Index = Obj1PtIndex + 1; Index < Obj2PtIndex; Index++) {
> pData3[Data3Num] = pMapMainSlotBord->Points[Index];
> NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
> Data3Num++;
> if (Data3Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSD Buffer Not enough==3==Data3Num:(%d)", Data3Num);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> }
> // Fus  Subborder
> MaxOffsetX = 3000;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt.x = TempPt.x;
> SubLinYStrPt.y = 0;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = 1000;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> // Fus  Subborder
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) &&
> (TRUE == bUpdataFsdObj2CalBoundaryFlag)) {
> MaxOffsetX = 6000;
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (TRUE == bUpdataFsdObj2CalBoundaryFlag)) {
> MaxOffsetX = 4000;
> } else {
> MaxOffsetX = 3000;
> }
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt1.x = TempPt.x;
> SubLinYStrPt1.y = 0;
> SubLinYEndPt1.x = SubLinYStrPt1.x;
> SubLinYEndPt1.y = 1000;
> SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
> SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);
> LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&LineXAngle);
> // Fus  Subborder Obj2 Line
> TempCarPos.CarAng = LineXAngle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(SubfDis, &TempLine1);
> TempCarPos.CarAng = Angle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
> SubLinXEndPt = SubLinXStrPt;
> SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
> SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);
> // Fus  Subborder Obj1 Line
> TempCarPos.CarAng = LineXAngle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[1];
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(-SubfDis, &TempLine1);
> TempCarPos.CarAng = Angle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[1];
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt1);
> SubLinXEndPt1 = SubLinXStrPt1;
> SubLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
> SubLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
> #if 0
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),"==APAMap_ParkingOutFusBoundaryByFSDMapInfo===SubLinXStrPt(%.2f,%.2f),SubLinXEndPt(%.2f,%.2f)=="
> "SubLinXStrPt1(%.2f,%.2f),SubLinXEndPt1(%.2f,%.2f)==SlotBordPt[0](%.2f,%.2f)==SlotBordPt[1](%.2f,%.2f)=="
> "SubLinYStrPt(%.2f,%.2f),SubLinYEndPt(%.2f,%.2f)==SubLinYStrPt1(%.2f,%.2f),SubLinYEndPt1(%.2f,%.2f)==bUpdataFsdObj2CalBoundaryFlag(%d)",
> SubLinXStrPt.x,
> SubLinXStrPt.y,
> SubLinXEndPt.x,
> SubLinXEndPt.y,
> SubLinXStrPt1.x,
> SubLinXStrPt1.y,
> SubLinXEndPt1.x,
> SubLinXEndPt1.y,
> APAMap_GInfo.SlotPar.SlotBordPt[0].x,
> APAMap_GInfo.SlotPar.SlotBordPt[0].y,
> APAMap_GInfo.SlotPar.SlotBordPt[1].x,
> APAMap_GInfo.SlotPar.SlotBordPt[1].y,
> SubLinYStrPt.x,
> SubLinYStrPt.y,
> SubLinYEndPt.x,
> SubLinYEndPt.y,
> SubLinYStrPt1.x,
> SubLinYStrPt1.y,
> SubLinYEndPt1.x,
> SubLinYEndPt1.y,
> bUpdataFsdObj2CalBoundaryFlag);
> TLOG_INFO << log_string;
> }
> #endif
> Data4Num = 0;
> for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
> pData4[Data4Num] = pMapSubSlotBord->Points[Index];
> NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
> Data4Num++;
> if (Data4Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSD Buffer Not enough==4==Data4Num:(%d)", Data4Num);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> }
> DataNumBk = 0;
> bCheckSubLane = FALSE;
> NSegNum = 0;
> i = 0;
> TopViewPtNum = pTopViewInfo->PointNum;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
> while (i < TopViewPtNum) {
> // get fsd data with same id;
> CurID = pTopViewInfo->InfoPoint[i].ID;
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.y;
> for (k = 1; k < 100; k++) {
> if ((i + k) < pTopViewInfo->PointNum) {
> if (pTopViewInfo->InfoPoint[i + k].ID != CurID) {
> break;
> } else {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
> .Point.x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
> .Point.y;
> }
> } else {
> break;
> }
> }
> DataNum = k;
> i += DataNum;
> //----------------------------------
> // Get valid fsd data for fus obj1bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, TempAng, &pData1[0], Data1Num,
> &NewProperty1[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData1[0], &Data1Num, &pPtStyle[0])) {
> // updata obj1 bordline;
> for (k = 0; k < Data1Num; k++) {
> NewProperty1[k] = pPtStyle[k];
> }
> if (Data1Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data1Num - 1; k++) {
> NewProperty1[k] = NewProperty1[k + 1];
> pData1[k] = pData1[k + 1];
> }
> Data1Num--;
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==FSDFusionObj1Success==");
> TLOG_INFO << log_string;
> }
> }
> //----------------------------------
> // Get valid fsd data for fus obj2bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt3, &MainLinXEndPt3, &TempPt, &fDis);
> if (((LocStyle != 0) && (bUpdataFsdObj2CalBoundaryFlag == TRUE)) ||
> (bUpdataFsdObj2CalBoundaryFlag == FALSE)) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, TempAng, &pData2[0], Data2Num,
> &NewProperty2[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData2[0], &Data2Num, &pPtStyle[0])) {
> // updata obj2 bordline;
> for (k = 0; k < Data2Num; k++) {
> NewProperty2[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==FSDFusionObj2Success==");
> TLOG_INFO << log_string;
> }
> //----------------------------------
> // Get valid fsd data for fus Subbordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt1, &SubLinXEndPt1, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> } else {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> } else {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, !bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
> for (k = 0; k < Data4Num; k++) {
> pDataBk[k] = pData4[k];
> }
> DataNumBk = Data4Num;
> }
> bFusvalid = FALSE;
> if (TRUE == APAMap_FusTwoLineSegments(
> !bSlotDataAtRigthSide, TempAng1, &pData4[0], Data4Num,
> &NewProperty4[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData4[0], &Data4Num, &pPtStyle[0])) {
> if (bCheckSubLane == TRUE) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &pData4[0], Data4Num,
> pRectPt, pRectLine)) {
> for (k = 0; k < DataNumBk; k++) {
> pData4[k] = pDataBk[k];
> }
> Data4Num = DataNumBk;
> } else {
> bFusvalid = TRUE;
> }
> } else {
> bFusvalid = TRUE;
> }
> }
> if (bFusvalid == TRUE) {
> // updata sublane;
> for (k = 0; k < Data4Num; k++) {
> NewProperty4[k] = pPtStyle[k];
> }
> if (Data4Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data4Num - 1; k++) {
> NewProperty4[k] = NewProperty4[k + 1];
> pData4[k] = pData4[k + 1];
> }
> Data4Num--;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==FSDFusionSubLaneSuccess==");
> TLOG_INFO << log_string;
> }
> }
> DataNum = Data1Num + Data3Num + Data2Num;
> if (DataNum <= FSD_BOUNDARY_PT_MAX_NUM) {
> for (Index = 0; Index < DataNum; Index++) {
> if (Index < Data1Num) {
> pMapMainSlotBord->Points[Index] = pData1[Index];
> pMapMainSlotBord->Property[Index] = NewProperty1[Index];
> } else if (Index < Data1Num + Data3Num) {
> pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
> pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];
> } else {
> pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
> pMapMainSlotBord->Property[Index] =
> NewProperty2[Index - Data1Num - Data3Num];
> }
> }
> pMapMainSlotBord->PtNum = DataNum;
> APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
> APAMap_GInfo.SlotPar.SlotStrIndex =
> APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
> APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
> APAMap_GInfo.SlotPar.SlotEndIndex =
> APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSDFusionMainSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> if (Data4Num <= FSD_BOUNDARY_PT_MAX_NUM) {
> for (Index = 0; Index < Data4Num; Index++) {
> pMapSubSlotBord->Points[Index] = pData4[Index];
> pMapSubSlotBord->Property[Index] = NewProperty4[Index];
> }
> pMapSubSlotBord->PtNum = Data4Num;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSDFusionSubSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FusBordByFSD==SlotIndex(%d,%d,%d,%d)==Offset(%d,%d))",
> APAMap_GInfo.SlotPar.Obj1PtIndex,
> APAMap_GInfo.SlotPar.SlotStrIndex,
> APAMap_GInfo.SlotPar.SlotEndIndex,
> APAMap_GInfo.SlotPar.Obj2PtIndex, OffsetIndex1, OffsetIndex2);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> void APAMap_ParkingOutLineParABCbyPoints(
> APACoordinateDataCalFloatType* NSegmentFilter, APA_ENUM_TYPE NSegNum,
> APALineParameterKBType* pLinePar) {
> // 函数根据多个点拟合一条直线
> APA_DISTANCE_CAL_FLOAT_TYPE sumX, sumY, sumXY, sumX2;
> APA_DISTANCE_TYPE i;
> APA_DISTANCE_CAL_FLOAT_TYPE denominator;
> sumX = 0.0;
> sumY = 0.0;
> sumXY = 0.0;
> sumX2 = 0.0;
> for (i = 0; i < NSegNum; i++) {
> sumX += NSegmentFilter[i].x;
> sumY += NSegmentFilter[i].y;
> sumXY += NSegmentFilter[i].x * NSegmentFilter[i].y;
> sumX2 += NSegmentFilter[i].x * NSegmentFilter[i].x;
> }
> denominator = (NSegNum * sumX2) - (sumX * sumX);
> if (denominator < 0.01) {
> pLinePar->K = (APA_DISTANCE_CAL_FLOAT_TYPE)0.0;
> pLinePar->B = (APA_DISTANCE_CAL_FLOAT_TYPE)0.0;
> return;
> }
> pLinePar->K = (APA_DISTANCE_CAL_FLOAT_TYPE)((NSegNum * sumXY - sumX * sumY) /
> denominator);
> pLinePar->B = (APA_DISTANCE_CAL_FLOAT_TYPE)((sumY * sumX2 - sumX * sumXY) /
> denominator);
> return;
> }
> #if 1
> BOOLEAN APAMap_ParkingOutFusBoundaryByLaneLineMapInfo() {
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_DISTANCE_TYPE i, j, m, n;
> APACoordinateDataCalFloatType temp;
> APACoordinateDataCalFloatType temp2;
> APA_ENUM_TYPE k;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType NSegment[127];
> APACoordinateDataCalFloatType NSegmentFilter[127];
> APACoordinateDataCalFloatType NSegmentFilter2[127];
> APA_ENUM_TYPE NSegNum;
> APA_ENUM_TYPE DataNum;
> st_MapLaneLine* pLaneLineInfo;
> Pt_Cnt_u16_t LaneLinePtNum;
> APA_ENUM_TYPE LocStyle;
> UCHAR CurID;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> uint8_t_INF ParkOutMode;
> APALineParameterABCType EndPosLine;
> APACarCoordinateDataCalFloatType TempCarPos;
> // APALineParameterABCType TempLine1, TempLine2;
> APACoordinateDataCalFloatType Obj2Pt;
> APACoordinateDataCalFloatType MainSlotBordTemp5;
> APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLength;
> APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLengthPre;
> APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLengthTemp;
> BOOLEAN bEndCarPosOnTheLeftOfNewSysAngFlag;
> BOOLEAN bLaneLineUpdatePerpFlag;
> BOOLEAN bInclineSlotChangeEndCarPosFlag;  // 斜列式车位更改终点位置标志位
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> BOOLEAN bUpdataLaneLineFlag;
> APA_DISTANCE_CAL_FLOAT_TYPE DebugLaneLine;
> APA_DISTANCE_CAL_FLOAT_TYPE DebugLaneLine2;
> APALineParameterKBType LaneLineKBType;
> static APALineParameterABCType LaneLineABCType;
> APA_DISTANCE_CAL_FLOAT_TYPE PointToLineDis;
> APACoordinateDataCalFloatType DataDebug[127];
> APACoordinateDataCalFloatType DataDebug2[127];
> static BOOLEAN bFindLaneLineFlagByPoints = FALSE;  // 找到车道线标志位
> static BOOLEAN bFindLaneLineFlagByLine = FALSE;    // 找到车道线标志位
> BOOLEAN bSearch;
> static APACoordinateDataCalFloatType MainSlotBordLaneLine1;
> static APACoordinateDataCalFloatType MainSlotBordLaneLine2;
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> // return TRUE;
> }
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> return TRUE;
> }
> #endif
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return TRUE;
> }
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> return TRUE;
> }
> if (TRUE == bLaneLineUpdateEndCarPosFlag) {
> return TRUE;
> }
> // 有车位框且车位框类型为斜列、阶梯斜列、水平的场景平行车位框closeline
> if ((TRUE == bCarryOutSlot) &&
> ((TRUE == bLabelAngledFlag) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL))) {
> return TRUE;
> }
> if (FALSE == bAfterNewAnchorPointFlag) {
> MainSlotBordLaneLine1.x = 0;
> MainSlotBordLaneLine1.y = 0;
> MainSlotBordLaneLine2.x = 0;
> MainSlotBordLaneLine2.y = 0;
> }
> pLaneLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.LaneLineInfo;
> MaxOffsetX = -4000;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> CurCarPos = APAMap_GInfo.CarPos;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> // Mainborder
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // LineYAngle = Angle;
> // Subborder
> MaxOffsetX = 5000;
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarPos.Coordinate, 0, Angle, Pto);
> TempDis = APAMap_ComCfg.HalfWidthOfCar;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x -= ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
> if (TempPt.x > -MaxOffsetX) {
> TempPt.x = -MaxOffsetX;
> }
> } else {
> TempPt.x += ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
> if (TempPt.x < MaxOffsetX) {
> TempPt.x = MaxOffsetX;
> }
> }
> SubLinYStrPt.x = TempPt.x;
> SubLinYStrPt.y = 0;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = 1000;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> for (i = 0; i < 2; i++) {
> DataDebug[i].x = 0;
> DataDebug[i].y = 0;
> DataDebug2[i].x = 0;
> DataDebug2[i].y = 0;
> }
> i = 0;
> DataNum = 0;
> LaneLinePtNum = pLaneLineInfo->PointNum;
> DebugLaneLine = 0;
> DebugLaneLine2 = 0;
> LaneLineLength = 0;
> LaneLineLengthPre = 0;
> LaneLineLengthTemp = 0;
> MainSlotBordTemp5.x = 0;
> MainSlotBordTemp5.y = 0;
> TempCarPos.CarAng = 0;
> NSegNum = 0;
> bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
> bLaneLineUpdatePerpFlag = FALSE;
> bInclineSlotChangeEndCarPosFlag = FALSE;
> #if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
> pLaneLineInfo->PointNum = 4;
> pLaneLineInfo->InfoPoint[0].Point.x = -1600;
> pLaneLineInfo->InfoPoint[0].Point.y = 0;
> pLaneLineInfo->InfoPoint[0].ID = 0;
> pLaneLineInfo->InfoPoint[1].Point.x = -2500;
> pLaneLineInfo->InfoPoint[1].Point.y = 2000;
> pLaneLineInfo->InfoPoint[1].ID = 0;
> pLaneLineInfo->InfoPoint[2].Point.x = -2500;
> pLaneLineInfo->InfoPoint[2].Point.y = 2100;
> pLaneLineInfo->InfoPoint[2].ID = 0;
> pLaneLineInfo->InfoPoint[3].Point.x = -1600;
> pLaneLineInfo->InfoPoint[3].Point.y = 4000;
> pLaneLineInfo->InfoPoint[3].ID = 0;
> LaneLinePtNum = pLaneLineInfo->PointNum;
> #endif
> bUpdataLaneLineFlag = FALSE;
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -2) {
> bUpdataLaneLineFlag = TRUE;
> DebugLaneLine += 100000;
> }
> } else {
> if (CurCarCoordinateX > 0) {
> bUpdataLaneLineFlag = TRUE;
> DebugLaneLine += 100000;
> }
> }
> }
> while (i < LaneLinePtNum) {
> // get LaneLine data with same id;
> CurID = pLaneLineInfo->InfoPoint[i].ID;
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i].Point.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i].Point.y;
> for (k = 1; k < 100; k++) {
> if ((i + k) < pLaneLineInfo->PointNum) {
> if (pLaneLineInfo->InfoPoint[i + k].ID != CurID) {
> break;
> } else {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i + k]
> .Point.x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i + k]
> .Point.y;
> }
> } else {
> break;
> }
> }
> DataNum = k;
> i += DataNum;
> if (DataNum < 2) {
> DataNum = 0;
> }
> //----------------------------------
> // Get valid LaneLine data;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> DebugLaneLine += 1;
> }
> }
> }
> // APA转锚点坐标系下
> for (m = 0; m < NSegNum; m++) {
> NSegment[m] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> NSegment[m], 0, Angle, Pto);
> }
> // 第二层数据过滤
> n = 0;
> for (m = 0; m < NSegNum; m++) {
> // 取锚点坐标系下y轴坐标小于-1m的车道线归类为次车道线（SubLaneLine），大于-1m的车道线归类为主车道线（MainLaneLine）；
> // 只对主车道线的数据做处理，次车道线的数据不参与处理，最后只平行主车道线。
> if ((NSegment[m].y < -1000) || (NSegment[m].y > 10000)) {
> } else {
> NSegmentFilter[n] = NSegment[m];
> n++;
> DebugLaneLine += 10;
> }
> }
> NSegNum = n;
> if (NSegNum < 2) {
> NSegNum = 0;
> }
> for (m = 0; m < NSegNum; m++) {
> DataDebug[m] = NSegmentFilter[m];  // 锚点坐标系下
> DataDebug2[m] = AlgCom_PointPosWithAngAndCenterPt(
> NSegmentFilter[m], Angle,
> Pto);  // Debug2转到与锚点相同的坐标系下（可能APA坐标系也可能锚点坐标系）
> }
> //----------------------------------
> // zqf:LaneLine update EndCarPos
> if ((FALSE == bLaneLineUpdateEndCarPosFlag) && (NSegNum >= 2) &&
> (FALSE == bUpdataLaneLineFlag)) {
> DebugLaneLine += 100;
> m = 0;
> LaneLineLength = 0;
> bSearch = TRUE;
> // 首先搜索是否有大于1m的车道线
> while (bSearch) {
> temp = NSegmentFilter[m];
> temp2 = NSegmentFilter[m + 1];
> LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> temp.x, temp.y, temp2.x, temp2.y);
> if (LaneLineLength > 1500) {
> break;
> }
> if (m >= NSegNum - 2) {
> bSearch = FALSE;
> break;
> }
> m += 2;
> }
> if (TRUE == bSearch) {
> // 其次搜索距离锚点最近的车道线，且车位线长度大于1.5m
> for (n = m; n < NSegNum - 1; n++) {
> if (NSegmentFilter[n].y < temp.y) {
> LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> NSegmentFilter[n].x, NSegmentFilter[n].y,
> NSegmentFilter[n + 1].x, NSegmentFilter[n + 1].y);
> if (LaneLineLength > 1500) {
> temp = NSegmentFilter[n];
> temp2 = NSegmentFilter[n + 1];
> }
> }
> n++;
> }
> if (temp.y > temp2.y) {
> MainSlotBordLaneLine1 = temp2;
> MainSlotBordLaneLine2 = temp;
> } else {
> MainSlotBordLaneLine1 = temp;
> MainSlotBordLaneLine2 = temp2;
> }
> bFindLaneLineFlagByPoints = TRUE;
> DebugLaneLine += 1000;
> }
> if (FALSE == bFindLaneLineFlagByPoints) {
> // 冒泡排序
> for (m = 0; m < NSegNum - 1; m++) {
> for (n = 0; n < NSegNum - m - 1; n++) {
> if (NSegmentFilter[n].y > NSegmentFilter[n + 1].y) {
> // 交换元素
> temp = NSegmentFilter[n];
> NSegmentFilter[n] = NSegmentFilter[n + 1];
> NSegmentFilter[n + 1] = temp;
> }
> }
> }
> // NSegmentFilter[0].x = NSegmentFilter[NSegNum-1].x + 500; // for test
> // 先默认取同个ID下的第一个点和最后一个点构成车道线
> LaneLineLengthPre = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> NSegmentFilter[0].x, NSegmentFilter[0].y,
> NSegmentFilter[NSegNum - 1].x, NSegmentFilter[NSegNum - 1].y);
> MainSlotBordLaneLine1 = NSegmentFilter[0];
> MainSlotBordLaneLine2 = NSegmentFilter[NSegNum - 1];
> for (j = 0; j < NSegNum; j++) {
> LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> NSegmentFilter[0].x, NSegmentFilter[0].y, NSegmentFilter[j].x,
> NSegmentFilter[j].y);
> if (LaneLineLength > LaneLineLengthPre) {
> LaneLineLengthPre = LaneLineLength;
> MainSlotBordLaneLine2 = NSegmentFilter[j];
> }
> }
> LaneLineLengthTemp = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y,
> MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y);
> if (LaneLineLengthTemp > 2000) {
> bFindLaneLineFlagByLine = TRUE;
> DebugLaneLine += 10000;
> }
> // 多个点先拟合出第一条直线，然后过滤掉超出与直线1m范围的点，再重新拟合第二条直线，以新的直线获取角度存储
> if (TRUE == bFindLaneLineFlagByLine) {
> APAMap_ParkingOutLineParABCbyPoints(&NSegmentFilter[0], NSegNum,
> &LaneLineKBType);
> LaneLineABCType = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> n = 0;
> for (m = 0; m < NSegNum; m++) {
> PointToLineDis =
> AlgCom_GetPointToLineDis(NSegmentFilter[m], LaneLineABCType);
> if (PointToLineDis <= 1000) {
> NSegmentFilter2[n] = NSegmentFilter[m];
> n++;
> }
> }
> if (n >= 2) {
> NSegNum = n;
> APAMap_ParkingOutLineParABCbyPoints(&NSegmentFilter2[0], NSegNum,
> &LaneLineKBType);
> LaneLineABCType = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> }
> }
> }
> }
> }
> if ((FALSE == bLaneLineUpdateEndCarPosFlag) &&
> (TRUE == bUpdataLaneLineFlag) &&
> ((TRUE == bFindLaneLineFlagByLine) ||
> (TRUE == bFindLaneLineFlagByPoints))) {
> DebugLaneLine2 += 1;
> if (TRUE == bFindLaneLineFlagByLine) {
> EndPosLine = LaneLineABCType;
> } else {
> EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(
> &MainSlotBordLaneLine1, &MainSlotBordLaneLine2);
> }
> if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
> DebugLaneLine2 += 10;
> return TRUE;
> }
> // zqf: update EndCarPos
> // TempCarPos.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> // TempCarPos.Coordinate = APAMap_GInfo.SlotPar.EndPos.Coordinate;
> // TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (MATH_ATAN(EndPosLine.A) >= 0) {
> DebugLaneLine2 += 100;
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
> } else {
> DebugLaneLine2 += 1000;
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
> }
> if (EndPosLine.LineType != APALineIsIncline) {
> DebugLaneLine2 += 10000;
> TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
> }
> // TempCarPos.Coordinate = MainSlotBordLaneLine1;
> // TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> // TempLine1 = LaneLineABCType;
> // AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2,
> // &MainSlotBordLaneLine2);
> if (APAMap_GInfo.SlotPar.EndPos.CarAng != TempCarPos.CarAng)
> // && (APAMap_GInfo.SlotPar.EndPos.Coordinate.x != MainSlotBordLaneLine1.x))
> {
> DebugLaneLine2 += 100000;
> if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) > (M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> (5 * M_PI / 16)) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (FALSE == bCarryOutSlot)) {
> DebugLaneLine2 += 1000000;
> bEndCarPosOnTheLeftOfNewSysAngFlag = TRUE;
> bLaneLineUpdateEndCarPosFlag = TRUE;
> } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> -(M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
> -(5 * M_PI / 16)) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (FALSE == bCarryOutSlot)) {
> DebugLaneLine2 += 10000000;
> bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
> bLaneLineUpdateEndCarPosFlag = TRUE;
> } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
> -(M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> (M_PI / 16))) {
> DebugLaneLine2 += 100000000;
> bLaneLineUpdateEndCarPosFlag = TRUE;
> bLaneLineUpdatePerpFlag = TRUE;
> }
> if (TRUE == bLabelAngledFlag) {
> DebugLaneLine2 += 1000000000;
> bLaneLineUpdateEndCarPosFlag = FALSE;
> bLaneLineUpdatePerpFlag = FALSE;
> }
> }
> if (TRUE == bLaneLineUpdateEndCarPosFlag) {
> APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
> APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
> InclineSlotOffsetX = 3000;
> InclineSlotOffsetY = 2000;
> MainSlotBordTemp5.x = 1500;
> MainSlotBordTemp5.y = 0;
> if (bSlotDataAtRigthSide == TRUE) {
> MainSlotBordTemp5.x = -MainSlotBordTemp5.x;
> }
> if ((MATH_FABS(MATH_FABS(APAMap_GInfo.SlotPar.Obj2Ang -
> APAMap_GInfo.NewCordSysAng) -
> M_PI_2) > (M_PI / 8)) ||
> (TRUE == bLaneLineUpdatePerpFlag)) {
> {
> char log_string[1024];
> snprintf(
> log_string, sizeof(log_string),
> "==LaneLineUpdate==Obj_Label_Angled_Slot==LaneLinePtNum(%d)\n"
> "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
> "bLaneLineUpdateEndCarPosFlag(%d)==LaneLineABCType(%.2f,%.2f,%."
> "2f)==TempCarPos.CarAng(%.2f)\n"
> "==MainSlotBordLaneLine2(%.2f,%.2f)==MainSlotBordLaneLine1(%.2f,%"
> ".2f)==DebugLaneLine(%.2f)==DebugLaneLine2(%.2f)==Obj2Ang(%.2f)=="
> "LaneLineLengthTemp(%.2f)"
> "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2["
> "0](%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)",
> LaneLinePtNum, bEndCarPosOnTheLeftOfNewSysAngFlag,
> bLaneLineUpdateEndCarPosFlag, LaneLineABCType.A,
> LaneLineABCType.B, LaneLineABCType.C, TempCarPos.CarAng,
> MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y,
> MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y, DebugLaneLine,
> DebugLaneLine2, APAMap_GInfo.SlotPar.Obj2Ang, LaneLineLengthTemp,
> DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
> DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
> DataDebug2[1].y);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
> if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) {
> if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = -InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
> if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
> if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = -InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> }
> APAMap_GInfo.SlotPar.EndPos.Coordinate = MainSlotBordTemp5;
> }
> }
> {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==LaneLineUpdate==LaneLinePtNum(%d)=="
> "bInclineSlotChangeEndCarPosFlag(%d)\n"
> "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
> "bLaneLineUpdateEndCarPosFlag(%d)==MainSlotBordTemp5(%.2f,%.2f)=="
> "TempCarPos.CarAng(%.2f)\n"
> "==MainSlotBordLaneLine2(%.2f,%.2f)==MainSlotBordLaneLine1(%.2f,%."
> "2f)==DebugLaneLine(%.2f)==DebugLaneLine2(%.2f)\n"
> "==LaneLineUpdate==TempCarPos.CarAng(%.2f)==NSegNum(%d)=="
> "bFindLaneLineFlagByLine(%d)==bFindLaneLineFlagByPoints(%d)=="
> "LaneLineABCType.A(%.2f)"
> "==LaneLineABCType.B(%.2f)==LaneLineABCType.C(%.2f)=="
> "LaneLineLengthTemp(%.2f)==LaneLineLengthPre(%.2f)=="
> "LaneLineLength(%.2f)"
> "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2[0]"
> "(%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)",
> LaneLinePtNum, bInclineSlotChangeEndCarPosFlag,
> bEndCarPosOnTheLeftOfNewSysAngFlag, bLaneLineUpdateEndCarPosFlag,
> MainSlotBordTemp5.x, MainSlotBordTemp5.y, TempCarPos.CarAng,
> MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y,
> MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y, DebugLaneLine,
> DebugLaneLine2, TempCarPos.CarAng, NSegNum,
> bFindLaneLineFlagByLine, bFindLaneLineFlagByPoints,
> LaneLineABCType.A, LaneLineABCType.B, LaneLineABCType.C,
> LaneLineLengthTemp, LaneLineLengthPre, LaneLineLength,
> DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
> DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
> DataDebug2[1].y);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> #endif
> #if 1
> BOOLEAN APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() {
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_DISTANCE_TYPE i, j, m, n;
> APACoordinateDataCalFloatType temp;
> APACoordinateDataCalFloatType temp2;
> APA_ENUM_TYPE k;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType NSegment[127];
> APACoordinateDataCalFloatType NSegmentFilter[127];
> APA_ENUM_TYPE NSegNum;
> APA_ENUM_TYPE LeftDataNum;
> APA_ENUM_TYPE RightDataNum;
> APA_DISTANCE_TYPE RefercLinePtNum;
> APA_DISTANCE_TYPE LeftRefercLinePtNum;
> APA_DISTANCE_TYPE RightRefercLinePtNum;
> APA_ENUM_TYPE LocStyle;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> uint8_t_INF ParkOutMode;
> APALineParameterABCType EndPosLine;
> APACarCoordinateDataCalFloatType TempCarPos;
> // APALineParameterABCType TempLine1, TempLine2;
> APACoordinateDataCalFloatType Obj2Pt;
> APACoordinateDataCalFloatType MainSlotBordTemp5;
> APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE RefercLineLength;
> APA_DISTANCE_CAL_FLOAT_TYPE RefercLineLengthPre;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> BOOLEAN bEndCarPosOnTheLeftOfNewSysAngFlag;
> BOOLEAN bRefercLineUpdatePerpFlag;
> BOOLEAN bUpdataRefercLineFlag;
> BOOLEAN bInclineSlotChangeEndCarPosFlag;  // 斜列式车位更改终点位置标志位
> plf_RefercLineInfo* pRefercLineInfo;
> APA_DISTANCE_CAL_FLOAT_TYPE DebugRefercLine;
> APA_DISTANCE_CAL_FLOAT_TYPE DebugRefercLine2;
> APACoordinateDataCalFloatType DataDebug[127];
> APACoordinateDataCalFloatType DataDebug2[127];
> static APACoordinateDataCalFloatType MainSlotBordRefercLine1;
> static APACoordinateDataCalFloatType MainSlotBordRefercLine2;
> static BOOLEAN bFindRefercLineFlag = FALSE;  // 找到车位线标志位
> BOOLEAN bSearch;
> APA_DISTANCE_CAL_FLOAT_TYPE ValidDis;
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> // return TRUE;
> }
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> return TRUE;
> }
> #endif
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return TRUE;
> }
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> return TRUE;
> }
> if ((TRUE == bRefercLineUpdateEndCarPosFlag) || (TRUE == bLabelAngledFlag)) {
> return TRUE;
> }
> if (FALSE == bAfterNewAnchorPointFlag) {
> MainSlotBordRefercLine1.x = 0;
> MainSlotBordRefercLine1.y = 0;
> MainSlotBordRefercLine2.x = 0;
> MainSlotBordRefercLine2.y = 0;
> }
> pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.RefercLineInfo;
> MaxOffsetX = -2000;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> CurCarPos = APAMap_GInfo.CarPos;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> // Mainborder
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // LineYAngle = Angle;
> // Subborder
> MaxOffsetX = 3000;
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarPos.Coordinate, 0, Angle, Pto);
> TempDis = APAMap_ComCfg.HalfWidthOfCar;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x -= ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
> if (TempPt.x > -MaxOffsetX) {
> TempPt.x = -MaxOffsetX;
> }
> } else {
> TempPt.x += ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
> if (TempPt.x < MaxOffsetX) {
> TempPt.x = MaxOffsetX;
> }
> }
> SubLinYStrPt.x = TempPt.x;
> SubLinYStrPt.y = 0;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = 1000;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> for (i = 0; i < 2; i++) {
> DataDebug[i].x = 0;
> DataDebug[i].y = 0;
> DataDebug2[i].x = 0;
> DataDebug2[i].y = 0;
> }
> i = 0;
> LeftDataNum = 0;
> RightDataNum = 0;
> RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
> LeftRefercLinePtNum = pRefercLineInfo->stLeftRefercLineInfo.RefercLineNum;
> RightRefercLinePtNum = pRefercLineInfo->stRightRefercLineInfo.RefercLineNum;
> DebugRefercLine = 0;
> DebugRefercLine2 = 0;
> RefercLineLength = 0;
> RefercLineLengthPre = 0;
> MainSlotBordTemp5.x = 0;
> MainSlotBordTemp5.y = 0;
> TempCarPos.CarAng = 0;
> NSegNum = 0;
> bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
> bRefercLineUpdatePerpFlag = FALSE;
> bInclineSlotChangeEndCarPosFlag = FALSE;
> #if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
> RefercLinePtNum = 4;
> LeftRefercLinePtNum = 2;
> RightRefercLinePtNum = 2;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt1.fx = -100;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt1.fy = -7000;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt2.fx = 0;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt2.fy = -4500;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt1.fx = -100;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt1.fy = -4000;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt2.fx = 0;
> pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt2.fy = -2500;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt1.fx = 100;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt1.fy = 100;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt2.fx = 0;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt2.fy = 2500;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt1.fx = -100;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt1.fy = 2600;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt2.fx = 0;
> pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt2.fy = 5000;
> #endif
> while (i < RefercLinePtNum) {
> // zqf: get RefercLine data
> if (i < LeftRefercLinePtNum) {
> Data[LeftDataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fx;
> Data[LeftDataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fy;
> LeftDataNum++;
> Data[LeftDataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fx;
> Data[LeftDataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fy;
> LeftDataNum++;
> } else {
> if ((i - LeftRefercLinePtNum) < RightRefercLinePtNum) {
> if (LeftDataNum + RightDataNum > 127) {
> break;
> }
> Data[LeftDataNum + RightDataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt1.fx;
> Data[LeftDataNum + RightDataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt1.fy;
> RightDataNum++;
> Data[LeftDataNum + RightDataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt2.fx;
> Data[LeftDataNum + RightDataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt2.fy;
> RightDataNum++;
> }
> }
> i++;
> }
> bUpdataRefercLineFlag = FALSE;
> // 判断在锚点转换之后，且车辆已开出车位，则不再检测车位线
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -2) {
> bUpdataRefercLineFlag = TRUE;
> DebugRefercLine += 1;
> }
> } else {
> if (CurCarCoordinateX > 0) {
> bUpdataRefercLineFlag = TRUE;
> DebugRefercLine += 1;
> }
> }
> }
> for (i = 0; i < 2; i++) {
> //----------------------------------
> // Get valid LeftRefercLine data;
> if (i == 0) {
> if (LeftRefercLinePtNum == 0) {
> LeftDataNum = 0;
> }
> NSegNum = 0;
> for (k = 0; k < LeftRefercLinePtNum; k++) {
> m = k * 2;
> TempPt = Data[m];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = Data[m];
> NSegNum++;
> NSegment[NSegNum] = Data[m + 1];
> NSegNum++;
> DebugRefercLine += 10;
> }
> }
> }
> } else {
> if (RightRefercLinePtNum == 0) {
> RightDataNum = 0;
> }
> NSegNum = 0;
> for (k = LeftRefercLinePtNum; k < RefercLinePtNum; k++) {
> m = k * 2;
> TempPt = Data[m];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = Data[m];
> NSegNum++;
> NSegment[NSegNum] = Data[m + 1];
> NSegNum++;
> DebugRefercLine += 100;
> }
> }
> }
> }
> // APA转锚点坐标系下
> for (m = 0; m < NSegNum; m++) {
> NSegment[m] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> NSegment[m], 0, Angle, Pto);
> }
> // 第二层数据过滤
> n = 0;
> for (j = 0; j < (NSegNum / 2); j++) {
> m = j * 2;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> ValidDis = -8000;
> } else {
> ValidDis = 0;
> }
> // 取锚点坐标系下y轴坐标小于0m的车位线归类为次车位线（SubRefercLine），大于0m的车位线归类为主车位线（MainRefercLine）；
> // 只对主车位线的数据做处理，次车位线的数据不参与处理，最后只平行主车位线。
> if ((NSegment[m].y < ValidDis) || (NSegment[m].y > 10000) ||
> (NSegment[m + 1].y < ValidDis) || (NSegment[m + 1].y > 10000)) {
> } else {
> NSegmentFilter[n] = NSegment[m];
> n++;
> NSegmentFilter[n] = NSegment[m + 1];
> n++;
> DebugRefercLine += 1000;
> }
> }
> NSegNum = n;
> if (NSegNum < 2) {
> NSegNum = 0;
> }
> for (m = 0; m < NSegNum; m++) {
> DataDebug[m] = NSegmentFilter[m];  // 锚点坐标系下
> DataDebug2[m] = AlgCom_PointPosWithAngAndCenterPt(
> NSegmentFilter[m], Angle,
> Pto);  // Debug2转到与锚点相同的坐标系下（可能APA坐标系也可能锚点坐标系）
> }
> // NSegmentFilter[0].x = NSegmentFilter[NSegNum-1].x + 500; // for test
> //----------------------------------
> // zqf:RefercLine update EndCarPos
> if ((FALSE == bRefercLineUpdateEndCarPosFlag) && (NSegNum >= 2) &&
> (FALSE == bUpdataRefercLineFlag)) {
> DebugRefercLine += 10000;
> m = 0;
> RefercLineLength = 0;
> bSearch = TRUE;
> // 首先搜索是否有大于1.5m的车位线
> while (bSearch) {
> temp = NSegmentFilter[m];
> temp2 = NSegmentFilter[m + 1];
> RefercLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> temp.x, temp.y, temp2.x, temp2.y);
> if (RefercLineLength > 1500) {
> break;
> }
> if (m >= NSegNum - 2) {
> bSearch = FALSE;
> break;
> }
> m += 2;
> }
> if (TRUE == bSearch) {
> // 其次搜索距离锚点最近的车位线，且车位线长度大于1.5m
> for (n = m; n < NSegNum - 1; n++) {
> if (NSegmentFilter[n].y < temp.y) {
> RefercLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
> NSegmentFilter[n].x, NSegmentFilter[n].y,
> NSegmentFilter[n + 1].x, NSegmentFilter[n + 1].y);
> if (RefercLineLength > 1500) {
> temp = NSegmentFilter[n];
> temp2 = NSegmentFilter[n + 1];
> }
> }
> n++;
> }
> if (temp.y > temp2.y) {
> MainSlotBordRefercLine1 = temp2;
> MainSlotBordRefercLine2 = temp;
> } else {
> MainSlotBordRefercLine1 = temp;
> MainSlotBordRefercLine2 = temp2;
> }
> bFindRefercLineFlag = TRUE;
> DebugRefercLine += 100000;
> }
> }
> }
> if ((FALSE == bRefercLineUpdateEndCarPosFlag) &&
> (TRUE == bUpdataRefercLineFlag) && (TRUE == bFindRefercLineFlag)) {
> DebugRefercLine2 += 1;
> EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(
> &MainSlotBordRefercLine1, &MainSlotBordRefercLine2);
> // EndPosLine = RefercLineABCType;
> if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
> DebugRefercLine2 += 10;
> return TRUE;
> }
> // zqf: update EndCarPos
> // TempCarPos.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> // TempCarPos.Coordinate = APAMap_GInfo.SlotPar.EndPos.Coordinate;
> // TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (MATH_ATAN(EndPosLine.A) >= 0) {
> DebugRefercLine2 += 100;
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
> } else {
> DebugRefercLine2 += 1000;
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
> }
> if (EndPosLine.LineType != APALineIsIncline) {
> TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
> }
> // 判断车道线已更新，且更新的角度小于车位参考线准备更新的角度，则把车位参考线当成误判，直接return返回。
> if (TRUE == bLaneLineUpdateEndCarPosFlag) {
> if (MATH_FABS(APAMap_GInfo.SlotPar.EndPos.CarAng) <
> MATH_FABS(TempCarPos.CarAng)) {
> DebugRefercLine2 += 10000;
> return TRUE;
> }
> }
> // TempCarPos.Coordinate = MainSlotBordRefercLine1;
> // TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> // TempLine1 = RefercLineABCType;
> // AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2,
> // &MainSlotBordRefercLine1);
> if (APAMap_GInfo.SlotPar.EndPos.CarAng != TempCarPos.CarAng)
> // && (APAMap_GInfo.SlotPar.EndPos.Coordinate.x !=
> // MainSlotBordRefercLine1.x))
> {
> if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) > (M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> (5 * M_PI / 16)) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
> DebugRefercLine2 += 100000;
> bEndCarPosOnTheLeftOfNewSysAngFlag = TRUE;
> bRefercLineUpdateEndCarPosFlag = TRUE;
> } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> -(M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
> -(5 * M_PI / 16)) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
> DebugRefercLine2 += 1000000;
> bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
> bRefercLineUpdateEndCarPosFlag = TRUE;
> } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
> -(M_PI / 16)) &&
> ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
> (M_PI / 16))) {
> DebugRefercLine2 += 10000000;
> bRefercLineUpdateEndCarPosFlag = TRUE;
> bRefercLineUpdatePerpFlag = TRUE;
> }
> }
> if (TRUE == bRefercLineUpdateEndCarPosFlag) {
> APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
> APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
> InclineSlotOffsetX = 3000;
> InclineSlotOffsetY = 2000;
> MainSlotBordTemp5.x = 1500;
> MainSlotBordTemp5.y = 0;
> if (bSlotDataAtRigthSide == TRUE) {
> MainSlotBordTemp5.x = -MainSlotBordTemp5.x;
> }
> if ((MATH_FABS(MATH_FABS(APAMap_GInfo.SlotPar.Obj2Ang -
> APAMap_GInfo.NewCordSysAng) -
> M_PI_2) > (M_PI / 8)) ||
> (TRUE == bRefercLineUpdatePerpFlag)) {
> {
> char log_string[1024];
> snprintf(
> log_string, sizeof(log_string),
> "==RefercLineUpdate==Obj_Label_Angled_Slot==RefercLinePtNum(%d)\n"
> "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
> "bRefercLineUpdateEndCarPosFlag(%d)==TempCarPos.CarAng(%.2f)\n"
> "==MainSlotBordRefercLine2(%.2f,%.2f)==MainSlotBordRefercLine1(%."
> "2f,%.2f)==DebugRefercLine(%.2f)==DebugRefercLine2(%.2f)=="
> "Obj2Ang(%.2f)\n"
> "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2["
> "0](%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)==bCarryOutSlot(%d)",
> RefercLinePtNum, bEndCarPosOnTheLeftOfNewSysAngFlag,
> bRefercLineUpdateEndCarPosFlag, TempCarPos.CarAng,
> MainSlotBordRefercLine2.x, MainSlotBordRefercLine2.y,
> MainSlotBordRefercLine1.x, MainSlotBordRefercLine1.y,
> DebugRefercLine, DebugRefercLine2, APAMap_GInfo.SlotPar.Obj2Ang,
> DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
> DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
> DataDebug2[1].y, bCarryOutSlot);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
> if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) {
> if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = -InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
> if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
> if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
> MainSlotBordTemp5.x = -InclineSlotOffsetX;
> MainSlotBordTemp5.y = -InclineSlotOffsetY;
> bInclineSlotChangeEndCarPosFlag = TRUE;
> }
> }
> APAMap_GInfo.SlotPar.EndPos.Coordinate = MainSlotBordTemp5;
> }
> }
> {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==RefercLineUpdate==RefercLinePtNum(%d)=="
> "bInclineSlotChangeEndCarPosFlag(%d)\n"
> "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
> "bRefercLineUpdateEndCarPosFlag(%d)==MainSlotBordTemp5(%.2f,%.2f)="
> "=EndPos.CarAng(%.2f)\n"
> "==MainSlotBordRefercLine2(%.2f,%.2f)==MainSlotBordRefercLine1(%."
> "2f,%.2f)"
> "==DebugRefercLine(%.2f)==DebugRefercLine2(%.2f)==TempCarPos."
> "CarAng(%.2f)==NSegNum(%d)\n"
> "==RefercLineLengthPre(%.2f)==RefercLineLength(%.2f)"
> "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2[0]"
> "(%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)==bCarryOutSlot(%d)",
> RefercLinePtNum, bInclineSlotChangeEndCarPosFlag,
> bEndCarPosOnTheLeftOfNewSysAngFlag, bRefercLineUpdateEndCarPosFlag,
> MainSlotBordTemp5.x, MainSlotBordTemp5.y,
> APAMap_GInfo.SlotPar.EndPos.CarAng, MainSlotBordRefercLine2.x,
> MainSlotBordRefercLine2.y, MainSlotBordRefercLine1.x,
> MainSlotBordRefercLine1.y, DebugRefercLine, DebugRefercLine2,
> TempCarPos.CarAng, NSegNum, RefercLineLengthPre, RefercLineLength,
> DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
> DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x, DataDebug2[1].y,
> bCarryOutSlot);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> #else
> BOOLEAN APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() {
> APA_ENUM_TYPE ParkSide;
> APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_ENUM_TYPE SlotStrIndex;
> // APA_ENUM_TYPE SlotEndIndex;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_ENUM_TYPE Index;
> APA_DISTANCE_TYPE i;
> APA_ENUM_TYPE k;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> // APA_ENUM_TYPE OffsetIndex2,OffsetIndex1;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType pData1[127];
> APACoordinateDataCalFloatType pData2[127];
> APACoordinateDataCalFloatType pData3[127];
> APACoordinateDataCalFloatType pData4[127];
> APACoordinateDataCalFloatType NSegment[127];
> uint8_t_INF pPtStyle[127];
> uint8_t_INF NewProperty1[127];
> uint8_t_INF NewProperty2[127];
> // uint8_t_INF NewProperty3[127];
> uint8_t_INF NewProperty4[127];
> uint8_t_INF NSegProperty[127];
> APA_ENUM_TYPE NSegNum;
> uint16_t_INF DataNum;
> APA_ENUM_TYPE Data1Num;
> APA_ENUM_TYPE Data2Num;
> APA_ENUM_TYPE Data3Num;
> APA_ENUM_TYPE Data4Num;
> plf_RefercLineInfo* pRefercLineInfo;
> APA_DISTANCE_TYPE RefercLinePtNum;
> APA_DISTANCE_TYPE LeftRefercLinePtNum;
> APA_DISTANCE_TYPE RightRefercLinePtNum;
> APA_ENUM_TYPE LocStyle;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
> APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
> // APA_DISTANCE_CAL_FLOAT_TYPE LineYAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
> APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
> APACarCoordinateDataCalFloatType TempCarPos;
> APALineParameterABCType TempLine1;
> APALineParameterABCType TempLine2;
> BOOLEAN bCheckSubLane;
> APACoordinateDataCalFloatType TempPt1, TempPt2;
> APACoordinateDataCalFloatType pDataBk[127];
> APA_ENUM_TYPE DataNumBk;
> BOOLEAN bFusvalid;
> #if 1
> APALineParameterABCType EndPosLine;
> APACoordinateDataCalFloatType MainSlotBordTemp1;
> APACoordinateDataCalFloatType MainSlotBordTemp2;
> #endif
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> return TRUE;
> }
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return TRUE;
> }
> if (FALSE == bAfterNewAnchorPointFlag) {
> return TRUE;
> }
> pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.RefercLineInfo;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> APAMap_CheckIfIgnoreFSDPtAtMainBoundary();
> APAMap_CheckIfIgnoreFSDPtAtSubBoundary();
> MaxOffsetX = 600;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> // zqf:PARALLEL_SIDE
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> } else {
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> }
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
> CurCarPos = APAMap_GInfo.CarPos;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&TempAng);
> // CurCarPos.Coordinate =
> // AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CurCarPos.Coordinate,0,Angle,Pto);
> // CurCarPos.CarAng -= Angle;
> // SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> // OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
> // OffsetIndex2 = Obj2PtIndex - SlotEndIndex;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // LineYAngle = Angle;
> // obj 1 border line
> MainLinXStrPt1 = OrgObj1Pt;
> MainLinXEndPt1 = MainLinXStrPt1;
> LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
> MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
> Data1Num = 0;
> for (Index = 0; Index <= Obj1PtIndex; Index++) {
> pData1[Data1Num] = pMapMainSlotBord->Points[Index];
> NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
> Data1Num++;
> }
> // obj2 borderline;
> MainLinXStrPt2 = OrgObj2Pt;
> MainLinXEndPt2 = MainLinXStrPt2;
> LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
> MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
> Data2Num = 0;
> for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
> pData2[Data2Num] = pMapMainSlotBord->Points[Index];
> NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
> Data2Num++;
> }
> // data pt in slot;
> Data3Num = 0;
> for (Index = SlotStrIndex; Index < Obj2PtIndex; Index++) {
> pData3[Data3Num] = pMapMainSlotBord->Points[Index];
> // NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
> Data3Num++;
> }
> // Fus  Subborder
> MaxOffsetX = 3000;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt.x = TempPt.x;
> SubLinYStrPt.y = 0;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = 1000;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> // Fus  Subborder
> MaxOffsetX = 3000;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt1.x = TempPt.x;
> SubLinYStrPt1.y = 0;
> SubLinYEndPt1.x = SubLinYStrPt1.x;
> SubLinYEndPt1.y = 1000;
> SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
> SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);
> LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&LineXAngle);
> TempCarPos.CarAng = LineXAngle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
> if (TempLine1.C < TempLine2.C) {
> TempLine1.C = TempLine2.C;
> }
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
> if (TempLine1.C < TempLine2.C) {
> TempLine1.C = TempLine2.C;
> }
> TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(1000, &TempLine1);
> TempCarPos.CarAng = Angle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
> SubLinXEndPt = SubLinXStrPt;
> SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
> SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);
> Data4Num = 0;
> for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
> pData4[Data4Num] = pMapSubSlotBord->Points[Index];
> NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
> Data4Num++;
> }
> NSegNum = 0;
> i = 0;
> DataNum = 0;
> bCheckSubLane = FALSE;
> RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
> LeftRefercLinePtNum = pRefercLineInfo->stLeftRefercLineInfo.RefercLineNum;
> RightRefercLinePtNum = pRefercLineInfo->stRightRefercLineInfo.RefercLineNum;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
> while (i < RefercLinePtNum) {
> // zqf: get RefercLine data
> if (i < LeftRefercLinePtNum) {
> Data[DataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fx;
> Data[DataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fy;
> if (i > 0) {
> if (bSlotDataAtRigthSide == TRUE) {
> if (Data[DataNum].y < Data[DataNum - 1].y) {
> TempPt = Data[DataNum];
> Data[DataNum] = Data[DataNum - 1];
> Data[DataNum - 1] = TempPt;
> }
> } else {
> if (Data[DataNum].y > Data[DataNum - 1].y) {
> TempPt = Data[DataNum];
> Data[DataNum] = Data[DataNum - 1];
> Data[DataNum - 1] = TempPt;
> }
> }
> }
> DataNum++;
> Data[DataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fx;
> Data[DataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fy;
> DataNum++;
> } else {
> if ((i - LeftRefercLinePtNum) < RightRefercLinePtNum) {
> Data[DataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt1.fx;
> Data[DataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt1.fy;
> if (i - LeftRefercLinePtNum > 0) {
> if (bSlotDataAtRigthSide == TRUE) {
> if (Data[DataNum].y < Data[DataNum - 1].y) {
> TempPt = Data[DataNum];
> Data[DataNum] = Data[DataNum - 1];
> Data[DataNum - 1] = TempPt;
> }
> } else {
> if (Data[DataNum].y > Data[DataNum - 1].y) {
> TempPt = Data[DataNum];
> Data[DataNum] = Data[DataNum - 1];
> Data[DataNum - 1] = TempPt;
> }
> }
> }
> DataNum++;
> Data[DataNum].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt2.fx;
> Data[DataNum].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
> .stRefercLineParam[i - LeftRefercLinePtNum]
> .pt2.fy;
> DataNum++;
> }
> }
> i++;
> }
> //----------------------------------
> // Get valid fsd data for fus obj1bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_FusTwoLineSegments(bSlotDataAtRigthSide, TempAng,
> &pData1[0], Data1Num, &NewProperty1[0],
> &NSegment[0], NSegNum, &NSegProperty[0],
> &pData1[0], &Data1Num, &pPtStyle[0])) {
> // updata obj1 bordline;
> for (k = 0; k < Data1Num; k++) {
> NewProperty1[k] = pPtStyle[k];
> }
> if (Data1Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data1Num - 1; k++) {
> NewProperty1[k] = NewProperty1[k + 1];
> pData1[k] = pData1[k + 1];
> }
> Data1Num--;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==RefercLineFusionObj1Success==");
> TLOG_INFO << log_string;
> }
> //----------------------------------
> // Get valid fsd data for fus obj2bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_FusTwoLineSegments(bSlotDataAtRigthSide, TempAng,
> &pData2[0], Data2Num, &NewProperty2[0],
> &NSegment[0], NSegNum, &NSegProperty[0],
> &pData2[0], &Data2Num, &pPtStyle[0])) {
> // updata obj2 bordline;
> for (k = 0; k < Data2Num; k++) {
> NewProperty2[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==RefercLineFusionObj2Success==");
> TLOG_INFO << log_string;
> }
> //----------------------------------
> // Get valid fsd data for fus Subbordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> } else {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
> }
> if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
> for (k = 0; k < Data4Num; k++) {
> pDataBk[k] = pData4[k];
> }
> DataNumBk = Data4Num;
> }
> bFusvalid = FALSE;
> APAMap_ReOderSegmentPt(TRUE, !bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> if (TRUE == APAMap_FusTwoLineSegments(!bSlotDataAtRigthSide, TempAng,
> &pData4[0], Data4Num, &NewProperty4[0],
> &NSegment[0], NSegNum, &NSegProperty[0],
> &pData4[0], &Data4Num, &pPtStyle[0])) {
> if (bCheckSubLane == TRUE) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &pData4[0], Data4Num,
> pRectPt, pRectLine)) {
> for (k = 0; k < DataNumBk; k++) {
> pData4[k] = pDataBk[k];
> }
> Data4Num = DataNumBk;
> } else {
> bFusvalid = TRUE;
> }
> } else {
> bFusvalid = TRUE;
> }
> }
> if (bFusvalid == TRUE) {
> // updata sublane;
> for (k = 0; k < Data4Num; k++) {
> NewProperty4[k] = pPtStyle[k];
> }
> if (Data4Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data4Num - 1; k++) {
> NewProperty4[k] = NewProperty4[k + 1];
> pData4[k] = pData4[k + 1];
> }
> Data4Num--;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==RefercLineFusionSubLaneSuccess==");
> TLOG_INFO << log_string;
> }
> DataNum = Data1Num + Data3Num + Data2Num;
> #if 0
> if(DataNum <= BOUNDARY_PT_MAX_NUM)
> {
> for (Index = 0; Index < DataNum; Index++) {
> if (Index < Data1Num) {
> pMapMainSlotBord->Points[Index] = pData1[Index];
> pMapMainSlotBord->Property[Index] = NewProperty1[Index];
> } else if (Index < Data1Num + Data3Num) {
> pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
> pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];
> } else {
> pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
> pMapMainSlotBord->Property[Index] =
> NewProperty2[Index - Data1Num - Data3Num];
> }
> }
> pMapMainSlotBord->PtNum = DataNum;
> APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
> APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
> APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
> APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
> }else
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSDFusionMainSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> if(Data4Num <= BOUNDARY_PT_MAX_NUM)
> {
> for (Index = 0; Index < Data4Num; Index++) {
> pMapSubSlotBord->Points[Index] = pData4[Index];
> pMapSubSlotBord->Property[Index] = NewProperty4[Index];
> }
> pMapSubSlotBord->PtNum = Data4Num;
> }else
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSDFusionSubSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> #endif
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==FusBordByFSD==SlotIndex(%d,%d,%d,%d))",
> APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
> APAMap_GInfo.SlotPar.SlotEndIndex, APAMap_GInfo.SlotPar.Obj2PtIndex);
> TLOG_INFO << log_string;
> }
> #if 1
> // zqf:for diagonal parking out slot
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
> (FALSE == bRefercLineUpdateEndCarPosFlag) &&
> (FALSE == bLaneLineUpdateEndCarPosFlag)) {
> MainSlotBordTemp1 = pData2[0];
> MainSlotBordTemp2 = pData2[DataNum - 1 - Data1Num - Data3Num];
> // MainSlotBordTemp2.x += 500; // for test
> EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(&MainSlotBordTemp1,
> &MainSlotBordTemp2);
> if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
> return TRUE;
> }
> if (EndPosLine.LineType == APALineIsIncline) {
> if (MATH_ATAN(EndPosLine.A) >= 0) {
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
> } else {
> TempCarPos.CarAng =
> APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
> }
> APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
> APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
> bRefercLineUpdateEndCarPosFlag = TRUE;
> }
> }
> #endif
> return TRUE;
> }
> #endif
> void APAMap_ParkingOutUpDataMapInfo() {
> uint8_t_INF LBoundaryPtNum, RBoundaryPtNum;
> APA_ENUM_TYPE mode;
> BOOLEAN bUpdataCalBoundaryFlag;
> #ifdef APAMAP_PARKOUT_FUS_SDG
> APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus();
> #endif
> #ifdef APAMAP_PARKOUT_FUS_PDC
> APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus();
> #endif
> LBoundaryPtNum = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> RBoundaryPtNum = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> BoudaryNum[4][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[4][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> APAMap_ParkingOutUpDataMapInfoBySlotCorInfo();
> BoudaryNum[5][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[5][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> bUpdataCalBoundaryFlag = TRUE;
> APAMap_ParkingOutSideSlotInfo(&bUpdataCalBoundaryFlag);
> if (TRUE == bUpdataCalBoundaryFlag) {
> APAMap_ParkingOutCalBoundaryByParkOutInfo();
> BoudaryNum2[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum2[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> }
> APAMap_ParkingOutFusBoundaryByFSDMapInfo();
> BoudaryNum[6][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[6][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> if (TRUE == bAfterNewAnchorPointFlag) {
> APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
> BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
> BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> }
> APAMap_FusBoundaryByODMapInfo();
> BoudaryNum[7][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[7][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> mode = 0;
> if (LBoundaryPtNum == APAMap_GInfo.OutLine.LeftBoundary.PtNum) {
> mode |= 0x02;
> }
> if (RBoundaryPtNum == APAMap_GInfo.OutLine.RightBoundary.PtNum) {
> mode |= 0x01;
> }
> #ifdef APAMAP_PARKOUT_FUS_SDG
> APAMap_ParkingOutUpDataMapBoundaryBySDGInfo();
> APAMap_ParkingOutDeleteMainSlotBord();
> #endif
> #ifdef APAMAP_PARKOUT_FUS_PDC
> APAMap_ParkingOutUpDataMapBoundaryByPDCInfo();
> APAMap_ParkingOutDeleteMainSlotBord();
> #endif
> APAMap_SmoothMapBoundary(mode);
> BoudaryNum[8][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
> BoudaryNum[8][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
> APAMap_ParkingOutEndCarPosUpdata();
> return;
> }
> void APAMap_ParkingOutCalSlotSlotAlignInfo() {
> APACoordinateDataCalFloatType Obj2PtTemp, Obj1PtTemp;
> Obj2PtTemp = APAMap_GInfo.SlotPar.SlotBordPt[0];
> Obj1PtTemp = APAMap_GInfo.SlotPar.SlotBordPt[1];
> if (TRUE == bAfterNewAnchorPointFlag) {
> Obj1PtTemp.x = Obj2PtTemp.x - (APAMap_GInfo.SlotPar.SlotBordPt[3].x -
> APAMap_GInfo.SlotPar.SlotBordPt[2].x);
> }
> APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1PtTemp;
> }
> #ifdef SUPPORT_PARKING_OUT_UWB
> APACoordinateDataCalFloatType APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> uint8_t_INF ParkOutMode, APACoordinateDataCalFloatType OrgPt,
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, APACoordinateDataCalFloatType Obj2Pt,
> BOOLEAN bSeizeEndCarPosFlag) {
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE StraightOutDefaultEndPosOffsetX;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt2, TempPt3;
> APACarCoordinateDataCalFloatType TempCarPos;
> BOOLEAN bObj2Exist;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosX;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosY;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
> APALineParameterABCType TempLine;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetY;
> // zqf:according to RemoteControlPos set EndCarPos, New Coordinate
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
> APA_DISTANCE_CAL_FLOAT_TYPE RemoContPosToEndCarPosDisX;
> APA_DISTANCE_CAL_FLOAT_TYPE RemoContPosToEndCarPosDisY;
> APA_DISTANCE_CAL_FLOAT_TYPE LenBetweenFBumperAndRemoContPos;
> APACoordinateDataCalFloatType RemoContPosTemp;
> stCor2d_cm_s16_t RemoContPos;
> APA_ENUM_TYPE ParkSide;
> APACoordinateDataCalFloatType EndCarPos;
> APACoordinateDataCalFloatType EndCarPosTemp;
> APA_DISTANCE_CAL_FLOAT_TYPE EndPosCarAng;
> static BOOLEAN bUWBPosUpdataFlag = FALSE;  // 接收到UWB信号标志位
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> EndCarPosTemp = APAMap_GInfo.SlotPar.EndPos.Coordinate;
> RemoContPos = APAMap_GInputData.ParkReqPar.Parkout_UWBPos;  // 车身坐标系下
> CarWidth = APAMap_ComCfg.WidthOfCar;                        // mm
> CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;         // mm, 3000
> SafeDisCal = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 250mm, 0 paralIn;
> LenBetweenFBumperAndRemoContPos = 1000;             // mm
> RemoContPosToEndCarPosDisX = CarWidth / 2 + SafeDisCal;
> RemoContPosToEndCarPosDisY = CarLFCal - LenBetweenFBumperAndRemoContPos;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> ParkOutEightMode = AlgCom_GetParkOutEightMode(ParkOutMode, ParkSide);
> #if 0
> if ((FALSE == bUWBPosUpdataFlag)
> && (RemoContPos.x != NO_OBJ_DISTANCE))
> {
> RemoContPosTemp.x = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.x;//车身坐标系下
> RemoContPosTemp.y = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.y;
> if ((ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)
> || (ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT)
> || (ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT)
> || (ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT))
> {
> EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisX;
> EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisY;
> }
> else if ((ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)
> || (ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT))
> {
> EndCarPos.x = RemoContPosTemp.x - RemoContPosToEndCarPosDisY;
> EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisX;
> }
> else if ((ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT)
> || (ParkOutEightMode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT))
> {
> EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisY;
> EndCarPos.y = RemoContPosTemp.y + RemoContPosToEndCarPosDisX;
> }
> else{}
> EndCarPos = AlgCom_PointPosWithAngAndCenterPt(EndCarPos, CurCarPos.CarAng, CurCarPos.Coordinate);//车身转APA坐标系下
> EndCarPosTemp = EndCarPos;
> bUWBPosUpdataFlag = TRUE;
> }
> #else
> if ((FALSE == bUWBPosUpdataFlag) && (RemoContPos.x != NO_OBJ_DISTANCE)) {
> RemoContPosTemp.x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.x;  // APA坐标系下
> RemoContPosTemp.y = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.y;
> RemoContPosTemp = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> RemoContPosTemp, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
> EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisY;
> EndCarPos.y = RemoContPosTemp.y + RemoContPosToEndCarPosDisX;
> } else if (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> EndCarPos.x = RemoContPosTemp.x - RemoContPosToEndCarPosDisY;
> EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisX;
> } else if ((ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) ||
> (ParkOutEightMode ==
> APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT)) {
> EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisX;
> EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisY;
> } else {
> }
> EndCarPos = AlgCom_PointPosWithAngAndCenterPt(EndCarPos, OrgAng,
> OrgPt);  // 锚点转APA坐标系下
> EndCarPosTemp = EndCarPos;
> bUWBPosUpdataFlag = TRUE;
> }
> #endif
> if (FALSE == bUWBPosUpdataFlag) {
> TempPt3.x = 0xff;
> APAMAP_Setfailcause(101);
> return TempPt3;
> }
> StraightOutDefaultEndPosOffsetX = 500;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
> UpdateCntOffsetX = 0;
> UpdateCntOffsetY = 0;
> UpdateCntGoStraightOffsetX = 0;
> UpdateCntGoStraightOffsetY = 0;
> fDis1 = 0;
> TempPt2.x = 0;
> TempPt2.y = 0;
> if (TRUE == bSeizeEndCarPosFlag) {
> UpdateCntOffsetY = 100;
> UpdateCntOffsetX = 150;
> UpdateCntGoStraightOffsetX = 100;
> if (TRUE == bFsdFromMapMainAndSubSlotBordFlag)  // 入侵的边界点来自双边界
> {
> UpdateCntOffsetY = 500;
> if (TRUE == bFsdFromMapMainSlotBordFlag)  // 入侵的边界点来自主边界
> {
> UpdateCntGoStraightOffsetY = 100;
> if (((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> FALSE))  // fsd in the left of EndCarPos
> ||
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> TRUE)))  // fsd in the right of EndCarPos
> {
> UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
> }
> }
> } else  // 入侵来自单边边界
> {
> if (TRUE == bFsdFromMapSubSlotBordFlag)  // 入侵的边界点来自子边界
> {
> if (((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> UpdateCntOffsetX = -UpdateCntOffsetX;  // 往主边界靠拢
> }
> if (TRUE == bFsdFromMapMainSlotBordFlag)  // 入侵的边界点来自主边界
> {
> UpdateCntGoStraightOffsetX = -UpdateCntGoStraightOffsetX;
> UpdateCntGoStraightOffsetY = 100;
> if (((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> FALSE))  // fsd in the left of EndCarPos
> ||
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> TRUE)))  // fsd in the right of EndCarPos
> {
> UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
> }
> }
> }
> }
> if (TRUE == bUWBPosUpdataFlag) {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndCarPosTemp, 0, OrgAng, OrgPt);           // APA转锚点坐标系下
> DefaultEndPosY = TempPt2.y - UpdateCntOffsetY;  // 锚点坐标系下
> DefaultEndPosX = -(MATH_ABS(TempPt2.x)) - UpdateCntOffsetX;
> } else {
> DefaultEndPosY = 2000 - UpdateCntOffsetY;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> DefaultEndPosX = -(APAMap_ComCfg.HalfWidthOfCar + 950) - UpdateCntOffsetX;
> } else {
> if (TRUE == bLabelAngledFlag) {
> if (TRUE == bObjLabelLadderFlag) {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 2000) - UpdateCntOffsetX;
> } else {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 1100) - UpdateCntOffsetX;
> }
> DefaultEndPosY = 5000 - UpdateCntOffsetY;
> } else {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 3100) - UpdateCntOffsetX;
> DefaultEndPosY = 4500 - UpdateCntOffsetY;
> }
> }
> }
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> TempAng = CurCarPos.CarAng - OrgAng;
> AlgCom_AngNormalized(&TempAng);
> if (FALSE == bAfterNewAnchorPointFlag) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
> fDis1 = -(StraightOutDefaultEndPosOffsetX +
> APAMap_ComCfg.LenBetweenRAxisAndRBumper -
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> } else {
> fDis1 = -(StraightOutDefaultEndPosOffsetX +
> APAMap_ComCfg.LenBetweenRAxisAndFBumper -
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> }
> } else {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndCarPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> fDis1 = (TempPt2.x +
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> }
> EndPosCarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> EndPosCarAng += M_PI;
> }
> AlgCom_AngNormalized(&EndPosCarAng);
> TempAng1 = EndPosCarAng - OrgAng;
> AlgCom_AngNormalized(&TempAng1);
> TempCarPos.CarAng = TempAng1;
> TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarPos.Coordinate, 0, OrgAng, OrgPt);
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TempLine.LineType == APALineIsVertical) {
> TempPt3.x = 0xff;
> APAMAP_Setfailcause(101);
> return TempPt3;
> } else {
> TempPt3.x = fDis1;
> TempPt3.y = TempLine.A * TempPt3.x + TempLine.C -
> (UpdateCntGoStraightOffsetY * MATH_SIN(MATH_FABS(TempAng)));
> #if 1
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutSetEndCarPosInOldCorSys==2==fDis1(%.2f)=="
> "MATH_SIN(MATH_FABS(TempAng))(%.2f)"
> "==MATH_SIN(MATH_FABS(TempAng1))(%.2f)==TempPt3(%.2f,%.2f)=="
> "TempLine.A,C(%.2f,%.2f)"
> "==UpdateCntGoStraightOffsetX(%.2f)=="
> "UpdateCntGoStraightOffsetY(%.2f)",
> fDis1, MATH_SIN(MATH_FABS(TempAng)),
> MATH_SIN(MATH_FABS(TempAng1)), TempPt3.x, TempPt3.y,
> TempLine.A, TempLine.C, UpdateCntGoStraightOffsetX,
> UpdateCntGoStraightOffsetY);
> TLOG_INFO << log_string;
> }
> #endif
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);
> if (FALSE == bAfterNewAnchorPointFlag) {
> APAMap_GInfo.SlotPar.EndPos.CarAng = CurCarPos.CarAng;
> } else {
> APAMap_GInfo.SlotPar.EndPos.CarAng = EndPosCarAng;
> }
> }
> } else {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Obj2Pt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> // TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
> // OrgAng, OrgPt);
> if ((bObj2Exist == FALSE) || (TRUE == bAfterNewAnchorPointFlag) ||
> (TRUE == bUWBPosUpdataFlag)) {
> TempPt2.x = 0;
> }
> if (bSlotDataAtRigthSide == FALSE) {
> DefaultEndPosX = -DefaultEndPosX + TempPt2.x;
> } else {
> DefaultEndPosX = DefaultEndPosX + TempPt2.x;
> }
> TempPt3.y = DefaultEndPosY;  // 锚点坐标系下
> TempPt3.x = DefaultEndPosX;
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng,
> OrgPt);  // 锚点转APA坐标系下
> APAMap_GInfo.SlotPar.EndPos.CarAng = OrgAng;
> }
> if (FALSE == bSeizeEndCarPosFlag) {
> TempPt3 = EndCarPosTemp;
> }
> return TempPt3;
> }
> #endif
> APACoordinateDataCalFloatType APAMap_ParkingOutSetEndCarPosInOldCorSys(
> uint8_t_INF ParkOutMode, APACoordinateDataCalFloatType OrgPt,
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, APACoordinateDataCalFloatType Obj2Pt,
> BOOLEAN bSeizeEndCarPosFlag) {
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE StraightOutDefaultEndPosOffsetX;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt2, TempPt3;
> APACarCoordinateDataCalFloatType TempCarPos;
> BOOLEAN bObj2Exist;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosX;
> APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosY;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
> APALineParameterABCType TempLine;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetY;
> APACoordinateDataCalFloatType EndCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE EndPosCarAng;
> EndCarPos = APAMap_GInfo.SlotPar.EndPos.Coordinate;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> StraightOutDefaultEndPosOffsetX = 500;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
> UpdateCntOffsetX = 0;
> UpdateCntOffsetY = 0;
> UpdateCntGoStraightOffsetX = 0;
> UpdateCntGoStraightOffsetY = 0;
> fDis1 = 0;
> TempPt2.x = 0;
> TempPt2.y = 0;
> if (TRUE == bSeizeEndCarPosFlag) {
> UpdateCntOffsetY = 100;
> UpdateCntOffsetX = 150;
> UpdateCntGoStraightOffsetX = 100;
> if (TRUE == bFsdFromMapMainAndSubSlotBordFlag)  // 入侵的边界点来自双边界
> {
> UpdateCntOffsetY = 500;
> if (TRUE == bFsdFromMapMainSlotBordFlag)  // 入侵的边界点来自主边界
> {
> UpdateCntGoStraightOffsetY = 100;
> if (((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> FALSE))  // fsd in the left of EndCarPos
> ||
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> TRUE)))  // fsd in the right of EndCarPos
> {
> UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
> }
> }
> } else  // 入侵来自单边边界
> {
> if (TRUE == bFsdFromMapSubSlotBordFlag)  // 入侵的边界点来自子边界
> {
> if (((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> UpdateCntOffsetX = -UpdateCntOffsetX;  // 往主边界靠拢
> }
> if (TRUE == bFsdFromMapMainSlotBordFlag)  // 入侵的边界点来自主边界
> {
> UpdateCntGoStraightOffsetX = -UpdateCntGoStraightOffsetX;
> UpdateCntGoStraightOffsetY = 100;
> if (((bFsdInRightOfEndCarPosFlag ==
> TRUE)  // fsd in the right of EndCarPos
> && (bSlotDataAtRigthSide == FALSE)) ||
> ((bFsdInRightOfEndCarPosFlag ==
> FALSE)  // fsd in the left of EndCarPos
> && (bSlotDataAtRigthSide == TRUE))) {
> UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
> }
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> FALSE))  // fsd in the left of EndCarPos
> ||
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
> (bFsdInRightOfEndCarPosFlag ==
> TRUE)))  // fsd in the right of EndCarPos
> {
> UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
> }
> }
> }
> }
> if (FALSE == bAfterNewAnchorPointFlag) {
> DefaultEndPosY = 2000 - UpdateCntOffsetY;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> DefaultEndPosX = -(APAMap_ComCfg.HalfWidthOfCar + 950) - UpdateCntOffsetX;
> } else {
> if (TRUE == bLabelAngledFlag) {
> if (TRUE == bObjLabelLadderFlag) {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 2000) - UpdateCntOffsetX;
> } else {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 1100) - UpdateCntOffsetX;
> }
> DefaultEndPosY = 5000 - UpdateCntOffsetY;
> } else {
> DefaultEndPosX =
> -(APAMap_ComCfg.HalfWidthOfCar + 3100) - UpdateCntOffsetX;
> DefaultEndPosY = 4500 - UpdateCntOffsetY;
> }
> }
> } else {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndCarPos, 0, OrgAng, OrgPt);               // APA转锚点坐标系下
> DefaultEndPosY = TempPt2.y - UpdateCntOffsetY;  // 锚点坐标系下
> DefaultEndPosX = -(MATH_ABS(TempPt2.x)) - UpdateCntOffsetX;
> }
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> TempAng = CurCarPos.CarAng - OrgAng;
> AlgCom_AngNormalized(&TempAng);
> if (FALSE == bAfterNewAnchorPointFlag) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
> fDis1 = -(StraightOutDefaultEndPosOffsetX +
> APAMap_ComCfg.LenBetweenRAxisAndRBumper -
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> } else {
> fDis1 = -(StraightOutDefaultEndPosOffsetX +
> APAMap_ComCfg.LenBetweenRAxisAndFBumper -
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> }
> } else {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndCarPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> fDis1 = (TempPt2.x +
> (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
> }
> EndPosCarAng = APAMap_GInfo.SlotPar.Obj2Ang;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> EndPosCarAng += M_PI;
> }
> AlgCom_AngNormalized(&EndPosCarAng);
> TempAng1 = EndPosCarAng - OrgAng;
> AlgCom_AngNormalized(&TempAng1);
> TempCarPos.CarAng = TempAng1;
> TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> CurCarPos.Coordinate, 0, OrgAng, OrgPt);
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TempLine.LineType == APALineIsVertical) {
> TempPt3.x = 0xff;
> APAMAP_Setfailcause(101);
> return TempPt3;
> } else {
> TempPt3.x = fDis1;
> TempPt3.y = TempLine.A * TempPt3.x + TempLine.C -
> (UpdateCntGoStraightOffsetY * MATH_SIN(MATH_FABS(TempAng)));
> #if 1
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutSetEndCarPosInOldCorSys==2==fDis1(%.2f)=="
> "MATH_SIN(MATH_FABS(TempAng))(%.2f)"
> "==MATH_SIN(MATH_FABS(TempAng1))(%.2f)==TempPt3(%.2f,%.2f)=="
> "TempLine.A,C(%.2f,%.2f)"
> "==UpdateCntGoStraightOffsetX(%.2f)=="
> "UpdateCntGoStraightOffsetY(%.2f)",
> fDis1, MATH_SIN(MATH_FABS(TempAng)),
> MATH_SIN(MATH_FABS(TempAng1)), TempPt3.x, TempPt3.y,
> TempLine.A, TempLine.C, UpdateCntGoStraightOffsetX,
> UpdateCntGoStraightOffsetY);
> TLOG_INFO << log_string;
> }
> #endif
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);
> if (FALSE == bAfterNewAnchorPointFlag) {
> APAMap_GInfo.SlotPar.EndPos.CarAng = CurCarPos.CarAng;
> } else {
> APAMap_GInfo.SlotPar.EndPos.CarAng = EndPosCarAng;
> }
> }
> } else {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Obj2Pt, 0, OrgAng, OrgPt);
> // TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
> // OrgAng, OrgPt);
> if ((bObj2Exist == FALSE) || (TRUE == bAfterNewAnchorPointFlag)) {
> TempPt2.x = 0;
> }
> if (bSlotDataAtRigthSide == FALSE) {
> DefaultEndPosX = -DefaultEndPosX + TempPt2.x;
> } else {
> DefaultEndPosX = DefaultEndPosX + TempPt2.x;
> }
> TempPt3.y = DefaultEndPosY;
> TempPt3.x = DefaultEndPosX;
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);
> APAMap_GInfo.SlotPar.EndPos.CarAng = OrgAng;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutSetEndCarPosInOldCorSys==UpdateCntOffsetX(%."
> "2f)==UpdateCntOffsetY(%.2f)==fDis1(%.2f)"
> "==TempPt2(%.2f,%.2f)==TempPt3(%.2f,%.2f)==bSeizeEndCarPosFlag(%d)"
> "==bSlotDataAtRigthSide(%d)==DefaultEndPos(%.2f,%.2f)"
> "==bAfterNewAnchorPointFlag(%d)==UpdateCntGoStraightOffsetX(%.2f)",
> UpdateCntOffsetX, UpdateCntOffsetY, fDis1, TempPt2.x, TempPt2.y,
> TempPt3.x, TempPt3.y, bSeizeEndCarPosFlag, bSlotDataAtRigthSide,
> DefaultEndPosX, DefaultEndPosY, bAfterNewAnchorPointFlag,
> UpdateCntGoStraightOffsetX);
> TLOG_INFO << log_string;
> }
> return TempPt3;
> }
> BOOLEAN APAMap_ParkingOutCenterEndCarPosInfo() {
> APACoordinateDataCalFloatType Data[100];
> APA_ENUM_TYPE DataNum1, DataNum2;
> APACarCoordinateDataCalFloatType EndPos;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType TempPt2;
> APACoordinateDataCalFloatType MainSlotBordPoint;
> APACoordinateDataCalFloatType SubSlotBordPoint;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> APACoordinateDataCalFloatType Pto;
> uint8_t_INF ParkOutMode;
> APA_ENUM_TYPE ParkSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> APA_INDEX_TYPE i, k, m;
> APA_ENUM_TYPE DataNum;
> BOOLEAN bSlotDataAtRigthSide;
> BOOLEAN bSearch;
> APACoordinateDataCalFloatType OrgPt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> BOOLEAN bCenterEndCarPosFlag = FALSE;  // 采用终点位置居中标志位
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return FALSE;
> }
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
> (TRUE == bFsdFromMapMainAndSubSlotBordFlag) ||
> ((FALSE == bFsdFromMapMainSlotBordFlag) &&
> (FALSE == bFsdFromMapSubSlotBordFlag))) {
> return FALSE;
> }
> // zqf:PARALLEL_SIDE
> CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
> fDis = 0;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> fDis = CarWidth + 1700;
> if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> } else {
> fDis = CarWidth + 2300;
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> }
> EndPos = APAMap_GInfo.SlotPar.EndPos;
> Pto = EndPos.Coordinate;
> i = 0;
> m = 0;
> DataNum = 0;
> fDis2 = 0;
> DataNum1 = pMapMainSlotBord->PtNum;
> DataNum2 = pMapSubSlotBord->PtNum;
> bSearch = TRUE;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> fDis2 = 5000;
> } else {
> fDis2 = 7000;
> }
> MainSlotBordPoint.x = 0;
> MainSlotBordPoint.y = 0;
> if (TRUE == bSlotDataAtRigthSide) {
> SubSlotBordPoint.x = -fDis2;
> } else {
> SubSlotBordPoint.x = fDis2;
> }
> SubSlotBordPoint.y = 0;
> fDis2 = 0;
> bCenterEndCarPosFlag = FALSE;
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> while (bSearch) {
> if (m == 0) {
> if (i < DataNum1) {
> for (k = 0; k < DataNum1; k++) {
> // get MainBoundary data
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].y;
> }
> i = DataNum1;
> DataNum = DataNum1;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 1) {
> if (i < DataNum2) {
> for (k = 0; k < DataNum2; k++) {
> // get SubBoundary data
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
> }
> i = DataNum2;
> DataNum = DataNum2;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 0) {
> //----------------------------------
> // Main SlotBord data
> for (k = 0; k < DataNum; k++) {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if ((TempPt2.y >
> (TempPt.y - APAMap_ComCfg.LenBetweenRAxisAndRBumper - 1000)) &&
> (TempPt2.y <
> (TempPt.y + APAMap_ComCfg.LenBetweenRAxisAndFBumper + 1000))) {
> if ((TempPt2.x < MainSlotBordPoint.x) &&
> (TRUE == bSlotDataAtRigthSide)) {
> MainSlotBordPoint = TempPt2;
> } else if ((TempPt2.x > MainSlotBordPoint.x) &&
> (FALSE == bSlotDataAtRigthSide)) {
> MainSlotBordPoint = TempPt2;
> }
> }
> }
> } else if (m == 1) {
> //----------------------------------
> // Sub SlotBord data
> for (k = 0; k < DataNum; k++) {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if ((TempPt2.y >
> (TempPt.y - APAMap_ComCfg.LenBetweenRAxisAndRBumper - 2000)) &&
> (TempPt2.y <
> (TempPt.y + APAMap_ComCfg.LenBetweenRAxisAndFBumper + 2000))) {
> if ((TempPt2.x > SubSlotBordPoint.x) &&
> (TRUE == bSlotDataAtRigthSide)) {
> SubSlotBordPoint = TempPt2;
> } else if ((TempPt2.x < SubSlotBordPoint.x) &&
> (FALSE == bSlotDataAtRigthSide)) {
> SubSlotBordPoint = TempPt2;
> }
> }
> }
> }
> if (m == 2) {
> bSearch = FALSE;
> }
> if (MATH_FABS(MainSlotBordPoint.x - SubSlotBordPoint.x) > fDis) {
> bCenterEndCarPosFlag = TRUE;
> } else {
> bCenterEndCarPosFlag = FALSE;
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutCenterEndCarPosInfo==Main Sub SlotBord data=="
> "==TempPt(%.2f,%.2f)==MainSlotBordPoint(%.2f,%.2f)==SubSlotBordPoint("
> "%.2f,%.2f)==Dis(%.2f)==bCenterEndCarPosFlag(%d)",
> TempPt.x, TempPt.y, MainSlotBordPoint.x, MainSlotBordPoint.y,
> SubSlotBordPoint.x, SubSlotBordPoint.y,
> MATH_FABS(MainSlotBordPoint.x - SubSlotBordPoint.x),
> bCenterEndCarPosFlag);
> TLOG_INFO << log_string;
> }
> }
> if (TRUE == bCenterEndCarPosFlag) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> fDis2 = 300;
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> fDis2 = 500;
> }
> if (FALSE == bSlotDataAtRigthSide) {
> fDis2 = -fDis2;
> }
> TempPt2.x = (MainSlotBordPoint.x + SubSlotBordPoint.x) / 2 + fDis2;
> TempPt2.y = TempPt.y;
> EndPos.Coordinate =
> AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, OrgPt);
> APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutCenterEndCarPosInfo"
> "==MainSlotBordPoint(%.2f,%.2f)==SubSlotBordPoint(%.2f,%.2f)=="
> "EndPos.Coordinate(%.2f,%.2f)==TempPt2(%.2f,%.2f)",
> MainSlotBordPoint.x, MainSlotBordPoint.y, SubSlotBordPoint.x,
> SubSlotBordPoint.y, EndPos.Coordinate.x, EndPos.Coordinate.y,
> TempPt2.x, TempPt2.y);
> TLOG_INFO << log_string;
> }
> }
> if (bCenterEndCarPosFlag == TRUE) {
> return TRUE;
> }
> return FALSE;
> }
> BOOLEAN APAMap_ParkingOutBoundarySeizeEndCarPosInfo() {
> APACoordinateDataCalFloatType Data[100];
> APA_ENUM_TYPE DataNum1, DataNum2;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal2;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal3;
> APACarCoordinateDataCalFloatType EndPos;
> APA_DISTANCE_TYPE NSegNum;
> APA_DISTANCE_TYPE ODNSegNum;
> APACoordinateDataCalFloatType TempPt;
> APA_ENUM_TYPE LocStyle;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType MainLinXStrPt, MainLinXEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
> APACoordinateDataCalFloatType CarLinYStrPt, CarLinYEndPt;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> // APACoordinateDataCalFloatType NSegment[127];
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> uint8_t_INF ParkOutMode;
> APA_ENUM_TYPE ParkSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> APA_INDEX_TYPE i, k, m;
> APA_ENUM_TYPE DataNum;
> BOOLEAN bSlotDataAtRigthSide;
> BOOLEAN bSearch;
> APACoordinateDataCalFloatType OrgPt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1, fDis2;
> APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APA_DISTANCE_TYPE GoStraightSafeDis;
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> st_MapODDataType* pODInfo;
> Obj_Information_t CurObjComInfo;
> // APACoordinateDataCalFloatType TempPt1;
> // APACoordinateDataCalFloatType TempPt2;
> #else
> tMap_VsPillarDataInfo_t* pVsPillarInfo;
> tMap_FusODObjDataInfo_t* pFusODObjInfo;
> #endif
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return FALSE;
> }
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> SafeDisCal = 500;
> SafeDisCal2 = 0;
> SafeDisCal3 = 1500;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> // zqf:PARALLEL_SIDE
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> } else {
> if (ParkSide == APA_CAR_PARK_AT_LEFT_SIDE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> }
> if ((FALSE == bPreventStepNRedundantFlag) &&
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND))) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> fDis1 = 4000;
> fDis2 = 3500;
> } else {
> fDis1 = 6000;
> fDis2 = 4000;
> }
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后
> {
> for (k = 0; k < pMapSubSlotBord->PtNum; k++) {
> // get SubBoundary data
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (bSlotDataAtRigthSide) {
> TempPt.x = -TempPt.x;
> }
> if ((TempPt.x < fDis1) && (TempPt.y > -1000)) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo:TempPt(%."
> "2f,%.2f)==",
> TempPt.x, TempPt.y);
> TLOG_INFO << log_string;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> SafeDisCal2 = 300;
> } else {
> SafeDisCal2 = 1000;
> }
> if (TempPt.x < fDis2) {
> SafeDisCal2 = 200;
> break;
> }
> }
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo:SafeDisCal2(%.2f)",
> SafeDisCal2);
> TLOG_INFO << log_string;
> }
> }
> }
> MaxOutOffsetX = APAMap_ComCfg.HalfWidthOfCar + SafeDisCal;
> MaxOutOffsetY =
> APAMap_ComCfg.LenBetweenRAxisAndFBumper + SafeDisCal + SafeDisCal3;
> MaxInnerOffsetX = -(APAMap_ComCfg.HalfWidthOfCar + SafeDisCal);
> MaxInnerOffsetY = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + SafeDisCal);
> EndPos = APAMap_GInfo.SlotPar.EndPos;
> Angle = EndPos.CarAng;
> Pto = EndPos.Coordinate;
> if (TRUE == bAfterNewAnchorPointFlag) {
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
> if (FALSE == bSlotDataAtRigthSide) {
> MaxOutOffsetX += SafeDisCal2;
> } else {
> MaxInnerOffsetX -= SafeDisCal2;
> }
> }
> }
> // MainLinX
> MainLinXStrPt.x = MaxInnerOffsetX;
> MainLinXStrPt.y = MaxInnerOffsetY;
> MainLinXEndPt.x = MaxOutOffsetX;
> MainLinXEndPt.y = MainLinXStrPt.y;
> MainLinXStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt, Angle, Pto);
> MainLinXEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt, Angle, Pto);
> // MainLinY
> MainLinYStrPt.x = MaxOutOffsetX;
> MainLinYStrPt.y = MaxInnerOffsetY;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = MaxOutOffsetY;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // SubLinX
> SubLinXStrPt.x = MaxInnerOffsetX;
> SubLinXStrPt.y = MaxOutOffsetY;
> SubLinXEndPt.x = MaxOutOffsetX;
> SubLinXEndPt.y = SubLinXStrPt.y;
> SubLinXStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinXStrPt, Angle, Pto);
> SubLinXEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinXEndPt, Angle, Pto);
> // SubLinY
> SubLinYStrPt.x = MaxInnerOffsetX;
> SubLinYStrPt.y = MaxInnerOffsetY;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = MaxOutOffsetY;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> // CarLinY
> CarLinYStrPt.x = 0;
> CarLinYStrPt.y = MaxInnerOffsetY;
> CarLinYEndPt.x = 0;
> CarLinYEndPt.y = MaxOutOffsetY;
> CarLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(CarLinYStrPt, Angle, Pto);
> CarLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(CarLinYEndPt, Angle, Pto);
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
> #else
> pVsPillarInfo = &APAMap_GInputData.VsPillarInfo;
> pFusODObjInfo = &APAMap_GInputData.FusODObjInfo;
> #endif
> i = 0;
> m = 0;
> NSegNum = 0;
> ODNSegNum = 0;
> DataNum = 0;
> DataNum1 = pMapMainSlotBord->PtNum;
> DataNum2 = pMapSubSlotBord->PtNum;
> bSearch = TRUE;
> while (bSearch) {
> if (m == 0) {
> if (i < DataNum1) {
> for (k = 0; k < DataNum1; k++) {
> // get MainBoundary data
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].y;
> }
> i = DataNum1;
> DataNum = DataNum1;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 1) {
> if (i < DataNum2) {
> for (k = 0; k < DataNum2; k++) {
> // get SubBoundary data
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
> }
> i = DataNum2;
> DataNum = DataNum2;
> } else {
> i = 0;
> m++;
> }
> }
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> if (m == 2) {
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
> (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
> (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
> (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
> (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
> (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
> (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
> (CurObjComInfo.Label == Obj_Label_WheelChock)) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Square.ObjNum) {
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> DataNum = 4;
> i++;
> } else {
> m++;
> i = 0;
> }
> }
> if (m == 3) {
> bSearch = FALSE;
> }
> #if 0
> if (m == 3) {
> if (i < pODInfo->Triangle.ObjNum) {
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.y;
> Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.x;
> Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.y;
> Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.x;
> Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.y;
> DataNum = 3;
> CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
> i++;
> } else {
> m++;
> i = 0;
> }
> }
> if (m == 4) {
> if (i < pODInfo->CirCular.ObjNum) {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.x;
> TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.y;
> TempPt2.x =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> TempPt2.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.x = -TempPt2.x;
> Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> DataNum = 4;
> CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
> i++;
> } else {
> m++;
> i = 0;
> }
> }
> if (m == 5) {
> if (i < pODInfo->Polygon.ObjNum) {
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> }
> DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
> CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
> i++;
> } else {
> i = 0;
> m++;
> bSearch = FALSE;
> }
> }
> #endif
> #else
> if (m == 2) {
> if (i < pVsPillarInfo->Pillar2InfoAtPark.PillarNum) {
> for (k = 0; k < 4; k++) {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar2InfoAtPark
> .Pillar[i]
> .SquarePillar.Pt[k]
> .x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar2InfoAtPark
> .Pillar[i]
> .SquarePillar.Pt[k]
> .y;
> }
> DataNum = 4;
> i++;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 3) {
> if (i < pVsPillarInfo->Pillar1InfoAtPark.PillarNum) {
> for (k = 0; k < 4; k++) {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar1InfoAtPark
> .Pillar[i]
> .SquarePillar.Pt[k]
> .x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar1InfoAtPark
> .Pillar[i]
> .SquarePillar.Pt[k]
> .y;
> }
> DataNum = 4;
> i++;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 4) {
> if (i < pFusODObjInfo->ObjNum) {
> for (k = 0; k < 4; k++) {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pFusODObjInfo->Obj[i].BBox.Pt[k].x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pFusODObjInfo->Obj[i].BBox.Pt[k].y;
> }
> DataNum = 4;
> i++;
> } else {
> i = 0;
> m++;
> }
> }
> if (m == 5) {
> bSearch = FALSE;
> }
> #endif
> if (TRUE == bSearch) {
> if ((m == 0) || (m == 1)) {
> //----------------------------------
> // Get valid fsd data
> #if 1
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> if (TRUE == bLabelAngledFlag) {
> GoStraightSafeDis = 500;
> } else {
> GoStraightSafeDis = 500;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
> fDis1 = 2000;
> FOffset = GoStraightSafeDis;
> BOffset = 50;
> } else if (ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> fDis1 = -1000;
> FOffset = 50;
> BOffset = GoStraightSafeDis;
> }
> if (m == 0) {
> LOffset = 20;
> ROffset = 20;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
> pRectPt, pRectLine);
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (TempPt.x < fDis1) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(
> 0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
> NSegNum++;
> bFsdFromMapMainSlotBordFlag = TRUE;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> bFsdInRightOfEndCarPosFlag = TRUE;
> } else {
> bFsdInRightOfEndCarPosFlag = FALSE;
> }
> }
> }
> } else  //(m == 1)
> {
> LOffset = 50;
> ROffset = 50;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
> pRectPt, pRectLine);
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (TempPt.x < fDis1) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(
> 0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
> NSegNum++;
> bFsdFromMapSubSlotBordFlag = TRUE;
> }
> }
> }
> if (NSegNum > 0) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==1==m(%d)"
> "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
> "bFsdFromMapMainSlotBordFlag(%d)=="
> "bFsdFromMapSubSlotBordFlag(%d)",
> m, bFsdInRightOfEndCarPosFlag, NSegNum,
> bFsdFromMapMainSlotBordFlag, bFsdFromMapSubSlotBordFlag);
> TLOG_INFO << log_string;
> }
> } else
> #endif
> {
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 1) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt, &MainLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 1) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> // NSegment[NSegNum] = TempPt;
> NSegNum++;
> if (m == 0) {
> bFsdFromMapMainSlotBordFlag = TRUE;
> }
> if (m == 1) {
> bFsdFromMapSubSlotBordFlag = TRUE;
> }
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> bFsdInRightOfEndCarPosFlag = TRUE;
> } else {
> bFsdInRightOfEndCarPosFlag = FALSE;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo=="
> "2==m(%d)"
> "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
> "bFsdFromMapMainSlotBordFlag(%d)=="
> "bFsdFromMapSubSlotBordFlag(%d)",
> m, bFsdInRightOfEndCarPosFlag, NSegNum,
> bFsdFromMapMainSlotBordFlag,
> bFsdFromMapSubSlotBordFlag);
> TLOG_INFO << log_string;
> }
> }
> }
> }
> }
> }
> if (NSegNum < 1) {
> fDis1 = -2000;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> FOffset = 2000;
> BOffset = 200;
> LOffset = 200;
> ROffset = 200;
> } else {
> FOffset = 2000;
> BOffset = 500;
> LOffset = 500;
> ROffset = 500;
> }
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
> pRectPt, pRectLine);
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (TempPt.y > fDis1) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(
> 0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
> NSegNum++;
> if (m == 0) {
> bFsdFromMapMainSlotBordFlag = TRUE;
> }
> if (m == 1) {
> bFsdFromMapSubSlotBordFlag = TRUE;
> }
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> bFsdInRightOfEndCarPosFlag = TRUE;
> } else {
> bFsdInRightOfEndCarPosFlag = FALSE;
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==3==m(%d)"
> "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
> "bFsdFromMapMainSlotBordFlag(%d)=="
> "bFsdFromMapSubSlotBordFlag(%d)",
> m, bFsdInRightOfEndCarPosFlag, NSegNum,
> bFsdFromMapMainSlotBordFlag, bFsdFromMapSubSlotBordFlag);
> TLOG_INFO << log_string;
> }
> }
> }
> }
> }
> }
> if ((m > 1) && (m < 5)) {
> //----------------------------------
> // Get valid OD data
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
> fDis1 = 1000;
> FOffset = 1000;
> BOffset = 50;
> } else if (ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
> fDis1 = -2000;
> FOffset = 50;
> BOffset = 1000;
> }
> LOffset = 50;
> ROffset = 50;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
> pRectPt, pRectLine);
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (TempPt.x < fDis1) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], DataNum,
> pRectPt, pRectLine)) {
> ODNSegNum++;
> }
> }
> } else {
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 1) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt, &MainLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 1) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> bFsdInRightOfEndCarPosFlag = TRUE;
> if (bSlotDataAtRigthSide) {
> bFsdFromMapMainSlotBordFlag = TRUE;
> } else {
> bFsdFromMapSubSlotBordFlag = TRUE;
> }
> } else {
> bFsdInRightOfEndCarPosFlag = FALSE;
> if (bSlotDataAtRigthSide) {
> bFsdFromMapSubSlotBordFlag = TRUE;
> } else {
> bFsdFromMapMainSlotBordFlag = TRUE;
> }
> }
> // NSegment[ODNSegNum] = TempPt;
> ODNSegNum++;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo=="
> "4==m(%d)"
> "==bFsdInRightOfEndCarPosFlag(%d)==ODNSegNum(%d)"
> "==bFsdFromMapMainSlotBordFlag(%d)=="
> "bFsdFromMapSubSlotBordFlag(%d)",
> m, bFsdInRightOfEndCarPosFlag, ODNSegNum,
> bFsdFromMapMainSlotBordFlag,
> bFsdFromMapSubSlotBordFlag);
> TLOG_INFO << log_string;
> }
> }
> }
> }
> }
> }
> }
> }
> }
> if ((NSegNum == 0) && (ODNSegNum == 0)) {
> bFsdFromMapMainSlotBordFlag = FALSE;
> bFsdFromMapSubSlotBordFlag = FALSE;
> }
> if ((TRUE == bFsdFromMapMainSlotBordFlag) &&
> (TRUE == bFsdFromMapSubSlotBordFlag)) {
> bFsdFromMapMainAndSubSlotBordFlag = TRUE;
> } else {
> bFsdFromMapMainAndSubSlotBordFlag = FALSE;
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==5==NSegNum(%d)=="
> "ODNSegNum(%d)"
> "==bFsdFromMapMainAndSubSlotBordFlag(%d)=="
> "bFsdFromMapMainSlotBordFlag(%d)==bFsdFromMapSubSlotBordFlag(%d)",
> NSegNum, ODNSegNum, bFsdFromMapMainAndSubSlotBordFlag,
> bFsdFromMapMainSlotBordFlag, bFsdFromMapSubSlotBordFlag);
> TLOG_INFO << log_string;
> }
> if ((NSegNum != 0) || (ODNSegNum != 0)) {
> return TRUE;
> }
> return FALSE;
> }
> void APAMap_ParkingOutEndCarPosUpdata(void) {
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType Obj2Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType OrgPt;
> APACarCoordinateDataCalFloatType EndPos;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> BOOLEAN bSlotDataAtRigthSide;
> APACarCoordinateDataCalFloatType TempCarPos;
> BOOLEAN bUpdataObjFlag;
> APA_ENUM_TYPE i;
> BOOLEAN bCenterEndCarPosFlag = FALSE;  // 采用终点位置居中标志位
> BOOLEAN bInsideSlotFlag;               // 车辆在车位内标志位
> BOOLEAN bSeizeEndCarPosFlag;           // fsd侵占终点位置标志位
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> bUpdataObjFlag = TRUE;
> bInsideSlotFlag = TRUE;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> // 除了直进直出，其他场景，当检测到车辆已泊出车位，且车头方向已与终点位置方向基本一致（角度偏差不超过5度），则把终点位置定在与车x轴坐标一致的位置，且y轴向前推进1m.
> if (TRUE ==
> bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位，则不再更新Obj1、Obj2
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if ((CurCarCoordinateX > 1) &&
> (MATH_ABS(APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x -
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x) > 200)) {
> bUpdataObjFlag = FALSE;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -1) {
> bInsideSlotFlag = FALSE;
> }
> } else {
> if (CurCarCoordinateX > 0) {
> bInsideSlotFlag = FALSE;
> }
> }
> }
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> if ((FALSE == bUpdataObjFlag) &&
> (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
> APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 36)) &&
> (FALSE == bPreventStepNRedundantFlag) &&
> (FALSE == bLaneLineUpdateEndCarPosFlag) &&
> (FALSE == bRefercLineUpdateEndCarPosFlag)) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==EndCarPosUpdata_1m===Before"
> "==CarPos.CarAng(%.2f),EndPos_CarAng(%.2f),EndPos_Coordinate(%"
> ".2f,%.2f)",
> APAMap_GInputData.CarLocInfo.CarPos.CarAng,
> APAMap_GInfo.SlotPar.EndPos.CarAng,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> EndPos.Coordinate = APAMap_GInputData.CarLocInfo.CarPos.Coordinate;
> EndPos.Coordinate.y += 1000;
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x == NO_OBJ_DISTANCE) {
> APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
> }
> #else
> APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
> #endif
> bPreventStepNRedundantFlag = TRUE;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==EndCarPosUpdata_1m===After"
> "==CarPos.CarAng(%.2f),EndPos_CarAng(%.2f),EndPos_Coordinate(%"
> ".2f,%.2f)",
> APAMap_GInputData.CarLocInfo.CarPos.CarAng,
> APAMap_GInfo.SlotPar.EndPos.CarAng,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==EndCarPosUpdata===Before==EndPos_Coordinate(%.2f,%.2f)",
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> // CenterEndCarPosUpdata
> if ((TRUE == bSeizeEndCarPosFlag) ||
> ((TRUE == bCntAddFlag) && (FALSE == bSeizeEndCarPosFlag) &&
> (FALSE == bPreventStepNRedundantFlag) && (FALSE == bInsideSlotFlag))) {
> bCenterEndCarPosFlag = APAMap_ParkingOutCenterEndCarPosInfo();
> if (TRUE == bCenterEndCarPosFlag) {
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==CenterEndCarPosUpdata===bCenterEndCarPosFlag(%d)"
> "==bSeizeEndCarPosFlag(%d)==EndPos_Coordinate(%.2f,%.2f)",
> bCenterEndCarPosFlag, bSeizeEndCarPosFlag,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> }
> if ((TRUE == bAfterNewAnchorPointFlag) &&
> (APAMap_GInfo.SlotPar.EndPos.Coordinate.y < -8000)) {
> bSeizeEndCarPosFlag = FALSE;
> }
> // EndCarPosUpdate
> i = 0;
> while (TRUE == bSeizeEndCarPosFlag) {
> TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> } else {
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> }
> #else
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> #endif
> APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
> if ((TRUE == bLaneLineUpdateEndCarPosFlag) ||
> (TRUE == bRefercLineUpdateEndCarPosFlag)) {
> APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
> }
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> i++;
> if (i > 9) {
> bSeizeEndCarPosFlag = FALSE;
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==EndCarPosUpdata===After==EndPos_Coordinate(%.2f,%.2f)",
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
> TLOG_INFO << log_string;
> }
> }
> void APAMAP_ParkingOutGetSlotBdPtByODObjs(
> APA_ENUM_TYPE Bordpttype, APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY) {
> APA_ENUM_TYPE i, j;
> APA_ENUM_TYPE k;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE OffsetY;
> APA_ENUM_TYPE TargetXLoc;
> APA_ENUM_TYPE TargetYLoc;
> APACoordinateDataCalFloatType LineXStrPt;
> APACoordinateDataCalFloatType LineXEndPt;
> APACoordinateDataCalFloatType LineYStrPt;
> APACoordinateDataCalFloatType LineYEndPt;
> APACoordinateDataCalFloatType Data[10];
> APA_ENUM_TYPE DataNum;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;
> // APACoordinateDataCalFloatType TempPt1;
> // APACoordinateDataCalFloatType TempPt2;
> st_MapODDataType* pODInfo;
> BOOLEAN bSearch;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APA_DISTANCE_CAL_FLOAT_TYPE ObjAng;
> APACoordinateDataCalFloatType OrgPt;
> BOOLEAN bDataAtRightSide;
> APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetX;
> APACoordinateDataCalFloatType OffsetYRefPt;
> APACoordinateDataCalFloatType OffsetXRefPt;
> APACoordinateDataType ODInSlotPtForOffsetX;
> APACoordinateDataType ODInSlotPtForOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> // APACarCoordinateDataCalFloatType TempCarPos;
> Obj_Information_t CurObjComInfo;
> uint8_t_INF ParkOutMode;
> #if 1
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> *pOffsetX = 0;
> *pOffsetY = 0;
> return;
> }
> pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
> #else
> if (APAMap_GInputData.VisObjsInfo.timestamp_ms == 0) {
> *pOffsetX = 0;
> *pOffsetY = 0;
> return;
> }
> pODInfo = &APAMap_GInputData.VisObjsInfo.ODInfo;
> #endif
> i = 0;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> MaxOutOffsetX = 2000;
> MaxOutOffsetY = 1000;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> MaxInnerOffsetX = 1500;
> } else {
> MaxInnerOffsetX = 2800;
> }
> bDataAtRightSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> if (Bordpttype == 1) {
> ObjAng = APAMap_GInfo.SlotPar.Obj2Ang;
> } else {
> ObjAng = APAMap_GInfo.SlotPar.Obj1Ang;
> }
> // ObjAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
> if (bDataAtRightSide == TRUE) {
> if (Bordpttype == 0) {
> TargetYLoc = 1;  // right;
> } else {
> TargetYLoc = 0;  // left;
> }
> TargetXLoc = 0;  // left;
> } else {
> if (Bordpttype == 0) {
> TargetYLoc = 0;  // left
> } else {
> TargetYLoc = 1;  // right;
> }
> TargetXLoc = 1;  // right
> }
> LineYStrPt = APAMap_GInfo.NewCordSysOPt;
> LineYEndPt.y = LineYStrPt.y + 1000 * MATH_COS(OrgAng);
> LineYEndPt.x = LineYStrPt.x - 1000 * MATH_SIN(OrgAng);
> if (Bordpttype == 0) {
> LineXStrPt = APAMap_GInfo.SlotPar.Obj1Pt;
> } else {
> LineXStrPt = APAMap_GInfo.SlotPar.Obj2Pt;
> }
> TempDis = APAMap_GetSearchMaxInnerY(Bordpttype, bDataAtRightSide, LineXStrPt,
> ObjAng);
> TempDis -= 300;
> if (TempDis < 0) {
> TempDis = 300;
> }
> MaxInnerOffsetY = TempDis;
> LineXEndPt.y = LineXStrPt.y + 1000 * MATH_COS(ObjAng);
> LineXEndPt.x = LineXStrPt.x - 1000 * MATH_SIN(ObjAng);
> bSearch = TRUE;
> i = 0;
> j = 0;
> k = 0;
> OffsetX = -MaxInnerOffsetX;
> OffsetY = -MaxOutOffsetY;
> PreOffsetY = 0;
> PreOffsetX = 0;
> ODInSlotPtForOffsetX.x = NO_OBJ_DISTANCE;
> ODInSlotPtForOffsetX.y = NO_OBJ_DISTANCE;
> ODInSlotPtForOffsetY.x = NO_OBJ_DISTANCE;
> ODInSlotPtForOffsetY.y = NO_OBJ_DISTANCE;
> while (bSearch) {
> if (j == 0) {
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
> (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
> (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
> (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
> (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
> (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
> (CurObjComInfo.Label == Obj_Label_Stone_Piers)) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Square.ObjNum) {
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> DataNum = 4;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> #if 0
> if (j == 1) {
> if (i < pODInfo->Triangle.ObjNum) {
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.y;
> Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.x;
> Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.y;
> Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.x;
> Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.y;
> DataNum = 3;
> CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 2) {
> if (i < pODInfo->CirCular.ObjNum) {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.x;
> TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.y;
> TempPt2.x =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> TempPt2.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.x = -TempPt2.x;
> Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
> DataNum = 4;
> CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 3) {
> if (i < pODInfo->Polygon.ObjNum) {
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> }
> DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
> CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
> } else {
> bSearch = FALSE;
> }
> }
> #else
> if (j == 1) {
> while (i < pODInfo->Polygon.ObjNum) {
> CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
> if (CurObjComInfo.Label == Obj_Label_Curb) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Polygon.ObjNum) {
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> }
> DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 2) {
> bSearch = FALSE;
> }
> #endif
> if (bSearch) {
> PreOffsetY = OffsetY;
> PreOffsetX = OffsetX;
> APAMAP_GetSlotBdPtOffsetByGivenObjPts(
> TargetXLoc, TargetYLoc, &LineXStrPt, &LineXEndPt, &LineYStrPt,
> &LineYEndPt, &Data[0], DataNum, MaxOutOffsetX, MaxInnerOffsetX,
> MaxOutOffsetY, MaxInnerOffsetY, &OffsetX, &OffsetY, &OffsetYRefPt,
> &OffsetXRefPt);
> if (PreOffsetX < OffsetX) {
> ODInSlotPtForOffsetX.x = (APA_DISTANCE_TYPE)OffsetXRefPt.x;
> ODInSlotPtForOffsetX.y = (APA_DISTANCE_TYPE)OffsetXRefPt.y;
> }
> if (PreOffsetY < OffsetY) {
> ODInSlotPtForOffsetY.x = (APA_DISTANCE_TYPE)OffsetYRefPt.x;
> ODInSlotPtForOffsetY.y = (APA_DISTANCE_TYPE)OffsetYRefPt.y;
> }
> }
> i++;
> };
> #ifdef DEBUG_PRINT_SLOTOBJ
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ODOffset(%u)(%f,%f),Max(%f),ODInSlotPtForOffsetX(%d,%d),"
> "ODInSlotPtForOffsetY(%d,%d))",
> Bordpttype, OffsetX, OffsetY, MaxInnerOffsetY,
> ODInSlotPtForOffsetX.x, ODInSlotPtForOffsetX.y,
> ODInSlotPtForOffsetY.x, ODInSlotPtForOffsetY.y);
> TLOG_INFO << log_string;
> #endif
> if ((OffsetX > 50) || (OffsetY > 50)) {
> if (OffsetX < 0) {
> OffsetX = 0;
> }
> *pOffsetX = OffsetX;
> *pOffsetY = OffsetY;
> } else {
> *pOffsetX = 0;
> *pOffsetY = 0;
> }
> APAMap_GInfo.SlotPar.ODPt[Bordpttype] = ODInSlotPtForOffsetX;
> return;
> }
> void APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
> APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
> APAMAP_ParkingOutGetSlotBdPtByODObjs(0, pOffsetX1, pOffsetY1);
> APAMAP_ParkingOutGetSlotBdPtByODObjs(1, pOffsetX2, pOffsetY2);
> return;
> }
> void APAMap_ParkingOutPickDispersedObstacles(ObstaclesInfo_INF* pObjInfo) {
> st_MapODDataType* pODInfo;
> BOOLEAN bSearch;
> APA_INDEX_TYPE i, j, k, m;
> APACoordinateDataCalFloatType Data[10];
> APACoordinateDataCalFloatType DataTemp[10];
> APA_ENUM_TYPE PtNum;
> // APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> // APACoordinateDataCalFloatType TempPt1, TempPt2;
> uint8_t_INF ObjNum;
> Obj_Information_t CurObjComInfo;
> APA_ENUM_TYPE ParkMode;
> APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
> // APA_DISTANCE_CAL_FLOAT_TYPE Offset;
> APACarCoordinateDataCalFloatType CarPos;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACoordinateDataCalFloatType pRectPtForLimiter[4];
> APALineParameterABCType pRectLineForLimiter[4];
> // float Confidence;
> uint8_t_INF ObjType;
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType pRectPt2[4];
> APALineParameterABCType pRectLine2[4];
> APACoordinateDataCalFloatType pRectPtForLimiter2[4];
> APALineParameterABCType pRectLineForLimiter2[4];
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType pRectPt3[4];
> APALineParameterABCType pRectLine3[4];
> APA_ENUM_TYPE Cnt;
> BOOLEAN bSlotDataAtRigthSide;
> #ifdef APA_MAP_DEBUG_INFO_LIMITER
> APA_ENUM_TYPE LimiterNum;
> LimiterNum = 0;
> #endif
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CarPos = APAMap_GInfo.SlotPar.EndPos;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> pObjInfo->obstacle_num = 0;
> pObjInfo->timestamp_ms = APAMap_GInputData.ParkReqPar.timestamp_ms;
> APAMap_CalAndAddRskOBjObstacles(pObjInfo);
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return;
> }
> pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
> // Angle = APAMap_GInfo.NewCordSysAng;
> if (pObjInfo == NULL) {
> return;
> }
> i = 0;
> j = 0;
> ObjNum = pObjInfo->obstacle_num;
> bSearch = TRUE;
> FOffset = 150;
> BOffset = 150;
> LOffset = 200;
> ROffset = 200;
> // Offset = 300;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos, pRectPt,
> pRectLine);
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos, pRectPt,
> pRectLine);
> FOffset =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndFBumper -
> APAMap_ComCfg.LenBetweenFRAxis);
> BOffset =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndRBumper -
> 100);
> LOffset = 200;
> ROffset = 200;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos,
> pRectPtForLimiter, pRectLineForLimiter);
> if (ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> FOffset = 10;
> BOffset = 10;
> LOffset = 10;
> ROffset = 10;
> } else {
> FOffset = 50;
> BOffset = 50;
> LOffset = 50;
> ROffset = 50;
> }
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos, pRectPt2,
> pRectLine2);
> FOffset =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndFBumper -
> APAMap_ComCfg.LenBetweenFRAxis);
> BOffset =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndRBumper -
> 100);
> LOffset = 200;
> ROffset = 200;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
> pRectPtForLimiter2, pRectLineForLimiter2);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide) {
> FOffset = 1500;
> BOffset = 1500;
> LOffset = -(APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
> ROffset = 10;
> } else {
> FOffset = 1500;
> BOffset = 1500;
> LOffset = 10;
> ROffset = -(APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
> }
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
> pRectPt3, pRectLine3);
> }
> while (bSearch) {
> PtNum = 0;
> if (j == 0) {
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
> (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
> (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
> (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
> (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
> (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
> (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
> (CurObjComInfo.Label == Obj_Label_WheelChock)) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Square.ObjNum) {
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> PtNum = 4;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> #if 0
> if (j == 1) {
> if (i < pODInfo->Triangle.ObjNum) {
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_1.y;
> Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.x;
> Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_2.y;
> Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.x;
> Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
> .Points.Point_3.y;
> CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
> Data[3] = {0.0, 0.0}; // to remove build warning
> PtNum = 3;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 2) {
> if (i < pODInfo->CirCular.ObjNum) {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.x;
> TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
> .CenterPoint.y;
> TempPt2.x =
> -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> TempPt2.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
> Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
> TempPt2.x = -TempPt2.x;
> Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
> TempPt2.y = -TempPt2.y;
> Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
> PtNum = 4;
> CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 3) 
> {
> #if 0
> if (i < pODInfo->Polygon.ObjNum) 
> {
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> }
> PtNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
> CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
> i++;
> } else
> #else
> #endif
> {
> PtNum = 0;
> bSearch = FALSE;
> }
> }
> #else
> if (j == 1) {
> while (i < pODInfo->Polygon.ObjNum) {
> CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
> if (CurObjComInfo.Label == Obj_Label_Curb) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Polygon.ObjNum) {
> Cnt = pODInfo->Polygon.Polygons[i].Points.PointNum;
> m = 0;
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> DataTemp[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> DataTemp[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &DataTemp[k], 1,
> pRectPt3, pRectLine3)) {
> Cnt--;
> if (Cnt < 0) {
> Cnt = 0;
> }
> continue;
> }
> }
> Data[m].x = DataTemp[k].x;
> Data[m].y = DataTemp[k].y;
> m++;
> }
> PtNum = Cnt;
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 2) {
> PtNum = 0;
> bSearch = FALSE;
> }
> #endif
> if (ObjNum >= OBSTACLE_MAX_NUM) {
> bSearch = FALSE;
> }
> if (bSearch) {
> ObjType = CurObjComInfo.Label + APA_MAP_PT_PROPERTY_OD_STR;
> for (k = 0; k < PtNum; k++) {
> pObjInfo->obstacle_list[ObjNum].polygon_point_list[k].x =
> Data[k].x * 0.001;
> pObjInfo->obstacle_list[ObjNum].polygon_point_list[k].y =
> Data[k].y * 0.001;
> }
> pObjInfo->obstacle_list[ObjNum].id = ObjNum;  // CurObjComInfo.ID;
> pObjInfo->obstacle_list[ObjNum].type = ObjType;
> if (ObjType != OD_OBJ_WheelChock) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
> pRectPt2, pRectLine2)) {
> pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
> } else if (TRUE == APAMap_CheckIfObjWithinRectArea(
> 0x01, &Data[0], PtNum, pRectPt, pRectLine)) {
> pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
> } else if (ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> #if 0
> if(TRUE == APAMap_CheckIfObjWithinRectArea(0x01,&Data[0],PtNum,pRectPt,pRectLine))
> {
> if(TRUE == APAMap_CheckIfObjWithinRectArea(0x01,&Data[0],PtNum,pRectPtForLimiter,pRectLineForLimiter))
> {
> pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
> }
> }
> #endif
> }
> } else {
> if ((ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) &&
> (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
> pRectPt2, pRectLine2))) {
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
> pRectPtForLimiter2,
> pRectLineForLimiter2)) {
> pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
> }
> } else {
> if (FALSE == APAMap_CheckIfObjWithinRectArea(
> 0x01, &Data[0], PtNum, pRectPtForLimiter2,
> pRectLineForLimiter2)) {
> bODWheelChockFlag = TRUE;
> }
> }
> }
> }
> pObjInfo->obstacle_list[ObjNum].confidence = CurObjComInfo.Confidence;
> pObjInfo->obstacle_list[ObjNum].polygon_point_num = PtNum;
> if ((CurObjComInfo.Speed.x == 0) && (CurObjComInfo.Speed.y == 0) &&
> (CurObjComInfo.Speed.z == 0) && (CurObjComInfo.Acceleration.x == 0) &&
> (CurObjComInfo.Acceleration.y == 0) &&
> (CurObjComInfo.Acceleration.z == 0)) {
> pObjInfo->obstacle_list[ObjNum].is_static = TRUE;
> } else {
> pObjInfo->obstacle_list[ObjNum].is_static = FALSE;
> }
> pObjInfo->obstacle_list[ObjNum].length = 0;
> pObjInfo->obstacle_list[ObjNum].width = 0;
> pObjInfo->obstacle_list[ObjNum].theta = 0;
> ObjNum++;
> }
> }
> pObjInfo->timestamp_ms = APAMap_GInputData.ParkReqPar.timestamp_ms;
> pObjInfo->obstacle_num = ObjNum;
> return;
> }
> BOOLEAN APAMap_ParkingOutUpDataMapInfoBySlotCorInfo() {
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> BOOLEAN bUpdataObjFlag;
> BOOLEAN bSlotDataAtRigthSide;
> uint8_t_INF ParkOutMode;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> bUpdataObjFlag = TRUE;
> bCntAddFlag = FALSE;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 6) &&
> (APAMap_GInputData.ParkReqPar.APAstate >= 4) &&
> ((APAMap_GInputData.ParkReqPar.Request_cmd == 2) ||
> (APAMap_GInputData.ParkReqPar.Request_cmd == 7))) {
> bAfterNewAnchorPointFlag = TRUE;
> } else {
> bAfterNewAnchorPointFlag = FALSE;
> }
> if ((TRUE == bAfterNewAnchorPointFlag) &&
> ((MATH_FABS(APAMap_GInfo.SlotPar.SlotBordPt[0].x) - 0.0) <= 2000) &&
> (APAMap_GInputData.ParkReqPar.request_cnt != APAMap_GInfo.lastreqcnt)) {
> bCntAddFlag = TRUE;
> }
> if (TRUE ==
> bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位，则不再更新Obj1、Obj2
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (CurCarCoordinateX > 0) {
> bUpdataObjFlag = FALSE;
> }
> } else {
> if (CurCarCoordinateX > 1) {
> bUpdataObjFlag = FALSE;
> }
> }
> }
> if (TRUE == bUpdataObjFlag) {
> APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
> }
> APAMap_ParkingOutCalSlotSlotAlignInfo();
> return TRUE;
> }
> BOOLEAN APAMap_ParkingOutObliqueRowStairsInfo() {
> APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_DISTANCE_TYPE i;
> APA_ENUM_TYPE k;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType NSegment[127];
> APA_ENUM_TYPE NSegNum;
> APA_ENUM_TYPE DataNum;
> st_MapTopViewFSD* pTopViewInfo;
> APA_DISTANCE_TYPE TopViewPtNum;
> APA_ENUM_TYPE LocStyle;
> UCHAR CurID;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType MainLinYStrPt2, MainLinYEndPt2;
> APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
> APACoordinateDataCalFloatType MainLinXStrPt3, MainLinXEndPt3;
> APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
> APA_DISTANCE_CAL_FLOAT_TYPE MinOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> uint8_t_INF ParkOutMode;
> BOOLEAN bUpdataObliqueRowStairsFlag;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
> // return FALSE;
> }
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> return FALSE;
> }
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> bUpdataObliqueRowStairsFlag = FALSE;
> if (TRUE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (CurCarCoordinateX > -3) {
> bUpdataObliqueRowStairsFlag = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (CurCarCoordinateX > -0.5) {
> bUpdataObliqueRowStairsFlag = TRUE;
> }
> }
> }
> if (FALSE == bUpdataObliqueRowStairsFlag) {
> return FALSE;
> }
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return FALSE;
> }
> pTopViewInfo = &APAMap_GInputData.TotalMapInfo.mapData.FSDInfo.TopView;
> MaxOffsetX = 2000;
> MinOffsetX = 1000;
> MaxOffsetY = 1000;
> OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
> OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> MainLinYStrPt2.x = MinOffsetX;
> MainLinYStrPt2.y = 0;
> MainLinYEndPt2.x = MainLinYStrPt2.x;
> MainLinYEndPt2.y = 1000;
> MainLinYStrPt2 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt2, Angle, Pto);
> MainLinYEndPt2 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt2, Angle, Pto);
> // obj2 borderline;
> MainLinXStrPt2 = OrgObj2Pt;
> MainLinXEndPt2 = MainLinXStrPt2;
> LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
> MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
> // obj2 borderline2;
> MainLinXStrPt3.x = 0;
> MainLinXStrPt3.y = MaxOffsetY;
> MainLinXEndPt3.x = 1000;
> MainLinXEndPt3.y = MainLinXStrPt3.y;
> MainLinXStrPt3 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt3, Angle, Pto);
> MainLinXEndPt3 =
> AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt3, Angle, Pto);
> NSegNum = 0;
> i = 0;
> TopViewPtNum = pTopViewInfo->PointNum;
> while (i < TopViewPtNum) {
> // get fsd data with same id;
> CurID = pTopViewInfo->InfoPoint[i].ID;
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.y;
> for (k = 1; k < 100; k++) {
> if ((i + k) < pTopViewInfo->PointNum) {
> if (pTopViewInfo->InfoPoint[i + k].ID != CurID) {
> break;
> } else {
> Data[k].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
> .Point.x;
> Data[k].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
> .Point.y;
> }
> } else {
> break;
> }
> }
> DataNum = k;
> i += DataNum;
> //----------------------------------
> // Get valid fsd data for fus obj2bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt2, &MainLinYEndPt2, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt3, &MainLinXEndPt3, &TempPt, &fDis);
> if (LocStyle != 0) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> }
> }
> if (NSegNum >= 3) {
> return TRUE;
> }
> }
> return FALSE;
> }
> void APAMap_ParkingOutMapScenarioModeCheck(
> BOOLEAN* pCarLeftSideExistSlot, BOOLEAN* pCarRightSideExistSlot,
> BOOLEAN* pCarForwardExistSlot, BOOLEAN* pCarBackwardExistSlot,
> BOOLEAN* pCarLeftSideExistODObj, BOOLEAN* pCarRightSideExistODObj,
> BOOLEAN* pCarForwardExistODObj, BOOLEAN* pCarBackwardExistODObj) {
> st_MapODDataType* vObjInfo;
> APA_ENUM_TYPE vObjNum;
> uint8_t_INF ParkOutMode;
> APALineParameterABCType LeftLine;
> APALineParameterABCType RightLine;
> APALineParameterABCType TopLine;
> APALineParameterABCType BottomLine;
> APALineParameterABCType pRectanglRegionLine[4], pRectanglRegionLine1[4];
> APACoordinateDataCalFloatType pPt, tempPt, tempPt1, tempRectanglePt[4],
> tempRectanglePt1[4];
> BOOLEAN bTurnToOppositeScenario, bTurnToOppositeScenario1;
> APA_INDEX_TYPE cPtInAreaNum, cPtInAreaNum1, RectRegionExistODObj,
> RectRegionExistODObj1;
> plf_RefercLineInfo* pRefercLineInfo;
> APA_DISTANCE_TYPE RefercLinePtNum;
> APACoordinateDataCalFloatType pSlotPt, pSlotPt1;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType OrgPt;
> BOOLEAN bSlotDataAtRigthSide;
> BOOLEAN CarLeftSideExistSlot;
> BOOLEAN CarRightSideExistSlot;
> BOOLEAN CarForwardExistSlot;
> BOOLEAN CarBackwardExistSlot;
> BOOLEAN CarLeftSideExistODObj;
> BOOLEAN CarRightSideExistODObj;
> BOOLEAN CarForwardExistODObj;
> BOOLEAN CarBackwardExistODObj;
> CarLeftSideExistSlot = *pCarLeftSideExistSlot;
> CarRightSideExistSlot = *pCarRightSideExistSlot;
> CarForwardExistSlot = *pCarForwardExistSlot;
> CarBackwardExistSlot = *pCarBackwardExistSlot;
> CarLeftSideExistODObj = *pCarLeftSideExistODObj;
> CarRightSideExistODObj = *pCarRightSideExistODObj;
> CarForwardExistODObj = *pCarForwardExistODObj;
> CarBackwardExistODObj = *pCarBackwardExistODObj;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> bTurnToOppositeScenario = FALSE;
> bTurnToOppositeScenario1 = FALSE;
> RectRegionExistODObj = 0;
> RectRegionExistODObj1 = 0;
> cPtInAreaNum = 0;
> cPtInAreaNum1 = 0;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> tempPt = APAMap_GInfo.SlotPar.SlotBordPt[0];
> tempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> tempPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> tempPt1 = APAMap_GInfo.SlotPar.SlotBordPt[1];
> tempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> tempPt1, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> if (ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)  // perpendicularparkingoutmode
> {
> if (bSlotDataAtRigthSide == TRUE)  // perp,HeadParkingOut,leftside
> {
> tempRectanglePt[0].x = tempPt.x + 500;
> tempRectanglePt[0].y = tempPt.y + 400;
> tempRectanglePt[1].x = tempPt.x - 1000;
> tempRectanglePt[1].y = tempPt.y + 400;
> tempRectanglePt[2].x = tempPt.x - 1000;
> tempRectanglePt[2].y = tempPt.y - 400;
> tempRectanglePt[3].x = tempPt.x + 500;
> tempRectanglePt[3].y = tempPt.y - 400;
> tempRectanglePt1[0].x = tempPt1.x + 500;
> tempRectanglePt1[0].y = tempPt1.y + 400;
> tempRectanglePt1[1].x = tempPt1.x - 1000;
> tempRectanglePt1[1].y = tempPt1.y + 400;
> tempRectanglePt1[2].x = tempPt1.x - 1000;
> tempRectanglePt1[2].y = tempPt1.y - 400;
> tempRectanglePt1[3].x = tempPt1.x + 500;
> tempRectanglePt1[3].y = tempPt1.y - 400;
> } else {
> tempRectanglePt[0].x = tempPt.x - 500;
> tempRectanglePt[0].y = tempPt.y - 400;
> tempRectanglePt[1].x = tempPt.x + 1000;
> tempRectanglePt[1].y = tempPt.y - 400;
> tempRectanglePt[2].x = tempPt.x + 1000;
> tempRectanglePt[2].y = tempPt.y + 400;
> tempRectanglePt[3].x = tempPt.x - 500;
> tempRectanglePt[3].y = tempPt.y + 400;
> tempRectanglePt1[0].x = tempPt1.x - 500;
> tempRectanglePt1[0].y = tempPt1.y - 400;
> tempRectanglePt1[1].x = tempPt1.x + 1000;
> tempRectanglePt1[1].y = tempPt1.y + 400;
> tempRectanglePt1[2].x = tempPt1.x + 1000;
> tempRectanglePt1[2].y = tempPt1.y + 400;
> tempRectanglePt1[3].x = tempPt1.x - 500;
> tempRectanglePt1[3].y = tempPt1.y + 400;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (bSlotDataAtRigthSide == TRUE)  // perp rear parkingout turn left
> {
> tempRectanglePt[0].x = tempPt.x - 500;
> tempRectanglePt[0].y = tempPt.y - 400;
> tempRectanglePt[1].x = tempPt.x + 1000;
> tempRectanglePt[1].y = tempPt.y - 400;
> tempRectanglePt[2].x = tempPt.x + 1000;
> tempRectanglePt[2].y = tempPt.y + 400;
> tempRectanglePt[3].x = tempPt.x - 500;
> tempRectanglePt[3].y = tempPt.y + 400;
> tempRectanglePt1[0].x = tempPt1.x - 500;
> tempRectanglePt1[0].y = tempPt1.y - 400;
> tempRectanglePt1[1].x = tempPt1.x + 1000;
> tempRectanglePt1[1].y = tempPt1.y - 400;
> tempRectanglePt1[2].x = tempPt1.x + 1000;
> tempRectanglePt1[2].y = tempPt1.y + 400;
> tempRectanglePt1[3].x = tempPt1.x - 500;
> tempRectanglePt1[3].y = tempPt1.y + 400;
> } else {
> tempRectanglePt[0].x = tempPt.x - 500;
> tempRectanglePt[0].y = tempPt.y - 400;
> tempRectanglePt[1].x = tempPt.x + 1000;
> tempRectanglePt[1].y = tempPt.y - 400;
> tempRectanglePt[2].x = tempPt.x + 1000;
> tempRectanglePt[2].y = tempPt.y + 400;
> tempRectanglePt[3].x = tempPt.x - 500;
> tempRectanglePt[3].y = tempPt.y + 400;
> tempRectanglePt1[0].x = tempPt1.x - 500;
> tempRectanglePt1[0].y = tempPt1.y - 400;
> tempRectanglePt1[1].x = tempPt1.x + 1000;
> tempRectanglePt1[1].y = tempPt1.y + 400;
> tempRectanglePt1[2].x = tempPt1.x + 1000;
> tempRectanglePt1[2].y = tempPt1.y + 400;
> tempRectanglePt1[3].x = tempPt1.x - 500;
> tempRectanglePt1[3].y = tempPt1.y + 400;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE)  // parall parkingout turn left
> {
> tempRectanglePt[0].x = tempPt.x - 500;
> tempRectanglePt[0].y = tempPt.y - 400;
> tempRectanglePt[1].x = tempPt.x + 1000;
> tempRectanglePt[1].y = tempPt.y - 400;
> tempRectanglePt[2].x = tempPt.x + 1000;
> tempRectanglePt[2].y = tempPt.y + 400;
> tempRectanglePt[3].x = tempPt.x - 500;
> tempRectanglePt[3].y = tempPt.y + 400;
> tempRectanglePt1[0].x = tempPt1.x - 500;
> tempRectanglePt1[0].y = tempPt1.y - 400;
> tempRectanglePt1[1].x = tempPt1.x + 1000;
> tempRectanglePt1[1].y = tempPt1.y - 400;
> tempRectanglePt1[2].x = tempPt1.x + 1000;
> tempRectanglePt1[2].y = tempPt1.y + 400;
> tempRectanglePt1[3].x = tempPt1.x - 500;
> tempRectanglePt1[3].y = tempPt1.y + 400;
> } else {
> tempRectanglePt[0].x = tempPt.x + 500;
> tempRectanglePt[0].y = tempPt.y - 400;
> tempRectanglePt[1].x = tempPt.x - 1000;
> tempRectanglePt[1].y = tempPt.y - 400;
> tempRectanglePt[2].x = tempPt.x - 1000;
> tempRectanglePt[2].y = tempPt.y + 400;
> tempRectanglePt[3].x = tempPt.x + 500;
> tempRectanglePt[3].y = tempPt.y + 400;
> tempRectanglePt1[0].x = tempPt1.x + 500;
> tempRectanglePt1[0].y = tempPt1.y - 400;
> tempRectanglePt1[1].x = tempPt1.x - 1000;
> tempRectanglePt1[1].y = tempPt1.y - 400;
> tempRectanglePt1[2].x = tempPt1.x - 1000;
> tempRectanglePt1[2].y = tempPt1.y + 400;
> tempRectanglePt1[3].x = tempPt1.x + 500;
> tempRectanglePt1[3].y = tempPt1.y + 400;
> }
> } else {
> return;
> }
> // get left rectangle 4 line
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt[0], tempRectanglePt[1],
> &LeftLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt[1], tempRectanglePt[2],
> &BottomLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt[2], tempRectanglePt[3],
> &RightLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt[3], tempRectanglePt[1],
> &TopLine);
> pRectanglRegionLine[0] = TopLine;
> pRectanglRegionLine[1] = BottomLine;
> pRectanglRegionLine[2] = LeftLine;
> pRectanglRegionLine[3] = RightLine;
> // get Right rectangle 4 line
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[0], tempRectanglePt1[1],
> &LeftLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[1], tempRectanglePt1[2],
> &BottomLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[2], tempRectanglePt1[3],
> &RightLine);
> AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[3], tempRectanglePt1[1],
> &TopLine);
> pRectanglRegionLine1[0] = TopLine;
> pRectanglRegionLine1[1] = BottomLine;
> pRectanglRegionLine1[2] = LeftLine;
> pRectanglRegionLine1[3] = RightLine;
> vObjInfo = &APAMap_GInputData.TotalMapInfo.mapData
> .ODInfo;  // getODobj,CheckRegionExistODObj
> vObjNum = (APA_ENUM_TYPE)vObjInfo->Square.ObjNum;
> if (vObjNum > 0) {
> RectRegionExistODObj = 0;
> RectRegionExistODObj1 = 0;
> for (uint16_t i = 0; i < vObjNum; i++) {
> if ((vObjInfo->Square.Quadrilaterals[i].ObjInfo.Label >=
> Obj_Label_Pedestrian) &&
> (vObjInfo->Square.Quadrilaterals[i].ObjInfo.Label <=
> Obj_Label_WarningTriangle)) {
> cPtInAreaNum = 0;
> cPtInAreaNum1 = 0;
> pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x);
> pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y);
> pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> bTurnToOppositeScenario =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
> if (bTurnToOppositeScenario == TRUE) {
> cPtInAreaNum++;
> }
> bTurnToOppositeScenario1 =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
> if (bTurnToOppositeScenario1 == TRUE) {
> cPtInAreaNum1++;
> }
> pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x);
> pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y);
> pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> bTurnToOppositeScenario =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
> if (bTurnToOppositeScenario == TRUE) {
> cPtInAreaNum++;
> }
> bTurnToOppositeScenario1 =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
> if (bTurnToOppositeScenario1 == TRUE) {
> cPtInAreaNum1++;
> }
> pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x);
> pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y);
> pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> bTurnToOppositeScenario =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
> if (bTurnToOppositeScenario == TRUE) {
> cPtInAreaNum++;
> }
> bTurnToOppositeScenario1 =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
> if (bTurnToOppositeScenario1 == TRUE) {
> cPtInAreaNum1++;
> }
> pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x);
> pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y);
> pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
> bTurnToOppositeScenario =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
> if (bTurnToOppositeScenario == TRUE) {
> cPtInAreaNum++;
> }
> bTurnToOppositeScenario1 =
> AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
> if (bTurnToOppositeScenario1 == TRUE) {
> cPtInAreaNum1++;
> }
> if (cPtInAreaNum > 1) {
> RectRegionExistODObj++;
> }
> if (cPtInAreaNum1 > 1) {
> RectRegionExistODObj1++;
> }
> }
> if (RectRegionExistODObj >= 3) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (bSlotDataAtRigthSide == FALSE) {
> CarLeftSideExistODObj = TRUE;
> } else {
> CarRightSideExistODObj = TRUE;
> }
> } else if (ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (bSlotDataAtRigthSide == FALSE) {
> CarLeftSideExistODObj = TRUE;
> } else {
> CarRightSideExistODObj = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE)  // parall parkingout turn left
> {
> CarForwardExistODObj = TRUE;
> } else {
> CarBackwardExistODObj = TRUE;
> }
> }
> } else {
> // CarLeftSideExistODObj = FALSE;
> }
> if (RectRegionExistODObj1 >= 3) {
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if (bSlotDataAtRigthSide == FALSE) {
> CarRightSideExistODObj = TRUE;
> } else {
> CarLeftSideExistODObj = TRUE;
> }
> } else if (ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if (bSlotDataAtRigthSide == FALSE) {
> CarRightSideExistODObj = TRUE;
> } else {
> CarLeftSideExistODObj = TRUE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE)  // parall parkingout turn left
> {
> CarBackwardExistODObj = TRUE;
> } else {
> CarForwardExistODObj = TRUE;
> }
> }
> } else {
> // CarRightSideExistODObj = FALSE;
> }
> }
> }
> pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData
> .RefercLineInfo;  // getslotcloselinetwopoint
> RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
> if (RefercLinePtNum > 0) {
> for (uint16_t i = 0; i < RefercLinePtNum; i++) {
> pSlotPt.x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fx;
> pSlotPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt1.fy;
> pSlotPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pSlotPt, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
> pSlotPt1.x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fx;
> pSlotPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
> .stRefercLineParam[i]
> .pt2.fy;
> pSlotPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pSlotPt1, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> if ((pSlotPt.x <= CurCarPos.Coordinate.x) &&
> (pSlotPt1.x <= CurCarPos.Coordinate.x) &&
> (pSlotPt.x > CurCarPos.Coordinate.x - 4500) &&
> (pSlotPt1.x > CurCarPos.Coordinate.x - 4500) &&
> (pSlotPt.y <= CurCarPos.Coordinate.y + 5000) &&
> (pSlotPt1.y <= CurCarPos.Coordinate.y + 5000) &&
> (pSlotPt.y > CurCarPos.Coordinate.y) &&
> (pSlotPt1.y > CurCarPos.Coordinate.y) &&
> (pSlotPt.x != pSlotPt1.x))  // checkthecarleftsideexistslot
> {
> CarLeftSideExistSlot = TRUE;
> } else {
> // CarLeftSideExistSlot = FALSE;
> }
> if ((pSlotPt.x >= CurCarPos.Coordinate.x) &&
> (pSlotPt1.x >= CurCarPos.Coordinate.x) &&
> (pSlotPt.x < CurCarPos.Coordinate.x + 4500) &&
> (pSlotPt1.x < CurCarPos.Coordinate.x + 4500) &&
> (pSlotPt.y <= CurCarPos.Coordinate.y + 5000) &&
> (pSlotPt1.y <= CurCarPos.Coordinate.y + 5000) &&
> (pSlotPt.y > CurCarPos.Coordinate.y) &&
> (pSlotPt1.y > CurCarPos.Coordinate.y) &&
> (pSlotPt.x != pSlotPt1.x))  // checkthecarRightsideexistslot
> {
> CarRightSideExistSlot = TRUE;
> } else {
> // CarRightSideExistSlot = FALSE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if ((pSlotPt.y <= (APAMap_ComCfg.LengthOfCar + 9000.0)) &&
> (pSlotPt1.y <= (APAMap_ComCfg.LengthOfCar + 9000.0)) &&
> (pSlotPt.y > APAMap_ComCfg.LengthOfCar) &&
> (pSlotPt1.y > APAMap_ComCfg.LengthOfCar) && (pSlotPt.x > -2500.0) &&
> (pSlotPt1.x > -2500.0) && (pSlotPt.x <= 2500.0) &&
> (pSlotPt1.x <= 2500.0)) {
> CarForwardExistSlot = TRUE;
> } else {
> }
> if ((pSlotPt.y <= (-APAMap_ComCfg.LenBetweenRAxisAndRBumper)) &&
> (pSlotPt1.y <= (-APAMap_ComCfg.LenBetweenRAxisAndRBumper)) &&
> (pSlotPt.y > (-APAMap_ComCfg.LenBetweenRAxisAndRBumper - 9000.0)) &&
> (pSlotPt1.y >
> (-APAMap_ComCfg.LenBetweenRAxisAndRBumper - 9000.0)) &&
> (pSlotPt.x > -2500.0) && (pSlotPt1.x > -2500.0) &&
> (pSlotPt.x <= 2500.0) && (pSlotPt1.x <= 2500.0)) {
> CarBackwardExistSlot = TRUE;
> } else {
> }
> }
> }
> } else {
> CarLeftSideExistSlot = FALSE;
> CarRightSideExistSlot = FALSE;
> CarForwardExistSlot = FALSE;
> CarBackwardExistSlot = FALSE;
> CarLeftSideExistODObj = FALSE;
> CarRightSideExistODObj = FALSE;
> CarForwardExistODObj = FALSE;
> CarBackwardExistODObj = FALSE;
> }
> *pCarLeftSideExistSlot = CarLeftSideExistSlot;
> *pCarRightSideExistSlot = CarRightSideExistSlot;
> *pCarForwardExistSlot = CarForwardExistSlot;
> *pCarBackwardExistSlot = CarBackwardExistSlot;
> *pCarLeftSideExistODObj = CarLeftSideExistODObj;
> *pCarRightSideExistODObj = CarRightSideExistODObj;
> *pCarForwardExistODObj = CarForwardExistODObj;
> *pCarBackwardExistODObj = CarBackwardExistODObj;
> return;
> }
> void APAMap_ParkingOutElectrFenceMapBulid(
> APACoordinateDataCalFloatType* pgetVPLSlotData,
> APACoordinateDataCalFloatType* pgetObj1Pt,
> APACoordinateDataCalFloatType* pgetObj2Pt,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCarSideToObj1LineDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCarSideToObj2LineDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCurSlotTopLineAngle,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCurSlotCloseLineAngle,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj1MoveDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj2MoveDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCloseLineMoveDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pSlotOutsideDis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pSlotInnerDis) {
> uint8_t_INF ParkOutMode;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType pPtcd, pPtb, pPta, pPth, pPtg, pPtgf, pPtfg,
> pPtf, pPte, pPtdc, tempPt, tempPt1;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, vDltAngle;
> APACoordinateDataCalFloatType OrgPt;
> APACoordinateDataCalFloatType tempSlotPt[4];
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis1, CloseLineMoveDis2;
> APA_DISTANCE_CAL_FLOAT_TYPE ObjLabelAngledObj1SafeDis,
> ObjLabelAngledObj2SafeDis;
> APA_DISTANCE_CAL_FLOAT_TYPE ObjParallelObj1SafeDis;
> APA_DISTANCE_CAL_FLOAT_TYPE ObjParallelObj2SafeDis;
> BOOLEAN CarLeftSideExistSlot;
> BOOLEAN CarRightSideExistSlot;
> BOOLEAN CarForwardExistSlot;
> BOOLEAN CarBackwardExistSlot;
> BOOLEAN CarLeftSideExistODObj;
> BOOLEAN CarRightSideExistODObj;
> BOOLEAN CarForwardExistODObj;
> BOOLEAN CarBackwardExistODObj;
> BOOLEAN ParkingOutClockwise;
> BOOLEAN CurSlotIsAngle;
> APACoordinateDataCalFloatType getVPLSlotData[SlotPtNum];
> uint8_t_INF i;
> APACoordinateDataCalFloatType getObj1Pt;
> APACoordinateDataCalFloatType getObj2Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj1LineDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj2LineDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CurSlotTopLineAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE CurSlotCloseLineAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1MoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2MoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE SlotOutsideDis;
> APA_DISTANCE_CAL_FLOAT_TYPE SlotInnerDis;
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
> APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACoordinateDataCalFloatType Data[100];
> APA_DISTANCE_TYPE ODNSegNum;
> st_MapODDataType* pODInfo;
> Obj_Information_t CurObjComInfo;
> APA_ENUM_TYPE DataNum;
> uint8_t_INF k;
> #endif
> for (i = 0; i < SlotPtNum; i++) {
> getVPLSlotData[i] = pgetVPLSlotData[i];
> }
> getObj1Pt = *pgetObj1Pt;
> getObj2Pt = *pgetObj2Pt;
> CarSideToObj1LineDis = *pCarSideToObj1LineDis;
> CarSideToObj2LineDis = *pCarSideToObj2LineDis;
> CurSlotTopLineAngle = *pCurSlotTopLineAngle;
> CurSlotCloseLineAngle = *pCurSlotCloseLineAngle;
> Obj1MoveDis = *pObj1MoveDis;
> Obj2MoveDis = *pObj2MoveDis;
> CloseLineMoveDis = *pCloseLineMoveDis;
> SlotOutsideDis = *pSlotOutsideDis;
> SlotInnerDis = *pSlotInnerDis;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> OrgAng = APAMap_GInfo.NewCordSysAng;
> OrgPt = APAMap_GInfo.NewCordSysOPt;
> tempPt = getObj1Pt;  //(0,0)
> tempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> tempPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下//(0,0)
> tempPt1 = getObj2Pt;            //(0,-2730)
> tempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> tempPt1, 0, OrgAng, OrgPt);  // APA转锚点坐标系下//(0,-2730)
> CarLeftSideExistSlot = FALSE;
> CarRightSideExistSlot = FALSE;
> CarForwardExistSlot = FALSE;
> CarBackwardExistSlot = FALSE;
> CarLeftSideExistODObj = FALSE;
> CarRightSideExistODObj = FALSE;
> CarForwardExistODObj = FALSE;
> CarBackwardExistODObj = FALSE;
> ParkingOutClockwise = FALSE;
> CurSlotIsAngle = FALSE;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> ObjLabelAngledObj1SafeDis = 300.0;
> ObjLabelAngledObj2SafeDis = 300.0;
> ObjParallelObj1SafeDis = 1800;
> ObjParallelObj2SafeDis = 1500;  // 900;
> #ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
> pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
> i = 0;
> k = 0;
> ODNSegNum = 0;
> DataNum = 0;
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
> (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
> (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
> (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
> (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
> (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
> (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
> (CurObjComInfo.Label == Obj_Label_WheelChock)) {
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> DataNum = 4;
> } else if (CurObjComInfo.Label == Obj_Label_Curb) {
> for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
> .Points.Point[k]
> .y;
> }
> DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
> } else {
> DataNum = 0;
> }
> if (DataNum != 0) {
> FOffset = 0;
> BOffset = 400;
> LOffset = 400;
> ROffset = 0;
> APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
> pRectPt, pRectLine);
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], DataNum,
> pRectPt, pRectLine)) {
> ODNSegNum++;
> break;
> }
> }
> i++;
> }
> if (ODNSegNum != 0) {
> ObjParallelObj2SafeDis = 1800;
> }
> #endif
> if (TRUE == bODWheelChockFlag) {
> ObjParallelObj2SafeDis = 2700;
> }
> for (i = 0; i < 4; i++) {
> tempSlotPt[i] = getVPLSlotData[i];
> tempSlotPt[i] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> tempSlotPt[i], 0, CurCarPos.CarAng, CurCarPos.Coordinate);
> }
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
> if (bSlotDataAtRigthSide == TRUE) {
> if (tempSlotPt[3].y > tempSlotPt[0].y + 200) {
> ParkingOutClockwise = TRUE;
> } else if (tempSlotPt[0].y > tempSlotPt[3].y + 200) {
> ParkingOutClockwise = FALSE;
> }
> } else {
> if (tempSlotPt[0].y > tempSlotPt[3].y + 200) {
> ParkingOutClockwise = TRUE;
> } else if (tempSlotPt[3].y > tempSlotPt[0].y + 200) {
> ParkingOutClockwise = FALSE;
> }
> }
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> if (bSlotDataAtRigthSide == TRUE) {
> if (tempSlotPt[1].y > tempSlotPt[2].y - 200) {
> ParkingOutClockwise = TRUE;
> } else if (tempSlotPt[1].y < tempSlotPt[2].y - 200) {
> ParkingOutClockwise = FALSE;
> }
> } else {
> if (tempSlotPt[1].y > tempSlotPt[2].y - 200) {
> ParkingOutClockwise = FALSE;
> } else if (tempSlotPt[1].y < tempSlotPt[2].y - 200) {
> ParkingOutClockwise = TRUE;
> }
> }
> } else {
> /* code */
> }
> vDltAngle = CurSlotTopLineAngle - CurSlotCloseLineAngle;
> if ((MATH_FABS(vDltAngle) > (100 * PI / 180.0)) ||
> (MATH_FABS(vDltAngle) < (80 * PI / 180.0))) {
> CurSlotIsAngle = TRUE;
> }
> APAMap_ParkingOutMapScenarioModeCheck(
> &CarLeftSideExistSlot, &CarRightSideExistSlot, &CarForwardExistSlot,
> &CarBackwardExistSlot, &CarLeftSideExistODObj, &CarRightSideExistODObj,
> &CarForwardExistODObj, &CarBackwardExistODObj);
> if ((ParkOutMode ==
> APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)  // perpendicularparkingoutmode
> || (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
> if (bSlotDataAtRigthSide == FALSE)  // perp,HeadParkingOut,leftside
> {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj1MoveDis = 300.0;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x - 500;
> pPtcd.y = tempPt.y + 500;
> pPtdc.x = tempPt.x - 500;
> pPtdc.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x - 1000;
> pPtcd.y = tempPt.y + 1000;
> pPtdc.x = tempPt.x - 1000;
> pPtdc.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = -1000.0;
> } else {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x - 500;
> pPtgf.y = tempPt1.y - 500;
> pPtfg.x = tempPt1.x - 500;
> pPtfg.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = 0.0;
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x - 1000;
> pPtgf.y = tempPt1.y - 1000;
> pPtfg.x = tempPt1.x - 1000;
> pPtfg.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = 0.0;
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt.y + 2000;
> pPta.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
> pPta.y = tempPt.y + 2000;
> pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
> pPth.y = tempPt1.y - 2000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt1.y - 2000;
> pPtf.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPte.y = pPtdc.y;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> } else  // perp,HeadParkingOut,rightside
> {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x + 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x + 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x + 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x + 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = (0.0);
> }
> pPta.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 500;
> pPta.y = tempPt1.y - 2000;
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt1.y - 2000;
> pPte.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPte.y = pPtdc.y;
> pPtf.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPtf.y = pPtfg.y;
> pPtg.x = pPtgf.x;
> pPtg.y = tempPt.y + 2000;
> pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
> pPth.y = tempPt.y + 200;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> }
> if (CurSlotIsAngle == TRUE) {
> if (bSlotDataAtRigthSide == FALSE)  // Perp AnlgSlot Leftside
> {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x - 500;
> pPtcd.y = tempPt.y + 500;
> pPtdc.x = tempPt.x - 500;
> pPtdc.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x - 1000;
> pPtcd.y = tempPt.y + 1000;
> pPtdc.x = tempPt.x - 1000;
> pPtdc.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x - 500;
> pPtgf.y = tempPt1.y - 500;
> pPtfg.x = tempPt1.x - 500;
> pPtfg.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-50.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x - 1000;
> pPtgf.y = tempPt1.y - 1000;
> pPtfg.x = tempPt1.x - 1000;
> pPtfg.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> if (ParkingOutClockwise == TRUE) {
> pPta.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPth.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPth.y = tempPt1.y - 2000;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
> } else {
> pPta.x = tempPt.x + 7000;
> pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPth.x = tempPt1.x + 7000;
> pPth.y = tempPt1.y - 2000;
> SlotOutsideDis = (7000.0);
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt1.y - 2000;
> pPtf.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPte.y = pPtdc.y;
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x + 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x + 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x + 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x + 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if (ParkingOutClockwise == TRUE) {
> pPta.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
> pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPth.y = tempPt.y + 2000;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
> } else {
> pPta.x = tempPt1.x - 7000;
> pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
> pPth.x = tempPt.x - 7000;
> pPth.y = tempPt.y + 2000;
> SlotOutsideDis = (7000.0);
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt.y + 2000;
> pPtf.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPte.y = pPtdc.y;
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> }
> }
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
> if (bSlotDataAtRigthSide == FALSE)  // perp,RearParkingOut,leftside
> {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj1MoveDis = 300.0;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x + 500;
> pPtgf.y = tempPt1.y - 500;
> pPtfg.x = tempPt1.x + 500;
> pPtfg.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x + 1000;
> pPtgf.y = tempPt1.y - 1000;
> pPtfg.x = tempPt1.x + 1000;
> pPtfg.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - 300;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - 300;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x + 500;
> pPtcd.y = tempPt.y + 500;
> pPtdc.x = tempPt.x + 500;
> pPtdc.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x + 1000;
> pPtcd.y = tempPt.y + 1000;
> pPtdc.x = tempPt.x + 1000;
> pPtdc.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + 300;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> CloseLineMoveDis1 = (0.0);
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt.y + 2000;
> pPta.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
> pPta.y = tempPt.y + 2000;
> pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
> pPth.y = tempPt1.y - 2000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt1.y - 2000;
> pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPte.y = pPtdc.y;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> } else  // perp,RearParkingOut,rightside
> {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = 300.0;
> Obj2MoveDis = 300.0;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x - 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = (500.0);
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> Obj2MoveDis = (300.0);
> CloseLineMoveDis2 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x - 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = (1000.0);
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - 300;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - 300;
> Obj2MoveDis = (300.0);
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x - 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = (500.0);
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = (300.0);
> CloseLineMoveDis1 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x - 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = (1000.0);
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + 300;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + 300;
> Obj1MoveDis = (300.0);
> CloseLineMoveDis1 = (0.0);
> }
> pPta.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 500;
> pPta.y = tempPt1.y - 2000;
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt1.y - 2000;
> pPte.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPte.y = pPtdc.y;
> pPtf.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPtf.y = pPtfg.y;
> pPtg.x = pPtgf.x;
> pPtg.y = tempPt.y + 2000;
> pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
> pPth.y = tempPt.y + 200;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> }
> if (CurSlotIsAngle == TRUE) {
> if (bSlotDataAtRigthSide == FALSE)  // angleslot parkingout leftside
> {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x + 500;
> pPtgf.y = tempPt1.y - 500;
> pPtfg.x = tempPt1.x + 500;
> pPtfg.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtgf.x = tempPt1.x + 1000;
> pPtgf.y = tempPt1.y - 1000;
> pPtfg.x = tempPt1.x + 1000;
> pPtfg.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtgf.x = tempPt1.x;
> pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtfg.x = tempPt1.x;
> pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x + 500;
> pPtcd.y = tempPt.y + 500;
> pPtdc.x = tempPt.x + 500;
> pPtdc.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt.x + 1000;
> pPtcd.y = tempPt.y + 1000;
> pPtdc.x = tempPt.x + 1000;
> pPtdc.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtcd.x = tempPt.x;
> pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt.x;
> pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if (ParkingOutClockwise == TRUE) {
> pPta.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
> pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPth.y = tempPt.y + 2000;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
> } else {
> pPta.x = tempPt1.x + 7000;
> pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
> pPth.x = tempPt.x + 7000;
> pPth.y = tempPt.y + 2000;
> SlotOutsideDis = (7000.0);
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt1.y - 2000;
> pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPte.y = pPtdc.y;
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> } else  // angleslot parkingout rightside
> {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj2SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x - 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarLeftSideExistSlot == FALSE) &&
> (CarLeftSideExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarLeftSideExistSlot == TRUE) &&
> (CarLeftSideExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x - 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
> Obj2MoveDis = ObjLabelAngledObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x - 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarRightSideExistSlot == FALSE) &&
> (CarRightSideExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarRightSideExistSlot == TRUE) &&
> (CarRightSideExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x - 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;
> Obj1MoveDis = ObjLabelAngledObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if (ParkingOutClockwise == TRUE) {
> pPta.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPth.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
> pPth.y = tempPt1.y - 2000;
> SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
> } else {
> pPta.x = tempPt.x - 7000;
> pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPth.x = tempPt1.x - 7000;
> pPth.y = tempPt1.y - 2000;
> SlotOutsideDis = (7000.0);
> }
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
> pPtg.x = pPtfg.x;
> pPtg.y = tempPt1.y - 2000;
> pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPtf.y = pPtfg.y;
> pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
> pPte.y = pPtdc.y;
> SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
> }
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE)  // Parall,ParkingOut,leftside
> {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarForwardExistSlot == FALSE) && (CarForwardExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x + 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarForwardExistSlot == FALSE) &&
> (CarForwardExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarForwardExistSlot == TRUE) &&
> (CarForwardExistODObj == TRUE)) {
> pPtfg.x = tempPt.x + 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x + 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if ((CarBackwardExistSlot == FALSE) && (CarBackwardExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x + 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarBackwardExistSlot == FALSE) &&
> (CarBackwardExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarBackwardExistSlot == TRUE) &&
> (CarBackwardExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x + 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x + 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> pPta.x = tempPt1.x - APAMap_ComCfg.WidthOfCar - 2500;
> pPta.y = tempPt1.y - 1000;
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt1.y - 1000;
> pPte.x = tempPt1.x + APAMap_ComCfg.WidthOfCar + 500;
> pPte.y = pPtdc.y;
> pPtf.x = tempPt.x + APAMap_ComCfg.WidthOfCar + 500;
> pPtf.y = pPtfg.y;
> pPtg.x = pPtgf.x;
> pPtg.y = tempPt.y + 4000;
> pPth.x = tempPt.x - APAMap_ComCfg.WidthOfCar - 2500;
> pPth.y = tempPt.y + 4000;
> SlotOutsideDis = (APAMap_ComCfg.WidthOfCar + 2500);
> SlotInnerDis = (-APAMap_ComCfg.WidthOfCar - 500);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis1 = 0.0;
> CloseLineMoveDis2 = 0.0;
> if ((CarForwardExistSlot == FALSE) && (CarForwardExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 500;
> pPtfg.y = tempPt.y + 500;
> pPtgf.x = tempPt.x - 500;
> pPtgf.y = tempPt.y + 500;
> Obj1MoveDis = 500.0;
> CloseLineMoveDis1 = (-500.0);
> } else if ((CarForwardExistSlot == FALSE) &&
> (CarForwardExistODObj == FALSE)) {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> } else if ((CarForwardExistSlot == TRUE) &&
> (CarForwardExistODObj == TRUE)) {
> pPtfg.x = tempPt.x - 1000;
> pPtfg.y = tempPt.y + 1000;
> pPtgf.x = tempPt.x - 1000;
> pPtgf.y = tempPt.y + 1000;
> Obj1MoveDis = 1000.0;
> CloseLineMoveDis1 = (-1000.0);
> } else {
> pPtfg.x = tempPt.x;
> pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
> pPtgf.x = tempPt.x;
> pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;
> Obj1MoveDis = ObjParallelObj1SafeDis;
> CloseLineMoveDis1 = (0.0);
> }
> if ((CarBackwardExistSlot == FALSE) && (CarBackwardExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 500;
> pPtcd.y = tempPt1.y - 500;
> pPtdc.x = tempPt1.x - 500;
> pPtdc.y = tempPt1.y - 500;
> Obj2MoveDis = 500.0;
> CloseLineMoveDis2 = (-500.0);
> } else if ((CarBackwardExistSlot == FALSE) &&
> (CarBackwardExistODObj == FALSE)) {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> } else if ((CarBackwardExistSlot == TRUE) &&
> (CarBackwardExistODObj == TRUE)) {
> pPtcd.x = tempPt1.x - 1000;
> pPtcd.y = tempPt1.y - 1000;
> pPtdc.x = tempPt1.x - 1000;
> pPtdc.y = tempPt1.y - 1000;
> Obj2MoveDis = 1000.0;
> CloseLineMoveDis2 = (-1000.0);
> } else {
> pPtcd.x = tempPt1.x;
> pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
> pPtdc.x = tempPt1.x;
> pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
> Obj2MoveDis = ObjParallelObj2SafeDis;
> CloseLineMoveDis2 = (0.0);
> }
> pPta.x = tempPt1.x + APAMap_ComCfg.WidthOfCar + 2500;
> pPta.y = tempPt1.y - 1000;
> pPtb.x = pPtdc.x;
> pPtb.y = tempPt1.y - 1000;
> pPte.x = tempPt1.x - APAMap_ComCfg.WidthOfCar - 500;
> pPte.y = pPtdc.y;
> pPtf.x = tempPt.x - APAMap_ComCfg.WidthOfCar - 500;
> pPtf.y = pPtfg.y;
> pPtg.x = pPtgf.x;
> pPtg.y = tempPt.y + 4000;
> pPth.x = tempPt.x + APAMap_ComCfg.WidthOfCar + 2500;
> pPth.y = tempPt.y + 4000;
> SlotOutsideDis = (APAMap_ComCfg.WidthOfCar + 2500);
> SlotInnerDis = (-APAMap_ComCfg.WidthOfCar - 500);
> }
> } else {
> return;
> }
> CloseLineMoveDis = (CloseLineMoveDis1 + CloseLineMoveDis2) / 2;
> Obj1MoveDis = Obj1MoveDis + CarSideToObj1LineDis;
> Obj2MoveDis = Obj2MoveDis + CarSideToObj2LineDis;
> if ((Obj1MoveDis < 400.0) && (CarSideToObj1LineDis <= 0)) {
> Obj1MoveDis = 400;
> }
> if ((Obj2MoveDis < 400.0) && (CarSideToObj2LineDis <= 0)) {
> Obj2MoveDis = 400;
> }
> APAMapEFOutputData.ElectronicFencePt[0].x = pPta.x;
> APAMapEFOutputData.ElectronicFencePt[0].y = pPta.y;
> APAMapEFOutputData.ElectronicFencePt[1].x = pPtb.x;
> APAMapEFOutputData.ElectronicFencePt[1].y = pPtb.y;
> APAMapEFOutputData.ElectronicFencePt[2].x = pPtcd.x;
> APAMapEFOutputData.ElectronicFencePt[2].y = pPtcd.y;
> APAMapEFOutputData.ElectronicFencePt[3].x = pPtdc.x;
> APAMapEFOutputData.ElectronicFencePt[3].y = pPtdc.y;
> APAMapEFOutputData.ElectronicFencePt[4].x = pPte.x;
> APAMapEFOutputData.ElectronicFencePt[4].y = pPte.y;
> APAMapEFOutputData.ElectronicFencePt[5].x = pPtf.x;
> APAMapEFOutputData.ElectronicFencePt[5].y = pPtf.y;
> APAMapEFOutputData.ElectronicFencePt[6].x = pPtfg.x;
> APAMapEFOutputData.ElectronicFencePt[6].y = pPtfg.y;
> APAMapEFOutputData.ElectronicFencePt[7].x = pPtgf.x;
> APAMapEFOutputData.ElectronicFencePt[7].y = pPtgf.y;
> APAMapEFOutputData.ElectronicFencePt[8].x = pPtg.x;
> APAMapEFOutputData.ElectronicFencePt[8].y = pPtg.y;
> APAMapEFOutputData.ElectronicFencePt[9].x = pPth.x;
> APAMapEFOutputData.ElectronicFencePt[9].y = pPth.y;
> APAMapEFOutputData.timeStamp_ms = APAMap_GInputData.CarLocInfo.timeStamp_ms;
> APAMapEFOutputData.CarPos.CarAng = CurCarPos.CarAng;
> APAMapEFOutputData.CarPos.Coordinate.x = CurCarPos.Coordinate.x;
> APAMapEFOutputData.CarPos.Coordinate.y = CurCarPos.Coordinate.y;
> for (i = 0; i < ElectrFencePtNum; i++) {
> APAMapEFOutputData.ElectronicFencePt[i] = AlgCom_PointPosWithAngAndCenterPt(
> APAMapEFOutputData.ElectronicFencePt[i], OrgAng, OrgPt);
> }
> for (i = 0; i < SlotPtNum; i++) {
> pgetVPLSlotData[i] = getVPLSlotData[i];
> }
> *pgetObj1Pt = getObj1Pt;
> *pgetObj2Pt = getObj2Pt;
> *pCarSideToObj1LineDis = CarSideToObj1LineDis;
> *pCarSideToObj2LineDis = CarSideToObj2LineDis;
> *pCurSlotTopLineAngle = CurSlotTopLineAngle;
> *pCurSlotCloseLineAngle = CurSlotCloseLineAngle;
> *pObj1MoveDis = Obj1MoveDis;
> *pObj2MoveDis = Obj2MoveDis;
> *pCloseLineMoveDis = CloseLineMoveDis;
> *pSlotOutsideDis = SlotOutsideDis;
> *pSlotInnerDis = SlotInnerDis;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ElectrFence==LfSideExistSlot(%d)==RtSideExistSlot(%d)\n"
> "==FdExistSlot(%d)==BdExistSlot(%d)==LfSideExistODObj(%d)=="
> "RtSideExistODObj(%d)==FdExistODObj(%d)==BdExistODObj(%d)\n"
> "==ParkingOutClockwise(%d)==CurSlotIsAngle(%d)==Obj1MoveDis(%f)=="
> "Obj2MoveDis(%f)==CloseLineMoveDis(%f)\n"
> "==SlotOutsideDis(%f)==SlotInnerDis(%f)",
> CarLeftSideExistSlot, CarRightSideExistSlot, CarForwardExistSlot,
> CarBackwardExistSlot, CarLeftSideExistODObj,
> CarRightSideExistODObj, CarForwardExistODObj,
> CarBackwardExistODObj, ParkingOutClockwise, CurSlotIsAngle,
> Obj1MoveDis, Obj2MoveDis, CloseLineMoveDis, SlotOutsideDis,
> SlotInnerDis);
> TLOG_INFO << log_string;
> }
> }
> BOOLEAN APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(
> APACoordinateDataCalFloatType* pVPLSlotPts,
> APACoordinateDataCalFloatType* pNewVPLSlotPts, APA_ENUM_TYPE* pOrgIndex) {
> uint8_t_INF ParkOutMode;
> BOOLEAN bSlotDataAtRigthSide;
> APA_ENUM_TYPE Obj2PtIndex;
> APA_ENUM_TYPE Obj1PtIndex;
> APA_ENUM_TYPE Obj2InnerIndex;
> APA_ENUM_TYPE Obj1InnerIndex;
> APALineParameterABCType CloseLine;
> APALineParameterABCType TopLine;
> APALineParameterABCType FarLine;
> APALineParameterABCType TempLine;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 0;     // Left_Top
> Obj1PtIndex = 1;     // Left_Bottom
> Obj2InnerIndex = 3;  // Right_Top
> Obj1InnerIndex = 2;  // Right_Bottom
> } else {
> Obj2PtIndex = 3;     // Right_Top
> Obj1PtIndex = 2;     // Right_Bottom
> Obj2InnerIndex = 0;  // Left_Top
> Obj1InnerIndex = 1;  // Left_Bottom
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1PtIndex],
> pVPLSlotPts[Obj2PtIndex], &CloseLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
> pVPLSlotPts[Obj2PtIndex], &CloseLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj2InnerIndex]);
> TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
> pVPLSlotPts[Obj1InnerIndex], &CloseLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 3;     // Right_Top
> Obj1PtIndex = 0;     // Left_Top
> Obj2InnerIndex = 2;  // Right_Bottom
> Obj1InnerIndex = 1;  // Left_Bottom
> } else {
> Obj2PtIndex = 0;     // Left_Top
> Obj1PtIndex = 3;     // Right_Top
> Obj2InnerIndex = 1;  // Left_Bottom
> Obj1InnerIndex = 2;  // Right_Bottom
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
> pVPLSlotPts[Obj2PtIndex], &TopLine);
> AlgCom_LineParABCByParallelLineAndPointOnLine(
> &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 1;     // Left_Bottom
> Obj1PtIndex = 2;     // Right_Bottom
> Obj2InnerIndex = 0;  // Left_Top
> Obj1InnerIndex = 3;  // Right_Top
> } else {
> Obj2PtIndex = 2;     // Right_Bottom
> Obj1PtIndex = 1;     // Left_Bottom
> Obj2InnerIndex = 3;  // Right_Top
> Obj1InnerIndex = 0;  // Left_Top
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
> pVPLSlotPts[Obj2PtIndex], &TopLine);
> AlgCom_LineParABCByParallelLineAndPointOnLine(
> &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else {
> // data errors;
> return FALSE;
> }
> pNewVPLSlotPts[0] = pVPLSlotPts[Obj2PtIndex];
> pNewVPLSlotPts[1] = pVPLSlotPts[Obj1PtIndex];
> pNewVPLSlotPts[2] = pVPLSlotPts[Obj1InnerIndex];
> pNewVPLSlotPts[3] = pVPLSlotPts[Obj2InnerIndex];
> pOrgIndex[0] = Obj2PtIndex;
> pOrgIndex[1] = Obj1PtIndex;
> pOrgIndex[2] = Obj1InnerIndex;
> pOrgIndex[3] = Obj2InnerIndex;
> return TRUE;
> }
> BOOLEAN APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot(
> APACoordinateDataCalFloatType* pObj2Pt,
> APACoordinateDataCalFloatType* pObj1Pt,
> APA_DISTANCE_CAL_FLOAT_TYPE* pSlotAng, APA_DISTANCE_CAL_FLOAT_TYPE ObjAng,
> APACoordinateDataCalFloatType* pVPLSlotPtsNearBy,
> BOOLEAN bSlotDataAtRigthSide, APA_ENUM_TYPE* pFailCause) {
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType TempPt1;
> APACoordinateDataCalFloatType TempPt2;
> APA_DISTANCE_CAL_FLOAT_TYPE SlotAngNearBy;
> APA_DISTANCE_CAL_FLOAT_TYPE ObjAngNearBy;
> APALineParameterABCType TempLine;
> APALineParameterABCType TempLine1;
> APA_DISTANCE_CAL_FLOAT_TYPE DeltaAng;
> APA_DISTANCE_CAL_FLOAT_TYPE NewSlotAng;
> BOOLEAN bRerAngSlot;
> APACarCoordinateDataCalFloatType TempCarPos;
> *pFailCause = 0;
> AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[1], pVPLSlotPtsNearBy[0],
> &SlotAngNearBy);
> DeltaAng = *pSlotAng - SlotAngNearBy;
> AlgCom_AngNormalized(&DeltaAng);
> if (MATH_FABS(DeltaAng) > 5.0 * PI / 180.0) {
> *pFailCause = 1;
> return FALSE;
> }
> AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[3], pVPLSlotPtsNearBy[0],
> &ObjAngNearBy);
> DeltaAng = ObjAng - ObjAngNearBy;
> AlgCom_AngNormalized(&DeltaAng);
> if (MATH_FABS(DeltaAng) > 5.0 * PI / 180.0) {
> *pFailCause = 2;
> return FALSE;
> }
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pVPLSlotPtsNearBy[0], 0, *pSlotAng, *pObj2Pt);
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pVPLSlotPtsNearBy[1], 0, *pSlotAng, *pObj2Pt);
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> *pObj1Pt, 0, *pSlotAng, *pObj2Pt);
> if (TempPt2.y > 0) {
> if (MATH_FABS(TempPt1.y) > 600)  // 300
> {
> *pFailCause = 3;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot===Test"
> "==TempPt2(%.2f,%.2f)==TempPt1(%.2f,%.2f)==TempPt(%.2f,%.2f)=="
> "MATH_FABS(TempPt2.y - TempPt.y))(%.2f)",
> TempPt2.x, TempPt2.y, TempPt1.x, TempPt1.y, TempPt.x, TempPt.y,
> MATH_FABS(TempPt2.y - TempPt.y));
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> if (((TempPt1.x < 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x > 0) && (bSlotDataAtRigthSide == FALSE))) {
> // 30~60;
> bRerAngSlot = FALSE;
> AlgCom_GetAngByTwoPts(*pObj1Pt, pVPLSlotPtsNearBy[1], &NewSlotAng);
> } else {
> // 120 ~150;
> bRerAngSlot = TRUE;
> AlgCom_GetAngByTwoPts(*pObj2Pt, pVPLSlotPtsNearBy[0], &NewSlotAng);
> }
> DeltaAng = NewSlotAng - *pSlotAng;
> AlgCom_AngNormalized(&DeltaAng);
> DeltaAng = MATH_FABS(DeltaAng);
> if (DeltaAng < 15.0 * PI / 180.0)  // 25
> {
> *pFailCause = 4;
> return FALSE;
> } else if (DeltaAng > 65.0 * PI / 180.0) {
> *pFailCause = 5;
> return FALSE;
> } else {
> if (bRerAngSlot == FALSE) {
> AlgCom_LineParABCbyTwoPoints(*pObj1Pt, pVPLSlotPtsNearBy[1], &TempLine);
> TempCarPos.CarAng = ObjAng;
> TempCarPos.Coordinate = *pObj2Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TRUE ==
> AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
> *pObj2Pt = TempPt1;
> *pSlotAng = NewSlotAng;
> return TRUE;
> } else {
> *pFailCause = 6;
> return FALSE;
> }
> } else {
> AlgCom_LineParABCbyTwoPoints(*pObj2Pt, pVPLSlotPtsNearBy[0], &TempLine);
> TempCarPos.CarAng = ObjAng;
> TempCarPos.Coordinate = *pObj1Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TRUE ==
> AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
> *pObj1Pt = TempPt1;
> *pSlotAng = NewSlotAng;
> return TRUE;
> } else {
> *pFailCause = 7;
> return FALSE;
> }
> }
> }
> } else {
> if (MATH_FABS(TempPt2.y - TempPt.y) > 600)  // 300
> {
> *pFailCause = 8;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot===Test"
> "==TempPt2(%.2f,%.2f)==TempPt1(%.2f,%.2f)==TempPt(%.2f,%.2f)=="
> "MATH_FABS(TempPt2.y - TempPt.y))(%.2f)",
> TempPt2.x, TempPt2.y, TempPt1.x, TempPt1.y, TempPt.x, TempPt.y,
> MATH_FABS(TempPt2.y - TempPt.y));
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> if (((TempPt2.x > 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt2.x < 0) && (bSlotDataAtRigthSide == FALSE))) {
> // 30~60;
> bRerAngSlot = FALSE;
> AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[1], *pObj1Pt, &NewSlotAng);
> } else {
> // 120 ~150;
> bRerAngSlot = TRUE;
> AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[0], *pObj2Pt, &NewSlotAng);
> }
> DeltaAng = NewSlotAng - *pSlotAng;
> AlgCom_AngNormalized(&DeltaAng);
> DeltaAng = MATH_FABS(DeltaAng);
> if (DeltaAng < 15.0 * PI / 180.0)  // 25
> {
> *pFailCause = 9;
> return FALSE;
> } else if (DeltaAng > 65.0 * PI / 180.0) {
> *pFailCause = 10;
> return FALSE;
> } else {
> if (bRerAngSlot == FALSE) {
> AlgCom_LineParABCbyTwoPoints(*pObj1Pt, pVPLSlotPtsNearBy[1], &TempLine);
> TempCarPos.CarAng = ObjAng;
> TempCarPos.Coordinate = *pObj2Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TRUE ==
> AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
> *pObj2Pt = TempPt1;
> *pSlotAng = NewSlotAng;
> return TRUE;
> } else {
> *pFailCause = 11;
> return FALSE;
> }
> } else {
> AlgCom_LineParABCbyTwoPoints(*pObj2Pt, pVPLSlotPtsNearBy[0], &TempLine);
> TempCarPos.CarAng = ObjAng;
> TempCarPos.Coordinate = *pObj1Pt;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> if (TRUE ==
> AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
> *pObj1Pt = TempPt1;
> *pSlotAng = NewSlotAng;
> return TRUE;
> } else {
> *pFailCause = 12;
> return FALSE;
> }
> }
> }
> }
> return FALSE;
> }
> BOOLEAN APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(
> APACoordinateDataCalFloatType* pVPLSlotPts,
> APACoordinateDataCalFloatType* pObj2Pt,
> APACoordinateDataCalFloatType* pObj1Pt,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Ang,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Ang,
> APA_DISTANCE_CAL_FLOAT_TYPE* pNewOrgAng,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Dis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Dis,
> APA_DISTANCE_CAL_FLOAT_TYPE* pCarOffsetX,
> APA_DISTANCE_CAL_FLOAT_TYPE* pMinSlotDpth,
> APA_DISTANCE_CAL_FLOAT_TYPE* pVPLSlotDpth) {
> APACarCoordinateDataCalFloatType CurCarPos;
> uint8_t_INF ParkOutMode;
> BOOLEAN bSlotDataAtRigthSide;
> APA_ENUM_TYPE Obj2PtIndex;
> APA_ENUM_TYPE Obj1PtIndex;
> APA_ENUM_TYPE Obj2InnerIndex;
> APA_ENUM_TYPE Obj1InnerIndex;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis2;
> APA_ENUM_TYPE LocStyle;
> APALineParameterABCType CloseLine;
> APALineParameterABCType TopLine;
> APALineParameterABCType FarLine;
> APALineParameterABCType TempLine;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, &pRectPt[0], &pRectLine[0]);
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 0;     // Left_Top
> Obj1PtIndex = 1;     // Left_Bottom
> Obj2InnerIndex = 3;  // Right_Top
> Obj1InnerIndex = 2;  // Right_Bottom
> } else {
> Obj2PtIndex = 3;     // Right_Top
> Obj1PtIndex = 2;     // Right_Bottom
> Obj2InnerIndex = 0;  // Left_Top
> Obj1InnerIndex = 1;  // Left_Bottom
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1PtIndex],
> pVPLSlotPts[Obj2PtIndex], &CloseLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
> pVPLSlotPts[Obj2PtIndex], &CloseLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj2InnerIndex]);
> TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
> pVPLSlotPts[Obj1InnerIndex], &CloseLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 3;     // Right_Top
> Obj1PtIndex = 0;     // Left_Top
> Obj2InnerIndex = 2;  // Right_Bottom
> Obj1InnerIndex = 1;  // Left_Bottom
> } else {
> Obj2PtIndex = 0;     // Left_Top
> Obj1PtIndex = 3;     // Right_Top
> Obj2InnerIndex = 1;  // Left_Bottom
> Obj1InnerIndex = 2;  // Right_Bottom
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
> pVPLSlotPts[Obj2PtIndex], &TopLine);
> AlgCom_LineParABCByParallelLineAndPointOnLine(
> &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
> if (bSlotDataAtRigthSide == TRUE) {
> Obj2PtIndex = 1;     // Left_Bottom
> Obj1PtIndex = 2;     // Right_Bottom
> Obj2InnerIndex = 0;  // Left_Top
> Obj1InnerIndex = 3;  // Right_Top
> } else {
> Obj2PtIndex = 2;     // Right_Bottom
> Obj1PtIndex = 1;     // Left_Bottom
> Obj2InnerIndex = 3;  // Right_Top
> Obj1InnerIndex = 0;  // Left_Top
> }
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
> pVPLSlotPts[Obj2PtIndex], &TopLine);
> AlgCom_LineParABCByParallelLineAndPointOnLine(
> &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
> AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
> pVPLSlotPts[Obj2InnerIndex], &FarLine);
> AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
> &pVPLSlotPts[Obj1InnerIndex]);
> } else {
> // data errors;
> return FALSE;
> }
> *pObj2Pt = pVPLSlotPts[Obj2PtIndex];
> *pObj1Pt = pVPLSlotPts[Obj1PtIndex];
> AlgCom_GetAngByTwoPts(pVPLSlotPts[Obj1PtIndex], pVPLSlotPts[Obj2PtIndex],
> pNewOrgAng);
> AlgCom_GetAngByTwoPts(pVPLSlotPts[Obj2InnerIndex], pVPLSlotPts[Obj2PtIndex],
> pObj2Ang);
> *pObj1Ang = *pObj2Ang;
> // Cal Obj2Dis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj2InnerIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj2PtIndex], &TempDis);
> if (((LocStyle == 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 0) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over TopLine;
> TempDis = -TempDis;
> }
> TempDis2 = TempDis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj2InnerIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj2InnerIndex], &TempDis);
> if (((LocStyle == 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 0) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over TopLine;
> TempDis = -TempDis;
> }
> if (TempDis2 > TempDis) {
> TempDis2 = TempDis;
> }
> *pObj2Dis = TempDis2;
> // Cal Obj1Dis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1InnerIndex], &pVPLSlotPts[Obj1PtIndex],
> &pRectPt[Obj1PtIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over BottomLine;
> TempDis = -TempDis;
> }
> TempDis1 = TempDis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1InnerIndex], &pVPLSlotPts[Obj1PtIndex],
> &pRectPt[Obj1InnerIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over BottomLine;
> TempDis = -TempDis;
> }
> if (TempDis1 > TempDis) {
> TempDis1 = TempDis;
> }
> *pObj1Dis = TempDis1;
> // Cal CarOffsetX;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj2PtIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> TempDis1 = TempDis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj1PtIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> if (TempDis1 > TempDis) {
> TempDis1 = TempDis;
> }
> *pCarOffsetX = TempDis1;
> // Cal pMinSlotDpth;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj2InnerIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> TempDis2 = TempDis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pRectPt[Obj1InnerIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> if (TempDis2 < TempDis) {
> TempDis2 = TempDis;
> }
> *pMinSlotDpth = TempDis2;
> // Cal pVPLSlotDpth;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pVPLSlotPts[Obj2InnerIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> TempDis2 = TempDis;
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
> &pVPLSlotPts[Obj1InnerIndex], &TempDis);
> if (((LocStyle == 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle == 1) && (bSlotDataAtRigthSide == FALSE))) {
> // CarCorPt Over CloseLine;
> TempDis = -TempDis;
> }
> if (TempDis2 < TempDis) {
> TempDis2 = TempDis;
> }
> *pVPLSlotDpth = TempDis2;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutGetSlotInfoFromLocVPLSlotByTotalMap===SlotPt:0(%.2f,%."
> "2f),1(%.2f,%.2f),"
> "2(%.2f,%.2f),3(%.2f,%.2f),Obj2Pt(%.2f,%.2f),Obj1Pt(%.2f,%.2f),"
> "ObjAng(%.2f),OrgAng(%.2f),Dis(%.2f,%.2f),CarOffsetX(%.2f),"
> "MinSlotDpth(%.2f),VPLSlotDpth(%.2f)",
> pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
> pVPLSlotPts[1].y, pVPLSlotPts[2].x, pVPLSlotPts[2].y,
> pVPLSlotPts[3].x, pVPLSlotPts[3].y, pObj2Pt->x, pObj2Pt->y,
> pObj1Pt->x, pObj1Pt->y, *pObj2Ang * 180.0 / PI,
> *pNewOrgAng * 180.0 / PI, *pObj2Dis, *pObj1Dis, *pCarOffsetX,
> *pMinSlotDpth, *pVPLSlotDpth);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> #if 1
> BOOLEAN APAMap_ParkingOutBuildSlotByOneSideNearbySlot(
> APACoordinateDataCalFloatType* pCurSegData,
> APACoordinateDataCalFloatType* pFirstSegData,
> APACoordinateDataCalFloatType* pSecondSegData,
> APACoordinateDataCalFloatType FirstNearByCarPosSlot,
> APACoordinateDataCalFloatType SecondNearByCarPosSlot,
> uint8_t_INF Data1Index, uint8_t_INF Data2Index, uint8_t Label,
> uint8_t_INF SlotSide) {
> /**
> * APA（自动泊车辅助）系统坐标数据计算相关变量声明
> * 使用浮点类型进行坐标数据计算
> */
> APACoordinateDataCalFloatType Data[4];  // 存储坐标数据的数组，包含4个元素
> uint8_t_INF i;                          //  循环计数器，8位无符号整数类型
> APACarCoordinateDataCalFloatType CurCarPos;  // 当前车辆位置坐标数据，浮点类型
> BOOLEAN bResult;
> APACoordinateDataCalFloatType FirstSegData[4];
> APACoordinateDataCalFloatType SecondSegData[4];
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> for (i = 0; i < 4; i++) {  //  循环处理4个数据点
> Data[i] = pCurSegData[i];
> FirstSegData[i] = pFirstSegData[i];
> SecondSegData[i] = pSecondSegData[i];
> }
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &FirstSegData[0], FirstNearByCarPosSlot, CurCarPos.CarAng);
> if (TRUE == bResult) {
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &SecondSegData[0], SecondNearByCarPosSlot, CurCarPos.CarAng);
> }
> if (FALSE == bResult) {
> return FALSE;
> }
> if (Label == Obj_Label_Ladder_Slot) {  //  处理阶梯车位（Ladder Slot）的情况
> if (0 == SlotSide)                   // left side slot
> {
> if (0 == Data1Index)  // FirstBottom  Data2Index == 2
> {
> Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
> Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
> Data[1] = FirstSegData[2];
> Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
> Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
> Data[3].x = 2 * Data[0].x - FirstSegData[0].x;
> Data[3].y = 2 * Data[0].y - FirstSegData[0].y;
> } else if (1 == Data1Index)  // FirstTop  Data2Index == 3
> {
> Data[0] = FirstSegData[3];
> Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
> Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
> Data[2].x = 2 * Data[1].x - FirstSegData[1].x;
> Data[2].y = 2 * Data[1].y - FirstSegData[1].y;
> Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
> Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
> } else if (2 == Data1Index)  // SecondBottom  Data2Index == 0
> {
> Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
> Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
> Data[1] = SecondSegData[2];
> Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
> Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
> Data[3].x = 2 * Data[0].x - SecondSegData[0].x;
> Data[3].y = 2 * Data[0].y - SecondSegData[0].y;
> } else if (3 == Data1Index)  // SecondTop  Data2Index == 1
> {
> Data[0] = SecondSegData[3];
> Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
> Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
> Data[2].x = 2 * Data[1].x - SecondSegData[1].x;
> Data[2].y = 2 * Data[1].y - SecondSegData[1].y;
> Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
> Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
> }
> } else  // right side slot
> {
> if (0 == Data1Index)  // FirstBottom  Data2Index == 2
> {
> Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
> Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
> Data[2] = FirstSegData[1];
> Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
> Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
> Data[0].x = 2 * Data[3].x - FirstSegData[3].x;
> Data[0].y = 2 * Data[3].y - FirstSegData[3].y;
> } else if (1 == Data1Index)  // FirstTop  Data2Index == 3
> {
> Data[3] = FirstSegData[0];
> Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
> Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
> Data[1].x = 2 * Data[2].x - FirstSegData[2].x;
> Data[1].y = 2 * Data[2].y - FirstSegData[2].y;
> Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
> Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
> } else if (2 == Data1Index)  // SecondBottom  Data2Index == 0
> {
> Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
> Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
> Data[2] = SecondSegData[1];
> Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
> Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
> Data[0].x = 2 * Data[3].x - SecondSegData[3].x;
> Data[0].y = 2 * Data[3].y - SecondSegData[3].y;
> } else if (3 == Data1Index)  // SecondTop  Data2Index == 1
> {
> Data[3] = SecondSegData[0];
> Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
> Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
> Data[1].x = 2 * Data[2].x - SecondSegData[2].x;
> Data[1].y = 2 * Data[2].y - SecondSegData[2].y;
> Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
> Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
> }
> }
> } else  // Obj_Label_Angled_Slot
> {
> if (0 == SlotSide)  // left side slot
> {
> if ((0 == Data1Index)      // FirstBottom  Data2Index == 2
> || (1 == Data1Index))  // FirstTop  Data2Index == 3
> {
> Data[0] = FirstSegData[3];
> Data[1] = FirstSegData[2];
> Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
> Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
> Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
> Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
> } else if ((2 == Data1Index)      // SecondBottom  Data2Index == 0
> || (3 == Data1Index))  // SecondTop  Data2Index == 1
> {
> Data[0] = SecondSegData[3];
> Data[1] = SecondSegData[2];
> Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
> Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
> Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
> Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
> }
> } else  // right side slot
> {
> if ((0 == Data1Index)      // FirstBottom  Data2Index == 2
> || (1 == Data1Index))  // FirstTop  Data2Index == 3
> {
> Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
> Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
> Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
> Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
> Data[2] = FirstSegData[1];
> Data[3] = FirstSegData[0];
> } else if ((2 == Data1Index)      // SecondBottom  Data2Index == 0
> || (3 == Data1Index))  // SecondTop  Data2Index == 1
> {
> Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
> Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
> Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
> Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
> Data[2] = SecondSegData[1];
> Data[3] = SecondSegData[0];
> }
> }
> }
> for (i = 0; i < 4; i++) {
> pCurSegData[i] = Data[i];
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildSlotByOneSideNearbySlot="
> "==SlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
> "Label(%d)==SlotSide(%d)==Data1Index(%d)",
> Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x, Data[2].y,
> Data[3].x, Data[3].y, Label, SlotSide, Data1Index);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> BOOLEAN APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
> uint8_t* pLabel, APACoordinateDataCalFloatType* pData,
> uint8_t_INF* pNearBySlotNumsByAngled, uint8_t_INF* pNearBySlotNumsByLadder,
> uint8_t_INF* pData1Index, APACoordinateDataCalFloatType* pNearByCarPosSlot1,
> APACoordinateDataCalFloatType* pData1, uint8_t_INF* pSlotSideIndex1) {
> APACoordinateDataCalFloatType NearByCarPosSlot[4];
> APACoordinateDataCalFloatType NearByCarPosSlot1;
> APACoordinateDataCalFloatType NearByCarPosSlot2;
> uint8_t_INF m;
> uint8_t_INF k;
> APACoordinateDataCalFloatType Data[4];
> APACoordinateDataCalFloatType Data1[4];
> APACoordinateDataCalFloatType Data2[4];
> uint8_t_INF Data1Index;
> uint8_t_INF Data2Index;
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> BOOLEAN bResult1;
> APACarCoordinateDataCalFloatType CurCarPos;
> uint8_t_INF NearBySlotNumsByLadder;
> uint8_t_INF NearBySlotNumsByAngled;
> uint8_t_INF ParkOutMode;
> uint8_t_INF SlotSide;
> uint8_t_INF SlotSideIndex1;
> uint8_t_INF SlotSideIndex2;
> BOOLEAN bSearch;
> uint8_t Label;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> return FALSE;
> }
> bResult1 = FALSE;
> Data1Index = *pData1Index;
> Data2Index = 0;
> NearByCarPosSlot1 = *pNearByCarPosSlot1;
> NearBySlotNumsByLadder = *pNearBySlotNumsByLadder;
> NearBySlotNumsByAngled = *pNearBySlotNumsByAngled;
> SlotSide = 0;
> SlotSideIndex1 = *pSlotSideIndex1;
> SlotSideIndex2 = 0;
> Label = *pLabel;
> for (k = 0; k < 4; k++) {
> Data[k] = pData[k];
> Data1[k] = pData1[k];
> }
> CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> bSearch = TRUE;
> while (bSearch) {
> NearByCarPosSlot[0].x = -(CarWidth + 500);  // FirstBottom
> NearByCarPosSlot[0].y = -1500;
> NearByCarPosSlot[1].x = -(CarWidth + 500);  // FirstTop
> NearByCarPosSlot[1].y = 1500;
> NearByCarPosSlot[2].x = -(2 * CarWidth + 1000);  // SecondBottom
> NearByCarPosSlot[2].y = -3000;
> NearByCarPosSlot[3].x = -(2 * CarWidth + 1000);  // SecondTop
> NearByCarPosSlot[3].y = 3000;
> // SlotSide 0:left , 1:right
> if (SlotSideIndex1 == 1) {
> SlotSide = 1;
> }
> if (SlotSide == 1) {
> for (k = 0; k < 4; k++) {
> NearByCarPosSlot[k].x = -NearByCarPosSlot[k].x;
> }
> }
> if ((Label == Obj_Label_Angled_Slot) || (Label == Obj_Label_Ladder_Slot)) {
> for (k = 0; k < 4; k++) {
> NearByCarPosSlot[k] = AlgCom_PointPosWithAngAndCenterPt(
> NearByCarPosSlot[k], CurCarPos.CarAng, CurCarPos.Coordinate);
> bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&NearByCarPosSlot[k],
> &Data[0], 4);
> if (TRUE == bResult1) {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot==Valid"
> "==bResult1(%d)==NearBySlotNumsByAngled(%d)=="
> "NearBySlotNumsByLadder(%d)==SlotSide(%d)"
> "==NearByCarPosSlot:(k(%d)(%.2f,%.2f))",
> bResult1, NearBySlotNumsByAngled, NearBySlotNumsByLadder,
> SlotSide, k, NearByCarPosSlot[k].x, NearByCarPosSlot[k].y);
> TLOG_INFO << log_string;
> }
> if (TRUE == bResult1) {
> // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
> if (NearBySlotNumsByLadder == 1) {
> if ((Label == Obj_Label_Perpen_Slot) ||
> (Label == Obj_Label_Parall_Slot)) {
> Label = Obj_Label_Ladder_Slot;
> }
> }
> if ((Label == Obj_Label_Angled_Slot) ||
> (Label == Obj_Label_Ladder_Slot)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot="
> "=Valid==Label(%d)",
> Label);
> TLOG_INFO << log_string;
> }
> if (Label == Obj_Label_Angled_Slot) {
> NearBySlotNumsByAngled++;
> } else if (Label == Obj_Label_Ladder_Slot) {
> NearBySlotNumsByLadder++;
> }
> if ((NearBySlotNumsByAngled == 1) || (NearBySlotNumsByLadder == 1)) {
> // first slot
> for (m = 0; m < 4; m++) {
> memcpy(&Data1[m], &Data[m], sizeof(Data1[m]));
> }
> Data1Index = k;
> memcpy(&NearByCarPosSlot1, &NearByCarPosSlot[k],
> sizeof(NearByCarPosSlot1));
> SlotSideIndex1 = SlotSide;
> } else if ((NearBySlotNumsByAngled >= 2)  // 单边两个斜列车位
> || (NearBySlotNumsByLadder >= 2))  // 单边两个斜列阶梯车位
> {
> Data2Index = k;
> SlotSideIndex2 = SlotSide;
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot=="
> "==Data1Index(%d)==Data2Index(%d)==SlotSideIndex1(%d)=="
> "SlotSideIndex2(%d)",
> Data1Index, Data2Index, SlotSideIndex1, SlotSideIndex2);
> TLOG_INFO << log_string;
> }
> if ((SlotSideIndex1 == SlotSideIndex2) &&
> (((Data1Index == 0) && (Data2Index == 2)) ||
> ((Data1Index == 1) && (Data2Index == 3)) ||
> ((Data1Index == 2) && (Data2Index == 0)) ||
> ((Data1Index == 3) && (Data2Index == 1)))) {
> for (m = 0; m < 4; m++) {
> memcpy(&Data2[m], &Data[m], sizeof(Data2[m]));
> }
> memcpy(&NearByCarPosSlot2, &NearByCarPosSlot[k],
> sizeof(NearByCarPosSlot2));
> } else {
> bResult1 = FALSE;
> continue;
> }
> // build CurCarPosSlot
> bResult1 = APAMap_ParkingOutBuildSlotByOneSideNearbySlot(
> &Data[0], &Data1[0], &Data2[0], NearByCarPosSlot1,
> NearByCarPosSlot2, Data1Index, Data2Index, Label, SlotSide);
> }
> break;
> }
> }
> }
> if (FALSE == bResult1) {
> SlotSide++;
> } else {
> bSearch = FALSE;
> }
> if (SlotSide >= 2) {
> SlotSide = 0;
> bSearch = FALSE;
> }
> }
> if (TRUE == bResult1) {
> if ((NearBySlotNumsByAngled < 2) && (NearBySlotNumsByLadder < 2)) {
> bResult1 = FALSE;
> }
> }
> if (TRUE == bResult1) {
> if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
> Label = Obj_Label_Angled_Slot;
> }
> }
> *pData1Index = Data1Index;
> *pNearByCarPosSlot1 = NearByCarPosSlot1;
> *pNearBySlotNumsByAngled = NearBySlotNumsByAngled;
> *pNearBySlotNumsByLadder = NearBySlotNumsByLadder;
> *pSlotSideIndex1 = SlotSideIndex1;
> *pLabel = Label;
> for (k = 0; k < 4; k++) {
> pData[k] = Data[k];
> pData1[k] = Data1[k];
> }
> if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot=="
> "bResult1(%d)==SlotSide(%d)==Label(%d)="
> "=FirstSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)="
> "=NearByCarPosSlot1(%.2f,%.2f)==Data1Index(%d)="
> "=SecondSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)"
> "==NearByCarPosSlot2(%.2f,%.2f)==Data2Index(%d)=",
> bResult1, SlotSide, Label, Data1[0].x, Data1[0].y, Data1[1].x,
> Data1[1].y, Data1[2].x, Data1[2].y, Data1[3].x, Data1[3].y,
> NearByCarPosSlot1.x, NearByCarPosSlot1.y, Data1Index, Data2[0].x,
> Data2[0].y, Data2[1].x, Data2[1].y, Data2[2].x, Data2[2].y,
> Data2[3].x, Data2[3].y, NearByCarPosSlot2.x, NearByCarPosSlot2.y,
> Data2Index);
> TLOG_INFO << log_string;
> }
> return bResult1;
> }
> BOOLEAN APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
> uint8_t* pLabel, APACoordinateDataCalFloatType* pData,
> uint8_t_INF* pNearBySlotNumsByAngled, uint8_t_INF* pNearBySlotNumsByLadder,
> uint8_t_INF* pNearBySlotNumsByPerpen, uint8_t_INF* pData1Index,
> APACoordinateDataCalFloatType* pNearByCarPosSlot1,
> APACoordinateDataCalFloatType* pData1) {
> APACoordinateDataCalFloatType NearByCarPosSlot[4];
> APACoordinateDataCalFloatType NearByCarPosSlot1;
> APACoordinateDataCalFloatType NearByCarPosSlot2;
> uint8_t_INF m;
> uint8_t_INF k;
> APACoordinateDataCalFloatType Data[4];
> APACoordinateDataCalFloatType Data1[4];
> APACoordinateDataCalFloatType Data2[4];
> uint8_t_INF Data1Index;
> uint8_t_INF Data2Index;
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> BOOLEAN bResult;
> BOOLEAN bResult1;
> APACarCoordinateDataCalFloatType CurCarPos;
> uint8_t_INF NearBySlotNumsByLadder;
> uint8_t_INF NearBySlotNumsByAngled;
> uint8_t_INF NearBySlotNumsByPerpen;
> uint8_t_INF ParkOutMode;
> uint8_t Label;
> APA_DISTANCE_CAL_FLOAT_TYPE Dis1, Dis2;
> APA_DISTANCE_CAL_FLOAT_TYPE Dis;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> return FALSE;
> }
> bResult1 = FALSE;
> Data1Index = *pData1Index;
> Data2Index = 0;
> NearByCarPosSlot1 = *pNearByCarPosSlot1;
> NearBySlotNumsByLadder = *pNearBySlotNumsByLadder;
> NearBySlotNumsByAngled = *pNearBySlotNumsByAngled;
> NearBySlotNumsByPerpen = *pNearBySlotNumsByPerpen;
> Label = *pLabel;
> for (k = 0; k < 4; k++) {
> Data[k] = pData[k];
> Data1[k] = pData1[k];
> }
> CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> NearByCarPosSlot[0].x = -(CarWidth + 500);  // BottomLeft
> NearByCarPosSlot[0].y = -2000;
> NearByCarPosSlot[1].x = (CarWidth + 500);  // BottomRight
> NearByCarPosSlot[1].y = -2000;
> NearByCarPosSlot[2].x = (CarWidth + 500);  // TopRight
> NearByCarPosSlot[2].y = 2000;
> NearByCarPosSlot[3].x = -(CarWidth + 500);  // TopLeft
> NearByCarPosSlot[3].y = 2000;
> if ((Label == Obj_Label_Angled_Slot) || (Label == Obj_Label_Ladder_Slot) ||
> (Label == Obj_Label_Perpen_Slot) || (Label == Obj_Label_Parall_Slot)) {
> for (k = 0; k < 4; k++) {
> NearByCarPosSlot[k] = AlgCom_PointPosWithAngAndCenterPt(
> NearByCarPosSlot[k], CurCarPos.CarAng, CurCarPos.Coordinate);
> bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&NearByCarPosSlot[k],
> &Data[0], 4);
> if (TRUE == bResult1) {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot==Valid"
> "==bResult1(%d)==NearBySlotNumsByAngled(%d)=="
> "NearBySlotNumsByLadder(%d)==NearBySlotNumsByPerpen(%d)==k(%d)",
> bResult1, NearBySlotNumsByAngled, NearBySlotNumsByLadder,
> NearBySlotNumsByPerpen, k);
> TLOG_INFO << log_string;
> }
> if (TRUE == bResult1) {
> // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
> if (NearBySlotNumsByLadder == 1) {
> if ((Label == Obj_Label_Perpen_Slot) ||
> (Label == Obj_Label_Parall_Slot)) {
> Label = Obj_Label_Ladder_Slot;
> }
> }
> // 代表当前周围车位为垂直车位，但车位与当前车辆的距离达到了阶梯车位水平的偏差，把车位类型修正为阶梯车位
> if (Label == Obj_Label_Perpen_Slot) {
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &Data[0], NearByCarPosSlot[k], CurCarPos.CarAng);
> if (TRUE == bResult) {
> Dis1 =
> (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg
> .LenBetweenRAxisAndRBumper +
> APAMap_ComCfg.HalfWidthOfCar +
> 300);
> Dis2 =
> (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg
> .LenBetweenRAxisAndFBumper +
> APAMap_ComCfg.HalfWidthOfCar);
> if (k == 0) {  // BottomLeft
> Dis = AlgCom_GetTwoPointDisFloat(Data[2], CurCarPos.Coordinate);
> if (Dis > Dis1) {
> Label = Obj_Label_Ladder_Slot;
> }
> } else if (k == 1) {  // BottomRight
> Dis = AlgCom_GetTwoPointDisFloat(Data[1], CurCarPos.Coordinate);
> if (Dis > Dis1) {
> Label = Obj_Label_Ladder_Slot;
> }
> } else if (k == 2) {  // TopRight
> Dis = AlgCom_GetTwoPointDisFloat(Data[0], CurCarPos.Coordinate);
> if (Dis > Dis2) {
> Label = Obj_Label_Ladder_Slot;
> }
> } else if (k == 3) {  // TopLeft
> Dis = AlgCom_GetTwoPointDisFloat(Data[3], CurCarPos.Coordinate);
> if (Dis > Dis2) {
> Label = Obj_Label_Ladder_Slot;
> }
> }
> }
> }
> if ((Label == Obj_Label_Angled_Slot) ||
> (Label == Obj_Label_Ladder_Slot)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot=="
> "Valid==Label(%d)",
> Label);
> TLOG_INFO << log_string;
> }
> if (Label == Obj_Label_Angled_Slot) {
> NearBySlotNumsByAngled++;
> } else if (Label == Obj_Label_Ladder_Slot) {
> NearBySlotNumsByLadder++;
> } else {
> NearBySlotNumsByPerpen++;
> }
> if ((NearBySlotNumsByAngled == 1) || (NearBySlotNumsByLadder == 1) ||
> (NearBySlotNumsByPerpen == 1)) {
> for (m = 0; m < 4; m++) {
> memcpy(&Data1[m], &Data[m], sizeof(Data1[m]));
> }
> Data1Index = k;
> memcpy(&NearByCarPosSlot1, &NearByCarPosSlot[k],
> sizeof(NearByCarPosSlot1));
> bResult1 = FALSE;
> } else if ((NearBySlotNumsByAngled >= 2)  // 左右两边各一个斜列车位
> ||
> (NearBySlotNumsByLadder >= 2)  // 左右两边各一个斜列阶梯车位
> || (NearBySlotNumsByPerpen >= 2))  // 左右两边各一个垂直车位
> {
> Data2Index = k;
> if ((((Data1Index == 0) || (Data1Index == 3)) && (Data2Index != 0) &&
> ((Data2Index != 3))) ||
> (((Data1Index == 1) || (Data1Index == 2)) && (Data2Index != 1) &&
> ((Data2Index != 2)))) {
> for (m = 0; m < 4; m++) {
> memcpy(&Data2[m], &Data[m], sizeof(Data2[m]));
> }
> memcpy(&NearByCarPosSlot2, &NearByCarPosSlot[k],
> sizeof(NearByCarPosSlot2));
> } else {
> bResult1 = FALSE;
> continue;
> }
> // build CurCarPosSlot
> bResult1 = APAMap_ParkingOutBuildSlotByTwoNearbySlot(
> &Data[0], &Data1[0], &Data2[0], NearByCarPosSlot1,
> NearByCarPosSlot2, Data1Index, Data2Index, Label);
> }
> break;
> }
> }
> }
> if (TRUE == bResult1) {
> if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
> Label = Obj_Label_Angled_Slot;
> } else if (NearBySlotNumsByPerpen >= 2) {
> Label = Obj_Label_Perpen_Slot;
> }
> }
> *pData1Index = Data1Index;
> *pNearByCarPosSlot1 = NearByCarPosSlot1;
> *pNearBySlotNumsByAngled = NearBySlotNumsByAngled;
> *pNearBySlotNumsByLadder = NearBySlotNumsByLadder;
> *pNearBySlotNumsByPerpen = NearBySlotNumsByPerpen;
> *pLabel = Label;
> for (k = 0; k < 4; k++) {
> pData[k] = Data[k];
> pData1[k] = Data1[k];
> }
> if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2) ||
> (NearBySlotNumsByPerpen >= 2)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot==bResult1(%"
> "d)==Label(%d)="
> "=FirstSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)="
> "=NearByCarPosSlot1(%.2f,%.2f)==Data1Index(%d)="
> "=SecondSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)"
> "==NearByCarPosSlot2(%.2f,%.2f)==Data2Index(%d)=",
> bResult1, Label, Data1[0].x, Data1[0].y, Data1[1].x, Data1[1].y,
> Data1[2].x, Data1[2].y, Data1[3].x, Data1[3].y,
> NearByCarPosSlot1.x, NearByCarPosSlot1.y, Data1Index, Data2[0].x,
> Data2[0].y, Data2[1].x, Data2[1].y, Data2[2].x, Data2[2].y,
> Data2[3].x, Data2[3].y, NearByCarPosSlot2.x, NearByCarPosSlot2.y,
> Data2Index);
> TLOG_INFO << log_string;
> }
> return bResult1;
> }
> BOOLEAN APAMap_ParkingOutBuildSlotByTwoNearbySlot(
> APACoordinateDataCalFloatType* pCurSegData,
> APACoordinateDataCalFloatType* pFirstSegData,
> APACoordinateDataCalFloatType* pSecondSegData,
> APACoordinateDataCalFloatType FirstNearByCarPosSlot,
> APACoordinateDataCalFloatType SecondNearByCarPosSlot,
> uint8_t_INF Data1Index, uint8_t_INF Data2Index, uint8_t Label) {
> APACoordinateDataCalFloatType Data[4];
> APACoordinateDataCalFloatType NSegment[2];
> uint8_t_INF i;
> APACarCoordinateDataCalFloatType CurCarPos;
> BOOLEAN bResult;
> APACoordinateDataCalFloatType TempCarPos;
> APALineParameterKBType LaneLineKBType;
> APALineParameterKBType LaneLineKBType2;
> APALineParameterABCType TempLine1;
> APALineParameterABCType TempLine2;
> APACoordinateDataCalFloatType FirstSegData[4];
> APACoordinateDataCalFloatType SecondSegData[4];
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> for (i = 0; i < 4; i++) {
> Data[i] = pCurSegData[i];
> FirstSegData[i] = pFirstSegData[i];
> SecondSegData[i] = pSecondSegData[i];
> }
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &FirstSegData[0], FirstNearByCarPosSlot, CurCarPos.CarAng);
> if (TRUE == bResult) {
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &SecondSegData[0], SecondNearByCarPosSlot, CurCarPos.CarAng);
> }
> if (FALSE == bResult) {
> return FALSE;
> }
> if (Label == Obj_Label_Angled_Slot) {
> if ((0 == Data1Index)  // Data2Index == 1,2
> || (3 == Data1Index)) {
> Data[0] = FirstSegData[3];
> Data[1] = FirstSegData[2];
> Data[2] = SecondSegData[1];
> Data[3] = SecondSegData[0];
> } else if ((1 == Data1Index)  // Data2Index == 0,3
> || (2 == Data1Index)) {
> Data[0] = SecondSegData[3];
> Data[1] = SecondSegData[2];
> Data[2] = FirstSegData[1];
> Data[3] = FirstSegData[0];
> } else {
> }
> } else  // Obj_Label_Ladder_Slot || Obj_Label_Perpen_Slot
> {
> if (0 == Data1Index)  // BottomLeft
> {
> Data[1] = FirstSegData[2];
> Data[3] = SecondSegData[0];
> /***
> * 3        2
> * ----------
> * |        |
> * | First  |
> * |        31--------2
> * |        |        |
> * ---------|        |
> * 0       1|  Cur   |3-------2
> *          |        |        |
> *          0-------1|        |
> *                   | Second |
> *                   |        |
> *                   0--------1
> */
> NSegment[0] = FirstSegData[0];
> NSegment[1] = SecondSegData[0];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = FirstSegData[2];
> NSegment[1] = FirstSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[0].x = FirstSegData[2].x;
> Data[0].y = FirstSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[0] = TempCarPos;
> }
> NSegment[0] = FirstSegData[2];
> NSegment[1] = SecondSegData[2];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = SecondSegData[0];
> NSegment[1] = SecondSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[2].x = SecondSegData[0].x;
> Data[2].y = SecondSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[2] = TempCarPos;
> }
> } else if (1 == Data1Index)  // BottomRight
> {
> Data[0] = SecondSegData[3];
> Data[2] = FirstSegData[1];
> NSegment[0] = FirstSegData[1];
> NSegment[1] = SecondSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = SecondSegData[2];
> NSegment[1] = SecondSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[1].x = SecondSegData[2].x;
> Data[1].y = SecondSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[1] = TempCarPos;
> }
> NSegment[0] = FirstSegData[3];
> NSegment[1] = SecondSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = FirstSegData[0];
> NSegment[1] = FirstSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[3].x = FirstSegData[0].x;
> Data[3].y = FirstSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[3] = TempCarPos;
> }
> } else if (2 == Data1Index)  // TopRight
> {
> Data[1] = SecondSegData[2];
> Data[3] = FirstSegData[0];
> NSegment[0] = FirstSegData[0];
> NSegment[1] = SecondSegData[0];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = SecondSegData[2];
> NSegment[1] = SecondSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[0].x = SecondSegData[2].x;
> Data[0].y = SecondSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[0] = TempCarPos;
> }
> NSegment[0] = FirstSegData[2];
> NSegment[1] = SecondSegData[2];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = FirstSegData[0];
> NSegment[1] = FirstSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[2].x = FirstSegData[0].x;
> Data[2].y = FirstSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[2] = TempCarPos;
> }
> } else if (3 == Data1Index)  // TopLeft
> {
> Data[0] = FirstSegData[3];
> Data[2] = SecondSegData[1];
> NSegment[0] = FirstSegData[1];
> NSegment[1] = SecondSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = FirstSegData[2];
> NSegment[1] = FirstSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[1].x = FirstSegData[2].x;
> Data[1].y = FirstSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[1] = TempCarPos;
> }
> NSegment[0] = FirstSegData[3];
> NSegment[1] = SecondSegData[3];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
> TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
> NSegment[0] = SecondSegData[0];
> NSegment[1] = SecondSegData[1];
> APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
> if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
> (MATH_FABS(LaneLineKBType2.K) > 500)) {
> Data[3].x = SecondSegData[0].x;
> Data[3].y = SecondSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
> } else {
> TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
> Data[3] = TempCarPos;
> }
> } else {
> }
> }
> for (i = 0; i < 4; i++) {
> pCurSegData[i] = Data[i];
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutBuildSlotByTwoNearbySlot="
> "==SlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)",
> Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x, Data[2].y,
> Data[3].x, Data[3].y);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> BOOLEAN APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(
> APACoordinateDataCalFloatType* pObj2Pt,
> APACoordinateDataCalFloatType* pObj1Pt,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Ang,
> APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Ang,
> APA_DISTANCE_CAL_FLOAT_TYPE* pNewOrgAng,
> APA_DISTANCE_CAL_FLOAT_TYPE* pMaxSubLane,
> APA_DISTANCE_CAL_FLOAT_TYPE* pMaxSlotInnerX) {
> st_MapODDataType* pODInfo;
> BOOLEAN bResult;  // 搜索结果
> BOOLEAN bSearch;
> uint8_t_INF i;
> uint8_t_INF j;
> uint8_t_INF k;
> APACoordinateDataCalFloatType Data[4];
> Obj_Information_t CurObjComInfo;
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType Obj2Pt;
> APACoordinateDataCalFloatType Obj1Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE NewOrgAng;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Dis;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1Dis;
> APA_DISTANCE_CAL_FLOAT_TYPE CarOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MinSlotDpth;
> APA_DISTANCE_CAL_FLOAT_TYPE VPLSlotDpth;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxObj2Dis;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxObj1Dis;
> APA_DISTANCE_CAL_FLOAT_TYPE CloseLineOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSubLaneX;
> APA_DISTANCE_CAL_FLOAT_TYPE MoveObj2Dis;
> APA_DISTANCE_CAL_FLOAT_TYPE MoveObj1Dis;
> APACarCoordinateDataCalFloatType TempCarPos;
> APALineParameterABCType TopLine, BotLine, CloseLine;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType DataNearBy[20][4]; /* 附近车位坐标数据数组，最多存储20个车位，每个车位4个坐标点
> */
> uint8_t_INF DataNearByNum; /* 附近车位数量 */
> BOOLEAN bResult1;
> uint8_t_INF LocLabel;
> APACoordinateDataCalFloatType LocSlotData[4];
> APACoordinateDataCalFloatType pVPLSlotPts[4];
> APACoordinateDataCalFloatType pTempVPLSlotPts[4];
> APA_ENUM_TYPE OrgIndex[4];
> APA_ENUM_TYPE TempOrgIndex[4];
> APACoordinateDataCalFloatType TempPt;
> APA_ENUM_TYPE FailCause;
> APALineParameterABCType TempLine;
> APALineParameterABCType TempLine1;
> BOOLEAN bCheckIfLadderSlot;
> uint8_t_INF ParkOutMode;
> uint8_t_INF NearByTwoSideSlotNumsByAngled;  /* 附近两侧斜列车位数量 */
> uint8_t_INF NearByTwoSideSlotNumsByLadder;  /* 车位两侧阶梯车位数量 */
> uint8_t_INF NearByTwoSideSlotNumsByLadder1; /* 车位两侧阶梯车位数量 */
> uint8_t_INF NearByTwoSideSlotNumsByPerpen;  /* 车位两侧垂直车位数量 */
> uint8_t_INF NearByOneSideSlotNumsByAngled;  /* 车位单侧斜列车位数量 */
> uint8_t_INF NearByOneSideSlotNumsByLadder;  /*邻近单侧邻近阶梯车位的数量 */
> uint8_t_INF NearByOneSideSlotNumsByLadder1; /* 单侧邻近阶梯车位的数量 */
> uint8_t_INF Data1Index;
> uint8_t_INF Data1IndexByOneSide;
> APACoordinateDataCalFloatType NearByCarPosSlot1;
> APACoordinateDataCalFloatType NearByCarPosSlot1ByOneSide;
> APACoordinateDataCalFloatType Data1[4];
> APACoordinateDataCalFloatType Data2[4];
> uint8_t_INF SlotSide;
> APACoordinateDataCalFloatType getVPLSlotData[SlotPtNum];
> APACoordinateDataCalFloatType getObj1Pt;
> APACoordinateDataCalFloatType getObj2Pt;
> APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj1LineDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj2LineDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CurSlotTopLineAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE CurSlotCloseLineAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1MoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2MoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis;
> APA_DISTANCE_CAL_FLOAT_TYPE SlotOutsideDis;
> APA_DISTANCE_CAL_FLOAT_TYPE SlotInnerDis;
> static BOOLEAN bObjLabelAngledFlag = FALSE;  // 斜列车位框标志位
> BOOLEAN bBuildAngledByNeaybyFlag;  // 可构造斜列或阶梯斜列标志位
> BOOLEAN bCurAngledFlag;  // 自身车位类型为斜列车位类型标志位
> BOOLEAN bBuildIfLadderSlotFlag;  // 阶梯车位类型建立斜列阶梯车位成功与否标志位
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
> #if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
> APACoordinateDataCalFloatType SlotATemp,SlotBTemp,SlotCTemp,SlotDTemp;
> //APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal2;
> APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal3;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
> #endif
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return FALSE;
> }
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> NewOrgAng = 0;
> NearByTwoSideSlotNumsByAngled = 0;
> NearByTwoSideSlotNumsByLadder1 = 0;
> NearByTwoSideSlotNumsByPerpen = 0;
> NearByOneSideSlotNumsByAngled = 0;
> NearByOneSideSlotNumsByLadder1 = 0;
> Data1Index = 0;
> Data1IndexByOneSide = 0;
> NearByCarPosSlot1.x = 0;
> NearByCarPosSlot1.y = 0;
> NearByCarPosSlot1ByOneSide.x = 0;
> NearByCarPosSlot1ByOneSide.y = 0;
> DataNearByNum = 0;
> SlotSide = 0;
> bResult1 = FALSE;
> bResult = FALSE;
> FailCause = 0;
> LocLabel = Obj_Label_Parall_Slot;
> bCheckIfLadderSlot = FALSE;
> bBuildIfLadderSlotFlag = FALSE;
> bBuildAngledByNeaybyFlag = FALSE;
> bCurAngledFlag = FALSE;
> CurObjComInfo = pODInfo->Square.Quadrilaterals[0].ObjInfo;
> for (k = 0; k < 4; k++) {
> LocSlotData[k].x = 0;
> LocSlotData[k].y = 0;
> Data1[k].x = 0;
> Data1[k].y = 0;
> Data2[k].x = 0;
> Data2[k].y = 0;
> }
> for (k = 0; k < SlotPtNum; k++) {
> getVPLSlotData[k].x = 0.0;
> getVPLSlotData[k].y = 0.0;
> }
> getObj1Pt.x = 0.0;
> getObj1Pt.y = 0.0;
> getObj2Pt.x = 0.0;
> getObj2Pt.x = 0.0;
> CarSideToObj1LineDis = 0.0;
> CarSideToObj2LineDis = 0.0;
> CurSlotTopLineAngle = 0.0;
> CurSlotCloseLineAngle = 0.0;
> Obj1MoveDis = 0.0;
> Obj2MoveDis = 0.0;
> CloseLineMoveDis = 0.0;
> SlotOutsideDis = 0.0;
> SlotInnerDis = 0.0;
> #if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
> CarWidth = APAMap_ComCfg.WidthOfCar; //mm
> //SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0]; // 250mm, 0 paralIn;
> SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1]; // 400mm, 1 PerpIn;
> CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper; //mm, 800
> CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper; //mm, 3000
> #if 0
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL){
> SafeDisCal3 = 0;
> } else {
> SafeDisCal3 = 200;
> }  
> SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 + SafeDisCal3;
> SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2 + SafeDisCal3);
> SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2 + SafeDisCal3);
> SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 + SafeDisCal3;
> #else  // 仿真模拟斜列式车位
> #if 0
> SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 + 1000;
> SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2);
> SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000);
> SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2;
> #else
> #if 0  // 仿真模拟车辆左边斜列车位
> SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotATemp.y = (CarLFCal + SafeDisCal2 + 1000) + 1000;
> SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
> SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) + 1000;
> SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotDTemp.y = (CarLFCal + SafeDisCal2) + 1000;
> #else  // 仿真模拟阶梯斜列车位 left
> #if 1
> SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotATemp.y = (CarLFCal + SafeDisCal2) + 1000;
> SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
> SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
> SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotDTemp.y = (CarLFCal + SafeDisCal2) + 1000;
> #else  // right
> SlotATemp.x = ((CarWidth / 2) +  SafeDisCal2);
> SlotATemp.y = (CarLFCal + SafeDisCal2) - 1000;
> SlotBTemp.x = ((CarWidth / 2) +  SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
> #endif
> #endif
> #endif
> #endif
> SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL){
> //pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Perpen_Slot;
> //pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Angled_Slot;
> pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Ladder_Slot;
> } else {
> pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Parall_Slot;
> }  
> pODInfo->Square.Quadrilaterals[0].Points.Point_1.x = SlotATemp.x;
> pODInfo->Square.Quadrilaterals[0].Points.Point_1.y = SlotATemp.y;
> pODInfo->Square.Quadrilaterals[0].Points.Point_2.x = SlotBTemp.x;
> pODInfo->Square.Quadrilaterals[0].Points.Point_2.y = SlotBTemp.y;
> pODInfo->Square.Quadrilaterals[0].Points.Point_3.x = SlotCTemp.x;
> pODInfo->Square.Quadrilaterals[0].Points.Point_3.y = SlotCTemp.y;
> pODInfo->Square.Quadrilaterals[0].Points.Point_4.x = SlotDTemp.x;
> pODInfo->Square.Quadrilaterals[0].Points.Point_4.y = SlotDTemp.y;
> pODInfo->Square.ObjNum = 1;
> #if 1  // 第二个车位
> #if 1  // 仿真当前垂直车位
> SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2;
> SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2);
> SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2);
> SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2;
> #else
> #if 0  // 仿真模拟车辆右边斜列车位
> SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 + 1000 - 1000;
> SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) - 1000;
> SlotDTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
> SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
> #else
> #if 1  // 仿真模拟阶梯斜列车位 right
> SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
> SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
> #else  // 仿真模拟阶梯斜列车位 left
> #if 0
> SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
> SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
> #else
> #if 0  // 仿真模拟阶梯斜列车位单边第二个 left
> SlotATemp.x = -(5 * (CarWidth / 2) + 4 * SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 + 2000;
> SlotBTemp.x = -(5 * (CarWidth / 2) + 4 * SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 2000;
> SlotCTemp.x = -(3 * (CarWidth / 2) +  3 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) + 2000;
> SlotDTemp.x = -(3 * (CarWidth / 2) +  3 * SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 + 2000;
> #else  // 仿真模拟阶梯斜列车位单边第二个 right
> SlotATemp.x = (3 * (CarWidth / 2) + 3 * SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 - 2000;
> SlotBTemp.x = (3 * (CarWidth / 2) + 3 * SafeDisCal2 - 100);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 2000;
> SlotCTemp.x = (5 * (CarWidth / 2) +  4 * SafeDisCal2 - 100);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 2000;
> SlotDTemp.x = (5 * (CarWidth / 2) +  4 * SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 - 2000;
> #endif
> #endif
> #endif
> #endif
> #endif
> SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Perpen_Slot;
> //pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Angled_Slot;
> //pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Ladder_Slot;
> pODInfo->Square.Quadrilaterals[1].Points.Point_1.x = SlotATemp.x;
> pODInfo->Square.Quadrilaterals[1].Points.Point_1.y = SlotATemp.y;
> pODInfo->Square.Quadrilaterals[1].Points.Point_2.x = SlotBTemp.x;
> pODInfo->Square.Quadrilaterals[1].Points.Point_2.y = SlotBTemp.y;
> pODInfo->Square.Quadrilaterals[1].Points.Point_3.x = SlotCTemp.x;
> pODInfo->Square.Quadrilaterals[1].Points.Point_3.y = SlotCTemp.y;
> pODInfo->Square.Quadrilaterals[1].Points.Point_4.x = SlotDTemp.x;
> pODInfo->Square.Quadrilaterals[1].Points.Point_4.y = SlotDTemp.y;
> pODInfo->Square.ObjNum = 2;
> #endif
> #if 1  // 第三个车位
> #if 0  // 仿真模拟阶梯斜列车位 right
> SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
> SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
> SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
> #else
> #if 1  // 仿真模拟车辆右边斜列车位
> SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotATemp.y = CarLFCal + SafeDisCal2 + 1000 - 1000;
> SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
> SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
> SlotCTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
> SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) - 1000;
> SlotDTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
> SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
> #endif
> #endif
> SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> //pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Perpen_Slot;
> pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Angled_Slot;
> //pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Ladder_Slot;
> pODInfo->Square.Quadrilaterals[2].Points.Point_1.x = SlotATemp.x;
> pODInfo->Square.Quadrilaterals[2].Points.Point_1.y = SlotATemp.y;
> pODInfo->Square.Quadrilaterals[2].Points.Point_2.x = SlotBTemp.x;
> pODInfo->Square.Quadrilaterals[2].Points.Point_2.y = SlotBTemp.y;
> pODInfo->Square.Quadrilaterals[2].Points.Point_3.x = SlotCTemp.x;
> pODInfo->Square.Quadrilaterals[2].Points.Point_3.y = SlotCTemp.y;
> pODInfo->Square.Quadrilaterals[2].Points.Point_4.x = SlotDTemp.x;
> pODInfo->Square.Quadrilaterals[2].Points.Point_4.y = SlotDTemp.y;
> pODInfo->Square.ObjNum = 3;
> #endif
> #endif
> // First
> // Search,先预遍历一遍，从前向后搜索，找出当前车辆周围是否存在斜列或阶梯斜列车位
> bSearch = TRUE;
> i = 0;
> j = 0;
> while (bSearch) {
> if (j == 0) {
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Angled_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Parall_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Ladder_Slot)) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Square.ObjNum) {
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutSlotInfoFromTotalMap==First==SlotPt:"
> "0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
> "CurLabel(%d)",
> Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x,
> Data[2].y, Data[3].x, Data[3].y, CurObjComInfo.Label);
> TLOG_INFO << log_string;
> }
> i++;
> } else {
> j++;
> i = 0;
> }
> }
> if (j == 1) {
> bSearch = FALSE;
> }
> if (bSearch == TRUE) {
> LocLabel = CurObjComInfo.Label;
> bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
> &LocLabel, &Data[0], &NearByTwoSideSlotNumsByAngled,
> &NearByTwoSideSlotNumsByLadder1, &NearByTwoSideSlotNumsByPerpen,
> &Data1Index, &NearByCarPosSlot1, &Data1[0]);
> if (FALSE == bResult1) {
> bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
> &LocLabel, &Data[0], &NearByOneSideSlotNumsByAngled,
> &NearByOneSideSlotNumsByLadder1, &Data1IndexByOneSide,
> &NearByCarPosSlot1ByOneSide, &Data2[0], &SlotSide);
> }
> if ((TRUE == bResult1) && (NearByTwoSideSlotNumsByPerpen < 2)) {
> bBuildAngledByNeaybyFlag = TRUE;
> }
> bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&CurCarPos.Coordinate,
> &Data[0], 4);
> if ((CurObjComInfo.Label == Obj_Label_Angled_Slot) &&
> (TRUE == bResult1)) {
> bCurAngledFlag = TRUE;
> }
> }
> }
> // Second Search,开始正式遍历，从前向后搜索，找出当前车位，没有则造车位
> NearByTwoSideSlotNumsByAngled = 0;
> NearByTwoSideSlotNumsByLadder = 0;
> NearByTwoSideSlotNumsByPerpen = 0;
> NearByOneSideSlotNumsByAngled = 0;
> NearByOneSideSlotNumsByLadder = 0;
> Data1Index = 0;
> Data1IndexByOneSide = 0;
> NearByCarPosSlot1.x = 0;
> NearByCarPosSlot1.y = 0;
> NearByCarPosSlot1ByOneSide.x = 0;
> NearByCarPosSlot1ByOneSide.y = 0;
> SlotSide = 0;
> for (k = 0; k < 4; k++) {
> Data1[k].x = 0;
> Data1[k].y = 0;
> Data2[k].x = 0;
> Data2[k].y = 0;
> }
> bSearch = TRUE;
> bResult1 = FALSE;
> i = 0;
> j = 0;
> while (bSearch) {
> if (j == 0) {
> while (i < pODInfo->Square.ObjNum) {
> CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
> if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Angled_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Parall_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Ladder_Slot)) {
> break;
> }
> i++;
> }
> if (i < pODInfo->Square.ObjNum) {  // 获取车位信息 Square.Quadrilaterals
> Data[0].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.x;
> Data[0].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_1.y;
> Data[1].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.x;
> Data[1].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_2.y;
> Data[2].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.x;
> Data[2].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_3.y;
> Data[3].x =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.x;
> Data[3].y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
> .Points.Point_4.y;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutSlotInfoFromTotalMap==Second==SlotPt:"
> "0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
> "CurLabel(%d)",
> Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x,
> Data[2].y, Data[3].x, Data[3].y, CurObjComInfo.Label);
> TLOG_INFO << log_string;
> }
> i++;
> } else {  // 搜索结束
> j++;
> i = 0;
> }
> }
> if (j == 1) {
> bSearch = FALSE;
> }
> if (bSearch == TRUE) {
> // 判断是否满足条件：没有真实的融合OD斜列车位类型车位框存在。则允许由周围车位框构造当前车位框。
> if (FALSE == bCurAngledFlag) {
> bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
> &CurObjComInfo.Label, &Data[0], &NearByTwoSideSlotNumsByAngled,
> &NearByTwoSideSlotNumsByLadder, &NearByTwoSideSlotNumsByPerpen,
> &Data1Index, &NearByCarPosSlot1, &Data1[0]);
> if (FALSE == bResult1) {
> bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
> &CurObjComInfo.Label, &Data[0], &NearByOneSideSlotNumsByAngled,
> &NearByOneSideSlotNumsByLadder, &Data1IndexByOneSide,
> &NearByCarPosSlot1ByOneSide, &Data2[0], &SlotSide);
> }
> if (TRUE == bResult1) {
> bCheckIfLadderSlot = FALSE;
> bResult = FALSE;
> }
> }
> if (FALSE == bResult1) {
> bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(
> &CurCarPos.Coordinate, &Data[0], 4);
> // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
> if (((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
> (CurObjComInfo.Label == Obj_Label_Parall_Slot)) &&
> (TRUE == bResult1)) {
> if ((NearByTwoSideSlotNumsByLadder1 > 0) ||
> (NearByOneSideSlotNumsByLadder1 >
> 0)) { /* * 判断附近一侧是否有可用车位 *
> 条件：检查NearByOneSideSlotNumsByLadder1变量是否大于0 *
> 如果大于0，表示存在可用车位，条件成立 */
> CurObjComInfo.Label = Obj_Label_Ladder_Slot;
> }
> }
> // 代表当前车位误识别成垂直，但根据周围车位可以构造出当前斜列车位类型的情况，把返回值置成FALSE，继续搜索附近斜列车位来构造斜列车位框
> if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) &&
> (TRUE == bResult1)) {
> if (TRUE == bBuildAngledByNeaybyFlag) {
> bResult1 = FALSE;
> }
> }
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==1111=======bResult(%d)==bResult1(%d)=="
> "bBuildAngledByNeaybyFlag(%d)==Label(%d)",
> bResult, bResult1, bBuildAngledByNeaybyFlag,
> CurObjComInfo.Label);
> TLOG_INFO << log_string;
> }
> #endif
> }
> if (bResult1 == TRUE) {  // 车辆位置在车位内
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==2222=======bResult(%d)==bResult1(%d)=="
> "DataNearByNum(%d)==Label(%d)",
> bResult, bResult1, DataNearByNum, CurObjComInfo.Label);
> TLOG_INFO << log_string;
> }
> #endif
> if (bResult == FALSE) {
> bResult = TRUE;  // 为什么搜索结束后 这里要置 True?
> LocLabel = CurObjComInfo.Label;
> for (k = 0; k < 4; k++) {
> LocSlotData[k] = Data[k];
> }
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (LocLabel == Obj_Label_Ladder_Slot) {
> bObjLabelAngledFlag = TRUE;
> bObjLabelLadderFlag = TRUE;
> } else if (LocLabel == Obj_Label_Angled_Slot) {
> bObjLabelAngledFlag = TRUE;
> bObjLabelLadderFlag = FALSE;
> } else {
> bObjLabelLadderFlag = FALSE;
> bObjLabelAngledFlag = FALSE;
> }
> }
> #ifdef APA_MAP_PARKOUT_LADDER_SLOT
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (LocLabel == Obj_Label_Ladder_Slot)) {
> bCheckIfLadderSlot = TRUE;
> }
> #endif
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==3333=======bResult(%d)==bResult1(%d)=="
> "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
> bResult, bResult1, DataNearByNum, bCheckIfLadderSlot);
> TLOG_INFO << log_string;
> }
> #endif
> }
> } else {
> if (DataNearByNum >= 20) {
> break;
> }
> for (k = 0; k < 4; k++) {
> DataNearBy[DataNearByNum][k] = Data[k];
> }
> DataNearByNum++;
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==4444=======bResult(%d)==bResult1(%d)=="
> "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
> bResult, bResult1, DataNearByNum, bCheckIfLadderSlot);
> TLOG_INFO << log_string;
> }
> #endif
> }
> }
> if (bResult == TRUE) {
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==5555=======bResult(%d)==bResult1(%d)==DataNearByNum(%"
> "d)==bCheckIfLadderSlot(%d)",
> bResult, bResult1, DataNearByNum, bCheckIfLadderSlot);
> TLOG_INFO << log_string;
> }
> #endif
> if (bCheckIfLadderSlot == FALSE) {
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==6666=======bResult(%d)==bResult1(%d)=="
> "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
> bResult, bResult1, DataNearByNum, bCheckIfLadderSlot);
> TLOG_INFO << log_string;
> }
> #endif
> break;
> } else {
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==Test==7777=======bResult(%d)==bResult1(%d)=="
> "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
> bResult, bResult1, DataNearByNum, bCheckIfLadderSlot);
> TLOG_INFO << log_string;
> }
> #endif
> bResult1 = FALSE;
> // continue search;
> }
> }
> }
> if (bResult == TRUE) {
> if ((bObjLabelAngledFlag == FALSE) &&
> ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT))) {
> bResult = FALSE;  // 车头、车尾直出就不需要继续搜索了
> }
> if (bResult == TRUE) {
> bResult = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &LocSlotData[0], CurCarPos.Coordinate, CurCarPos.CarAng);
> if (bResult == FALSE) {
> FailCause = 3;
> }
> } else {
> FailCause = 2;
> }
> } else {
> FailCause = 1;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutGetLocSlotFromTotalMap==Result(%d)==Cause(%d)"
> "=LocSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),"
> "LocLabel(%d),LadderSlot(%d),SlotNumNearBy(%d)",
> bResult, FailCause, LocSlotData[0].x, LocSlotData[0].y,
> LocSlotData[1].x, LocSlotData[1].y, LocSlotData[2].x,
> LocSlotData[2].y, LocSlotData[3].x, LocSlotData[3].y, LocLabel,
> bCheckIfLadderSlot, DataNearByNum);
> TLOG_INFO << log_string;
> }
> if ((bResult == TRUE) && (bCheckIfLadderSlot == TRUE)) {
> if (DataNearByNum > 0) {
> APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(  //  根据泊车模式重新排序VPL车位点
> &LocSlotData[0], &pVPLSlotPts[0], &OrgIndex[0]);
> AlgCom_GetAngByTwoPts(pVPLSlotPts[1], pVPLSlotPts[0],
> &NewOrgAng);  //  通过两点计算角度，获取新的原始角度
> AlgCom_GetAngByTwoPts(pVPLSlotPts[3], pVPLSlotPts[0], &Obj2Ang);
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutLocVPLPts===pVPLSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),"
> "2(%.2f,%.2f),3(%.2f,%.2f)==OrgIndex(%d,%d,%d,%d)"
> "==CloseLineAng(%.2f)==Obj2Ang(%.2f)==NearBySlotNum(%d)",
> pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
> pVPLSlotPts[1].y, pVPLSlotPts[2].x, pVPLSlotPts[2].y,
> pVPLSlotPts[3].x, pVPLSlotPts[3].y, OrgIndex[0], OrgIndex[1],
> OrgIndex[2], OrgIndex[3], NewOrgAng * 180.0 / PI,
> Obj2Ang * 180.0 / PI, DataNearByNum);
> TLOG_INFO << log_string;
> }
> for (i = 0; i < DataNearByNum; i++) {
> FailCause = 0;
> if (TRUE == AlgCom_LineParABCbyTwoPoints(DataNearBy[i][0],
> DataNearBy[i][2], &TempLine)) {
> if (TRUE == AlgCom_LineParABCbyTwoPoints(
> DataNearBy[i][1], DataNearBy[i][3], &TempLine1)) {
> if (1 ==
> AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt)) {
> if (TRUE == AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
> &DataNearBy[i][0], TempPt, CurCarPos.CarAng)) {
> APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(
> &DataNearBy[i][0], &pTempVPLSlotPts[0], &TempOrgIndex[0]);
> if (TRUE == APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot(
> &pVPLSlotPts[0], &pVPLSlotPts[1], &NewOrgAng,
> Obj2Ang, &pTempVPLSlotPts[0],
> bSlotDataAtRigthSide, &FailCause)) {
> for (k = 0; k < 4; k++) {
> LocSlotData[OrgIndex[k]] = pVPLSlotPts[k];
> }
> bBuildIfLadderSlotFlag = TRUE;
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutCheckIfTargetSlotIsLadderSlotSuccess!"
> "=NearBySlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%."
> "2f),3(%.2f,%.2f))"
> "=OrdVPLSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%."
> "2f),3(%.2f,%.2f))"
> "==NewObj2(%.2f,%.2f)==NewObj1(%.2f,%.2f)=="
> "NewOrgAng(%.2f)",
> DataNearBy[i][0].x, DataNearBy[i][0].y,
> DataNearBy[i][1].x, DataNearBy[i][1].y,
> DataNearBy[i][2].x, DataNearBy[i][2].y,
> DataNearBy[i][3].x, DataNearBy[i][3].y,
> pTempVPLSlotPts[0].x, pTempVPLSlotPts[0].y,
> pTempVPLSlotPts[1].x, pTempVPLSlotPts[1].y,
> pTempVPLSlotPts[2].x, pTempVPLSlotPts[2].y,
> pTempVPLSlotPts[3].x, pTempVPLSlotPts[3].y,
> pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
> pVPLSlotPts[1].y, NewOrgAng);
> TLOG_INFO << log_string;
> break;
> }
> } else {
> FailCause = 0x40;
> }
> } else {
> FailCause = 0x30;
> }
> } else {
> FailCause = 0x20;
> }
> } else {
> FailCause = 0x10;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==ParkOutCheckIfTargetSlotIsLadderSlotFailCase(%d)==="
> "NearBySlotPt:0(%.2f,%.2f),1(%.2f,%.2f)"
> "2(%.2f,%.2f),3(%.2f,%.2f)",
> FailCause, DataNearBy[i][0].x, DataNearBy[i][0].y,
> DataNearBy[i][1].x, DataNearBy[i][1].y, DataNearBy[i][2].x,
> DataNearBy[i][2].y, DataNearBy[i][3].x, DataNearBy[i][3].y);
> TLOG_INFO << log_string;
> }
> }
> }
> if (bBuildIfLadderSlotFlag == FALSE) {
> bObjLabelLadderFlag = FALSE;
> bObjLabelAngledFlag = FALSE;
> }
> }
> if (bResult == TRUE) {
> bResult = APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(
> &LocSlotData[0], &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &NewOrgAng,
> &Obj2Dis, &Obj1Dis, &CarOffsetX, &MinSlotDpth, &VPLSlotDpth);
> if (bResult == TRUE) {
> for (i = 0; i < 4; i++) {
> getVPLSlotData[i] = LocSlotData[i];
> }
> getObj1Pt = Obj1Pt;
> getObj2Pt = Obj2Pt;
> CurSlotTopLineAngle = Obj2Ang;
> CurSlotCloseLineAngle = NewOrgAng;
> CarSideToObj1LineDis = Obj1Dis;
> CarSideToObj2LineDis = Obj2Dis;
> #ifdef SUPPORT_ELECTRONIC_FENCE_MAP
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FirstParkingOutElectrFenceMapBulid");
> TLOG_INFO << log_string;
> }
> APAMap_ParkingOutElectrFenceMapBulid(
> &getVPLSlotData[0], &getObj1Pt, &getObj2Pt, &CarSideToObj1LineDis,
> &CarSideToObj2LineDis, &CurSlotTopLineAngle, &CurSlotCloseLineAngle,
> &Obj1MoveDis, &Obj2MoveDis, &CloseLineMoveDis, &SlotOutsideDis,
> &SlotInnerDis);
> MaxObj2Dis = Obj2MoveDis;
> MaxObj1Dis = Obj1MoveDis;
> CloseLineOffsetX = CloseLineMoveDis;
> MaxSubLaneX = SlotOutsideDis;
> // MinSlotDpth = SlotInnerDis;
> #else
> MaxObj2Dis = 1000;  // 500;
> MaxObj1Dis = 1000;  // 500;
> CloseLineOffsetX = 0;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> MaxSubLaneX = 5000;
> } else {
> MaxSubLaneX = 7000;
> }
> #endif
> MoveObj2Dis = MaxObj2Dis - Obj2Dis;  //>0 make slot big;
> MoveObj1Dis = MaxObj1Dis - Obj1Dis;  //>0 need makeslot big;
> TempCarPos.Coordinate = Obj2Pt;
> TempCarPos.CarAng = Obj2Ang;
> TopLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempCarPos.Coordinate = Obj1Pt;
> TempCarPos.CarAng = Obj1Ang;
> BotLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempCarPos.Coordinate = Obj2Pt;
> TempCarPos.CarAng = NewOrgAng;
> CloseLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> // move obj2 and obj1;
> if (FALSE == bSlotDataAtRigthSide) {
> MoveObj2Dis = -MoveObj2Dis;
> MoveObj1Dis = -MoveObj1Dis;
> CloseLineOffsetX = -CloseLineOffsetX;
> }
> TopLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &TopLine, Obj2Ang, MoveObj2Dis);
> BotLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &BotLine, Obj1Ang, -MoveObj1Dis);
> CloseLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
> &CloseLine, NewOrgAng, -CloseLineOffsetX);
> AlgCom_CrossPointOfTwoLines(&TopLine, &CloseLine, &Obj2Pt);
> AlgCom_CrossPointOfTwoLines(&BotLine, &CloseLine, &Obj1Pt);
> // construct par;
> *pObj2Pt = Obj2Pt;
> *pObj1Pt = Obj1Pt;
> *pObj2Ang = Obj2Ang;
> *pObj1Ang = Obj1Ang;
> *pNewOrgAng = NewOrgAng;
> *pMaxSubLane = MaxSubLaneX;
> *pMaxSlotInnerX = MinSlotDpth;
> }
> }
> // 针对水平泊出，如果车身角度和锚点坐标系的y轴方向夹角大于5度(0.0873)，则直接采用传统水平泊出构造虚拟车位框
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (TRUE == bResult) &&
> (MATH_FABS(CurCarPos.CarAng - NewOrgAng) > M_PI / 36)) {
> bResult = FALSE;
> }
> if (FALSE == bResult) {
> bCarryOutSlot = FALSE;
> } else {
> bCarryOutSlot = TRUE;
> }
> // 针对垂直和斜列泊出，如果Obj2角度和锚点角度夹角基本为90度（误差正负10度），则判定为垂直车位类型
> TempAng = Obj2Ang;
> AlgCom_AngNormalized(&TempAng);
> TempAng1 = NewOrgAng;
> AlgCom_AngNormalized(&TempAng1);
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (TRUE == bCarryOutSlot) &&
> (MATH_FABS(TempAng - TempAng1) > (M_PI / 2 - M_PI / 18)) &&
> (MATH_FABS(TempAng - TempAng1) < (M_PI / 2 + M_PI / 18))) {
> bObjLabelAngledFlag = FALSE;
> }
> // 针对垂直车位泊出，如果Obj2角度和锚点角度夹角大于93度（1.63）或小于87度（1.51），则直接采用传统垂直泊出构造虚拟车位框
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (TRUE == bCarryOutSlot) && (FALSE == bObjLabelAngledFlag) &&
> ((MATH_FABS(TempAng - TempAng1) > 1.63) ||
> (MATH_FABS(TempAng - TempAng1) < 1.51))) {
> bCarryOutSlot = FALSE;
> bResult = FALSE;
> }
> if ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (TRUE == bCarryOutSlot) && (TRUE == bObjLabelAngledFlag)) {
> bLabelAngledFlag = TRUE;
> }
> #ifdef SUPPORT_PARKING_OUT_DEBUG
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap=="
> "bResult(%d)==bResult1(%d)==bCarryOutSlot(%d)==bObjLabelAngledFlag(%d)="
> "=bLabelAngledFlag(%d)==bObjLabelLadderFlag(%d)",
> bResult, bResult1, bCarryOutSlot, bObjLabelAngledFlag, bLabelAngledFlag,
> bObjLabelLadderFlag);
> TLOG_INFO << log_string;
> }
> #endif
> return bResult;
> }
> #endif
> BOOLEAN APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo() {
> APA_ENUM_TYPE SlotType;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType OrgPt;
> APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
> APACoordinateDataCalFloatType TempPt1, TempPt2, TempPt3;
> APA_DISTANCE_TYPE SlotLength;
> APA_DISTANCE_TYPE SlotDepth;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
> APALineParameterABCType TempLine;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
> APA_ENUM_TYPE ParkSide;
> BOOLEAN bResult;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSubLaneX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotInnerX;
> BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位
> bSeizeEndCarPosFlag = FALSE;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> ParkOutEightMode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
> // zqf-GetParkOutEightMode
> ParkSide = APAMap_GInputData.ParkReqPar.parkside;
> ParkOutEightMode = AlgCom_GetParkOutEightMode(ParkOutMode, ParkSide);
> // zqf-SetParkOutObjPt
> bResult = APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(
> &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &OrgAng, &MaxSubLaneX,
> &MaxSlotInnerX);
> if (FALSE == bResult) {
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==Carry out 1.APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()==");
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==Carry out 2.APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()==\n"
> "==Obj2Pt(%.2f,%.2f)==Obj1Pt(%.2f,%.2f)==Obj2Ang(%.2f)==Obj1Ang(%.2f)=="
> "OrgAng(%.2f)==MaxSubLaneX(%.2f)==MaxSlotInnerX(%.2f)",
> Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, Obj2Ang, Obj1Ang, OrgAng,
> MaxSubLaneX, MaxSlotInnerX);
> TLOG_INFO << log_string;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);
> } else {
> TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);
> }
> fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);
> fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);
> SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
> if (((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - 150.0)) ||
> ((ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
> (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - 150.0))) {
> APAMAP_Setfailcause(100);
> return FALSE;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> SlotType = 0;
> SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
> } else if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> SlotType = 1;
> SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
> } else {
> SlotType = 1;
> SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
> }
> OrgPt = Obj2Pt;
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
> } else {
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.HalfWidthOfCar);
> }
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt2, 0, OrgAng, OrgPt);
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt3, 0, OrgAng, OrgPt);
> } else {
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
> } else {
> TempPt1.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> }
> TempPt1.x = APAMap_ComCfg.HalfWidthOfCar;
> TempPt2 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt2, 0, OrgAng, OrgPt);
> TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.HalfWidthOfCar);
> TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt3, 0, OrgAng, OrgPt);
> }
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt2.x = -TempPt2.x;
> TempPt3.x = -TempPt3.x;
> }
> if (TempPt2.x > TempPt3.x) {
> MaxSlotPtX = TempPt2.x;
> } else {
> MaxSlotPtX = TempPt3.x;
> }
> if (MaxSlotInnerX > MaxSlotPtX) {
> MaxSlotPtX = MaxSlotInnerX;
> }
> APAMap_GInfo.SlotPar.slotCarEndPosXBackUp = MaxSlotPtX;
> APAMap_GInfo.SlotPar.SlotDepth = SlotDepth;
> APAMap_GInfo.SlotPar.SlotLen = SlotLength;
> #ifdef SUPPORT_PARKING_OUT_UWB
> if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> } else {
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> }
> #else
> TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
> ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
> #endif
> if (TempPt3.x == 0xff) {
> APAMAP_Setfailcause(101);
> return FALSE;
> }
> APAMap_GInfo.SlotPar.EndPos.Coordinate = TempPt3;
> APAMap_GInfo.SlotPar.EndPosLine =
> AlgCom_LineParABCByCurrentCarPosition(&APAMap_GInfo.SlotPar.EndPos, 0);
> APAMap_GInfo.bDataMirrored = FALSE;
> APAMap_GInfo.bCordSysReSet = FALSE;
> APAMap_GInfo.SlotPar.SlotType = SlotType;
> APAMap_GInfo.SlotPar.bObj2Exist = TRUE;
> APAMap_GInfo.SlotPar.bObj1Exist = TRUE;
> APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
> APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
> APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
> APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;
> APAMap_GInfo.NewCordSysOPt = OrgPt;
> APAMap_GInfo.NewCordSysAng = OrgAng;
> APAMap_GInfo.SlotPar.Obj2Ang = Obj2Ang;
> APAMap_GInfo.SlotPar.Obj1Ang = Obj1Ang;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FirstBuildMapObjAndEndCarPos==Obj2Pt(%.2f,%.2f,%.2f)==Obj1Pt(%."
> "2f,%.2f,%.2f)==NewCordSysOPt(%.2f,%.2f,%.2f)"
> "==SlotBordPt[0](%.2f,%.2f)====SlotBordPt[1](%.2f,%.2f)==EndPos(%."
> "2f,%.2f,%.2f)==SlotLen(%d)==SlotDepth(%d)==APAstate(%d)=="
> "APARunningstate(%d)",
> APAMap_GInfo.SlotPar.Obj2Pt.x, APAMap_GInfo.SlotPar.Obj2Pt.y,
> APAMap_GInfo.SlotPar.Obj2Ang, APAMap_GInfo.SlotPar.Obj1Pt.x,
> APAMap_GInfo.SlotPar.Obj1Pt.y, APAMap_GInfo.SlotPar.Obj1Ang,
> APAMap_GInfo.NewCordSysOPt.x, APAMap_GInfo.NewCordSysOPt.y,
> APAMap_GInfo.NewCordSysAng, APAMap_GInfo.SlotPar.SlotBordPt[0].x,
> APAMap_GInfo.SlotPar.SlotBordPt[0].y,
> APAMap_GInfo.SlotPar.SlotBordPt[1].x,
> APAMap_GInfo.SlotPar.SlotBordPt[1].y,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
> APAMap_GInfo.SlotPar.EndPos.Coordinate.y,
> APAMap_GInfo.SlotPar.EndPos.CarAng, APAMap_GInfo.SlotPar.SlotLen,
> APAMap_GInfo.SlotPar.SlotDepth,
> APAMap_GInputData.ParkReqPar.APAstate,
> APAMap_GInputData.ParkReqPar.APARunningstate);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> #if 1
> void APAMap_ParkingOutBkDataBfSDGFusInit(void) {
> APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Invalid;
> APAMap_BkDataBfSDGFus.MapMainSlotBord.PtNum = 0;
> return;
> }
> void APAMap_ParkingOutBkSDGOutPutDataInit(void) {
> APAMap_BkSDGOutPutData.Obj2PtNum = 0;
> APAMap_BkSDGOutPutData.Obj1PtNum = 0;
> return;
> }
> void APAMap_ParkingOutSiftAndSeqSDGPts(
> APACarCoordinateDataCalFloatType* pCurCarPos,
> APACoordinateDataCalFloatType* pLeftSeg,
> APACoordinateDataCalFloatType* pRightSeg, uint8_INF* pLeftSegNum,
> uint8_INF* pRightSegNum) {
> #ifdef APAMAP_PARKOUT_USE_SDG_OBJS
> APACoordinateDataCalFloatType NewPto;
> APA_DISTANCE_CAL_FLOAT_TYPE NewAngle;
> APACoordinateDataCalFloatType TempSeg[2];
> APACoordinateDataCalFloatType TempPt;
> APA_DISTANCE_TYPE i, j, k;
> BOOLEAN bValid;
> APA_ENUM_TYPE ObjLocIndex;
> APA_ENUM_TYPE CurLoc;
> APA_DISTANCE_CAL_FLOAT_TYPE MinY;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxY;
> APACoordinateDataCalFloatType* pTarSeg;
> uint8_INF* pTarSegNum;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACarCoordinateDataCalFloatType TempCarPos;
> APACoordinateDataCalFloatType NewSeg[2];
> APACoordinateDataCalFloatType AddSeg[2][2];
> BOOLEAN bAddSeg[2];
> APA_DISTANCE_CAL_FLOAT_TYPE VirSegX[2];
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> APA_ENUM_TYPE AddPtNum;
> st_MapUSS* pSDGInfo;
> *pLeftSegNum = 0;
> *pRightSegNum = 0;
> pSDGInfo = &APAMap_GInputData.TotalMapInfo.mapData.USSObjInfo;
> #ifdef APAMAP_PARKOUT_PCDEMO_USE_DEFAULT_SDG_OBJS
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType Pto;
> BOOLEAN bSlotDataAtRigthSide;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> pSDGInfo->ObjNum = 6;
> pSDGInfo->Obj[0].Pt[0].x = 0;
> pSDGInfo->Obj[0].Pt[0].y = 0;
> pSDGInfo->Obj[0].Pt[1].x = 300;
> pSDGInfo->Obj[0].Pt[1].y = -300;
> pSDGInfo->Obj[1].Pt[0].x = 3500;
> pSDGInfo->Obj[1].Pt[0].y = -300;
> pSDGInfo->Obj[1].Pt[1].x = 2500;
> pSDGInfo->Obj[1].Pt[1].y = -400;
> pSDGInfo->Obj[2].Pt[0].x = 3500;
> pSDGInfo->Obj[2].Pt[0].y = -3000;
> pSDGInfo->Obj[2].Pt[1].x = 1500;
> pSDGInfo->Obj[2].Pt[1].y = -2800;
> pSDGInfo->Obj[3].Pt[0].x = 0;
> pSDGInfo->Obj[3].Pt[0].y = -3000;
> pSDGInfo->Obj[3].Pt[1].x = 500;
> pSDGInfo->Obj[3].Pt[1].y = -3200;
> pSDGInfo->Obj[4].Pt[0].x = 4000;
> pSDGInfo->Obj[4].Pt[0].y = -2900;
> pSDGInfo->Obj[4].Pt[1].x = 4500;
> pSDGInfo->Obj[4].Pt[1].y = -2700;
> pSDGInfo->Obj[5].Pt[0].x = 4500;
> pSDGInfo->Obj[5].Pt[0].y = 200;
> pSDGInfo->Obj[5].Pt[1].x = 4000;
> pSDGInfo->Obj[5].Pt[1].y = -400;
> if (bSlotDataAtRigthSide == FALSE) {
> for (i = 0; i < pSDGInfo->ObjNum; i++) {
> pSDGInfo->Obj[i].Pt[0].x = -pSDGInfo->Obj[i].Pt[0].x;
> pSDGInfo->Obj[i].Pt[1].x = -pSDGInfo->Obj[i].Pt[1].x;
> }
> }
> for (i = 0; i < pSDGInfo->ObjNum; i++) {
> TempPt.x = pSDGInfo->Obj[i].Pt[0].x;
> TempPt.y = pSDGInfo->Obj[i].Pt[0].y;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> pSDGInfo->Obj[i].Pt[0].x = TempPt.x;
> pSDGInfo->Obj[i].Pt[0].y = TempPt.y;
> TempPt.x = pSDGInfo->Obj[i].Pt[1].x;
> TempPt.y = pSDGInfo->Obj[i].Pt[1].y;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> pSDGInfo->Obj[i].Pt[1].x = TempPt.x;
> pSDGInfo->Obj[i].Pt[1].y = TempPt.y;
> }
> #else
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return;
> }
> #endif
> if ((pSDGInfo->ObjNum <= 0) || (pSDGInfo->ObjNum > MAP_US_OBJ_EXTR_MAX_NUM)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==NoSDGPts:(%d)",
> pSDGInfo->ObjNum);
> TLOG_INFO << log_string;
> return;
> }
> if (pSDGInfo->ObjNum > 0) {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==OrgSDGPts==(%d):0[(%d,%d),(%d,%d)],1[(%d,%d),(%d,%d)],2[(%d,%d)"
> ",(%d,%d)],"
> "3[(%d,%d),(%d,%d)],4[(%d,%d),(%d,%d)],5[(%d,%d),(%d,%d)],6[(%d,%"
> "d),(%d,%d)],7[(%d,%d),(%d,%d)],"
> "8[(%d,%d),(%d,%d)],9[(%d,%d),(%d,%d)],10[(%d,%d),(%d,%d)],11[(%d,"
> "%d),(%d,%d)],12[(%d,%d),(%d,%d)]",
> pSDGInfo->ObjNum, pSDGInfo->Obj[0].Pt[0].x,
> pSDGInfo->Obj[0].Pt[0].y, pSDGInfo->Obj[0].Pt[1].x,
> pSDGInfo->Obj[0].Pt[1].y, pSDGInfo->Obj[1].Pt[0].x,
> pSDGInfo->Obj[1].Pt[0].y, pSDGInfo->Obj[1].Pt[1].x,
> pSDGInfo->Obj[1].Pt[1].y, pSDGInfo->Obj[2].Pt[0].x,
> pSDGInfo->Obj[2].Pt[0].y, pSDGInfo->Obj[2].Pt[1].x,
> pSDGInfo->Obj[2].Pt[1].y, pSDGInfo->Obj[3].Pt[0].x,
> pSDGInfo->Obj[3].Pt[0].y, pSDGInfo->Obj[3].Pt[1].x,
> pSDGInfo->Obj[3].Pt[1].y, pSDGInfo->Obj[4].Pt[0].x,
> pSDGInfo->Obj[4].Pt[0].y, pSDGInfo->Obj[4].Pt[1].x,
> pSDGInfo->Obj[4].Pt[1].y, pSDGInfo->Obj[5].Pt[0].x,
> pSDGInfo->Obj[5].Pt[0].y, pSDGInfo->Obj[5].Pt[1].x,
> pSDGInfo->Obj[5].Pt[1].y, pSDGInfo->Obj[6].Pt[0].x,
> pSDGInfo->Obj[6].Pt[0].y, pSDGInfo->Obj[6].Pt[1].x,
> pSDGInfo->Obj[6].Pt[1].y, pSDGInfo->Obj[7].Pt[0].x,
> pSDGInfo->Obj[7].Pt[0].y, pSDGInfo->Obj[7].Pt[1].x,
> pSDGInfo->Obj[7].Pt[1].y, pSDGInfo->Obj[8].Pt[0].x,
> pSDGInfo->Obj[8].Pt[0].y, pSDGInfo->Obj[8].Pt[1].x,
> pSDGInfo->Obj[8].Pt[1].y, pSDGInfo->Obj[9].Pt[0].x,
> pSDGInfo->Obj[9].Pt[0].y, pSDGInfo->Obj[9].Pt[1].x,
> pSDGInfo->Obj[9].Pt[1].y, pSDGInfo->Obj[10].Pt[0].x,
> pSDGInfo->Obj[10].Pt[0].y, pSDGInfo->Obj[10].Pt[1].x,
> pSDGInfo->Obj[10].Pt[1].y, pSDGInfo->Obj[11].Pt[0].x,
> pSDGInfo->Obj[11].Pt[0].y, pSDGInfo->Obj[11].Pt[1].x,
> pSDGInfo->Obj[11].Pt[1].y, pSDGInfo->Obj[12].Pt[0].x,
> pSDGInfo->Obj[12].Pt[0].y, pSDGInfo->Obj[12].Pt[1].x,
> pSDGInfo->Obj[12].Pt[1].y);
> TLOG_INFO << log_string;
> }
> MaxY = APAMap_ComCfg.LenBetweenRAxisAndFBumper + 1500;
> MinY = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + 1000);
> NewPto = pCurCarPos->Coordinate;
> NewAngle = pCurCarPos->CarAng;
> TempCarPos.Coordinate.x = 0;
> TempCarPos.Coordinate.y = 0;
> TempCarPos.CarAng = 0;
> VirSegX[0] = -(APAMap_ComCfg.HalfWidthOfCar + 500);
> VirSegX[1] = (APAMap_ComCfg.HalfWidthOfCar + 500);
> APAMap_GetCarRectArea(100, 100, 100, 100, TempCarPos, &pRectPt[0],
> &pRectLine[0]);
> for (i = 0; i < pSDGInfo->ObjNum; i++) {
> bValid = FALSE;
> ObjLocIndex = -1;
> for (j = 0; j < 2; j++) {
> TempSeg[j].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSDGInfo->Obj[i].Pt[j].x;
> TempSeg[j].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSDGInfo->Obj[i].Pt[j].y;
> TempSeg[j] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempSeg[j], 0, NewAngle, NewPto);
> if ((TempSeg[j].y > MinY) && (TempSeg[j].y < MaxY)) {
> bValid = TRUE;
> }
> if (TempSeg[j].x < 0) {
> CurLoc = 0;  // Left;
> } else {
> CurLoc = 1;  // Right;
> }
> if ((ObjLocIndex == -1) || (ObjLocIndex == CurLoc)) {
> ObjLocIndex = CurLoc;
> } else {
> if (TempSeg[j].y > 0) {
> ObjLocIndex = 2;  // Front;
> } else {
> ObjLocIndex = 3;  // Back;
> }
> }
> }
> if (bValid == TRUE) {
> if (TempSeg[1].y > TempSeg[0].y) {
> TempPt = TempSeg[0];
> TempSeg[0] = TempSeg[1];
> TempSeg[1] = TempPt;
> }
> if (ObjLocIndex == 0) {
> pTarSeg = &pLeftSeg[0];
> pTarSegNum = pLeftSegNum;
> } else if (ObjLocIndex == 1) {
> pTarSeg = &pRightSeg[0];
> pTarSegNum = pRightSegNum;
> } else {
> pTarSeg = NULL;
> pTarSegNum = NULL;
> }
> if (pTarSeg != NULL) {
> j = 0;
> while (j < *pTarSegNum) {
> if (TempSeg[0].y > pTarSeg[j].y) {
> break;
> }
> j += 2;
> }
> k = *pTarSegNum - 1;
> AddPtNum = 2;
> if ((*pTarSegNum + AddPtNum) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
> // buff is not big enough
> return;
> }
> while (k >= j) {
> pTarSeg[k + AddPtNum] = pTarSeg[k];
> k--;
> }
> pTarSeg[j] = TempSeg[0];
> pTarSeg[j + 1] = TempSeg[1];
> *pTarSegNum += AddPtNum;
> }
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==SiftAndSeqSDGPts:Org(%d,%d)",
> *pLeftSegNum, *pRightSegNum);
> TLOG_INFO << log_string;
> }
> for (j = 0; j < 2; j++) {
> if (j == 0) {
> pTarSeg = &pLeftSeg[0];
> pTarSegNum = pLeftSegNum;
> } else {
> pTarSeg = &pRightSeg[0];
> pTarSegNum = pRightSegNum;
> }
> i = 2;
> AddPtNum = 0;
> while (i < *pTarSegNum) {
> AddPtNum = 0;
> bAddSeg[0] = FALSE;
> if ((pTarSeg[i].y < pTarSeg[i - 1].y)) {
> // check FrontSeg;
> NewSeg[0] = pTarSeg[i - 1];
> NewSeg[1] = pTarSeg[i];
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NewSeg[0], 2,
> pRectPt, pRectLine)) {
> bAddSeg[0] = TRUE;
> if (j == 0) {
> fDis = VirSegX[0];
> if (fDis > NewSeg[0].x) {
> fDis = NewSeg[0].x;
> }
> if (fDis > NewSeg[1].x) {
> fDis = NewSeg[1].x;
> }
> } else {
> fDis = VirSegX[1];
> if (fDis < NewSeg[0].x) {
> fDis = NewSeg[0].x;
> }
> if (fDis < NewSeg[1].x) {
> fDis = NewSeg[1].x;
> }
> }
> AddSeg[0][0].x = fDis;
> AddSeg[0][0].y = NewSeg[0].y;
> AddSeg[0][1].x = fDis;
> AddSeg[0][1].y = NewSeg[1].y;
> }
> }
> bAddSeg[1] = FALSE;
> if (((i + 2) < *pTarSegNum) && (pTarSeg[i + 1].y > pTarSeg[i + 2].y)) {
> // check BackSeg;
> NewSeg[0] = pTarSeg[i + 1];
> NewSeg[1] = pTarSeg[i + 2];
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NewSeg[0], 2,
> pRectPt, pRectLine)) {
> bAddSeg[1] = TRUE;
> if (j == 0) {
> fDis = VirSegX[0];
> if (fDis > NewSeg[0].x) {
> fDis = NewSeg[0].x;
> }
> if (fDis > NewSeg[1].x) {
> fDis = NewSeg[1].x;
> }
> } else {
> fDis = VirSegX[1];
> if (fDis < NewSeg[0].x) {
> fDis = NewSeg[0].x;
> }
> if (fDis < NewSeg[1].x) {
> fDis = NewSeg[1].x;
> }
> }
> AddSeg[1][0].x = fDis;
> AddSeg[1][0].y = NewSeg[0].y;
> AddSeg[1][1].x = fDis;
> AddSeg[1][1].y = NewSeg[1].y;
> }
> }
> if (bAddSeg[0] == TRUE) {
> AddPtNum += 2;
> }
> if (bAddSeg[1] == TRUE) {
> AddPtNum += 2;
> }
> if ((*pTarSegNum + AddPtNum) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
> // buff is not big enough
> return;
> }
> k = *pTarSegNum - 1;
> while (k >= i + 2) {
> pTarSeg[k + AddPtNum] = pTarSeg[k];
> k--;
> }
> k = i + AddPtNum;
> if (bAddSeg[1] == TRUE) {
> pTarSeg[k] = AddSeg[1][0];
> pTarSeg[k + 1] = AddSeg[1][1];
> }
> if (bAddSeg[0] == TRUE) {
> pTarSeg[i + 2] = pTarSeg[i];
> pTarSeg[i + 3] = pTarSeg[i + 1];
> pTarSeg[i] = AddSeg[0][0];
> pTarSeg[i + 1] = AddSeg[0][1];
> }
> i += AddPtNum;
> i += 4;
> *pTarSegNum += AddPtNum;
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==SiftAndSeqSDGPts:Add(%d,%d)",
> *pLeftSegNum, *pRightSegNum);
> TLOG_INFO << log_string;
> }
> #else
> *pLeftSegNum = 0;
> *pRightSegNum = 0;
> #endif
> return;
> }
> BOOLEAN APAMap_ParkingOutGetSDGInfoByParkMode(
> APACarCoordinateDataCalFloatType* pCurCarPos,
> APACoordinateDataCalFloatType* pLeftSeg,
> APACoordinateDataCalFloatType* pRightSeg, uint8_INF u8LeftSegNum,
> uint8_INF u8RightSegNum, APACoordinateDataCalFloatType* pSDGObj2Info,
> APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE* pObj2PtNum,
> APA_ENUM_TYPE* pObj1PtNum) {
> APA_ENUM_TYPE i, k;
> uint8_t_INF ParkOutMode;
> uint8_t_INF ParkMode;
> APACoordinateDataCalFloatType NewPto;
> APA_DISTANCE_CAL_FLOAT_TYPE NewAngle;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType* pObj2Src;
> APACoordinateDataCalFloatType* pObj1Src;
> APA_ENUM_TYPE Obj2Num;
> APA_ENUM_TYPE Obj1Num;
> APA_ENUM_TYPE Obj2StrIndex;
> APA_ENUM_TYPE Obj1StrIndex;
> APA_ENUM_TYPE Obj1SearchStep;
> APA_ENUM_TYPE Obj2SearchStep;
> APA_ENUM_TYPE LeftSegNum;
> APA_ENUM_TYPE RightSegNum;
> APACoordinateDataCalFloatType TempPt;
> *pObj2PtNum = 0;
> *pObj1PtNum = 0;
> ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> NewPto = pCurCarPos->Coordinate;
> NewAngle = pCurCarPos->CarAng;
> if (u8LeftSegNum > APA_MAP_BK_SDG_OUTPUT_MAX_NUM) {
> LeftSegNum = 0;
> } else {
> LeftSegNum = (APA_ENUM_TYPE)u8LeftSegNum;
> }
> if (u8RightSegNum > APA_MAP_BK_SDG_OUTPUT_MAX_NUM) {
> RightSegNum = 0;
> } else {
> RightSegNum = (APA_ENUM_TYPE)u8RightSegNum;
> }
> AlgCom_SmoothSegMent(TRUE, 1000, 10, pLeftSeg, &LeftSegNum);
> AlgCom_SmoothSegMent(FALSE, 1000, 10, pRightSeg, &RightSegNum);
> if (LeftSegNum > 0) {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==GetSDGInfoByParkMode==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%."
> "2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f),"
> "5(%.2f,%.2f),6(%.2f,%.2f),7(%.2f,%.2f),8(%.2f,%.2f),9(%.2f,%.2f),"
> "10(%.2f,%.2f),11(%.2f,%.2f),"
> "12(%.2f,%.2f),13(%.2f,%.2f),14(%.2f,%.2f),15(%.2f,%.2f),16(%.2f,%"
> ".2f),17(%.2f,%.2f),18(%.2f,%.2f),19(%.2f,%.2f)",
> LeftSegNum, pLeftSeg[0].x, pLeftSeg[0].y, pLeftSeg[1].x,
> pLeftSeg[1].y, pLeftSeg[2].x, pLeftSeg[2].y, pLeftSeg[3].x,
> pLeftSeg[3].y, pLeftSeg[4].x, pLeftSeg[4].y, pLeftSeg[5].x,
> pLeftSeg[5].y, pLeftSeg[6].x, pLeftSeg[6].y, pLeftSeg[7].x,
> pLeftSeg[7].y, pLeftSeg[8].x, pLeftSeg[8].y, pLeftSeg[9].x,
> pLeftSeg[9].y, pLeftSeg[10].x, pLeftSeg[10].y, pLeftSeg[11].x,
> pLeftSeg[11].y, pLeftSeg[12].x, pLeftSeg[12].y, pLeftSeg[13].x,
> pLeftSeg[13].y, pLeftSeg[14].x, pLeftSeg[14].y, pLeftSeg[15].x,
> pLeftSeg[15].y, pLeftSeg[16].x, pLeftSeg[16].y, pLeftSeg[17].x,
> pLeftSeg[17].y, pLeftSeg[18].x, pLeftSeg[18].y, pLeftSeg[19].x,
> pLeftSeg[19].y);
> TLOG_INFO << log_string;
> }
> if (RightSegNum > 0) {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==GetSDGInfoByParkMode==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%."
> "2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f),"
> "5(%.2f,%.2f),6(%.2f,%.2f),7(%.2f,%.2f),8(%.2f,%.2f),9(%.2f,%.2f),"
> "10(%.2f,%.2f),11(%.2f,%.2f),"
> "12(%.2f,%.2f),13(%.2f,%.2f),14(%.2f,%.2f),15(%.2f,%.2f),16(%.2f,%"
> ".2f),17(%.2f,%.2f),18(%.2f,%.2f),19(%.2f,%.2f)",
> RightSegNum, pRightSeg[0].x, pRightSeg[0].y, pRightSeg[1].x,
> pRightSeg[1].y, pRightSeg[2].x, pRightSeg[2].y, pRightSeg[3].x,
> pRightSeg[3].y, pRightSeg[4].x, pRightSeg[4].y, pRightSeg[5].x,
> pRightSeg[5].y, pRightSeg[6].x, pRightSeg[6].y, pRightSeg[7].x,
> pRightSeg[7].y, pRightSeg[8].x, pRightSeg[8].y, pRightSeg[9].x,
> pRightSeg[9].y, pRightSeg[10].x, pRightSeg[10].y, pRightSeg[11].x,
> pRightSeg[11].y, pRightSeg[12].x, pRightSeg[12].y, pRightSeg[13].x,
> pRightSeg[13].y, pRightSeg[14].x, pRightSeg[14].y, pRightSeg[15].x,
> pRightSeg[15].y, pRightSeg[16].x, pRightSeg[16].y, pRightSeg[17].x,
> pRightSeg[17].y, pRightSeg[18].x, pRightSeg[18].y, pRightSeg[19].x,
> pRightSeg[19].y);
> TLOG_INFO << log_string;
> }
> if ((LeftSegNum == 0) && (RightSegNum == 0)) {
> return FALSE;
> }
> if ((ParkMode != APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
> return FALSE;
> }
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
> if (bSlotDataAtRigthSide == TRUE) {
> pObj2Src = pRightSeg;
> Obj2Num = RightSegNum;
> pObj1Src = pLeftSeg;
> Obj1Num = LeftSegNum;
> } else {
> pObj2Src = pLeftSeg;
> Obj2Num = LeftSegNum;
> pObj1Src = pRightSeg;
> Obj1Num = RightSegNum;
> }
> Obj2StrIndex = Obj2Num - 1;
> Obj2SearchStep = -1;
> Obj1StrIndex = 0;
> Obj1SearchStep = 1;
> } else {
> if (bSlotDataAtRigthSide == TRUE) {
> pObj2Src = pLeftSeg;
> Obj2Num = LeftSegNum;
> pObj1Src = pRightSeg;
> Obj1Num = RightSegNum;
> } else {
> pObj2Src = pRightSeg;
> Obj2Num = RightSegNum;
> pObj1Src = pLeftSeg;
> Obj1Num = LeftSegNum;
> }
> Obj2StrIndex = 0;
> Obj2SearchStep = 1;
> Obj1StrIndex = Obj1Num - 1;
> Obj1SearchStep = -1;
> }
> i = Obj2StrIndex;
> k = 0;
> while (k < Obj2Num) {
> TempPt = pObj2Src[i];
> pSDGObj2Info[k] =
> AlgCom_PointPosWithAngAndCenterPt(TempPt, NewAngle, NewPto);
> k++;
> i += Obj2SearchStep;
> }
> *pObj2PtNum = Obj2Num;
> i = Obj1StrIndex;
> k = 0;
> while (k < Obj1Num) {
> TempPt = pObj1Src[i];
> pSDGObj1Info[k] =
> AlgCom_PointPosWithAngAndCenterPt(TempPt, NewAngle, NewPto);
> k++;
> i += Obj1SearchStep;
> }
> *pObj1PtNum = Obj1Num;
> return TRUE;
> }
> void APAMap_ParkingOutGetBkSDGOutPutData(
> APACoordinateDataCalFloatType* pSDGObj2Info,
> APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE* pObj2PtNum,
> APA_ENUM_TYPE* pObj1PtNum) {
> APA_ENUM_TYPE i;
> *pObj2PtNum = 0;
> *pObj1PtNum = 0;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
> if ((APAMap_BkSDGOutPutData.Obj2PtNum > 0) &&
> (APAMap_BkSDGOutPutData.Obj2PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
> for (i = 0; i < APAMap_BkSDGOutPutData.Obj2PtNum; i++) {
> pSDGObj2Info[i] = APAMap_BkSDGOutPutData.SDGObj2Info[i];
> }
> *pObj2PtNum = APAMap_BkSDGOutPutData.Obj2PtNum;
> }
> if ((APAMap_BkSDGOutPutData.Obj1PtNum > 0) &&
> (APAMap_BkSDGOutPutData.Obj1PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
> for (i = 0; i < APAMap_BkSDGOutPutData.Obj1PtNum; i++) {
> pSDGObj1Info[i] = APAMap_BkSDGOutPutData.SDGObj1Info[i];
> }
> *pObj1PtNum = APAMap_BkSDGOutPutData.Obj1PtNum;
> }
> }
> return;
> }
> void APAMap_ParkingOutSaveBkSDGOutPutData(
> APACoordinateDataCalFloatType* pSDGObj2Info,
> APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE Obj2PtNum,
> APA_ENUM_TYPE Obj1PtNum) {
> APA_ENUM_TYPE i;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
> if ((Obj2PtNum > 0) && (Obj2PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
> for (i = 0; i < Obj2PtNum; i++) {
> APAMap_BkSDGOutPutData.SDGObj2Info[i] = pSDGObj2Info[i];
> }
> APAMap_BkSDGOutPutData.Obj2PtNum = Obj2PtNum;
> }
> if ((Obj1PtNum > 0) && (Obj1PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
> for (i = 0; i < Obj1PtNum; i++) {
> APAMap_BkSDGOutPutData.SDGObj1Info[i] = pSDGObj1Info[i];
> }
> APAMap_BkSDGOutPutData.Obj1PtNum = Obj1PtNum;
> }
> }
> return;
> }
> void APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus(void) {
> BOOLEAN bSlotDataAtRigthSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pSlotBordBk;
> APA_ENUM_TYPE i;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> }
> pSlotBordBk = &APAMap_BkDataBfSDGFus.MapMainSlotBord;
> if ((pSlotBordBk->PtNum > 0) &&
> (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pSlotBordBk->PtNum; i++) {
> pMapMainSlotBord->Property[i] = pSlotBordBk->Property[i];
> pMapMainSlotBord->Points[i] = pSlotBordBk->Points[i];
> }
> pMapMainSlotBord->PtNum = pSlotBordBk->PtNum;
> }
> APAMap_GInfo.SlotPar.Obj1PtIndex = APAMap_BkDataBfSDGFus.Obj1PtIndex;
> APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_BkDataBfSDGFus.SlotStrIndex;
> APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_BkDataBfSDGFus.SlotEndIndex;
> APAMap_GInfo.SlotPar.Obj2PtIndex = APAMap_BkDataBfSDGFus.Obj2PtIndex;
> }
> return;
> }
> void APAMap_ParkingOutDeleteMainSlotBord(void) {
> BOOLEAN bSlotDataAtRigthSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> APA_ENUM_TYPE i, j;
> APACoordinateDataCalFloatType TempPt1;
> APACoordinateDataCalFloatType TempPt2;
> APACoordinateDataCalFloatType TempPt3;
> APACoordinateDataCalFloatType TempPt4;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_DISTANCE_TYPE PrePtNum;
> APA_DISTANCE_TYPE PtNumTemp;
> APACoordinateDataCalFloatType Pto;
> PrePtNum = 0;
> PtNumTemp = 0;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> }
> PrePtNum = pMapMainSlotBord->PtNum;
> if (pMapMainSlotBord->PtNum > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[PrePtNum - 1], 0, Angle, Pto);
> for (i = 2; i < (PrePtNum - APAMap_GInfo.SlotPar.SlotEndIndex); i++) {
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[PrePtNum - i], 0, Angle, Pto);
> if (MATH_FABS(TempPt1.y - TempPt2.y) < 0.1) {
> pMapMainSlotBord->PtNum -= 1;
> }
> }
> #if 1
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[PrePtNum - 2], 0, Angle, Pto);
> #endif
> TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[0], 0, Angle, Pto);
> for (i = 1; i < APAMap_GInfo.SlotPar.SlotStrIndex; i++) {
> TempPt4 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[i], 0, Angle, Pto);
> if (MATH_FABS(TempPt3.y - TempPt4.y) < 0.1) {
> PtNumTemp++;
> }
> }
> if (PtNumTemp > 0) {
> for (j = 0; j < (pMapMainSlotBord->PtNum - PtNumTemp); j++) {
> pMapMainSlotBord->Property[j] =
> pMapMainSlotBord->Property[j + PtNumTemp];
> pMapMainSlotBord->Points[j] = pMapMainSlotBord->Points[j + PtNumTemp];
> }
> pMapMainSlotBord->PtNum -= PtNumTemp;
> APAMap_GInfo.SlotPar.Obj1PtIndex -= PtNumTemp;
> APAMap_GInfo.SlotPar.SlotStrIndex -= PtNumTemp;
> APAMap_GInfo.SlotPar.SlotEndIndex -= PtNumTemp;
> APAMap_GInfo.SlotPar.Obj2PtIndex -= PtNumTemp;
> }
> #if 1
> TempPt4 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pMapMainSlotBord->Points[1], 0, Angle, Pto);
> #endif
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==DeleteMainSlotBord==PrePtNum(%d)==PtNum(%d)==TempPt1(%.2f,%.2f)="
> "=TempPt2(%.2f,%.2f)"
> "==TempPt3(%.2f,%.2f)==TempPt4(%.2f,%.2f)==SlotIndex(%d,%d,%d,%d)",
> PrePtNum, pMapMainSlotBord->PtNum, TempPt1.x, TempPt1.y, TempPt2.x,
> TempPt2.y, TempPt3.x, TempPt3.y, TempPt4.x, TempPt4.y,
> APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
> APAMap_GInfo.SlotPar.SlotEndIndex,
> APAMap_GInfo.SlotPar.Obj2PtIndex);
> TLOG_INFO << log_string;
> }
> }
> }
> return;
> }
> void APAMap_ParkingOutSaveBkDataBfSDGFus(void) {
> BOOLEAN bSlotDataAtRigthSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pSlotBordBk;
> APA_ENUM_TYPE i;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> }
> pSlotBordBk = &APAMap_BkDataBfSDGFus.MapMainSlotBord;
> if ((pMapMainSlotBord->PtNum > 0) &&
> (pMapMainSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pMapMainSlotBord->PtNum; i++) {
> pSlotBordBk->Property[i] = pMapMainSlotBord->Property[i];
> pSlotBordBk->Points[i] = pMapMainSlotBord->Points[i];
> }
> pSlotBordBk->PtNum = pMapMainSlotBord->PtNum;
> }
> APAMap_BkDataBfSDGFus.Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> APAMap_BkDataBfSDGFus.SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> APAMap_BkDataBfSDGFus.SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> APAMap_BkDataBfSDGFus.Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> }
> return;
> }
> void APAMap_ParkingOutUpDataMapBoundaryBySDGInfo(void) {
> APACoordinateDataCalFloatType pSDGObj2Info[APA_MAP_BK_SDG_OUTPUT_MAX_NUM];
> APACoordinateDataCalFloatType pSDGObj1Info[APA_MAP_BK_SDG_OUTPUT_MAX_NUM];
> APA_ENUM_TYPE Obj2PtNum;
> APA_ENUM_TYPE Obj1PtNum;
> if (TRUE == APAMap_ParkingOutCheckIfFusBoundarySDGInfo()) {
> if (TRUE == APAMap_ParkingOutGetSDGInfoPt(&pSDGObj2Info[0],
> &pSDGObj1Info[0], &Obj2PtNum,
> &Obj1PtNum)) {
> APAMap_ParkingOutSaveBkSDGOutPutData(&pSDGObj2Info[0], &pSDGObj1Info[0],
> Obj2PtNum, Obj1PtNum);
> } else {
> APAMap_ParkingOutGetBkSDGOutPutData(&pSDGObj2Info[0], &pSDGObj1Info[0],
> &Obj2PtNum, &Obj1PtNum);
> }
> if ((Obj2PtNum > 0) || (Obj1PtNum > 0)) {
> APAMap_ParkingOutSaveBkDataBfSDGFus();
> APAMap_ParkingOutFusBoundaryBySDGInfo(&pSDGObj2Info[0], &pSDGObj1Info[0],
> Obj2PtNum, Obj1PtNum);
> APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Updata;
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==SDGFusionBorderUpData!==");
> TLOG_INFO << log_string;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FusSDGStatus:(%d)==BkDataBfSDGFus:Index(%d,%d,%d,%d),BkBordPtNum("
> "%d)==BkSDGOutPutDataNum:(%d,%d)",
> APAMap_BkDataBfSDGFus.FusSDGStatus,
> APAMap_BkDataBfSDGFus.SlotStrIndex,
> APAMap_BkDataBfSDGFus.SlotEndIndex,
> APAMap_BkDataBfSDGFus.Obj1PtIndex, APAMap_BkDataBfSDGFus.Obj2PtIndex,
> APAMap_BkDataBfSDGFus.MapMainSlotBord.PtNum,
> APAMap_BkSDGOutPutData.Obj2PtNum, APAMap_BkSDGOutPutData.Obj1PtNum);
> TLOG_INFO << log_string;
> return;
> }
> BOOLEAN APAMap_ParkingOutCheckIfFusBoundarySDGInfo(void) {
> uint8_t_INF ParkOutMode;
> uint8_t_INF ParkMode;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType Pto;
> BOOLEAN bSlotDataAtRigthSide;
> BOOLEAN bResult;
> BOOLEAN bResult1;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACoordinateDataCalFloatType TempPt2;
> APACoordinateDataCalFloatType TempPt1;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis2;
> APACarCoordinateDataCalFloatType TempCarPos;
> APALineParameterABCType TopLine;
> APALineParameterABCType BottomLine;
> BOOLEAN bCarPosValidForFusSDG;
> APA_ENUM_TYPE CarCorIndex[4];
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> CurCarPos = APAMap_GInfo.CarPos;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, &pRectPt[0], &pRectLine[0]);
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.Obj2Pt;
> TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempCarPos.Coordinate, 0, Angle, Pto);
> Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;
> Obj2Ang -= Angle;
> TempCarPos.CarAng = Obj2Ang;
> TopLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.Obj1Pt;
> TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempCarPos.Coordinate, 0, Angle, Pto);
> Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;
> Obj1Ang -= Angle;
> TempCarPos.CarAng = Obj1Ang;
> BottomLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> bCarPosValidForFusSDG = FALSE;
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Keep) {
> return FALSE;
> }
> if ((ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) &&
> (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
> if ((ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
> (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
> CarCorIndex[0] = 1;
> CarCorIndex[1] = 2;
> CarCorIndex[2] = 0;
> CarCorIndex[3] = 3;
> } else {
> CarCorIndex[0] = 0;
> CarCorIndex[1] = 3;
> CarCorIndex[2] = 1;
> CarCorIndex[3] = 2;
> }
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pRectPt[CarCorIndex[0]], 0, Angle, Pto);
> TempDis1 = TopLine.A * TempPt1.x + TopLine.C - TempPt1.y;
> TempDis2 = BottomLine.A * TempPt1.x + BottomLine.C - TempPt1.y;
> if ((TempDis1 >= 0) && (TempDis2 <= 0)) {
> bResult = TRUE;
> } else {
> bResult = FALSE;
> }
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pRectPt[CarCorIndex[1]], 0, Angle, Pto);
> TempDis1 = TopLine.A * TempPt2.x + TopLine.C - TempPt2.y;
> TempDis2 = BottomLine.A * TempPt2.x + BottomLine.C - TempPt2.y;
> if ((TempDis1 >= 0) && (TempDis2 <= 0)) {
> bResult1 = TRUE;
> } else {
> bResult1 = FALSE;
> }
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt2.x = -TempPt2.x;
> TempPt1.x = -TempPt1.x;
> }
> if (TempPt2.x < TempPt1.x) {
> TempDis = TempPt2.x;
> } else {
> TempDis = TempPt1.x;
> }
> if ((bResult == TRUE) && (bResult1 == TRUE) && (TempDis > 0)) {
> // car two innercor still in slot;
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pRectPt[CarCorIndex[2]], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pRectPt[CarCorIndex[3]], 0, Angle, Pto);
> if (bSlotDataAtRigthSide == FALSE) {
> TempPt2.x = -TempPt2.x;
> TempPt1.x = -TempPt1.x;
> }
> if (TempPt2.x < TempPt1.x) {
> TempDis = TempPt2.x;
> } else {
> TempDis = TempPt1.x;
> }
> if (TempDis > -3500)  //-1500
> {
> // car two out corner still not far away from slot;
> bCarPosValidForFusSDG = TRUE;
> }
> }
> if (bCarPosValidForFusSDG == FALSE) {
> APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Keep;
> }
> } else {
> APAMap_ParkingOutBkDataBfSDGFusInit();
> }
> return bCarPosValidForFusSDG;
> }
> BOOLEAN
> APAMap_ParkingOutGetSDGInfoPt(APACoordinateDataCalFloatType* pSDGObj2Info,
> APACoordinateDataCalFloatType* pSDGObj1Info,
> APA_ENUM_TYPE* pObj2PtNum,
> APA_ENUM_TYPE* pObj1PtNum) {
> #ifdef APAMAP_PARKOUT_USE_SDG_OBJS
> #if 0
> APACoordinateDataCalFloatType Obj1SDGPt[5];
> APACoordinateDataCalFloatType Obj2SDGPt[5];
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType Pto;
> BOOLEAN bSlotDataAtRigthSide;
> APA_ENUM_TYPE i;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> for(i = 0; i < 5;i++)
> {
> Obj2SDGPt[i].x = 0;
> Obj2SDGPt[i].y = 0;
> Obj1SDGPt[i].x = 0;
> Obj1SDGPt[i].y = 0;
> }
> *pObj2PtNum = 2;
> *pObj1PtNum = 2;
> Obj2SDGPt[0].x = 3500;
> Obj2SDGPt[0].y = -1800;//200;//-100;
> Obj2SDGPt[1].x = 0;
> Obj2SDGPt[1].y = 400;//-100;//200;
> Obj1SDGPt[0].x = 0;
> Obj1SDGPt[0].y = -4300;//-3900;//-4200;
> Obj1SDGPt[1].x = 3500;
> Obj1SDGPt[1].y = -5300;//-4200;//-3900;
> if(bSlotDataAtRigthSide == FALSE)
> {
> for(i = 0; i < *pObj2PtNum;i++)
> {
> Obj2SDGPt[i].x = -Obj2SDGPt[i].x;
> }
> for(i = 0; i < *pObj1PtNum;i++)
> {
> Obj1SDGPt[i].x = -Obj1SDGPt[i].x;
> }
> }
> for(i = 0; i < *pObj2PtNum;i++)
> {
> pSDGObj2Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj2SDGPt[i],Angle,Pto);
> }
> for(i = 0; i < *pObj1PtNum;i++)
> {
> pSDGObj1Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj1SDGPt[i],Angle,Pto);
> }
> {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==SDGDebugData==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)"
> "==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)",
> *pObj2PtNum,
> pSDGObj2Info[0].x,pSDGObj2Info[0].y,
> pSDGObj2Info[1].x,pSDGObj2Info[1].y,
> pSDGObj2Info[2].x,pSDGObj2Info[2].y,
> pSDGObj2Info[3].x,pSDGObj2Info[3].y,
> pSDGObj2Info[4].x,pSDGObj2Info[4].y,
> *pObj1PtNum,
> pSDGObj1Info[0].x,pSDGObj1Info[0].y,
> pSDGObj1Info[1].x,pSDGObj1Info[1].y,
> pSDGObj1Info[2].x,pSDGObj1Info[2].y,
> pSDGObj1Info[3].x,pSDGObj1Info[3].y,
> pSDGObj1Info[4].x,pSDGObj1Info[4].y);
> TLOG_INFO << log_string;
> }
> return TRUE;
> #endif
> APACoordinateDataCalFloatType LeftSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> APACoordinateDataCalFloatType RightSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> uint8_INF LeftSegNum;
> uint8_INF RightSegNum;
> APACarCoordinateDataCalFloatType CurCarPos;
> CurCarPos = APAMap_GInfo.CarPos;
> APAMap_ParkingOutSiftAndSeqSDGPts(&CurCarPos, &LeftSeg[0], &RightSeg[0],
> &LeftSegNum, &RightSegNum);
> if (TRUE == APAMap_ParkingOutGetSDGInfoByParkMode(
> &CurCarPos, &LeftSeg[0], &RightSeg[0], LeftSegNum,
> RightSegNum, pSDGObj2Info, pSDGObj1Info, pObj2PtNum,
> pObj1PtNum)) {
> return TRUE;
> } else {
> return FALSE;
> }
> #else
> return FALSE;
> #endif
> }
> // APAMap_BkDataBfSDGFus
> BOOLEAN APAMap_ParkingOutFusBoundaryBySDGInfo(
> APACoordinateDataCalFloatType* pSDGObj2Info,
> APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE Obj2PtNum,
> APA_ENUM_TYPE Obj1PtNum) {
> APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
> APACoordinateDataCalFloatType Obj2InnerPt, Obj1InnerPt;
> APACoordinateDataCalFloatType SDGObj2EndPt, SDGObj1StrPt;
> APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
> APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
> APA_ENUM_TYPE Index;
> APA_DISTANCE_TYPE i;
> APA_ENUM_TYPE k;
> tMap_BoundPt_t* pMapMainSlotBord;
> APA_DISTANCE_CAL_FLOAT_TYPE OuterBorderFusAng;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj2InnerBorderFusAng;
> APA_DISTANCE_CAL_FLOAT_TYPE Obj1InnerBorderFusAng;
> BOOLEAN bSlotDataAtRigthSide;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType Pto;
> APACarCoordinateDataCalFloatType TempCarPos;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType pData1[127];
> APACoordinateDataCalFloatType pData2[127];
> APACoordinateDataCalFloatType pData3[127];
> APACoordinateDataCalFloatType pData4[127];
> APACoordinateDataCalFloatType pData5[127];
> APA_ENUM_TYPE SDGDataPtNum;
> uint8_t_INF pPtStyle[127];
> uint8_t_INF NewProperty1[127];
> uint8_t_INF NewProperty2[127];
> uint8_t_INF NewProperty3[127];
> uint8_t_INF NewProperty4[127];
> uint8_t_INF NewProperty5[127];
> uint8_t_INF SDGSegProperty[127];
> uint16_t_INF DataNum;
> APA_ENUM_TYPE Data1Num;
> APA_ENUM_TYPE Data2Num;
> APA_ENUM_TYPE Data3Num;
> APA_ENUM_TYPE Data4Num;
> APA_ENUM_TYPE Data5Num;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
> APALineParameterABCType TempLine1;
> APALineParameterABCType TempLine2;
> APACoordinateDataCalFloatType StrPt, EndPt;
> APACoordinateDataCalFloatType TempPt1;
> APALineParameterABCType BordLine[2];
> BOOLEAN bCallNewObjPt;
> APACoordinateDataCalFloatType NewObjPt;
> APACoordinateDataCalFloatType SDGObj2OuterInfo[100];
> APA_ENUM_TYPE SDGObj2OutPtNum;
> APACoordinateDataCalFloatType SDGObj2InnerInfo[100];
> APA_ENUM_TYPE SDGObj2InnerPtNum;
> APACoordinateDataCalFloatType SDGObj1OuterInfo[100];
> APA_ENUM_TYPE SDGObj1OutPtNum;
> APACoordinateDataCalFloatType SDGObj1InnerInfo[100];
> APA_ENUM_TYPE SDGObj1InnerPtNum;
> BOOLEAN bCrossBordLine;
> APACoordinateDataCalFloatType* pSDGData;
> APA_ENUM_TYPE CrossPtIndex;
> APACoordinateDataCalFloatType CrossPt;
> APACoordinateDataCalFloatType Obj2ValidPt[100];
> APACoordinateDataCalFloatType Obj1ValidPt[100];
> APA_ENUM_TYPE ValidObj2Num;
> APA_ENUM_TYPE ValidObj1Num;
> APA_DISTANCE_CAL_FLOAT_TYPE PrePtY;
> APACarCoordinateDataCalFloatType CurCarPos;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APA_ENUM_TYPE ValidObjNum;
> BOOLEAN bValidFlag;
> CurCarPos = APAMap_GInfo.CarPos;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if ((Obj2PtNum <= 0) && (Obj1PtNum <= 0)) {
> return FALSE;
> }
> MaxOffsetX = 1500;
> MaxOffsetY = 2000;
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> }
> SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> OrgObj2Pt = pMapMainSlotBord->Points[Obj2PtIndex];
> OrgObj1Pt = pMapMainSlotBord->Points[Obj1PtIndex];
> Obj2InnerPt = pMapMainSlotBord->Points[SlotEndIndex];
> Obj1InnerPt = pMapMainSlotBord->Points[SlotStrIndex];
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> if (bSlotDataAtRigthSide) {
> OuterBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&OuterBorderFusAng);
> Obj2InnerBorderFusAng = Angle;
> Obj1InnerBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI);
> AlgCom_AngNormalized(&Obj1InnerBorderFusAng);
> } else {
> OuterBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
> AlgCom_AngNormalized(&OuterBorderFusAng);
> Obj2InnerBorderFusAng = Angle;
> Obj1InnerBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI);
> AlgCom_AngNormalized(&Obj1InnerBorderFusAng);
> }
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> TempPt.x = MaxOffsetX;
> TempPt.y = 0;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> TempCarPos.Coordinate = TempPt;
> TempCarPos.CarAng = Angle;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> i = Obj2PtNum - 1;
> bCrossBordLine = FALSE;
> CrossPtIndex = 0;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
> while (i > 0) {
> StrPt = pSDGObj2Info[i];
> EndPt = pSDGObj2Info[i - 1];
> if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
> if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt)) {
> if (TRUE ==
> AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
> bCrossBordLine = TRUE;
> CrossPtIndex = i;
> CrossPt = TempPt;
> break;
> }
> }
> }
> i--;
> }
> if (bCrossBordLine == FALSE) {
> CrossPt = pSDGObj2Info[Obj2PtNum - 1];
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CrossPt, 0,
> Angle, Pto);
> if (((TempPt1.x >= MaxOffsetX) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x <= MaxOffsetX) && (bSlotDataAtRigthSide == FALSE))) {
> CrossPtIndex = Obj2PtNum;
> } else {
> CrossPtIndex = 0;
> }
> }
> ValidObjNum = 0;
> ValidObj2Num = 0;
> for (Index = 0; Index < CrossPtIndex; Index++) {
> Obj2ValidPt[ValidObj2Num] = pSDGObj2Info[Index];
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> Obj2ValidPt[ValidObj2Num], 0, Angle, Pto);
> if (ValidObj2Num < (CrossPtIndex - 1)) {
> ValidObjNum = ValidObj2Num + 1;
> Obj2ValidPt[ValidObjNum] = pSDGObj2Info[CrossPtIndex - 1];
> } else {
> ValidObjNum = ValidObj2Num;
> }
> bValidFlag = FALSE;
> if (FALSE == APAMap_CheckIfObjWithinRectArea(
> 0x01, &Obj2ValidPt[0], ValidObjNum, pRectPt, pRectLine)) {
> if (TempPt1.y <= MaxOffsetY) {
> ValidObj2Num++;
> bValidFlag = TRUE;
> } else if (((bSlotDataAtRigthSide == TRUE) && (TempPt1.x >= 0)) ||
> ((bSlotDataAtRigthSide == FALSE) && (TempPt1.x <= 0))) {
> ValidObj2Num++;
> bValidFlag = TRUE;
> }
> }
> if (FALSE == bValidFlag) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj2OuterBorder==1==Obj2ValidPt[%d](%.2f,%.2f)"
> "==TempPt1(%.2f,%.2f)",
> ValidObj2Num, Obj2ValidPt[ValidObj2Num].x,
> Obj2ValidPt[ValidObj2Num].y, TempPt1.x, TempPt1.y);
> TLOG_INFO << log_string;
> }
> }
> if (bCrossBordLine == TRUE) {
> Obj2ValidPt[ValidObj2Num] = CrossPt;
> ValidObj2Num++;
> }
> i = 0;
> bCrossBordLine = FALSE;
> CrossPtIndex = 0;
> while (i < Obj1PtNum - 1) {
> StrPt = pSDGObj1Info[i];
> EndPt = pSDGObj1Info[i + 1];
> if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
> if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt)) {
> if (TRUE ==
> AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
> bCrossBordLine = TRUE;
> CrossPtIndex = i + 1;
> CrossPt = TempPt;
> break;
> }
> }
> }
> i++;
> }
> if (bCrossBordLine == FALSE) {
> CrossPt = pSDGObj1Info[Obj1PtNum - 1];
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CrossPt, 0,
> Angle, Pto);
> if (((TempPt1.x >= MaxOffsetX) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x <= MaxOffsetX) && (bSlotDataAtRigthSide == FALSE))) {
> CrossPtIndex = 0;
> } else {
> CrossPtIndex = Obj1PtNum;
> }
> }
> ValidObjNum = 0;
> ValidObj1Num = 0;
> if (bCrossBordLine == TRUE) {
> Obj1ValidPt[ValidObj1Num] = CrossPt;
> ValidObj1Num++;
> }
> for (Index = CrossPtIndex; Index < Obj1PtNum; Index++) {
> Obj1ValidPt[ValidObj1Num] = pSDGObj1Info[Index];
> if (ValidObj1Num < (Obj1PtNum - 1)) {
> ValidObjNum = ValidObj1Num + 1;
> Obj1ValidPt[ValidObjNum] = pSDGObj1Info[ValidObjNum];
> } else {
> ValidObjNum = ValidObj1Num;
> }
> if (FALSE == APAMap_CheckIfObjWithinRectArea(
> 0x01, &Obj1ValidPt[0], ValidObjNum, pRectPt, pRectLine)) {
> ValidObj1Num++;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj2OuterBorder==2==Obj1ValidPt[%d](%.2f,%.2f)",
> ValidObj1Num, Obj1ValidPt[ValidObj1Num].x,
> Obj1ValidPt[ValidObj1Num].y);
> TLOG_INFO << log_string;
> }
> }
> // obj1 borderline;
> Data1Num = 0;
> for (Index = 0; Index <= Obj1PtIndex; Index++) {
> pData1[Data1Num] = pMapMainSlotBord->Points[Index];
> NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
> Data1Num++;
> if (Data1Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDG Buffer Not enough==1==Data1Num:(%d)", Data1Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // obj2 borderline;
> Data2Num = 0;
> for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
> pData2[Data2Num] = pMapMainSlotBord->Points[Index];
> NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
> Data2Num++;
> if (Data2Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDG Buffer Not enough==2==Data2Num:(%d)", Data2Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // obj1Inner borderline;
> Data3Num = 0;
> for (Index = Obj1PtIndex; Index <= SlotStrIndex; Index++) {
> pData3[Data3Num] = pMapMainSlotBord->Points[Index];
> NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
> Data3Num++;
> if (Data3Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDG Buffer Not enough==3==Data3Num:(%d)", Data3Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // obj2Inner borderline;
> Data4Num = 0;
> for (Index = SlotEndIndex; Index <= Obj2PtIndex; Index++) {
> pData4[Data4Num] = pMapMainSlotBord->Points[Index];
> NewProperty4[Data4Num] = pMapMainSlotBord->Property[Index];
> Data4Num++;
> if (Data4Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDG Buffer Not enough==4==Data4Num:(%d)", Data4Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // SlotInner Borderline;
> Data5Num = 0;
> for (Index = SlotStrIndex + 1; Index < SlotEndIndex; Index++) {
> pData5[Data5Num] = pMapMainSlotBord->Points[Index];
> NewProperty5[Data5Num] = pMapMainSlotBord->Property[Index];
> Data5Num++;
> if (Data5Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDG Buffer Not enough==5==Data5Num:(%d)", Data5Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> for (k = 0; k < 100; k++) {
> SDGSegProperty[k] = 0;
> }
> AlgCom_LineParABCbyTwoPoints(OrgObj2Pt, Obj2InnerPt, &BordLine[1]);
> AlgCom_LineParABCbyTwoPoints(OrgObj1Pt, Obj1InnerPt, &BordLine[0]);
> TempCarPos.Coordinate = pMapMainSlotBord->Points[pMapMainSlotBord->PtNum - 1];
> TempCarPos.CarAng = OuterBorderFusAng;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempCarPos.Coordinate = Obj2ValidPt[ValidObj2Num - 1];
> TempCarPos.CarAng = Angle;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SDGObj2EndPt);
> TempCarPos.Coordinate = pMapMainSlotBord->Points[0];
> TempCarPos.CarAng = OuterBorderFusAng;
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempCarPos.Coordinate = Obj1ValidPt[0];
> TempCarPos.CarAng = Angle;
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SDGObj1StrPt);
> bCallNewObjPt = FALSE;
> bCrossBordLine = FALSE;
> SDGDataPtNum = ValidObj2Num;
> pSDGData = &Obj2ValidPt[0];
> CrossPtIndex = 0;
> NewObjPt = OrgObj2Pt;
> for (i = SDGDataPtNum - 1; i >= 0; i--) {
> if (i == SDGDataPtNum - 1) {
> StrPt = SDGObj2EndPt;
> EndPt = pSDGData[i];
> } else {
> StrPt = pSDGData[i + 1];
> EndPt = pSDGData[i];
> }
> if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
> if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &BordLine[1], &TempPt)) {
> if (TRUE ==
> AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt, 0, Angle, OrgObj2Pt);
> if (((TempPt1.x < 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x > 0) && (bSlotDataAtRigthSide == FALSE))) {
> bCallNewObjPt = TRUE;
> NewObjPt = TempPt;
> }
> bCrossBordLine = TRUE;
> CrossPtIndex = i;
> break;
> }
> }
> }
> }
> if (bCrossBordLine == FALSE) {
> SDGObj2OutPtNum = 0;
> for (i = 0; i < SDGDataPtNum; i++) {
> SDGObj2OuterInfo[SDGObj2OutPtNum] = pSDGData[i];
> SDGObj2OutPtNum++;
> }
> SDGObj2OuterInfo[SDGObj2OutPtNum] = SDGObj2EndPt;
> SDGObj2OutPtNum++;
> SDGObj2InnerPtNum = 0;
> } else {
> SDGObj2OutPtNum = 0;
> for (i = CrossPtIndex; i < SDGDataPtNum; i++) {
> SDGObj2OuterInfo[SDGObj2OutPtNum] = pSDGData[i];
> SDGObj2OutPtNum++;
> }
> SDGObj2OuterInfo[SDGObj2OutPtNum] = SDGObj2EndPt;
> SDGObj2OutPtNum++;
> SDGObj2InnerPtNum = 0;
> k = CrossPtIndex + 1;
> if (k > SDGDataPtNum - 1) {
> k = SDGDataPtNum - 1;
> }
> for (i = 0; i <= k; i++) {
> SDGObj2InnerInfo[SDGObj2InnerPtNum] = pSDGData[i];
> SDGObj2InnerPtNum++;
> }
> }
> i = 0;
> CrossPtIndex = 0;
> while (i < SDGObj2OutPtNum - 1) {
> TempPt = SDGObj2OuterInfo[i];
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(TempPt, 0,
> Angle, Pto);
> if (i == 0) {
> PrePtY = TempPt.y;
> } else if (TempPt.y < PrePtY) {
> CrossPtIndex = i;
> }
> i++;
> }
> k = 0;
> for (i = CrossPtIndex; i < SDGObj2OutPtNum; i++) {
> SDGObj2OuterInfo[k] = SDGObj2OuterInfo[i];
> k++;
> }
> SDGObj2OutPtNum = k;
> if (bCallNewObjPt == TRUE) {
> pData4[Data4Num - 1] = NewObjPt;
> i = 0;
> while (i < Data2Num) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData2[i], 0, Angle, NewObjPt);
> if (TempPt1.y >= 0) {
> if (TempPt1.y == 0) {
> if (((TempPt1.x < 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x > 0) && (bSlotDataAtRigthSide == FALSE))) {
> break;
> }
> } else {
> break;
> }
> }
> i++;
> }
> pData2[0] = NewObjPt;
> Index = 1;
> if (i == 0) {
> k = Data2Num;
> while (k > 0) {
> pData2[k + 1] = pData2[k];
> NewProperty2[k + 1] = NewProperty2[k];
> Index++;
> k--;
> }
> } else {
> for (k = i; k < Data2Num; k++) {
> pData2[Index] = pData2[k];
> NewProperty2[Index] = NewProperty2[k];
> Index++;
> }
> }
> Data2Num = Index;
> }
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, OuterBorderFusAng, &pData2[0], Data2Num,
> &NewProperty2[0], &SDGObj2OuterInfo[0], SDGObj2OutPtNum,
> &SDGSegProperty[0], &pData2[0], &Data2Num, &pPtStyle[0])) {
> // updata obj2 bordline;
> for (k = 0; k < Data2Num; k++) {
> NewProperty2[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj2OuterBorderSuccess==");
> TLOG_INFO << log_string;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj2OuterBorder==(%f)==Data2Num(%d)==InfoNum(%d)=="
> "Data1:0(%f,%f)1(%f,%f)2(%f,%f)"
> "==SDGObj2OuterInfo:0(%f,%f)1(%f,%f)2(%f,%f)",
> OuterBorderFusAng, Data2Num, SDGObj2OutPtNum, pData2[0].x,
> pData2[0].y, pData2[1].x, pData2[1].y, pData2[2].x, pData2[2].y,
> SDGObj2OuterInfo[0].x, SDGObj2OuterInfo[0].y,
> SDGObj2OuterInfo[1].x, SDGObj2OuterInfo[1].y,
> SDGObj2OuterInfo[2].x, SDGObj2OuterInfo[2].y);
> TLOG_INFO << log_string;
> }
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, Obj2InnerBorderFusAng, &pData4[0],
> Data4Num, &NewProperty4[0], &SDGObj2InnerInfo[0],
> SDGObj2InnerPtNum, &SDGSegProperty[0], &pData4[0], &Data4Num,
> &pPtStyle[0])) {
> // updata obj2 Innerbordline;
> for (k = 0; k < Data4Num; k++) {
> NewProperty4[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj2InnerBorderSuccess==");
> TLOG_INFO << log_string;
> }
> bCallNewObjPt = FALSE;
> bCrossBordLine = FALSE;
> SDGDataPtNum = ValidObj1Num;
> pSDGData = &Obj1ValidPt[0];
> CrossPtIndex = 0;
> NewObjPt = OrgObj1Pt;
> for (i = 0; i < SDGDataPtNum; i++) {
> if (i == 0) {
> StrPt = SDGObj1StrPt;
> EndPt = pSDGData[0];
> } else {
> StrPt = pSDGData[i - 1];
> EndPt = pSDGData[i];
> }
> if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
> if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &BordLine[0], &TempPt)) {
> if (TRUE ==
> AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempPt, 0, Angle, OrgObj1Pt);
> if (((TempPt1.x < 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x > 0) && (bSlotDataAtRigthSide == FALSE))) {
> bCallNewObjPt = TRUE;
> NewObjPt = TempPt;
> }
> bCrossBordLine = TRUE;
> CrossPtIndex = i;
> break;
> }
> }
> }
> }
> if (bCrossBordLine == FALSE) {
> SDGObj1OutPtNum = 1;
> SDGObj1OuterInfo[0] = SDGObj1StrPt;
> for (i = 0; i < SDGDataPtNum; i++) {
> SDGObj1OuterInfo[SDGObj1OutPtNum] = pSDGData[i];
> SDGObj1OutPtNum++;
> }
> SDGObj1InnerPtNum = 0;
> } else {
> SDGObj1OutPtNum = 1;
> SDGObj1OuterInfo[0] = SDGObj1StrPt;
> for (i = 0; i <= CrossPtIndex; i++) {
> SDGObj1OuterInfo[SDGObj1OutPtNum] = pSDGData[i];
> SDGObj1OutPtNum++;
> }
> SDGObj1InnerPtNum = 0;
> k = CrossPtIndex - 1;
> if (k < 0) {
> k = 0;
> }
> for (i = k; i <= SDGDataPtNum - 1; i++) {
> SDGObj1InnerInfo[SDGObj1InnerPtNum] = pSDGData[i];
> SDGObj1InnerPtNum++;
> }
> }
> i = 0;
> CrossPtIndex = SDGObj1OutPtNum;
> while (i < SDGObj1OutPtNum - 1) {
> TempPt = SDGObj1OuterInfo[i];
> TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(TempPt, 0,
> Angle, Pto);
> if (i == 0) {
> PrePtY = TempPt.y;
> } else if (TempPt.y < PrePtY) {
> CrossPtIndex = i;
> }
> i++;
> }
> k = 0;
> for (i = 0; i < CrossPtIndex; i++) {
> SDGObj1OuterInfo[k] = SDGObj1OuterInfo[i];
> k++;
> }
> SDGObj1OutPtNum = k;
> if (bCallNewObjPt == TRUE) {
> pData3[0] = NewObjPt;
> i = Data1Num - 1;
> while (i >= 0) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[i], 0, Angle, NewObjPt);
> if (TempPt1.y >= 0) {
> if (TempPt1.y == 0) {
> if (((TempPt1.x > 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x < 0) && (bSlotDataAtRigthSide == FALSE))) {
> break;
> }
> }
> } else {
> i++;
> break;
> }
> i--;
> }
> Index = 0;
> for (k = 0; k < i; k++) {
> pData1[Index] = pData1[k];
> NewProperty1[Index] = NewProperty1[k];
> Index++;
> }
> pData1[Index] = NewObjPt;
> Index++;
> Data1Num = Index;
> }
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, OuterBorderFusAng, &pData1[0], Data1Num,
> &NewProperty1[0], &SDGObj1OuterInfo[0], SDGObj1OutPtNum,
> &SDGSegProperty[0], &pData1[0], &Data1Num, &pPtStyle[0])) {
> // updata obj1 bordline;
> for (k = 0; k < Data1Num; k++) {
> NewProperty1[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj1OuterBorderSuccess==");
> TLOG_INFO << log_string;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj1OuterBorder==(%f)==Data1Num(%d)==InfoNum(%d)=="
> "Data1:0(%f,%f)1(%f,%f)2(%f,%f)"
> "==SDGObj1OuterInfo:0(%f,%f)1(%f,%f)2(%f,%f)",
> OuterBorderFusAng, Data1Num, SDGObj1OutPtNum, pData1[0].x,
> pData1[0].y, pData1[1].x, pData1[1].y, pData1[2].x, pData1[2].y,
> SDGObj1OuterInfo[0].x, SDGObj1OuterInfo[0].y,
> SDGObj1OuterInfo[1].x, SDGObj1OuterInfo[1].y,
> SDGObj1OuterInfo[2].x, SDGObj1OuterInfo[2].y);
> TLOG_INFO << log_string;
> }
> // 3
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, Obj1InnerBorderFusAng, &pData3[0],
> Data3Num, &NewProperty3[0], &SDGObj1InnerInfo[0],
> SDGObj1InnerPtNum, &SDGSegProperty[0], &pData3[0], &Data3Num,
> &pPtStyle[0])) {
> // updata obj1 Innerbordline;
> for (k = 0; k < Data3Num; k++) {
> NewProperty3[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==SDGFusionObj1InnerBorderSuccess==");
> TLOG_INFO << log_string;
> }
> DataNum = Data1Num + Data3Num + Data5Num + Data4Num + Data2Num - 2;
> if (DataNum <= BOUNDARY_PT_MAX_NUM) {
> Index = 0;
> for (k = 0; k < Data1Num; k++) {
> pMapMainSlotBord->Points[Index] = pData1[Index];
> pMapMainSlotBord->Property[Index] = NewProperty1[Index];
> Index++;
> }
> APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
> for (k = 1; k < Data3Num; k++) {
> pMapMainSlotBord->Points[Index] = pData3[k];
> pMapMainSlotBord->Property[Index] = NewProperty3[k];
> Index++;
> }
> APAMap_GInfo.SlotPar.SlotStrIndex = Index - 1;
> for (k = 0; k < Data5Num; k++) {
> pMapMainSlotBord->Points[Index] = pData5[k];
> pMapMainSlotBord->Property[Index] = NewProperty5[k];
> Index++;
> }
> APAMap_GInfo.SlotPar.SlotEndIndex = Index;
> for (k = 0; k < Data4Num - 1; k++) {
> pMapMainSlotBord->Points[Index] = pData4[k];
> pMapMainSlotBord->Property[Index] = NewProperty4[k];
> Index++;
> }
> APAMap_GInfo.SlotPar.Obj2PtIndex = Index;
> for (k = 0; k < Data2Num; k++) {
> pMapMainSlotBord->Points[Index] = pData2[k];
> pMapMainSlotBord->Property[Index] = NewProperty2[k];
> Index++;
> }
> pMapMainSlotBord->PtNum = DataNum;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FSDFusionSubSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==FusBordBySDG==SlotIndex(%d,%d,%d,%d)==OrgSDGPtNum(%d,%d)=="
> "ValidSDGPtNum(%d,%d)"
> "==SDGObj2OutPtNum(%d)==SDGObj2InnerPtNum(%d)==SDGObj1OutPtNum(%d)=="
> "SDGObj1InnerPtNum(%d)",
> APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
> APAMap_GInfo.SlotPar.SlotEndIndex, APAMap_GInfo.SlotPar.Obj2PtIndex,
> Obj2PtNum, Obj1PtNum, ValidObj2Num, ValidObj1Num, SDGObj2OutPtNum,
> SDGObj2InnerPtNum, SDGObj1OutPtNum, SDGObj1InnerPtNum);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> tAPAParkProcEightParkingOutModeType APAMap_ParkingOutGetEightParkOutMode() {
> return ParkOutEightMode;
> }
> void APAMap_ParkingOutCarPosInvadeSlotBorderInfo(
> APACoordinateDataCalFloatType* pObj2Pt,
> APACoordinateDataCalFloatType* pObj1Pt, BOOLEAN bCarLeaveSlotFlag) {
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateY;
> APA_DISTANCE_CAL_FLOAT_TYPE Temp1, Temp2;
> uint8_t_INF ParkOutMode;
> APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
> BOOLEAN bBloundarySeizeCarFlag;  // 边界侵占车辆标志位
> APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
> APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
> if (FALSE == bAfterNewAnchorPointFlag)  // 判断在锚点转换之后
> {
> return;
> }
> if (TRUE == bCarLeaveSlotFlag)  // 判断车离开车位框后
> {
> return;
> }
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if (ParkOutMode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
> return;
> }
> Obj2Pt = *pObj2Pt;
> Obj1Pt = *pObj1Pt;
> bBloundarySeizeCarFlag = FALSE;
> CurCarCoordinateY = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.y;
> CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
> CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
> Temp1 = CurCarCoordinateY - CarLRCal - 100;
> Temp2 = CurCarCoordinateY + CarLFCal + 100;
> if (Temp1 < Obj1Pt.y) {
> bBloundarySeizeCarFlag = TRUE;
> Obj1Pt.y = Temp1 - 300;
> }
> if (Temp2 > Obj2Pt.y) {
> bBloundarySeizeCarFlag = TRUE;
> Obj2Pt.y = Temp2 + 300;
> }
> *pObj2Pt = Obj2Pt;
> *pObj1Pt = Obj1Pt;
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==APAMap_ParkingOutCarPosInvadeSlotBorderInfo=="
> "bBloundarySeizeCarFlag(%d)"
> "==Obj2Pt(%.2f,%.2f)==Obj1Pt(%.2f,%.2f)==Temp2(%.2f)==Temp1(%.2f)",
> bBloundarySeizeCarFlag, Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y,
> Temp2, Temp1);
> TLOG_INFO << log_string;
> }
> return;
> }
> void APAMap_ParkingOutSideSlotInfo(BOOLEAN* pbUpdataCalBoundaryFlag) {
> BOOLEAN bUpdataCalBoundaryFlag;
> APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
> BOOLEAN bSlotDataAtRigthSide;
> uint8_t_INF ParkOutMode;
> bUpdataCalBoundaryFlag = *pbUpdataCalBoundaryFlag;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
> ParkOutMode = APAMap_GInputData.ParkReqPar.parkoutmode;
> if (TRUE ==
> bAfterNewAnchorPointFlag)  // 判断在锚点转换之后，且车辆已开出车位，则不再初始化主边界
> {
> if (bSlotDataAtRigthSide) {
> CurCarCoordinateX = -CurCarCoordinateX;
> }
> if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
> if ((CurCarCoordinateX > 1) ||
> (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
> APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 4))) {
> bUpdataCalBoundaryFlag = FALSE;
> }
> } else if (ParkOutMode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
> if ((CurCarCoordinateX > 2) ||
> (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
> APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 4))) {
> bUpdataCalBoundaryFlag = FALSE;
> }
> } else {
> if (CurCarCoordinateX > 0.3) {
> bUpdataCalBoundaryFlag = FALSE;
> }
> }
> if (TRUE == bLabelAngledFlag) {
> if (TRUE == bLabelAngledParkingOutSlotFlag) {
> bUpdataCalBoundaryFlag = FALSE;
> }
> }
> if (TRUE == bCntAddFlag) {
> bUpdataCalBoundaryFlag = TRUE;
> }
> }
> *pbUpdataCalBoundaryFlag = bUpdataCalBoundaryFlag;
> return;
> }
> void APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus(void) {
> BOOLEAN bSlotDataAtRigthSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> tMap_BoundPt_t* pSlotBordBk;
> APA_ENUM_TYPE i;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (APAMap_BkDataBfPDCFus.FusPDCStatus == APAMap_FusPDCStatus_Updata) {
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> pSlotBordBk = &APAMap_BkDataBfPDCFus.MapMainSlotBord;
> if ((pSlotBordBk->PtNum > 0) &&
> (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pSlotBordBk->PtNum; i++) {
> pMapMainSlotBord->Property[i] = pSlotBordBk->Property[i];
> pMapMainSlotBord->Points[i] = pSlotBordBk->Points[i];
> }
> pMapMainSlotBord->PtNum = pSlotBordBk->PtNum;
> }
> APAMap_GInfo.SlotPar.Obj1PtIndex = APAMap_BkDataBfPDCFus.Obj1PtIndex;
> APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_BkDataBfPDCFus.SlotStrIndex;
> APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_BkDataBfPDCFus.SlotEndIndex;
> APAMap_GInfo.SlotPar.Obj2PtIndex = APAMap_BkDataBfPDCFus.Obj2PtIndex;
> pSlotBordBk = &APAMap_BkDataBfPDCFus.MapSubSlotBord;
> if ((pSlotBordBk->PtNum > 0) &&
> (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pSlotBordBk->PtNum; i++) {
> pMapSubSlotBord->Property[i] = pSlotBordBk->Property[i];
> pMapSubSlotBord->Points[i] = pSlotBordBk->Points[i];
> }
> pMapSubSlotBord->PtNum = pSlotBordBk->PtNum;
> }
> APAMap_BkDataBfPDCFus.timestamp_ms =
> APAMap_GInputData.CarLocInfo.timeStamp_ms;
> APAMap_BkDataBfPDCFus.OrgSysAtGMap =
> APAMap_GInputData.CarLocInfo.OrgSysAtGMap;
> }
> return;
> }
> void APAMap_ParkingOutUpDataMapBoundaryByPDCInfo(void) {
> APACoordinateDataCalFloatType pMainSlotPDCInfo[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
> APACoordinateDataCalFloatType pSubSlotPDCInfo[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
> uint8_INF pMainSlotPtID[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
> uint8_INF pSubSlotPtID[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
> APA_ENUM_TYPE MainSidePtNum;
> APA_ENUM_TYPE SubSidePtNum;
> if (TRUE == APAMap_ParkingOutCheckIfFusBoundaryPDCInfo()) {
> if (TRUE == APAMap_ParkingOutGetPDCInfoPt(
> &pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0], pMainSlotPtID,
> pSubSlotPtID, &MainSidePtNum, &SubSidePtNum)) {
> } else {
> APAMap_GetBkPDCOutPutData(&pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0],
> pMainSlotPtID, pSubSlotPtID, &MainSidePtNum,
> &SubSidePtNum);
> }
> if ((MainSidePtNum > 0) || (SubSidePtNum > 0)) {
> APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Updata;
> APAMap_SaveBkPDCOutPutData(&pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0],
> pMainSlotPtID, pSubSlotPtID, MainSidePtNum,
> SubSidePtNum);
> APAMap_ParkingOutSaveBkDataBfPDCFus();
> APAMap_ParkingOutFusBoundaryByPDCInfo(
> &pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0], pMainSlotPtID,
> pSubSlotPtID, MainSidePtNum, SubSidePtNum);
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==PDCFusionBorderUpData!==");
> TLOG_INFO << log_string;
> }
> }
> {
> char log_string[512];
> snprintf(
> log_string, sizeof(log_string),
> "==ParkingOutFusPDCStatus:(%d)==BkDataBfPDCFus:Index(%d,%d,%d,%d),"
> "BkBordPtNum(%d,%d)==BkPDCOutPutDataNum:(%d,%d)",
> APAMap_BkDataBfPDCFus.FusPDCStatus, APAMap_BkDataBfPDCFus.SlotStrIndex,
> APAMap_BkDataBfPDCFus.SlotEndIndex, APAMap_BkDataBfPDCFus.Obj1PtIndex,
> APAMap_BkDataBfPDCFus.Obj2PtIndex,
> APAMap_BkDataBfPDCFus.MapMainSlotBord.PtNum,
> APAMap_BkDataBfPDCFus.MapSubSlotBord.PtNum,
> APAMap_BkPDCOutPutData.PDCMainSidePtNum,
> APAMap_BkPDCOutPutData.PDCSubSidePtNum);
> TLOG_INFO << log_string;
> }
> return;
> }
> BOOLEAN APAMap_ParkingOutCheckIfFusBoundaryPDCInfo(void) {
> if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Keep) {
> APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Updata;
> return TRUE;
> } else {
> APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Invalid;
> return FALSE;
> }
> }
> BOOLEAN APAMap_ParkingOutGetPDCInfoPt(
> APACoordinateDataCalFloatType* pPDCMainSlotInfo,
> APACoordinateDataCalFloatType* pPDCSubSlotInfo, uint8_INF* pMainSlotPtID,
> uint8_INF* pSubSlotPtID, APA_ENUM_TYPE* pPDCMainSidePtNum,
> APA_ENUM_TYPE* pPDCSubSidePtNum) {
> #if 1  // #ifdef APAMAP_USE_PDC_OBJS
> #if 0
> APACoordinateDataCalFloatType Obj1PDCPt[5];
> APACoordinateDataCalFloatType Obj2PDCPt[5];
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType Pto;
> BOOLEAN bSlotDataAtRigthSide;
> APA_ENUM_TYPE i;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> for(i = 0; i < 5;i++)
> {
> Obj2PDCPt[i].x = 0;
> Obj2PDCPt[i].y = 0;
> Obj1PDCPt[i].x = 0;
> Obj1PDCPt[i].y = 0;
> }
> *pObj2PtNum = 2;
> *pObj1PtNum = 2;
> Obj2PDCPt[0].x = 3500;
> Obj2PDCPt[0].y = 200;//-100;
> Obj2PDCPt[1].x = 0;
> Obj2PDCPt[1].y = -100;//200;
> Obj1PDCPt[0].x = 0;
> Obj1PDCPt[0].y = -3900;//-4200;
> Obj1PDCPt[1].x = 3500;
> Obj1PDCPt[1].y = -4200;//-3900;
> if(bSlotDataAtRigthSide == FALSE)
> {
> for(i = 0; i < *pObj2PtNum;i++)
> {
> Obj2PDCPt[i].x = -Obj2PDCPt[i].x;
> }
> for(i = 0; i < *pObj1PtNum;i++)
> {
> Obj1PDCPt[i].x = -Obj1PDCPt[i].x;
> }
> }
> for(i = 0; i < *pObj2PtNum;i++)
> {
> pPDCObj2Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj2PDCPt[i],Angle,Pto);
> }
> for(i = 0; i < *pObj1PtNum;i++)
> {
> pPDCObj1Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj1PDCPt[i],Angle,Pto);
> }
> {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==PDCDebugData==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)"
> "==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)",
> *pObj2PtNum,
> pPDCObj2Info[0].x,pPDCObj2Info[0].y,
> pPDCObj2Info[1].x,pPDCObj2Info[1].y,
> pPDCObj2Info[2].x,pPDCObj2Info[2].y,
> pPDCObj2Info[3].x,pPDCObj2Info[3].y,
> pPDCObj2Info[4].x,pPDCObj2Info[4].y,
> *pObj1PtNum,
> pPDCObj1Info[0].x,pPDCObj1Info[0].y,
> pPDCObj1Info[1].x,pPDCObj1Info[1].y,
> pPDCObj1Info[2].x,pPDCObj1Info[2].y,
> pPDCObj1Info[3].x,pPDCObj1Info[3].y,
> pPDCObj1Info[4].x,pPDCObj1Info[4].y);
> TLOG_INFO << log_string;
> }
> return TRUE;
> #endif
> APACoordinateDataCalFloatType LeftSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> APACoordinateDataCalFloatType RightSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> uint8_INF pLeftPtID[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> uint8_INF pRightPtID[MAP_US_OBJ_EXTR_MAX_NUM * 2];
> uint8_INF LeftSegNum;
> uint8_INF RightSegNum;
> // APACarCoordinateDataCalFloatType CurCarPos;
> // CurCarPos = APAMap_GInfo.CarPos;
> APAMap_ParkingOutSiftAndSeqPDCPts(&LeftSeg[0], &RightSeg[0], pLeftPtID,
> pRightPtID, &LeftSegNum, &RightSegNum);
> if (TRUE == APAMap_ParkingOutGetPDCInfoByParkSide(
> &LeftSeg[0], &RightSeg[0], pLeftPtID, pRightPtID, LeftSegNum,
> RightSegNum, pPDCMainSlotInfo, pPDCSubSlotInfo, pMainSlotPtID,
> pSubSlotPtID, pPDCMainSidePtNum, pPDCSubSidePtNum)) {
> return TRUE;
> } else {
> return FALSE;
> }
> #else
> return FALSE;
> #endif
> }
> void APAMap_ParkingOutSiftAndSeqPDCPts(APACoordinateDataCalFloatType* pLeftSeg,
> APACoordinateDataCalFloatType* pRightSeg,
> uint8_INF* pLeftPtID,
> uint8_INF* pRightPtID,
> uint8_INF* pLeftSegNum,
> uint8_INF* pRightSegNum) {
> #ifdef APAMAP_PARKOUT_FUS_PDC
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APACoordinateDataCalFloatType TempSeg[2];
> APACoordinateDataCalFloatType TempPt;
> APA_DISTANCE_TYPE i, j, k;
> APA_ENUM_TYPE ObjLocIndex;
> APA_ENUM_TYPE CurLoc;
> APACoordinateDataCalFloatType* pTarSeg;
> uint8_INF* pTarSegNum;
> // APACoordinateDataCalFloatType pRectPt[4];
> // APALineParameterABCType pRectLine[4];
> // APACarCoordinateDataCalFloatType TempCarPos;
> // APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> st_MapUSS* pPDCInfo;
> uint8_INF PtID;
> uint8_INF* pTargetID;
> APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
> APA_DISTANCE_CAL_FLOAT_TYPE CenterX;
> BOOLEAN bSlotDataAtRigthSide;
> *pLeftSegNum = 0;
> *pRightSegNum = 0;
> pPDCInfo = &APAMap_GInputData.TotalMapInfo.mapData.USSObjInfo;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> #ifdef APAMAP_PARKOUT_PCDEMO_USE_DEFAULT_SDG_OBJS
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> pPDCInfo->ObjNum = 6;
> pPDCInfo->Obj[0].Pt[0].x = 0;
> pPDCInfo->Obj[0].Pt[0].y = 0;
> pPDCInfo->Obj[0].Pt[1].x = -300;
> pPDCInfo->Obj[0].Pt[1].y = 300;
> pPDCInfo->Obj[1].Pt[0].x = -300;
> pPDCInfo->Obj[1].Pt[0].y = 3500;
> pPDCInfo->Obj[1].Pt[1].x = -400;
> pPDCInfo->Obj[1].Pt[1].y = 2500;
> pPDCInfo->Obj[2].Pt[0].x = -6000;
> pPDCInfo->Obj[2].Pt[0].y = 3000;
> pPDCInfo->Obj[2].Pt[1].x = -5500;
> pPDCInfo->Obj[2].Pt[1].y = -1000;
> pPDCInfo->Obj[3].Pt[0].x = -5000;
> pPDCInfo->Obj[3].Pt[0].y = -2000;
> pPDCInfo->Obj[3].Pt[1].x = -6500;
> pPDCInfo->Obj[3].Pt[1].y = -3000;
> pPDCInfo->Obj[4].Pt[0].x = -400;
> pPDCInfo->Obj[4].Pt[0].y = -5000;
> pPDCInfo->Obj[4].Pt[1].x = -300;
> pPDCInfo->Obj[4].Pt[1].y = -4000;
> pPDCInfo->Obj[5].Pt[0].x = -300;
> pPDCInfo->Obj[5].Pt[0].y = -3300;
> pPDCInfo->Obj[5].Pt[1].x = 0;
> pPDCInfo->Obj[5].Pt[1].y = -3000;
> if (bSlotDataAtRigthSide == FALSE) {
> for (i = 0; i < pPDCInfo->ObjNum; i++) {
> pPDCInfo->Obj[i].Pt[0].x = -pPDCInfo->Obj[i].Pt[0].x;
> pPDCInfo->Obj[i].Pt[1].x = -pPDCInfo->Obj[i].Pt[1].x;
> }
> }
> for (i = 0; i < pPDCInfo->ObjNum; i++) {
> TempPt.x = pPDCInfo->Obj[i].Pt[0].x;
> TempPt.y = pPDCInfo->Obj[i].Pt[0].y;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> pPDCInfo->Obj[i].Pt[0].x = TempPt.x;
> pPDCInfo->Obj[i].Pt[0].y = TempPt.y;
> TempPt.x = pPDCInfo->Obj[i].Pt[1].x;
> TempPt.y = pPDCInfo->Obj[i].Pt[1].y;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> pPDCInfo->Obj[i].Pt[1].x = TempPt.x;
> pPDCInfo->Obj[i].Pt[1].y = TempPt.y;
> }
> #else
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return;
> }
> #endif
> if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
> return;
> }
> if ((pPDCInfo->ObjNum <= 0) || (pPDCInfo->ObjNum > MAP_US_OBJ_EXTR_MAX_NUM)) {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==NoPDCPts:(%d)",
> pPDCInfo->ObjNum);
> TLOG_INFO << log_string;
> return;
> }
> if (pPDCInfo->ObjNum > 0) {
> char log_string[1024];
> snprintf(log_string, sizeof(log_string),
> "==OrgPDCPts==(%d):0[(%d,%d),(%d,%d)],1[(%d,%d),(%d,%d)],2[(%d,%d)"
> ",(%d,%d)],"
> "3[(%d,%d),(%d,%d)],4[(%d,%d),(%d,%d)],5[(%d,%d),(%d,%d)],6[(%d,%"
> "d),(%d,%d)],7[(%d,%d),(%d,%d)],"
> "8[(%d,%d),(%d,%d)],9[(%d,%d),(%d,%d)],10[(%d,%d),(%d,%d)],11[(%d,"
> "%d),(%d,%d)],12[(%d,%d),(%d,%d)]",
> pPDCInfo->ObjNum, pPDCInfo->Obj[0].Pt[0].x,
> pPDCInfo->Obj[0].Pt[0].y, pPDCInfo->Obj[0].Pt[1].x,
> pPDCInfo->Obj[0].Pt[1].y, pPDCInfo->Obj[1].Pt[0].x,
> pPDCInfo->Obj[1].Pt[0].y, pPDCInfo->Obj[1].Pt[1].x,
> pPDCInfo->Obj[1].Pt[1].y, pPDCInfo->Obj[2].Pt[0].x,
> pPDCInfo->Obj[2].Pt[0].y, pPDCInfo->Obj[2].Pt[1].x,
> pPDCInfo->Obj[2].Pt[1].y, pPDCInfo->Obj[3].Pt[0].x,
> pPDCInfo->Obj[3].Pt[0].y, pPDCInfo->Obj[3].Pt[1].x,
> pPDCInfo->Obj[3].Pt[1].y, pPDCInfo->Obj[4].Pt[0].x,
> pPDCInfo->Obj[4].Pt[0].y, pPDCInfo->Obj[4].Pt[1].x,
> pPDCInfo->Obj[4].Pt[1].y, pPDCInfo->Obj[5].Pt[0].x,
> pPDCInfo->Obj[5].Pt[0].y, pPDCInfo->Obj[5].Pt[1].x,
> pPDCInfo->Obj[5].Pt[1].y, pPDCInfo->Obj[6].Pt[0].x,
> pPDCInfo->Obj[6].Pt[0].y, pPDCInfo->Obj[6].Pt[1].x,
> pPDCInfo->Obj[6].Pt[1].y, pPDCInfo->Obj[7].Pt[0].x,
> pPDCInfo->Obj[7].Pt[0].y, pPDCInfo->Obj[7].Pt[1].x,
> pPDCInfo->Obj[7].Pt[1].y, pPDCInfo->Obj[8].Pt[0].x,
> pPDCInfo->Obj[8].Pt[0].y, pPDCInfo->Obj[8].Pt[1].x,
> pPDCInfo->Obj[8].Pt[1].y, pPDCInfo->Obj[9].Pt[0].x,
> pPDCInfo->Obj[9].Pt[0].y, pPDCInfo->Obj[9].Pt[1].x,
> pPDCInfo->Obj[9].Pt[1].y, pPDCInfo->Obj[10].Pt[0].x,
> pPDCInfo->Obj[10].Pt[0].y, pPDCInfo->Obj[10].Pt[1].x,
> pPDCInfo->Obj[10].Pt[1].y, pPDCInfo->Obj[11].Pt[0].x,
> pPDCInfo->Obj[11].Pt[0].y, pPDCInfo->Obj[11].Pt[1].x,
> pPDCInfo->Obj[11].Pt[1].y, pPDCInfo->Obj[12].Pt[0].x,
> pPDCInfo->Obj[12].Pt[0].y, pPDCInfo->Obj[12].Pt[1].x,
> pPDCInfo->Obj[12].Pt[1].y);
> TLOG_INFO << log_string;
> }
> // TempCarPos.Coordinate.x = 0;
> // TempCarPos.Coordinate.y = 0;
> // TempCarPos.CarAng = 0;
> // APAMap_GetCarRectArea(100,100,100,100,TempCarPos,&pRectPt[0],&pRectLine[0]);
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (bSlotDataAtRigthSide == TRUE) {
> CenterX = -3000;
> } else {
> CenterX = 3000;
> }
> for (i = 0; i < pPDCInfo->ObjNum; i++) {
> ObjLocIndex = -1;
> for (j = 0; j < 2; j++) {
> TempSeg[j].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pPDCInfo->Obj[i].Pt[j].x;
> TempSeg[j].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pPDCInfo->Obj[i].Pt[j].y;
> TempSeg[j] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> TempSeg[j], 0, Angle, Pto);
> if (TempSeg[j].x < CenterX) {
> CurLoc = 0;  // Left;
> } else {
> CurLoc = 1;  // Right;
> }
> if ((ObjLocIndex == -1) || (ObjLocIndex == CurLoc)) {
> ObjLocIndex = CurLoc;
> } else {
> if (TempSeg[j].y > 0) {
> ObjLocIndex = 2;  // Front;
> } else {
> ObjLocIndex = 3;  // Back;
> }
> }
> }
> {
> if (TempSeg[1].y > TempSeg[0].y) {
> TempPt = TempSeg[0];
> TempSeg[0] = TempSeg[1];
> TempSeg[1] = TempPt;
> }
> if (ObjLocIndex == 0) {
> pTarSeg = &pLeftSeg[0];
> pTarSegNum = pLeftSegNum;
> } else if (ObjLocIndex == 1) {
> pTarSeg = &pRightSeg[0];
> pTarSegNum = pRightSegNum;
> } else {
> pTarSeg = NULL;
> pTarSegNum = NULL;
> }
> if (pTarSeg != NULL) {
> j = 0;
> while (j < *pTarSegNum) {
> if (TempSeg[0].y > pTarSeg[j].y) {
> break;
> }
> j += 2;
> }
> k = *pTarSegNum - 1;
> if ((*pTarSegNum + 2) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
> // buff is not big enough
> return;
> }
> while (k >= j) {
> pTarSeg[k + 2] = pTarSeg[k];
> k--;
> }
> pTarSeg[j] = TempSeg[0];
> pTarSeg[j + 1] = TempSeg[1];
> *pTarSegNum += 2;
> }
> }
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==SiftAndSeqPDCPts:Org(%d,%d)",
> *pLeftSegNum, *pRightSegNum);
> TLOG_INFO << log_string;
> }
> for (j = 0; j < 2; j++) {
> if (j == 0) {
> pTarSeg = &pLeftSeg[0];
> pTarSegNum = pLeftSegNum;
> pTargetID = pLeftPtID;
> } else {
> pTarSeg = &pRightSeg[0];
> pTarSegNum = pRightSegNum;
> pTargetID = pRightPtID;
> }
> PtID = 1;
> i = 0;
> while (i < *pTarSegNum) {
> if (i > 1) {
> TempDis = AlgCom_GetTwoPointDisFloat(pTarSeg[i - 1], pTarSeg[i]);
> if (TempDis > 500) {
> PtID++;
> }
> }
> pTargetID[i] = PtID;
> pTargetID[i + 1] = PtID;
> i += 2;
> };
> }
> #else
> *pLeftSegNum = 0;
> *pRightSegNum = 0;
> #endif
> return;
> }
> BOOLEAN APAMap_ParkingOutGetPDCInfoByParkSide(
> APACoordinateDataCalFloatType* pLeftSeg,
> APACoordinateDataCalFloatType* pRightSeg, uint8_INF* pLeftPtID,
> uint8_INF* pRightPtID, uint8_INF u8LeftSegNum, uint8_INF u8RightSegNum,
> APACoordinateDataCalFloatType* pMainSlotPDCInfo,
> APACoordinateDataCalFloatType* pSubSlotPDCInfo, uint8_INF* pMainSlotPtID,
> uint8_INF* pSubSlotPtID, APA_ENUM_TYPE* pMainSlotPDCPtNum,
> APA_ENUM_TYPE* pSubSlotPDCPtNum) {
> APA_ENUM_TYPE i, k;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType* pMainSlotSrc;
> APACoordinateDataCalFloatType* pSubSlotSrc;
> uint8_t_INF* pMainSlotPtIDSrc;
> uint8_t_INF* pSubSlotPtIDSrc;
> APA_ENUM_TYPE MainSidePtNum;
> APA_ENUM_TYPE SubSidePtNum;
> APA_ENUM_TYPE MainSideStrIndex;
> APA_ENUM_TYPE SubSideStrIndex;
> APA_ENUM_TYPE SubSideSearchStep;
> APA_ENUM_TYPE MainSideSearchStep;
> APA_ENUM_TYPE LeftSegNum;
> APA_ENUM_TYPE RightSegNum;
> APACoordinateDataCalFloatType TempPt;
> *pMainSlotPDCPtNum = 0;
> *pSubSlotPDCPtNum = 0;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> if (u8LeftSegNum > APA_MAP_BK_PDC_OUTPUT_MAX_NUM) {
> LeftSegNum = 0;
> } else {
> LeftSegNum = (APA_ENUM_TYPE)u8LeftSegNum;
> }
> if (u8RightSegNum > APA_MAP_BK_PDC_OUTPUT_MAX_NUM) {
> RightSegNum = 0;
> } else {
> RightSegNum = (APA_ENUM_TYPE)u8RightSegNum;
> }
> // AlgCom_SmoothSegMent(TRUE,1000,10,pLeftSeg,&LeftSegNum);
> // AlgCom_SmoothSegMent(FALSE,1000,10,pRightSeg,&RightSegNum);
> if (LeftSegNum > 0) {
> char log_string[1024];
> snprintf(
> log_string, sizeof(log_string),
> "==GetPDCInfoByParkMode==Left(%d):0(%.2f,%.2f,%d),1(%.2f,%.2f,%d),2(%."
> "2f,%.2f,%d),3(%.2f,%.2f,%d),4(%.2f,%.2f,%d),"
> "5(%.2f,%.2f,%d),6(%.2f,%.2f,%d),7(%.2f,%.2f,%d),8(%.2f,%.2f,%d),9(%."
> "2f,%.2f,%d),10(%.2f,%.2f,%d),11(%.2f,%.2f,%d),"
> "12(%.2f,%.2f,%d),13(%.2f,%.2f,%d),14(%.2f,%.2f,%d),15(%.2f,%.2f,%d),"
> "16(%.2f,%.2f,%d),17(%.2f,%.2f,%d),18(%.2f,%.2f,%d),19(%.2f,%.2f,%d)",
> LeftSegNum, pLeftSeg[0].x, pLeftSeg[0].y, pLeftPtID[0], pLeftSeg[1].x,
> pLeftSeg[1].y, pLeftPtID[1], pLeftSeg[2].x, pLeftSeg[2].y, pLeftPtID[2],
> pLeftSeg[3].x, pLeftSeg[3].y, pLeftPtID[3], pLeftSeg[4].x,
> pLeftSeg[4].y, pLeftPtID[4], pLeftSeg[5].x, pLeftSeg[5].y, pLeftPtID[5],
> pLeftSeg[6].x, pLeftSeg[6].y, pLeftPtID[6], pLeftSeg[7].x,
> pLeftSeg[7].y, pLeftPtID[7], pLeftSeg[8].x, pLeftSeg[8].y, pLeftPtID[8],
> pLeftSeg[9].x, pLeftSeg[9].y, pLeftPtID[9], pLeftSeg[10].x,
> pLeftSeg[10].y, pLeftPtID[10], pLeftSeg[11].x, pLeftSeg[11].y,
> pLeftPtID[11], pLeftSeg[12].x, pLeftSeg[12].y, pLeftPtID[12],
> pLeftSeg[13].x, pLeftSeg[13].y, pLeftPtID[13], pLeftSeg[14].x,
> pLeftSeg[14].y, pLeftPtID[14], pLeftSeg[15].x, pLeftSeg[15].y,
> pLeftPtID[15], pLeftSeg[16].x, pLeftSeg[16].y, pLeftPtID[16],
> pLeftSeg[17].x, pLeftSeg[17].y, pLeftPtID[17], pLeftSeg[18].x,
> pLeftSeg[18].y, pLeftPtID[18], pLeftSeg[19].x, pLeftSeg[19].y,
> pLeftPtID[19]);
> TLOG_INFO << log_string;
> }
> if (RightSegNum > 0) {
> char log_string[1024];
> snprintf(
> log_string, sizeof(log_string),
> "==GetPDCInfoByParkMode==Right(%d):0(%.2f,%.2f,%d),1(%.2f,%.2f,%d),2(%."
> "2f,%.2f,%d),3(%.2f,%.2f,%d),4(%.2f,%.2f,%d),"
> "5(%.2f,%.2f,%d),6(%.2f,%.2f,%d),7(%.2f,%.2f,%d),8(%.2f,%.2f,%d),9(%."
> "2f,%.2f,%d),10(%.2f,%.2f,%d),11(%.2f,%.2f,%d),"
> "12(%.2f,%.2f,%d),13(%.2f,%.2f,%d),14(%.2f,%.2f,%d),15(%.2f,%.2f,%d),"
> "16(%.2f,%.2f,%d),17(%.2f,%.2f,%d),18(%.2f,%.2f,%d),19(%.2f,%.2f,%d)",
> RightSegNum, pRightSeg[0].x, pRightSeg[0].y, pRightPtID[0],
> pRightSeg[1].x, pRightSeg[1].y, pRightPtID[1], pRightSeg[2].x,
> pRightSeg[2].y, pRightPtID[2], pRightSeg[3].x, pRightSeg[3].y,
> pRightPtID[3], pRightSeg[4].x, pRightSeg[4].y, pRightPtID[4],
> pRightSeg[5].x, pRightSeg[5].y, pRightPtID[5], pRightSeg[6].x,
> pRightSeg[6].y, pRightPtID[6], pRightSeg[7].x, pRightSeg[7].y,
> pRightPtID[7], pRightSeg[8].x, pRightSeg[8].y, pRightPtID[8],
> pRightSeg[9].x, pRightSeg[9].y, pRightPtID[9], pRightSeg[10].x,
> pRightSeg[10].y, pRightPtID[10], pRightSeg[11].x, pRightSeg[11].y,
> pRightPtID[11], pRightSeg[12].x, pRightSeg[12].y, pRightPtID[12],
> pRightSeg[13].x, pRightSeg[13].y, pRightPtID[13], pRightSeg[14].x,
> pRightSeg[14].y, pRightPtID[14], pRightSeg[15].x, pRightSeg[15].y,
> pRightPtID[15], pRightSeg[16].x, pRightSeg[16].y, pRightPtID[16],
> pRightSeg[17].x, pRightSeg[17].y, pRightPtID[17], pRightSeg[18].x,
> pRightSeg[18].y, pRightPtID[18], pRightSeg[19].x, pRightSeg[19].y,
> pRightPtID[19]);
> TLOG_INFO << log_string;
> }
> if ((LeftSegNum == 0) && (RightSegNum == 0)) {
> return FALSE;
> }
> if (bSlotDataAtRigthSide == TRUE) {
> pMainSlotSrc = pRightSeg;
> MainSidePtNum = RightSegNum;
> pSubSlotSrc = pLeftSeg;
> SubSidePtNum = LeftSegNum;
> pMainSlotPtIDSrc = pRightPtID;
> pSubSlotPtIDSrc = pLeftPtID;
> } else {
> pMainSlotSrc = pLeftSeg;
> MainSidePtNum = LeftSegNum;
> pSubSlotSrc = pRightSeg;
> SubSidePtNum = RightSegNum;
> pMainSlotPtIDSrc = pLeftPtID;
> pSubSlotPtIDSrc = pRightPtID;
> }
> MainSideStrIndex = MainSidePtNum - 1;
> MainSideSearchStep = -1;
> SubSideStrIndex = SubSidePtNum - 1;
> SubSideSearchStep = -1;
> i = MainSideStrIndex;
> k = 0;
> while (k < MainSidePtNum) {
> pMainSlotPtID[k] = pMainSlotPtIDSrc[i];
> TempPt = pMainSlotSrc[i];
> pMainSlotPDCInfo[k] = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> k++;
> i += MainSideSearchStep;
> }
> *pMainSlotPDCPtNum = MainSidePtNum;
> i = SubSideStrIndex;
> k = 0;
> while (k < SubSidePtNum) {
> pSubSlotPtID[k] = pSubSlotPtIDSrc[i];
> TempPt = pSubSlotSrc[i];
> pSubSlotPDCInfo[k] = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
> k++;
> i += SubSideSearchStep;
> }
> *pSubSlotPDCPtNum = SubSidePtNum;
> return TRUE;
> }
> void APAMap_ParkingOutSaveBkDataBfPDCFus(void) {
> BOOLEAN bSlotDataAtRigthSide;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> tMap_BoundPt_t* pSlotBordBk;
> APA_ENUM_TYPE i;
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (APAMap_BkDataBfPDCFus.FusPDCStatus == APAMap_FusPDCStatus_Updata) {
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> pSlotBordBk = &APAMap_BkDataBfPDCFus.MapMainSlotBord;
> if ((pMapMainSlotBord->PtNum > 0) &&
> (pMapMainSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pMapMainSlotBord->PtNum; i++) {
> pSlotBordBk->Property[i] = pMapMainSlotBord->Property[i];
> pSlotBordBk->Points[i] = pMapMainSlotBord->Points[i];
> }
> pSlotBordBk->PtNum = pMapMainSlotBord->PtNum;
> }
> APAMap_BkDataBfPDCFus.Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> APAMap_BkDataBfPDCFus.SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> APAMap_BkDataBfPDCFus.SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> APAMap_BkDataBfPDCFus.Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> pSlotBordBk = &APAMap_BkDataBfPDCFus.MapSubSlotBord;
> if ((pMapSubSlotBord->PtNum > 0) &&
> (pMapSubSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
> for (i = 0; i < pMapSubSlotBord->PtNum; i++) {
> pSlotBordBk->Property[i] = pMapSubSlotBord->Property[i];
> pSlotBordBk->Points[i] = pMapSubSlotBord->Points[i];
> }
> pSlotBordBk->PtNum = pMapSubSlotBord->PtNum;
> }
> }
> return;
> }
> BOOLEAN APAMap_ParkingOutFusBoundaryByPDCInfo(
> APACoordinateDataCalFloatType* pMainSlotPDCInfo,
> APACoordinateDataCalFloatType* pSubSlotPDCInfo, uint8_INF* pMainSlotPtID,
> uint8_INF* pSubSlotPtID, APA_ENUM_TYPE MainSlotPDCPtNum,
> APA_ENUM_TYPE SubSlotPDCPtNum) {
> APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
> APACarCoordinateDataCalFloatType CurCarPos;
> APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
> APACoordinateDataCalFloatType Pto;
> APA_DISTANCE_CAL_FLOAT_TYPE Angle;
> APA_ENUM_TYPE Index;
> APA_ENUM_TYPE i, k;
> tMap_BoundPt_t* pMapMainSlotBord;
> tMap_BoundPt_t* pMapSubSlotBord;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
> APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
> BOOLEAN bSlotDataAtRigthSide;
> APACoordinateDataCalFloatType TempPt;
> APACoordinateDataCalFloatType Data[127];
> APACoordinateDataCalFloatType pData1[127];
> APACoordinateDataCalFloatType pData2[127];
> APACoordinateDataCalFloatType pData3[127];
> APACoordinateDataCalFloatType pData4[127];
> APACoordinateDataCalFloatType NSegment[127];
> uint8_t_INF pPtStyle[127];
> uint8_t_INF NewProperty1[127];
> uint8_t_INF NewProperty2[127];
> uint8_t_INF NewProperty3[127];
> uint8_t_INF NewProperty4[127];
> uint8_t_INF NSegProperty[127];
> APA_ENUM_TYPE NSegNum;
> uint16_t_INF DataNum;
> APA_ENUM_TYPE Data1Num;
> APA_ENUM_TYPE Data2Num;
> APA_ENUM_TYPE Data3Num;
> APA_ENUM_TYPE Data4Num;
> APA_ENUM_TYPE LocStyle;
> APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
> APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
> APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
> APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
> APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
> // APA_DISTANCE_CAL_FLOAT_TYPE LineYAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
> APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
> APA_DISTANCE_CAL_FLOAT_TYPE fDis;
> APACoordinateDataCalFloatType pRectPt[4];
> APALineParameterABCType pRectLine[4];
> APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
> APALineParameterABCType TempLine1;
> APALineParameterABCType TempLine2;
> APACarCoordinateDataCalFloatType TempCarPos;
> APACoordinateDataCalFloatType TempPt1, TempPt2;
> APA_ENUM_TYPE OffsetIndex2, OffsetIndex1;
> APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
> APACoordinateDataCalFloatType pDataBk[127];
> APA_ENUM_TYPE DataNumBk;
> BOOLEAN bCheckSubLane;
> BOOLEAN bFusvalid;
> uint8_t_INF CurID;
> if ((MainSlotPDCPtNum <= 0) && (SubSlotPDCPtNum <= 0)) {
> return FALSE;
> }
> bSlotDataAtRigthSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
> if (bSlotDataAtRigthSide == FALSE) {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> } else {
> pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
> pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
> }
> OrgObj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];
> OrgObj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];
> CurCarPos = APAMap_GInfo.CarPos;
> Pto = APAMap_GInfo.NewCordSysOPt;
> Angle = APAMap_GInfo.NewCordSysAng;
> TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&TempAng);
> TempAng1 = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
> AlgCom_AngNormalized(&TempAng1);
> if (bSlotDataAtRigthSide == FALSE) {
> fDis = TempAng;
> TempAng = TempAng1;
> TempAng1 = fDis;
> }
> SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
> SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
> Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
> Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
> OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
> OffsetIndex2 = Obj2PtIndex - SlotEndIndex;
> MaxOffsetX = 1500;
> if (bSlotDataAtRigthSide == TRUE) {
> MaxOffsetX = -MaxOffsetX;
> }
> MainLinYStrPt.x = MaxOffsetX;
> MainLinYStrPt.y = 0;
> MainLinYEndPt.x = MainLinYStrPt.x;
> MainLinYEndPt.y = 1000;
> MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
> MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
> // LineYAngle = Angle;
> // obj 1 border line
> MainLinXStrPt1 = OrgObj1Pt;
> MainLinXEndPt1 = OrgObj1Pt;
> LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
> MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
> Data1Num = 0;
> for (Index = 0; Index <= Obj1PtIndex; Index++) {
> pData1[Data1Num] = pMapMainSlotBord->Points[Index];
> NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
> Data1Num++;
> if (Data1Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDC Buffer Not enough==1==Data1Num:(%d)", Data1Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // obj2 borderline;
> MainLinXStrPt2 = OrgObj2Pt;
> MainLinXEndPt2 = OrgObj2Pt;
> LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
> MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
> MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
> Data2Num = 0;
> for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
> pData2[Data2Num] = pMapMainSlotBord->Points[Index];
> NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
> Data2Num++;
> if (Data2Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDC Buffer Not enough==2==Data2Num:(%d)", Data2Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // data pt in slot;
> Data3Num = 0;
> for (Index = Obj1PtIndex + 1; Index < Obj2PtIndex; Index++) {
> pData3[Data3Num] = pMapMainSlotBord->Points[Index];
> NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
> Data3Num++;
> if (Data3Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDC Buffer Not enough==3==Data3Num:(%d)", Data3Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> // Fus  Subborder
> MaxOffsetX = 3000;
> if (APAMap_GInputData.ParkReqPar.parkmode ==
> APA_PARKPROC_PARKING_MODE_PARALLEL) {
> MaxOffsetX = 2500;
> } else {
> MaxOffsetX = 3000;
> }
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt.x = TempPt.x;
> SubLinYStrPt.y = 0;
> SubLinYEndPt.x = SubLinYStrPt.x;
> SubLinYEndPt.y = 1000;
> SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
> SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
> // Fus  Subborder
> MaxOffsetX = 3000;
> if (bSlotDataAtRigthSide == TRUE) {
> TempPt.x = -MaxOffsetX;
> } else {
> TempPt.x = MaxOffsetX;
> }
> SubLinYStrPt1.x = TempPt.x;
> SubLinYStrPt1.y = 0;
> SubLinYEndPt1.x = SubLinYStrPt1.x;
> SubLinYEndPt1.y = 1000;
> SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
> SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);
> LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
> AlgCom_AngNormalized(&LineXAngle);
> TempCarPos.CarAng = LineXAngle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
> if (TempLine1.C < TempLine2.C) {
> TempLine1.C = TempLine2.C;
> }
> TempPt.y =
> (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
> TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
> TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
> CurCarPos.Coordinate);
> AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
> if (TempLine1.C < TempLine2.C) {
> TempLine1.C = TempLine2.C;
> }
> TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(1000, &TempLine1);
> TempCarPos.CarAng = Angle;
> TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
> TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
> AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
> SubLinXEndPt = SubLinXStrPt;
> SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
> SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);
> Data4Num = 0;
> for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
> pData4[Data4Num] = pMapSubSlotBord->Points[Index];
> NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
> Data4Num++;
> if (Data4Num > 127) {
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDC Buffer Not enough==4==Data4Num:(%d)", Data3Num);
> TLOG_INFO << log_string;
> }
> return FALSE;
> }
> }
> DataNumBk = 0;
> bCheckSubLane = FALSE;
> if (((0 < APAMAP_GetBindAlleySlotCase()) &&
> (3 > APAMAP_GetBindAlleySlotCase())) ||
> (APAMap_GInfo.bAddFrontFSDPtToMapBord == TRUE)) {
> bCheckSubLane = TRUE;
> }
> NSegNum = 0;
> APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
> if (MainSlotPDCPtNum > 1) {
> // get fsd data with same id;
> i = 0;
> while (i < MainSlotPDCPtNum) {
> // get fsd data with same id;
> CurID = pMainSlotPtID[i];
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i].x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i].y;
> for (k = 1; k < 100; k++) {
> if ((i + k) < MainSlotPDCPtNum) {
> if (pMainSlotPtID[i + k] != CurID) {
> break;
> } else {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i + k].x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i + k].y;
> }
> } else {
> break;
> }
> }
> DataNum = k;
> i += DataNum;
> // Get valid fsd data for fus obj1bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> AlgCom_SmoothSegMent(bSlotDataAtRigthSide, 500, 10, &NSegment[0],
> &NSegNum);
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, TempAng, &pData1[0], Data1Num,
> &NewProperty1[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData1[0], &Data1Num, &pPtStyle[0])) {
> // updata obj1 bordline;
> for (k = 0; k < Data1Num; k++) {
> NewProperty1[k] = pPtStyle[k];
> }
> if (Data1Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData1[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data1Num - 1; k++) {
> NewProperty1[k] = NewProperty1[k + 1];
> pData1[k] = pData1[k + 1];
> }
> Data1Num--;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==PDCFusionObj1Success==");
> TLOG_INFO << log_string;
> }
> //----------------------------------
> // Get valid fsd data for fus obj2bordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
> if (((LocStyle != 0) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 1) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> #ifdef SUPPORT_BLIND_ALLEY_SLOT
> if ((0 != APAMAP_GetBindAlleySlotCase()) ||
> (APAMap_GInfo.bAddFrontFSDPtToMapBord == TRUE)) {
> NSegNum = 0;
> }
> #endif
> APAMap_ReOderSegmentPt(TRUE, bSlotDataAtRigthSide, &NSegment[0], &NSegNum,
> Pto, Angle);
> AlgCom_SmoothSegMent(bSlotDataAtRigthSide, 500, 10, &NSegment[0],
> &NSegNum);
> if (TRUE == APAMap_FusTwoLineSegments(
> bSlotDataAtRigthSide, TempAng, &pData2[0], Data2Num,
> &NewProperty2[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData2[0], &Data2Num, &pPtStyle[0])) {
> // updata obj2 bordline;
> for (k = 0; k < Data2Num; k++) {
> NewProperty2[k] = pPtStyle[k];
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==PDCFusionObj2Success==");
> TLOG_INFO << log_string;
> }
> }
> }
> if (SubSlotPDCPtNum > 1) {
> i = 0;
> while (i < SubSlotPDCPtNum) {
> // get fsd data with same id;
> CurID = pSubSlotPtID[i];
> Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i].x;
> Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i].y;
> for (k = 1; k < 100; k++) {
> if ((i + k) < SubSlotPDCPtNum) {
> if (pSubSlotPtID[i + k] != CurID) {
> break;
> } else {
> Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i + k].x;
> Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i + k].y;
> }
> } else {
> break;
> }
> }
> DataNum = k;
> i += DataNum;
> //----------------------------------
> // Get valid PDC data for fus Subbordline;
> NSegNum = 0;
> for (k = 0; k < DataNum; k++) {
> TempPt = Data[k];
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
> if (LocStyle != 0) {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> } else {
> LocStyle = AlgCom_GetPointLocationAccordGivenVector(
> &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
> if (((LocStyle != 1) && (bSlotDataAtRigthSide == TRUE)) ||
> ((LocStyle != 0) && (bSlotDataAtRigthSide == FALSE))) {
> NSegment[NSegNum] = TempPt;
> NSegNum++;
> }
> }
> }
> for (k = 0; k < NSegNum; k++) {
> NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
> }
> APAMap_ReOderSegmentPt(TRUE, !bSlotDataAtRigthSide, &NSegment[0],
> &NSegNum, Pto, Angle);
> AlgCom_SmoothSegMent(!bSlotDataAtRigthSide, 500, 10, &NSegment[0],
> &NSegNum);
> if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
> pRectPt, pRectLine)) {
> NSegNum = 0;
> }
> if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
> for (k = 0; k < Data4Num; k++) {
> pDataBk[k] = pData4[k];
> }
> DataNumBk = Data4Num;
> }
> bFusvalid = FALSE;
> if (TRUE == APAMap_FusTwoLineSegments(
> !bSlotDataAtRigthSide, TempAng1, &pData4[0], Data4Num,
> &NewProperty4[0], &NSegment[0], NSegNum, &NSegProperty[0],
> &pData4[0], &Data4Num, &pPtStyle[0])) {
> if (bCheckSubLane == TRUE) {
> if (TRUE == APAMap_CheckIfObjWithinRectArea(
> 0x00, &pData4[0], Data4Num, pRectPt, pRectLine)) {
> for (k = 0; k < DataNumBk; k++) {
> pData4[k] = pDataBk[k];
> }
> Data4Num = DataNumBk;
> } else {
> bFusvalid = TRUE;
> }
> } else {
> bFusvalid = TRUE;
> }
> }
> if (bFusvalid == TRUE) {
> // updata sublane;
> for (k = 0; k < Data4Num; k++) {
> NewProperty4[k] = pPtStyle[k];
> }
> if (Data4Num > 2) {
> TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[0], 0, Angle, Pto);
> TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
> pData4[1], 0, Angle, Pto);
> if ((TempPt1.y >= TempPt2.y) &&
> (((TempPt1.x <= TempPt2.x) && (bSlotDataAtRigthSide == TRUE)) ||
> ((TempPt1.x >= TempPt2.x) && (bSlotDataAtRigthSide == FALSE)))) {
> TempPt1.x = TempPt2.x;
> pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
> for (k = 1; k < Data4Num - 1; k++) {
> NewProperty4[k] = NewProperty4[k + 1];
> pData4[k] = pData4[k + 1];
> }
> Data4Num--;
> }
> }
> char log_string[512];
> snprintf(log_string, sizeof(log_string), "==PDCFusionSubLaneSuccess==");
> TLOG_INFO << log_string;
> }
> }
> }
> DataNum = Data1Num + Data3Num + Data2Num;
> if (DataNum <= BOUNDARY_PT_MAX_NUM) {
> for (Index = 0; Index < DataNum; Index++) {
> if (Index < Data1Num) {
> pMapMainSlotBord->Points[Index] = pData1[Index];
> pMapMainSlotBord->Property[Index] = NewProperty1[Index];
> } else if (Index < Data1Num + Data3Num) {
> pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
> pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];
> } else {
> pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
> pMapMainSlotBord->Property[Index] =
> NewProperty2[Index - Data1Num - Data3Num];
> }
> }
> pMapMainSlotBord->PtNum = DataNum;
> APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
> APAMap_GInfo.SlotPar.SlotStrIndex =
> APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
> APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
> APAMap_GInfo.SlotPar.SlotEndIndex =
> APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDCFusionMainSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> if (Data4Num <= BOUNDARY_PT_MAX_NUM) {
> for (Index = 0; Index < Data4Num; Index++) {
> pMapSubSlotBord->Points[Index] = pData4[Index];
> pMapSubSlotBord->Property[Index] = NewProperty4[Index];
> }
> pMapSubSlotBord->PtNum = Data4Num;
> } else {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==PDCFusionSubSlotFail For Buffer Not enough==");
> TLOG_INFO << log_string;
> }
> {
> char log_string[512];
> snprintf(log_string, sizeof(log_string),
> "==FusBordByPDC==SlotIndex(%d,%d,%d,%d)==Offset(%d,%d))",
> APAMap_GInfo.SlotPar.Obj1PtIndex,
> APAMap_GInfo.SlotPar.SlotStrIndex,
> APAMap_GInfo.SlotPar.SlotEndIndex,
> APAMap_GInfo.SlotPar.Obj2PtIndex, OffsetIndex1, OffsetIndex2);
> TLOG_INFO << log_string;
> }
> return TRUE;
> }
> #endif
> #endif

#### _Assistant_

> 这段代码本质上是一个**APA 自动泊车中的“泊出地图构建/更新模块”**，主要职责是：
> 
> 1. 根据当前泊出请求、车辆位置、车位信息，先确定“目标车位”；
> 2. 计算车位边界点（Obj1/Obj2、SlotBordPt）；
> 3. 生成泊出通道的左右边界；
> 4. 再融合多种传感器/地图源：
>    - FSD 顶视图
>    - OD 障碍物
>    - 车道线 LaneLine
>    - 车位参考线 RefercLine
>    - 超声/SDG/PDC
> 5. 动态调整**终点车位姿 EndPos**；
> 6. 输出一个供规划使用的“可行泊出地图”。
> 
> ---
> 
> # 一、这段代码在做什么
> 
> 从文件名和函数名看：
> 
> - `MapParkingOut.h`
> - `APAMap_ParkingOutTask()`
> - `APAMap_ParkingOutCalMapInfo()`
> 
> 这说明它是**泊出（Parking Out）地图生成任务**。
> 
> 它不是轨迹规划器本身，而更像是**规划前的环境建图/车位建模模块**。
> 
> 规划器真正关心的是：
> 
> - 车位在哪
> - 车位边界长什么样
> - 通道左右边界怎么走
> - 终点车位姿在哪里
> - 周围障碍是否侵占通道或终点位姿
> 
> 这段代码就是负责把这些算出来。
> 
> ---
> 
> # 二、先看整体主流程
> 
> 核心入口函数：
> 
> ```cpp
> void APAMap_ParkingOutTask()
> ```
> 
> 它的逻辑可以概括成：
> 
> ## 1）是否是“更新帧”
> 如果满足：
> 
> - `APARunningstate >= 4`
> - `Request_cmd == 1`
> - `request_cnt == lastreqcnt`
> 
> 则认为不是第一次建图，而是**同一请求的更新帧**，只做：
> 
> ```cpp
> APAMap_ParkingOutUpDataMapInfo();
> return;
> ```
> 
> 也就是：**不重建车位，只更新地图边界和终点信息**。
> 
> ---
> 
> ## 2）是否是“首次建图”
> 如果 `Request_cmd == 1 || Request_cmd == 6`，则进入首次建图流程：
> 
> ### 第一步：初始化/调试标志复位
> ```cpp
> APAMap_ParkingOutDebugInit();
> ```
> 
> ### 第二步：算车位参数
> ```cpp
> bResult = APAMap_ParkingOutCalMapSlotPar();
> ```
> 
> ### 第三步：算车位边界点
> ```cpp
> bResult = APAMap_ParkingOutCalSlotInfo();
> ```
> 
> ### 第四步：算地图边界
> ```cpp
> bResult = APAMap_ParkingOutCalMapInfo();
> ```
> 
> ### 第五步：检查当前车姿是否合法
> ```cpp
> bResult = APAMap_ParkingOutCheckIfCarPosIsValid();
> ```
> 
> 如果任何一步失败，就设置 fail cause。
> 
> ---
> 
> ## 3）如果不是首次建图
> 则要求当前左右边界点数都足够，否则失败：
> 
> ```cpp
> if (LeftBoundary.PtNum < 2 || RightBoundary.PtNum < 2)
> ```
> 
> 然后只更新地图：
> 
> ```cpp
> APAMap_ParkingOutUpDataMapInfo();
> ```
> 
> ---
> 
> # 三、这个模块的核心数据模型
> 
> 虽然你没贴结构体定义，但从使用上能推断出一些重要成员。
> 
> ---
> 
> ## 1）输入：`APAMap_GInputData`
> 
> 这是整个模块的输入总线，主要包含：
> 
> ### 泊车请求参数
> ```cpp
> APAMap_GInputData.ParkReqPar
> ```
> 
> 里面有：
> 
> - `parkmode`
> - `parkoutmode`
> - `parkside`
> - `Request_cmd`
> - `Request_SlotId`
> - `Request_SlotFusionMode`
> - `APARunningstate`
> - `APAstate`
> - `request_cnt`
> - `SlotDataIsNotMirrored`
> - `Parkout_UWBPos`
> 
> ---
> 
> ### 车辆位置
> ```cpp
> APAMap_GInputData.CarLocInfo.CarPos
> ```
> 
> 包括：
> 
> - `Coordinate.x / y`
> - `CarAng`
> 
> ---
> 
> ### 各类地图/感知源
> - `TotalMapInfo.mapData.FSDInfo.TopView`
> - `TotalMapInfo.mapData.ODInfo`
> - `TotalMapInfo.mapData.LaneLineInfo`
> - `TotalMapInfo.mapData.RefercLineInfo`
> - `TotalMapInfo.mapData.USSObjInfo`
> - `PAobjInfo`
> - `Usslot / Vplslot / FusSlot`
> 
> ---
> 
> ## 2）内部状态：`APAMap_GInfo`
> 
> 这是计算结果和中间态，核心有：
> 
> ### 车位参数 `SlotPar`
> 例如：
> 
> - `SlotID`
> - `SlotSide`
> - `SlotIndex`
> - `FusSlotIndex`
> - `SlotBordPt[4]`
> - `Obj1Pt / Obj2Pt`
> - `Obj1Ang / Obj2Ang`
> - `SlotLen`
> - `SlotDepth`
> - `EndPos`
> - `EndPosLine`
> - `Obj1PtIndex / Obj2PtIndex`
> - `SlotStrIndex / SlotEndIndex`
> 
> ---
> 
> ### 输出边界 `OutLine`
> - `LeftBoundary`
> - `RightBoundary`
> 
> ---
> 
> ### 坐标系信息
> - `NewCordSysOPt`
> - `NewCordSysAng`
> 
> 这个非常关键：说明模块内部会构造一个**新的锚点坐标系**来做车位/通道建模。
> 
> ---
> 
> # 四、最关键的几个概念
> 
> ---
> 
> ## 1）Obj1 / Obj2 是什么
> 
> 代码里经常出现：
> 
> - `Obj1Pt`
> - `Obj2Pt`
> - `Obj1Ang`
> - `Obj2Ang`
> 
> 从逻辑看，它们代表**车位两侧的边界参考点**，通常可以理解成：
> 
> - Obj1：靠一侧的停车边界点
> - Obj2：另一侧的停车边界点
> 
> 然后通过它们的角度与位置，构造车位边界线和主边界。
> 
> ---
> 
> ## 2）SlotBordPt[0..3] 是什么
> 
> 它看起来像车位框关键点：
> 
> - `[0] / [1]` 通常是两个主要边界点
> - `[2] / [3]` 是边界延伸或内边界点
> 
> 代码里：
> ```cpp
> APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
> APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
> ```
> 
> 后面又把 `[2] / [3]` 设为主边界中的关键点。
> 
> 所以它是**车位边界几何的核心点集**。
> 
> ---
> 
> ## 3）EndPos 是什么
> 
> ```cpp
> APAMap_GInfo.SlotPar.EndPos
> ```
> 
> 这是**泊出最终目标车位姿**，也就是规划器最后想把车开到的位置和朝向。
> 
> 它不是固定值，会根据：
> 
> - FSD/OD 是否侵占终点
> - 车道线
> - 车位参考线
> - UWB 遥控点位
> - 是否已经出车位
> 
> 不断动态调整。
> 
> ---
> 
> ## 4）主边界 / 子边界
> 
> 地图最终输出左右边界，但内部其实有：
> 
> - 主边界（Main boundary）：靠车位一侧，更重要
> - 子边界（Sub boundary）：通道另一侧
> 
> 代码根据 `bSlotDataAtRigthSide` 和 `parkside` 决定左右哪个是主边界。
> 
> ---
> 
> # 五、主流程详细拆解
> 
> ---
> 
> ## A. `APAMap_ParkingOutCalMapSlotPar()`
> 作用：**确定目标车位和基础参数**
> 
> 它主要做这些事情：
> 
> ### 1）确定车位数据是在车右侧还是左侧
> 通过：
> 
> - `parkoutmode`
> - `parkside`
> - `SlotDataIsNotMirrored`
> 
> 算出：
> 
> ```cpp
> bSlotDataAtRigthSide
> ```
> 
> 这个变量贯穿全模块。
> 
> ---
> 
> ### 2）初始化各种车位点缓存
> 把 `VplPt / UsPt / ODPt / FSDPt / PAPt` 先清成 `NO_OBJ_DISTANCE`。
> 
> ---
> 
> ### 3）特殊模式直接返回
> 如果 `parkmode` 是：
> 
> - `PARKING_OUT`
> - `PARKEXIT`
> 
> 则直接拿当前车位置信息，不去从 slot 列表里找 ID。
> 
> ---
> 
> ### 4）从不同车位源查找 SlotID
> 根据 `FusionMode`：
> 
> - US slot
> - VPL slot
> - Fusion slot
> 
> 遍历数据，找到匹配的 `SlotID`。
> 
> 失败则：
> ```cpp
> APAMAP_Setfailcause(2);
> ```
> 
> ---
> 
> ## B. `APAMap_ParkingOutCalSlotInfo()`
> 作用：**计算车位边界点**
> 
> 它的顺序是：
> 
> ### 1）优先尝试从“车位框/总图车位信息”直接算
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();
> ```
> 
> 如果编译宏允许，说明系统更信任感知到的完整车位框。
> 
> ---
> 
> ### 2）失败再退回“传统泊出信息构造”
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();
> ```
> 
> 即按泊出模式、车姿、车尺寸等推一个默认车位框。
> 
> ---
> 
> ### 3）再融合 FSD 和 OD 做修正
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
> ```
> 
> 这是最核心的修正步骤。
> 
> ---
> 
> # 六、`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 在干嘛
> 
> 这是整份代码里最重要的函数之一。
> 
> 它做的事情是：
> 
> > 先拿一个“初始车位框”，再利用 FSD、OD、超声等障碍信息，把车位边界点往安全方向收缩/平移。
> 
> ---
> 
> ## 1）获取初始边界点
> ```cpp
> Obj2Pt = SlotBordPt[0];
> Obj1Pt = SlotBordPt[1];
> SlotLen = SlotPar.SlotLen;
> EndPos = SlotPar.EndPos;
> ```
> 
> ---
> 
> ## 2）计算各类 offset
> 它分别求出不同源对车位边界的偏移建议：
> 
> ### 传感器偏移
> ```cpp
> APAMap_CalSlotBorderPtOffsetBySensorMapInfo(...)
> ```
> 
> ### FSD 偏移
> ```cpp
> APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(...)
> ```
> 
> ### OD 偏移
> ```cpp
> APAMap_CalSlotBorderPtOffsetByODMapInfo(...)
> ```
> 
> 然后把三者取最大值：
> ```cpp
> Offset = max(FSD, Sensor, OD)
> ```
> 
> 意思是：**谁更保守，就听谁的**。
> 
> ---
> 
> ## 3）计算离车身的内侧距离
> 通过：
> 
> - 车位边界线
> - 当前车位点
> - 车身 bumper 距离
> 
> 算出 `fDis1 / fDis2`，即车内剩余空间。
> 
> ---
> 
> ## 4）必要时更新默认边界
> 如果满足：
> 
> - 已经做过锚点转换 `bAfterNewAnchorPointFlag`
> - 车已经部分开出车位
> 
> 则有时不再继续收缩边界，而是把 offset 清零：
> 
> ```cpp
> OffsetX1 = OffsetX2 = OffsetY1 = OffsetY2 = 0;
> ```
> 
> 因为这时更关注车外通道，而不是原车位内侧。
> 
> ---
> 
> ## 5）安全性校验
> 算出修正后剩余空间：
> 
> ```cpp
> NewDis = (fDis1 - OffsetY1) + (fDis2 - OffsetY2);
> ```
> 
> 如果太小：
> ```cpp
> APAMAP_Setfailcause(58);
> return FALSE;
> ```
> 
> 即：**车位被压缩得已经不够安全了**。
> 
> ---
> 
> ## 6）通过几何线求新 Obj1 / Obj2
> 它做的是典型的二维几何：
> 
> - 以 Obj1/Obj2 为基础点
> - 建立平行线
> - 按 offset 平移线
> - 求交点
> - 得到新的边界点
> 
> 用了很多：
> - `AlgCom_LineParABCByCurrentCarPosition`
> - `AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine`
> - `AlgCom_CrossPointOfTwoLines`
> 
> ---
> 
> ## 7）重新计算终点位姿 EndPos
> 如果没有被以下条件锁住：
> 
> - 锚点转换后
> - 车道线更新过终点
> - 参考线更新过终点
> 
> 则重新计算终点：
> 
> ```cpp
> EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(...)
> ```
> 
> 如果支持 UWB，则优先用 UWB 版本。
> 
> ---
> 
> # 七、`APAMap_ParkingOutCalMapInfo()` 在干嘛
> 
> 这是**生成最终边界地图**的总函数。
> 
> 它的流程非常清晰：
> 
> ---
> 
> ## 1）先生成默认边界
> ```cpp
> APAMap_ParkingOutCalBoundaryByParkOutInfo();
> ```
> 
> 也就是先构造一个“模板通道”。
> 
> ---
> 
> ## 2）融合 FSD 边界
> ```cpp
> APAMap_ParkingOutFusBoundaryByFSDMapInfo();
> ```
> 
> 将视觉/顶视图感知到的边界段融合进去。
> 
> ---
> 
> ## 3）融合车道线
> ```cpp
> APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
> ```
> 
> 车道线不仅可以修边界，还可能改变终点姿态。
> 
> ---
> 
> ## 4）融合车位参考线
> ```cpp
> APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
> ```
> 
> 类似车道线，但更偏向车位线/参考线。
> 
> ---
> 
> ## 5）融合 OD 障碍
> ```cpp
> APAMap_FusBoundaryByODMapInfo();
> ```
> 
> 把障碍物对通道边界的收缩影响融合进来。
> 
> ---
> 
> ## 6）平滑边界
> ```cpp
> APAMap_SmoothMapBoundary(0);
> ```
> 
> ---
> 
> ## 7）检查终点位姿是否被边界/障碍侵占
> ```cpp
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> ```
> 
> 如果侵占：
> 
> - 尝试终点居中 `APAMap_ParkingOutCenterEndCarPosInfo()`
> - 再不行就循环移动终点，最多 9 次
> 
> ---
> 
> # 八、默认边界怎么构造：`APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 
> 这个函数是在“没有感知修正时”的基础模型。
> 
> 它做法是：
> 
> 1. 根据泊出模式（平行/头出/尾出）确定默认边界宽度；
> 2. 根据是否已经出车位、是否短车位、是否斜列，调节默认边界；
> 3. 用 Obj1、Obj2、车位方向、通道宽度生成主边界 `MainBoundary`；
> 4. 再在对侧生成一条很远的子边界 `SubBoundary`。
> 
> 最终把左右边界写到：
> 
> ```cpp
> APAMap_GInfo.OutLine.LeftBoundary
> APAMap_GInfo.OutLine.RightBoundary
> ```
> 
> ---
> 
> # 九、车道线 / 参考线 对终点的影响
> 
> 这部分很值得注意。
> 
> ---
> 
> ## 1）`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> 
> 它主要做两件事：
> 
> ### 边界融合
> 把符合空间范围的车道线段融合进主边界/子边界。
> 
> ### 更新终点姿态
> 如果检测到一条可信的主车道线：
> 
> - 拟合直线
> - 根据直线斜率算出终点朝向 `EndPos.CarAng`
> - 必要时修改 `EndPosLine`
> 
> 这是为了让泊出终点更贴合真实道路方向。
> 
> ---
> 
> ## 2）`APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> 
> 类似，但它用的是车位参考线。
> 
> 逻辑也一样：
> 
> - 过滤可用线段
> - 找最近且足够长的线
> - 根据线方向修正终点姿态
> 
> 并且它会和 LaneLine 有优先级关系：
> 如果车道线已经更新终点，参考线角度更差，就会放弃。
> 
> ---
> 
> # 十、终点位姿是怎么动态修的
> 
> 终点位姿相关函数：
> 
> - `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> - `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> - `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> - `APAMap_ParkingOutCenterEndCarPosInfo()`
> - `APAMap_ParkingOutEndCarPosUpdata()`
> 
> ---
> 
> ## 1）默认终点
> 不同泊出模式对应不同默认策略：
> 
> - 平行泊出：终点在车位外侧通道中
> - 垂直/斜列泊出：终点在车位前方/后方通道中
> - 直进直出模式：终点沿当前车身方向延展
> 
> ---
> 
> ## 2）如果终点被侵占
> `APAMap_ParkingOut

#### _User_

> 继续解析各个函数

#### _Assistant_

> 可以，我继续按**“函数级别”**往下拆。
> 
> 不过先说明一下：  
> 你现在贴出来的是**函数名和调用链信息**，我能基于命名、调用顺序、典型 APA 泊出建图逻辑，做**高可信的结构解析**；但如果你希望我做到**逐行变量含义、每个 if 分支、每个 fail cause 的精确解释**，最好把 `.c/.cpp` 的实现贴出来，我可以继续做成“源码注释版解析”。
> 
> 下面我先按你这套代码里已经出现的函数，继续细化。
> 
> ---
> 
> # 一、先按模块分组看这些函数
> 
> 这批函数大致可以分成 6 类：
> 
> ---
> 
> ## 1）任务调度类
> 负责控制整个泊出建图流程
> 
> - `APAMap_ParkingOutTask()`
> - `APAMap_ParkingOutUpDataMapInfo()`
> - `APAMap_ParkingOutDebugInit()`
> 
> ---
> 
> ## 2）车位参数确定类
> 负责找到“到底是哪个车位”、这个车位的基本属性是什么
> 
> - `APAMap_ParkingOutCalMapSlotPar()`
> 
> ---
> 
> ## 3）车位几何构造类
> 负责求车位边界点、Obj1/Obj2、SlotBordPt、车位长度深度等
> 
> - `APAMap_ParkingOutCalSlotInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> ---
> 
> ## 4）边界地图构造类
> 负责生成通道左右边界，并融合环境信息
> 
> - `APAMap_ParkingOutCalMapInfo()`
> - `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> - `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> - `APAMap_FusBoundaryByODMapInfo()`
> - `APAMap_SmoothMapBoundary()`
> 
> ---
> 
> ## 5）终点位姿调整类
> 负责确定 `EndPos`
> 
> - `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> - `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> - `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> - `APAMap_ParkingOutCenterEndCarPosInfo()`
> - `APAMap_ParkingOutEndCarPosUpdata()`
> 
> ---
> 
> ## 6）合法性/保护类
> 负责判断算出来的地图是否可用
> 
> - `APAMap_ParkingOutCheckIfCarPosIsValid()`
> - `APAMAP_Setfailcause(...)`
> 
> ---
> 
> 下面我按函数一个个解释。
> 
> ---
> 
> # 二、`APAMap_ParkingOutDebugInit()`
> 
> 这个函数一般是**建图前的状态清零**。
> 
> 它很可能做这些事：
> 
> - 清空上一帧调试标志；
> - 复位某些融合来源标记；
> - 清空边界来源记录；
> - 清掉终点更新标志；
> - 重置 fail cause；
> - 重置 “某条线是否更新过终点姿态” 的布尔量。
> 
> 典型会包含类似语义的变量：
> 
> - `bLaneLineUpdateEndPosFlag = FALSE`
> - `bReferLineUpdateEndPosFlag = FALSE`
> - `bAfterNewAnchorPointFlag = FALSE`
> - `bSeizeEndCarPosFlag = FALSE`
> 
> ---
> 
> ## 这个函数的重要性
> 它虽然不是核心算法，但很关键。  
> 因为泊出地图构建不是一次性的，很多状态是“跨帧”的，如果不清理：
> 
> - 上一帧的终点角度可能污染当前帧；
> - 上一帧的边界侵占标志可能导致本帧误修正；
> - 调试变量会误导后续判断。
> 
> 所以它是**防止状态串帧**的。
> 
> ---
> 
> # 三、`APAMap_ParkingOutCalMapSlotPar()`
> 
> 这是泊出地图里最重要的前置函数之一。  
> 它的本质是：
> 
> > **先搞清楚：这次要泊出的到底是哪一个车位，以及这个车位的基础属性是什么。**
> 
> ---
> 
> ## 它大概率完成的事情
> 
> ---
> 
> ### 1）读取泊出请求
> 从：
> 
> - `Request_cmd`
> - `Request_SlotId`
> - `Request_SlotFusionMode`
> - `parkside`
> - `parkoutmode`
> - `parkmode`
> 
> 中判断当前任务是什么类型。
> 
> 比如：
> 
> - 是正常 APA 泊出；
> - 是遥控泊出；
> - 是融合车位；
> - 是超声车位；
> - 是视觉车位；
> - 是直进直出；
> - 是平行/垂直/斜列。
> 
> ---
> 
> ### 2）判断车位数据是否需要镜像
> 你前面提到过：
> 
> - `SlotDataIsNotMirrored`
> - `bSlotDataAtRigthSide`
> 
> 这说明感知层给上来的车位，有时是按“固定一侧模板”定义的，建图时要根据当前真实泊车侧做左右映射。
> 
> 所以这个函数会算出：
> 
> - 当前目标车位在车左还是车右；
> - 感知数据是否已经是正确侧；
> - 若不是，后续几何计算时要不要做镜像变换。
> 
> ---
> 
> ### 3）从不同车位源中找目标 Slot
> 它应该会根据 fusion mode 去不同源里找同一个 `SlotID`：
> 
> - USSlot
> - VPLSlot
> - FusSlot
> 
> 常见逻辑是：
> 
> ```text
> 如果指定融合车位 -> 在 FusSlot 里找
> 如果指定视觉车位 -> 在 VPLSlot 里找
> 如果指定超声车位 -> 在 Usslot 里找
> ```
> 
> 找到后，把索引保存到：
> 
> - `SlotIndex`
> - `FusSlotIndex`
> - 或其他内部变量
> 
> ---
> 
> ### 4）提取车位基本参数
> 例如：
> 
> - 车位长度 `SlotLen`
> - 车位深度 `SlotDepth`
> - 车位角度
> - 车位朝向
> - 车位中心
> - 左右边界信息
> - 是否有效的车位类型
> 
> ---
> 
> ### 5）初始化不同来源的边界参考点
> 你前面提到这些点会先清为默认无效值：
> 
> - `VplPt`
> - `UsPt`
> - `ODPt`
> - `FSDPt`
> - `PAPt`
> 
> 这说明该函数还负责**初始化多源车位边界候选点容器**，为后面融合做准备。
> 
> ---
> 
> ## 这个函数失败意味着什么
> 如果这里失败，往往说明：
> 
> - 没找到请求的车位；
> - 车位源数据为空；
> - 车位类型不支持当前泊出模式；
> - 车位信息不完整。
> 
> 这类失败是“**建图前提不成立**”。
> 
> ---
> 
> # 四、`APAMap_ParkingOutCalSlotInfo()`
> 
> 这个函数的目标是：
> 
> > 从“车位参数”进一步算出“车位的实际几何边界信息”。
> 
> 也就是把抽象的车位，变成几何上的点和线。
> 
> ---
> 
> ## 它大概率分三步
> 
> ---
> 
> ### Step 1：从现成车位框直接提边界
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();
> ```
> 
> 这一步是**优先使用感知层给出的车位框**。  
> 如果感知已经给出比较完整的四点框、边线、角点，那就最好直接用。
> 
> 优点：
> 
> - 贴近真实车位；
> - 不用靠车尺寸和模板猜；
> - 对斜列/异形情况更准确。
> 
> ---
> 
> ### Step 2：如果现成车位框不可靠，就回退到模板推算
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();
> ```
> 
> 这一步是**兜底逻辑**。
> 
> 就是如果：
> 
> - 感知框不完整；
> - 角点质量不够；
> - 车位线检测不到；
> - 数据源不一致；
> 
> 那就根据：
> 
> - 当前车姿
> - 车位模式
> - 车位方向
> - 泊出侧
> - 标准车位尺寸
> 
> 来推一个“合理的近似车位”。
> 
> 这是很多 APA 系统必须有的 fallback。
> 
> ---
> 
> ### Step 3：再用 FSD/OD/传感器做边界修正
> ```cpp
> APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
> ```
> 
> 这一步是“**先有粗车位，再做环境修边**”。
> 
> 也就是说：
> 
> - 前两步得到的是“理论车位框”
> - 这一步把环境中的障碍、车位边沿、顶视图轮廓真正融合进去
> 
> 最后得到更安全、更真实的 Obj1/Obj2 和车位边界。
> 
> ---
> 
> # 五、`APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> 
> 这个函数的作用是：
> 
> > 直接从已有“车位框感知信息”中提取泊出建图需要的边界点。
> 
> ---
> 
> ## 它通常会做什么
> 
> ---
> 
> ### 1）读取车位框的角点
> 假设感知层给的是：
> 
> - 4 个角点
> - 或中线 + 宽度/深度
> - 或边线集合
> 
> 那么这里会把它们统一转换到建图坐标系下。
> 
> ---
> 
> ### 2）识别哪条边是车位口、哪条边是车位底边
> 泊出最关心的是：
> 
> - 开口边在哪；
> - 左边界和右边界是哪条；
> - 车位深度方向朝哪里。
> 
> 因此它会根据：
> 
> - 当前车头朝向；
> - 车位角度；
> - parkside；
> - parkoutmode；
> 
> 判断哪个角点作为：
> 
> - `Obj1Pt`
> - `Obj2Pt`
> - `SlotBordPt[0..3]`
> 
> ---
> 
> ### 3）设置初始 `EndPos`
> 一旦车位框确定，通常就能先给出一个初始终点姿态：
> 
> - 位置在车位开口外一定距离；
> - 朝向与车位或道路方向一致。
> 
> ---
> 
> ### 4）做几何合法性检查
> 比如：
> 
> - 车位宽度是不是太窄；
> - 四个点是否交叉；
> - 边长是否异常；
> - 角度是否跳变太大。
> 
> 不合法就失败，回退到模板法。
> 
> ---
> 
> ## 这个函数的特点
> 它是“**精确来源优先**”。
> 
> 如果这个函数成功，后续模板推算的依赖就会变小，整体地图质量更高。
> 
> ---
> 
> # 六、`APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> 
> 这是一个典型的**兜底构造函数**。
> 
> 它的核心思想是：
> 
> > 即便没有高质量车位框，也要根据车辆当前状态和标准几何关系，推一个能用的泊出车位。
> 
> ---
> 
> ## 它大概率怎么推
> 
> ---
> 
> ### 1）确定车位方向
> 根据：
> 
> - 当前车角度 `CarAng`
> - 泊出模式
> - 停车侧
> - 请求中保存的车位方向信息
> 
> 推一个车位主方向。
> 
> 平行位通常和道路平行，垂直位通常和道路垂直。
> 
> ---
> 
> ### 2）设置车位宽度/深度
> 如果没有真实感知值，就采用标定参数：
> 
> - 标准平行车位长度
> - 标准垂直车位深度
> - 默认安全边距
> 
> 这些参数常见来自配置表。
> 
> ---
> 
> ### 3）从当前车身位置反推车位边界
> 因为在泊出任务中，车通常已经在车位内，所以可以利用：
> 
> - 车辆中心
> - 车长车宽
> - 车头/车尾距
> - 车位边距假设
> 
> 倒推出车位左右界和前后界。
> 
> ---
> 
> ### 4）生成 `Obj1Pt / Obj2Pt / SlotBordPt`
> 最终形成一个简化矩形或平行四边形车位。
> 
> ---
> 
> ### 5）给出默认终点 `EndPos`
> 这个终点通常不一定很精确，但能保证规划先有目标可走。
> 
> ---
> 
> ## 它的价值
> 这个函数不是为了“最准”，而是为了“**永远尽量别空地图**”。
> 
> 在 APA 系统里，fallback 很重要。  
> 只要 fallback 合理，系统鲁棒性就会很高。
> 
> ---
> 
> # 七、`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> 这是车位修正核心函数，我继续拆细一点。
> 
> ---
> 
> ## 它的总体目的
> 
> > 用环境感知结果去校正初始车位边界，使车位模型更贴近真实环境，并确保终点和可行空间安全。
> 
> ---
> 
> ## 可以把它理解成 4 个子任务
> 
> ---
> 
> ### 子任务 1：收集各来源对边界的“压缩建议”
> 你前面已经看到它会调用：
> 
> - `APAMap_CalSlotBorderPtOffsetBySensorMapInfo(...)`
> - `APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(...)`
> - `APAMap_CalSlotBorderPtOffsetByODMapInfo(...)`
> 
> 这三个函数本质都是在回答：
> 
> > “如果考虑这一类传感器，这个车位边界应该往里缩多少？”
> 
> 结果通常表现为：
> 
> - `OffsetX1 / OffsetY1`
> - `OffsetX2 / OffsetY2`
> 
> 或者某种沿法向/切向的偏移量。
> 
> ---
> 
> ### 子任务 2：取最保守结果
> 把多个来源的 offset 做融合，通常取：
> 
> - 最大收缩量；
> - 最靠近障碍的限制值；
> - 最高优先级来源。
> 
> 这是典型安全设计：  
> **宁可保守，不可过宽。**
> 
> ---
> 
> ### 子任务 3：检查收缩后空间是否还足够
> 它会计算：
> 
> - 车辆和左右边界的剩余距离；
> - 收缩后总可用宽度；
> - 是否小于车宽 + 安全裕度。
> 
> 如果太小，就 fail。
> 
> 这一步很关键，因为有时感知会把车位压得过窄，规划再做也没有意义。
> 
> ---
> 
> ### 子任务 4：更新边界点和终点
> 在通过几何运算后，得到新的：
> 
> - `Obj1Pt`
> - `Obj2Pt`
> - `SlotBordPt[...]`
> - `EndPos`
> 
> 如果某些高优先级来源已经更新过终点姿态，这一步可能只更新位置，不再更新朝向。
> 
> ---
> 
> ## 这个函数的本质
> 它其实是在做：
> 
> > **“车位几何优化 + 安全约束过滤 + 终点重算”**
> 
> 是泊出建图里最“工程化”的函数之一。
> 
> ---
> 
> # 八、`APAMap_ParkingOutCalMapInfo()`
> 
> 这是整个“地图构造”阶段的总控函数。
> 
> 可以理解成：
> 
> > 先生成一版默认通道，再一步步把各种环境边界融合进去，最后检查终点是否可用。
> 
> ---
> 
> ## 它大致是一个流水线
> 
> ---
> 
> ### 1）默认边界生成
> ```cpp
> APAMap_ParkingOutCalBoundaryByParkOutInfo();
> ```
> 
> 给规划器先准备一版“至少能走”的左右边界。
> 
> ---
> 
> ### 2）FSD 融合
> ```cpp
> APAMap_ParkingOutFusBoundaryByFSDMapInfo();
> ```
> 
> 把顶视图/视觉检测到的边沿、路缘、停车线等融进去。
> 
> ---
> 
> ### 3）LaneLine 融合
> ```cpp
> APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
> ```
> 
> 利用道路车道线去修正边界与终点朝向。
> 
> ---
> 
> ### 4）RefercLine 融合
> ```cpp
> APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
> ```
> 
> 利用车位参考线、引导线、边线等继续优化。
> 
> ---
> 
> ### 5）OD 融合
> ```cpp
> APAMap_FusBoundaryByODMapInfo();
> ```
> 
> 把实际障碍物侵占进边界里。
> 
> ---
> 
> ### 6）平滑
> ```cpp
> APAMap_SmoothMapBoundary(0);
> ```
> 
> 避免边界折线过于尖锐。
> 
> ---
> 
> ### 7）终点侵占检查
> ```cpp
> APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> ```
> 
> 如果终点被压住，再调整终点。
> 
> ---
> 
> ## 这个函数的设计思想
> 不是一上来就完全依赖传感器。  
> 而是：
> 
> **模板初始化 → 感知逐步修正 → 安全检查 → 终点重定位**
> 
> 这是比较成熟的车载工程做法。
> 
> ---
> 
> # 九、`APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 
> 这个函数是“**默认通道生成器**”。
> 
> ---
> 
> ## 它在生成什么
> 它生成的是规划器真正要走的两条边界：
> 
> - 左边界 `LeftBoundary`
> - 右边界 `RightBoundary`
> 
> ---
> 
> ## 它通常怎么做
> 
> ---
> 
> ### 1）确定主边界与子边界
> 根据车位在哪一侧，决定：
> 
> - 主边界是靠车位一侧
> - 子边界是远离车位一侧
> 
> 因为靠车位这一侧更重要，精度要求更高。
> 
> ---
> 
> ### 2）确定通道延展方向
> 根据泊出模式：
> 
> - 平行泊出：沿道路方向延展
> - 垂直泊出：

#### _User_

> 继续

#### _Assistant_

> 好，我接着上次没说完的地方，继续往下拆。
> 
> 上次停在：
> 
> > `APAMap_ParkingOutCalBoundaryByParkOutInfo()`  
> > 根据泊出模式生成默认左右边界
> 
> 我从这里继续。
> 
> ---
> 
> # 九、`APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 
> 这个函数是**默认通道边界生成函数**。  
> 它做的不是“识别环境”，而是：
> 
> > 先根据车位几何、车辆朝向、泊出模式，生成一版理论上的可行驶通道。
> 
> 这版边界后面还会被 FSD、车道线、障碍物继续修正。
> 
> ---
> 
> ## 它通常怎么做
> 
> ### 1）确定通道延展方向
> 根据泊出模式区分：
> 
> - **平行泊出**：边界主要沿道路方向延展；
> - **垂直泊出**：边界主要沿车位口外法向延展；
> - **斜列泊出**：边界沿车位开口方向和道路方向的组合延展。
> 
> 也就是说，这一步先确定“通道主轴”。
> 
> ---
> 
> ### 2）确定主边界和副边界
> 通常会有两条边：
> 
> - **MainBoundary**：更贴近车位一侧/更关键的一侧；
> - **SubBoundary**：通道另一侧。
> 
> 然后再根据 `parkside` 或 `bSlotDataAtRigthSide` 决定：
> 
> - MainBoundary 对应 LeftBoundary 还是 RightBoundary；
> - SubBoundary 对应另一边。
> 
> ---
> 
> ### 3）设置默认通道宽度
> 这个宽度一般来自：
> 
> - 车宽；
> - 车外轮廓安全余量；
> - 泊出模式经验参数；
> - 当前是否已经“部分出位”。
> 
> 比如：
> 
> - 如果车还基本在车位内，通道会保守一些；
> - 如果车已经出位，边界可能放得更远，强调道路通行空间。
> 
> ---
> 
> ### 4）用 Obj1 / Obj2 拉出边界折线
> 通常 `Obj1Pt / Obj2Pt` 是车位口两侧的关键点。  
> 以它们为起点，沿通道方向外扩若干距离，生成折线点列。
> 
> 常见结果是：
> 
> - 主边界：经过车位口一侧，往外延伸；
> - 副边界：在通道另一侧平行延伸。
> 
> 所以你会看到 `PtNum`、`Boundary[i]` 这样的点集结构。
> 
> ---
> 
> ### 5）处理特殊情况
> 这个函数里通常会夹带很多工程修补逻辑，例如：
> 
> - 短车位：默认边界更快放宽；
> - 斜列位：边界不完全平行，需要带角度；
> - 已经过锚点变换：边界起点不再完全依赖原始 Slot 点；
> - 车辆已经明显出位：起始段不要再贴着车位边框走。
> 
> ---
> 
> ## 它的本质
> 这个函数是在做一件很朴素但很重要的事：
> 
> > **先给规划器一条“理论上肯定存在”的通道骨架。**
> 
> 后面所有感知融合，本质上都是在这条骨架上做“修边”。
> 
> ---
> 
> # 十、`APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> 
> 这个函数是把 **FSD TopView** 信息融合到边界中。
> 
> 这里的 FSD 通常可以理解为：
> 
> - 顶视图语义边界；
> - 路沿、边缘、停车线；
> - 某些视觉/占用网格提取的边界轮廓。
> 
> ---
> 
> ## 它解决什么问题
> 
> 默认边界是模板生成的，可能：
> 
> - 过宽；
> - 没贴真实路沿；
> - 没避开真实边缘；
> - 没反映局部弯折。
> 
> FSD 融合就是为了让边界更像真实可行驶区域。
> 
> ---
> 
> ## 它大概率的内部流程
> 
> ### 1）读取 TopView 边界候选点
> 从 `TopView` 中取出：
> 
> - 边缘点；
> - 线段；
> - 障碍边界；
> - 可通行区域边沿。
> 
> ---
> 
> ### 2）筛选与泊出方向相关的点
> 不是所有 TopView 点都能用。  
> 通常只选：
> 
> - 在车辆前方/后方一定区域内；
> - 靠近默认边界附近；
> - 与通道方向大体一致；
> - 连续性较好、置信度较高的点。
> 
> ---
> 
> ### 3）按左右侧分别归类
> 把 FSD 点投影到通道坐标系后，判断它更接近：
> 
> - 左边界；
> - 右边界；
> - 或者无效点。
> 
> ---
> 
> ### 4）替换或压缩默认边界
> 融合方式常见有两种：
> 
> #### 方式 A：直接替换局部边界段
> 如果 FSD 检到了一段很清晰的边缘线，就直接用它替代默认边界某段。
> 
> #### 方式 B：对默认边界施加约束
> 如果 FSD 只说明“这里不能更外了”，那就把默认边界往内收。
> 
> ---
> 
> ### 5）保证边界连续
> 融合后通常还会做：
> 
> - 点排序；
> - 去抖；
> - 删除跳点；
> - 合并过近点。
> 
> 否则规划器看到的边界会断裂或锯齿。
> 
> ---
> 
> ## 这个函数的特点
> FSD 对几何边界通常比“纯模板”更真实，但也容易受噪声影响。  
> 所以它通常不会完全推翻默认地图，而是：
> 
> > **在默认地图上做可信局部修正。**
> 
> ---
> 
> # 十一、`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> 
> 这个函数是把 **车道线** 融入泊出地图。
> 
> 它的作用一般不只是改边界，还会**修正终点姿态角**。
> 
> ---
> 
> ## 为什么车道线对泊出这么重要
> 
> 泊出之后，车辆通常要进入车道。  
> 所以终点位姿不能只看车位，还要看道路方向。
> 
> 如果车道线可靠，终点角度最好和车道线对齐，这样：
> 
> - 规划更自然；
> - 出位后更容易接入车道；
> - 后续控制也更稳。
> 
> ---
> 
> ## 它的典型逻辑
> 
> ### 1）读取 LaneLine 数据
> 可能包含：
> 
> - 左右车道线点列；
> - 线段拟合参数；
> - 置信度；
> - 线型。
> 
> ---
> 
> ### 2）筛掉无关的车道线
> 例如剔除：
> 
> - 太远的；
> - 太短的；
> - 与当前道路方向差异过大的；
> - 与泊出区域空间位置不匹配的线。
> 
> ---
> 
> ### 3）找到“最可信的主车道线”
> 通常会选满足这些条件的一条或两条线：
> 
> - 离车辆/终点较近；
> - 长度足够；
> - 方向稳定；
> - 点数够多；
> - 位于车道合理一侧。
> 
> ---
> 
> ### 4）用车道线修边界
> 如果 LaneLine 与默认边界很接近，就可以：
> 
> - 把边界往车道线靠齐；
> - 或用车道线替代局部边界段。
> 
> 尤其是泊出后外部道路区域，车道线是很好的边界参考。
> 
> ---
> 
> ### 5）用车道线修正 `EndPos.CarAng`
> 这通常是更重要的一步。
> 
> 做法一般是：
> 
> - 拟合车道线方向角；
> - 把终点朝向向这个角度收敛；
> - 设置标志位，例如  
>   `bLaneLineUpdateEndPosFlag = TRUE`
> 
> 这样后面其他模块就知道：
> 
> > “终点朝向已经被更高可信的道路信息更新过了。”
> 
> ---
> 
> ## 这个函数的重要性
> 它相当于把“车位坐标系”过渡到“道路坐标系”。
> 
> 也就是说：
> 
> - 车位边界解决“怎么出位”；
> - 车道线解决“出了位以后朝哪儿摆正”。
> 
> ---
> 
> # 十二、`APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> 
> 这个函数和 LaneLine 很像，但对象是 **参考线 RefercLine**。
> 
> 从命名看，这种参考线通常可能是：
> 
> - 车位边线；
> - 引导线；
> - 路沿参考线；
> - 泊车场地中的人工参考边界。
> 
> ---
> 
> ## 它和 LaneLine 的区别
> 
> ### LaneLine 更偏“道路”
> 它表达的是道路通行方向。
> 
> ### RefercLine 更偏“结构化场地”
> 它表达的是：
> 
> - 车位线；
> - 场地边界；
> - 停车区引导线；
> - 更局部、更接近泊位的几何参考。
> 
> ---
> 
> ## 这个函数的典型作用
> 
> ### 1）辅助修正边界
> 当道路车道线不明显，但车位参考线很清楚时，RefercLine 能更准确地描述：
> 
> - 车位口方向；
> - 靠边一侧的真实边界；
> - 通道局部结构。
> 
> ---
> 
> ### 2）修正终点角度
> 如果 LaneLine 不可用，RefercLine 常常会成为次优来源。
> 
> 逻辑可能是：
> 
> - 若 `bLaneLineUpdateEndPosFlag == FALSE`  
>   则允许 RefercLine 更新终点角；
> - 若 LaneLine 已经更新过终点，则 RefercLine 只有在更可信时才覆盖。
> 
> ---
> 
> ### 3）提高车位口附近几何精度
> 很多泊出问题，不是在远处，而是在**刚出位这几米**。  
> 参考线通常对这段几何更敏感。
> 
> ---
> 
> ## 它的工程意义
> 这个函数体现了一个典型思路：
> 
> > **道路信息和场地信息都要用，但要分优先级。**
> 
> 通常优先级是：
> 
> 1. 车道线决定道路方向；
> 2. 参考线决定局部车位口结构；
> 3. 若道路信息缺失，参考线再承担更多职责。
> 
> ---
> 
> # 十三、`APAMap_FusBoundaryByODMapInfo()`
> 
> 这个函数是用 **OD 障碍物** 融合边界。
> 
> OD 一般可以理解为 Object Detection 或障碍物检测模块输出。
> 
> ---
> 
> ## 它要解决的问题
> 
> 即使：
> 
> - 车位框是对的；
> - 车道线是对的；
> - FSD 也看到了路沿；
> 
> 但是如果通道里停了一辆车、放了障碍桶、或者墙体侵入，规划也不能照原边界走。
> 
> 所以 OD 融合的目标是：
> 
> > **把动态或静态障碍物对可行驶空间的侵占反映到边界里。**
> 
> ---
> 
> ## 它的典型处理方式
> 
> ### 1）读取障碍物列表
> 每个障碍物可能有：
> 
> - 中心点；
> - 长宽；
> - 方向；
> - 包围盒四角；
> - 类别；
> - 置信度。
> 
> ---
> 
> ### 2）投影到泊出通道坐标系
> 把障碍物变换到当前通道的参考坐标系下，看它是否侵入：
> 
> - 左边界附近；
> - 右边界附近；
> - 中间终点区域。
> 
> ---
> 
> ### 3）按边界侧进行收缩
> 如果某个障碍物离左边界很近，就把左边界往内收；
> 离右边界很近，就收右边界。
> 
> 如果障碍物直接占据通道中央，有可能：
> 
> - 两侧都收；
> - 或直接判终点被侵占；
> - 或判整个地图不可用。
> 
> ---
> 
> ### 4）处理前向终点区域
> 有些障碍不贴边，但正好挡在终点位置上。  
> 这时边界不一定怎么变，但 `EndPos` 会被判定“被占”。
> 
> 因此这个函数和后面的终点侵占判断关系很紧密。
> 
> ---
> 
> ## 它和 FSD 的区别
> FSD 更像“边缘/轮廓类信息”；  
> OD 更像“明确的实体障碍信息”。
> 
> 所以常见策略是：
> 
> - FSD 修边界形状；
> - OD 修安全距离和侵占区域。
> 
> ---
> 
> # 十四、`APAMap_SmoothMapBoundary(0)`
> 
> 这个函数是**边界平滑函数**。
> 
> 前面融合了那么多来源之后，边界很容易出现：
> 
> - 折点突变；
> - 局部锯齿；
> - 点间距不均；
> - 短距离内角度抖动。
> 
> 规划器通常不喜欢这样的地图，所以要平滑。
> 
> ---
> 
> ## 它可能做的事
> 
> ### 1）删除过近点
> 如果相邻边界点距离太小，就删掉冗余点。
> 
> ### 2）限制角度突变
> 如果某两个线段夹角太尖，做圆滑过渡或中值修正。
> 
> ### 3）统一点间距
> 让边界采样更均匀，方便规划器使用。
> 
> ### 4）保留关键结构点
> 比如车位口、边界转折起点这些关键点不能被过度平滑掉。
> 
> ---
> 
> ## 为什么平滑不能太强
> 因为泊出场景往往很窄，如果过度平滑：
> 
> - 真实障碍边沿会被抹掉；
> - 局部收缩会被放宽；
> - 规划看起来更顺，但安全性变差。
> 
> 所以这个函数一般是在：
> 
> > **保持安全约束前提下，做轻量平滑。**
> 
> ---
> 
> # 十五、`APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> 
> 这个函数非常关键。  
> 它在回答一个问题：
> 
> > **当前计算出来的终点位姿 EndPos，会不会被地图边界或障碍物“占住”？**
> 
> 也就是终点是否可停、可到达。
> 
> ---
> 
> ## 它通常怎么判断
> 
> ### 1）把终点位姿变成车辆包络
> 根据 `EndPos` 的：
> 
> - 位置；
> - 角度；
> - 车长车宽；
> 
> 算出终点状态下车辆的矩形包络。
> 
> ---
> 
> ### 2）与左右边界检查关系
> 看车辆包络是否：
> 
> - 压到左边界；
> - 压到右边界；
> - 超出允许通道区域。
> 
> ---
> 
> ### 3）与障碍物检查关系
> 如果终点车身矩形和某个障碍物包围盒重叠，就说明终点被占。
> 
> ---
> 
> ### 4）与车位局部结构检查关系
> 例如：
> 
> - 终点是否压住车位边线；
> - 是否侵入不允许区域；
> - 是否偏得过大。
> 
> ---
> 
> ## 返回值的含义
> 你前面提到：
> 
> ```cpp
> bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
> ```
> 
> 这说明它一般返回：
> 
> - `TRUE`：终点被侵占；
> - `FALSE`：终点可用。
> 
> ---
> 
> ## 这个函数的定位
> 它相当于一个**终点可行性守门员**。
> 
> 前面所有模块都在“猜”一个终点；  
> 而这个函数负责最后问一句：
> 
> > “这个终点真的能放下一辆车吗？”
> 
> ---
> 
> # 十六、`APAMap_ParkingOutCenterEndCarPosInfo()`
> 
> 如果终点被侵占，这个函数通常是**第一层补救策略**。
> 
> 它的意思大致是：
> 
> > 把终点向通道中间或更合理的位置居中调整，看能不能恢复可用。
> 
> ---
> 
> ## 为什么先做“居中”
> 因为很多终点侵占并不是完全无解，而是：
> 
> - 终点稍微偏左了；
> - 或被某一侧边界压住；
> - 或角度正确但横向位置偏了一点。
> 
> 这时最自然的修复方式不是大改终点，而是先尝试“居中”。
> 
> ---
> 
> ## 它通常怎么做
> 
> ### 1）计算终点相对左右边界的位置
> 求：
> 
> - 终点中心到左边界的距离；
> - 到右边界的距离。
> 
> ### 2）如果两边不对称，就往中间挪
> 使车辆在终点时：
> 
> - 左右余量更均衡；
> - 更不容易压边。
> 
> ### 3）保留终点朝向不变或轻微变化
> 通常先不大改角度，以免破坏前面 LaneLine/RefercLine 的朝向优化。
> 
> ---
> 
> ## 这个函数的性质
> 它是个**低成本修复**。
> 
> 如果有效，说明问题只是终点位置有偏差；  
> 如果无效，说明可能需要更强的终点搜索或直接判失败。
> 
> ---
> 
> # 十七、`APAMap_ParkingOutEndCarPosUpdata()`
> 
> 这个函数从命名看，是**终点位姿更新函数**。  
> 它通常用于在多轮修正过程中，按某种策略迭代调整 `EndPos`。
> 
> ---
> 
> ## 它可能在哪些场景被调用
> 
> ### 1）终点被侵占后循环平移
> 你前面提到过可能“最多 9 次”循环修正。  
> 那这个函数很可能负责：
> 
> - 每次向前/向后平移一点；
> - 或向左右偏移一点；
> - 或按边界方向滑动一点。
> 
> ---
> 
> ### 2）车辆已经部分出位
> 此时终点不再是标准模板点，而要根据当前车姿和边界重新算一个更合理的目标点。
> 
> ---
> 
> ### 3）锚点坐标系变化后
> 如果做了 `NewAnchorPoint` 切换，原来的终点在旧坐标系

#### _User_

> 继续

#### _Assistant_

> 好，我继续接着讲，先把上次停住的 `APAMap_ParkingOutEndCarPosUpdata()` 讲完，然后把剩下几个关键函数也拆掉。
> 
> ---
> 
> # 十七、`APAMap_ParkingOutEndCarPosUpdata()`
> 
> 上次说到，这个函数很像是一个**终点位姿迭代更新器**。
> 
> 它通常不是“第一次生成终点”的函数，而是：
> 
> > 在终点被侵占、边界变化、车已经部分出位、坐标系切换等情况下，对 `EndPos` 做二次或多次修正。
> 
> ---
> 
> ## 它可能的更新策略
> 
> ### 1）沿通道方向平移
> 这是最常见的。
> 
> 如果终点前方或后方有侵占，就沿着通道主方向把终点往外/往里挪一小步：
> 
> - 平移 0.1m
> - 或 0.2m
> - 或按离散步长多次搜索
> 
> 这种方法简单而且稳定。
> 
> ---
> 
> ### 2）沿横向微调
> 如果终点主要是“贴左边”或者“贴右边”，就做横向偏移：
> 
> - 向通道中间移动
> - 或向可用空间更大的一侧移动
> 
> 这通常和 `CenterEndCarPosInfo()` 搭配使用。
> 
> ---
> 
> ### 3）角度保持、只改位置
> 如果终点角度已经被 LaneLine 或 RefercLine 高可信更新过，  
> 那这个函数一般不会轻易改角度，只改：
> 
> - `EndPos.Coordinate.x`
> - `EndPos.Coordinate.y`
> 
> 这样能避免终点方向来回抖。
> 
> ---
> 
> ### 4）在新旧坐标系之间重投影
> 如果系统内部有：
> 
> - 旧坐标系
> - 新锚点坐标系
> 
> 那终点更新后，常常需要重新同步：
> 
> - `EndPos`
> - `EndPosLine`
> - 以及一些与边界相关的派生量
> 
> ---
> 
> ## 它的工程作用
> 这个函数本质上是在做：
> 
> > **“终点搜索”或“终点调整”**
> 
> 比起一次性把终点算死，它提供了一种更鲁棒的机制：
> 
> - 先给出候选终点；
> - 如果不行，再小步试探；
> - 尽量找到一个可停、可达、不过界的解。
> 
> ---
> 
> # 十八、`APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> 
> 这是一个很核心的函数。  
> 从名字可以直接读出来：
> 
> > **在旧坐标系下设置泊出终点位姿。**
> 
> 它一般是“默认终点”的主计算函数。
> 
> ---
> 
> ## 为什么强调 “OldCorSys”
> 这说明系统里至少存在两套参考系中的一种切换：
> 
> - 原始车体/地图坐标系
> - 新锚点坐标系
> 
> 而这个函数负责的是：
> 
> > 按原始几何关系，先把终点算出来。
> 
> 这通常发生在：
> 
> - 初始建图；
> - 尚未做 anchor 更新；
> - 或需要回退到初始模型时。
> 
> ---
> 
> ## 它一般依据什么来算终点
> 
> ### 1）车位模式
> 不同车位模式终点定义完全不同。
> 
> #### 平行泊出
> 终点一般设置在车位外通道中，且车身方向与道路平行。
> 
> #### 垂直泊出
> 终点一般设置在车位口前方一定距离，车身姿态逐渐接近道路方向。
> 
> #### 斜列泊出
> 终点要兼顾车位斜角和道路方向，通常不是简单平移。
> 
> ---
> 
> ### 2）车位关键点
> 比如：
> 
> - `Obj1Pt`
> - `Obj2Pt`
> - `SlotBordPt[0..3]`
> 
> 终点的横向位置往往取决于车位口中心，  
> 纵向位置取决于车位外延距离或通道宽度。
> 
> ---
> 
> ### 3）车辆尺寸和安全边距
> 终点不能只是“几何中心点”，还要考虑：
> 
> - 车长车宽
> - 前后保险杠安全距离
> - 车门侧边余量
> - 最小可行驶通道宽度
> 
> 所以它得到的不是理想中心，而是“**可停放车辆包络后仍安全**”的位置。
> 
> ---
> 
> ### 4）泊出方向
> 有些泊出是：
> 
> - 车头先出
> - 车尾先出
> 
> 终点的定义会不同。  
> 比如车头先出时，终点可能设置得更偏道路前向。
> 
> ---
> 
> ## 这个函数的输出
> 最重要的就是：
> 
> - `EndPos.Coordinate`
> - `EndPos.CarAng`
> 
> 有时还会顺便更新：
> 
> - `EndPosLine`
> - 与终点关联的参考线参数
> 
> ---
> 
> ## 这个函数的意义
> 它是整个泊出规划的“默认目标生成器”。  
> 后续所有优化，基本都是在它算出的终点附近修。
> 
> ---
> 
> # 十九、`APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> 
> 这个函数是上一个函数的**UWB 版本增强**。
> 
> 意思很明显：
> 
> > 如果有 UWB 遥控泊车 / 精定位点位，就优先使用 UWB 信息来修正或直接设定终点。
> 
> ---
> 
> ## UWB 为什么会影响终点
> 
> 因为在某些泊出场景下，终点并不想只是“正常出位后的道路中间位置”，而是：
> 
> - 用户遥控指定的目标点；
> - 某个精定位停车区域；
> - 某个更适合接管或停止的位置。
> 
> UWB 提供的是相对精确的目标参考。
> 
> ---
> 
> ## 它的典型逻辑
> 
> ### 1）先检查 UWB 数据是否有效
> 比如看：
> 
> - 定位状态
> - 信号质量
> - 目标点是否存在
> - 点位是否在合理范围内
> 
> 如果无效，就回退普通版本。
> 
> ---
> 
> ### 2）将 UWB 点转换到当前地图坐标系
> 因为 UWB 坐标未必和泊出地图坐标系一致，所以要做：
> 
> - 平移
> - 旋转
> - 镜像或侧向换算
> 
> ---
> 
> ### 3）把 UWB 点作为终点位置或约束
> 有两种常见方式：
> 
> #### 方式 A：直接替代默认终点
> 如果 UWB 目标非常可信，就直接设为 `EndPos`。
> 
> #### 方式 B：作为默认终点的修正参考
> 比如限制终点必须向 UWB 点靠近，但仍保留道路方向或边界约束。
> 
> ---
> 
> ### 4）终点角度如何处理
> UWB 通常更擅长给位置，不一定给可靠角度。  
> 所以角度可能仍来自：
> 
> - 车道线
> - 参考线
> - 车位方向
> - 当前车身方向
> 
> 也就是说它经常是：
> 
> > **位置听 UWB，角度听道路/车位几何。**
> 
> ---
> 
> ## 这个函数的特点
> 它体现的是一个很典型的融合思想：
> 
> - 几何建图负责“环境合理”
> - UWB 负责“目标更符合用户意图/精定位需求”
> 
> ---
> 
> # 二十、`APAMap_ParkingOutUpDataMapInfo()`
> 
> 这个函数名字非常关键。  
> 前面主流程里你已经看到了：
> 
> - 如果不是首次建图，而是同一个请求持续执行；
> - 或运行到某些阶段；
> - 就不再全量重建，而是调用这个函数。
> 
> 所以它的本质是：
> 
> > **泊出地图的增量更新函数。**
> 
> ---
> 
> ## 为什么需要“更新”而不是“重建”
> 泊出过程是连续的，车辆每一帧都在动。  
> 如果每一帧都从头重建，会有几个问题：
> 
> - 地图抖动大；
> - 计算开销高；
> - 上一帧稳定的结构容易被重新打散；
> - 终点可能来回跳变。
> 
> 因此工程上通常做法是：
> 
> - 第一次全量构图；
> - 后续帧做稳定更新。
> 
> ---
> 
> ## 它可能更新哪些内容
> 
> ### 1）边界点列
> 根据车辆当前位置和新感知，调整左右边界的可用段。
> 
> ### 2）终点位姿
> 如果车已经接近终点或者通道发生变化，需要修正 `EndPos`。
> 
> ### 3）锚点坐标系
> 如果车辆已经大幅移动，可能触发 anchor 切换或局部重定位。
> 
> ### 4）边界有效长度
> 随着车辆向外走，原来在车后方的边界段可能不再重要，前方边界段更重要。
> 
> ### 5）侵占状态
> 重新检查：
> 
> - 是否有新障碍进入通道；
> - 终点是否被占；
> - 当前边界是否还能容纳车辆。
> 
> ---
> 
> ## 它和 `CalMapInfo()` 的差别
> 
> ### `CalMapInfo()`
> 是**从零开始生成一版完整地图**。
> 
> ### `UpDataMapInfo()`
> 是**基于已有地图做局部刷新**。
> 
> 你可以把它理解为：
> 
> - `CalMapInfo()` 是初始化地图；
> - `UpDataMapInfo()` 是运行时维护地图。
> 
> ---
> 
> ## 这个函数里常见的工程动作
> 通常会有很多“平稳性”处理，比如：
> 
> - 对终点位置做限幅，避免一步跳太多；
> - 对边界更新做低通滤波；
> - 如果新感知不稳定，保留上一帧结果；
> - 如果当前车已明显出位，逐渐降低车位内部边界权重。
> 
> ---
> 
> # 二十一、`APAMap_ParkingOutCheckIfCarPosIsValid()`
> 
> 这个函数是**当前车辆姿态合法性检查**。
> 
> 它回答的问题是：
> 
> > **以当前车辆的位置和姿态，泊出建图结果是否还成立？**
> 
> ---
> 
> ## 为什么这个检查很重要
> 因为泊出建图默认有一些前提，比如：
> 
> - 车确实在目标车位附近；
> - 车姿态和车位关系没有完全错乱；
> - 车没有偏离得太离谱；
> - 车仍处于泊出任务可控范围。
> 
> 如果这些前提不成立，那么地图即使算出来，也不一定能用。
> 
> ---
> 
> ## 它可能检查的内容
> 
> ### 1）车是否在车位合理区域内
> 比如：
> 
> - 车中心是否落在车位框附近；
> - 车头/车尾是否大致还在边界允许范围内。
> 
> 如果车已经明显不在该车位内，说明目标车位或车姿识别可能错了。
> 
> ---
> 
> ### 2）车辆朝向是否过于异常
> 例如：
> 
> - 车位方向和当前车角度差太大；
> - 当前车姿与预期泊出模式矛盾；
> - 已经转得太离谱，不适合用当前建图继续。
> 
> ---
> 
> ### 3）车与边界是否初始就冲突
> 如果地图刚建好，当前车身就已经压边或与边界冲突，那说明：
> 
> - 地图有问题；
> - 车位边界有问题；
> - 或当前车姿与目标车位不一致。
> 
> ---
> 
> ### 4）车是否处于可规划区间
> 比如离终点太近、太远、位置越界等，也可能被判无效。
> 
> ---
> 
> ## 这个函数失败通常意味着什么
> 往往说明是“**任务前提错误**”，而不是单纯边界小修能解决的问题。  
> 比如：
> 
> - 锁错车位；
> - 感知车位和真实车姿不一致；
> - 车已经不在预期泊出工况里；
> - 请求参数与现场状态不匹配。
> 
> ---
> 
> # 二十二、`APAMAP_Setfailcause(...)`
> 
> 这个虽然不是算法函数，但很值得单独说一下。
> 
> 它本质上是：
> 
> > **故障原因上报码 / 失败原因记录器**
> 
> ---
> 
> ## 它的重要性
> 在车载系统里，失败不能只说“失败了”，必须能回答：
> 
> - 是没找到车位？
> - 是车位太窄？
> - 是终点被侵占？
> - 是边界点数量不够？
> - 是当前车姿非法？
> - 是传感器数据无效？
> 
> 所以这种 fail cause 非常关键，既用于：
> 
> - 调试定位
> - HMI 提示
> - 上层状态机处理
> - 数据回灌分析
> 
> ---
> 
> ## 你前面提到过的一些 fail cause 语义
> 
> ### `2`
> 大概率是：
> - 没找到请求的车位；
> - SlotID 匹配失败。
> 
> ### `58`
> 大概率是：
> - 车位边界收缩后空间不足；
> - 安全宽度不够。
> 
> 当然这些编号的精确定义还是要看宏或枚举表。
> 
> ---
> 
> ## 它在系统里的作用
> 这个函数是把“算法结果”变成“系统决策依据”的桥梁。
> 
> 因为上层状态机通常不会读懂复杂几何，只会看：
> 
> - 成功/失败
> - 失败原因码
> - 是否允许重试/回退/退出
> 
> ---
> 
> # 二十三、几个你前面提到但还没展开的底层偏移函数
> 
> 前面在 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 里，你提到几个很关键的 helper。  
> 这些函数虽然像“子函数”，但实际上决定了车位修边的质量。
> 
> ---
> 
> ## 1）`APAMap_CalSlotBorderPtOffsetBySensorMapInfo(...)`
> 
> 这个函数通常是：
> 
> > 根据近距离传感器信息，计算车位边界点应该收缩多少。
> 
> 这里的 Sensor 往往包含：
> 
> - 超声 USS
> - PDC
> - 近距障碍检测
> - 车位口边沿探测
> 
> ---
> 
> ### 它大概在做什么
> 
> - 找出车位边界附近的最近物体；
> - 算这个物体相对 `Obj1/Obj2` 的法向距离；
> - 如果太近，就给出一个 inward offset；
> - 用来缩小车位边界或修正车位口位置。
> 
> ---
> 
> ### 它的特点
> 近距传感器对“很近的实际侵占”很敏感，  
> 特别适合修：
> 
> - 轮挡
> - 近墙
> - 侧向停靠车辆
> - 车位边沿附近的小障碍
> 
> 但它覆盖范围有限，所以通常只负责**近场保守修正**。
> 
> ---
> 
> ## 2）`APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(...)`
> 
> 这个函数是：
> 
> > 根据顶视图 FSD 结果，计算车位边界偏移量。
> 
> ---
> 
> ### 它擅长修什么
> - 视觉上看见的边线
> - 路沿
> - 停车线
> - 地面结构边界
> - 停车场内规则轮廓
> 
> ---
> 
> ### 它的本质
> 和 Sensor 不同，它更像是“**连续形状约束**”。
> 
> Sensor 更像最近点保护；  
> FSD 更像空间边缘拟合。
> 
> 所以它给出的 offset 往往对边界整体更平顺、更贴合真实轮廓。
> 
> ---
> 
> ## 3）`APAMap_CalSlotBorderPtOffsetByODMapInfo(...)`
> 
> 这个函数是：
> 
> > 根据障碍物检测结果，计算车位边界点偏移量。
> 
> ---
> 
> ### 它重点考虑什么
> 不是边线，而是实体障碍：
> 
> - 相邻车辆
> - 障碍桶
> - 墙角
> - 柱子
> - 行人区域外围包络
> 
> ---
> 
> ### 它和 Sensor/FSD 的互补关系
> - Sensor：近距离强保护
> - FSD：边界形状约束
> - OD：实体目标侵占约束
> 
> 三者最后一起取最大或最保守偏移，就形成较鲁棒的车位边界收缩结果。
> 
> ---
> 
> # 二十四、与几何计算相关的通用函数
> 
> 你前面提到过一些 `AlgCom_*` 之类函数。  
> 这些不是泊出业务函数，但在理解逻辑时很关键。
> 
> ---
> 
> ## 1）`AlgCom_LineParABCByCurrentCarPosition(...)`
> 这类函数一般是：
> 
> > 根据当前车位置姿态，求一条参考直线的 ABC 参数。
> 
> 常用于：
> 
> - 构造车身方向线；
> - 构造与车位方向平行/垂直的线；
> - 方便后面求点到线距离、平移直线、求交点。
> 
> ---
> 
> ## 2）`AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(...)`
> 这类函数一般是：
> 
> > 已知一条线和目标角度/平移距离，求其平行线参数。
> 
> 这是边界偏移里非常常见的操作：
> 
> - 原边界往里平移一段；
> - 构造安全余量边界；
> - 用偏移线和另一条边求新的角点。
> 
> ---
> 
> ## 3）`AlgCom_CrossPointOfTwoLines(...)`
> 这类函数就是：
> 
> > 求两条直线交点。
> 
> 在车位几何更新中非常常见，特别是：
> 
> - 通过两条偏移边界线求新 `Obj1Pt`
> - 求新 `Obj2Pt`
> - 求车位口四角
> 
> ---
> 
> ## 这些底层几何函数意味着什么
> 说明这套代码不是简单做点位硬编码，而是：
> 
> > **用几何直线模型去构造和修正车位与边界。**
> 
> 这通常代表算法结构比较成熟，方便做：
> 
> - 平行/垂直偏移
> - 不同泊出模式统一处理
> - 边界与终点一致性维护
> 
> ---
> 
> # 二

#### _User_

> 继续 不要听

#### _Assistant_

> **Continuing analysis in Chinese**
> 
> I think the user likely wants an answer in Chinese. The phrase "继续 不要听" might be a typo, and it seems like they might mean "继续，不要停," which translates to "continue, don't stop." So, I should proceed with the analysis without any tools. I’ll be careful to keep the response concise while touching on the remaining functions, possible call graph, and failure modes. Let's get this right!**Continuing analysis in Chinese**
> 
> I think the user likely wants an answer in Chinese. The phrase "继续 不要听" might be a typo, and it seems like they might mean "继续，不要停," which translates to "continue, don't stop." So, I should proceed with the analysis without any tools. I’ll be careful to keep the response concise while touching on the remaining functions, possible call graph, and failure modes. Let's get this right!

#### _Assistant_

> 好，我继续。你这句“不要听”我理解成“**不要停**”，那我就继续按源码架构深挖。
> 
> 上次讲到 **二十四：底层几何函数**，下面继续往更深一层拆：  
> 我会把这套泊出建图代码从“函数解析”推进到“**真实执行语义**”。
> 
> ---
> 
> # 二十五、这套代码的“真实控制逻辑”到底是什么
> 
> 如果把所有函数串起来，这个模块实际上在做 4 件事：
> 
> ---
> 
> ## 1）锁定目标车位
> 由这些函数负责：
> 
> - `APAMap_ParkingOutCalMapSlotPar()`
> - `APAMap_ParkingOutCalSlotInfo()`
> 
> 目标是得到：
> 
> - 车位是哪一个
> - 车位在哪
> - 车位边界口在哪
> - 泊出是往哪边出
> 
> ---
> 
> ## 2）把车位变成“通道”
> 由这些函数负责：
> 
> - `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> - `APAMap_ParkingOutCalMapInfo()`
> 
> 目标是得到：
> 
> - 左右边界
> - 主边界/副边界
> - 初始可行驶通道
> 
> ---
> 
> ## 3）把“理论通道”改成“真实通道”
> 由这些函数负责：
> 
> - `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> - `APAMap_FusBoundaryByODMapInfo()`
> 
> 目标是：
> 
> - 靠真实感知修边
> - 让边界更像真实道路/车位口/障碍情况
> 
> ---
> 
> ## 4）找一个真正能停的终点
> 由这些函数负责：
> 
> - `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> - `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> - `APAMap_ParkingOutCenterEndCarPosInfo()`
> - `APAMap_ParkingOutEndCarPosUpdata()`
> 
> 目标是：
> 
> - 终点不能只“理论上对”
> - 还得“真能放下一辆车”
> - 而且姿态要合理、边界不能侵占
> 
> ---
> 
> 所以这整套代码不是单纯“建图”，更准确说是：
> 
> > **“泊出场景下的目标车位建模 + 通道边界构建 + 终点修正”模块**
> 
> ---
> 
> # 二十六、`Obj1Pt / Obj2Pt` 在整个系统里到底扮演什么角色
> 
> 前面已经讲过它们是关键边界点，但这里我再更系统一点解释。
> 
> ---
> 
> ## 1）它们不是普通点，而是“车位口两侧支点”
> 从使用方式看，`Obj1Pt / Obj2Pt` 很像：
> 
> - 车位开口边两端点；
> - 或车位两侧最关键的参考边界点；
> - 是后续生成边界、终点、朝向的几何支撑点。
> 
> 换句话说：
> 
> 如果没有这两个点，后面很多几何就无从展开。
> 
> ---
> 
> ## 2）它们决定默认通道的起始形状
> `CalBoundaryByParkOutInfo()` 基本一定会依赖：
> 
> - Obj1
> - Obj2
> - 车位方向
> 
> 来把边界从车位口“拉出去”。
> 
> 也就是说：
> 
> - Obj1/Obj2 决定通道从哪里开始
> - 通道第一段的方向和宽度也往往跟它们有关
> 
> ---
> 
> ## 3）它们也影响终点的位置
> 终点通常不是凭空选出来，而是跟“车位口中心”和“车位朝向”强相关。
> 
> 而车位口中心大概率就是：
> 
> ```text
> (Obj1Pt + Obj2Pt) / 2
> ```
> 
> 或某种由二者派生的中点/法向外扩点。
> 
> 所以：
> 
> > **Obj1/Obj2 错了，EndPos 大概率也会错。**
> 
> ---
> 
> ## 4）它们还是多源融合的“落脚点”
> FSD/OD/Sensor 最后往往不是直接改整个地图，而是先改：
> 
> - Obj1 偏一点
> - Obj2 缩一点
> - SlotBordPt 某个角点修一点
> 
> 再由这些新几何量重新派生边界和终点。
> 
> 这就是为什么 `CalSlotBorderPtByFSDAndODMapInfo()` 很核心。
> 
> ---
> 
> # 二十七、`SlotBordPt[0..3]` 很可能怎么定义
> 
> 虽然你没贴结构体，但从调用关系能大致推断。
> 
> ---
> 
> ## 一种非常常见的定义方式是：
> 
> - `SlotBordPt[0]`：车位口一侧角点
> - `SlotBordPt[1]`：车位口另一侧角点
> - `SlotBordPt[2]`：车位内侧某角点
> - `SlotBordPt[3]`：另一内侧角点
> 
> 也就是一个四边形车位框。
> 
> ---
> 
> ## 为什么前两个点特别重要
> 因为从你的描述看：
> 
> ```cpp
> SlotBordPt[0] = Obj2Pt;
> SlotBordPt[1] = Obj1Pt;
> ```
> 
> 这很像是在说：
> 
> - `[0] [1]` 就是车位口或关键边界口
> - `[2] [3]` 是通过延展/几何投影得到的另一条边
> 
> 所以在泊出场景中：
> 
> - `[0][1]` 决定“从哪里出去”
> - `[2][3]` 决定“车位内部有多深、多宽”
> 
> ---
> 
> ## 它对规划的直接作用
> 规划器不一定直接用 `SlotBordPt` 做轨迹，但它强烈影响：
> 
> - 边界起点
> - 出位方向
> - 当前是否仍在车位内
> - 车位是否足够宽深
> 
> 所以 `SlotBordPt` 更像是**车位几何的真值载体**。
> 
> ---
> 
> # 二十八、为什么要有 `MainBoundary` 和 `SubBoundary`
> 
> 这不是多余设计，反而很像成熟工程代码的典型结构。
> 
> ---
> 
> ## 1）因为两侧边界重要性不一样
> 泊出时，靠车位那一侧和远离车位那一侧，意义完全不同。
> 
> ### 靠车位一侧
> 通常更复杂，因为这里可能有：
> 
> - 相邻车
> - 墙
> - 车位线
> - 近距障碍
> - 车位口边界
> 
> 所以这侧更需要精细修正。
> 
> ### 远离车位一侧
> 通常更像通道开放区域，边界更“宽松”。
> 
> 所以把两侧分成：
> 
> - MainBoundary
> - SubBoundary
> 
> 是合理的。
> 
> ---
> 
> ## 2）主边界经常被重点融合
> 例如：
> 
> - FSD 更可能优先修主边界
> - RefercLine 更可能优先修主边界
> - 车位口附近障碍更可能体现在主边界
> 
> 副边界很多时候只是提供通道宽度约束。
> 
> ---
> 
> ## 3）最终输出时再映射成 Left/Right
> 内部算法可能不关心“左/右”这个绝对概念，  
> 它更关心：
> 
> - 哪边是贴车位的一边
> - 哪边是另一边
> 
> 最后再根据 `parkside` 映射成：
> 
> - `LeftBoundary`
> - `RightBoundary`
> 
> 这能让代码适配左右泊出，而不用写两套。
> 
> ---
> 
> # 二十九、`bAfterNewAnchorPointFlag` 这类标志意味着什么
> 
> 你前面提到过这个变量，我这里专门解释它背后的工程思想。
> 
> ---
> 
> ## 1）说明系统会发生“参考锚点切换”
> 也就是说，泊出过程中：
> 
> - 一开始地图以车位或初始车身为参考
> - 后面车开出一段后，这个参考系可能不再稳定或不再方便
> - 系统切到新的 anchor point 来继续维护地图
> 
> 这在泊车系统里很常见。
> 
> ---
> 
> ## 2）为什么要切 anchor
> 因为泊出不是静态场景：
> 
> - 车一直在动
> - 地图范围会从“车位内部”扩展到“道路通道”
> - 原始车位坐标系可能越来越不适合描述后续道路边界
> 
> 所以一旦车“明显出位”，系统就可能说：
> 
> > 现在不要再死盯着车位了，改用新的局部参考系描述通道。
> 
> ---
> 
> ## 3）这会影响哪些东西
> anchor 切换会影响：
> 
> - Obj1/Obj2 是否继续参与强约束
> - 终点是否还按旧车位几何来算
> - 边界是否继续以内侧车位边为主
> - 各种 offset 是否还继续压缩原车位边界
> 
> 这也解释了你前面看到的现象：
> 
> 在某些条件下 offset 会被清零。
> 
> 因为当车已经出位后，再纠结车位内部边界没意义了。
> 
> ---
> 
> # 三十、`LaneLine` 和 `RefercLine` 为什么都能改终点角度
> 
> 这个设计其实很有代表性，说明系统在处理“**局部结构方向**”和“**道路通行方向**”的融合。
> 
> ---
> 
> ## 1）终点角度不能只看车位
> 如果只按车位方向定终点角，会有问题：
> 
> - 出位后可能斜着横在通道里
> - 和道路方向不一致
> - 后续接管或继续行驶不自然
> 
> 所以终点角度必须考虑道路或通道的真实方向。
> 
> ---
> 
> ## 2）LaneLine 是道路方向的强证据
> 如果车道线可靠，那么终点朝向最应该靠近它。
> 
> 这很合理，因为车道线反映：
> 
> - 道路主轴
> - 通行规则方向
> - 外部驾驶语义
> 
> ---
> 
> ## 3）RefercLine 是局部结构方向的补充证据
> 但很多停车场里：
> 
> - 车道线未必清楚
> - 地面反光
> - 道路不是标准城市车道
> 
> 这时参考线可能更稳定。
> 
> 比如：
> 
> - 停车区引导线
> - 车位延长线
> - 通道结构边
> 
> 所以 RefercLine 可以作为 LaneLine 的补充甚至替代。
> 
> ---
> 
> ## 4）这两者大概率有优先级仲裁
> 很可能存在这类逻辑：
> 
> - 如果 LaneLine 已经成功更新过终点角，RefercLine 不轻易覆盖
> - 除非 RefercLine 更近、更长、更稳定
> - 若 LaneLine 缺失，则 RefercLine 接管
> 
> 这也是为什么会有类似：
> 
> - `bLaneLineUpdateEndPosFlag`
> - `bReferLineUpdateEndPosFlag`
> 
> 这样的状态量。
> 
> ---
> 
> # 三十一、这套代码里的“终点”其实不止一个层次
> 
> 你看到的是 `EndPos`，但真实逻辑上，它可能有 3 层概念。
> 
> ---
> 
> ## 1）几何默认终点
> 由这些函数产生：
> 
> - `SetEndCarPosInOldCorSys()`
> - `SetEndCarPosInOldCorSysByUWB()`
> 
> 它回答的是：
> 
> > “按车位和模式看，车应该大致停到哪里？”
> 
> ---
> 
> ## 2）边界约束终点
> 由这些函数修正：
> 
> - `BoundarySeizeEndCarPosInfo()`
> - `CenterEndCarPosInfo()`
> - `EndCarPosUpdata()`
> 
> 它回答的是：
> 
> > “这个终点在当前边界里真的放得下吗？”
> 
> ---
> 
> ## 3）运行时动态终点
> 由这些函数持续维护：
> 
> - `UpDataMapInfo()`
> - 以及车道线/参考线融合函数
> 
> 它回答的是：
> 
> > “随着车在运动，这个终点要不要微调？”
> 
> ---
> 
> 所以 `EndPos` 不是一次算完就固定，而是：
> 
> > **默认生成 → 可行性检查 → 动态更新**
> 
> ---
> 
> # 三十二、为什么 `APAMap_ParkingOutTask()` 要区分“首次建图”和“更新帧”
> 
> 这个设计很关键。
> 
> ---
> 
> ## 1）首次建图要解决“有没有地图”
> 首次建图时，系统还不知道：
> 
> - 目标车位到底是哪一个
> - Obj1/Obj2 是什么
> - 边界怎么起
> - EndPos 在哪
> 
> 所以必须做完整流程：
> 
> - 选车位
> - 算车位边界
> - 算地图
> - 检查合法性
> 
> ---
> 
> ## 2）更新帧只解决“地图需不需要跟着车动”
> 一旦这些初始量确定了，后续就不需要每帧重来。
> 
> 原因：
> 
> - 重建太容易抖
> - 传感器每帧有噪声
> - 规划器更喜欢稳定地图
> 
> 所以更新帧大概率做：
> 
> - 小幅修边界
> - 小幅修终点
> - 更新当前有效边界段
> - 检查新的障碍侵占
> 
> ---
> 
> ## 3）这体现的是“稳定优先”原则
> 车载规划最怕的是地图跳。
> 
> 比如上一帧终点在这里，下一帧又跳 30cm，规划器就会：
> 
> - 轨迹频繁重算
> - 转角来回变
> - 用户感觉车身犹豫
> 
> 所以区分“初始化”和“维护”是非常合理的。
> 
> ---
> 
> # 三十三、如果从“失败原因”反推源码分支，可以怎么理解
> 
> 你前面提到了一些 fail cause，我这里把常见失败类目抽象一下。
> 
> ---
> 
> ## 1）车位获取失败类
> 出现在：
> 
> - `CalMapSlotPar()`
> 
> 可能原因：
> 
> - `SlotID` 没找到
> - 感知车位源为空
> - 指定融合模式不支持
> - 车位索引非法
> 
> 特点：  
> **任务无法起步**
> 
> ---
> 
> ## 2）车位几何构造失败类
> 出现在：
> 
> - `CalSlotInfo()`
> - `CalSlotBorderPtByParkOutSlotInfo()`
> - `CalSlotBorderPtByParkOutInfo()`
> 
> 可能原因：
> 
> - 车位框角点异常
> - 车位宽深不合理
> - 边界点交叉
> - 推导出的 Obj1/Obj2 不合法
> 
> 特点：  
> **车位有，但几何不可用**
> 
> ---
> 
> ## 3）边界收缩后空间不足类
> 出现在：
> 
> - `CalSlotBorderPtByFSDAndODMapInfo()`
> 
> 可能原因：
> 
> - 左右压缩后剩余宽度太小
> - 障碍侵占严重
> - 相邻车辆/墙太近
> 
> 特点：  
> **车位建出来了，但实际不安全**
> 
> ---
> 
> ## 4）边界点不足类
> 出现在：
> 
> - 非首次建图且 `LeftBoundary/RightBoundary.PtNum < 2`
> 
> 特点：  
> **地图维护失败，边界无法构成有效折线**
> 
> ---
> 
> ## 5）当前车姿不合法类
> 出现在：
> 
> - `CheckIfCarPosIsValid()`
> 
> 特点：  
> **地图和现实车姿不一致，继续规划风险大**
> 
> ---
> 
> ## 6）终点侵占且修复失败类
> 出现在：
> 
> - `BoundarySeizeEndCarPosInfo()`
> - `CenterEndCarPosInfo()`
> - `EndCarPosUpdata()`
> 
> 特点：  
> **车位和通道都存在，但找不到合适的终点**
> 
> ---
> 
> # 三十四、如果你要读源码，最值得重点盯的变量有哪些
> 
> 如果你下一步准备把 `.c/.cpp` 贴出来，我建议优先盯这些量，因为它们最能暴露逻辑主线。
> 
> ---
> 
> ## 1）车位类核心变量
> - `SlotID`
> - `SlotIndex`
> - `FusSlotIndex`
> - `SlotSide`
> - `SlotLen`
> - `SlotDepth`
> 
> ---
> 
> ## 2）几何核心变量
> - `Obj1Pt`
> - `Obj2Pt`
> - `Obj1Ang`
> - `Obj2Ang`
> - `SlotBordPt[4]`
> 
> ---
> 
> ## 3）终点核心变量
> - `EndPos`
> - `EndPosLine`
> - `bLaneLineUpdateEndPosFlag`
> - `bReferLineUpdateEndPosFlag`
> - `bSeizeEndCarPosFlag`
> 
> ---
> 
> ## 4）边界核心变量
> - `MainBoundary`
> - `SubBoundary`
> - `LeftBoundary`
> - `RightBoundary`
> - `PtNum`
> 
> ---
> 
> ## 5）坐标系切换变量
> - `NewCordSysOPt`
> - `NewCordSysAng`
> - `bAfterNewAnchorPointFlag`
> 
> ---
> 
> ## 6）多源融合变量
> - `OffsetX1/OffsetY1`
> - `OffsetX2/OffsetY2`
> - `FSDOffset`
> - `ODOffset`
> - `SensorOffset`
> 
> ---
> 
> 这些量一旦你贴出来，我基本可以把每一段分支都翻译成人话。
> 
> ---
> 
> # 三十五、如果按“源码阅读顺序”，下一步最适合看哪几个函数
> 
> 如果你准备继续看源文件，我建议按这个顺序读，而不是从头乱翻。
> 
> ---
> 
> ## 第一层：总控
> 1. `APAMap_ParkingOutTask()`
> 2. `APAMap_ParkingOutCalMapInfo()`
> 3. `APAMap_ParkingOutUpDataMapInfo()`
> 
> 先搞清整体调度。
> 
> ---
> 
> ## 第二层：车位构造
> 4. `APAMap_ParkingOutCalMapSlotPar()`
> 5. `APAMap_ParkingOutCalSlotInfo()`
> 6. `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> 这三者决定“地图起点”。
> 
> ---
> 
> ## 第三层：边界融合
> 7. `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 8. `APAMap

#### _User_

> 继续不要停

#### _Assistant_

> 好，我继续，接着上次的 **三十五** 往下讲，而且这次我不只列函数名，我把它们之间的“隐含逻辑”和“工程意图”也一起展开。
> 
> 上次停在源码阅读顺序的第三层，我从那里接着说。
> 
> ---
> 
> # 三十五、如果按“源码阅读顺序”，下一步最适合看哪几个函数
> 
> 上次已经说到：
> 
> - 总控层
> - 车位构造层
> - 边界融合层
> 
> 我把第三层补完，再继续第四层。
> 
> ---
> 
> ## 第三层：边界融合
> 建议重点看：
> 
> 7. `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 8. `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> 9. `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> 10. `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> 11. `APAMap_FusBoundaryByODMapInfo()`
> 
> 为什么这一层很重要？
> 
> 因为这里决定了一个核心问题：
> 
> > 地图到底是“模板地图”，还是“感知地图”。
> 
> 如果你看源码发现：
> - FSD/LaneLine/OD 只是轻微修饰默认边界，  
> 那么这个系统偏“模板主导”。
> 
> 如果你看源码发现：
> - 感知边界能大面积替换默认边界，  
> 那么这个系统偏“感知主导”。
> 
> 这两种系统风格差别很大。
> 
> ---
> 
> ## 第四层：终点修正层
> 建议接着看：
> 
> 12. `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> 13. `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> 14. `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> 15. `APAMap_ParkingOutCenterEndCarPosInfo()`
> 16. `APAMap_ParkingOutEndCarPosUpdata()`
> 
> 这层决定的是：
> 
> > 规划器最终要追的目标位姿，究竟怎么来的。
> 
> 这是泊出系统里最容易“感觉怪”的地方。  
> 用户看到的往往不是边界，而是：
> 
> - 为什么车停到这里？
> - 为什么车出位后还要再摆一下？
> - 为什么终点偏左/偏右？
> - 为什么这次停得更正？
> 
> 答案通常就在这几个函数里。
> 
> ---
> 
> ## 第五层：保护与状态层
> 最后再看：
> 
> 17. `APAMap_ParkingOutCheckIfCarPosIsValid()`
> 18. `APAMAP_Setfailcause(...)`
> 
> 这层虽然不是主算法，但能帮助你快速判断：
> 
> - 哪些分支被作者认为是“严重错误”
> - 哪些情况只是“继续修正”
> - 哪些情况会直接退出泊出任务
> 
> ---
> 
> # 三十六、`APAMap_ParkingOutUpDataMapInfo()` 更深一层可能在做什么
> 
> 前面只是宏观解释过这个函数，现在我再更细一点，把它拆成“可能的内部阶段”。
> 
> 这个函数很可能包含下面 5 段逻辑。
> 
> ---
> 
> ## 1）根据当前车身位置裁剪边界有效段
> 随着车往外开，原来靠近车尾、位于车后方的边界段，意义会越来越小。  
> 更新函数可能会：
> 
> - 删除已经越过的后向边界点；
> - 保留前方更长的边界；
> - 重设边界起点，让地图“跟着车向前滚动”。
> 
> 这和局部地图更新很像。
> 
> ---
> 
> ## 2）根据新感知对局部边界做替换
> 不是全图替换，而是只替换当前车附近或前方若干米的边界段。  
> 原因是：
> 
> - 当前附近区域最影响规划；
> - 远处感知不稳定；
> - 全量替换会导致地图跳变。
> 
> 所以这个函数里如果你看到：
> - 只更新某几个 boundary 点
> - 只更新点数的一部分
> - 对新旧边界做插值融合
> 
> 这是非常合理的。
> 
> ---
> 
> ## 3）重算或微调终点
> 随着车向外走，终点可能要从“车位出口前方一点”逐渐调整为“通道中居中位置”。
> 
> 这时 `UpDataMapInfo()` 很可能会检查：
> 
> - 当前车距终点是否过近
> - 终点是否仍在边界有效范围内
> - 当前终点是否被新障碍侵占
> 
> 然后再调用：
> - `EndCarPosUpdata()`
> - 或重新做一轮 `BoundarySeizeEndCarPosInfo()`
> 
> ---
> 
> ## 4）处理“从车位阶段”到“道路阶段”的逻辑切换
> 这通常是最像状态机的部分。
> 
> 比如车辆刚开始泊出时，系统更关心：
> - 车位口宽不宽
> - 左右邻车近不近
> 
> 但当车辆已经出了大半时，系统更关心：
> - 通道左右边界
> - 道路方向
> - 前方障碍
> 
> 于是更新函数可能会逐渐降低以下量的权重：
> - SlotBordPt
> - 车位内部边界
> - Obj1/Obj2 对终点的直接约束
> 
> 同时提高：
> - LaneLine
> - RefercLine
> - 通道边界
> - OD 前向障碍
> 
> 这就是典型的**阶段切换**。
> 
> ---
> 
> ## 5）保持地图平稳
> 这个函数里很可能有大量“限幅/滤波/保持上一帧”的逻辑。
> 
> 你如果看到类似思路，不要觉得“啰嗦”，这是车载工程必须的：
> 
> - 新边界变化太大时，不直接采用；
> - 新终点跳动太大时，缓慢逼近；
> - 某一帧感知丢失时，保留上一帧结果；
> - 边界点数突然少时，不立即清空地图。
> 
> 这部分往往比几何公式还重要，因为它决定了实际车感是否平顺。
> 
> ---
> 
> # 三十七、`APAMap_ParkingOutCheckIfCarPosIsValid()` 很可能有哪些具体判据
> 
> 前面说了它是合法性检查，现在更细化一下，推测它内部可能包含的判据类型。
> 
> ---
> 
> ## 1）点到车位框的距离判据
> 看当前车中心点、前轴中心或车尾点，距离目标车位中心/边界是否合理。
> 
> 比如：
> 
> - 离车位中心太远
> - 车已经偏离车位很多
> - 车体大部分不在车位合理包围区内
> 
> 都会触发 invalid。
> 
> ---
> 
> ## 2）车身姿态与车位姿态夹角判据
> 这通常是最常见的一类。
> 
> 如果当前车角度与车位主方向差值过大，例如：
> - 平行位却出现接近垂直的车姿
> - 垂直位却和车位法向差很多
> 
> 则可能认为：
> - 目标车位不是当前车所在车位
> - 或当前姿态已超出该泊出模板适用范围
> 
> ---
> 
> ## 3）车辆与边界的重叠判据
> 也就是地图刚生成完，就先看看：
> 
> - 当前车矩形有没有压左边界
> - 有没有压右边界
> - 有没有已经和障碍重叠
> 
> 如果一开始就重叠，那说明：
> - 地图边界太激进
> - 或车位几何与真实车身不一致
> 
> 这是一个很典型的 sanity check。
> 
> ---
> 
> ## 4）车辆是否越过关键断面
> 例如可能存在一个“车位口断面”：
> - 如果车已经完全越过，却还在使用“车位内”逻辑，可能 invalid
> - 或反过来，车根本没在预期车位内，却被当成可泊出状态，也 invalid
> 
> ---
> 
> ## 5）与请求参数一致性检查
> 比如请求说：
> - 右侧泊出
> - 某个 SlotId
> - 某种泊出模式
> 
> 但当前车姿显示更像是：
> - 车在左侧车位
> - 或停姿与该模式不符
> 
> 那就可能判请求与现场不一致。
> 
> ---
> 
> # 三十八、这套代码很可能存在“阈值驱动”的大量经验参数
> 
> 这类 APA 工程代码的一个核心特点是：
> 
> > 看起来是几何算法，实际上到处都是经验阈值。
> 
> 你如果以后贴源码，十有八九会看到很多类似：
> 
> - 最小边界点数阈值
> - 角度差阈值
> - 长度阈值
> - 宽度阈值
> - 点间距阈值
> - 障碍侵占阈值
> - 终点平移步长
> - 最大迭代次数
> - 置信度阈值
> 
> ---
> 
> ## 这些参数一般分成三类
> 
> ### 1）安全类参数
> 例如：
> - 最小剩余通道宽度
> - 与障碍的最小间距
> - 终点姿态包络安全余量
> 
> 这类参数通常非常保守。
> 
> ---
> 
> ### 2）稳定性参数
> 例如：
> - 终点变化限幅
> - 边界更新滞回
> - 地图点数最小要求
> - 感知替换阈值
> 
> 这类参数决定“车会不会抖”。
> 
> ---
> 
> ### 3）场景识别参数
> 例如：
> - 平行位和垂直位的角度分界
> - 是否已经出位的距离阈值
> - 是否启用新 anchor 的距离阈值
> 
> 这类参数决定“逻辑分支走哪条”。
> 
> ---
> 
> ## 为什么这些参数很重要
> 因为很多时候你看到函数名很漂亮，但真实行为往往由阈值决定。
> 
> 例如：
> - 是用 LaneLine 还是不用，可能只差 1 个角度阈值；
> - 终点是否居中，可能只差 10cm；
> - 是否判失败，可能只差一个安全余量。
> 
> 所以读源码时，除了看流程，还一定要盯这些常量。
> 
> ---
> 
> # 三十九、`FSD / OD / LaneLine / RefercLine` 的融合优先级很可能是什么
> 
> 虽然你没贴具体代码，但从典型架构和你前面描述看，我推测它们的优先级可能大致是这样。
> 
> ---
> 
> ## 对“终点角度”的优先级
> 大概率是：
> 
> 1. LaneLine
> 2. RefercLine
> 3. 车位方向/默认方向
> 4. 当前车身方向兜底
> 
> 原因是：
> - LaneLine 最代表道路方向
> - RefercLine 次之
> - 车位方向只能反映局部停位，不一定代表出位后的目标方向
> - 当前车身方向最不可靠，只能兜底
> 
> ---
> 
> ## 对“车位边界收缩”的优先级
> 大概率是：
> 1. Sensor/OD 保守约束
> 2. FSD 形状约束
> 3. 默认车位框
> 
> 原因是：
> - 障碍实体侵占必须先让路
> - FSD 可以修形状，但不能无视实体障碍
> - 默认车位框只是初值
> 
> ---
> 
> ## 对“运行时边界外延”的优先级
> 大概率是：
> 1. LaneLine / RefercLine
> 2. FSD
> 3. 默认通道模板
> 
> 因为出位后的通道更像道路/场地结构，而不是车位内部几何。
> 
> ---
> 
> ## 这种优先级说明了什么
> 说明整个系统其实在做两种约束的平衡：
> 
> - 结构约束：道路/车位应该长什么样
> - 安全约束：哪里有障碍、哪里不能去
> 
> 终点角度更偏结构约束；  
> 边界收缩更偏安全约束。
> 
> ---
> 
> # 四十、为什么 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` 会先修车位，再修地图
> 
> 这是个很值得注意的设计。
> 
> 很多人会直觉觉得：
> 
> > 直接对最终左右边界融合传感器不就行了？
> 
> 但这套代码不是这么做，它先修车位边界点，再去生成地图。  
> 这说明作者的思路是：
> 
> > **车位几何是地图的源头，先把源头修对，后面派生结果会更一致。**
> 
> ---
> 
> ## 这样做的好处
> 
> ### 1）终点和边界保持一致
> 如果你只修最终边界，不修车位本身，可能会出现：
> - 边界往内缩了
> - 但终点还按旧车位口在算
> 
> 那就会不一致。
> 
> 先修车位，再重新生成终点和边界，就能保持一致性。
> 
> ---
> 
> ### 2）不同泊出模式复用更容易
> 因为无论平行位、垂直位还是斜列位，最终都可以抽象为：
> - 先得到更真实的 `Obj1/Obj2`
> - 再用统一逻辑生成边界和终点
> 
> 这比直接在最终边界层做大量分支要干净。
> 
> ---
> 
> ### 3）便于失败早发现
> 如果车位本身被修到已经不够容车了，可以直接 fail。  
> 而不是等到规划器那边才发现通道不合理。
> 
> ---
> 
> # 四十一、这套代码很可能有一个“车位内阶段”和“车位外阶段”
> 
> 虽然你没贴显式状态变量，但从逻辑看，几乎肯定存在这个隐含状态。
> 
> ---
> 
> ## 1）车位内阶段
> 特点：
> - 更依赖 Slot 信息
> - 更关注车位口宽度
> - 更关注左右邻车/近场障碍
> - EndPos 还和车位几何强相关
> 
> 典型函数权重高的有：
> - `CalMapSlotPar`
> - `CalSlotInfo`
> - `CalSlotBorderPtByFSDAndODMapInfo`
> 
> ---
> 
> ## 2）过渡阶段
> 特点：
> - 车已经部分出位
> - 旧车位几何还有效，但不再是唯一依据
> - anchor 可能开始切换
> - 通道边界开始比车位边界更重要
> 
> 典型信号：
> - `bAfterNewAnchorPointFlag`
> - 某些 offset 清零
> - EndPos 开始更多受 LaneLine / RefercLine 影响
> 
> ---
> 
> ## 3）车位外阶段
> 特点：
> - 主要关心道路通道
> - 终点主要受通道居中/道路方向影响
> - 原始车位信息更多只作为历史参考
> 
> 这时如果代码写得成熟，车位内部信息的权重会逐渐退场。
> 
> ---
> 
> ## 为什么这个分阶段重要
> 因为泊出不是一个静态问题。  
> 如果全程都按“车还在车位内”来算，就会出现：
> 
> - 出位后地图不自然
> - 终点还盯着车位
> - 道路方向接不顺
> 
> 所以这个“隐含状态机”非常关键。
> 
> ---
> 
> # 四十二、`EndPosLine` 很可能是什么
> 
> 你前面多次提到 `EndPosLine`，这玩意儿通常不只是一个附属量，它很可能承担以下几种作用之一。
> 
> ---
> 
> ## 1）终点车身朝向的几何表达
> `EndPos` 是点和角度，  
> 而 `EndPosLine` 很可能是将终点角度转换成一条参考直线，方便：
> 
> - 点到线距离计算
> - 与边界夹角计算
> - 平移终点时保持姿态一致
> 
> ---
> 
> ## 2）终点搜索时的约束线
> 例如终点被侵占后，系统可能不是乱找，而是沿一条参考线滑动搜索。
> 
> 这条线就很可能是 `EndPosLine`。
> 
> 比如：
> - 沿道路方向找
> - 沿车位口外法向找
> - 沿 LaneLine 方向找
> 
> ---
> 
> ## 3）终点与道路/边界的一致性校验线
> 如果终点朝向与车道线/参考线差异太大，就能通过比较 `EndPosLine` 和这些线的夹角来判断。
> 
> ---
> 
> ## 这个变量为什么值得看
> 如果你贴源码，这个量很可能是理解“终点怎么挪”的关键。  
> 因为很多时候终点更新不是直接改角度，而是：
> 
> - 先定一条线
> - 再在线上找一个合法的位置
> 
> ---
> 
> # 四十三、这套代码里“边界侵占终点”的判断，极可能不是简单矩形碰撞
> 
> 虽然表面看 `BoundarySeizeEndCarPosInfo()` 好像就是碰撞检查，但实际工程里往往更复杂。
> 
> ---
> 
> ## 可能不仅检查硬碰撞，还检查“安全压线”
> 例如即使终点车身矩形没有穿过边界，但如果距离边界太近，也可能判为 seize。
> 
> 因为规划终点不是只要“不撞”，还要：
> - 留足控制误差
> - 留足建图误差
> - 留足动态执行余量
> 
> 所以它很可能判断的是：
> - 几何重叠
> - 或最小安全距离不足
> 
> ---
> 
> ## 可能会分前后左右不同余量
> 因为车辆在终点的风险方向不同：
> 
> - 侧向余量通常最敏感
> - 前后余量可能略宽松或有不同规则
> 
> 所以这个函数里可能不是一个统一阈值，而是：
> - 左侧阈值
> - 右侧阈值
> - 前方阈值
> - 后方阈值
> 
> ---
> 
> ## 可能会考虑车辆朝向后的投影长度
> 终点车身不是固定轴对齐矩形，而是带角度的。  
> 因此和边界关系多半要通过坐标变换、投

#### _User_

> Continue your response exactly where you left off:

#### _Assistant_

> 影包络、点到线距离等方式来判断，而不是简单 AABB 碰撞。
> 
> 这也意味着：
> 
> > `BoundarySeizeEndCarPosInfo()` 很可能是一个“带姿态、带安全余量”的终点占用检测器。
> 
> ---
> 
> # 四十四、`APAMap_ParkingOutCenterEndCarPosInfo()` 和 `APAMap_ParkingOutEndCarPosUpdata()` 的区别
> 
> 这两个函数名字看起来都像“修终点”，但职责很可能不一样。
> 
> ---
> 
> ## 1）`CenterEndCarPosInfo()` 更像一次性“居中修正”
> 它的特点通常是：
> 
> - 修正方向明确：往通道中间挪
> - 目标明确：让左右余量更均衡
> - 动作幅度有限：不做大范围搜索
> 
> 它像是在说：
> 
> > “先别急着复杂搜索，先把终点摆正一点试试。”
> 
> ---
> 
> ## 2）`EndCarPosUpdata()` 更像迭代式“搜索更新”
> 它的特点通常是：
> 
> - 可以重复多次调用
> - 每次可能小步移动
> - 不一定只往中间，还可能沿通道方向挪
> - 可能带有步长、最大次数、方向切换等逻辑
> 
> 它像是在说：
> 
> > “居中还不够，那就开始搜索一个可落脚的位置。”
> 
> ---
> 
> ## 3）为什么要分成两个函数
> 因为这体现了一个很合理的修复层级：
> 
> ### 第一层：最小改动修复
> 先尝试把终点轻微居中，不破坏已有结构。
> 
> ### 第二层：系统性搜索修复
> 如果轻微修复不行，再做更大范围更新。
> 
> 这样做的好处是：
> - 终点更稳定
> - 不会一上来就大幅跳动
> - 更符合车载系统“先保守、后积极”的原则
> 
> ---
> 
> # 四十五、`APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()` 里最值得注意的其实不是边界，而是“线选择”
> 
> 如果以后你贴源码，这个函数里最值得读的通常不是最后怎么改边界，而是：
> 
> > 它到底怎么从很多条车道线里选出“可用于泊出”的那一条或两条。
> 
> 因为在停车场或低速通道中，LaneLine 数据通常并不总是干净。
> 
> ---
> 
> ## 它可能有哪些选择条件
> 
> ### 1）空间位置条件
> 例如只看：
> - 终点附近的线
> - 车前某个矩形区域内的线
> - 与当前通道相交或平行的线
> 
> ---
> 
> ### 2）长度条件
> 太短的线通常不可靠。  
> 所以常见会要求：
> - 点数不少于某个阈值
> - 拟合长度超过某个阈值
> 
> ---
> 
> ### 3）方向条件
> 如果线的角度和当前通道方向差太大，多半不会用。  
> 因为：
> - 横穿线可能是杂线
> - 垂直线可能不是车道方向
> 
> ---
> 
> ### 4）侧向位置条件
> 如果系统要用某一侧的 LaneLine 修主边界，它得先判断这条线位于：
> - 左侧
> - 右侧
> - 还是中间无意义区域
> 
> ---
> 
> ### 5）连续性或置信度条件
> 有些车道线即使存在，但断续严重或置信度低，也不适合拿来改终点。
> 
> ---
> 
> ## 为什么“选线”比“融合”更关键
> 因为一旦选错线：
> - 边界会歪
> - 终点角度会错
> - 整个地图会被错误强约束
> 
> 所以这类函数往往 70% 的价值在“筛线”，30% 才在“怎么融合”。
> 
> ---
> 
> # 四十六、`APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()` 里最值得注意的是“替代条件”
> 
> 这个函数的关键点通常是：
> 
> > 在什么情况下，参考线有资格替代 LaneLine 或默认方向。
> 
> 这很像一个仲裁问题。
> 
> ---
> 
> ## 可能的替代触发条件
> 
> ### 1）LaneLine 不可用
> 比如：
> - 没检测到
> - 线太短
> - 方向不稳
> - 位置不合理
> 
> 这时 RefercLine 就成为主要结构信息来源。
> 
> ---
> 
> ### 2）RefercLine 离车位口更近
> 参考线往往比道路车道线更接近局部场地结构。  
> 在车位口附近，RefercLine 可能更代表真实可行方向。
> 
> ---
> 
> ### 3）RefercLine 与当前车位几何更一致
> 如果参考线和车位方向高度一致，而 LaneLine 角度有明显偏差，系统可能更信参考线。
> 
> ---
> 
> ### 4）处于特定场景模式
> 比如：
> - 地下车库
> - 室内停车区
> - 非标准道路车道环境
> 
> 这种场景 LaneLine 的语义弱，而 RefercLine 的语义强。
> 
> ---
> 
> ## 所以这个函数你读源码时要找什么
> 重点找以下判断：
> 
> - 什么情况下直接 return，不用参考线
> - 什么情况下设置 `bReferLineUpdateEndPosFlag`
> - 它是否检查 `bLaneLineUpdateEndPosFlag`
> - 它是否有“角度更优/距离更近/长度更长”的比较逻辑
> 
> 这几处最能说明它在整个系统里的权重。
> 
> ---
> 
> # 四十七、`APAMap_FusBoundaryByODMapInfo()` 可能不仅改边界，还可能暗中影响终点搜索范围
> 
> 表面上这个函数是“障碍物融合边界”，但工程上它还有可能起到另一个作用：
> 
> > 缩小终点允许落点的搜索空间。
> 
> ---
> 
> ## 为什么会这样
> 因为如果某个障碍物侵入了终点附近区域，那么系统不一定马上判失败，它可能会：
> 
> - 先缩边界
> - 让终点搜索只能在更窄的区域内进行
> 
> 这样一来，后续 `BoundarySeizeEndCarPosInfo()` 或 `EndCarPosUpdata()` 就是在一个已经被障碍裁剪过的空间里找解。
> 
> ---
> 
> ## 这有什么好处
> 可以避免终点搜索在明显不可能的位置浪费迭代。
> 
> 也就是说，OD 融合不只是“地图更真实”，还是在提前告诉终点优化模块：
> 
> > “别往这些区域找了，那里已经被占了。”
> 
> ---
> 
> # 四十八、为什么这套代码里“地图平滑”放在融合后而不是融合前
> 
> 这一点也挺说明设计思想。
> 
> ---
> 
> ## 如果在融合前平滑
> 那相当于先把默认边界弄顺，再拿感知去改。  
> 问题是：
> - 感知改完以后还是会有新的折点和噪声
> - 最终结果仍然不够顺
> 
> ---
> 
> ## 放在融合后平滑的意义
> 这表示作者想要的是：
> 
> > **最终交给规划器的边界是经过所有约束后的“成品边界”**
> 
> 这样平滑处理的是最终结果，更有效。
> 
> ---
> 
> ## 但这也要求平滑算法足够克制
> 因为融合后边界已经包含：
> - 障碍约束
> - 车道线约束
> - 参考线约束
> - 车位口约束
> 
> 如果平滑太猛，就会把这些真实约束又抹掉。  
> 所以这一步一般应该是“小修边”，不是“大整形”。
> 
> ---
> 
> # 四十九、从系统架构看，这套泊出地图模块很可能是“规划前置过滤器”
> 
> 它不仅仅在生产地图，还在做很多“替规划器过滤无效场景”的事情。
> 
> ---
> 
> ## 1）它提前淘汰不靠谱车位
> 通过：
> - `CalMapSlotPar()`
> - `CalSlotInfo()`
> 
> ---
> 
> ## 2）它提前淘汰空间不足场景
> 通过：
> - `CalSlotBorderPtByFSDAndODMapInfo()`
> 
> ---
> 
> ## 3）它提前淘汰终点不可落场景
> 通过：
> - `BoundarySeizeEndCarPosInfo()`
> - `CenterEndCarPosInfo()`
> - `EndCarPosUpdata()`
> 
> ---
> 
> ## 4）它提前淘汰当前车姿不匹配场景
> 通过：
> - `CheckIfCarPosIsValid()`
> 
> ---
> 
> ## 所以规划器拿到的不是“原始环境”
> 而是一个已经经过筛选的环境：
> 
> - 目标车位可用
> - 通道大体可用
> - 终点尽量可用
> - 当前车姿大体匹配
> 
> 这能大幅减轻规划器负担。
> 
> ---
> 
> # 五十、如果从“规划器视角”看，这个模块输出了什么
> 
> 虽然代码里你看到的是各种中间量，但从规划器视角，最终真正有价值的输出大概就 4 类。
> 
> ---
> 
> ## 1）左右边界
> 最直接的是：
> 
> - `LeftBoundary`
> - `RightBoundary`
> 
> 规划器用它们定义：
> - 可行驶走廊
> - 约束区域
> - 碰撞边界参考
> 
> ---
> 
> ## 2）终点位姿
> 也就是：
> 
> - `EndPos`
> 
> 规划器要知道最终把车开到哪里、朝向什么角度。
> 
> ---
> 
> ## 3）车位几何基准
> 比如：
> - `Obj1Pt / Obj2Pt`
> - `SlotBordPt`
> - `SlotLen / SlotDepth`
> 
> 这些有时会被规划器或上层策略用于：
> - 初始轨迹模板选择
> - 出位阶段识别
> - 是否还处于车位内判断
> 
> ---
> 
> ## 4）失败/状态信息
> 比如：
> - fail cause
> - 地图是否有效
> - 当前是否更新帧
> - 终点是否被侵占
> 
> 这些给上层状态机很重要。
> 
> ---
> 
> ## 换句话说
> 这个模块对规划器说的是：
> 
> > “我已经帮你把环境压缩成一条能走的通道和一个尽量能到的终点了，你在这里面做轨迹就行。”
> 
> ---
> 
> # 五十一、如果从“调试问题”角度看，最容易出 bug 的地方在哪
> 
> 如果这套代码在路测中出现“泊出行为异常”，我会优先怀疑下面这些点。
> 
> ---
> 
> ## 1）车位选错
> 现象：
> - 地图方向整体就不对
> - 终点在错误一侧
> - 边界和当前车姿完全不搭
> 
> 重点查：
> - `Request_SlotId`
> - `FusionMode`
> - `SlotIndex/FusSlotIndex`
> - `parkside`
> - 镜像处理逻辑
> 
> ---
> 
> ## 2）Obj1/Obj2 修错
> 现象：
> - 边界起点歪
> - 车位口太窄/太宽
> - EndPos 明显偏移
> 
> 重点查：
> - `CalSlotBorderPtByParkOutSlotInfo()`
> - `CalSlotBorderPtByParkOutInfo()`
> - `CalSlotBorderPtByFSDAndODMapInfo()`
> 
> ---
> 
> ## 3）LaneLine/RefercLine 选错线
> 现象：
> - 终点角度突然歪掉
> - 出位后车辆总想斜着摆
> - 地图边界某一侧突然贴到奇怪的线
> 
> 重点查：
> - 线筛选条件
> - 角度阈值
> - 距离阈值
> - 优先级仲裁
> 
> ---
> 
> ## 4）更新帧抖动
> 现象：
> - 边界左右晃
> - 终点每帧跳
> - 规划轨迹反复重算
> 
> 重点查：
> - `UpDataMapInfo()`
> - 是否缺少限幅/滤波
> - 感知失效时是否回退到上一帧
> - 是否每帧误触发重建
> 
> ---
> 
> ## 5）终点修正过度
> 现象：
> - 车明明能出，却因为终点一直被挪来挪去导致犹豫
> - 或终点被修到离谱位置
> 
> 重点查：
> - `BoundarySeizeEndCarPosInfo()`
> - `CenterEndCarPosInfo()`
> - `EndCarPosUpdata()`
> - 最大迭代次数
> - 每次平移步长
> 
> ---
> 
> # 五十二、如果你要给这份代码写“注释文档”，最合适的结构是什么
> 
> 如果你之后准备整理成设计说明或源码注释，我建议按下面结构写，比按函数一个个平铺更清楚。
> 
> ---
> 
> ## 第一章：模块目标
> 说明：
> - 这是 APA 泊出地图构建模块
> - 输入是什么
> - 输出是什么
> - 不负责什么（例如不直接做轨迹优化）
> 
> ---
> 
> ## 第二章：主流程
> 说明：
> - 首次建图流程
> - 更新帧流程
> - 失败退出路径
> 
> ---
> 
> ## 第三章：车位建模
> 说明：
> - 车位选择
> - Obj1/Obj2 定义
> - SlotBordPt 定义
> - 多源修边机制
> 
> ---
> 
> ## 第四章：边界建模
> 说明：
> - 默认边界生成
> - FSD/LaneLine/RefercLine/OD 融合顺序
> - Main/Sub 到 Left/Right 的映射
> 
> ---
> 
> ## 第五章：终点建模
> 说明：
> - 默认终点生成
> - UWB 终点修正
> - 终点侵占判定
> - 终点居中与迭代更新
> 
> ---
> 
> ## 第六章：运行时维护
> 说明：
> - 增量更新
> - 坐标系切换
> - 阶段切换：车位内/车位外
> - 平稳性处理
> 
> ---
> 
> ## 第七章：失败原因
> 说明：
> - fail cause 分类
> - 每类失败的触发条件
> - 上层如何处理
> 
> ---
> 
> # 五十三、如果进一步深挖，这套代码背后其实是一个“弱状态机”
> 
> 虽然函数名里没明显写状态机，但逻辑上它几乎肯定是有状态的。
> 
> ---
> 
> ## 状态 1：待初始化
> 条件：
> - 新请求到来
> - `Request_cmd` 有效
> - 需要建立新的泊出地图
> 
> 动作：
> - DebugInit
> - 车位选择
> - 车位几何构建
> - 边界初始化
> 
> ---
> 
> ## 状态 2：初始地图有效
> 条件：
> - `CalMapInfo()` 成功
> - `CheckIfCarPosIsValid()` 成功
> 
> 动作：
> - 向规划器输出第一版有效地图
> 
> ---
> 
> ## 状态 3：运行中更新
> 条件：
> - 同一请求持续执行
> - `request_cnt == lastreqcnt`
> - APA 运行状态进入执行阶段
> 
> 动作：
> - `UpDataMapInfo()`
> - 继续边界维护和终点维护
> 
> ---
> 
> ## 状态 4：阶段切换
> 条件：
> - 车明显出位
> - 触发新 anchor
> - 车位约束权重下降
> 
> 动作：
> - 逐渐从“车位驱动”切到“通道驱动”
> 
> ---
> 
> ## 状态 5：失败退出
> 条件：
> - 车位无效
> - 终点不可落
> - 当前车姿无效
> - 边界丢失
> 
> 动作：
> - `Setfailcause`
> - 上层退出或重试
> 
> ---
> 
> # 五十四、如果把这些函数按“输入-处理-输出”抽象，可以得到一条很清晰的数据流
> 
> ---
> 
> ## 输入层
> 来自 `APAMap_GInputData`：
> - 请求参数
> - 当前车姿
> - 车位源
> - FSD
> - OD
> - LaneLine
> - RefercLine
> - USS/UWB 等
> 
> ---
> 
> ## 处理层 1：车位抽象
> 输出：
> - `SlotPar`
> - `Obj1Pt/Obj2Pt`
> - `SlotBordPt`
> - 初始 `EndPos`
> 
> ---
> 
> ## 处理层 2：通道抽象
> 输出：
> - `MainBoundary`
> - `SubBoundary`
> - `LeftBoundary`
> - `RightBoundary`
> 
> ---
> 
> ## 处理层 3：约束融合
> 输出：
> - 修正后的边界
> - 修正后的终点
> - 是否存在侵占
> 
> ---
> 
> ## 处理层 4：有效性筛查
> 输出：
> - 最终地图有效/无效
> - fail cause
> - 当前阶段可否继续
> 
> ---
> 
> ## 最终输出层
> 给规划器/状态机：
> - 边界
> - 终点
> - 状态
> - 故障码
> 
> ---
> 
> # 五十五、如果你下一步要我继续，最适合的方式有三种
> 
> 我现在已经把这套代码的**函数级架构逻辑**基本拆到比较深了。  
> 下一步如果你还想继续，我建议三种方向，你任选一种，我都可以接着干。
> 
> ---
> 
> ## 方向 A：继续纯逻辑深挖
> 我可以继续往下讲：
> 
> - 平行泊出 / 垂直泊出 / 斜列泊出 在这些函数里分别会怎么处理
> - 各函数内部可能有哪些典型 if 分支
> - `Obj1/Obj2/EndPos` 在三种车位模式下各自的几何意义
> 
> 如果你想“先把思路吃透”，这个方向最好。
> 
> ---
> 
> ## 方向 B：按源码文件逐函数精读
> 你把 `MapParkingOut.c/.cpp` 具体实现贴出来，  
> 我可以直接给你做：
> 
> - 每个函数逐行解释
> - 每个变量含义
> - 每个 if/else 为什么这么写
> - 每个 fail cause 对应什么场景
> 
> 这个会最精准。
> 
> ---
> 
> ## 方向 C：我直接帮你反推出一版“模块设计文档”
> 我可以基于现在的分析，直接整理成一份更正式的文档结构，比如：
> 
> - 模块概述
> - 处理流程
> - 数据结构说明
> - 关键函数职责
> - 多源融合策略
> - 终点更新策略
> - 失败场景说明
> 
> 这个适合你做汇报、写注释或知识沉淀。
> 
> ---
> 
> 如果

#### _User_

> Continue your response exactly where you left off:

#### _Assistant_

> 你愿意，我下一条可以直接进入：
> 
> 1. **平行泊出 / 垂直泊出 / 斜列泊出 的分模式细化解析**  
> 2. **终点 `EndPos` 的详细生成与修正机制**  
> 3. **更新帧 `UpDataMapInfo()` 的潜在内部状态机**  
> 4. **fail cause 编号体系可能的完整语义归类**
> 
> 既然你现在是让我“继续”，那我就默认往 **方向 A：分模式细化** 继续深入。
> 
> ---
> 
> # 五十六、平行泊出 / 垂直泊出 / 斜列泊出，在这套代码里很可能是三套“同框架、不同参数化”的逻辑
> 
> 这套代码从结构上看，并不像是三套完全独立算法，更像是：
> 
> > **同一个泊出地图框架，针对不同车位类型切换几何基准、边界外延方向、终点定义方式。**
> 
> 也就是说，函数还是这些函数：
> 
> - `CalMapSlotPar`
> - `CalSlotInfo`
> - `CalBoundaryByParkOutInfo`
> - `SetEndCarPosInOldCorSys`
> - `BoundarySeizeEndCarPosInfo`
> 
> 但里面的具体计算，会根据车位模式走不同分支。
> 
> ---
> 
> ## 一、平行泊出的典型语义
> 
> 平行位最核心的特征是：
> 
> - 车位主方向与道路方向大体平行
> - 泊出时车辆要先脱离前后约束，再并入道路
> - 车位长度约束通常比宽度约束更关键
> - 终点姿态通常也应该与道路平行
> 
> ---
> 
> ### 1）在 `CalMapSlotPar()` 里，平行位最关注什么
> 平行位通常特别关注：
> 
> - 车位长度是否足够
> - 前车 / 后车是否构成边界
> - 车位口位置是否明确
> - 当前车是更靠前还是更靠后
> 
> 因为平行位泊出路径往往受前后障碍强烈影响。
> 
> 也就是说，平行位的 `SlotLen`、车位首尾边界、`Obj1/Obj2` 的定义，很可能和垂直位不一样。
> 
> ---
> 
> ### 2）在 `CalSlotInfo()` 里，平行位的 Obj1/Obj2 更可能代表什么
> 对于平行位，`Obj1Pt / Obj2Pt` 很可能不是“车位深处左右角”，而更像：
> 
> - 车位开口边沿的两个关键约束点
> - 或车位一侧边界上决定出位口的两个支点
> 
> 简化理解的话，平行位的 Obj1/Obj2 更像是：
> 
> > “从车位向道路脱离时，必须重点关注的两个边界锚点”
> 
> 它们决定车辆最初摆出时的“口子”有多大。
> 
> ---
> 
> ### 3）在 `CalBoundaryByParkOutInfo()` 里，平行位的默认边界怎么拉
> 平行位的通道大概率这样生成：
> 
> - 沿道路方向延展长边界
> - 靠车位一侧的边界会更贴近相邻车/车位边
> - 另一侧边界则沿道路外侧或通道中心方向给出较宽松空间
> 
> 因此平行位边界的关键是：
> 
> - **纵向延展长**
> - **横向宽度相对窄**
> - **刚出车位口的前几米特别关键**
> 
> ---
> 
> ### 4）在 `SetEndCarPosInOldCorSys()` 里，平行位的终点怎么定义
> 平行位终点通常不是简单“往前挪一点”，而是要满足：
> 
> - 车辆已经完全脱离前后车辆包络
> - 车身角度趋于与道路平行
> - 车身中心尽量接近可通行区域中线或合理行驶位置
> 
> 所以平行位终点一般会带有明显的“**并入道路**”语义。
> 
> 这也是为什么：
> - LaneLine 对平行位终点角度尤其重要
> - RefercLine 次之
> - 单纯车位方向意义反而没那么大
> 
> ---
> 
> ### 5）平行位最容易在哪出问题
> 从工程上看，平行位最容易出问题的是：
> 
> #### a. 前后障碍约束处理不稳
> 比如前后邻车检测有抖动，会导致车位长度和出位口判断变化。
> 
> #### b. 终点角度飘
> 如果 LaneLine 不稳，而 RefercLine 又不可靠，终点朝向可能左右跳。
> 
> #### c. 车位口边界修错
> 只要 Obj1/Obj2 错一点，平行位初始摆车空间就会明显变差。
> 
> ---
> 
> ## 二、垂直泊出的典型语义
> 
> 垂直位和道路关系通常是：
> 
> - 车位主方向与道路方向大体垂直
> - 车辆要从车位深处退出到道路通道
> - 车位深度、车位口宽度、道路宽度都很关键
> - 泊出后车身通常要转向道路方向
> 
> ---
> 
> ### 1）在 `CalMapSlotPar()` 里，垂直位最关注什么
> 垂直位通常更关注：
> 
> - 车位深度 `SlotDepth`
> - 车位口宽度
> - 车位左右邻近障碍
> - 车前/车后是否贴近墙体或障碍
> 
> 因为对于垂直位，车辆首先要解决的是：
> 
> > “能不能顺利从一个深槽里出来”
> 
> ---
> 
> ### 2）在 `CalSlotInfo()` 里，垂直位的 SlotBordPt 更像完整车位框
> 相比平行位，垂直位通常更适合用一个清晰的四边形框来描述：
> 
> - 车位口两点
> - 车位底边两点
> 
> 所以如果感知层给了较完整车位框，垂直位通常能更直接地利用它。
> 
> 这也是为什么：
> - `CalSlotBorderPtByParkOutSlotInfo()` 对垂直位可能更有效
> - 车位框 fallback 也更容易做成标准矩形
> 
> ---
> 
> ### 3）在 `CalBoundaryByParkOutInfo()` 里，垂直位默认边界怎么拉
> 垂直位通道大概率这样构造：
> 
> - 从车位口向外法向拉出一条主轴
> - 沿这条主轴向道路方向延展
> - 左右边界围绕车位口展开，再在道路区域逐渐平顺过渡
> 
> 也就是说，垂直位的地图重点通常分两段：
> 
> #### 第一段：车位口脱离段
> 非常依赖车位几何和口宽。
> 
> #### 第二段：道路对齐段
> 更依赖车道线/参考线/通道边界。
> 
> ---
> 
> ### 4）在 `SetEndCarPosInOldCorSys()` 里，垂直位的终点如何定义
> 垂直位的终点一般要满足：
> 
> - 车辆已经完全脱离车位口
> - 车身朝向开始接近道路方向
> - 终点位于通道内合理中心位置
> - 不压两侧边界或前方障碍
> 
> 相比平行位，垂直位终点更像：
> 
> > “先出来，再摆正”
> 
> 因此终点位置往往离车位口更近，但对终点姿态的约束更强。
> 
> ---
> 
> ### 5）垂直位最容易在哪出问题
> #### a. 车位口宽度被压缩过度
> 如果 FSD/OD/Sensor 把车位口收得太窄，就可能直接判 fail。
> 
> #### b. 出位后道路边界接续不好
> 如果车位口逻辑和道路逻辑之间过渡不自然，规划会出现姿态别扭。
> 
> #### c. 终点落在通道偏侧位置
> 如果终点还过度受车位口影响，而没有被通道中心修正，就会显得偏。
> 
> ---
> 
> ## 三、斜列泊出的典型语义
> 
> 斜列位最复杂，因为它天然带角度。
> 
> 它兼具两类特征：
> 
> - 像垂直位一样，需要从车位内部退出
> - 又像平行位一样，对道路方向和车位方向的夹角特别敏感
> 
> 所以斜列位通常是这套代码里最依赖几何细节的一类。
> 
> ---
> 
> ### 1）在 `CalMapSlotPar()` 里，斜列位最关键的是角度定义
> 斜列位首先要搞清楚：
> 
> - 车位主方向到底是多少度
> - 开口朝哪边
> - 当前车辆姿态和车位角度关系是否一致
> - 泊出后道路主方向与车位方向相差多少
> 
> 换句话说，斜列位比其他模式更依赖：
> 
> - 角度估计稳定性
> - 坐标变换正确性
> - 镜像处理正确性
> 
> ---
> 
> ### 2）在 `CalSlotInfo()` 里，斜列位最容易受 `Obj1/Obj2` 误差影响
> 因为斜列位不是标准正交结构，一旦：
> 
> - Obj1 稍微偏了
> - Obj2 稍微偏了
> - 或车位角度估错了几度
> 
> 后面生成的：
> - 边界方向
> - 终点朝向
> - 通道宽度投影
> 
> 都会一起偏。
> 
> 所以斜列位通常对 `CalSlotBorderPtByFSDAndODMapInfo()` 的质量特别敏感。
> 
> ---
> 
> ### 3）在 `CalBoundaryByParkOutInfo()` 里，斜列位通道怎么拉
> 斜列位默认边界通常不是简单平行四边形，也不是标准垂直通道，而更像：
> 
> - 起始段沿车位法向或开口方向
> - 过渡段逐渐向道路方向旋转
> - 最终段与道路或参考线对齐
> 
> 这意味着它的边界天生更容易出现折点，所以：
> 
> - `SmoothMapBoundary()` 对斜列位更重要
> - LaneLine / RefercLine 对斜列位终点角度的修正也更重要
> 
> ---
> 
> ### 4）在 `SetEndCarPosInOldCorSys()` 里，斜列位的终点更像“折中结果”
> 斜列位终点通常不是纯车位方向，也不是纯道路方向，而是一个阶段性折中：
> 
> - 位置要先保证能从斜位脱离
> - 角度又要逐渐向道路方向收敛
> 
> 如果你看源码，大概率会发现斜列位的终点计算分支会比平行/垂直更复杂。
> 
> ---
> 
> ### 5）斜列位最容易在哪出问题
> #### a. 角度抖动传导到整张地图
> 只要车位角度波动，边界和终点会一起波动。
> 
> #### b. 镜像/左右侧判断错误
> 斜列位左右一旦判错，地图很容易整体翻转到另一边。
> 
> #### c. 平滑过度
> 因为斜列位本身有合理折线，如果平滑太强，反而会把真实方向结构抹掉。
> 
> ---
> 
> # 五十七、三种模式下，`EndPos` 的含义其实也不一样
> 
> 虽然变量名都叫 `EndPos`，但从语义上它不是同一个“终点概念”。
> 
> ---
> 
> ## 1）平行位的 EndPos
> 更像：
> 
> > “车辆成功并入道路后，一个与道路方向一致的稳定停止位姿”
> 
> 重点是：
> - 并入
> - 对齐道路
> - 脱离前后约束
> 
> ---
> 
> ## 2）垂直位的 EndPos
> 更像：
> 
> > “车辆刚好脱离车位口并在通道中获得可继续规划姿态的位姿”
> 
> 重点是：
> - 脱离车位口
> - 进入通道
> - 保证可转入道路方向
> 
> ---
> 
> ## 3）斜列位的 EndPos
> 更像：
> 
> > “车辆从斜角车位退出后，在通道内形成一个几何连续且约束合理的过渡位姿”
> 
> 重点是：
> - 几何过渡
> - 角度连续
> - 道路接续自然
> 
> ---
> 
> # 五十八、三种模式下，`BoundarySeizeEndCarPosInfo()` 的“难点”也不同
> 
> 这个函数虽然接口统一，但判断难点会随模式变化。
> 
> ---
> 
> ## 平行位
> 难点在：
> - 车身长边与前后约束关系
> - 是否完全脱离邻车包络
> - 道路方向是否足够摆正
> 
> ---
> 
> ## 垂直位
> 难点在：
> - 车位口两侧是否压边
> - 车头/车尾是否仍侵入车位边框
> - 通道宽度是否足够容纳终点姿态
> 
> ---
> 
> ## 斜列位
> 难点在：
> - 带角度的包络投影
> - 终点姿态与边界折线关系
> - 局部空间看起来够，但姿态一摆就不够
> 
> ---
> 
> # 五十九、如果源码里有“模式分支”，最可能出现在哪几个函数里
> 
> 如果你之后贴源码，我建议重点在这些函数里找：
> 
> ---
> 
> ## 1）`APAMap_ParkingOutCalMapSlotPar()`
> 这里通常有最早的模式判定：
> 
> - 平行 / 垂直 / 斜列
> - 左侧 / 右侧
> - 车头先出 / 车尾先出
> 
> 这是全局分支入口。
> 
> ---
> 
> ## 2）`APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> 这里很可能按模式分别设置：
> 
> - 默认车位宽深
> - Obj1/Obj2 的构造方法
> - 车位口与底边的定义方式
> 
> ---
> 
> ## 3）`APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 这里大概率按模式分别设置：
> 
> - 通道主轴方向
> - 边界延展长度
> - 主边界/副边界的初始间距
> - 折线形状
> 
> ---
> 
> ## 4）`APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> 这里几乎肯定按模式分别计算：
> 
> - 终点位置
> - 终点角度
> - 终点相对车位口的偏移
> 
> ---
> 
> ## 5）`APAMap_ParkingOutCheckIfCarPosIsValid()`
> 这里很可能按模式分别检查：
> 
> - 当前车姿与车位方向是否匹配
> - 是否仍处于合理泊出阶段
> - 当前姿态是否超出模板适用范围
> 
> ---
> 
> # 六十、如果从“作者写代码的思路”看，这套系统很可能遵循了三个原则
> 
> ---
> 
> ## 原则 1：先有模板，再拿感知修
> 这在所有函数链里都能看出来：
> 
> - 先选车位
> - 先生成默认边界
> - 先生成默认终点
> - 再融合 FSD / OD / LaneLine / RefercLine
> 
> 这说明作者不信任感知单独闭环，而是采用：
> 
> > 模板保底 + 感知修正
> 
> ---
> 
> ## 原则 2：车位阶段与道路阶段逐步切换
> 通过：
> - `bAfterNewAnchorPointFlag`
> - `UpDataMapInfo()`
> - LaneLine / RefercLine 对终点的影响
> 
> 可以看出作者知道：
> 
> > 泊出不是一直在解决同一个问题。
> 
> 起初是“如何离开车位”，后来是“如何进入通道”。
> 
> ---
> 
> ## 原则 3：终点不是固定答案，而是可调整对象
> 通过：
> - `SetEndCarPos...`
> - `BoundarySeizeEndCarPosInfo()`
> - `CenterEndCarPosInfo()`
> - `EndCarPosUpdata()`
> 
> 能看出终点是一个可迭代优化量，而不是一次性输出。
> 
> 这比很多简单系统更鲁棒。
> 
> ---
> 
> # 六十一、接下来可以继续深挖的一个重点：`EndPos` 的计算公式思路
> 
> 如果继续往下讲，最值得下一步展开的是：
> 
> > **`EndPos` 到底会怎么通过几何关系被算出来**
> 
> 因为这是整个泊出模块中，最直接影响“车最后想去哪”的部分。
> 
> 这部分我可以继续拆成：
> 
> 1. `EndPos` 的输入依赖有哪些  
> 2. 默认终点怎么由车位口中心推出来  
> 3. 为什么要引入 `EndPosLine`  
> 4. 终点被侵占后是沿哪个方向搜索  
> 5. LaneLine / RefercLine / UWB 分别改的是位置还是角度  
> 6. 为什么终点会有“居中”和“滑移”两层修正
> 
> 如果你要，我下一条就直接进入 **六十二：`EndPos` 的完整几何与修正逻辑**。

#### _User_

> 反推出一版“模块设计文档”

#### _Assistant_

> **Crafting design document**
> 
> It looks like I need to provide an answer in Chinese for the user’s request, which translates to creating a formal design document. I shouldn't use any code blocks, so I’ll focus on a structured document for the APA Map ParkingOut module. I plan to include sections that cover purpose, scope, input/output, dependencies, architecture, and more. My goal is to be concise while still providing enough detail. Let's get that structured!**Crafting design document**
> 
> It looks like I need to provide an answer in Chinese for the user’s request, which translates to creating a formal design document. I shouldn't use any code blocks, so I’ll focus on a structured document for the APA Map ParkingOut module. I plan to include sections that cover purpose, scope, input/output, dependencies, architecture, and more. My goal is to be concise while still providing enough detail. Let's get that structured!

#### _Assistant_

> 下面给你一版基于现有函数名、调用链和典型 APA 泊出建图逻辑**反推出的“模块设计文档”**。  
> 我会尽量按正式设计文档的风格来写，同时注明哪些内容是**推断**。
> 
> ---
> 
> # APA 泊出地图模块设计文档（反推版）
> 
> ## 1. 文档说明
> 
> ### 1.1 文档目的
> 本文档用于描述 APA 自动泊车系统中**泊出地图构建模块**的设计思路、输入输出、核心流程、关键数据结构、融合策略、终点位姿生成逻辑、状态维护与异常处理机制。
> 
> ### 1.2 文档性质
> 本设计文档基于以下信息反向推导形成：
> 
> - 模块名：`MapParkingOut`
> - 关键接口：
>   - `APAMap_ParkingOutTask()`
>   - `APAMap_ParkingOutCalMapInfo()`
>   - `APAMap_ParkingOutUpDataMapInfo()`
> - 相关子函数命名与调用链
> - 典型 APA 泊出建图架构经验
> 
> 因此本文档为**反推设计文档**，用于理解模块架构与源码意图，不保证与原始设计文档逐字一致。
> 
> ---
> 
> # 2. 模块定位
> 
> ## 2.1 模块名称
> 泊出地图构建模块 / Parking Out Map Builder
> 
> ## 2.2 模块职责
> 该模块负责在 APA 泊出任务中，根据：
> 
> - 用户/状态机发起的泊出请求
> - 当前车辆位姿
> - 目标车位信息
> - 多源感知信息（FSD / OD / LaneLine / RefercLine / USS / UWB 等）
> 
> 构建一份供规划器使用的**泊出局部地图**，主要包括：
> 
> 1. 目标车位几何模型
> 2. 泊出通道左右边界
> 3. 泊出终点位姿 `EndPos`
> 4. 地图有效性状态
> 5. 失败原因码
> 
> ## 2.3 模块边界
> 
> ### 本模块负责
> - 目标车位选择与参数提取
> - 车位边界点构建
> - 通道边界初始化与多源融合
> - 终点位姿生成与修正
> - 地图运行中增量更新
> - 合法性检测与失败上报
> 
> ### 本模块不负责
> - 完整轨迹搜索/轨迹优化
> - 车辆控制执行
> - 原始感知检测
> - 全局路径规划
> 
> ---
> 
> # 3. 模块目标
> 
> 本模块的核心目标是向下游规划器提供一份：
> 
> - 几何上连续
> - 与真实环境一致
> - 满足安全边距
> - 终点可落位
> - 帧间稳定
> 
> 的泊出局部地图。
> 
> 其本质是：
> 
> > 从“目标车位 + 当前环境”中抽象出一条规划可用的泊出走廊与目标位姿。
> 
> ---
> 
> # 4. 输入与输出
> 
> ## 4.1 输入
> 
> 推测输入总线为：
> 
> - `APAMap_GInputData`
> 
> ### 4.1.1 请求类输入
> 来自 `ParkReqPar`：
> 
> - `Request_cmd`
> - `request_cnt`
> - `Request_SlotId`
> - `Request_SlotFusionMode`
> - `parkmode`
> - `parkoutmode`
> - `parkside`
> - `APARunningstate`
> - `APAstate`
> - `SlotDataIsNotMirrored`
> - `Parkout_UWBPos`
> 
> ### 4.1.2 车辆状态输入
> 来自 `CarLocInfo`：
> 
> - 当前车辆坐标 `x/y`
> - 当前车身角 `CarAng`
> 
> ### 4.1.3 车位类输入
> 可能来自：
> 
> - US Slot
> - VPL Slot
> - Fusion Slot
> - ParkOut SlotInfo
> 
> ### 4.1.4 环境感知输入
> 可能包括：
> 
> - `TopView FSD`
> - `ODInfo`
> - `LaneLineInfo`
> - `RefercLineInfo`
> - `USSObjInfo`
> - `PAObjInfo`
> - 其他近场传感器信息
> 
> ---
> 
> ## 4.2 输出
> 
> 推测主要输出保存在：
> 
> - `APAMap_GInfo`
> 
> ### 4.2.1 地图几何输出
> - `OutLine.LeftBoundary`
> - `OutLine.RightBoundary`
> 
> ### 4.2.2 车位建模输出
> - `SlotPar.Obj1Pt`
> - `SlotPar.Obj2Pt`
> - `SlotPar.SlotBordPt[4]`
> - `SlotPar.SlotLen`
> - `SlotPar.SlotDepth`
> 
> ### 4.2.3 终点输出
> - `SlotPar.EndPos`
> - `SlotPar.EndPosLine`
> 
> ### 4.2.4 状态输出
> - 地图有效/无效
> - 当前是否首次建图/更新帧
> - 各类终点更新标志
> - fail cause
> 
> ---
> 
> # 5. 总体设计思路
> 
> 该模块采用如下总体设计原则：
> 
> ## 5.1 模板优先，感知修正
> 先根据车位信息和泊出模式生成默认地图，再由感知信息逐步修正。
> 
> ## 5.2 车位阶段与通道阶段分离
> 泊出早期更依赖车位几何，泊出后期更依赖通道边界和道路方向。
> 
> ## 5.3 首次建图与增量更新分离
> 首次建图重构完整地图；后续帧仅在已有地图基础上做局部更新和稳定维护。
> 
> ## 5.4 终点位姿可迭代修正
> 终点不是一次性固定值，而是先生成默认值，再通过侵占检测和搜索进行调整。
> 
> ---
> 
> # 6. 总体流程
> 
> ## 6.1 入口函数
> - `APAMap_ParkingOutTask()`
> 
> ## 6.2 流程概述
> 
> ### 场景 A：首次建图
> 触发条件：
> - 新请求到来，或非更新帧
> 
> 流程：
> 
> 1. 调试/状态初始化
> 2. 车位参数计算
> 3. 车位边界计算
> 4. 泊出地图边界构建
> 5. 当前车姿合法性检查
> 6. 输出地图或失败原因
> 
> ---
> 
> ### 场景 B：更新帧
> 触发条件：
> - 同一泊出请求持续执行
> - `request_cnt == lastreqcnt`
> - 运行状态进入执行阶段
> 
> 流程：
> 
> 1. 保持已有车位几何
> 2. 增量更新边界
> 3. 增量更新终点
> 4. 检查侵占与有效性
> 5. 输出更新后的地图
> 
> ---
> 
> # 7. 模块架构分层
> 
> ---
> 
> ## 7.1 任务调度层
> 负责泊出地图任务的主控调度。
> 
> ### 关键函数
> - `APAMap_ParkingOutTask()`
> - `APAMap_ParkingOutDebugInit()`
> - `APAMap_ParkingOutUpDataMapInfo()`
> 
> ---
> 
> ## 7.2 车位建模层
> 负责识别并建立目标车位几何模型。
> 
> ### 关键函数
> - `APAMap_ParkingOutCalMapSlotPar()`
> - `APAMap_ParkingOutCalSlotInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> - `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> ---
> 
> ## 7.3 通道边界构建层
> 负责根据车位和环境生成规划可用通道边界。
> 
> ### 关键函数
> - `APAMap_ParkingOutCalMapInfo()`
> - `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> - `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> - `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> - `APAMap_FusBoundaryByODMapInfo()`
> - `APAMap_SmoothMapBoundary()`
> 
> ---
> 
> ## 7.4 终点位姿构建层
> 负责生成和修正泊出终点位姿。
> 
> ### 关键函数
> - `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> - `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> - `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> - `APAMap_ParkingOutCenterEndCarPosInfo()`
> - `APAMap_ParkingOutEndCarPosUpdata()`
> 
> ---
> 
> ## 7.5 有效性与异常管理层
> 负责车姿合理性校验和失败上报。
> 
> ### 关键函数
> - `APAMap_ParkingOutCheckIfCarPosIsValid()`
> - `APAMAP_Setfailcause(...)`
> 
> ---
> 
> # 8. 核心数据模型
> 
> ---
> 
> ## 8.1 输入数据模型：`APAMap_GInputData`
> 统一承载本模块输入信息。
> 
> ### 主要子域
> - 泊车请求信息
> - 当前车身位姿
> - 车位检测信息
> - 多源地图/感知信息
> 
> ---
> 
> ## 8.2 输出/内部状态模型：`APAMap_GInfo`
> 统一保存本模块内部计算结果与对外输出。
> 
> ### 主要子域
> 
> #### 8.2.1 `SlotPar`
> 描述目标车位及泊出目标信息。
> 
> 可能包含：
> - `SlotID`
> - `SlotIndex`
> - `FusSlotIndex`
> - `SlotSide`
> - `SlotLen`
> - `SlotDepth`
> - `Obj1Pt`
> - `Obj2Pt`
> - `Obj1Ang`
> - `Obj2Ang`
> - `SlotBordPt[4]`
> - `EndPos`
> - `EndPosLine`
> 
> #### 8.2.2 `OutLine`
> 描述最终可用通道边界。
> 
> 可能包含：
> - `LeftBoundary`
> - `RightBoundary`
> 
> #### 8.2.3 坐标系信息
> 可能包含：
> - `NewCordSysOPt`
> - `NewCordSysAng`
> - 坐标系切换标志
> 
> ---
> 
> # 9. 关键几何概念定义
> 
> ---
> 
> ## 9.1 车位关键点 `Obj1Pt / Obj2Pt`
> 推测为车位口两侧或车位两侧关键边界支点，用于：
> 
> - 构造车位边界
> - 定义通道起点
> - 计算终点位置
> - 进行多源边界修正
> 
> ---
> 
> ## 9.2 车位边界点 `SlotBordPt[4]`
> 推测为目标车位四边形边界关键点，用于：
> 
> - 表示车位框
> - 表示车位口和车位底边
> - 评估车位宽深
> - 约束当前车是否仍处于目标车位
> 
> ---
> 
> ## 9.3 主边界 / 副边界
> 内部边界表示形式，分别对应：
> 
> - `MainBoundary`：更贴近车位侧或更关键侧
> - `SubBoundary`：通道另一侧边界
> 
> 最终再映射到：
> - `LeftBoundary`
> - `RightBoundary`
> 
> ---
> 
> ## 9.4 终点位姿 `EndPos`
> 泊出规划目标位姿，包含：
> 
> - 目标坐标
> - 目标航向角
> 
> 其值由：
> - 车位几何
> - 道路/参考线方向
> - 障碍侵占结果
> - UWB 目标点
> - 运行时更新状态
> 
> 共同决定。
> 
> ---
> 
> ## 9.5 终点参考线 `EndPosLine`
> 推测用于：
> 
> - 终点姿态线性表达
> - 沿线搜索合法终点
> - 与边界/车道线做方向一致性计算
> 
> ---
> 
> # 10. 详细设计
> 
> ---
> 
> ## 10.1 调试/初始化
> 
> ### 函数
> - `APAMap_ParkingOutDebugInit()`
> 
> ### 职责
> - 清空上帧调试状态
> - 重置终点更新标志
> - 重置侵占标志
> - 初始化相关融合状态
> - 清理失败原因缓存
> 
> ### 设计意图
> 防止跨帧状态污染，保证首次建图逻辑干净启动。
> 
> ---
> 
> ## 10.2 车位参数计算
> 
> ### 函数
> - `APAMap_ParkingOutCalMapSlotPar()`
> 
> ### 职责
> 1. 解析泊出请求
> 2. 判断车位在车身左/右哪一侧
> 3. 根据 FusionMode 选择车位数据源
> 4. 根据 SlotID 匹配目标车位
> 5. 提取车位基础参数
> 6. 初始化多源边界候选数据
> 
> ### 失败场景
> - SlotID 未找到
> - 数据源无效
> - 车位模式不支持
> - 左右侧/镜像关系异常
> 
> ---
> 
> ## 10.3 车位边界构建
> 
> ### 函数
> - `APAMap_ParkingOutCalSlotInfo()`
> 
> ### 职责
> 以分级方式构造车位边界：
> 
> 1. 优先从感知车位框中直接提取
> 2. 若失败则根据模板/泊出信息兜底推算
> 3. 最后用 FSD/OD/Sensor 对边界进行修正
> 
> ### 输出
> - `Obj1Pt / Obj2Pt`
> - `SlotBordPt`
> - 初始 `EndPos`
> 
> ---
> 
> ## 10.4 基于感知车位框的边界计算
> 
> ### 函数
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> 
> ### 职责
> - 读取现成车位框角点
> - 判断车位口与车位底边
> - 建立目标车位几何
> - 尝试直接生成 `Obj1/Obj2`
> - 给出初始终点位姿
> 
> ### 适用特点
> - 精度高
> - 依赖感知车位框完整性
> - 对标准垂直/斜列位更友好
> 
> ---
> 
> ## 10.5 基于模板的边界兜底构建
> 
> ### 函数
> - `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> 
> ### 职责
> 在感知车位框不可靠时，根据：
> 
> - 当前车姿
> - 泊出模式
> - 车位方向
> - 标准车位尺寸
> - 安全边距
> 
> 推算一个可用的车位几何模型。
> 
> ### 设计意图
> 保证即使感知不完美，也尽量能构造出可用泊出地图。
> 
> ---
> 
> ## 10.6 基于 FSD/OD/Sensor 的车位修正
> 
> ### 函数
> - `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> ### 职责
> 在已有车位框基础上，融合多源感知对车位边界进行收缩或修正。
> 
> ### 处理步骤
> 1. 读取初始 `Obj1 / Obj2 / SlotLen / EndPos`
> 2. 分别计算：
>    - Sensor Offset
>    - FSD Offset
>    - OD Offset
> 3. 取最保守结果作为收缩量
> 4. 判断收缩后剩余宽度是否足够
> 5. 更新修正后的边界点
> 6. 重新计算默认终点
> 
> ### 失败条件
> - 车位被压缩后不满足车辆通过/驻车安全余量
> 
> ### 设计意图
> 先修正车位源头，再统一派生边界和终点，保证内部一致性。
> 
> ---
> 
> # 11. 通道边界构建设计
> 
> ---
> 
> ## 11.1 总控函数
> 
> ### 函数
> - `APAMap_ParkingOutCalMapInfo()`
> 
> ### 职责
> 按流水线完成地图边界构建：
> 
> 1. 生成默认边界
> 2. 融合 FSD
> 3. 融合 LaneLine
> 4. 融合 RefercLine
> 5. 融合 OD
> 6. 平滑边界
> 7. 检查终点是否被侵占
> 8. 必要时重新调整终点
> 
> ---
> 
> ## 11.2 默认边界生成
> 
> ### 函数
> - `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 
> ### 职责
> 根据：
> - 车位几何
> - 泊出模式
> - 泊出侧
> - 当前阶段
> 
> 生成默认通道边界。
> 
> ### 生成逻辑
> - 确定通道主轴方向
> - 构造主边界与副边界
> - 设定默认通道宽度
> - 将边界映射为左/右边界输出
> 
> ### 模式特征
> - 平行位：沿道路方向长距离延展
> - 垂直位：从车位口法向拉出通道
> - 斜列位：起始沿车位方向，后续逐渐向道路方向过渡
> 
> ---
> 
> ## 11.3 FSD 边界融合
> 
> ### 函数
> - `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> 
> ### 职责
> 使用 FSD TopView 边界信息对默认边界做局部修正。
> 
> ### 典型作用
> - 用视觉边缘替换局部边界段
> - 将边界收缩到真实路沿/停车线附近
> - 改善默认地图与真实环境的贴合度
> 
> ---
> 
> ## 11.4 LaneLine 边界与终

#### _User_

> Continue your response exactly where you left off:

#### _Assistant_

> 点融合
> 
> ### 函数
> - `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> 
> ### 职责
> 1. 从 LaneLine 中筛选与泊出通道相关的有效线
> 2. 用车道线更新边界局部结构
> 3. 用车道线方向更新终点朝向角
> 
> ### 设计意图
> 让泊出终点更贴近真实道路方向，增强出位后姿态自然性。
> 
> ### 典型优先级
> LaneLine 通常是终点朝向更新的高优先级来源。
> 
> ---
> 
> ## 11.5 RefercLine 边界与终点融合
> 
> ### 函数
> - `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> 
> ### 职责
> 1. 利用参考线修正车位口/通道局部结构
> 2. 在 LaneLine 不可用或质量不足时，更新终点方向
> 3. 增强停车场结构化环境中的几何稳定性
> 
> ### 设计意图
> 补充 LaneLine 在停车场景下可能缺失的道路/结构方向信息。
> 
> ---
> 
> ## 11.6 OD 障碍边界融合
> 
> ### 函数
> - `APAMap_FusBoundaryByODMapInfo()`
> 
> ### 职责
> 将障碍物侵占信息投影到通道边界中，收缩边界或限制终点搜索区域。
> 
> ### 典型作用
> - 避开相邻车辆
> - 避开障碍桶、柱子、墙角
> - 提前裁剪不可行驶区域
> 
> ---
> 
> ## 11.7 边界平滑
> 
> ### 函数
> - `APAMap_SmoothMapBoundary()`
> 
> ### 职责
> 对融合后的边界点列进行轻量平滑，包括：
> 
> - 删除冗余点
> - 限制局部角度突变
> - 统一点间距
> - 保留关键转折点
> 
> ### 设计原则
> 平滑仅改善规划可用性，不得破坏安全边界约束。
> 
> ---
> 
> # 12. 终点位姿设计
> 
> ---
> 
> ## 12.1 默认终点生成
> 
> ### 函数
> - `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> 
> ### 职责
> 在旧坐标系下，根据：
> 
> - 车位模式
> - 车位关键点
> - 通道方向
> - 车辆尺寸
> - 安全余量
> 
> 生成默认泊出终点位姿。
> 
> ### 模式差异
> - 平行位：强调并入道路后的稳定姿态
> - 垂直位：强调脱离车位口后的可继续行驶姿态
> - 斜列位：强调从斜位过渡到道路方向的平滑姿态
> 
> ---
> 
> ## 12.2 UWB 终点修正
> 
> ### 函数
> - `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> 
> ### 职责
> 若存在有效 UWB 目标点，则根据 UWB 位置信息替代或修正默认终点位置。
> 
> ### 典型策略
> - 位置优先听 UWB
> - 角度仍优先由 LaneLine / RefercLine / 通道方向决定
> 
> ---
> 
> ## 12.3 终点侵占检查
> 
> ### 函数
> - `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> 
> ### 职责
> 判断当前 `EndPos` 是否被：
> 
> - 左右边界
> - 障碍物
> - 车位局部结构
> - 安全余量要求
> 
> 所侵占。
> 
> ### 判断内容
> - 终点车辆包络与边界/障碍是否重叠
> - 是否过近导致安全余量不足
> - 是否超出允许通道范围
> 
> ---
> 
> ## 12.4 终点居中修正
> 
> ### 函数
> - `APAMap_ParkingOutCenterEndCarPosInfo()`
> 
> ### 职责
> 当终点被单侧压边或位置偏斜时，尝试将终点横向向通道中心移动，以恢复可落位性。
> 
> ### 设计定位
> 作为第一层低成本修复策略。
> 
> ---
> 
> ## 12.5 终点迭代更新
> 
> ### 函数
> - `APAMap_ParkingOutEndCarPosUpdata()`
> 
> ### 职责
> 当居中修正仍无法解除终点侵占时，沿预定方向对终点进行迭代平移/搜索，寻找合法终点。
> 
> ### 可能更新方向
> - 沿通道主轴方向平移
> - 沿横向轻微偏移
> - 在线性约束 `EndPosLine` 上滑动
> 
> ### 设计定位
> 作为第二层系统性修复策略。
> 
> ---
> 
> # 13. 运行时更新设计
> 
> ---
> 
> ## 13.1 增量更新入口
> 
> ### 函数
> - `APAMap_ParkingOutUpDataMapInfo()`
> 
> ### 职责
> 在同一泊出请求执行过程中，对已有地图进行运行时维护。
> 
> ---
> 
> ## 13.2 更新内容
> 1. 根据当前车位姿裁剪边界有效段
> 2. 用新感知更新局部边界
> 3. 重新评估终点侵占情况
> 4. 小步更新终点位置
> 5. 在必要时处理坐标系锚点切换
> 6. 保持地图帧间稳定性
> 
> ---
> 
> ## 13.3 更新设计原则
> - 不重新全量建图，避免地图抖动
> - 保留历史有效结果，提高鲁棒性
> - 局部更新优先于全局替换
> - 对边界与终点更新施加限幅
> 
> ---
> 
> # 14. 坐标系与阶段切换设计
> 
> ---
> 
> ## 14.1 新旧坐标系
> 模块内部推测存在：
> 
> - 初始/旧坐标系
> - 新锚点坐标系
> 
> 对应变量：
> - `NewCordSysOPt`
> - `NewCordSysAng`
> - `bAfterNewAnchorPointFlag`
> 
> ---
> 
> ## 14.2 设计动机
> 当车辆从车位内部逐渐驶向通道时：
> 
> - 原车位局部坐标系不再适合描述后续边界
> - 需要切换到更贴近当前可行驶区域的参考系
> 
> ---
> 
> ## 14.3 阶段切换
> 模块内部隐含至少三个阶段：
> 
> ### 车位内阶段
> - 车位几何为主要约束
> - 重点关注车位口、邻车、近场障碍
> 
> ### 过渡阶段
> - 车位和通道约束共同生效
> - 可能触发 anchor 切换
> 
> ### 通道阶段
> - 通道边界与道路方向为主要约束
> - LaneLine / RefercLine 权重提升
> 
> ---
> 
> # 15. 模式适配设计
> 
> ---
> 
> ## 15.1 平行泊出
> ### 设计关注点
> - 前后障碍约束
> - 并入道路方向
> - 车身与道路平行
> 
> ### 地图特征
> - 边界沿道路方向长距离展开
> - 终点更偏道路对齐语义
> 
> ---
> 
> ## 15.2 垂直泊出
> ### 设计关注点
> - 车位口宽度
> - 车位深度
> - 从车位深处退出到通道
> 
> ### 地图特征
> - 起始段以车位口法向为主
> - 后续逐渐向道路方向过渡
> 
> ---
> 
> ## 15.3 斜列泊出
> ### 设计关注点
> - 车位角度稳定性
> - 车位方向与道路方向的过渡
> - 镜像与侧向判断正确性
> 
> ### 地图特征
> - 起始方向带斜角
> - 边界更容易出现几何折线
> - 终点更强调角度连续性
> 
> ---
> 
> # 16. 合法性检查设计
> 
> ---
> 
> ## 16.1 当前车姿合法性检查
> 
> ### 函数
> - `APAMap_ParkingOutCheckIfCarPosIsValid()`
> 
> ### 检查内容
> 1. 当前车是否仍处于目标车位合理区域内
> 2. 当前车角度是否与车位模式匹配
> 3. 当前车身是否与边界发生明显冲突
> 4. 请求参数与当前车姿是否一致
> 
> ### 设计意图
> 在地图输出给规划器前，确认当前场景仍满足泊出地图假设。
> 
> ---
> 
> # 17. 失败处理设计
> 
> ---
> 
> ## 17.1 失败记录接口
> 
> ### 函数
> - `APAMAP_Setfailcause(...)`
> 
> ### 职责
> 记录当前建图或更新过程中的失败原因码，供：
> 
> - 上层状态机处理
> - 调试分析
> - HMI 提示
> - 数据回放定位
> 
> ---
> 
> ## 17.2 失败类型归类
> 
> ### A. 车位获取失败
> 可能原因：
> - 未找到目标 SlotID
> - FusionMode 对应数据源无有效车位
> 
> ### B. 车位几何失败
> 可能原因：
> - 车位框不合法
> - Obj1/Obj2 无法构造
> - 宽深异常
> 
> ### C. 空间不足失败
> 可能原因：
> - FSD/OD/Sensor 收缩后剩余宽度不足
> 
> ### D. 地图边界失败
> 可能原因：
> - 边界点数不足
> - 无法形成有效折线
> 
> ### E. 当前车姿非法
> 可能原因：
> - 车姿与车位模式严重不匹配
> - 当前车已不在有效泊出区域
> 
> ### F. 终点不可落
> 可能原因：
> - 终点持续被侵占
> - 经居中与迭代更新后仍无法找到合法终点
> 
> ---
> 
> # 18. 多源融合策略设计
> 
> ---
> 
> ## 18.1 融合总体原则
> - 先生成默认模型
> - 再用多源感知局部修正
> - 采用保守原则处理空间约束
> - 不同源在不同问题上优先级不同
> 
> ---
> 
> ## 18.2 对终点朝向的优先级
> 推测优先级：
> 
> 1. LaneLine
> 2. RefercLine
> 3. 车位方向
> 4. 当前车身方向兜底
> 
> ---
> 
> ## 18.3 对车位边界收缩的优先级
> 推测优先级：
> 
> 1. Sensor / OD 的实体障碍约束
> 2. FSD 的边缘结构约束
> 3. 默认车位模型
> 
> ---
> 
> ## 18.4 对运行时边界外延的优先级
> 推测优先级：
> 
> 4. LaneLine / RefercLine
> 5. FSD
> 6. 默认边界模板
> 
> ---
> 
> # 19. 典型执行时序
> 
> ---
> 
> ## 19.1 首次建图时序
> 7. `APAMap_ParkingOutTask()`
> 8. `APAMap_ParkingOutDebugInit()`
> 9. `APAMap_ParkingOutCalMapSlotPar()`
> 10. `APAMap_ParkingOutCalSlotInfo()`
> 11. `APAMap_ParkingOutCalMapInfo()`
> 12. `APAMap_ParkingOutCheckIfCarPosIsValid()`
> 13. 输出地图或 fail cause
> 
> ---
> 
> ## 19.2 更新帧时序
> 14. `APAMap_ParkingOutTask()`
> 15. 判断更新帧条件成立
> 16. `APAMap_ParkingOutUpDataMapInfo()`
> 17. 更新边界/终点/侵占状态
> 18. 输出更新后的地图
> 
> ---
> 
> # 20. 对下游规划器的接口语义
> 
> 本模块输出给规划器的核心信息可以抽象为：
> 
> ## 20.1 几何约束
> - 左右边界折线
> 
> ## 20.2 目标约束
> - 终点位姿 `EndPos`
> 
> ## 20.3 场景语义
> - 当前是否仍在车位内/已进入通道
> - 当前地图是否有效
> 
> ## 20.4 异常状态
> - 失败原因码
> - 是否需要放弃本次泊出地图
> 
> ---
> 
> # 21. 设计特点总结
> 
> 本模块具备如下典型特点：
> 
> 1. **模板驱动 + 感知修正**
> 2. **首次建图与运行更新分离**
> 3. **车位内与通道阶段渐进切换**
> 4. **终点位姿可迭代修正**
> 5. **边界与终点耦合一致性较强**
> 6. **多源融合具有明显优先级**
> 7. **具备工程化的失败保护机制**
> 
> ---
> 
> # 22. 风险点与调试重点
> 
> ---
> 
> ## 22.1 风险点
> 8. 目标车位选错
> 9. `Obj1/Obj2` 构造偏差
> 10. LaneLine / RefercLine 选线错误
> 11. 更新帧地图抖动
> 12. 终点修正过度或不足
> 13. 锚点切换时边界/终点不连续
> 
> ---
> 
> ## 22.2 调试重点变量
> - `SlotID / SlotIndex / FusSlotIndex`
> - `Obj1Pt / Obj2Pt`
> - `SlotBordPt`
> - `EndPos / EndPosLine`
> - `LeftBoundary / RightBoundary`
> - `bLaneLineUpdateEndPosFlag`
> - `bReferLineUpdateEndPosFlag`
> - `bAfterNewAnchorPointFlag`
> - `bSeizeEndCarPosFlag`
> - `OffsetX/Y`
> - fail cause
> 
> ---
> 
> # 23. 后续建议
> 
> 若需要将本文档进一步精确化，建议补充以下内容：
> 
> 1. `MapParkingOut.c/.cpp` 具体实现源码
> 2. `APAMap_GInfo` / `APAMap_GInputData` 结构体定义
> 3. fail cause 枚举表
> 4. 车位模式枚举定义
> 5. 关键阈值参数表
> 
> 补充后可进一步完善为：
> 
> - 逐函数设计说明
> - 逐变量定义文档
> - 状态机图
> - 数据流图
> - 时序图
> - 失败场景对照表
> 
> ---
> 
> 如果你愿意，我下一步可以继续直接帮你把这份文档升级成下面两种之一：
> 
> 1. **更正式的软件设计说明书版本**  
>    包括：模块概述、接口定义、状态机、时序、异常处理、测试点
> 
> 2. **更适合源码注释/知识传承的版本**  
>    包括：每个函数一节、输入输出、核心逻辑、注意事项、常见 bug 点
> 
> 如果你要，我可以直接继续输出。

#### _User_

> 继续把文档生成更适合源码注释/知识传承的版本

#### _Assistant_

> **Creating a Document Structure**
> 
> It seems the user needs the answer in Chinese, so no tools are necessary. I should continue with the document in a way that's suitable for source comments and knowledge transfer. I'll probably want to structure it by function, detailing inputs, outputs, logic, and potential pitfalls. I think I should include inferred information without being too terse. My goal is to craft a comprehensive yet manageable document that conveys the necessary knowledge effectively.**Creating a Document Structure**
> 
> It seems the user needs the answer in Chinese, so no tools are necessary. I should continue with the document in a way that's suitable for source comments and knowledge transfer. I'll probably want to structure it by function, detailing inputs, outputs, logic, and potential pitfalls. I think I should include inferred information without being too terse. My goal is to craft a comprehensive yet manageable document that conveys the necessary knowledge effectively.

#### _Assistant_

> 下面我把前一版“模块设计文档”继续整理成一版**更适合源码注释 / 知识传承 / 新人接手阅读**的版本。
> 
> 这版会更偏“工程手册”风格，重点回答：
> 
> - 这个函数是干什么的
> - 为什么会有这个函数
> - 输入依赖什么
> - 输出改了哪些全局量
> - 成功/失败分别意味着什么
> - 阅读和调试时要盯哪些变量
> - 常见坑点是什么
> 
> 我会尽量按**源码阅读顺序**组织。
> 
> ---
> 
> # MapParkingOut 模块知识传承文档（源码注释版 / 反推版）
> 
> ---
> 
> # 1. 文档用途
> 
> 本文档用于帮助后来维护该模块的开发者快速理解：
> 
> - 泊出地图模块整体架构
> - 各关键函数的职责边界
> - 主要数据流转关系
> - 多源融合逻辑
> - 终点位姿修正逻辑
> - 常见问题定位方法
> 
> > 说明：本文档基于函数名、调用链和典型 APA 工程逻辑反推，不保证与原始设计逐字一致，但适合用于阅读源码和接手维护。
> 
> ---
> 
> # 2. 阅读这份源码前先记住三句话
> 
> ## 2.1 这不是轨迹规划器
> 这个模块主要做的是：
> 
> - 选车位
> - 算车位边界
> - 算通道边界
> - 算终点位姿
> - 判断当前地图是否可用
> 
> 它输出的是**“规划约束”**，不是最终轨迹。
> 
> ---
> 
> ## 2.2 这不是纯感知模块
> 它不是完全相信感知结果直接出地图，而是：
> 
> > 先生成模板地图，再用感知修正。
> 
> 所以源码里你会看到很多“默认值 + fallback + 融合 + 修补”的逻辑。
> 
> ---
> 
> ## 2.3 `EndPos` 和边界是耦合的
> 不要把终点位姿和左右边界分开理解。  
> 这套模块的特点是：
> 
> - 车位修了，终点可能重算
> - 边界修了，终点可能被侵占
> - 终点变了，又可能反过来要求边界/搜索更新
> 
> 所以这是一个“**边界—终点联动系统**”。
> 
> ---
> 
> # 3. 建议的源码阅读顺序
> 
> 如果你第一次读这个模块，建议按这个顺序：
> 
> ## 第一组：主流程
> 1. `APAMap_ParkingOutTask()`
> 2. `APAMap_ParkingOutCalMapInfo()`
> 3. `APAMap_ParkingOutUpDataMapInfo()`
> 
> 先搞清：**什么时候重建，什么时候更新。**
> 
> ---
> 
> ## 第二组：车位构造
> 4. `APAMap_ParkingOutCalMapSlotPar()`
> 5. `APAMap_ParkingOutCalSlotInfo()`
> 6. `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> 7. `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> 8. `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> 再搞清：**目标车位是怎么被抽象成几何的。**
> 
> ---
> 
> ## 第三组：边界融合
> 9. `APAMap_ParkingOutCalBoundaryByParkOutInfo()`
> 10. `APAMap_ParkingOutFusBoundaryByFSDMapInfo()`
> 11. `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()`
> 12. `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()`
> 13. `APAMap_FusBoundaryByODMapInfo()`
> 14. `APAMap_SmoothMapBoundary()`
> 
> 再搞清：**默认通道是怎么变成真实通道的。**
> 
> ---
> 
> ## 第四组：终点位姿
> 15. `APAMap_ParkingOutSetEndCarPosInOldCorSys()`
> 16. `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()`
> 17. `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()`
> 18. `APAMap_ParkingOutCenterEndCarPosInfo()`
> 19. `APAMap_ParkingOutEndCarPosUpdata()`
> 
> 最后搞清：**车最终想去哪，为什么会挪来挪去。**
> 
> ---
> 
> # 4. 模块核心状态量总览
> 
> 读源码时优先盯下面这些量。
> 
> ---
> 
> ## 4.1 输入总线
> ### `APAMap_GInputData`
> 模块绝大多数输入都从这里来。
> 
> 建议优先看它里面这几类：
> 
> - `ParkReqPar`
> - `CarLocInfo`
> - `TotalMapInfo`
> - `USSObjInfo / PAObjInfo`
> - 车位源（Usslot / Vplslot / FusSlot）
> 
> ---
> 
> ## 4.2 输出 / 内部总状态
> ### `APAMap_GInfo`
> 模块计算结果基本都落在这里。
> 
> 重点看：
> 
> - `SlotPar`
> - `OutLine`
> - 坐标系切换信息
> - 一些标志位
> 
> ---
> 
> ## 4.3 车位相关核心量
> ### `SlotPar`
> 可能包含：
> 
> - `SlotID`
> - `SlotIndex`
> - `FusSlotIndex`
> - `SlotSide`
> - `SlotLen`
> - `SlotDepth`
> - `Obj1Pt`
> - `Obj2Pt`
> - `SlotBordPt[4]`
> - `EndPos`
> - `EndPosLine`
> 
> 这部分是理解整个模块的关键。
> 
> ---
> 
> ## 4.4 边界相关核心量
> - `MainBoundary`
> - `SubBoundary`
> - `LeftBoundary`
> - `RightBoundary`
> - `PtNum`
> 
> ---
> 
> ## 4.5 标志位
> 重点看这些语义：
> 
> - `bLaneLineUpdateEndPosFlag`
> - `bReferLineUpdateEndPosFlag`
> - `bAfterNewAnchorPointFlag`
> - `bSeizeEndCarPosFlag`
> 
> 它们经常决定后续分支。
> 
> ---
> 
> # 5. 总控函数注释说明
> 
> ---
> 
> ## 5.1 `APAMap_ParkingOutTask()`
> 
> ### 函数职责
> 泊出地图模块主入口。  
> 负责判断当前是：
> 
> - 新请求首次建图
> - 同一请求执行中的更新帧
> - 无效请求/异常状态
> 
> 并调度后续建图或更新逻辑。
> 
> ---
> 
> ### 为什么会有这个函数
> 因为泊出地图不是每帧都要从零开始构建。  
> 工程上需要区分：
> 
> - **初始化阶段**
> - **运行维护阶段**
> 
> 这个函数就是总调度器。
> 
> ---
> 
> ### 主要输入依赖
> 从 `APAMap_GInputData.ParkReqPar` 中读取：
> 
> - `Request_cmd`
> - `request_cnt`
> - `lastreqcnt`
> - `APARunningstate`
> - `parkmode`
> - `parkoutmode`
> 
> ---
> 
> ### 主要调用链
> 典型首次建图：
> 
> 1. `APAMap_ParkingOutDebugInit()`
> 2. `APAMap_ParkingOutCalMapSlotPar()`
> 3. `APAMap_ParkingOutCalSlotInfo()`
> 4. `APAMap_ParkingOutCalMapInfo()`
> 5. `APAMap_ParkingOutCheckIfCarPosIsValid()`
> 
> 典型更新帧：
> 
> 6. `APAMap_ParkingOutUpDataMapInfo()`
> 
> ---
> 
> ### 主要输出影响
> - 更新 `APAMap_GInfo` 中整个泊出地图状态
> - 设置 fail cause
> - 决定是否继续输出有效地图
> 
> ---
> 
> ### 阅读时重点关注
> - “更新帧条件”具体是什么
> - 首次建图失败后如何退出
> - 边界点数不足时如何处理
> - `Request_cmd == 1 / 6` 分别代表什么
> 
> ---
> 
> ### 常见坑点
> 1. **误把更新帧当首次建图**
>    - 地图会频繁重建，导致抖动
> 
> 2. **误把首次建图当更新帧**
>    - 还没初始化好就开始更新，容易访问旧数据
> 
> 3. **request_cnt / lastreqcnt 逻辑理解错**
>    - 很容易导致状态机异常
> 
> ---
> 
> ### 建议在源码边上加的注释
> 可以补类似说明：
> 
> > 判断本帧是否为同一泊出请求的运行更新帧。  
> > 若是，则只做增量地图维护，避免重建引起地图抖动。
> 
> ---
> 
> ## 5.2 `APAMap_ParkingOutDebugInit()`
> 
> ### 函数职责
> 重置本次泊出建图相关的调试状态与标志位。
> 
> ---
> 
> ### 为什么需要它
> 泊出地图是跨帧运行的，如果不清理：
> 
> - 上一帧的终点更新标记会污染本帧
> - 上一帧的边界侵占状态会影响本帧判断
> - 调试输出会失真
> 
> ---
> 
> ### 典型会重置的内容
> 推测包括：
> 
> - 终点由车道线更新标志
> - 终点由参考线更新标志
> - 终点被侵占标志
> - 锚点切换标志
> - 调试缓存
> - fail cause 初值
> 
> ---
> 
> ### 常见坑点
> - 漏清某个标志位，导致后续分支“误以为已经更新过”
> - 把应跨帧保持的状态也清掉，导致地图不稳定
> 
> ---
> 
> # 6. 车位参数与几何构造函数注释说明
> 
> ---
> 
> ## 6.1 `APAMap_ParkingOutCalMapSlotPar()`
> 
> ### 函数职责
> 确定当前泊出任务对应的**目标车位**，并提取目标车位的基础参数。
> 
> ---
> 
> ### 为什么需要它
> 后续所有几何计算的前提都是：
> 
> > 先知道要从哪个车位泊出。
> 
> 如果车位选错，后面所有边界、终点都会错。
> 
> ---
> 
> ### 主要输入依赖
> - `Request_SlotId`
> - `Request_SlotFusionMode`
> - `parkmode`
> - `parkoutmode`
> - `parkside`
> - `SlotDataIsNotMirrored`
> 
> 以及各类车位源：
> 
> - US Slot
> - VPL Slot
> - Fusion Slot
> 
> ---
> 
> ### 主要输出
> 通常写入 `APAMap_GInfo.SlotPar`，例如：
> 
> - `SlotID`
> - `SlotIndex`
> - `FusSlotIndex`
> - `SlotSide`
> - `SlotLen`
> - `SlotDepth`
> - 车位左右侧信息
> 
> ---
> 
> ### 函数内部通常做什么
> 1. 判断车位在左侧还是右侧
> 2. 判断感知数据是否需要镜像
> 3. 根据 fusion mode 选择车位源
> 4. 根据 SlotID 查找目标车位
> 5. 初始化多源边界点缓存
> 
> ---
> 
> ### 成功意味着什么
> 说明：
> 
> - 已经找到本次要泊出的目标车位
> - 后续可以开始构造车位几何
> 
> ---
> 
> ### 失败意味着什么
> 常见是：
> 
> - 没找到目标车位
> - 车位源数据无效
> - 当前请求参数与现场不匹配
> 
> 这类失败一般是“建图前提不成立”。
> 
> ---
> 
> ### 阅读时重点关注
> - 左右侧判断逻辑
> - 镜像逻辑
> - 不同 FusionMode 的查找分支
> - 特殊 parkmode 是否直接绕过 slot 查找
> 
> ---
> 
> ### 常见坑点
> 1. **左/右侧判反**
>    - 后面边界整体翻转
> 
> 2. **Slot 数据镜像处理错误**
>    - 斜列位尤其容易出问题
> 
> 3. **SlotIndex / FusSlotIndex 混用**
>    - 调试时很容易看错车位来源
> 
> ---
> 
> ### 建议补充的源码注释
> > 根据请求 SlotID 和车位融合模式，锁定目标车位；  
> > 同时确定车位所在侧及是否需要镜像，以统一后续几何构造方向。
> 
> ---
> 
> ## 6.2 `APAMap_ParkingOutCalSlotInfo()`
> 
> ### 函数职责
> 构造目标车位的几何边界信息。
> 
> ---
> 
> ### 为什么需要它
> `CalMapSlotPar()` 只解决“哪个车位”，  
> `CalSlotInfo()` 解决的是：
> 
> > “这个车位在几何上长什么样？”
> 
> 也就是把抽象车位转成：
> 
> - `Obj1Pt`
> - `Obj2Pt`
> - `SlotBordPt`
> - 初始 `EndPos`
> 
> ---
> 
> ### 主要输入依赖
> - `SlotPar` 中的目标车位基础参数
> - 当前车姿
> - 感知车位框
> - FSD / OD / Sensor 信息
> 
> ---
> 
> ### 主要输出
> 更新 `APAMap_GInfo.SlotPar` 中的：
> 
> - `Obj1Pt / Obj2Pt`
> - `SlotBordPt[4]`
> - `Obj1Ang / Obj2Ang`
> - `EndPos`
> - `SlotLen / SlotDepth` 可能也会被修正
> 
> ---
> 
> ### 典型处理顺序
> 1. 尝试用感知车位框直接构造
> 2. 若失败，则用模板方式兜底
> 3. 再用 FSD / OD / Sensor 修边界
> 
> ---
> 
> ### 设计意图
> 这是一个“**优先精确来源，失败再回退**”的分层构造逻辑。
> 
> ---
> 
> ### 常见坑点
> - 误以为这里只做一件事，实际上这是一个调度函数
> - 忽略后半段的 FSD/OD 修正，容易误判为什么 Obj1/Obj2 被改了
> - 初始车位框正确，但后续被感知收缩导致边界变化
> 
> ---
> 
> ## 6.3 `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()`
> 
> ### 函数职责
> 优先从感知给出的车位框信息中，直接提取车位边界点。
> 
> ---
> 
> ### 为什么需要它
> 如果感知层已经给出了高质量车位框，直接利用通常比模板推算更准确。
> 
> ---
> 
> ### 主要输入依赖
> - ParkOut SlotInfo
> - 感知角点/边线
> - 当前车姿
> - 车位模式
> 
> ---
> 
> ### 主要输出
> - 初始 `Obj1Pt / Obj2Pt`
> - `SlotBordPt[4]`
> - 初始 `EndPos`
> 
> ---
> 
> ### 你在源码里重点看什么
> - 四个角点怎么排序
> - 哪两个角点被认作车位口
> - `Obj1/Obj2` 到底取自哪两个点
> - 宽度/深度合法性判断
> 
> ---
> 
> ### 常见坑点
> 1. 角点顺序不统一
> 2. 车位口识别错误
> 3. 左右侧映射与感知坐标定义不一致
> 
> ---
> 
> ## 6.4 `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()`
> 
> ### 函数职责
> 当感知车位框不可靠时，根据泊出模式和当前车姿推一个默认车位框。
> 
> ---
> 
> ### 为什么需要它
> 这是整个模块的**兜底逻辑**。  
> 没有它，感知稍微缺一点就会整张地图失败。
> 
> ---
> 
> ### 主要输入依赖
> - 当前车身位置与姿态
> - 泊出模式
> - 车位标准尺寸参数
> - 泊出侧信息
> 
> ---
> 
> ### 主要输出
> - 一组可用的 `Obj1/Obj2`
> - `SlotBordPt`
> - 默认 `EndPos`
> 
> ---
> 
> ### 工程定位
> 它不追求最真实，而追求：
> 
> > “即使感知不完美，也先给规划器一个合理可用的车位框。”
> 
> ---
> 
> ### 常见坑点
> - 标准尺寸参数太激进或太保守
> - 当前车姿异常时仍强行推车位，导致后续地图很怪
> - 平行/垂直/斜列模式使用了不匹配的模板
> 
> ---
> 
> ## 6.5 `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()`
> 
> ### 函数职责
> 使用 FSD / OD / Sensor 对初始车位边界进行保守修正。
> 
> ---
> 
> ### 为什么这是关键函数
> 因为它决定：
> 
> - 车位边界是否贴近真实环境
> - 车位是否被收缩到太窄
> - `EndPos` 是否需要重算
> 
> 它是“车位几何优化”的核心函数。
> 
> ---
> 
> ### 主要输入依赖
> - 初始 `Obj1/Obj2`
> - 初始 `SlotBordPt`
> - 当前 `EndPos`
> - FSD 边缘信息
> - OD 障碍物信息
> - Sensor 近距信息
> 
> ---
> 
> ### 主要输出
> - 修正后的 `Obj1/Obj2`
> - 修正后的 `SlotBordPt`
> - 可能更新的 `EndPos`
> 
> ---
> 
> ### 内部典型步骤
> 1. 计算 Sensor offset
> 2. 计算 FSD offset
> 3. 计算 OD offset
> 4. 取最保守偏移
> 5. 检查收缩后剩余宽度是否足够
> 6. 根据几何关系更新边界点
> 7. 必要时重算默认终点
> 
> ---
> 
> ### 阅读时重点关注
> - offset 是沿哪个方向定义的
> - 三类 offset 是取 max 还是加和
> - 宽度不足阈值是多少
> - 在什么条件下 offset 被清零
> - 在什么条件下禁止重算终点角度
> 
> ---
> 
> ### 常见坑点
> 1. **认为 FSD/OD 只修边界，不影响终点**
>    - 实际上终点常被联动更新
> 
> 2. **理解错 offset 方向**
>    - 很容易误以为“外扩”，实际可能是“内缩”
> 
> 3. **忽略锚点切换后的特殊逻辑**
>    - 某些阶段可能不再继续收缩原车位边界
> 
> ---
> 
> ### 建议源码注释
> > 基于多源感知对初始车位框进行保守收缩，优先保证车辆包络安全