#include "MapParkingOut.h"

#include "APAMapCfg.h"
#include "AlgCom.h"
#include "Map.h"
#include "MapType.h"
#include "Map_DeadendScenario_Decider.h"
#include "common/log_wrap.h"
#include "data_exchange/someip/planning_data_interface.h"
#include "stdio.h"
#ifdef SUPPORT_PARKING_OUT_SYSTEM
namespace {

// Parking-out runtime flags that used to be scattered as translation-unit
// global BOOLEAN variables. Keeping them in one state object makes reset,
// debugging, and later ownership migration safer.
struct ParkingOutRuntimeFlags {
  BOOLEAN cnt_add = FALSE;  // cnt+1轨迹重算标志位，下一帧会清空FALSE
  BOOLEAN lane_line_update_end_car_pos = FALSE;  // 车道线更新终点位置标志位
  BOOLEAN reference_line_update_end_car_pos = FALSE;  // 车位参考线更新终点位置标志位
  BOOLEAN after_new_anchor_point = FALSE;  // 锚点转换后标志位
  BOOLEAN fsd_in_right_of_end_car_pos = FALSE;  // FSD点位入侵终点位置右边标志位
  BOOLEAN fsd_from_main_slot_border = FALSE;  // 入侵的边界点是否来自主边界标志位
  BOOLEAN fsd_from_sub_slot_border = FALSE;  // 入侵的边界点是否来自子边界标志位
  BOOLEAN fsd_from_main_and_sub_slot_border = FALSE;  // 入侵的边界点是否来自主子边界标志位
  BOOLEAN prevent_step_n_redundant = FALSE;  // 防多走标志位
  BOOLEAN shortest_slot_len = FALSE;  // 水平极小车位标志位
  BOOLEAN short_slot_len = FALSE;  // 水平小车位标志位
  BOOLEAN longest_slot_len = FALSE;  // 水平极大车位标志位
  BOOLEAN carry_out_slot = FALSE;  // 采用车位框标志位
  BOOLEAN label_angled = FALSE;  // 斜列车位框标志位
  BOOLEAN obj_label_ladder = FALSE;  // 斜列阶梯车位框标志位
  BOOLEAN label_angled_parking_out_slot = FALSE;  // 斜列车位泊出车位后标志位
  BOOLEAN od_wheel_chock = FALSE;  // 水平泊出车位内有轮挡标志位
};

struct ParkingOutRuntimeState {
  ParkingOutRuntimeFlags flags;
  tAPAParkProcEightParkingOutModeType eight_mode =
      APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
};

ParkingOutRuntimeState s_parking_out_state;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kEndPosSafeDistanceMm = 250.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kMinValidOdOffsetMm = 50.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultBoundaryReserveMm = 600.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kMinBoundaryRemainMm = 200.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kBoundaryBackoffMm = 100.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kShortestSlotBoundaryBackoffMm = 150.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kMaxObj2OffsetXMm = 1000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParkOutInfoDefaultOffsetXMm = 100.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kSlotInnerSafeDistanceMm = 100.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelSlotMinExtraLenMm = 700.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kPerpendicularSlotMinExtraWidthMm = 500.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kSlotLengthFailMarginMm = 150.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultRoadWidthParallelMm = 5000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultRoadWidthNonParallelMm = 7000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultMainBoundaryOffsetMm = 1000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultNonParallelBoundaryOffsetMm = 2000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultBoundaryObj3Mm = 300.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelLongestSlotExtraLenMm = 2000.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelShortSlotExtraLenMm = 1500.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelShortestSlotExtraLenMm = 1100.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kHeadTurnRoundUpdateXThresholdM = -1.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kRearTurnRoundSlotOffsetUpdateXThresholdM = 2.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultUpdateXThresholdM = 0.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kCntAddObj2KeepXThresholdM = 1.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kInvalidEndPosX = 0xff;
constexpr APA_ENUM_TYPE kMaxEndPosUpdateCount = 9;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kBoundaryWideChannelThresholdMm = 900.0;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kDefaultObjPointLimitXMm = 3000.0;

}  // namespace

tMap_MapBkInfo_BeForeFusSDG_t APAMap_BkDataBfSDGFus;
tMap_MapBkInfo_SDGBkOutPutData_t APAMap_BkSDGOutPutData;


/* ======================== Parking-out local helpers ======================== */
static BOOLEAN APAMap_IsBuildOrRebuildRequest(void) {
  return ((APAMap_GInputData.ParkReqPar.Request_cmd == 1) ||
          (APAMap_GInputData.ParkReqPar.Request_cmd == 6))
             ? TRUE
             : FALSE;
}

static BOOLEAN APAMap_IsParkingOutOrExitMode(void) {
  return ((APAMap_GInputData.ParkReqPar.parkmode ==
           APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
          (APAMap_GInputData.ParkReqPar.parkmode ==
           APA_PARKPROC_PARKING_MODE_PARKEXIT))
             ? TRUE
             : FALSE;
}

static BOOLEAN APAMap_ShouldUpdateMapOnlyForSameRequest(void) {
  return ((APAMap_GInputData.ParkReqPar.APARunningstate >= 4) &&
          (APAMap_GInputData.ParkReqPar.Request_cmd == 1) &&
          (APAMap_GInputData.ParkReqPar.request_cnt == APAMap_GInfo.lastreqcnt))
             ? TRUE
             : FALSE;
}

static void APAMap_ResetCoordinateToNoObject(APACoordinateDataCalFloatType* pPt) {
  pPt->x = NO_OBJ_DISTANCE;
  pPt->y = NO_OBJ_DISTANCE;
}

static void APAMap_ResetSlotObjectPoints(void) {
  for (uint8_t_INF i = 0; i < 2; ++i) {
    APAMap_ResetCoordinateToNoObject(&APAMap_GInfo.SlotPar.VplPt[i]);
    APAMap_ResetCoordinateToNoObject(&APAMap_GInfo.SlotPar.UsPt[i]);
    APAMap_ResetCoordinateToNoObject(&APAMap_GInfo.SlotPar.ODPt[i]);
    APAMap_ResetCoordinateToNoObject(&APAMap_GInfo.SlotPar.FSDPt[i]);
    APAMap_ResetCoordinateToNoObject(&APAMap_GInfo.SlotPar.PAPt[i]);
  }
}

static BOOLEAN APAMap_IsSlotDataAtRightSide(APA_ENUM_TYPE park_side,
                                            uint8_t_INF park_out_mode,
                                            BOOLEAN slot_data_is_not_mirrored) {
  const BOOLEAN park_at_left =
      (park_side == APA_CAR_PARK_AT_LEFT_SIDE) ? TRUE : FALSE;

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    return (park_at_left == slot_data_is_not_mirrored) ? TRUE : FALSE;
  }
  return (park_at_left != slot_data_is_not_mirrored) ? TRUE : FALSE;
}

static APA_ENUM_TYPE APAMap_GetSlotSideByParkSide(APA_ENUM_TYPE park_side) {
  return (park_side == APA_CAR_PARK_AT_RIGHT_SIDE) ? 0 : 1;
}

static BOOLEAN APAMap_FindUsSlot(APA_ENUM_TYPE slot_side,
                                 uint16_t_INF slot_id,
                                 uint8_t_INF* pSlotIndex) {
  uint8_t_INF slot_index;

  for (slot_index = 0; slot_index < APA_SLOT_SUPPORT_MAX_SLOT_NUM; ++slot_index) {
    if (APAMap_GInputData.Usslot.USSlot[slot_side].SlotPar[slot_index].SlotID ==
        slot_id) {
      *pSlotIndex = slot_index;
      return TRUE;
    }
  }

  *pSlotIndex = slot_index;
  return FALSE;
}

static BOOLEAN APAMap_FindVplSlot(APA_ENUM_TYPE slot_side,
                                  uint16_t_INF slot_id,
                                  uint8_t_INF* pSlotIndex) {
  uint8_t_INF slot_index;

  for (slot_index = 0; slot_index < APA_VPL_SLOT_PROC_MAX_VPL_SLOT_NUM;
       ++slot_index) {
    if (APAMap_GInputData.Vplslot.VPLSlot[slot_side].Slot[slot_index].SlotID ==
        slot_id) {
      *pSlotIndex = slot_index;
      return TRUE;
    }
  }

  *pSlotIndex = slot_index;
  return FALSE;
}

static BOOLEAN APAMap_FindFusionSlot(APA_ENUM_TYPE slot_side,
                                     uint16_t_INF slot_id,
                                     uint8_t_INF fusion_mode,
                                     uint8_t_INF* pFusSlotIndex) {
  uint8_t_INF fus_slot_index;

  for (fus_slot_index = 0;
       fus_slot_index < APAMap_GInputData.FusSlot.FusionSlot[slot_side].SlotNum;
       ++fus_slot_index) {
    if ((APAMap_GInputData.FusSlot.FusionSlot[slot_side]
             .Slot[fus_slot_index]
             .FusedByVPLSlotID == slot_id) &&
        (APAMap_GInputData.FusSlot.FusionSlot[slot_side]
             .Slot[fus_slot_index]
             .FusionMode == fusion_mode)) {
      *pFusSlotIndex = fus_slot_index;
      return TRUE;
    }
  }

  *pFusSlotIndex = fus_slot_index;
  return FALSE;
}

static void APAMap_SaveSelectedSlotPar(uint16_t_INF slot_id,
                                       APA_ENUM_TYPE slot_side,
                                       uint8_t_INF slot_index,
                                       uint8_t_INF fus_slot_index) {
  APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
  APAMap_GInfo.SlotPar.SlotID = slot_id;
  APAMap_GInfo.SlotPar.SlotSide = slot_side;
  APAMap_GInfo.SlotPar.SlotIndex = slot_index;
  APAMap_GInfo.SlotPar.FusSlotIndex = fus_slot_index;
}

static void APAMap_LogFirstParkOutBuild(void) {
  char log_string[512];
  snprintf(log_string, sizeof(log_string),
           "==First APAMapParkout Build cmd(%d)===request_cnt(%lld)==",
           APAMap_GInputData.ParkReqPar.Request_cmd,
           APAMap_GInputData.ParkReqPar.request_cnt);
  TLOG_INFO << log_string;
}

static void APAMap_ResetDebugWhenParkOutFinished(void) {
  if (APAMap_GInputData.ParkReqPar.APARunningstate >= 7) {
    APAMap_ParkingOutDebugInit();
  }
}

static BOOLEAN APAMap_UpdateMapOnlyOrSetFailCause(void) {
  if ((APAMap_GInfo.OutLine.LeftBoundary.PtNum < 2) ||
      (APAMap_GInfo.OutLine.RightBoundary.PtNum < 2)) {
    TLOG_DEBUG << "start APAMAP_Setfailcause(59)...";
    APAMap_DataInit();
    APAMap_ParkingOutDebugInit();
    APAMAP_Setfailcause(59);
    return FALSE;
  }

  APAMap_ParkingOutUpDataMapInfo();
  APAMap_GInfo.bCalResult = TRUE;
  return TRUE;
}

static void APAMap_ResetParkingOutRuntimeState(void) {
  s_parking_out_state = ParkingOutRuntimeState{};
  s_parking_out_state.eight_mode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
}

static void APAMap_ResetElectronicFenceOutput(void) {
  for (uint16_t i = 0; i < ElectrFencePtNum; ++i) {
    APAMapEFOutputData.ElectronicFencePt[i].x = 0.0;
    APAMapEFOutputData.ElectronicFencePt[i].y = 0.0;
  }

  APAMapEFOutputData.CarPos.CarAng = 0.0;
  APAMapEFOutputData.CarPos.Coordinate.x = 0.0;
  APAMapEFOutputData.CarPos.Coordinate.y = 0.0;
  APAMapEFOutputData.timeStamp_ms = 0.0;
}

static void APAMap_UpdateMaxOffset(APA_DISTANCE_CAL_FLOAT_TYPE* target,
                                     APA_DISTANCE_CAL_FLOAT_TYPE candidate) {
  if (*target < candidate) {
    *target = candidate;
  }
}

static void APAMap_MergeSlotBorderOffsets(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2,
    APA_DISTANCE_CAL_FLOAT_TYPE candidate_x1,
    APA_DISTANCE_CAL_FLOAT_TYPE candidate_y1,
    APA_DISTANCE_CAL_FLOAT_TYPE candidate_x2,
    APA_DISTANCE_CAL_FLOAT_TYPE candidate_y2) {
  APAMap_UpdateMaxOffset(pOffsetX1, candidate_x1);
  APAMap_UpdateMaxOffset(pOffsetY1, candidate_y1);
  APAMap_UpdateMaxOffset(pOffsetX2, candidate_x2);
  APAMap_UpdateMaxOffset(pOffsetY2, candidate_y2);
}

static void APAMap_ClearSmallOffset(APA_DISTANCE_CAL_FLOAT_TYPE* pOffset,
                                    APA_DISTANCE_CAL_FLOAT_TYPE min_valid_offset) {
  if (*pOffset < min_valid_offset) {
    *pOffset = 0;
  }
}

static void APAMap_FilterSmallOdOffsets(
    APA_DISTANCE_CAL_FLOAT_TYPE* pODOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pODOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pODOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pODOffsetY2) {
  APAMap_ClearSmallOffset(pODOffsetX1, kMinValidOdOffsetMm);
  APAMap_ClearSmallOffset(pODOffsetY1, kMinValidOdOffsetMm);
  APAMap_ClearSmallOffset(pODOffsetX2, kMinValidOdOffsetMm);
  APAMap_ClearSmallOffset(pODOffsetY2, kMinValidOdOffsetMm);
}

static BOOLEAN APAMap_ShouldUpdateDefaultBorderByCarX(
    uint8_t_INF park_out_mode,
    APA_DISTANCE_CAL_FLOAT_TYPE car_coordinate_x_m) {
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
    return (car_coordinate_x_m > kHeadTurnRoundUpdateXThresholdM) ? TRUE : FALSE;
  }
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
    return (car_coordinate_x_m > kRearTurnRoundSlotOffsetUpdateXThresholdM)
               ? TRUE
               : FALSE;
  }
  return (car_coordinate_x_m > kDefaultUpdateXThresholdM) ? TRUE : FALSE;
}

static BOOLEAN APAMap_ShouldUpdateSlotBorderAfterAnchor(
    uint8_t_INF park_out_mode,
    BOOLEAN slot_data_at_right_side,
    APA_DISTANCE_CAL_FLOAT_TYPE car_coordinate_x_m) {
  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    return FALSE;
  }

  if (TRUE == slot_data_at_right_side) {
    car_coordinate_x_m = -car_coordinate_x_m;
  }
  return APAMap_ShouldUpdateDefaultBorderByCarX(park_out_mode,
                                                car_coordinate_x_m);
}

static BOOLEAN APAMap_ApplyAngledSlotDefaultBorderState(
    BOOLEAN update_default_border) {
  if (TRUE == s_parking_out_state.flags.label_angled) {
    if (TRUE == update_default_border) {
      s_parking_out_state.flags.label_angled_parking_out_slot = TRUE;
    }
    if (TRUE == s_parking_out_state.flags.label_angled_parking_out_slot) {
      update_default_border = TRUE;
    }
  }
  return update_default_border;
}

static void APAMap_ResetAllSlotBorderOffsets(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  *pOffsetX1 = 0;
  *pOffsetY1 = 0;
  *pOffsetX2 = 0;
  *pOffsetY2 = 0;
}

static void APAMap_ApplyDefaultSlotBorderOffset(
    uint8_t_INF park_out_mode,
    APA_DISTANCE_CAL_FLOAT_TYPE fDis1,
    APA_DISTANCE_CAL_FLOAT_TYPE fDis2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  APA_DISTANCE_CAL_FLOAT_TYPE default_offset_y1 = 0;
  APA_DISTANCE_CAL_FLOAT_TYPE default_offset_y2 = 0;

  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (FALSE == s_parking_out_state.flags.carry_out_slot)) {
    default_offset_y1 = fDis1 - kDefaultBoundaryReserveMm;
    default_offset_y2 = fDis2 - kDefaultBoundaryReserveMm;
  }

  APAMap_UpdateMaxOffset(pOffsetY1, default_offset_y1);
  APAMap_UpdateMaxOffset(pOffsetY2, default_offset_y2);
}

static void APAMap_ClearSlotYOffsetForSmallSlot(
    uint8_t_INF park_out_mode,
    APA_DISTANCE_CAL_FLOAT_TYPE slot_len,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_len < (APAMap_ComCfg.LengthOfCar + kParallelSlotMinExtraLenMm)) {
      *pOffsetY1 = 0;
      *pOffsetY2 = 0;
    }
    return;
  }

  if (slot_len < (APAMap_ComCfg.WidthOfCar + kPerpendicularSlotMinExtraWidthMm)) {
    *pOffsetY1 = 0;
    *pOffsetY2 = 0;
  }
}

static void APAMap_UpdateParallelSlotLengthFlags(
    uint8_t_INF park_out_mode,
    APA_DISTANCE_CAL_FLOAT_TYPE slot_len) {
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    s_parking_out_state.flags.longest_slot_len = FALSE;
    s_parking_out_state.flags.short_slot_len = FALSE;
    s_parking_out_state.flags.shortest_slot_len = FALSE;

    if (slot_len >
        (APAMap_ComCfg.LengthOfCar + kParallelLongestSlotExtraLenMm)) {
      s_parking_out_state.flags.longest_slot_len = TRUE;
    } else if (slot_len <=
               (APAMap_ComCfg.LengthOfCar + kParallelShortSlotExtraLenMm)) {
      s_parking_out_state.flags.short_slot_len = TRUE;
      if (slot_len <=
          (APAMap_ComCfg.LengthOfCar + kParallelShortestSlotExtraLenMm)) {
        s_parking_out_state.flags.shortest_slot_len = TRUE;
      }
    }
    return;
  }

  s_parking_out_state.flags.short_slot_len = FALSE;
  s_parking_out_state.flags.shortest_slot_len = FALSE;
}

static BOOLEAN APAMap_ShouldKeepCurrentEndPos(void) {
  return ((TRUE == s_parking_out_state.flags.after_new_anchor_point) ||
          (TRUE == s_parking_out_state.flags.lane_line_update_end_car_pos) ||
          (TRUE ==
           s_parking_out_state.flags.reference_line_update_end_car_pos))
             ? TRUE
             : FALSE;
}

static BOOLEAN APAMap_ShouldSuppressObj2XOffsetForCntAdd(
    BOOLEAN slot_data_at_right_side,
    APA_DISTANCE_CAL_FLOAT_TYPE car_coordinate_x_m) {
  if (FALSE == s_parking_out_state.flags.cnt_add) {
    return FALSE;
  }
  if (TRUE == slot_data_at_right_side) {
    car_coordinate_x_m = -car_coordinate_x_m;
  }
  return (car_coordinate_x_m > kCntAddObj2KeepXThresholdM) ? TRUE : FALSE;
}

struct ParkingOutBoundaryDefaultOffsets {
  APA_DISTANCE_CAL_FLOAT_TYPE obj1;
  APA_DISTANCE_CAL_FLOAT_TYPE obj2;
  APA_DISTANCE_CAL_FLOAT_TYPE obj3;
};

static ParkingOutBoundaryDefaultOffsets APAMap_GetInitialBoundaryDefaultOffsets(
    uint8_t_INF park_out_mode) {
  ParkingOutBoundaryDefaultOffsets offsets = {0, 0, 0};

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    offsets.obj1 = kDefaultMainBoundaryOffsetMm;
    offsets.obj2 = kDefaultMainBoundaryOffsetMm;
    if (TRUE == s_parking_out_state.flags.longest_slot_len) {
      offsets.obj3 = 200;
    } else {
      offsets.obj3 = 250;
      if (TRUE == s_parking_out_state.flags.short_slot_len) {
        offsets.obj3 = 300;
      }
      if (TRUE == s_parking_out_state.flags.shortest_slot_len) {
        offsets.obj3 = 400;
      }
    }
    return offsets;
  }

  if (TRUE == s_parking_out_state.flags.label_angled) {
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
      offsets.obj1 = 4000;
      offsets.obj2 = 4000;
    } else {
      offsets.obj1 = kDefaultMainBoundaryOffsetMm;
      offsets.obj2 = kDefaultMainBoundaryOffsetMm;
    }
  } else {
    offsets.obj1 = kDefaultNonParallelBoundaryOffsetMm;
    offsets.obj2 = kDefaultNonParallelBoundaryOffsetMm;
  }
  offsets.obj3 = kDefaultBoundaryObj3Mm;
  return offsets;
}

static void APAMap_UpdateBoundaryFlagsAfterAnchor(
    uint8_t_INF park_out_mode,
    BOOLEAN slot_data_at_right_side,
    APACarCoordinateDataCalFloatType end_pos,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCarCoordinateXM,
    BOOLEAN* pUpdateDefaultBoundary,
    BOOLEAN* pUpdateSubBoundary,
    BOOLEAN* pWideChannelForParallel) {
  *pUpdateDefaultBoundary = FALSE;
  *pUpdateSubBoundary = FALSE;
  *pWideChannelForParallel = FALSE;

  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    return;
  }

  if (TRUE == slot_data_at_right_side) {
    *pCarCoordinateXM = -(*pCarCoordinateXM);
  }

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
    if (*pCarCoordinateXM > kHeadTurnRoundUpdateXThresholdM) {
      *pUpdateDefaultBoundary = TRUE;
    }
  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
    if (*pCarCoordinateXM > kDefaultUpdateXThresholdM) {
      *pUpdateDefaultBoundary = TRUE;
    }
  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (*pCarCoordinateXM > kDefaultUpdateXThresholdM) {
      *pUpdateDefaultBoundary = TRUE;
    } else {
      *pUpdateSubBoundary = TRUE;
    }
  } else {
    if (*pCarCoordinateXM > kDefaultUpdateXThresholdM) {
      *pUpdateDefaultBoundary = TRUE;
    }
  }

  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (MATH_FABS(end_pos.Coordinate.x) >
       (APAMap_ComCfg.HalfWidthOfCar + kBoundaryWideChannelThresholdMm))) {
    *pWideChannelForParallel = TRUE;
  }
}

static void APAMap_UpdateBoundaryDefaultOffsetsWhenCarInSlot(
    uint8_t_INF park_out_mode,
    BOOLEAN wide_channel_for_parallel,
    ParkingOutBoundaryDefaultOffsets* pOffsets) {
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    pOffsets->obj1 = kDefaultMainBoundaryOffsetMm;
    if (TRUE == s_parking_out_state.flags.longest_slot_len) {
      pOffsets->obj2 = 0;
      return;
    }

    // 大于0.9m（通道宽）则允许用保守内缩策略；小于0.9米（通道窄）则用激进的内缩策略。
    if (TRUE == wide_channel_for_parallel) {
      pOffsets->obj2 = 0;
      if (TRUE == s_parking_out_state.flags.short_slot_len) {
        pOffsets->obj2 = 100;  // 50;
      }
      if (TRUE == s_parking_out_state.flags.shortest_slot_len) {
        pOffsets->obj2 = 200;  // 100;
      }
    } else {
      if (FALSE == s_parking_out_state.flags.shortest_slot_len) {
        pOffsets->obj2 = 500;
      } else {
        pOffsets->obj2 = kDefaultMainBoundaryOffsetMm;
      }
    }
    return;
  }

  if ((TRUE == s_parking_out_state.flags.label_angled) &&
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
    pOffsets->obj1 = kDefaultNonParallelBoundaryOffsetMm;
    pOffsets->obj2 = kDefaultNonParallelBoundaryOffsetMm;
  }
#if 0
  else if ((FALSE == s_parking_out_state.flags.label_angled)
    && (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND))
  {
    pOffsets->obj1 = 0;
    pOffsets->obj2 = 0;
  }
#endif
  else {
    pOffsets->obj1 = kDefaultMainBoundaryOffsetMm;
    pOffsets->obj2 = kDefaultMainBoundaryOffsetMm;
  }
}

static APA_DISTANCE_CAL_FLOAT_TYPE APAMap_LimitDefaultObjX(
    APA_DISTANCE_CAL_FLOAT_TYPE obj_x,
    BOOLEAN obj_exist,
    APA_DISTANCE_CAL_FLOAT_TYPE default_limit_x) {
  if ((FALSE == obj_exist) && (obj_x > default_limit_x)) {
    return default_limit_x;
  }
  return obj_x;
}

static void APAMap_SaveParkOutBoundaryBySlotSide(
    BOOLEAN slot_data_at_right_side,
    BOOLEAN update_sub_boundary,
    const tMap_BoundPt_t* pMainBoundary,
    const tMap_BoundPt_t* pSubBoundary) {
  if ((FALSE == s_parking_out_state.flags.after_new_anchor_point) ||
      (TRUE == update_sub_boundary)) {
    if (TRUE == slot_data_at_right_side) {
      APAMap_GInfo.OutLine.LeftBoundary = *pSubBoundary;
      APAMap_GInfo.OutLine.RightBoundary = *pMainBoundary;
    } else {
      APAMap_GInfo.OutLine.LeftBoundary = *pMainBoundary;
      APAMap_GInfo.OutLine.RightBoundary = *pSubBoundary;
    }
    return;
  }

  // 锚点转换之后不再重置子边界，但需要重置主边界。
  if (TRUE == slot_data_at_right_side) {
    APAMap_GInfo.OutLine.RightBoundary = *pMainBoundary;
  } else {
    APAMap_GInfo.OutLine.LeftBoundary = *pMainBoundary;
  }
}

static APACoordinateDataCalFloatType APAMap_GetParkOutEndPosCoordinate(
    uint8_t_INF park_out_mode,
    APACoordinateDataCalFloatType OrgPt,
    APA_DISTANCE_CAL_FLOAT_TYPE OrgAng,
    APACoordinateDataCalFloatType Obj2Pt,
    BOOLEAN bSeizeEndCarPosFlag) {
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    return APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
  }
#endif
  return APAMap_ParkingOutSetEndCarPosInOldCorSys(
      park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
}

static void APAMap_UpdateEndPosUntilBoundaryNotSeized(
    uint8_t_INF park_out_mode,
    APACoordinateDataCalFloatType OrgPt,
    APA_DISTANCE_CAL_FLOAT_TYPE OrgAng,
    APACoordinateDataCalFloatType Obj2Pt) {
  APA_ENUM_TYPE UpdateCnt = 0;
  BOOLEAN bCenterEndCarPosFlag = FALSE;
  BOOLEAN bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();

  if (TRUE == bSeizeEndCarPosFlag) {
    bCenterEndCarPosFlag = APAMap_ParkingOutCenterEndCarPosInfo();
    if (TRUE == bCenterEndCarPosFlag) {
      bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
    }

    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==CenterEndCarPosUpdata===bCenterEndCarPosFlag(%d)"
             "==bSeizeEndCarPosFlag(%d)==EndPos_Coordinate(%.2f,%.2f)",
             bCenterEndCarPosFlag, bSeizeEndCarPosFlag,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
    TLOG_INFO << log_string;
  }

  while (TRUE == bSeizeEndCarPosFlag) {
    APAMap_GInfo.SlotPar.EndPos.Coordinate = APAMap_GetParkOutEndPosCoordinate(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();

    ++UpdateCnt;
    if (UpdateCnt > kMaxEndPosUpdateCount) {
      bSeizeEndCarPosFlag = FALSE;
    }
  }
}

/* ====================== End parking-out local helpers ====================== */
void APAMap_ParkingOutTask() {
  BOOLEAN result = FALSE;

  if (TRUE == APAMap_ShouldUpdateMapOnlyForSameRequest()) {
    APAMap_ParkingOutUpDataMapInfo();
    return;
  }

  if (FALSE == APAMap_IsBuildOrRebuildRequest()) {
    if (FALSE == APAMap_UpdateMapOnlyOrSetFailCause()) {
      return;
    }
    APAMap_ResetDebugWhenParkOutFinished();
    return;
  }

  APAMap_LogFirstParkOutBuild();
  APAMap_GInfo.calcnt++;

  if ((APAMap_GInputData.ParkReqPar.APAstate <= 3) &&
      (APAMap_GInputData.ParkReqPar.APARunningstate >= 1)) {
    APAMap_ParkingOutDebugInit();
  }

  // APAMap_GInfo.status = APAMapStatus_BUSY;
  result = APAMap_ParkingOutCalMapSlotPar();
  if (TRUE == result) {
    result = APAMap_ParkingOutCalSlotInfo();
  }
  if (TRUE == result) {
    result = APAMap_ParkingOutCalMapInfo();
    if (FALSE == result) {
      APAMAP_Setfailcause(45);
    }
  }
  if (TRUE == result) {
    result = APAMap_ParkingOutCheckIfCarPosIsValid();
  }

  if (FALSE == result) {
    APAMap_GInfo.failcalcnt++;
  }
  APAMap_GInfo.bCalResult = result;
  // APAMap_GInfo.status = APAMapStatus_CALFINISHED;

  APAMap_ResetDebugWhenParkOutFinished();
  // APAMap_SetOutputData();
}


BOOLEAN APAMap_ParkingOutCalMapSlotPar() {
  const uint8_t_INF park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  const uint16_t_INF slot_id = APAMap_GInputData.ParkReqPar.Request_SlotId;
  const APA_ENUM_TYPE park_side = APAMap_GInputData.ParkReqPar.parkside;
  const uint8_t_INF fusion_mode =
      APAMap_GInputData.ParkReqPar.Request_SlotFusionMode;

  APA_ENUM_TYPE slot_side = 0;
  uint8_t_INF slot_index = 0;
  uint8_t_INF fus_slot_index = MAP_SLOT_INVALID_INDEX;
  BOOLEAN result = FALSE;

  APAMap_GInfo.SlotPar.bSlotDataAtRigthSide = APAMap_IsSlotDataAtRightSide(
      park_side, park_out_mode, APAMap_GInputData.ParkReqPar.SlotDataIsNotMirrored);
  APAMap_ResetSlotObjectPoints();

  if (TRUE == APAMap_IsParkingOutOrExitMode()) {
    APAMap_GInfo.CarPos = APAMap_GInputData.CarLocInfo.CarPos;
    APAMap_GInfo.SlotPar.SlotID = slot_id;
    APAMap_GInfo.SlotPar.SlotIndex = 0;
    APAMap_GInfo.SlotPar.FusSlotIndex = 0;
    return TRUE;
  }

  if (slot_id == APA_VPL_SLOT_PROC_INVALID_SLOT_ID) {
    APAMAP_Setfailcause(1);
    return FALSE;
  }

  slot_side = APAMap_GetSlotSideByParkSide(park_side);

  if (fusion_mode == APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {
    result = APAMap_FindUsSlot(slot_side, slot_id, &slot_index);
  } else {
    result = APAMap_FindVplSlot(slot_side, slot_id, &slot_index);
    if (fusion_mode != APASLOTFUSIONPROC_FUSION_SLOT_MODE_VPLSLOT) {
      result = APAMap_FindFusionSlot(slot_side, slot_id, fusion_mode,
                                      &fus_slot_index);
    }
  }

  if (FALSE == result) {
    APAMAP_Setfailcause(2);
    return FALSE;
  }

  if (APAMap_GInputData.ParkReqPar.APARunningstate == 0) {
    APAMAP_Resetlastreqcnt();
  }

  APAMap_SaveSelectedSlotPar(slot_id, slot_side, slot_index, fus_slot_index);
  return TRUE;
}


BOOLEAN APAMap_ParkingOutCalSlotInfo() {
  BOOLEAN result;

#ifdef APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO
  result = APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo();
#else
  result = FALSE;
#endif

  if (FALSE == result) {
    result = APAMap_ParkingOutCalSlotBorderPtByParkOutInfo();
  }

#if 1
  if (TRUE == result) {
    result = APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
    debug1++;
  }
#endif

  return result;
}


void APAMap_CalSlotBorderPtOffsetBySensorMapInfo(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  APAMAP_GetSlotBdPtBySensorObjs(0, pOffsetX1, pOffsetY1);
  APAMAP_GetSlotBdPtBySensorObjs(1, pOffsetX2, pOffsetY2);
}


/**
 * @brief 根据FSD和OD地图信息计算停车位边界点
 * @return BOOLEAN 计算成功返回TRUE，失败返回FALSE
 */
BOOLEAN APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo(void) {
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;  // 停车位边界点坐标
  // 声明变量：停车位长度
  APA_DISTANCE_CAL_FLOAT_TYPE SlotLen;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1, fDis2;     // 距离参数
  APALineParameterABCType EndPosLine;           // 停车位终点线参数
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultOffsetY1;  // 默认Y方向偏移量1
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultOffsetY2;  // 默认Y方向偏移量2
  APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetX1;      // FSD X方向偏移量1
  APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetY1;
  APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetX2;  // FSD X方向偏移量2
  APA_DISTANCE_CAL_FLOAT_TYPE FSDOffsetY2;  // FSD Y方向偏移量2
  APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetX1;   // OD X方向偏移量1
  APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetY1;   // OD Y方向偏移量1
  APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetX2;   // OD X方向偏移量2
  APA_DISTANCE_CAL_FLOAT_TYPE ODOffsetY2;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetX1;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetY1;
  // 声明变量：综合偏移量X1和Y1
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetX2;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetY2;
  APA_DISTANCE_CAL_FLOAT_TYPE NewDis1, NewDis2;
  APA_DISTANCE_CAL_FLOAT_TYPE NewDis;
  APALineParameterABCType Line;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDis;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  APACarCoordinateDataCalFloatType TempCarPos1, TempCarPos2;
  APALineParameterABCType TempLine1, TempLine2;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType OrgPt;
  APACarCoordinateDataCalFloatType EndPos;
  BOOLEAN slot_data_at_right_side;
  uint8_t_INF park_out_mode;
  APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetX1;
  APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetY1;
  APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetX2;
  APA_DISTANCE_CAL_FLOAT_TYPE SensorOffsetY2;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
  APACoordinateDataCalFloatType TempPt;
  BOOLEAN bUpdataDefaulBordenFlag;
  APA_DISTANCE_CAL_FLOAT_TYPE BloundaryOffsetY;
  BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位

  // 检查是否为停车出库模式
  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    // return TRUE;
  }
  bSeizeEndCarPosFlag = FALSE;  // 初始化FSD侵占终点位置标志位为FALSE
  SafeDis = kEndPosSafeDistanceMm;                // 设置安全距离为250mm
  // 获取停车位边界点长度
  Obj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];  // APAMap_GInfo.SlotPar.Obj2Pt;
  Obj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];  // APAMap_GInfo.SlotPar.Obj1Pt;
  SlotLen = APAMap_GInfo.SlotPar.SlotLen;
  EndPos = APAMap_GInfo.SlotPar.EndPos;
  EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  BloundaryOffsetY = 0;

  // 根据传感器地图信息计算停车位边界点偏移量
  APAMap_CalSlotBorderPtOffsetBySensorMapInfo(&SensorOffsetX1, &SensorOffsetY1,
                                              &SensorOffsetX2, &SensorOffsetY2);
#if 1
  // 根据顶部视角FSD地图信息计算停车位边界点偏移量
  APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo(&FSDOffsetX1, &FSDOffsetY1,
                                                  &FSDOffsetX2, &FSDOffsetY2);
#else
  FSDOffsetX1 = 0;
  FSDOffsetY1 = 0;
  FSDOffsetX2 = 0;
  FSDOffsetY2 = 0;
#endif
  // 初始化综合偏移量为FSD偏移量
  OffsetX1 = FSDOffsetX1;
  OffsetY1 = FSDOffsetY1;
  OffsetX2 = FSDOffsetX2;
  OffsetY2 = FSDOffsetY2;

  // 确保偏移量不小于传感器偏移量
  APAMap_MergeSlotBorderOffsets(&OffsetX1, &OffsetY1, &OffsetX2, &OffsetY2,
                                SensorOffsetX1, SensorOffsetY1,
                                SensorOffsetX2, SensorOffsetY2);

#if 1
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
  // 使用总地图目标计算OD偏移量
  APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(&ODOffsetX1, &ODOffsetY1,
                                                    &ODOffsetX2, &ODOffsetY2);
#else
  // 根据OD地图信息计算停车位边界点偏移量
  APAMap_CalSlotBorderPtOffsetByODMapInfo(&ODOffsetX1, &ODOffsetY1, &ODOffsetX2,
                                          &ODOffsetY2);
#endif
  // 小于有效门限的OD偏移视为无效
  APAMap_FilterSmallOdOffsets(&ODOffsetX1, &ODOffsetY1, &ODOffsetX2,
                              &ODOffsetY2);
#else
  ODOffsetX1 = 0;
  ODOffsetY1 = 0;
  ODOffsetX2 = 0;
  ODOffsetY2 = 0;
#endif
  // 更新综合偏移量，确保不小于OD偏移量
  APAMap_MergeSlotBorderOffsets(&OffsetX1, &OffsetY1, &OffsetX2, &OffsetY2,
                                ODOffsetX1, ODOffsetY1,
                                ODOffsetX2, ODOffsetY2);

  // zqf:Mix FSD && OD Data Obj1/2
  if (TRUE == s_parking_out_state.flags.carry_out_slot) {
    fDis2 = APAMap_GetSearchMaxInnerY(1, slot_data_at_right_side, Obj2Pt,
                                      APAMap_GInfo.SlotPar.Obj2Ang);
    fDis1 = APAMap_GetSearchMaxInnerY(0, slot_data_at_right_side, Obj1Pt,
                                      APAMap_GInfo.SlotPar.Obj1Ang);
  } else {
    Line = APAMAP_GetSlotLineByCarPos();
    TempDis1 = APAMap_GetDisByCarPosToBumper(1);
    fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, Line) - TempDis1;
    TempDis1 = APAMap_GetDisByCarPosToBumper(0);
    fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, Line) - TempDis1;
  }
  bUpdataDefaulBordenFlag = APAMap_ShouldUpdateSlotBorderAfterAnchor(
      park_out_mode, slot_data_at_right_side, CurCarCoordinateX);
  bUpdataDefaulBordenFlag =
      APAMap_ApplyAngledSlotDefaultBorderState(bUpdataDefaulBordenFlag);

  if (FALSE == bUpdataDefaulBordenFlag) {
    APAMap_ApplyDefaultSlotBorderOffset(park_out_mode, fDis1, fDis2,
                                        &OffsetY1, &OffsetY2);
  } else {
    APAMap_ResetAllSlotBorderOffsets(&OffsetX1, &OffsetY1, &OffsetX2,
                                     &OffsetY2);
  }

  if (APAMap_GInputData.ParkReqPar.Request_SlotFusionMode ==
      APASLOTFUSIONPROC_FUSION_SLOT_MODE_USSLOT) {
    // movedis < 0  move upward;
    NewDis1 = fDis1 - OffsetY1;
    NewDis2 = fDis2 - OffsetY2;
    NewDis = NewDis1 + NewDis2;
    // if (NewDis < 2 * SafeDis) {
    //   APAMAP_Setfailcause(57);
    //   return FALSE;
    // }
  } else {
    NewDis1 = fDis1 - OffsetY1;
    NewDis2 = fDis2 - OffsetY2;
    NewDis = NewDis1 + NewDis2;
    if (NewDis < 2 * SafeDis) {
      APAMAP_Setfailcause(58);
#ifdef DEBUG_PRINT_SLOTOBJ
      char log_string[1024];
      snprintf(log_string, sizeof(log_string),
               "==app==FailCause58Debug==ObjPt2(%.2f,%.2f)=ObjPt1(%.2f,%.2f)=="
               "EndPos(%.2f,%.2f)=="
               "Dis(%.2f,%.2f)==Offset(%.2f,%.2f)==FSDOffset(%.2f,%.2f)=="
               "ODOffset(%.2f,%.2f)=\n==HalfWideCar(%u)=="
               "SensorOffset1(%.2f,%.2f)==SensorOffset2(%.2f,%.2f)==",
               Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, EndPosLine.A,
               EndPosLine.C, fDis1, fDis2, OffsetY1, OffsetY2, FSDOffsetY1,
               FSDOffsetY2, ODOffsetY1, ODOffsetY2,
               APAMap_ComCfg.HalfWidthOfCar, SensorOffsetX1, SensorOffsetY1,
               SensorOffsetX2, SensorOffsetY2);
      TLOG_INFO << log_string;
#endif
      return FALSE;
    }
  }

  APAMap_ClearSlotYOffsetForSmallSlot(park_out_mode,
                                       APAMap_GInfo.SlotPar.SlotLen,
                                       &OffsetY1, &OffsetY2);

  // obj1
  TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj1Ang;
  TempCarPos2.Coordinate = Obj1Pt;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
  if (OffsetY1 != 0) {
    TempDis = OffsetY1;
    if ((fDis1 - OffsetY1) < kMinBoundaryRemainMm) {
      TempDis = OffsetY1 - kBoundaryBackoffMm;
    }
    if (slot_data_at_right_side == FALSE) {
      TempDis = -TempDis;
    }
    TempLine2 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
        &TempLine2, TempCarPos2.CarAng, TempDis);
  }
  TempCarPos1.CarAng = OrgAng;
  TempCarPos1.Coordinate = Obj1Pt;
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
  if (OffsetX1 != 0) {
    TempDis = OffsetX1;
    if (slot_data_at_right_side) {
      TempDis = -TempDis;
    }
    TempLine1 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
        &TempLine1, TempCarPos1.CarAng, TempDis);
  }
  if ((OffsetY1 != 0) || (OffsetX1 != 0)) {
    AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &Obj1Pt);
  }

  // obj2
  TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
  TempCarPos2.Coordinate = Obj2Pt;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
  if (OffsetY2 != 0) {
    TempDis = OffsetY2 + BloundaryOffsetY;
    if ((fDis2 - OffsetY2) < kMinBoundaryRemainMm) {
      BloundaryOffsetY = -kBoundaryBackoffMm;
      if (TRUE == s_parking_out_state.flags.shortest_slot_len) {
        TempDis = OffsetY2 - kShortestSlotBoundaryBackoffMm;
      }
    }
    if (slot_data_at_right_side == TRUE) {
      TempDis = -TempDis;
    }
    TempLine2 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
        &TempLine2, TempCarPos2.CarAng, TempDis);
  }
  TempCarPos1.CarAng = OrgAng;
  TempCarPos1.Coordinate = Obj2Pt;
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
  if (TRUE == APAMap_ShouldSuppressObj2XOffsetForCntAdd(
                  slot_data_at_right_side, CurCarCoordinateX)) {
    OffsetX2 = 0;
  }
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (FALSE == s_parking_out_state.flags.after_new_anchor_point) && (TRUE == s_parking_out_state.flags.short_slot_len)) {
    OffsetX2 = 0;
  }
  if ((OffsetX2 != 0) || (TRUE == s_parking_out_state.flags.short_slot_len)) {
    TempDis = OffsetX2;
    if (TempDis > kMaxObj2OffsetXMm) {
      TempDis = kMaxObj2OffsetXMm;
    }
    if (slot_data_at_right_side) {
      TempDis = -TempDis;
    }
    TempLine1 = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
        &TempLine1, TempCarPos1.CarAng, TempDis);
  }
  if ((OffsetY2 != 0) || (OffsetX2 != 0)) {
    AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &Obj2Pt);
  }
  APAMap_ParkingOutCarPosInvadeSlotBorderInfo(&Obj2Pt, &Obj1Pt,
                                              bUpdataDefaulBordenFlag);
  TempCarPos2.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
  TempCarPos2.Coordinate = Obj2Pt;
  Line = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos2, 0);
  SlotLen = AlgCom_GetPointToLineDis(Obj1Pt, Line);

  APAMap_UpdateParallelSlotLengthFlags(park_out_mode, SlotLen);

  // Recal endpos;
  if (TRUE == APAMap_ShouldKeepCurrentEndPos()) {
    EndPos = APAMap_GInfo.SlotPar.EndPos;
    EndPosLine = APAMap_GInfo.SlotPar.EndPosLine;
  } else {
    // zqf: add EndCarPos update
#ifdef SUPPORT_PARKING_OUT_UWB
    if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
          park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==1.ParkoutUWBPos==Parkout_UWBPos(%d,%d)==EndPos.Coordinate(%"
                 ".2f,%.2f)==",
                 APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x,
                 APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y,
                 EndPos.Coordinate.x, EndPos.Coordinate.y);
        TLOG_INFO << log_string;
      }
    } else {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
          park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==2.Parkout==EndPos.Coordinate(%.2f,%.2f)==",
                 EndPos.Coordinate.x, EndPos.Coordinate.y);
        TLOG_INFO << log_string;
      }
    }
#else
    EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
#endif
    EndPosLine = AlgCom_LineParABCByCurrentCarPosition(&EndPos, 0);
    if (EndPos.Coordinate.x == kInvalidEndPosX) {
      APAMAP_Setfailcause(101);
      return FALSE;
    }
  }
  {
    char log_string[1024];
    snprintf(
        log_string, sizeof(log_string),
        "==app==ObjbyFSDAndOD==ObjPt2(%.2f,%.2f)=ObjPt1(%.2f,%.2f)==EndPos(%."
        "2f,%.2f)==SlotLen(%.2f)==LengthOfCar(%d)==WidthOfCar(%d)"
        "==Dis(%.2f,%.2f)==OffsetX(%.2f,%.2f)==OffsetY(%.2f,%.2f)==FSDOffset(2("
        "%.2f,%.2f),1(%.2f,%.2f))==ODOffset(2(%f,%f),1(%.2f,%.2f))==\n"
        "==SensorOffset1(%.2f,%.2f)==SensorOffset2(%.2f,%.2f)=="
        "bLonggestSlotLen(%d)==bShortSlotLen(%d)==bShortestSlotLen(%d)=="
        "BloundaryOffsetY(%.2f)",
        Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y, EndPosLine.A, EndPosLine.C,
        SlotLen, APAMap_ComCfg.LengthOfCar, APAMap_ComCfg.WidthOfCar, fDis1,
        fDis2, OffsetX1, OffsetX2, OffsetY1, OffsetY2, FSDOffsetX2, FSDOffsetY2,
        FSDOffsetX1, FSDOffsetY1, ODOffsetX2, ODOffsetY2, ODOffsetX1,
        ODOffsetY1, SensorOffsetX1, SensorOffsetY1, SensorOffsetX2,
        SensorOffsetY2, s_parking_out_state.flags.longest_slot_len, s_parking_out_state.flags.short_slot_len, s_parking_out_state.flags.shortest_slot_len,
        BloundaryOffsetY);
    TLOG_INFO << log_string;
  }
  if ((OffsetY2 != 0) || (OffsetX2 != 0)) {
    APAMap_GInfo.SlotPar.bObj2Exist = TRUE;
  }
  if ((OffsetY1 != 0) || (OffsetX1 != 0)) {
    APAMap_GInfo.SlotPar.bObj1Exist = TRUE;
  }
  APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
  APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;
  APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
  APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
  TempCarPos1.CarAng = OrgAng;
  TempCarPos1.Coordinate = OrgPt;
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
  TempCarPos1.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos1, 0);
  if (OffsetX2 != 0) {
    AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, Obj2Pt,
                                                  &TempLine2);
    AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt);
    APAMap_GInfo.SlotPar.SlotBordPt[0] = TempPt;
  }
  if (OffsetX1 != 0) {
    AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, Obj1Pt,
                                                  &TempLine2);
    AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt);
    APAMap_GInfo.SlotPar.SlotBordPt[1] = TempPt;
  }
  APAMap_GInfo.NewCordSysOPt = APAMap_GInfo.SlotPar.SlotBordPt[0];
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SlotBordPt(2(%f,%f),1(%.2f,%.2f))==",
             APAMap_GInfo.SlotPar.SlotBordPt[0].x,
             APAMap_GInfo.SlotPar.SlotBordPt[0].y,
             APAMap_GInfo.SlotPar.SlotBordPt[1].x,
             APAMap_GInfo.SlotPar.SlotBordPt[1].y);
    TLOG_INFO << log_string;
  }
  APAMap_GInfo.SlotPar.SlotLen = (APA_DISTANCE_TYPE)SlotLen;
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x == NO_OBJ_DISTANCE) {
    APAMap_GInfo.SlotPar.EndPos = EndPos;
  }
#else
  APAMap_GInfo.SlotPar.EndPos = EndPos;
#endif
  APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
  return TRUE;
}

APACoordinateDataCalFloatType AlgCom_SetParkOutObj1Pt(
    uint8_t_INF park_out_eight_mode, APACarCoordinateDataCalFloatType CurCarPos) {
  APACoordinateDataCalFloatType Obj1Pt;
  APACoordinateDataCalFloatType TempPt1;
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1, SafeDisCal2;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPARALLELSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPRearSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToDefaultBoundary;

  CarWidth = APAMap_ComCfg.WidthOfCar;                 // mm
  SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0];  // 250mm, 0 paralIn;
  SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 400mm, 1 PerpIn;
  CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
  CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
  SafeDisToPARALLELSlotPt = 2000;                      // mm
  SafeDisToPERPSlotPt = 500;                           // mm
  SafeDisToPERPRearSlotPt = 300;                       // mm
  SafeDisToDefaultBoundary = 0;                        // mm

  if ((park_out_eight_mode ==
       APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_eight_mode ==
       APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)) {
    TempPt1.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                  SafeDisToDefaultBoundary);
    TempPt1.y = CarLFCal + SafeDisCal2;  // - 100;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
    TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                SafeDisToDefaultBoundary;
    TempPt1.y = CarLFCal + SafeDisCal2;  // - 100;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
    TempPt1.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
                  SafeDisToDefaultBoundary);
    TempPt1.y = -(CarLRCal + SafeDisCal2);  // - 100);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) {
    TempPt1.x = -((CarWidth / 2) + SafeDisCal1);
    TempPt1.y = -(CarLRCal + SafeDisCal1 + SafeDisToPARALLELSlotPt);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
    TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                SafeDisToDefaultBoundary;
    TempPt1.y = -(CarLRCal + SafeDisCal2);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
    TempPt1.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
                SafeDisToDefaultBoundary;
    TempPt1.y = -(CarLRCal + SafeDisCal2);  // - 100);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT) {
    TempPt1.x = (CarWidth / 2) + SafeDisCal1;
    TempPt1.y = -(CarLRCal + SafeDisCal1 + SafeDisToPARALLELSlotPt);
  } else {
  }
  Obj1Pt = AlgCom_PointPosWithAngAndCenterPt(TempPt1, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  return Obj1Pt;
}

APACoordinateDataCalFloatType AlgCom_SetParkOutObj2Pt(
    uint8_t_INF park_out_eight_mode, APACarCoordinateDataCalFloatType CurCarPos) {
  APACoordinateDataCalFloatType Obj2Pt;
  APACoordinateDataCalFloatType TempPt2;
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1, SafeDisCal2;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPARALLELSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToPERPRearSlotPt;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToDefaultBoundary;

  CarWidth = APAMap_ComCfg.WidthOfCar;                 // mm
  SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0];  // 250mm, 0 paralIn;
  SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 400mm, 1 PerpIn;
  CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
  CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
  SafeDisToPARALLELSlotPt = 1500;                      // mm
  SafeDisToPERPSlotPt = 500;                           // mm
  SafeDisToPERPRearSlotPt = 300;                       // mm
  SafeDisToDefaultBoundary = 0;                        // mm

  if ((park_out_eight_mode ==
       APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_eight_mode ==
       APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)) {
    TempPt2.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                SafeDisToDefaultBoundary;
    TempPt2.y = CarLFCal + SafeDisCal2;  // - 100;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT) {
    TempPt2.x = (CarWidth / 2) + SafeDisCal1;
    TempPt2.y = CarLFCal + SafeDisCal1 + SafeDisToPARALLELSlotPt;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
    TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                  SafeDisToDefaultBoundary);
    TempPt2.y = CarLFCal + SafeDisCal2;  // - 100;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) {
    TempPt2.x = -((CarWidth / 2) + SafeDisCal1);
    TempPt2.y = CarLFCal + SafeDisCal1 + SafeDisToPARALLELSlotPt;
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
    TempPt2.x = (CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
                SafeDisToDefaultBoundary;
    TempPt2.y = -(CarLRCal + SafeDisCal2);  // - 100);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
    TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPSlotPt +
                  SafeDisToDefaultBoundary);
    TempPt2.y = -(CarLRCal + SafeDisCal2);
  } else if (park_out_eight_mode ==
             APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
    TempPt2.x = -((CarWidth / 2) + SafeDisCal2 + SafeDisToPERPRearSlotPt +
                  SafeDisToDefaultBoundary);
    TempPt2.y = -(CarLRCal + SafeDisCal2);  // - 100);
  } else {
  }
  Obj2Pt = AlgCom_PointPosWithAngAndCenterPt(TempPt2, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  return Obj2Pt;
}

APA_DISTANCE_CAL_FLOAT_TYPE
AlgCom_SetParkingOutObjAng(uint8_t_INF park_out_eight_mode,
                           APA_DISTANCE_CAL_FLOAT_TYPE OrgAng) {
  switch (park_out_eight_mode) {
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT:
      return (APA_DISTANCE_CAL_FLOAT_TYPE)(OrgAng + (PI / 2.0));

    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT:
    case APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT:
      return (APA_DISTANCE_CAL_FLOAT_TYPE)(OrgAng - (PI / 2.0));

    default:
      return 0;
  }
}


void APAMap_ParkingOutDebugInit(void) {
  APAMap_ResetParkingOutRuntimeState();
  APAMap_ResetElectronicFenceOutput();
  APAMap_ParkingOutBkSDGOutPutDataInit();
  APAMap_ParkingOutBkDataBfSDGFusInit();
}


BOOLEAN APAMap_ParkingOutCalSlotBorderPtByParkOutInfo() {
  APA_ENUM_TYPE SlotType;
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
  BOOLEAN bObj2Exist, bObj1Exist;
  BOOLEAN slot_data_at_right_side;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType TempPt1, TempPt2, TempPt3;
  APA_DISTANCE_TYPE SlotLength;
  APA_DISTANCE_TYPE SlotDepth;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
  APALineParameterABCType TempLine;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisToSlotInnerPt;
  APA_ENUM_TYPE park_side;
  BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位

  bSeizeEndCarPosFlag = FALSE;
  OffsetX = kParkOutInfoDefaultOffsetXMm;
  SafeDisToSlotInnerPt = kSlotInnerSafeDistanceMm;  // 300;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  s_parking_out_state.eight_mode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
  // zqf-GetParkOutEightMode
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  s_parking_out_state.eight_mode = AlgCom_GetParkOutEightMode(park_out_mode, park_side);
  // zqf-SetParkOutObjPt
  TLOG_DEBUG << "start AlgCom_SetParkOutObj1Pt...";
  Obj1Pt = AlgCom_SetParkOutObj1Pt(s_parking_out_state.eight_mode, CurCarPos);
  Obj2Pt = AlgCom_SetParkOutObj2Pt(s_parking_out_state.eight_mode, CurCarPos);

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);
  } else {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);
  }
  fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);
  fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);
  SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
  if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - kSlotLengthFailMarginMm)) ||
      ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - kSlotLengthFailMarginMm))) {
    APAMAP_Setfailcause(100);
    return FALSE;
  }
  TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      Obj2Pt, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    // OrgPt.y = APAMap_ComCfg.LenBetweenRAxisAndFBumper + 300.0;
    // TempPt3.y = TempPt2.y;
    // if (slot_data_at_right_side == TRUE) {
    //   TempPt3.x = -(APAMap_ComCfg.HalfWidthOfCar +
    //                 OffsetX);  // slot at right,left paral park out
    // } else {
    //   TempPt3.x = (APAMap_ComCfg.HalfWidthOfCar + OffsetX);
    // }
    OrgAng = 0;
    SlotType = 0;
    SlotDepth = (APA_DISTANCE_TYPE)(APAMap_ComCfg.WidthOfCar + OffsetX + 300);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
    // TempPt3.y = (APAMap_ComCfg.LenBetweenRAxisAndFBumper + OffsetX);
    // TempPt3.x = TempPt2.x;
    if (slot_data_at_right_side == FALSE) {
      OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(PI / 2.0);
    } else {
      OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(-PI / 2.0);
    }
    SlotType = 1;
    SlotDepth = (APA_DISTANCE_TYPE)(APAMap_ComCfg.LengthOfCar + OffsetX + 300);
  } else {
    // TempPt3.y = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + OffsetX);
    // TempPt3.x = TempPt2.x;
    if (slot_data_at_right_side == FALSE) {
      OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(-PI / 2.0);
    } else {
      OrgAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(PI / 2.0);
    }
    SlotType = 1;
    SlotDepth = APAMap_ComCfg.LengthOfCar + (APA_DISTANCE_TYPE)OffsetX + 300;
  }

  OrgAng = OrgAng + CurCarPos.CarAng;
  // OrgPt = AlgCom_PointPosWithAngAndCenterPt(TempPt3, CurCarPos.CarAng,
  //                                           CurCarPos.Coordinate);
  OrgPt = Obj2Pt;
  bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
  bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == FALSE) {
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
    if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
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
  if (slot_data_at_right_side == FALSE) {
    TempPt2.x = -TempPt2.x;
    TempPt3.x = -TempPt3.x;
  }
  if (TempPt2.x > TempPt3.x) {
    MaxSlotPtX = TempPt2.x;
  } else {
    MaxSlotPtX = TempPt3.x;
  }
  MaxSlotPtX += SafeDisToSlotInnerPt;
  APAMap_GInfo.SlotPar.slotCarEndPosXBackUp = MaxSlotPtX;

  APAMap_GInfo.SlotPar.SlotDepth = SlotDepth;
  APAMap_GInfo.SlotPar.SlotLen = SlotLength;
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==1.ParkoutUWBPos==Parkout_UWBPos(%d,%d)==TempPt3(%.2f,%.2f)==",
               APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x,
               APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y, TempPt3.x,
               TempPt3.y);
      TLOG_INFO << log_string;
    }
  } else {
    TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==2.Parkout==EndPos.Coordinate(%.2f,%.2f)==", TempPt3.x,
               TempPt3.y);
      TLOG_INFO << log_string;
    }
  }
#else
  TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
      park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
#endif
  if (TempPt3.x == kInvalidEndPosX) {
    APAMAP_Setfailcause(101);
    return FALSE;
  }
  APAMap_GInfo.SlotPar.EndPos.Coordinate = TempPt3;
  APAMap_GInfo.SlotPar.EndPosLine =
      AlgCom_LineParABCByCurrentCarPosition(&APAMap_GInfo.SlotPar.EndPos, 0);
  APAMap_GInfo.bDataMirrored = FALSE;
  APAMap_GInfo.bCordSysReSet = FALSE;
  APAMap_GInfo.SlotPar.SlotType = SlotType;
  APAMap_GInfo.SlotPar.bObj2Exist = bObj2Exist;
  APAMap_GInfo.SlotPar.bObj1Exist = bObj1Exist;
  APAMap_GInfo.SlotPar.SlotBordPt[0] = Obj2Pt;
  APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1Pt;
  APAMap_GInfo.SlotPar.Obj2Pt = Obj2Pt;
  APAMap_GInfo.SlotPar.Obj1Pt = Obj1Pt;

  APAMap_GInfo.NewCordSysOPt = OrgPt;
  APAMap_GInfo.NewCordSysAng = OrgAng;
  APAMap_GInfo.SlotPar.Obj2Ang =
      AlgCom_SetParkingOutObjAng(s_parking_out_state.eight_mode, OrgAng);
  APAMap_GInfo.SlotPar.Obj1Ang = APAMap_GInfo.SlotPar.Obj2Ang;
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

// zqf: add PAObj for update Obj
void APAMAP_GetSlotBdPtBySensorObjs(APA_ENUM_TYPE Bordpttype,
                                    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,
                                    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY) {
  APA_DISTANCE_TYPE i;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetY;
  APACoordinateDataCalFloatType LineXStrPt;
  APACoordinateDataCalFloatType Data[UPA_APA_SNS_DT_NON_TRIANGLE_OBJ_ARRAY_NUM];
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;
  tMap_PAObjInfo_t* pPAobjInfo;
  BOOLEAN bSearch;
  APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
  APALineParameterABCType TempLine;
  APA_DISTANCE_TYPE FrontMidSnsDis;
  APA_DISTANCE_TYPE RearMidSnsDis;
  uint8_t_INF park_out_mode;
  BOOLEAN slot_data_at_right_side;

  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    *pOffsetX = 0;
    *pOffsetY = 0;
    return;
  }
  pPAobjInfo = &APAMap_GInputData.PAobjInfo;
  MaxOutOffsetY = 1000;
  MaxInnerOffsetX = 2000;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (FALSE == s_parking_out_state.flags.label_angled) {
      MaxInnerOffsetX = 4000;
    }
  } else {
    MaxInnerOffsetX = 1500;
  }
  MaxInnerOffsetY = APAMap_GInfo.SlotPar.SlotLen;

  if (TRUE == s_parking_out_state.flags.carry_out_slot) {
    if (Bordpttype == 0) {
      TempDis = APAMap_GetSearchMaxInnerY(0, slot_data_at_right_side,
                                          APAMap_GInfo.SlotPar.SlotBordPt[1],
                                          APAMap_GInfo.SlotPar.Obj1Ang);
    } else {
      TempDis = APAMap_GetSearchMaxInnerY(1, slot_data_at_right_side,
                                          APAMap_GInfo.SlotPar.SlotBordPt[0],
                                          APAMap_GInfo.SlotPar.Obj2Ang);
    }
  } else {
    TempDis1 = APAMap_GetDisByCarPosToBumper(Bordpttype);
    TempLine = APAMAP_GetSlotLineByCarPos();
    if (Bordpttype == 0) {
      LineXStrPt = APAMap_GInfo.SlotPar.SlotBordPt[1];  // Obj1
    } else {
      LineXStrPt = APAMap_GInfo.SlotPar.SlotBordPt[0];  // Obj2
    }
    TempDis = AlgCom_GetPointToLineDis(LineXStrPt, TempLine);
    TempDis -= TempDis1;
  }
  if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    TempDis -= 300;
  } else {
    TempDis -= 100;
  }
  if (TempDis < 0) {
    TempDis = 0;
  }
  MaxInnerOffsetY = TempDis;
  bSearch = TRUE;
  i = 0;
  OffsetX = -MaxInnerOffsetX;
  OffsetY = -MaxOutOffsetY;
  PreOffsetY = 0;
  PreOffsetX = 0;
  FrontMidSnsDis = NO_OBJ_DISTANCE;
  RearMidSnsDis = NO_OBJ_DISTANCE;
  for (i = 0; i < UPA_APA_SNS_DT_NON_TRIANGLE_OBJ_ARRAY_NUM; i++) {
    if (i < UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM) {
      Data[i].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PARearSys].wX[i];
      Data[i].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PARearSys].wY[i];
      if (RearMidSnsDis > Data[i].y) {
        RearMidSnsDis = Data[i].y;
      }
    } else {
      Data[i].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PAFrontSys]
              .wX[i - UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM];
      Data[i].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pPAobjInfo->SnsNearDis[PAFrontSys]
              .wY[i - UPA_SNS_DT_TRIANGLE_OBJ_ARRAY_NUM];
      if (FrontMidSnsDis > Data[i].y) {
        FrontMidSnsDis = Data[i].y;
      }
    }
  }
  if (bSearch) {
    PreOffsetY = OffsetY;
    PreOffsetX = OffsetX;
    if (Bordpttype == 1) {
      if ((TRUE == APAMap_CheckInputDataIsValidByTimeStamp(
                       APAMap_GInputData.CarLocInfo.timeStamp_ms,
                       pPAobjInfo->timeStamp_ms, 5000)) &&
          (pPAobjInfo->timeStamp_ms != 0)) {
        if (FrontMidSnsDis > pPAobjInfo->SnsNearDis[PAFrontSys]
                                 .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX]) {
          FrontMidSnsDis = pPAobjInfo->SnsNearDis[PAFrontSys]
                               .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX];
        }
        if (FrontMidSnsDis > pPAobjInfo->SnsNearDis[PAFrontSys]
                                 .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX]) {
          FrontMidSnsDis = pPAobjInfo->SnsNearDis[PAFrontSys]
                               .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX];
        }
        if (FrontMidSnsDis >
            pPAobjInfo->SnsNearDis[PAFrontSys].wY[LM_RM_TRIANGLE_OBJ_INDEX]) {
          FrontMidSnsDis =
              pPAobjInfo->SnsNearDis[PAFrontSys].wDis[LM_RM_TRIANGLE_OBJ_INDEX];
        }
        if (FrontMidSnsDis >
            pPAobjInfo->SnsNearDis[PAFrontSys].wY[RM_LM_TRIANGLE_OBJ_INDEX]) {
          FrontMidSnsDis =
              pPAobjInfo->SnsNearDis[PAFrontSys].wDis[RM_LM_TRIANGLE_OBJ_INDEX];
        }
        OffsetY = MaxInnerOffsetY - FrontMidSnsDis;
      } else {
        FrontMidSnsDis = NO_OBJ_DISTANCE;
        OffsetY = 0;
      }
    } else {
      if ((TRUE == APAMap_CheckInputDataIsValidByTimeStamp(
                       APAMap_GInputData.CarLocInfo.timeStamp_ms,
                       pPAobjInfo->timeStamp_ms, 5000)) &&
          (pPAobjInfo->timeStamp_ms != 0)) {
        if (RearMidSnsDis > pPAobjInfo->SnsNearDis[PARearSys]
                                .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX]) {
          RearMidSnsDis = pPAobjInfo->SnsNearDis[PARearSys]
                              .wDis[UPA_APA_SNS_RM_EMIT_RM_RX_OBJ_INDEX];
        }
        if (RearMidSnsDis > pPAobjInfo->SnsNearDis[PARearSys]
                                .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX]) {
          RearMidSnsDis = pPAobjInfo->SnsNearDis[PARearSys]
                              .wDis[UPA_APA_SNS_LM_EMIT_LM_RX_OBJ_INDEX];
        }
        if (RearMidSnsDis >
            pPAobjInfo->SnsNearDis[PARearSys].wY[LM_RM_TRIANGLE_OBJ_INDEX]) {
          RearMidSnsDis =
              pPAobjInfo->SnsNearDis[PARearSys].wDis[LM_RM_TRIANGLE_OBJ_INDEX];
        }
        if (RearMidSnsDis >
            pPAobjInfo->SnsNearDis[PARearSys].wY[RM_LM_TRIANGLE_OBJ_INDEX]) {
          RearMidSnsDis =
              pPAobjInfo->SnsNearDis[PARearSys].wDis[RM_LM_TRIANGLE_OBJ_INDEX];
        }
        OffsetY = MaxInnerOffsetY - RearMidSnsDis;
      } else {
        RearMidSnsDis = NO_OBJ_DISTANCE;
        OffsetY = 0;
      }
    }
    if (OffsetY > MaxInnerOffsetY) {
      OffsetY = MaxInnerOffsetY;
    }
  }
  bSearch = FALSE;
  char log_string[512];
  snprintf(log_string, sizeof(log_string),
           "==PAOffset(%u)(%f,%f),Max(%f),PreOffsetY(%f,%f),FrontMidSnsDis(%d),"
           "RearMidSnsDis(%d)))",
           Bordpttype, OffsetX, OffsetY, MaxInnerOffsetY, PreOffsetX,
           PreOffsetY, FrontMidSnsDis, RearMidSnsDis);
  TLOG_INFO << log_string;
  if (OffsetY > 50) {
    *pOffsetX = 0;
    *pOffsetY = OffsetY;
  } else {
    *pOffsetX = 0;
    *pOffsetY = 0;
  }
  return;
}

BOOLEAN APAMap_ParkingOutCalMapInfo() {
  BOOLEAN result;
  const uint8_t_INF park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  const APA_DISTANCE_CAL_FLOAT_TYPE OrgAng = APAMap_GInfo.NewCordSysAng;
  const APACoordinateDataCalFloatType OrgPt = APAMap_GInfo.NewCordSysOPt;
  const APACoordinateDataCalFloatType Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;

  result = APAMap_ParkingOutCalBoundaryByParkOutInfo();
  BoudaryNum[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;

#ifdef APAMAP_PARKOUT_FUS_PDC
  if (TRUE == result) {
    APAMap_CalMapSubBoundaryByPDCInfo();
  }
#endif

  if (TRUE == result) {
    result = APAMap_ParkingOutFusBoundaryByFSDMapInfo();
    BoudaryNum[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
    debug3++;
  }

  if (TRUE == result) {
    result = APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
    BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }

  if (TRUE == result) {
    result = APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
    BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }

  if (TRUE == result) {
    result = APAMap_FusBoundaryByODMapInfo();
    BoudaryNum[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
    debug4++;
  }

  if (TRUE == result) {
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

  APAMap_UpdateEndPosUntilBoundaryNotSeized(park_out_mode, OrgPt, OrgAng,
                                            Obj2Pt);
  return result;
}


BOOLEAN APAMap_ParkingOutCheckIfCarPosIsValid() {
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Ptcc[4];
  if ((APAMap_GInputData.ParkReqPar.parkmode ==
       APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
      (APAMap_GInputData.ParkReqPar.parkmode ==
       APA_PARKPROC_PARKING_MODE_PARKEXIT)) {
    return TRUE;
  }
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  CurCarPos = APAMap_GInfo.CarPos;
  CurCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      CurCarPos.Coordinate, 0, Angle, Pto);
  CurCarPos.CarAng -= Angle;
  if (slot_data_at_right_side == FALSE) {
    CurCarPos.Coordinate.x = -CurCarPos.Coordinate.x;
    CurCarPos.CarAng = -CurCarPos.CarAng;
  }
  AlgCom_AngNormalized(&CurCarPos.CarAng);
  if (MATH_FABS(CurCarPos.CarAng) > 90.0 * PI / 180.0) {
    APAMAP_Setfailcause(47);
    return FALSE;
  }
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  Ptcc[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                              CurCarPos.Coordinate);
  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
  Ptcc[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                              CurCarPos.Coordinate);
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  Ptcc[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                              CurCarPos.Coordinate);
  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.LenBetweenRAxisAndRBumper;
  Ptcc[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                              CurCarPos.Coordinate);
  if ((Ptcc[0].x > APAMap_ComCfg.HalfWidthOfCar) ||
      (Ptcc[1].x > APAMap_ComCfg.HalfWidthOfCar)) {
    APAMAP_Setfailcause(48);
    return FALSE;
  }
  if ((Ptcc[2].x < -7000.0) || (Ptcc[3].x < -7000.0)) {
    APAMAP_Setfailcause(49);
    return FALSE;
  }
  return TRUE;
}

bool_t_INF APAMap_ParkingOutCalBoundaryByParkOutInfo() {
  APA_DISTANCE_CAL_FLOAT_TYPE MaxY, MinY;
  BOOLEAN slot_data_at_right_side;
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
  ParkingOutBoundaryDefaultOffsets default_offsets;
  uint8_t_INF park_out_mode;
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
  DefaultObj2PtX = kDefaultObjPointLimitXMm;
  DefaultObj1PtX = kDefaultObjPointLimitXMm;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;

  default_offsets = APAMap_GetInitialBoundaryDefaultOffsets(park_out_mode);
  APAMap_UpdateBoundaryFlagsAfterAnchor(park_out_mode, slot_data_at_right_side,
                                        EndPos, &CurCarCoordinateX,
                                        &bUpdataDefaulBordenFlag,
                                        &bUpdataSubBoundaryFlag,
                                        &bWideChannelforParallelFlag);
  if (FALSE == bUpdataDefaulBordenFlag) {
    APAMap_UpdateBoundaryDefaultOffsetsWhenCarInSlot(
        park_out_mode, bWideChannelforParallelFlag, &default_offsets);
  }

  DefaulBordenObj1 = default_offsets.obj1;
  DefaulBordenObj2 = default_offsets.obj2;
  DefaulBordenObj3 = default_offsets.obj3;
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
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;  // APA CorSys
  Obj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
  Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;
  Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;

  bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
  bObj1Exist = APAMap_GInputData.SlotUpData.bObj1Exist;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  MaxSlotPtX = APAMap_GInfo.SlotPar.slotCarEndPosXBackUp + DefaulBordenObj3;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  if (TRUE == s_parking_out_state.flags.label_angled) {
    LabelAngledDis = 1000;
  } else {
    LabelAngledDis = 0;
  }
  MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(5000 + APAMap_ComCfg.LengthOfCar +
                                       LabelAngledDis);
  MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
                                       APAMap_GInfo.SlotPar.SlotLen - 1000);
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    MaxDefaultRoadWith = kDefaultRoadWidthParallelMm;
  } else {
    MaxDefaultRoadWith = kDefaultRoadWidthNonParallelMm;
  }
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    MaxY = (APA_DISTANCE_CAL_FLOAT_TYPE)(12000 + APAMap_ComCfg.LengthOfCar +
                                         APAMap_ComCfg.LengthOfCar);
    MinY = (APA_DISTANCE_CAL_FLOAT_TYPE)(-APAMap_ComCfg.LengthOfCar -
                                         APAMap_GInfo.SlotPar.SlotLen -
                                         APAMap_GInfo.SlotPar.SlotLen - 10000);
    if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
        (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
      RemoContPos.x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)
              APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x;  // APA坐标系下
      RemoContPos.y = (APA_DISTANCE_CAL_FLOAT_TYPE)
                          APAMap_GInputData.ParkReqPar.Parkout_UWBPos.y;
      RemoContPos = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          RemoContPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
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
  if (slot_data_at_right_side == FALSE) {
    Obj2Ang = -Obj2Ang;
    Obj1Ang = -Obj1Ang;
  }
  Obj2Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj2Pt, 0,
                                                               OrgAng, OrgPt);
  Obj1Pt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
                                                               OrgAng, OrgPt);
  if (slot_data_at_right_side == FALSE) {
    Obj2Pt.x = -Obj2Pt.x;
    Obj1Pt.x = -Obj1Pt.x;
  }
  Obj2X = APAMap_LimitDefaultObjX(Obj2Pt.x, bObj2Exist, DefaultObj2PtX);
  Obj1X = APAMap_LimitDefaultObjX(Obj1Pt.x, bObj1Exist, DefaultObj1PtX);
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
    if (slot_data_at_right_side == FALSE) {
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
    if (slot_data_at_right_side == FALSE) {
      TempPt.x = -TempPt.x;
    }
    SubBoundary.Points[i] =
        AlgCom_PointPosWithAngAndCenterPt(TempPt, OrgAng, OrgPt);
  }

  APAMap_SaveParkOutBoundaryBySlotSide(slot_data_at_right_side,
                                          bUpdataSubBoundaryFlag,
                                          &MainBoudary, &SubBoundary);
  APAMap_GInfo.SlotPar.SlotStrIndex = 2;
  APAMap_GInfo.SlotPar.SlotEndIndex = 3;
  APAMap_GInfo.SlotPar.Obj1PtIndex = 1;
  APAMap_GInfo.SlotPar.Obj2PtIndex = 4;
  APAMap_GInfo.SlotPar.SlotBordPt[2] = MainBoudary.Points[2];
  APAMap_GInfo.SlotPar.SlotBordPt[3] = MainBoudary.Points[3];
  return TRUE;
}

APALineParameterABCType APAMap_ParkingOutLineParABCByMainSlotBord(
    APACoordinateDataCalFloatType* MainSlotBordTemp1,
    APACoordinateDataCalFloatType* MainSlotBordTemp2) {
  APALineParameterABCType LineofPar11;  // change LinePar for LineofPar11 //QA.C
  APA_DISTANCE_CAL_FLOAT_TYPE DeltaConstant;
  float k, b;

  LineofPar11 = APAMap_GInfo.SlotPar.EndPosLine;
  DeltaConstant =
      MainSlotBordTemp1->y - APAMap_GInfo.SlotPar.EndPos.Coordinate.y;
  if (MATH_FABS(MainSlotBordTemp1->x - MainSlotBordTemp2->x) <
      __FLT_EPSILON__) {
    return LineofPar11;  // x=C;
  }
  if (MATH_FABS(MainSlotBordTemp1->y - MainSlotBordTemp2->y) <
      __FLT_EPSILON__) {
    return LineofPar11;  // y=C;
  }
  LineofPar11.LineType = APALineIsIncline;
  k = (MainSlotBordTemp2->y - MainSlotBordTemp1->y) /
      (MainSlotBordTemp2->x - MainSlotBordTemp1->x);
  b = (MainSlotBordTemp1->y - k * MainSlotBordTemp1->x) - DeltaConstant;

  if (MATH_FABS(k) < __FLT_EPSILON__) {
    LineofPar11.A = (APA_DISTANCE_CAL_FLOAT_TYPE)0;
    LineofPar11.B = (APA_DISTANCE_CAL_FLOAT_TYPE)-1;
    LineofPar11.C = APAMap_GInfo.SlotPar.EndPos.Coordinate.y;
    return LineofPar11;  // y=C;
  }
  LineofPar11.A = (APA_DISTANCE_CAL_FLOAT_TYPE)k;
  LineofPar11.B = (APA_DISTANCE_CAL_FLOAT_TYPE)-1;
  LineofPar11.C = (APA_DISTANCE_CAL_FLOAT_TYPE)b;

  return LineofPar11;
}

BOOLEAN APAMap_ParkingOutFusBoundaryByFSDMapInfo() {
  APA_ENUM_TYPE park_side;
  APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_ENUM_TYPE Index;
  APA_DISTANCE_TYPE i;
  APA_ENUM_TYPE k;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType pData1[127];
  APACoordinateDataCalFloatType pData2[127];
  APACoordinateDataCalFloatType pData3[127];
  APACoordinateDataCalFloatType pData4[127];
  APACoordinateDataCalFloatType NSegment[127];
  uint8_t_INF pPtStyle[127];
  uint8_t_INF NewProperty1[127];
  uint8_t_INF NewProperty2[127];
  uint8_t_INF NewProperty3[127];
  uint8_t_INF NewProperty4[127];
  uint8_t_INF NSegProperty[127];
  APA_ENUM_TYPE NSegNum;
  uint16_t_INF DataNum;
  APA_ENUM_TYPE Data1Num;
  APA_ENUM_TYPE Data2Num;
  APA_ENUM_TYPE Data3Num;
  APA_ENUM_TYPE Data4Num;
  st_MapTopViewFSD* pTopViewInfo;
  APA_DISTANCE_TYPE TopViewPtNum;
  APA_ENUM_TYPE LocStyle;
  UCHAR CurID;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
  APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
  APACoordinateDataCalFloatType MainLinXStrPt3, MainLinXEndPt3;
  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
  APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
  APACoordinateDataCalFloatType SubLinXStrPt1, SubLinXEndPt1;
  APALineParameterABCType TempLine1;
  APALineParameterABCType TempLine2;
  APACarCoordinateDataCalFloatType TempCarPos;
  APACoordinateDataCalFloatType TempPt1, TempPt2;
  APA_ENUM_TYPE OffsetIndex2, OffsetIndex1;
  APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
  APACoordinateDataCalFloatType pDataBk[127];
  APA_ENUM_TYPE DataNumBk;
  BOOLEAN bCheckSubLane;
  BOOLEAN bFusvalid;
  APACoordinateDataCalFloatType CurCarCoordinate;
  BOOLEAN bUpdataFsdObj2CalBoundaryFlag;
  BOOLEAN bObliqueRowStairsFlag;  // 阶梯斜列式场景标志位
  APA_DISTANCE_CAL_FLOAT_TYPE SubfDis;
#if 0
    APALineParameterABCType EndPosLine;
    APACoordinateDataCalFloatType MainSlotBordTemp1;
    APACoordinateDataCalFloatType MainSlotBordTemp2;
#endif

  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    // return TRUE;
  }
#if 1
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return TRUE;
  }
  pTopViewInfo = &APAMap_GInputData.TotalMapInfo.mapData.FSDInfo.TopView;
#else
  if (APAMap_GInputData.VisObjsInfo.timestamp_ms == 0) {
    return TRUE;
  }
  pTopViewInfo = &APAMap_GInputData.VisObjsInfo.FSDInfo.TopView;
#endif
  APAMap_CheckIfIgnoreFSDPtAtMainBoundary();
  APAMap_CheckIfIgnoreFSDPtAtSubBoundary();
  bObliqueRowStairsFlag = APAMap_ParkingOutObliqueRowStairsInfo();
  if (TRUE == bObliqueRowStairsFlag) {
    MaxOffsetX = 2000;
  } else {
    MaxOffsetX = 1000;  // 600;//700;//2000;
  }
  MaxOffsetY = 1000;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  // zqf:PARALLEL_SIDE
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (park_side == APA_CAR_PARK_AT_RIGHT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  } else {
    if (park_side == APA_CAR_PARK_AT_LEFT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  }
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;

  OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
  CurCarPos = APAMap_GInfo.CarPos;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&TempAng);
  TempAng1 = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
  AlgCom_AngNormalized(&TempAng1);
  if (park_side == APA_CAR_PARK_AT_LEFT_SIDE) {
    fDis = TempAng;
    TempAng = TempAng1;
    TempAng1 = fDis;
  }
  SubfDis = 0;
  if (TRUE == s_parking_out_state.flags.label_angled) {
    SubfDis = 3000;
  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    SubfDis = 1500;
  } else {
    SubfDis = 2000;
  }

  SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
  SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
  Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
  Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
  OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
  OffsetIndex2 = Obj2PtIndex - SlotEndIndex;
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // LineYAngle = Angle;

  // obj1 border line
  MainLinXStrPt1 = OrgObj1Pt;
  MainLinXEndPt1 = MainLinXStrPt1;
  LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
  MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
  Data1Num = 0;
  for (Index = 0; Index <= Obj1PtIndex; Index++) {
    pData1[Data1Num] = pMapMainSlotBord->Points[Index];
    NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
    Data1Num++;
    if (Data1Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==FSD Buffer Not enough==1==Data1Num:(%d)", Data1Num);
        TLOG_INFO << log_string;
      }
      return TRUE;
    }
  }

  // obj2 borderline;
  MainLinXStrPt2 = OrgObj2Pt;
  MainLinXEndPt2 = MainLinXStrPt2;
  LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
  MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
  Data2Num = 0;
  for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
    pData2[Data2Num] = pMapMainSlotBord->Points[Index];
    NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
    Data2Num++;
    if (Data2Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==FSD Buffer Not enough==2==Data2Num:(%d)", Data2Num);
        TLOG_INFO << log_string;
      }
      return TRUE;
    }
  }

  // obj2 borderline2;
  MainLinXStrPt3.x = 0;
  MainLinXStrPt3.y = MaxOffsetY;
  MainLinXEndPt3.x = 1000;
  MainLinXEndPt3.y = MainLinXStrPt3.y;
  MainLinXStrPt3 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt3, Angle, Pto);
  MainLinXEndPt3 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt3, Angle, Pto);
  CurCarCoordinate = APAMap_GInputData.CarLocInfo.CarPos.Coordinate;
  CurCarCoordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      CurCarCoordinate, 0, Angle, Pto);
  bUpdataFsdObj2CalBoundaryFlag = FALSE;
  if (slot_data_at_right_side) {
    CurCarCoordinate.x = -CurCarCoordinate.x;
  }
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
    if (CurCarCoordinate.x < -1) {
      bUpdataFsdObj2CalBoundaryFlag = TRUE;
    }
  } else {
    if (CurCarCoordinate.x < 0) {
      bUpdataFsdObj2CalBoundaryFlag = TRUE;
    }
  }
  // data pt in slot;
  Data3Num = 0;
  for (Index = Obj1PtIndex + 1; Index < Obj2PtIndex; Index++) {
    pData3[Data3Num] = pMapMainSlotBord->Points[Index];
    NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
    Data3Num++;
    if (Data3Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==FSD Buffer Not enough==3==Data3Num:(%d)", Data3Num);
        TLOG_INFO << log_string;
      }
      return TRUE;
    }
  }
  // Fus  Subborder
  MaxOffsetX = 3000;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt.x = TempPt.x;
  SubLinYStrPt.y = 0;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = 1000;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
  // Fus  Subborder
  if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
       (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) &&
      (TRUE == bUpdataFsdObj2CalBoundaryFlag)) {
    MaxOffsetX = 6000;
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
             (TRUE == bUpdataFsdObj2CalBoundaryFlag)) {
    MaxOffsetX = 4000;
  } else {
    MaxOffsetX = 3000;
  }
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt1.x = TempPt.x;
  SubLinYStrPt1.y = 0;
  SubLinYEndPt1.x = SubLinYStrPt1.x;
  SubLinYEndPt1.y = 1000;
  SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
  SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);

  LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&LineXAngle);
  // Fus  Subborder Obj2 Line
  TempCarPos.CarAng = LineXAngle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(SubfDis, &TempLine1);

  TempCarPos.CarAng = Angle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
  SubLinXEndPt = SubLinXStrPt;
  SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
  SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);

  // Fus  Subborder Obj1 Line
  TempCarPos.CarAng = LineXAngle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[1];
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(-SubfDis, &TempLine1);

  TempCarPos.CarAng = Angle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[1];
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt1);
  SubLinXEndPt1 = SubLinXStrPt1;
  SubLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
  SubLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
#if 0
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),"==APAMap_ParkingOutFusBoundaryByFSDMapInfo===SubLinXStrPt(%.2f,%.2f),SubLinXEndPt(%.2f,%.2f)=="
            "SubLinXStrPt1(%.2f,%.2f),SubLinXEndPt1(%.2f,%.2f)==SlotBordPt[0](%.2f,%.2f)==SlotBordPt[1](%.2f,%.2f)=="
            "SubLinYStrPt(%.2f,%.2f),SubLinYEndPt(%.2f,%.2f)==SubLinYStrPt1(%.2f,%.2f),SubLinYEndPt1(%.2f,%.2f)==bUpdataFsdObj2CalBoundaryFlag(%d)",
            SubLinXStrPt.x,
            SubLinXStrPt.y,
            SubLinXEndPt.x,
            SubLinXEndPt.y,
            SubLinXStrPt1.x,
            SubLinXStrPt1.y,
            SubLinXEndPt1.x,
            SubLinXEndPt1.y,
            APAMap_GInfo.SlotPar.SlotBordPt[0].x,
            APAMap_GInfo.SlotPar.SlotBordPt[0].y,
            APAMap_GInfo.SlotPar.SlotBordPt[1].x,
            APAMap_GInfo.SlotPar.SlotBordPt[1].y,
            SubLinYStrPt.x,
            SubLinYStrPt.y,
            SubLinYEndPt.x,
            SubLinYEndPt.y,
            SubLinYStrPt1.x,
            SubLinYStrPt1.y,
            SubLinYEndPt1.x,
            SubLinYEndPt1.y,
            bUpdataFsdObj2CalBoundaryFlag);
        TLOG_INFO << log_string;
      }
#endif
  Data4Num = 0;
  for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
    pData4[Data4Num] = pMapSubSlotBord->Points[Index];
    NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
    Data4Num++;
    if (Data4Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==FSD Buffer Not enough==4==Data4Num:(%d)", Data4Num);
        TLOG_INFO << log_string;
      }
      return TRUE;
    }
  }
  DataNumBk = 0;
  bCheckSubLane = FALSE;
  NSegNum = 0;
  i = 0;
  TopViewPtNum = pTopViewInfo->PointNum;
  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
  while (i < TopViewPtNum) {
    // get fsd data with same id;
    CurID = pTopViewInfo->InfoPoint[i].ID;
    Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.x;
    Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.y;
    for (k = 1; k < 100; k++) {
      if ((i + k) < pTopViewInfo->PointNum) {
        if (pTopViewInfo->InfoPoint[i + k].ID != CurID) {
          break;
        } else {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
                  .Point.x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
                  .Point.y;
        }
      } else {
        break;
      }
    }
    DataNum = k;
    i += DataNum;
    //----------------------------------
    // Get valid fsd data for fus obj1bordline;
    NSegNum = 0;
    for (k = 0; k < DataNum; k++) {
      TempPt = Data[k];
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
      if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
        if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
          NSegment[NSegNum] = TempPt;
          NSegNum++;
        }
      }
    }
    for (k = 0; k < NSegNum; k++) {
      NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
    }
    APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                           Pto, Angle);
    if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                pRectPt, pRectLine)) {
      NSegNum = 0;
    }
    if (TRUE == APAMap_FusTwoLineSegments(
                    slot_data_at_right_side, TempAng, &pData1[0], Data1Num,
                    &NewProperty1[0], &NSegment[0], NSegNum, &NSegProperty[0],
                    &pData1[0], &Data1Num, &pPtStyle[0])) {
      // updata obj1 bordline;
      for (k = 0; k < Data1Num; k++) {
        NewProperty1[k] = pPtStyle[k];
      }
      if (Data1Num > 2) {
        TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pData1[0], 0, Angle, Pto);
        TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pData1[1], 0, Angle, Pto);
        if ((TempPt1.y >= TempPt2.y) &&
            (((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
             ((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
          TempPt1.x = TempPt2.x;
          pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
          for (k = 1; k < Data1Num - 1; k++) {
            NewProperty1[k] = NewProperty1[k + 1];
            pData1[k] = pData1[k + 1];
          }
          Data1Num--;
        }
      }
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string), "==FSDFusionObj1Success==");
        TLOG_INFO << log_string;
      }
    }
    //----------------------------------
    // Get valid fsd data for fus obj2bordline;
    NSegNum = 0;
    for (k = 0; k < DataNum; k++) {
      TempPt = Data[k];
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
      if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
        if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &MainLinXStrPt3, &MainLinXEndPt3, &TempPt, &fDis);
          if (((LocStyle != 0) && (bUpdataFsdObj2CalBoundaryFlag == TRUE)) ||
              (bUpdataFsdObj2CalBoundaryFlag == FALSE)) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        }
      }
    }
    for (k = 0; k < NSegNum; k++) {
      NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
    }
    if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                pRectPt, pRectLine)) {
      NSegNum = 0;
    }
    APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                           Pto, Angle);
    if (TRUE == APAMap_FusTwoLineSegments(
                    slot_data_at_right_side, TempAng, &pData2[0], Data2Num,
                    &NewProperty2[0], &NSegment[0], NSegNum, &NSegProperty[0],
                    &pData2[0], &Data2Num, &pPtStyle[0])) {
      // updata obj2 bordline;
      for (k = 0; k < Data2Num; k++) {
        NewProperty2[k] = pPtStyle[k];
      }
      char log_string[512];
      snprintf(log_string, sizeof(log_string), "==FSDFusionObj2Success==");
      TLOG_INFO << log_string;
    }
    //----------------------------------
    // Get valid fsd data for fus Subbordline;
    NSegNum = 0;
    for (k = 0; k < DataNum; k++) {
      TempPt = Data[k];
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
      if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &SubLinXStrPt1, &SubLinXEndPt1, &TempPt, &fDis);
        if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
          if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        } else {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
          if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        }
      } else {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
        if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
          NSegment[NSegNum] = TempPt;
          NSegNum++;
        }
      }
    }
    for (k = 0; k < NSegNum; k++) {
      NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
    }
    APAMap_ReOderSegmentPt(TRUE, !slot_data_at_right_side, &NSegment[0], &NSegNum,
                           Pto, Angle);
    if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                pRectPt, pRectLine)) {
      NSegNum = 0;
    }
    if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
      for (k = 0; k < Data4Num; k++) {
        pDataBk[k] = pData4[k];
      }
      DataNumBk = Data4Num;
    }
    bFusvalid = FALSE;
    if (TRUE == APAMap_FusTwoLineSegments(
                    !slot_data_at_right_side, TempAng1, &pData4[0], Data4Num,
                    &NewProperty4[0], &NSegment[0], NSegNum, &NSegProperty[0],
                    &pData4[0], &Data4Num, &pPtStyle[0])) {
      if (bCheckSubLane == TRUE) {
        if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &pData4[0], Data4Num,
                                                    pRectPt, pRectLine)) {
          for (k = 0; k < DataNumBk; k++) {
            pData4[k] = pDataBk[k];
          }
          Data4Num = DataNumBk;
        } else {
          bFusvalid = TRUE;
        }
      } else {
        bFusvalid = TRUE;
      }
    }
    if (bFusvalid == TRUE) {
      // updata sublane;
      for (k = 0; k < Data4Num; k++) {
        NewProperty4[k] = pPtStyle[k];
      }
      if (Data4Num > 2) {
        TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pData4[0], 0, Angle, Pto);
        TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pData4[1], 0, Angle, Pto);
        if ((TempPt1.y >= TempPt2.y) &&
            (((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
             ((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
          TempPt1.x = TempPt2.x;
          pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
          for (k = 1; k < Data4Num - 1; k++) {
            NewProperty4[k] = NewProperty4[k + 1];
            pData4[k] = pData4[k + 1];
          }
          Data4Num--;
        }
      }
      char log_string[512];
      snprintf(log_string, sizeof(log_string), "==FSDFusionSubLaneSuccess==");
      TLOG_INFO << log_string;
    }
  }

  DataNum = Data1Num + Data3Num + Data2Num;
  if (DataNum <= FSD_BOUNDARY_PT_MAX_NUM) {
    for (Index = 0; Index < DataNum; Index++) {
      if (Index < Data1Num) {
        pMapMainSlotBord->Points[Index] = pData1[Index];
        pMapMainSlotBord->Property[Index] = NewProperty1[Index];
      } else if (Index < Data1Num + Data3Num) {
        pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
        pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];

      } else {
        pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
        pMapMainSlotBord->Property[Index] =
            NewProperty2[Index - Data1Num - Data3Num];
      }
    }
    pMapMainSlotBord->PtNum = DataNum;
    APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
    APAMap_GInfo.SlotPar.SlotStrIndex =
        APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
    APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
    APAMap_GInfo.SlotPar.SlotEndIndex =
        APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FSDFusionMainSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  if (Data4Num <= FSD_BOUNDARY_PT_MAX_NUM) {
    for (Index = 0; Index < Data4Num; Index++) {
      pMapSubSlotBord->Points[Index] = pData4[Index];
      pMapSubSlotBord->Property[Index] = NewProperty4[Index];
    }
    pMapSubSlotBord->PtNum = Data4Num;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FSDFusionSubSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FusBordByFSD==SlotIndex(%d,%d,%d,%d)==Offset(%d,%d))",
             APAMap_GInfo.SlotPar.Obj1PtIndex,
             APAMap_GInfo.SlotPar.SlotStrIndex,
             APAMap_GInfo.SlotPar.SlotEndIndex,
             APAMap_GInfo.SlotPar.Obj2PtIndex, OffsetIndex1, OffsetIndex2);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
void APAMap_ParkingOutLineParABCbyPoints(
    APACoordinateDataCalFloatType* NSegmentFilter, APA_ENUM_TYPE NSegNum,
    APALineParameterKBType* pLinePar) {
  // 函数根据多个点拟合一条直线
  APA_DISTANCE_CAL_FLOAT_TYPE sumX, sumY, sumXY, sumX2;
  APA_DISTANCE_TYPE i;
  APA_DISTANCE_CAL_FLOAT_TYPE denominator;
  sumX = 0.0;
  sumY = 0.0;
  sumXY = 0.0;
  sumX2 = 0.0;

  for (i = 0; i < NSegNum; i++) {
    sumX += NSegmentFilter[i].x;
    sumY += NSegmentFilter[i].y;
    sumXY += NSegmentFilter[i].x * NSegmentFilter[i].y;
    sumX2 += NSegmentFilter[i].x * NSegmentFilter[i].x;
  }

  denominator = (NSegNum * sumX2) - (sumX * sumX);
  if (denominator < 0.01) {
    pLinePar->K = (APA_DISTANCE_CAL_FLOAT_TYPE)0.0;
    pLinePar->B = (APA_DISTANCE_CAL_FLOAT_TYPE)0.0;
    return;
  }

  pLinePar->K = (APA_DISTANCE_CAL_FLOAT_TYPE)((NSegNum * sumXY - sumX * sumY) /
                                              denominator);
  pLinePar->B = (APA_DISTANCE_CAL_FLOAT_TYPE)((sumY * sumX2 - sumX * sumXY) /
                                              denominator);
  return;
}
#if 1
BOOLEAN APAMap_ParkingOutFusBoundaryByLaneLineMapInfo() {
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_DISTANCE_TYPE i, j, m, n;
  APACoordinateDataCalFloatType temp;
  APACoordinateDataCalFloatType temp2;
  APA_ENUM_TYPE k;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType NSegment[127];
  APACoordinateDataCalFloatType NSegmentFilter[127];
  APACoordinateDataCalFloatType NSegmentFilter2[127];
  APA_ENUM_TYPE NSegNum;
  APA_ENUM_TYPE DataNum;
  st_MapLaneLine* pLaneLineInfo;
  Pt_Cnt_u16_t LaneLinePtNum;
  APA_ENUM_TYPE LocStyle;
  UCHAR CurID;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  uint8_t_INF park_out_mode;
  APALineParameterABCType EndPosLine;
  APACarCoordinateDataCalFloatType TempCarPos;
  // APALineParameterABCType TempLine1, TempLine2;
  APACoordinateDataCalFloatType Obj2Pt;
  APACoordinateDataCalFloatType MainSlotBordTemp5;
  APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLength;
  APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLengthPre;
  APA_DISTANCE_CAL_FLOAT_TYPE LaneLineLengthTemp;
  BOOLEAN bEndCarPosOnTheLeftOfNewSysAngFlag;
  BOOLEAN bLaneLineUpdatePerpFlag;
  BOOLEAN bInclineSlotChangeEndCarPosFlag;  // 斜列式车位更改终点位置标志位
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  BOOLEAN bUpdataLaneLineFlag;
  APA_DISTANCE_CAL_FLOAT_TYPE DebugLaneLine;
  APA_DISTANCE_CAL_FLOAT_TYPE DebugLaneLine2;
  APALineParameterKBType LaneLineKBType;
  static APALineParameterABCType LaneLineABCType;
  APA_DISTANCE_CAL_FLOAT_TYPE PointToLineDis;
  APACoordinateDataCalFloatType DataDebug[127];
  APACoordinateDataCalFloatType DataDebug2[127];
  static BOOLEAN bFindLaneLineFlagByPoints = FALSE;  // 找到车道线标志位
  static BOOLEAN bFindLaneLineFlagByLine = FALSE;    // 找到车道线标志位
  BOOLEAN bSearch;
  static APACoordinateDataCalFloatType MainSlotBordLaneLine1;
  static APACoordinateDataCalFloatType MainSlotBordLaneLine2;

  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    // return TRUE;
  }
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    return TRUE;
  }
#endif
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return TRUE;
  }
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    return TRUE;
  }
  if (TRUE == s_parking_out_state.flags.lane_line_update_end_car_pos) {
    return TRUE;
  }
  // 有车位框且车位框类型为斜列、阶梯斜列、水平的场景平行车位框closeline
  if ((TRUE == s_parking_out_state.flags.carry_out_slot) &&
      ((TRUE == s_parking_out_state.flags.label_angled) ||
       (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL))) {
    return TRUE;
  }
  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    MainSlotBordLaneLine1.x = 0;
    MainSlotBordLaneLine1.y = 0;
    MainSlotBordLaneLine2.x = 0;
    MainSlotBordLaneLine2.y = 0;
  }
  pLaneLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.LaneLineInfo;
  MaxOffsetX = -4000;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  CurCarPos = APAMap_GInfo.CarPos;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }

  // Mainborder
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // LineYAngle = Angle;

  // Subborder
  MaxOffsetX = 5000;
  TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      CurCarPos.Coordinate, 0, Angle, Pto);
  TempDis = APAMap_ComCfg.HalfWidthOfCar;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x -= ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
    if (TempPt.x > -MaxOffsetX) {
      TempPt.x = -MaxOffsetX;
    }
  } else {
    TempPt.x += ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
    if (TempPt.x < MaxOffsetX) {
      TempPt.x = MaxOffsetX;
    }
  }
  SubLinYStrPt.x = TempPt.x;
  SubLinYStrPt.y = 0;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = 1000;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);

  for (i = 0; i < 2; i++) {
    DataDebug[i].x = 0;
    DataDebug[i].y = 0;
    DataDebug2[i].x = 0;
    DataDebug2[i].y = 0;
  }
  i = 0;
  DataNum = 0;
  LaneLinePtNum = pLaneLineInfo->PointNum;
  DebugLaneLine = 0;
  DebugLaneLine2 = 0;
  LaneLineLength = 0;
  LaneLineLengthPre = 0;
  LaneLineLengthTemp = 0;
  MainSlotBordTemp5.x = 0;
  MainSlotBordTemp5.y = 0;
  TempCarPos.CarAng = 0;
  NSegNum = 0;
  bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
  bLaneLineUpdatePerpFlag = FALSE;
  bInclineSlotChangeEndCarPosFlag = FALSE;
#if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
    pLaneLineInfo->PointNum = 4;
    pLaneLineInfo->InfoPoint[0].Point.x = -1600;
    pLaneLineInfo->InfoPoint[0].Point.y = 0;
    pLaneLineInfo->InfoPoint[0].ID = 0;
    pLaneLineInfo->InfoPoint[1].Point.x = -2500;
    pLaneLineInfo->InfoPoint[1].Point.y = 2000;
    pLaneLineInfo->InfoPoint[1].ID = 0;
    pLaneLineInfo->InfoPoint[2].Point.x = -2500;
    pLaneLineInfo->InfoPoint[2].Point.y = 2100;
    pLaneLineInfo->InfoPoint[2].ID = 0;
    pLaneLineInfo->InfoPoint[3].Point.x = -1600;
    pLaneLineInfo->InfoPoint[3].Point.y = 4000;
    pLaneLineInfo->InfoPoint[3].ID = 0;
    LaneLinePtNum = pLaneLineInfo->PointNum;
#endif
  bUpdataLaneLineFlag = FALSE;
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if (CurCarCoordinateX > -2) {
        bUpdataLaneLineFlag = TRUE;
        DebugLaneLine += 100000;
      }
    } else {
      if (CurCarCoordinateX > 0) {
        bUpdataLaneLineFlag = TRUE;
        DebugLaneLine += 100000;
      }
    }
  }
  while (i < LaneLinePtNum) {
    // get LaneLine data with same id;
    CurID = pLaneLineInfo->InfoPoint[i].ID;
    Data[0].x =
        (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i].Point.x;
    Data[0].y =
        (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i].Point.y;
    for (k = 1; k < 100; k++) {
      if ((i + k) < pLaneLineInfo->PointNum) {
        if (pLaneLineInfo->InfoPoint[i + k].ID != CurID) {
          break;
        } else {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i + k]
                  .Point.x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pLaneLineInfo->InfoPoint[i + k]
                  .Point.y;
        }
      } else {
        break;
      }
    }
    DataNum = k;
    i += DataNum;
    if (DataNum < 2) {
      DataNum = 0;
    }
    //----------------------------------
    // Get valid LaneLine data;
    NSegNum = 0;
    for (k = 0; k < DataNum; k++) {
      TempPt = Data[k];
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
      if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
        if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
          NSegment[NSegNum] = TempPt;
          NSegNum++;
          DebugLaneLine += 1;
        }
      }
    }
    // APA转锚点坐标系下
    for (m = 0; m < NSegNum; m++) {
      NSegment[m] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          NSegment[m], 0, Angle, Pto);
    }
    // 第二层数据过滤
    n = 0;
    for (m = 0; m < NSegNum; m++) {
      // 取锚点坐标系下y轴坐标小于-1m的车道线归类为次车道线（SubLaneLine），大于-1m的车道线归类为主车道线（MainLaneLine）；
      // 只对主车道线的数据做处理，次车道线的数据不参与处理，最后只平行主车道线。
      if ((NSegment[m].y < -1000) || (NSegment[m].y > 10000)) {
      } else {
        NSegmentFilter[n] = NSegment[m];
        n++;
        DebugLaneLine += 10;
      }
    }
    NSegNum = n;
    if (NSegNum < 2) {
      NSegNum = 0;
    }
    for (m = 0; m < NSegNum; m++) {
      DataDebug[m] = NSegmentFilter[m];  // 锚点坐标系下
      DataDebug2[m] = AlgCom_PointPosWithAngAndCenterPt(
          NSegmentFilter[m], Angle,
          Pto);  // Debug2转到与锚点相同的坐标系下（可能APA坐标系也可能锚点坐标系）
    }
    //----------------------------------
    // zqf:LaneLine update EndCarPos
    if ((FALSE == s_parking_out_state.flags.lane_line_update_end_car_pos) && (NSegNum >= 2) &&
        (FALSE == bUpdataLaneLineFlag)) {
      DebugLaneLine += 100;
      m = 0;
      LaneLineLength = 0;
      bSearch = TRUE;
      // 首先搜索是否有大于1m的车道线
      while (bSearch) {
        temp = NSegmentFilter[m];
        temp2 = NSegmentFilter[m + 1];
        LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
            temp.x, temp.y, temp2.x, temp2.y);
        if (LaneLineLength > 1500) {
          break;
        }
        if (m >= NSegNum - 2) {
          bSearch = FALSE;
          break;
        }
        m += 2;
      }
      if (TRUE == bSearch) {
        // 其次搜索距离锚点最近的车道线，且车位线长度大于1.5m
        for (n = m; n < NSegNum - 1; n++) {
          if (NSegmentFilter[n].y < temp.y) {
            LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
                NSegmentFilter[n].x, NSegmentFilter[n].y,
                NSegmentFilter[n + 1].x, NSegmentFilter[n + 1].y);
            if (LaneLineLength > 1500) {
              temp = NSegmentFilter[n];
              temp2 = NSegmentFilter[n + 1];
            }
          }
          n++;
        }
        if (temp.y > temp2.y) {
          MainSlotBordLaneLine1 = temp2;
          MainSlotBordLaneLine2 = temp;
        } else {
          MainSlotBordLaneLine1 = temp;
          MainSlotBordLaneLine2 = temp2;
        }
        bFindLaneLineFlagByPoints = TRUE;
        DebugLaneLine += 1000;
      }
      if (FALSE == bFindLaneLineFlagByPoints) {
        // 冒泡排序
        for (m = 0; m < NSegNum - 1; m++) {
          for (n = 0; n < NSegNum - m - 1; n++) {
            if (NSegmentFilter[n].y > NSegmentFilter[n + 1].y) {
              // 交换元素
              temp = NSegmentFilter[n];
              NSegmentFilter[n] = NSegmentFilter[n + 1];
              NSegmentFilter[n + 1] = temp;
            }
          }
        }
        // NSegmentFilter[0].x = NSegmentFilter[NSegNum-1].x + 500; // for test
        // 先默认取同个ID下的第一个点和最后一个点构成车道线
        LaneLineLengthPre = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
            NSegmentFilter[0].x, NSegmentFilter[0].y,
            NSegmentFilter[NSegNum - 1].x, NSegmentFilter[NSegNum - 1].y);
        MainSlotBordLaneLine1 = NSegmentFilter[0];
        MainSlotBordLaneLine2 = NSegmentFilter[NSegNum - 1];
        for (j = 0; j < NSegNum; j++) {
          LaneLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
              NSegmentFilter[0].x, NSegmentFilter[0].y, NSegmentFilter[j].x,
              NSegmentFilter[j].y);
          if (LaneLineLength > LaneLineLengthPre) {
            LaneLineLengthPre = LaneLineLength;
            MainSlotBordLaneLine2 = NSegmentFilter[j];
          }
        }
        LaneLineLengthTemp = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
            MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y,
            MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y);
        if (LaneLineLengthTemp > 2000) {
          bFindLaneLineFlagByLine = TRUE;
          DebugLaneLine += 10000;
        }
        // 多个点先拟合出第一条直线，然后过滤掉超出与直线1m范围的点，再重新拟合第二条直线，以新的直线获取角度存储
        if (TRUE == bFindLaneLineFlagByLine) {
          APAMap_ParkingOutLineParABCbyPoints(&NSegmentFilter[0], NSegNum,
                                              &LaneLineKBType);
          LaneLineABCType = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
          n = 0;
          for (m = 0; m < NSegNum; m++) {
            PointToLineDis =
                AlgCom_GetPointToLineDis(NSegmentFilter[m], LaneLineABCType);
            if (PointToLineDis <= 1000) {
              NSegmentFilter2[n] = NSegmentFilter[m];
              n++;
            }
          }
          if (n >= 2) {
            NSegNum = n;
            APAMap_ParkingOutLineParABCbyPoints(&NSegmentFilter2[0], NSegNum,
                                                &LaneLineKBType);
            LaneLineABCType = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
          }
        }
      }
    }
  }

  if ((FALSE == s_parking_out_state.flags.lane_line_update_end_car_pos) &&
      (TRUE == bUpdataLaneLineFlag) &&
      ((TRUE == bFindLaneLineFlagByLine) ||
       (TRUE == bFindLaneLineFlagByPoints))) {
    DebugLaneLine2 += 1;
    if (TRUE == bFindLaneLineFlagByLine) {
      EndPosLine = LaneLineABCType;
    } else {
      EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(
          &MainSlotBordLaneLine1, &MainSlotBordLaneLine2);
    }
    if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
      DebugLaneLine2 += 10;
      return TRUE;
    }
    // zqf: update EndCarPos
    // TempCarPos.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
    // TempCarPos.Coordinate = APAMap_GInfo.SlotPar.EndPos.Coordinate;
    // TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    if (MATH_ATAN(EndPosLine.A) >= 0) {
      DebugLaneLine2 += 100;
      TempCarPos.CarAng =
          APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
    } else {
      DebugLaneLine2 += 1000;
      TempCarPos.CarAng =
          APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
    }
    if (EndPosLine.LineType != APALineIsIncline) {
      DebugLaneLine2 += 10000;
      TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
    }

    // TempCarPos.Coordinate = MainSlotBordLaneLine1;
    // TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    // TempLine1 = LaneLineABCType;
    // AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2,
    // &MainSlotBordLaneLine2);
    if (APAMap_GInfo.SlotPar.EndPos.CarAng != TempCarPos.CarAng)
    // && (APAMap_GInfo.SlotPar.EndPos.Coordinate.x != MainSlotBordLaneLine1.x))
    {
      DebugLaneLine2 += 100000;
      if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) > (M_PI / 16)) &&
          ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
           (5 * M_PI / 16)) &&
          (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
          (FALSE == s_parking_out_state.flags.carry_out_slot)) {
        DebugLaneLine2 += 1000000;
        bEndCarPosOnTheLeftOfNewSysAngFlag = TRUE;
        s_parking_out_state.flags.lane_line_update_end_car_pos = TRUE;
      } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
                  -(M_PI / 16)) &&
                 ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
                  -(5 * M_PI / 16)) &&
                 (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
                 (FALSE == s_parking_out_state.flags.carry_out_slot)) {
        DebugLaneLine2 += 10000000;
        bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
        s_parking_out_state.flags.lane_line_update_end_car_pos = TRUE;
      } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
                  -(M_PI / 16)) &&
                 ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
                  (M_PI / 16))) {
        DebugLaneLine2 += 100000000;
        s_parking_out_state.flags.lane_line_update_end_car_pos = TRUE;
        bLaneLineUpdatePerpFlag = TRUE;
      }
      if (TRUE == s_parking_out_state.flags.label_angled) {
        DebugLaneLine2 += 1000000000;
        s_parking_out_state.flags.lane_line_update_end_car_pos = FALSE;
        bLaneLineUpdatePerpFlag = FALSE;
      }
    }

    if (TRUE == s_parking_out_state.flags.lane_line_update_end_car_pos) {
      APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
      APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
      InclineSlotOffsetX = 3000;
      InclineSlotOffsetY = 2000;
      MainSlotBordTemp5.x = 1500;
      MainSlotBordTemp5.y = 0;
      if (slot_data_at_right_side == TRUE) {
        MainSlotBordTemp5.x = -MainSlotBordTemp5.x;
      }
      if ((MATH_FABS(MATH_FABS(APAMap_GInfo.SlotPar.Obj2Ang -
                               APAMap_GInfo.NewCordSysAng) -
                     M_PI_2) > (M_PI / 8)) ||
          (TRUE == bLaneLineUpdatePerpFlag)) {
        {
          char log_string[1024];
          snprintf(
              log_string, sizeof(log_string),
              "==LaneLineUpdate==Obj_Label_Angled_Slot==LaneLinePtNum(%d)\n"
              "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
              "bLaneLineUpdateEndCarPosFlag(%d)==LaneLineABCType(%.2f,%.2f,%."
              "2f)==TempCarPos.CarAng(%.2f)\n"
              "==MainSlotBordLaneLine2(%.2f,%.2f)==MainSlotBordLaneLine1(%.2f,%"
              ".2f)==DebugLaneLine(%.2f)==DebugLaneLine2(%.2f)==Obj2Ang(%.2f)=="
              "LaneLineLengthTemp(%.2f)"
              "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2["
              "0](%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)",
              LaneLinePtNum, bEndCarPosOnTheLeftOfNewSysAngFlag,
              s_parking_out_state.flags.lane_line_update_end_car_pos, LaneLineABCType.A,
              LaneLineABCType.B, LaneLineABCType.C, TempCarPos.CarAng,
              MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y,
              MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y, DebugLaneLine,
              DebugLaneLine2, APAMap_GInfo.SlotPar.Obj2Ang, LaneLineLengthTemp,
              DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
              DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
              DataDebug2[1].y);
          TLOG_INFO << log_string;
        }
        return TRUE;
      }
      if (s_parking_out_state.eight_mode ==
          APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
        if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) {
        if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = -InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
        if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
        if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = -InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      }
      APAMap_GInfo.SlotPar.EndPos.Coordinate = MainSlotBordTemp5;
    }
  }
  {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==LaneLineUpdate==LaneLinePtNum(%d)=="
             "bInclineSlotChangeEndCarPosFlag(%d)\n"
             "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
             "bLaneLineUpdateEndCarPosFlag(%d)==MainSlotBordTemp5(%.2f,%.2f)=="
             "TempCarPos.CarAng(%.2f)\n"
             "==MainSlotBordLaneLine2(%.2f,%.2f)==MainSlotBordLaneLine1(%.2f,%."
             "2f)==DebugLaneLine(%.2f)==DebugLaneLine2(%.2f)\n"
             "==LaneLineUpdate==TempCarPos.CarAng(%.2f)==NSegNum(%d)=="
             "bFindLaneLineFlagByLine(%d)==bFindLaneLineFlagByPoints(%d)=="
             "LaneLineABCType.A(%.2f)"
             "==LaneLineABCType.B(%.2f)==LaneLineABCType.C(%.2f)=="
             "LaneLineLengthTemp(%.2f)==LaneLineLengthPre(%.2f)=="
             "LaneLineLength(%.2f)"
             "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2[0]"
             "(%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)",
             LaneLinePtNum, bInclineSlotChangeEndCarPosFlag,
             bEndCarPosOnTheLeftOfNewSysAngFlag, s_parking_out_state.flags.lane_line_update_end_car_pos,
             MainSlotBordTemp5.x, MainSlotBordTemp5.y, TempCarPos.CarAng,
             MainSlotBordLaneLine2.x, MainSlotBordLaneLine2.y,
             MainSlotBordLaneLine1.x, MainSlotBordLaneLine1.y, DebugLaneLine,
             DebugLaneLine2, TempCarPos.CarAng, NSegNum,
             bFindLaneLineFlagByLine, bFindLaneLineFlagByPoints,
             LaneLineABCType.A, LaneLineABCType.B, LaneLineABCType.C,
             LaneLineLengthTemp, LaneLineLengthPre, LaneLineLength,
             DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
             DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
             DataDebug2[1].y);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
#endif
#if 1
BOOLEAN APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() {
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_DISTANCE_TYPE i, j, m, n;
  APACoordinateDataCalFloatType temp;
  APACoordinateDataCalFloatType temp2;
  APA_ENUM_TYPE k;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType NSegment[127];
  APACoordinateDataCalFloatType NSegmentFilter[127];
  APA_ENUM_TYPE NSegNum;
  APA_ENUM_TYPE LeftDataNum;
  APA_ENUM_TYPE RightDataNum;
  APA_DISTANCE_TYPE RefercLinePtNum;
  APA_DISTANCE_TYPE LeftRefercLinePtNum;
  APA_DISTANCE_TYPE RightRefercLinePtNum;
  APA_ENUM_TYPE LocStyle;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  uint8_t_INF park_out_mode;
  APALineParameterABCType EndPosLine;
  APACarCoordinateDataCalFloatType TempCarPos;
  // APALineParameterABCType TempLine1, TempLine2;
  APACoordinateDataCalFloatType Obj2Pt;
  APACoordinateDataCalFloatType MainSlotBordTemp5;
  APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE InclineSlotOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE RefercLineLength;
  APA_DISTANCE_CAL_FLOAT_TYPE RefercLineLengthPre;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  BOOLEAN bEndCarPosOnTheLeftOfNewSysAngFlag;
  BOOLEAN bRefercLineUpdatePerpFlag;
  BOOLEAN bUpdataRefercLineFlag;
  BOOLEAN bInclineSlotChangeEndCarPosFlag;  // 斜列式车位更改终点位置标志位
  plf_RefercLineInfo* pRefercLineInfo;
  APA_DISTANCE_CAL_FLOAT_TYPE DebugRefercLine;
  APA_DISTANCE_CAL_FLOAT_TYPE DebugRefercLine2;
  APACoordinateDataCalFloatType DataDebug[127];
  APACoordinateDataCalFloatType DataDebug2[127];
  static APACoordinateDataCalFloatType MainSlotBordRefercLine1;
  static APACoordinateDataCalFloatType MainSlotBordRefercLine2;
  static BOOLEAN bFindRefercLineFlag = FALSE;  // 找到车位线标志位
  BOOLEAN bSearch;
  APA_DISTANCE_CAL_FLOAT_TYPE ValidDis;

  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    // return TRUE;
  }
#ifdef SUPPORT_PARKING_OUT_UWB
  if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
    return TRUE;
  }
#endif
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return TRUE;
  }
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    return TRUE;
  }
  if ((TRUE == s_parking_out_state.flags.reference_line_update_end_car_pos) || (TRUE == s_parking_out_state.flags.label_angled)) {
    return TRUE;
  }
  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    MainSlotBordRefercLine1.x = 0;
    MainSlotBordRefercLine1.y = 0;
    MainSlotBordRefercLine2.x = 0;
    MainSlotBordRefercLine2.y = 0;
  }
  pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.RefercLineInfo;
  MaxOffsetX = -2000;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  CurCarPos = APAMap_GInfo.CarPos;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }

  // Mainborder
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // LineYAngle = Angle;

  // Subborder
  MaxOffsetX = 3000;
  TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      CurCarPos.Coordinate, 0, Angle, Pto);
  TempDis = APAMap_ComCfg.HalfWidthOfCar;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x -= ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
    if (TempPt.x > -MaxOffsetX) {
      TempPt.x = -MaxOffsetX;
    }
  } else {
    TempPt.x += ((APA_DISTANCE_CAL_FLOAT_TYPE)TempDis + 500);
    if (TempPt.x < MaxOffsetX) {
      TempPt.x = MaxOffsetX;
    }
  }
  SubLinYStrPt.x = TempPt.x;
  SubLinYStrPt.y = 0;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = 1000;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);

  for (i = 0; i < 2; i++) {
    DataDebug[i].x = 0;
    DataDebug[i].y = 0;
    DataDebug2[i].x = 0;
    DataDebug2[i].y = 0;
  }
  i = 0;
  LeftDataNum = 0;
  RightDataNum = 0;
  RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
  LeftRefercLinePtNum = pRefercLineInfo->stLeftRefercLineInfo.RefercLineNum;
  RightRefercLinePtNum = pRefercLineInfo->stRightRefercLineInfo.RefercLineNum;
  DebugRefercLine = 0;
  DebugRefercLine2 = 0;
  RefercLineLength = 0;
  RefercLineLengthPre = 0;
  MainSlotBordTemp5.x = 0;
  MainSlotBordTemp5.y = 0;
  TempCarPos.CarAng = 0;
  NSegNum = 0;
  bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
  bRefercLineUpdatePerpFlag = FALSE;
  bInclineSlotChangeEndCarPosFlag = FALSE;
#if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
    RefercLinePtNum = 4;
    LeftRefercLinePtNum = 2;
    RightRefercLinePtNum = 2;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt1.fx = -100;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt1.fy = -7000;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt2.fx = 0;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[0].pt2.fy = -4500;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt1.fx = -100;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt1.fy = -4000;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt2.fx = 0;
    pRefercLineInfo->stLeftRefercLineInfo.stRefercLineParam[1].pt2.fy = -2500;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt1.fx = 100;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt1.fy = 100;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt2.fx = 0;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[0].pt2.fy = 2500;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt1.fx = -100;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt1.fy = 2600;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt2.fx = 0;
    pRefercLineInfo->stRightRefercLineInfo.stRefercLineParam[1].pt2.fy = 5000;
#endif
  while (i < RefercLinePtNum) {
    // zqf: get RefercLine data
    if (i < LeftRefercLinePtNum) {
      Data[LeftDataNum].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fx;
      Data[LeftDataNum].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fy;
      LeftDataNum++;
      Data[LeftDataNum].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fx;
      Data[LeftDataNum].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fy;
      LeftDataNum++;
    } else {
      if ((i - LeftRefercLinePtNum) < RightRefercLinePtNum) {
        if (LeftDataNum + RightDataNum > 127) {
          break;
        }
        Data[LeftDataNum + RightDataNum].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt1.fx;
        Data[LeftDataNum + RightDataNum].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt1.fy;
        RightDataNum++;
        Data[LeftDataNum + RightDataNum].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt2.fx;
        Data[LeftDataNum + RightDataNum].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt2.fy;
        RightDataNum++;
      }
    }
    i++;
  }

  bUpdataRefercLineFlag = FALSE;
  // 判断在锚点转换之后，且车辆已开出车位，则不再检测车位线
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if (CurCarCoordinateX > -2) {
        bUpdataRefercLineFlag = TRUE;
        DebugRefercLine += 1;
      }
    } else {
      if (CurCarCoordinateX > 0) {
        bUpdataRefercLineFlag = TRUE;
        DebugRefercLine += 1;
      }
    }
  }
  for (i = 0; i < 2; i++) {
    //----------------------------------
    // Get valid LeftRefercLine data;
    if (i == 0) {
      if (LeftRefercLinePtNum == 0) {
        LeftDataNum = 0;
      }
      NSegNum = 0;
      for (k = 0; k < LeftRefercLinePtNum; k++) {
        m = k * 2;
        TempPt = Data[m];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
        if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
          if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = Data[m];
            NSegNum++;
            NSegment[NSegNum] = Data[m + 1];
            NSegNum++;
            DebugRefercLine += 10;
          }
        }
      }
    } else {
      if (RightRefercLinePtNum == 0) {
        RightDataNum = 0;
      }
      NSegNum = 0;
      for (k = LeftRefercLinePtNum; k < RefercLinePtNum; k++) {
        m = k * 2;
        TempPt = Data[m];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
        if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
          if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = Data[m];
            NSegNum++;
            NSegment[NSegNum] = Data[m + 1];
            NSegNum++;
            DebugRefercLine += 100;
          }
        }
      }
    }
    // APA转锚点坐标系下
    for (m = 0; m < NSegNum; m++) {
      NSegment[m] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          NSegment[m], 0, Angle, Pto);
    }
    // 第二层数据过滤
    n = 0;
    for (j = 0; j < (NSegNum / 2); j++) {
      m = j * 2;
      if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
        ValidDis = -8000;
      } else {
        ValidDis = 0;
      }
      // 取锚点坐标系下y轴坐标小于0m的车位线归类为次车位线（SubRefercLine），大于0m的车位线归类为主车位线（MainRefercLine）；
      // 只对主车位线的数据做处理，次车位线的数据不参与处理，最后只平行主车位线。
      if ((NSegment[m].y < ValidDis) || (NSegment[m].y > 10000) ||
          (NSegment[m + 1].y < ValidDis) || (NSegment[m + 1].y > 10000)) {
      } else {
        NSegmentFilter[n] = NSegment[m];
        n++;
        NSegmentFilter[n] = NSegment[m + 1];
        n++;
        DebugRefercLine += 1000;
      }
    }
    NSegNum = n;

    if (NSegNum < 2) {
      NSegNum = 0;
    }
    for (m = 0; m < NSegNum; m++) {
      DataDebug[m] = NSegmentFilter[m];  // 锚点坐标系下
      DataDebug2[m] = AlgCom_PointPosWithAngAndCenterPt(
          NSegmentFilter[m], Angle,
          Pto);  // Debug2转到与锚点相同的坐标系下（可能APA坐标系也可能锚点坐标系）
    }
    // NSegmentFilter[0].x = NSegmentFilter[NSegNum-1].x + 500; // for test
    //----------------------------------
    // zqf:RefercLine update EndCarPos
    if ((FALSE == s_parking_out_state.flags.reference_line_update_end_car_pos) && (NSegNum >= 2) &&
        (FALSE == bUpdataRefercLineFlag)) {
      DebugRefercLine += 10000;
      m = 0;
      RefercLineLength = 0;
      bSearch = TRUE;
      // 首先搜索是否有大于1.5m的车位线
      while (bSearch) {
        temp = NSegmentFilter[m];
        temp2 = NSegmentFilter[m + 1];
        RefercLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
            temp.x, temp.y, temp2.x, temp2.y);
        if (RefercLineLength > 1500) {
          break;
        }
        if (m >= NSegNum - 2) {
          bSearch = FALSE;
          break;
        }
        m += 2;
      }
      if (TRUE == bSearch) {
        // 其次搜索距离锚点最近的车位线，且车位线长度大于1.5m
        for (n = m; n < NSegNum - 1; n++) {
          if (NSegmentFilter[n].y < temp.y) {
            RefercLineLength = (APA_DISTANCE_TYPE)AlgCom_GetTwoPointDisInt(
                NSegmentFilter[n].x, NSegmentFilter[n].y,
                NSegmentFilter[n + 1].x, NSegmentFilter[n + 1].y);
            if (RefercLineLength > 1500) {
              temp = NSegmentFilter[n];
              temp2 = NSegmentFilter[n + 1];
            }
          }
          n++;
        }
        if (temp.y > temp2.y) {
          MainSlotBordRefercLine1 = temp2;
          MainSlotBordRefercLine2 = temp;
        } else {
          MainSlotBordRefercLine1 = temp;
          MainSlotBordRefercLine2 = temp2;
        }
        bFindRefercLineFlag = TRUE;
        DebugRefercLine += 100000;
      }
    }
  }
  if ((FALSE == s_parking_out_state.flags.reference_line_update_end_car_pos) &&
      (TRUE == bUpdataRefercLineFlag) && (TRUE == bFindRefercLineFlag)) {
    DebugRefercLine2 += 1;
    EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(
        &MainSlotBordRefercLine1, &MainSlotBordRefercLine2);
    // EndPosLine = RefercLineABCType;
    if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
      DebugRefercLine2 += 10;
      return TRUE;
    }
    // zqf: update EndCarPos
    // TempCarPos.CarAng = APAMap_GInfo.SlotPar.Obj2Ang;
    // TempCarPos.Coordinate = APAMap_GInfo.SlotPar.EndPos.Coordinate;
    // TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    if (MATH_ATAN(EndPosLine.A) >= 0) {
      DebugRefercLine2 += 100;
      TempCarPos.CarAng =
          APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
    } else {
      DebugRefercLine2 += 1000;
      TempCarPos.CarAng =
          APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
    }
    if (EndPosLine.LineType != APALineIsIncline) {
      TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
    }
    // 判断车道线已更新，且更新的角度小于车位参考线准备更新的角度，则把车位参考线当成误判，直接return返回。
    if (TRUE == s_parking_out_state.flags.lane_line_update_end_car_pos) {
      if (MATH_FABS(APAMap_GInfo.SlotPar.EndPos.CarAng) <
          MATH_FABS(TempCarPos.CarAng)) {
        DebugRefercLine2 += 10000;
        return TRUE;
      }
    }
    // TempCarPos.Coordinate = MainSlotBordRefercLine1;
    // TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    // TempLine1 = RefercLineABCType;
    // AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2,
    // &MainSlotBordRefercLine1);
    if (APAMap_GInfo.SlotPar.EndPos.CarAng != TempCarPos.CarAng)
    // && (APAMap_GInfo.SlotPar.EndPos.Coordinate.x !=
    // MainSlotBordRefercLine1.x))
    {
      if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) > (M_PI / 16)) &&
          ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
           (5 * M_PI / 16)) &&
          (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
        DebugRefercLine2 += 100000;
        bEndCarPosOnTheLeftOfNewSysAngFlag = TRUE;
        s_parking_out_state.flags.reference_line_update_end_car_pos = TRUE;
      } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
                  -(M_PI / 16)) &&
                 ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
                  -(5 * M_PI / 16)) &&
                 (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
        DebugRefercLine2 += 1000000;
        bEndCarPosOnTheLeftOfNewSysAngFlag = FALSE;
        s_parking_out_state.flags.reference_line_update_end_car_pos = TRUE;
      } else if (((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) >
                  -(M_PI / 16)) &&
                 ((TempCarPos.CarAng - APAMap_GInfo.NewCordSysAng) <
                  (M_PI / 16))) {
        DebugRefercLine2 += 10000000;
        s_parking_out_state.flags.reference_line_update_end_car_pos = TRUE;
        bRefercLineUpdatePerpFlag = TRUE;
      }
    }

    if (TRUE == s_parking_out_state.flags.reference_line_update_end_car_pos) {
      APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
      APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
      InclineSlotOffsetX = 3000;
      InclineSlotOffsetY = 2000;
      MainSlotBordTemp5.x = 1500;
      MainSlotBordTemp5.y = 0;
      if (slot_data_at_right_side == TRUE) {
        MainSlotBordTemp5.x = -MainSlotBordTemp5.x;
      }
      if ((MATH_FABS(MATH_FABS(APAMap_GInfo.SlotPar.Obj2Ang -
                               APAMap_GInfo.NewCordSysAng) -
                     M_PI_2) > (M_PI / 8)) ||
          (TRUE == bRefercLineUpdatePerpFlag)) {
        {
          char log_string[1024];
          snprintf(
              log_string, sizeof(log_string),
              "==RefercLineUpdate==Obj_Label_Angled_Slot==RefercLinePtNum(%d)\n"
              "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
              "bRefercLineUpdateEndCarPosFlag(%d)==TempCarPos.CarAng(%.2f)\n"
              "==MainSlotBordRefercLine2(%.2f,%.2f)==MainSlotBordRefercLine1(%."
              "2f,%.2f)==DebugRefercLine(%.2f)==DebugRefercLine2(%.2f)=="
              "Obj2Ang(%.2f)\n"
              "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2["
              "0](%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)==bCarryOutSlot(%d)",
              RefercLinePtNum, bEndCarPosOnTheLeftOfNewSysAngFlag,
              s_parking_out_state.flags.reference_line_update_end_car_pos, TempCarPos.CarAng,
              MainSlotBordRefercLine2.x, MainSlotBordRefercLine2.y,
              MainSlotBordRefercLine1.x, MainSlotBordRefercLine1.y,
              DebugRefercLine, DebugRefercLine2, APAMap_GInfo.SlotPar.Obj2Ang,
              DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
              DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x,
              DataDebug2[1].y, s_parking_out_state.flags.carry_out_slot);
          TLOG_INFO << log_string;
        }
        return TRUE;
      }
      if (s_parking_out_state.eight_mode ==
          APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) {
        if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) {
        if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = -InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) {
        if (FALSE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      } else if (s_parking_out_state.eight_mode ==
                 APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) {
        if (TRUE == bEndCarPosOnTheLeftOfNewSysAngFlag) {
          MainSlotBordTemp5.x = -InclineSlotOffsetX;
          MainSlotBordTemp5.y = -InclineSlotOffsetY;
          bInclineSlotChangeEndCarPosFlag = TRUE;
        }
      }
      APAMap_GInfo.SlotPar.EndPos.Coordinate = MainSlotBordTemp5;
    }
  }
  {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==RefercLineUpdate==RefercLinePtNum(%d)=="
             "bInclineSlotChangeEndCarPosFlag(%d)\n"
             "==bEndCarPosOnTheLeftOfNewSysAngFlag(%d)=="
             "bRefercLineUpdateEndCarPosFlag(%d)==MainSlotBordTemp5(%.2f,%.2f)="
             "=EndPos.CarAng(%.2f)\n"
             "==MainSlotBordRefercLine2(%.2f,%.2f)==MainSlotBordRefercLine1(%."
             "2f,%.2f)"
             "==DebugRefercLine(%.2f)==DebugRefercLine2(%.2f)==TempCarPos."
             "CarAng(%.2f)==NSegNum(%d)\n"
             "==RefercLineLengthPre(%.2f)==RefercLineLength(%.2f)"
             "==DataDebug[0](%.2f,%.2f)==DataDebug[1](%.2f,%.2f)==DataDebug2[0]"
             "(%.2f,%.2f)==DataDebug2[1](%.2f,%.2f)==bCarryOutSlot(%d)",
             RefercLinePtNum, bInclineSlotChangeEndCarPosFlag,
             bEndCarPosOnTheLeftOfNewSysAngFlag, s_parking_out_state.flags.reference_line_update_end_car_pos,
             MainSlotBordTemp5.x, MainSlotBordTemp5.y,
             APAMap_GInfo.SlotPar.EndPos.CarAng, MainSlotBordRefercLine2.x,
             MainSlotBordRefercLine2.y, MainSlotBordRefercLine1.x,
             MainSlotBordRefercLine1.y, DebugRefercLine, DebugRefercLine2,
             TempCarPos.CarAng, NSegNum, RefercLineLengthPre, RefercLineLength,
             DataDebug[0].x, DataDebug[0].y, DataDebug[1].x, DataDebug[1].y,
             DataDebug2[0].x, DataDebug2[0].y, DataDebug2[1].x, DataDebug2[1].y,
             s_parking_out_state.flags.carry_out_slot);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
#else
BOOLEAN APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() {
  APA_ENUM_TYPE park_side;
  APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_ENUM_TYPE SlotStrIndex;
  // APA_ENUM_TYPE SlotEndIndex;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_ENUM_TYPE Index;
  APA_DISTANCE_TYPE i;
  APA_ENUM_TYPE k;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  // APA_ENUM_TYPE OffsetIndex2,OffsetIndex1;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType pData1[127];
  APACoordinateDataCalFloatType pData2[127];
  APACoordinateDataCalFloatType pData3[127];
  APACoordinateDataCalFloatType pData4[127];
  APACoordinateDataCalFloatType NSegment[127];
  uint8_t_INF pPtStyle[127];
  uint8_t_INF NewProperty1[127];
  uint8_t_INF NewProperty2[127];
  // uint8_t_INF NewProperty3[127];
  uint8_t_INF NewProperty4[127];
  uint8_t_INF NSegProperty[127];
  APA_ENUM_TYPE NSegNum;
  uint16_t_INF DataNum;
  APA_ENUM_TYPE Data1Num;
  APA_ENUM_TYPE Data2Num;
  APA_ENUM_TYPE Data3Num;
  APA_ENUM_TYPE Data4Num;
  plf_RefercLineInfo* pRefercLineInfo;
  APA_DISTANCE_TYPE RefercLinePtNum;
  APA_DISTANCE_TYPE LeftRefercLinePtNum;
  APA_DISTANCE_TYPE RightRefercLinePtNum;
  APA_ENUM_TYPE LocStyle;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
  APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
  // APA_DISTANCE_CAL_FLOAT_TYPE LineYAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
  APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
  APACarCoordinateDataCalFloatType TempCarPos;
  APALineParameterABCType TempLine1;
  APALineParameterABCType TempLine2;
  BOOLEAN bCheckSubLane;
  APACoordinateDataCalFloatType TempPt1, TempPt2;
  APACoordinateDataCalFloatType pDataBk[127];
  APA_ENUM_TYPE DataNumBk;
  BOOLEAN bFusvalid;
#if 1
  APALineParameterABCType EndPosLine;
  APACoordinateDataCalFloatType MainSlotBordTemp1;
  APACoordinateDataCalFloatType MainSlotBordTemp2;
#endif
  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    return TRUE;
  }
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return TRUE;
  }
  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    return TRUE;
  }
  pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData.RefercLineInfo;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  APAMap_CheckIfIgnoreFSDPtAtMainBoundary();
  APAMap_CheckIfIgnoreFSDPtAtSubBoundary();
  MaxOffsetX = 600;
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  // zqf:PARALLEL_SIDE
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (park_side == APA_CAR_PARK_AT_RIGHT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  } else {
    if (park_side == APA_CAR_PARK_AT_LEFT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  }
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;

  OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
  CurCarPos = APAMap_GInfo.CarPos;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&TempAng);
  // CurCarPos.Coordinate =
  // AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CurCarPos.Coordinate,0,Angle,Pto);
  // CurCarPos.CarAng -= Angle;

  // SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
  SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
  Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
  Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
  // OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
  // OffsetIndex2 = Obj2PtIndex - SlotEndIndex;

  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // LineYAngle = Angle;

  // obj 1 border line
  MainLinXStrPt1 = OrgObj1Pt;
  MainLinXEndPt1 = MainLinXStrPt1;
  LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
  MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
  Data1Num = 0;
  for (Index = 0; Index <= Obj1PtIndex; Index++) {
    pData1[Data1Num] = pMapMainSlotBord->Points[Index];
    NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
    Data1Num++;
  }

  // obj2 borderline;
  MainLinXStrPt2 = OrgObj2Pt;
  MainLinXEndPt2 = MainLinXStrPt2;
  LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
  MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
  Data2Num = 0;
  for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
    pData2[Data2Num] = pMapMainSlotBord->Points[Index];
    NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
    Data2Num++;
  }
  // data pt in slot;
  Data3Num = 0;
  for (Index = SlotStrIndex; Index < Obj2PtIndex; Index++) {
    pData3[Data3Num] = pMapMainSlotBord->Points[Index];
    // NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
    Data3Num++;
  }
  // Fus  Subborder
  MaxOffsetX = 3000;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt.x = TempPt.x;
  SubLinYStrPt.y = 0;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = 1000;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
  // Fus  Subborder
  MaxOffsetX = 3000;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt1.x = TempPt.x;
  SubLinYStrPt1.y = 0;
  SubLinYEndPt1.x = SubLinYStrPt1.x;
  SubLinYEndPt1.y = 1000;
  SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
  SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);

  LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&LineXAngle);
  TempCarPos.CarAng = LineXAngle;

  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
  TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
  if (TempLine1.C < TempLine2.C) {
    TempLine1.C = TempLine2.C;
  }

  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
  TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
  if (TempLine1.C < TempLine2.C) {
    TempLine1.C = TempLine2.C;
  }

  TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(1000, &TempLine1);

  TempCarPos.CarAng = Angle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
  SubLinXEndPt = SubLinXStrPt;
  SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
  SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);

  Data4Num = 0;
  for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
    pData4[Data4Num] = pMapSubSlotBord->Points[Index];
    NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
    Data4Num++;
  }

  NSegNum = 0;
  i = 0;
  DataNum = 0;
  bCheckSubLane = FALSE;
  RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
  LeftRefercLinePtNum = pRefercLineInfo->stLeftRefercLineInfo.RefercLineNum;
  RightRefercLinePtNum = pRefercLineInfo->stRightRefercLineInfo.RefercLineNum;
  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
  while (i < RefercLinePtNum) {
    // zqf: get RefercLine data
    if (i < LeftRefercLinePtNum) {
      Data[DataNum].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fx;
      Data[DataNum].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fy;
      if (i > 0) {
        if (slot_data_at_right_side == TRUE) {
          if (Data[DataNum].y < Data[DataNum - 1].y) {
            TempPt = Data[DataNum];
            Data[DataNum] = Data[DataNum - 1];
            Data[DataNum - 1] = TempPt;
          }
        } else {
          if (Data[DataNum].y > Data[DataNum - 1].y) {
            TempPt = Data[DataNum];
            Data[DataNum] = Data[DataNum - 1];
            Data[DataNum - 1] = TempPt;
          }
        }
      }
      DataNum++;
      Data[DataNum].x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fx;
      Data[DataNum].y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fy;
      DataNum++;
    } else {
      if ((i - LeftRefercLinePtNum) < RightRefercLinePtNum) {
        Data[DataNum].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt1.fx;
        Data[DataNum].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt1.fy;
        if (i - LeftRefercLinePtNum > 0) {
          if (slot_data_at_right_side == TRUE) {
            if (Data[DataNum].y < Data[DataNum - 1].y) {
              TempPt = Data[DataNum];
              Data[DataNum] = Data[DataNum - 1];
              Data[DataNum - 1] = TempPt;
            }
          } else {
            if (Data[DataNum].y > Data[DataNum - 1].y) {
              TempPt = Data[DataNum];
              Data[DataNum] = Data[DataNum - 1];
              Data[DataNum - 1] = TempPt;
            }
          }
        }
        DataNum++;
        Data[DataNum].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt2.fx;
        Data[DataNum].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stRightRefercLineInfo
                .stRefercLineParam[i - LeftRefercLinePtNum]
                .pt2.fy;
        DataNum++;
      }
    }
    i++;
  }
  //----------------------------------
  // Get valid fsd data for fus obj1bordline;
  NSegNum = 0;
  for (k = 0; k < DataNum; k++) {
    TempPt = Data[k];
    LocStyle = AlgCom_GetPointLocationAccordGivenVector(
        &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
    if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
        ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
      if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
        NSegment[NSegNum] = TempPt;
        NSegNum++;
      }
    }
  }
  if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                              pRectPt, pRectLine)) {
    NSegNum = 0;
  }
  for (k = 0; k < NSegNum; k++) {
    NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
  }
  APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                         Pto, Angle);
  if (TRUE == APAMap_FusTwoLineSegments(slot_data_at_right_side, TempAng,
                                        &pData1[0], Data1Num, &NewProperty1[0],
                                        &NSegment[0], NSegNum, &NSegProperty[0],
                                        &pData1[0], &Data1Num, &pPtStyle[0])) {
    // updata obj1 bordline;

    for (k = 0; k < Data1Num; k++) {
      NewProperty1[k] = pPtStyle[k];
    }
    if (Data1Num > 2) {
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData1[0], 0, Angle, Pto);
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData1[1], 0, Angle, Pto);
      if ((TempPt1.y >= TempPt2.y) &&
          (((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
           ((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
        TempPt1.x = TempPt2.x;
        pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
        for (k = 1; k < Data1Num - 1; k++) {
          NewProperty1[k] = NewProperty1[k + 1];
          pData1[k] = pData1[k + 1];
        }
        Data1Num--;
      }
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==RefercLineFusionObj1Success==");
    TLOG_INFO << log_string;
  }
  //----------------------------------
  // Get valid fsd data for fus obj2bordline;
  NSegNum = 0;
  for (k = 0; k < DataNum; k++) {
    TempPt = Data[k];
    LocStyle = AlgCom_GetPointLocationAccordGivenVector(
        &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
    if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
        ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
      if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
        NSegment[NSegNum] = TempPt;
        NSegNum++;
      }
    }
  }
  if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                              pRectPt, pRectLine)) {
    NSegNum = 0;
  }
  for (k = 0; k < NSegNum; k++) {
    NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
  }
  APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                         Pto, Angle);
  if (TRUE == APAMap_FusTwoLineSegments(slot_data_at_right_side, TempAng,
                                        &pData2[0], Data2Num, &NewProperty2[0],
                                        &NSegment[0], NSegNum, &NSegProperty[0],
                                        &pData2[0], &Data2Num, &pPtStyle[0])) {
    // updata obj2 bordline;
    for (k = 0; k < Data2Num; k++) {
      NewProperty2[k] = pPtStyle[k];
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==RefercLineFusionObj2Success==");
    TLOG_INFO << log_string;
  }
  //----------------------------------
  // Get valid fsd data for fus Subbordline;
  NSegNum = 0;
  for (k = 0; k < DataNum; k++) {
    TempPt = Data[k];
    LocStyle = AlgCom_GetPointLocationAccordGivenVector(
        &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
    if (LocStyle != 0) {
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
      if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
        NSegment[NSegNum] = TempPt;
        NSegNum++;
      }
    } else {
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
      if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
        NSegment[NSegNum] = TempPt;
        NSegNum++;
      }
    }
  }
  if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                              pRectPt, pRectLine)) {
    NSegNum = 0;
  }
  for (k = 0; k < NSegNum; k++) {
    NSegProperty[k] = APA_MAP_PT_PROPERTY_REFERCLINE_STR;
  }
  if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
    for (k = 0; k < Data4Num; k++) {
      pDataBk[k] = pData4[k];
    }
    DataNumBk = Data4Num;
  }
  bFusvalid = FALSE;
  APAMap_ReOderSegmentPt(TRUE, !slot_data_at_right_side, &NSegment[0], &NSegNum,
                         Pto, Angle);
  if (TRUE == APAMap_FusTwoLineSegments(!slot_data_at_right_side, TempAng,
                                        &pData4[0], Data4Num, &NewProperty4[0],
                                        &NSegment[0], NSegNum, &NSegProperty[0],
                                        &pData4[0], &Data4Num, &pPtStyle[0])) {
    if (bCheckSubLane == TRUE) {
      if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &pData4[0], Data4Num,
                                                  pRectPt, pRectLine)) {
        for (k = 0; k < DataNumBk; k++) {
          pData4[k] = pDataBk[k];
        }
        Data4Num = DataNumBk;
      } else {
        bFusvalid = TRUE;
      }
    } else {
      bFusvalid = TRUE;
    }
  }
  if (bFusvalid == TRUE) {
    // updata sublane;
    for (k = 0; k < Data4Num; k++) {
      NewProperty4[k] = pPtStyle[k];
    }
    if (Data4Num > 2) {
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData4[0], 0, Angle, Pto);
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData4[1], 0, Angle, Pto);
      if ((TempPt1.y >= TempPt2.y) &&
          (((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
           ((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
        TempPt1.x = TempPt2.x;
        pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
        for (k = 1; k < Data4Num - 1; k++) {
          NewProperty4[k] = NewProperty4[k + 1];
          pData4[k] = pData4[k + 1];
        }
        Data4Num--;
      }
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==RefercLineFusionSubLaneSuccess==");
    TLOG_INFO << log_string;
  }

  DataNum = Data1Num + Data3Num + Data2Num;
#if 0
  if(DataNum <= BOUNDARY_PT_MAX_NUM)
  {
    for (Index = 0; Index < DataNum; Index++) {
      if (Index < Data1Num) {
        pMapMainSlotBord->Points[Index] = pData1[Index];
        pMapMainSlotBord->Property[Index] = NewProperty1[Index];
      } else if (Index < Data1Num + Data3Num) {
        pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
        pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];

      } else {
        pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
        pMapMainSlotBord->Property[Index] =
            NewProperty2[Index - Data1Num - Data3Num];
      }
    }
    pMapMainSlotBord->PtNum = DataNum;
    APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
    APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
    APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
    APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
  }else
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
        "==FSDFusionMainSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  if(Data4Num <= BOUNDARY_PT_MAX_NUM)
  {
    for (Index = 0; Index < Data4Num; Index++) {
      pMapSubSlotBord->Points[Index] = pData4[Index];
      pMapSubSlotBord->Property[Index] = NewProperty4[Index];
    }
    pMapSubSlotBord->PtNum = Data4Num;
  }else
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
        "==FSDFusionSubSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
#endif
  {
    char log_string[512];
    snprintf(
        log_string, sizeof(log_string),
        "==FusBordByFSD==SlotIndex(%d,%d,%d,%d))",
        APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
        APAMap_GInfo.SlotPar.SlotEndIndex, APAMap_GInfo.SlotPar.Obj2PtIndex);
    TLOG_INFO << log_string;
  }
#if 1
  // zqf:for diagonal parking out slot
  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
      (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
      (FALSE == s_parking_out_state.flags.reference_line_update_end_car_pos) &&
      (FALSE == s_parking_out_state.flags.lane_line_update_end_car_pos)) {
    MainSlotBordTemp1 = pData2[0];
    MainSlotBordTemp2 = pData2[DataNum - 1 - Data1Num - Data3Num];
    // MainSlotBordTemp2.x += 500; // for test
    EndPosLine = APAMap_ParkingOutLineParABCByMainSlotBord(&MainSlotBordTemp1,
                                                           &MainSlotBordTemp2);
    if (MATH_FABS(MATH_ATAN(EndPosLine.A)) > (M_PI / 2)) {
      return TRUE;
    }
    if (EndPosLine.LineType == APALineIsIncline) {
      if (MATH_ATAN(EndPosLine.A) >= 0) {
        TempCarPos.CarAng =
            APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) - (M_PI / 2));
      } else {
        TempCarPos.CarAng =
            APAMap_GInfo.NewCordSysAng + (MATH_ATAN(EndPosLine.A) + (M_PI / 2));
      }
      APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
      APAMap_GInfo.SlotPar.EndPosLine = EndPosLine;
      s_parking_out_state.flags.reference_line_update_end_car_pos = TRUE;
    }
  }
#endif
  return TRUE;
}
#endif

void APAMap_ParkingOutUpDataMapInfo() {
  uint8_t_INF LBoundaryPtNum, RBoundaryPtNum;
  APA_ENUM_TYPE mode;
  BOOLEAN bUpdataCalBoundaryFlag;
#ifdef APAMAP_PARKOUT_FUS_SDG
  APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus();
#endif
#ifdef APAMAP_PARKOUT_FUS_PDC
  APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus();
#endif
  LBoundaryPtNum = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  RBoundaryPtNum = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  BoudaryNum[4][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[4][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  APAMap_ParkingOutUpDataMapInfoBySlotCorInfo();
  BoudaryNum[5][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[5][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  bUpdataCalBoundaryFlag = TRUE;
  APAMap_ParkingOutSideSlotInfo(&bUpdataCalBoundaryFlag);
  if (TRUE == bUpdataCalBoundaryFlag) {
    APAMap_ParkingOutCalBoundaryByParkOutInfo();
    BoudaryNum2[0][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[0][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }
  APAMap_ParkingOutFusBoundaryByFSDMapInfo();
  BoudaryNum[6][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[6][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point) {
    APAMap_ParkingOutFusBoundaryByLaneLineMapInfo();
    BoudaryNum2[2][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[2][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;

    APAMap_ParkingOutFusBoundaryByRefercLineMapInfo();
    BoudaryNum2[1][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
    BoudaryNum2[1][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  }
  APAMap_FusBoundaryByODMapInfo();
  BoudaryNum[7][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[7][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  mode = 0;
  if (LBoundaryPtNum == APAMap_GInfo.OutLine.LeftBoundary.PtNum) {
    mode |= 0x02;
  }
  if (RBoundaryPtNum == APAMap_GInfo.OutLine.RightBoundary.PtNum) {
    mode |= 0x01;
  }
#ifdef APAMAP_PARKOUT_FUS_SDG
  APAMap_ParkingOutUpDataMapBoundaryBySDGInfo();
  APAMap_ParkingOutDeleteMainSlotBord();
#endif
#ifdef APAMAP_PARKOUT_FUS_PDC
  APAMap_ParkingOutUpDataMapBoundaryByPDCInfo();
  APAMap_ParkingOutDeleteMainSlotBord();
#endif
  APAMap_SmoothMapBoundary(mode);
  BoudaryNum[8][0] = APAMap_GInfo.OutLine.LeftBoundary.PtNum;
  BoudaryNum[8][1] = APAMap_GInfo.OutLine.RightBoundary.PtNum;
  APAMap_ParkingOutEndCarPosUpdata();
  return;
}

void APAMap_ParkingOutCalSlotSlotAlignInfo() {
  APACoordinateDataCalFloatType Obj2PtTemp, Obj1PtTemp;

  Obj2PtTemp = APAMap_GInfo.SlotPar.SlotBordPt[0];
  Obj1PtTemp = APAMap_GInfo.SlotPar.SlotBordPt[1];
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point) {
    Obj1PtTemp.x = Obj2PtTemp.x - (APAMap_GInfo.SlotPar.SlotBordPt[3].x -
                                   APAMap_GInfo.SlotPar.SlotBordPt[2].x);
  }
  APAMap_GInfo.SlotPar.SlotBordPt[1] = Obj1PtTemp;
}

#ifdef SUPPORT_PARKING_OUT_UWB
APACoordinateDataCalFloatType APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
    uint8_t_INF park_out_mode, APACoordinateDataCalFloatType OrgPt,
    APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, APACoordinateDataCalFloatType Obj2Pt,
    BOOLEAN bSeizeEndCarPosFlag) {
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE StraightOutDefaultEndPosOffsetX;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt2, TempPt3;
  APACarCoordinateDataCalFloatType TempCarPos;
  BOOLEAN bObj2Exist;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosX;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosY;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
  APALineParameterABCType TempLine;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetY;
  // zqf:according to RemoteControlPos set EndCarPos, New Coordinate
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
  APA_DISTANCE_CAL_FLOAT_TYPE RemoContPosToEndCarPosDisX;
  APA_DISTANCE_CAL_FLOAT_TYPE RemoContPosToEndCarPosDisY;
  APA_DISTANCE_CAL_FLOAT_TYPE LenBetweenFBumperAndRemoContPos;
  APACoordinateDataCalFloatType RemoContPosTemp;
  stCor2d_cm_s16_t RemoContPos;
  APA_ENUM_TYPE park_side;
  APACoordinateDataCalFloatType EndCarPos;
  APACoordinateDataCalFloatType EndCarPosTemp;
  APA_DISTANCE_CAL_FLOAT_TYPE EndPosCarAng;
  static BOOLEAN bUWBPosUpdataFlag = FALSE;  // 接收到UWB信号标志位

  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  EndCarPosTemp = APAMap_GInfo.SlotPar.EndPos.Coordinate;
  RemoContPos = APAMap_GInputData.ParkReqPar.Parkout_UWBPos;  // 车身坐标系下
  CarWidth = APAMap_ComCfg.WidthOfCar;                        // mm
  CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;         // mm, 3000
  SafeDisCal = APAMap_ComCfg.ObjInSlotMinSafeDis[1];  // 250mm, 0 paralIn;
  LenBetweenFBumperAndRemoContPos = 1000;             // mm
  RemoContPosToEndCarPosDisX = CarWidth / 2 + SafeDisCal;
  RemoContPosToEndCarPosDisY = CarLFCal - LenBetweenFBumperAndRemoContPos;
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  s_parking_out_state.eight_mode = AlgCom_GetParkOutEightMode(park_out_mode, park_side);

#if 0
  if ((FALSE == bUWBPosUpdataFlag)
    && (RemoContPos.x != NO_OBJ_DISTANCE))
  {
      RemoContPosTemp.x = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.x;//车身坐标系下
      RemoContPosTemp.y = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.y;
      if ((s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)
          || (s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT)
          || (s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT)
          || (s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT))
      {
        EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisX;
        EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisY;
      }
      else if ((s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT)
              || (s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT))
      {
        EndCarPos.x = RemoContPosTemp.x - RemoContPosToEndCarPosDisY;
        EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisX;
      }
      else if ((s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT)
              || (s_parking_out_state.eight_mode == APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT))
      {
        EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisY;
        EndCarPos.y = RemoContPosTemp.y + RemoContPosToEndCarPosDisX;
      }
      else{}
      EndCarPos = AlgCom_PointPosWithAngAndCenterPt(EndCarPos, CurCarPos.CarAng, CurCarPos.Coordinate);//车身转APA坐标系下
      EndCarPosTemp = EndCarPos;
      bUWBPosUpdataFlag = TRUE;
  }
#else
  if ((FALSE == bUWBPosUpdataFlag) && (RemoContPos.x != NO_OBJ_DISTANCE)) {
    RemoContPosTemp.x =
        (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.x;  // APA坐标系下
    RemoContPosTemp.y = (APA_DISTANCE_CAL_FLOAT_TYPE)RemoContPos.y;
    RemoContPosTemp = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        RemoContPosTemp, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
    if (s_parking_out_state.eight_mode ==
        APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
      EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisY;
      EndCarPos.y = RemoContPosTemp.y + RemoContPosToEndCarPosDisX;
    } else if (s_parking_out_state.eight_mode ==
               APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
      EndCarPos.x = RemoContPosTemp.x - RemoContPosToEndCarPosDisY;
      EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisX;
    } else if ((s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_RIGHT) ||
               (s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PERP_LEFT) ||
               (s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_LEFT) ||
               (s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_REAR_PERP_RIGHT) ||
               (s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_LEFT) ||
               (s_parking_out_state.eight_mode ==
                APA_PARKPROC_EIGHT_PARKING_OUT_MODE_HEAD_PARALLEL_RIGHT)) {
      EndCarPos.x = RemoContPosTemp.x + RemoContPosToEndCarPosDisX;
      EndCarPos.y = RemoContPosTemp.y - RemoContPosToEndCarPosDisY;
    } else {
    }
    EndCarPos = AlgCom_PointPosWithAngAndCenterPt(EndCarPos, OrgAng,
                                                  OrgPt);  // 锚点转APA坐标系下
    EndCarPosTemp = EndCarPos;
    bUWBPosUpdataFlag = TRUE;
  }
#endif
  if (FALSE == bUWBPosUpdataFlag) {
    TempPt3.x = 0xff;
    APAMAP_Setfailcause(101);
    return TempPt3;
  }
  StraightOutDefaultEndPosOffsetX = 500;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
  UpdateCntOffsetX = 0;
  UpdateCntOffsetY = 0;
  UpdateCntGoStraightOffsetX = 0;
  UpdateCntGoStraightOffsetY = 0;
  fDis1 = 0;
  TempPt2.x = 0;
  TempPt2.y = 0;
  if (TRUE == bSeizeEndCarPosFlag) {
    UpdateCntOffsetY = 100;
    UpdateCntOffsetX = 150;
    UpdateCntGoStraightOffsetX = kParkOutInfoDefaultOffsetXMm;
    if (TRUE == s_parking_out_state.flags.fsd_from_main_and_sub_slot_border)  // 入侵的边界点来自双边界
    {
      UpdateCntOffsetY = 500;
      if (TRUE == s_parking_out_state.flags.fsd_from_main_slot_border)  // 入侵的边界点来自主边界
      {
        UpdateCntGoStraightOffsetY = 100;
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE))  // fsd in the left of EndCarPos
            ||
            ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)))  // fsd in the right of EndCarPos
        {
          UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
        }
      }
    } else  // 入侵来自单边边界
    {
      if (TRUE == s_parking_out_state.flags.fsd_from_sub_slot_border)  // 入侵的边界点来自子边界
      {
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        UpdateCntOffsetX = -UpdateCntOffsetX;  // 往主边界靠拢
      }
      if (TRUE == s_parking_out_state.flags.fsd_from_main_slot_border)  // 入侵的边界点来自主边界
      {
        UpdateCntGoStraightOffsetX = -UpdateCntGoStraightOffsetX;
        UpdateCntGoStraightOffsetY = 100;
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE))  // fsd in the left of EndCarPos
            ||
            ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)))  // fsd in the right of EndCarPos
        {
          UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
        }
      }
    }
  }

  if (TRUE == bUWBPosUpdataFlag) {
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        EndCarPosTemp, 0, OrgAng, OrgPt);           // APA转锚点坐标系下
    DefaultEndPosY = TempPt2.y - UpdateCntOffsetY;  // 锚点坐标系下
    DefaultEndPosX = -(MATH_ABS(TempPt2.x)) - UpdateCntOffsetX;
  } else {
    DefaultEndPosY = 2000 - UpdateCntOffsetY;
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      DefaultEndPosX = -(APAMap_ComCfg.HalfWidthOfCar + 950) - UpdateCntOffsetX;
    } else {
      if (TRUE == s_parking_out_state.flags.label_angled) {
        if (TRUE == s_parking_out_state.flags.obj_label_ladder) {
          DefaultEndPosX =
              -(APAMap_ComCfg.HalfWidthOfCar + 2000) - UpdateCntOffsetX;
        } else {
          DefaultEndPosX =
              -(APAMap_ComCfg.HalfWidthOfCar + 1100) - UpdateCntOffsetX;
        }
        DefaultEndPosY = 5000 - UpdateCntOffsetY;
      } else {
        DefaultEndPosX =
            -(APAMap_ComCfg.HalfWidthOfCar + 3100) - UpdateCntOffsetX;
        DefaultEndPosY = 4500 - UpdateCntOffsetY;
      }
    }
  }

  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    TempAng = CurCarPos.CarAng - OrgAng;
    AlgCom_AngNormalized(&TempAng);
    if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
      if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
        fDis1 = -(StraightOutDefaultEndPosOffsetX +
                  APAMap_ComCfg.LenBetweenRAxisAndRBumper -
                  (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
      } else {
        fDis1 = -(StraightOutDefaultEndPosOffsetX +
                  APAMap_ComCfg.LenBetweenRAxisAndFBumper -
                  (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
      }
    } else {
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          EndCarPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
      fDis1 = (TempPt2.x +
               (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
    }
    EndPosCarAng = APAMap_GInfo.SlotPar.Obj2Ang;
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
      EndPosCarAng += M_PI;
    }
    AlgCom_AngNormalized(&EndPosCarAng);

    TempAng1 = EndPosCarAng - OrgAng;
    AlgCom_AngNormalized(&TempAng1);
    TempCarPos.CarAng = TempAng1;
    TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        CurCarPos.Coordinate, 0, OrgAng, OrgPt);
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    if (TempLine.LineType == APALineIsVertical) {
      TempPt3.x = 0xff;
      APAMAP_Setfailcause(101);
      return TempPt3;
    } else {
      TempPt3.x = fDis1;
      TempPt3.y = TempLine.A * TempPt3.x + TempLine.C -
                  (UpdateCntGoStraightOffsetY * MATH_SIN(MATH_FABS(TempAng)));
#if 1
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==APAMap_ParkingOutSetEndCarPosInOldCorSys==2==fDis1(%.2f)=="
                 "MATH_SIN(MATH_FABS(TempAng))(%.2f)"
                 "==MATH_SIN(MATH_FABS(TempAng1))(%.2f)==TempPt3(%.2f,%.2f)=="
                 "TempLine.A,C(%.2f,%.2f)"
                 "==UpdateCntGoStraightOffsetX(%.2f)=="
                 "UpdateCntGoStraightOffsetY(%.2f)",
                 fDis1, MATH_SIN(MATH_FABS(TempAng)),
                 MATH_SIN(MATH_FABS(TempAng1)), TempPt3.x, TempPt3.y,
                 TempLine.A, TempLine.C, UpdateCntGoStraightOffsetX,
                 UpdateCntGoStraightOffsetY);
        TLOG_INFO << log_string;
      }
#endif
      TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);
      if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
        APAMap_GInfo.SlotPar.EndPos.CarAng = CurCarPos.CarAng;
      } else {
        APAMap_GInfo.SlotPar.EndPos.CarAng = EndPosCarAng;
      }
    }
  } else {
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        Obj2Pt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
    // TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
    // OrgAng, OrgPt);

    if ((bObj2Exist == FALSE) || (TRUE == s_parking_out_state.flags.after_new_anchor_point) ||
        (TRUE == bUWBPosUpdataFlag)) {
      TempPt2.x = 0;
    }
    if (slot_data_at_right_side == FALSE) {
      DefaultEndPosX = -DefaultEndPosX + TempPt2.x;
    } else {
      DefaultEndPosX = DefaultEndPosX + TempPt2.x;
    }
    TempPt3.y = DefaultEndPosY;  // 锚点坐标系下
    TempPt3.x = DefaultEndPosX;
    TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng,
                                                OrgPt);  // 锚点转APA坐标系下

    APAMap_GInfo.SlotPar.EndPos.CarAng = OrgAng;
  }
  if (FALSE == bSeizeEndCarPosFlag) {
    TempPt3 = EndCarPosTemp;
  }
  return TempPt3;
}
#endif

APACoordinateDataCalFloatType APAMap_ParkingOutSetEndCarPosInOldCorSys(
    uint8_t_INF park_out_mode, APACoordinateDataCalFloatType OrgPt,
    APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, APACoordinateDataCalFloatType Obj2Pt,
    BOOLEAN bSeizeEndCarPosFlag) {
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE StraightOutDefaultEndPosOffsetX;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt2, TempPt3;
  APACarCoordinateDataCalFloatType TempCarPos;
  BOOLEAN bObj2Exist;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosX;
  APA_DISTANCE_CAL_FLOAT_TYPE DefaultEndPosY;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
  APALineParameterABCType TempLine;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE UpdateCntGoStraightOffsetY;
  APACoordinateDataCalFloatType EndCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE EndPosCarAng;

  EndCarPos = APAMap_GInfo.SlotPar.EndPos.Coordinate;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  StraightOutDefaultEndPosOffsetX = 500;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  bObj2Exist = APAMap_GInputData.SlotUpData.bObj2Exist;
  UpdateCntOffsetX = 0;
  UpdateCntOffsetY = 0;
  UpdateCntGoStraightOffsetX = 0;
  UpdateCntGoStraightOffsetY = 0;
  fDis1 = 0;
  TempPt2.x = 0;
  TempPt2.y = 0;
  if (TRUE == bSeizeEndCarPosFlag) {
    UpdateCntOffsetY = 100;
    UpdateCntOffsetX = 150;
    UpdateCntGoStraightOffsetX = kParkOutInfoDefaultOffsetXMm;
    if (TRUE == s_parking_out_state.flags.fsd_from_main_and_sub_slot_border)  // 入侵的边界点来自双边界
    {
      UpdateCntOffsetY = 500;
      if (TRUE == s_parking_out_state.flags.fsd_from_main_slot_border)  // 入侵的边界点来自主边界
      {
        UpdateCntGoStraightOffsetY = 100;
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE))  // fsd in the left of EndCarPos
            ||
            ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)))  // fsd in the right of EndCarPos
        {
          UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
        }
      }
    } else  // 入侵来自单边边界
    {
      if (TRUE == s_parking_out_state.flags.fsd_from_sub_slot_border)  // 入侵的边界点来自子边界
      {
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        UpdateCntOffsetX = -UpdateCntOffsetX;  // 往主边界靠拢
      }
      if (TRUE == s_parking_out_state.flags.fsd_from_main_slot_border)  // 入侵的边界点来自主边界
      {
        UpdateCntGoStraightOffsetX = -UpdateCntGoStraightOffsetX;
        UpdateCntGoStraightOffsetY = 100;
        if (((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)  // fsd in the right of EndCarPos
             && (slot_data_at_right_side == FALSE)) ||
            ((s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE)  // fsd in the left of EndCarPos
             && (slot_data_at_right_side == TRUE))) {
          UpdateCntOffsetX = APAMap_ComCfg.HalfWidthOfCar;
        }
        if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              FALSE))  // fsd in the left of EndCarPos
            ||
            ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) &&
             (s_parking_out_state.flags.fsd_in_right_of_end_car_pos ==
              TRUE)))  // fsd in the right of EndCarPos
        {
          UpdateCntGoStraightOffsetY = -UpdateCntGoStraightOffsetY;
        }
      }
    }
  }

  if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
    DefaultEndPosY = 2000 - UpdateCntOffsetY;
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      DefaultEndPosX = -(APAMap_ComCfg.HalfWidthOfCar + 950) - UpdateCntOffsetX;
    } else {
      if (TRUE == s_parking_out_state.flags.label_angled) {
        if (TRUE == s_parking_out_state.flags.obj_label_ladder) {
          DefaultEndPosX =
              -(APAMap_ComCfg.HalfWidthOfCar + 2000) - UpdateCntOffsetX;
        } else {
          DefaultEndPosX =
              -(APAMap_ComCfg.HalfWidthOfCar + 1100) - UpdateCntOffsetX;
        }
        DefaultEndPosY = 5000 - UpdateCntOffsetY;
      } else {
        DefaultEndPosX =
            -(APAMap_ComCfg.HalfWidthOfCar + 3100) - UpdateCntOffsetX;
        DefaultEndPosY = 4500 - UpdateCntOffsetY;
      }
    }
  } else {
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        EndCarPos, 0, OrgAng, OrgPt);               // APA转锚点坐标系下
    DefaultEndPosY = TempPt2.y - UpdateCntOffsetY;  // 锚点坐标系下
    DefaultEndPosX = -(MATH_ABS(TempPt2.x)) - UpdateCntOffsetX;
  }

  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    TempAng = CurCarPos.CarAng - OrgAng;
    AlgCom_AngNormalized(&TempAng);
    if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
      if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
        fDis1 = -(StraightOutDefaultEndPosOffsetX +
                  APAMap_ComCfg.LenBetweenRAxisAndRBumper -
                  (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
      } else {
        fDis1 = -(StraightOutDefaultEndPosOffsetX +
                  APAMap_ComCfg.LenBetweenRAxisAndFBumper -
                  (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
      }
    } else {
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          EndCarPos, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
      fDis1 = (TempPt2.x +
               (UpdateCntGoStraightOffsetX * MATH_SIN(MATH_FABS(TempAng))));
    }
    EndPosCarAng = APAMap_GInfo.SlotPar.Obj2Ang;
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
      EndPosCarAng += M_PI;
    }
    AlgCom_AngNormalized(&EndPosCarAng);

    TempAng1 = EndPosCarAng - OrgAng;
    AlgCom_AngNormalized(&TempAng1);
    TempCarPos.CarAng = TempAng1;
    TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        CurCarPos.Coordinate, 0, OrgAng, OrgPt);
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
    if (TempLine.LineType == APALineIsVertical) {
      TempPt3.x = 0xff;
      APAMAP_Setfailcause(101);
      return TempPt3;
    } else {
      TempPt3.x = fDis1;
      TempPt3.y = TempLine.A * TempPt3.x + TempLine.C -
                  (UpdateCntGoStraightOffsetY * MATH_SIN(MATH_FABS(TempAng)));
#if 1
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==APAMap_ParkingOutSetEndCarPosInOldCorSys==2==fDis1(%.2f)=="
                 "MATH_SIN(MATH_FABS(TempAng))(%.2f)"
                 "==MATH_SIN(MATH_FABS(TempAng1))(%.2f)==TempPt3(%.2f,%.2f)=="
                 "TempLine.A,C(%.2f,%.2f)"
                 "==UpdateCntGoStraightOffsetX(%.2f)=="
                 "UpdateCntGoStraightOffsetY(%.2f)",
                 fDis1, MATH_SIN(MATH_FABS(TempAng)),
                 MATH_SIN(MATH_FABS(TempAng1)), TempPt3.x, TempPt3.y,
                 TempLine.A, TempLine.C, UpdateCntGoStraightOffsetX,
                 UpdateCntGoStraightOffsetY);
        TLOG_INFO << log_string;
      }
#endif
      TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);
      if (FALSE == s_parking_out_state.flags.after_new_anchor_point) {
        APAMap_GInfo.SlotPar.EndPos.CarAng = CurCarPos.CarAng;
      } else {
        APAMap_GInfo.SlotPar.EndPos.CarAng = EndPosCarAng;
      }
    }
  } else {
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        Obj2Pt, 0, OrgAng, OrgPt);
    // TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(Obj1Pt, 0,
    // OrgAng, OrgPt);

    if ((bObj2Exist == FALSE) || (TRUE == s_parking_out_state.flags.after_new_anchor_point)) {
      TempPt2.x = 0;
    }
    if (slot_data_at_right_side == FALSE) {
      DefaultEndPosX = -DefaultEndPosX + TempPt2.x;
    } else {
      DefaultEndPosX = DefaultEndPosX + TempPt2.x;
    }
    TempPt3.y = DefaultEndPosY;
    TempPt3.x = DefaultEndPosX;
    TempPt3 = AlgCom_PointPosWithAngAndCenterPt(TempPt3, OrgAng, OrgPt);

    APAMap_GInfo.SlotPar.EndPos.CarAng = OrgAng;
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutSetEndCarPosInOldCorSys==UpdateCntOffsetX(%."
             "2f)==UpdateCntOffsetY(%.2f)==fDis1(%.2f)"
             "==TempPt2(%.2f,%.2f)==TempPt3(%.2f,%.2f)==bSeizeEndCarPosFlag(%d)"
             "==bSlotDataAtRigthSide(%d)==DefaultEndPos(%.2f,%.2f)"
             "==bAfterNewAnchorPointFlag(%d)==UpdateCntGoStraightOffsetX(%.2f)",
             UpdateCntOffsetX, UpdateCntOffsetY, fDis1, TempPt2.x, TempPt2.y,
             TempPt3.x, TempPt3.y, bSeizeEndCarPosFlag, slot_data_at_right_side,
             DefaultEndPosX, DefaultEndPosY, s_parking_out_state.flags.after_new_anchor_point,
             UpdateCntGoStraightOffsetX);
    TLOG_INFO << log_string;
  }
  return TempPt3;
}

BOOLEAN APAMap_ParkingOutCenterEndCarPosInfo() {
  APACoordinateDataCalFloatType Data[100];
  APA_ENUM_TYPE DataNum1, DataNum2;
  APACarCoordinateDataCalFloatType EndPos;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType TempPt2;
  APACoordinateDataCalFloatType MainSlotBordPoint;
  APACoordinateDataCalFloatType SubSlotBordPoint;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  APACoordinateDataCalFloatType Pto;
  uint8_t_INF park_out_mode;
  APA_ENUM_TYPE park_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  APA_INDEX_TYPE i, k, m;
  APA_ENUM_TYPE DataNum;
  BOOLEAN slot_data_at_right_side;
  BOOLEAN bSearch;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  BOOLEAN bCenterEndCarPosFlag = FALSE;  // 采用终点位置居中标志位
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis2;

  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return FALSE;
  }
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
      (TRUE == s_parking_out_state.flags.fsd_from_main_and_sub_slot_border) ||
      ((FALSE == s_parking_out_state.flags.fsd_from_main_slot_border) &&
       (FALSE == s_parking_out_state.flags.fsd_from_sub_slot_border))) {
    return FALSE;
  }
  // zqf:PARALLEL_SIDE
  CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
  fDis = 0;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    fDis = CarWidth + 1700;
    if (park_side == APA_CAR_PARK_AT_RIGHT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  } else {
    fDis = CarWidth + 2300;
    if (park_side == APA_CAR_PARK_AT_LEFT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  }
  EndPos = APAMap_GInfo.SlotPar.EndPos;
  Pto = EndPos.Coordinate;
  i = 0;
  m = 0;
  DataNum = 0;
  fDis2 = 0;
  DataNum1 = pMapMainSlotBord->PtNum;
  DataNum2 = pMapSubSlotBord->PtNum;
  bSearch = TRUE;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    fDis2 = 5000;
  } else {
    fDis2 = 7000;
  }
  MainSlotBordPoint.x = 0;
  MainSlotBordPoint.y = 0;
  if (TRUE == slot_data_at_right_side) {
    SubSlotBordPoint.x = -fDis2;
  } else {
    SubSlotBordPoint.x = fDis2;
  }
  SubSlotBordPoint.y = 0;
  fDis2 = 0;
  bCenterEndCarPosFlag = FALSE;
  TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
  while (bSearch) {
    if (m == 0) {
      if (i < DataNum1) {
        for (k = 0; k < DataNum1; k++) {
          // get MainBoundary data
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].y;
        }
        i = DataNum1;
        DataNum = DataNum1;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 1) {
      if (i < DataNum2) {
        for (k = 0; k < DataNum2; k++) {
          // get SubBoundary data
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
        }
        i = DataNum2;
        DataNum = DataNum2;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 0) {
      //----------------------------------
      // Main SlotBord data
      for (k = 0; k < DataNum; k++) {
        TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        if ((TempPt2.y >
             (TempPt.y - APAMap_ComCfg.LenBetweenRAxisAndRBumper - 1000)) &&
            (TempPt2.y <
             (TempPt.y + APAMap_ComCfg.LenBetweenRAxisAndFBumper + 1000))) {
          if ((TempPt2.x < MainSlotBordPoint.x) &&
              (TRUE == slot_data_at_right_side)) {
            MainSlotBordPoint = TempPt2;
          } else if ((TempPt2.x > MainSlotBordPoint.x) &&
                     (FALSE == slot_data_at_right_side)) {
            MainSlotBordPoint = TempPt2;
          }
        }
      }
    } else if (m == 1) {
      //----------------------------------
      // Sub SlotBord data
      for (k = 0; k < DataNum; k++) {
        TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        if ((TempPt2.y >
             (TempPt.y - APAMap_ComCfg.LenBetweenRAxisAndRBumper - 2000)) &&
            (TempPt2.y <
             (TempPt.y + APAMap_ComCfg.LenBetweenRAxisAndFBumper + 2000))) {
          if ((TempPt2.x > SubSlotBordPoint.x) &&
              (TRUE == slot_data_at_right_side)) {
            SubSlotBordPoint = TempPt2;
          } else if ((TempPt2.x < SubSlotBordPoint.x) &&
                     (FALSE == slot_data_at_right_side)) {
            SubSlotBordPoint = TempPt2;
          }
        }
      }
    }
    if (m == 2) {
      bSearch = FALSE;
    }
    if (MATH_FABS(MainSlotBordPoint.x - SubSlotBordPoint.x) > fDis) {
      bCenterEndCarPosFlag = TRUE;
    } else {
      bCenterEndCarPosFlag = FALSE;
    }
    {
      char log_string[512];
      snprintf(
          log_string, sizeof(log_string),
          "==APAMap_ParkingOutCenterEndCarPosInfo==Main Sub SlotBord data=="
          "==TempPt(%.2f,%.2f)==MainSlotBordPoint(%.2f,%.2f)==SubSlotBordPoint("
          "%.2f,%.2f)==Dis(%.2f)==bCenterEndCarPosFlag(%d)",
          TempPt.x, TempPt.y, MainSlotBordPoint.x, MainSlotBordPoint.y,
          SubSlotBordPoint.x, SubSlotBordPoint.y,
          MATH_FABS(MainSlotBordPoint.x - SubSlotBordPoint.x),
          bCenterEndCarPosFlag);
      TLOG_INFO << log_string;
    }
  }
  if (TRUE == bCenterEndCarPosFlag) {
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      fDis2 = 300;
    } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      fDis2 = 500;
    }
    if (FALSE == slot_data_at_right_side) {
      fDis2 = -fDis2;
    }
    TempPt2.x = (MainSlotBordPoint.x + SubSlotBordPoint.x) / 2 + fDis2;
    TempPt2.y = TempPt.y;
    EndPos.Coordinate =
        AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, OrgPt);
    APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
    {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==APAMap_ParkingOutCenterEndCarPosInfo"
               "==MainSlotBordPoint(%.2f,%.2f)==SubSlotBordPoint(%.2f,%.2f)=="
               "EndPos.Coordinate(%.2f,%.2f)==TempPt2(%.2f,%.2f)",
               MainSlotBordPoint.x, MainSlotBordPoint.y, SubSlotBordPoint.x,
               SubSlotBordPoint.y, EndPos.Coordinate.x, EndPos.Coordinate.y,
               TempPt2.x, TempPt2.y);
      TLOG_INFO << log_string;
    }
  }
  if (bCenterEndCarPosFlag == TRUE) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN APAMap_ParkingOutBoundarySeizeEndCarPosInfo() {
  APACoordinateDataCalFloatType Data[100];
  APA_ENUM_TYPE DataNum1, DataNum2;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal2;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal3;
  APACarCoordinateDataCalFloatType EndPos;
  APA_DISTANCE_TYPE NSegNum;
  APA_DISTANCE_TYPE ODNSegNum;
  APACoordinateDataCalFloatType TempPt;
  APA_ENUM_TYPE LocStyle;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType MainLinXStrPt, MainLinXEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
  APACoordinateDataCalFloatType CarLinYStrPt, CarLinYEndPt;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  // APACoordinateDataCalFloatType NSegment[127];
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  uint8_t_INF park_out_mode;
  APA_ENUM_TYPE park_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  APA_INDEX_TYPE i, k, m;
  APA_ENUM_TYPE DataNum;
  BOOLEAN slot_data_at_right_side;
  BOOLEAN bSearch;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1, fDis2;
  APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APA_DISTANCE_TYPE GoStraightSafeDis;
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
  st_MapODDataType* pODInfo;
  Obj_Information_t CurObjComInfo;
  // APACoordinateDataCalFloatType TempPt1;
  // APACoordinateDataCalFloatType TempPt2;
#else
  tMap_VsPillarDataInfo_t* pVsPillarInfo;
  tMap_FusODObjDataInfo_t* pFusODObjInfo;
#endif

  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return FALSE;
  }
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  SafeDisCal = 500;
  SafeDisCal2 = 0;
  SafeDisCal3 = 1500;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  // zqf:PARALLEL_SIDE
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (park_side == APA_CAR_PARK_AT_RIGHT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  } else {
    if (park_side == APA_CAR_PARK_AT_LEFT_SIDE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
  }
  if ((FALSE == s_parking_out_state.flags.prevent_step_n_redundant) &&
      ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) ||
       (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
       (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND))) {
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      fDis1 = 4000;
      fDis2 = 3500;
    } else {
      fDis1 = 6000;
      fDis2 = 4000;
    }
    if (TRUE == s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后
    {
      for (k = 0; k < pMapSubSlotBord->PtNum; k++) {
        // get SubBoundary data
        Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
        Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
        TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            Data[k], 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        if (slot_data_at_right_side) {
          TempPt.x = -TempPt.x;
        }
        if ((TempPt.x < fDis1) && (TempPt.y > -1000)) {
          {
            char log_string[512];
            snprintf(log_string, sizeof(log_string),
                     "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo:TempPt(%."
                     "2f,%.2f)==",
                     TempPt.x, TempPt.y);
            TLOG_INFO << log_string;
          }
          if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
            SafeDisCal2 = 300;
          } else {
            SafeDisCal2 = 1000;
          }
          if (TempPt.x < fDis2) {
            SafeDisCal2 = 200;
            break;
          }
        }
      }
      {
        char log_string[512];
        snprintf(
            log_string, sizeof(log_string),
            "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo:SafeDisCal2(%.2f)",
            SafeDisCal2);
        TLOG_INFO << log_string;
      }
    }
  }
  MaxOutOffsetX = APAMap_ComCfg.HalfWidthOfCar + SafeDisCal;
  MaxOutOffsetY =
      APAMap_ComCfg.LenBetweenRAxisAndFBumper + SafeDisCal + SafeDisCal3;
  MaxInnerOffsetX = -(APAMap_ComCfg.HalfWidthOfCar + SafeDisCal);
  MaxInnerOffsetY = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + SafeDisCal);
  EndPos = APAMap_GInfo.SlotPar.EndPos;
  Angle = EndPos.CarAng;
  Pto = EndPos.Coordinate;
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point) {
    if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
      if (FALSE == slot_data_at_right_side) {
        MaxOutOffsetX += SafeDisCal2;
      } else {
        MaxInnerOffsetX -= SafeDisCal2;
      }
    }
  }
  // MainLinX
  MainLinXStrPt.x = MaxInnerOffsetX;
  MainLinXStrPt.y = MaxInnerOffsetY;
  MainLinXEndPt.x = MaxOutOffsetX;
  MainLinXEndPt.y = MainLinXStrPt.y;
  MainLinXStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt, Angle, Pto);
  MainLinXEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt, Angle, Pto);
  // MainLinY
  MainLinYStrPt.x = MaxOutOffsetX;
  MainLinYStrPt.y = MaxInnerOffsetY;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = MaxOutOffsetY;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // SubLinX
  SubLinXStrPt.x = MaxInnerOffsetX;
  SubLinXStrPt.y = MaxOutOffsetY;
  SubLinXEndPt.x = MaxOutOffsetX;
  SubLinXEndPt.y = SubLinXStrPt.y;
  SubLinXStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinXStrPt, Angle, Pto);
  SubLinXEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinXEndPt, Angle, Pto);
  // SubLinY
  SubLinYStrPt.x = MaxInnerOffsetX;
  SubLinYStrPt.y = MaxInnerOffsetY;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = MaxOutOffsetY;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
  // CarLinY
  CarLinYStrPt.x = 0;
  CarLinYStrPt.y = MaxInnerOffsetY;
  CarLinYEndPt.x = 0;
  CarLinYEndPt.y = MaxOutOffsetY;
  CarLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(CarLinYStrPt, Angle, Pto);
  CarLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(CarLinYEndPt, Angle, Pto);
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
  pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
#else
  pVsPillarInfo = &APAMap_GInputData.VsPillarInfo;
  pFusODObjInfo = &APAMap_GInputData.FusODObjInfo;
#endif
  i = 0;
  m = 0;
  NSegNum = 0;
  ODNSegNum = 0;
  DataNum = 0;
  DataNum1 = pMapMainSlotBord->PtNum;
  DataNum2 = pMapSubSlotBord->PtNum;
  bSearch = TRUE;
  while (bSearch) {
    if (m == 0) {
      if (i < DataNum1) {
        for (k = 0; k < DataNum1; k++) {
          // get MainBoundary data
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pMapMainSlotBord->Points[k].y;
        }
        i = DataNum1;
        DataNum = DataNum1;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 1) {
      if (i < DataNum2) {
        for (k = 0; k < DataNum2; k++) {
          // get SubBoundary data
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMapSubSlotBord->Points[k].y;
        }
        i = DataNum2;
        DataNum = DataNum2;
      } else {
        i = 0;
        m++;
      }
    }
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
    if (m == 2) {
      while (i < pODInfo->Square.ObjNum) {
        CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
        if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
            (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
            (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
            (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
            (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
            (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
            (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
            (CurObjComInfo.Label == Obj_Label_WheelChock)) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Square.ObjNum) {
        Data[0].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.x;
        Data[0].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.y;
        Data[1].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.x;
        Data[1].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.y;
        Data[2].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.x;
        Data[2].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.y;
        Data[3].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.x;
        Data[3].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.y;
        DataNum = 4;
        i++;
      } else {
        m++;
        i = 0;
      }
    }
    if (m == 3) {
      bSearch = FALSE;
    }
#if 0
    if (m == 3) {
     
      if (i < pODInfo->Triangle.ObjNum) {
        Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.x;
        Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.y;
        Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.x;
        Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.y;
        Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.x;
        Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.y;
        DataNum = 3;
        CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
        i++;
      } else {
        m++;
        i = 0;
      }
    }
    if (m == 4) {
      if (i < pODInfo->CirCular.ObjNum) {
        TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.x;
        TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.y;
        TempPt2.x =
            -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        TempPt2.y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.x = -TempPt2.x;
        Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        DataNum = 4;
        CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
        i++;
      } else {
        m++;
        i = 0;
      }
    }
    if (m == 5) {
      if (i < pODInfo->Polygon.ObjNum) {
        for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .y;
        }
        DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
        CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
        i++;
      } else {
        i = 0;
        m++;
        bSearch = FALSE;
      }
    }
#endif
#else
    if (m == 2) {
      if (i < pVsPillarInfo->Pillar2InfoAtPark.PillarNum) {
        for (k = 0; k < 4; k++) {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar2InfoAtPark
                  .Pillar[i]
                  .SquarePillar.Pt[k]
                  .x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar2InfoAtPark
                  .Pillar[i]
                  .SquarePillar.Pt[k]
                  .y;
        }
        DataNum = 4;
        i++;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 3) {
      if (i < pVsPillarInfo->Pillar1InfoAtPark.PillarNum) {
        for (k = 0; k < 4; k++) {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar1InfoAtPark
                  .Pillar[i]
                  .SquarePillar.Pt[k]
                  .x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pVsPillarInfo->Pillar1InfoAtPark
                  .Pillar[i]
                  .SquarePillar.Pt[k]
                  .y;
        }
        DataNum = 4;
        i++;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 4) {
      if (i < pFusODObjInfo->ObjNum) {
        for (k = 0; k < 4; k++) {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pFusODObjInfo->Obj[i].BBox.Pt[k].x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pFusODObjInfo->Obj[i].BBox.Pt[k].y;
        }
        DataNum = 4;
        i++;
      } else {
        i = 0;
        m++;
      }
    }
    if (m == 5) {
      bSearch = FALSE;
    }
#endif
    if (TRUE == bSearch) {
    if ((m == 0) || (m == 1)) {
      //----------------------------------
      // Get valid fsd data
#if 1
      if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
          (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
        if (TRUE == s_parking_out_state.flags.label_angled) {
          GoStraightSafeDis = 500;
        } else {
          GoStraightSafeDis = 500;
        }
        if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
          fDis1 = 2000;
          FOffset = GoStraightSafeDis;
          BOffset = 50;
        } else if (park_out_mode ==
                   APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
          fDis1 = -1000;
          FOffset = 50;
          BOffset = GoStraightSafeDis;
        }
        if (m == 0) {
          LOffset = 20;
          ROffset = 20;
          APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
                                pRectPt, pRectLine);
          TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
          if (TempPt.x < fDis1) {
              if (TRUE == APAMap_CheckIfObjWithinRectArea(
                              0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
              NSegNum++;
              s_parking_out_state.flags.fsd_from_main_slot_border = TRUE;
              LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                  &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
              if (LocStyle != 0) {
                s_parking_out_state.flags.fsd_in_right_of_end_car_pos = TRUE;
              } else {
                s_parking_out_state.flags.fsd_in_right_of_end_car_pos = FALSE;
              }
            }
          }
        } else  //(m == 1)
        {
          LOffset = 50;
          ROffset = 50;
          APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
                                pRectPt, pRectLine);
          TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
          if (TempPt.x < fDis1) {
              if (TRUE == APAMap_CheckIfObjWithinRectArea(
                              0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
              NSegNum++;
              s_parking_out_state.flags.fsd_from_sub_slot_border = TRUE;
              }
            }
          }
          if (NSegNum > 0) {
            char log_string[512];
            snprintf(log_string, sizeof(log_string),
                     "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==1==m(%d)"
                     "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
                     "bFsdFromMapMainSlotBordFlag(%d)=="
                     "bFsdFromMapSubSlotBordFlag(%d)",
                     m, s_parking_out_state.flags.fsd_in_right_of_end_car_pos, NSegNum,
                     s_parking_out_state.flags.fsd_from_main_slot_border, s_parking_out_state.flags.fsd_from_sub_slot_border);
            TLOG_INFO << log_string;
          }
      } else
#endif
      {
        for (k = 0; k < DataNum; k++) {
          TempPt = Data[k];
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
          if (LocStyle != 1) {
            LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                  &MainLinXStrPt, &MainLinXEndPt, &TempPt, &fDis);
              if (LocStyle != 1) {
                LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
            if (LocStyle != 0) {
              LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                  &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
              if (LocStyle != 0) {
                // NSegment[NSegNum] = TempPt;
                NSegNum++;
                if (m == 0) {
                  s_parking_out_state.flags.fsd_from_main_slot_border = TRUE;
                }
                if (m == 1) {
                  s_parking_out_state.flags.fsd_from_sub_slot_border = TRUE;
                }
                LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                    &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
                if (LocStyle != 0) {
                  s_parking_out_state.flags.fsd_in_right_of_end_car_pos = TRUE;
                } else {
                  s_parking_out_state.flags.fsd_in_right_of_end_car_pos = FALSE;
                }
                {
                  char log_string[512];
                  snprintf(log_string, sizeof(log_string),
                               "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo=="
                               "2==m(%d)"
                           "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
                           "bFsdFromMapMainSlotBordFlag(%d)=="
                           "bFsdFromMapSubSlotBordFlag(%d)",
                           m, s_parking_out_state.flags.fsd_in_right_of_end_car_pos, NSegNum,
                           s_parking_out_state.flags.fsd_from_main_slot_border,
                           s_parking_out_state.flags.fsd_from_sub_slot_border);
                  TLOG_INFO << log_string;
                }
              }
            }
          }
        }
      }
      if (NSegNum < 1) {
        fDis1 = -2000;
        if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
          FOffset = 2000;
          BOffset = 200;
          LOffset = 200;
          ROffset = 200;
        } else {
          FOffset = 2000;
          BOffset = 500;
          LOffset = 500;
          ROffset = 500;
        }
        APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
                              pRectPt, pRectLine);
        TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        if (TempPt.y > fDis1) {
              if (TRUE == APAMap_CheckIfObjWithinRectArea(
                              0x00, &Data[0], DataNum, pRectPt, pRectLine)) {
            NSegNum++;
            if (m == 0) {
              s_parking_out_state.flags.fsd_from_main_slot_border = TRUE;
            }
            if (m == 1) {
              s_parking_out_state.flags.fsd_from_sub_slot_border = TRUE;
            }
            LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
            if (LocStyle != 0) {
              s_parking_out_state.flags.fsd_in_right_of_end_car_pos = TRUE;
            } else {
              s_parking_out_state.flags.fsd_in_right_of_end_car_pos = FALSE;
            }
            {
              char log_string[512];
                  snprintf(
                      log_string, sizeof(log_string),
                       "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==3==m(%d)"
                       "==bFsdInRightOfEndCarPosFlag(%d)==NSegNum(%d)=="
                       "bFsdFromMapMainSlotBordFlag(%d)=="
                       "bFsdFromMapSubSlotBordFlag(%d)",
                       m, s_parking_out_state.flags.fsd_in_right_of_end_car_pos, NSegNum,
                       s_parking_out_state.flags.fsd_from_main_slot_border, s_parking_out_state.flags.fsd_from_sub_slot_border);
              TLOG_INFO << log_string;
            }
          }
        }
      }
    }
  }
  if ((m > 1) && (m < 5)) {
    //----------------------------------
    // Get valid OD data
    if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
      if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) {
        fDis1 = 1000;
        FOffset = 1000;
        BOffset = 50;
      } else if (park_out_mode ==
                 APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) {
        fDis1 = -2000;
        FOffset = 50;
        BOffset = 1000;
      }
      LOffset = 50;
      ROffset = 50;
          APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, EndPos,
                                pRectPt, pRectLine);
      TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          EndPos.Coordinate, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
      if (TempPt.x < fDis1) {
        if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], DataNum,
                                                    pRectPt, pRectLine)) {
          ODNSegNum++;
        }
      }
    } else {
      for (k = 0; k < DataNum; k++) {
        TempPt = Data[k];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
        if (LocStyle != 1) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                  &MainLinXStrPt, &MainLinXEndPt, &TempPt, &fDis);
          if (LocStyle != 1) {
            LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
            if (LocStyle != 0) {
              LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                  &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
              if (LocStyle != 0) {
                LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                    &CarLinYStrPt, &CarLinYEndPt, &TempPt, &fDis);
                if (LocStyle != 0) {
                  s_parking_out_state.flags.fsd_in_right_of_end_car_pos = TRUE;
                  if (slot_data_at_right_side) {
                    s_parking_out_state.flags.fsd_from_main_slot_border = TRUE;
                  } else {
                    s_parking_out_state.flags.fsd_from_sub_slot_border = TRUE;
                  }
                } else {
                  s_parking_out_state.flags.fsd_in_right_of_end_car_pos = FALSE;
                  if (slot_data_at_right_side) {
                    s_parking_out_state.flags.fsd_from_sub_slot_border = TRUE;
                  } else {
                    s_parking_out_state.flags.fsd_from_main_slot_border = TRUE;
                  }
                }
                // NSegment[ODNSegNum] = TempPt;
                ODNSegNum++;
                {
                  char log_string[512];
                  snprintf(log_string, sizeof(log_string),
                               "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo=="
                               "4==m(%d)"
                               "==bFsdInRightOfEndCarPosFlag(%d)==ODNSegNum(%d)"
                               "==bFsdFromMapMainSlotBordFlag(%d)=="
                           "bFsdFromMapSubSlotBordFlag(%d)",
                           m, s_parking_out_state.flags.fsd_in_right_of_end_car_pos, ODNSegNum,
                           s_parking_out_state.flags.fsd_from_main_slot_border,
                           s_parking_out_state.flags.fsd_from_sub_slot_border);
                  TLOG_INFO << log_string;
                    }
                }
              }
            }
          }
        }
      }
    }
  }
  if ((NSegNum == 0) && (ODNSegNum == 0)) {
    s_parking_out_state.flags.fsd_from_main_slot_border = FALSE;
    s_parking_out_state.flags.fsd_from_sub_slot_border = FALSE;
  }
  if ((TRUE == s_parking_out_state.flags.fsd_from_main_slot_border) &&
      (TRUE == s_parking_out_state.flags.fsd_from_sub_slot_border)) {
    s_parking_out_state.flags.fsd_from_main_and_sub_slot_border = TRUE;
  } else {
    s_parking_out_state.flags.fsd_from_main_and_sub_slot_border = FALSE;
  }
}
{
  char log_string[512];
  snprintf(log_string, sizeof(log_string),
           "==APAMap_ParkingOutBoundarySeizeEndCarPosInfo==5==NSegNum(%d)=="
           "ODNSegNum(%d)"
           "==bFsdFromMapMainAndSubSlotBordFlag(%d)=="
           "bFsdFromMapMainSlotBordFlag(%d)==bFsdFromMapSubSlotBordFlag(%d)",
           NSegNum, ODNSegNum, s_parking_out_state.flags.fsd_from_main_and_sub_slot_border,
           s_parking_out_state.flags.fsd_from_main_slot_border, s_parking_out_state.flags.fsd_from_sub_slot_border);
  TLOG_INFO << log_string;
}
if ((NSegNum != 0) || (ODNSegNum != 0)) {
  return TRUE;
}
return FALSE;
}

void APAMap_ParkingOutEndCarPosUpdata(void) {
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType Obj2Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType OrgPt;
  APACarCoordinateDataCalFloatType EndPos;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  BOOLEAN slot_data_at_right_side;
  APACarCoordinateDataCalFloatType TempCarPos;
  BOOLEAN bUpdataObjFlag;
  APA_ENUM_TYPE i;
  BOOLEAN bCenterEndCarPosFlag = FALSE;  // 采用终点位置居中标志位
  BOOLEAN bInsideSlotFlag;               // 车辆在车位内标志位
  BOOLEAN bSeizeEndCarPosFlag;           // fsd侵占终点位置标志位

  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  bUpdataObjFlag = TRUE;
  bInsideSlotFlag = TRUE;

  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  Obj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  // 除了直进直出，其他场景，当检测到车辆已泊出车位，且车头方向已与终点位置方向基本一致（角度偏差不超过5度），则把终点位置定在与车x轴坐标一致的位置，且y轴向前推进1m.
  if (TRUE ==
      s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位，则不再更新Obj1、Obj2
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if ((CurCarCoordinateX > 1) &&
        (MATH_ABS(APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x -
                  APAMap_GInfo.SlotPar.EndPos.Coordinate.x) > 200)) {
      bUpdataObjFlag = FALSE;
    }
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if (CurCarCoordinateX > -1) {
        bInsideSlotFlag = FALSE;
      }
    } else {
      if (CurCarCoordinateX > 0) {
        bInsideSlotFlag = FALSE;
      }
    }
  }
  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) &&
      (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    if ((FALSE == bUpdataObjFlag) &&
        (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
                   APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 36)) &&
        (FALSE == s_parking_out_state.flags.prevent_step_n_redundant) &&
        (FALSE == s_parking_out_state.flags.lane_line_update_end_car_pos) &&
        (FALSE == s_parking_out_state.flags.reference_line_update_end_car_pos)) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==EndCarPosUpdata_1m===Before"
                 "==CarPos.CarAng(%.2f),EndPos_CarAng(%.2f),EndPos_Coordinate(%"
                 ".2f,%.2f)",
                 APAMap_GInputData.CarLocInfo.CarPos.CarAng,
                 APAMap_GInfo.SlotPar.EndPos.CarAng,
                 APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
                 APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
        TLOG_INFO << log_string;
      }
      EndPos.Coordinate = APAMap_GInputData.CarLocInfo.CarPos.Coordinate;
      EndPos.Coordinate.y += 1000;
#ifdef SUPPORT_PARKING_OUT_UWB
      if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x == NO_OBJ_DISTANCE) {
        APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
      }
#else
      APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
#endif
      s_parking_out_state.flags.prevent_step_n_redundant = TRUE;
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==EndCarPosUpdata_1m===After"
                 "==CarPos.CarAng(%.2f),EndPos_CarAng(%.2f),EndPos_Coordinate(%"
                 ".2f,%.2f)",
                 APAMap_GInputData.CarLocInfo.CarPos.CarAng,
                 APAMap_GInfo.SlotPar.EndPos.CarAng,
                 APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
                 APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
        TLOG_INFO << log_string;
      }
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==EndCarPosUpdata===Before==EndPos_Coordinate(%.2f,%.2f)",
             APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
    TLOG_INFO << log_string;
  }
  bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
  // CenterEndCarPosUpdata
  if ((TRUE == bSeizeEndCarPosFlag) ||
      ((TRUE == s_parking_out_state.flags.cnt_add) && (FALSE == bSeizeEndCarPosFlag) &&
       (FALSE == s_parking_out_state.flags.prevent_step_n_redundant) && (FALSE == bInsideSlotFlag))) {
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
  if ((TRUE == s_parking_out_state.flags.after_new_anchor_point) &&
      (APAMap_GInfo.SlotPar.EndPos.Coordinate.y < -8000)) {
    bSeizeEndCarPosFlag = FALSE;
  }
  // EndCarPosUpdate
  i = 0;
  while (TRUE == bSeizeEndCarPosFlag) {
    TempCarPos.CarAng = APAMap_GInfo.SlotPar.EndPos.CarAng;
#ifdef SUPPORT_PARKING_OUT_UWB
    if (APAMap_GInputData.ParkReqPar.Parkout_UWBPos.x != NO_OBJ_DISTANCE) {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB(
          park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    } else {
      EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
          park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    }
#else
    EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
#endif
    APAMap_GInfo.SlotPar.EndPos.Coordinate = EndPos.Coordinate;
    if ((TRUE == s_parking_out_state.flags.lane_line_update_end_car_pos) ||
        (TRUE == s_parking_out_state.flags.reference_line_update_end_car_pos)) {
      APAMap_GInfo.SlotPar.EndPos.CarAng = TempCarPos.CarAng;
    }
    bSeizeEndCarPosFlag = APAMap_ParkingOutBoundarySeizeEndCarPosInfo();
    i++;
    if (i > 9) {
      bSeizeEndCarPosFlag = FALSE;
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==EndCarPosUpdata===After==EndPos_Coordinate(%.2f,%.2f)",
             APAMap_GInfo.SlotPar.EndPos.Coordinate.x,
             APAMap_GInfo.SlotPar.EndPos.Coordinate.y);
    TLOG_INFO << log_string;
  }
}
void APAMAP_ParkingOutGetSlotBdPtByODObjs(
    APA_ENUM_TYPE Bordpttype, APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY) {
  APA_ENUM_TYPE i, j;
  APA_ENUM_TYPE k;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE OffsetY;
  APA_ENUM_TYPE TargetXLoc;
  APA_ENUM_TYPE TargetYLoc;
  APACoordinateDataCalFloatType LineXStrPt;
  APACoordinateDataCalFloatType LineXEndPt;
  APACoordinateDataCalFloatType LineYStrPt;
  APACoordinateDataCalFloatType LineYEndPt;
  APACoordinateDataCalFloatType Data[10];
  APA_ENUM_TYPE DataNum;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOutOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxInnerOffsetY;

  // APACoordinateDataCalFloatType TempPt1;
  // APACoordinateDataCalFloatType TempPt2;
  st_MapODDataType* pODInfo;
  BOOLEAN bSearch;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APA_DISTANCE_CAL_FLOAT_TYPE ObjAng;
  APACoordinateDataCalFloatType OrgPt;
  BOOLEAN bDataAtRightSide;
  APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE PreOffsetX;
  APACoordinateDataCalFloatType OffsetYRefPt;
  APACoordinateDataCalFloatType OffsetXRefPt;
  APACoordinateDataType ODInSlotPtForOffsetX;
  APACoordinateDataType ODInSlotPtForOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  // APACarCoordinateDataCalFloatType TempCarPos;
  Obj_Information_t CurObjComInfo;
  uint8_t_INF park_out_mode;
#if 1
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    *pOffsetX = 0;
    *pOffsetY = 0;
    return;
  }
  pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
#else
  if (APAMap_GInputData.VisObjsInfo.timestamp_ms == 0) {
    *pOffsetX = 0;
    *pOffsetY = 0;
    return;
  }
  pODInfo = &APAMap_GInputData.VisObjsInfo.ODInfo;
#endif
  i = 0;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  MaxOutOffsetX = 2000;
  MaxOutOffsetY = 1000;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    MaxInnerOffsetX = 1500;
  } else {
    MaxInnerOffsetX = 2800;
  }
  bDataAtRightSide = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  if (Bordpttype == 1) {
    ObjAng = APAMap_GInfo.SlotPar.Obj2Ang;
  } else {
    ObjAng = APAMap_GInfo.SlotPar.Obj1Ang;
  }
  // ObjAng = APAMap_GInfo.SlotPar.EndPos.CarAng;

  if (bDataAtRightSide == TRUE) {
    if (Bordpttype == 0) {
      TargetYLoc = 1;  // right;
    } else {
      TargetYLoc = 0;  // left;
    }
    TargetXLoc = 0;  // left;
  } else {
    if (Bordpttype == 0) {
      TargetYLoc = 0;  // left
    } else {
      TargetYLoc = 1;  // right;
    }
    TargetXLoc = 1;  // right
  }
  LineYStrPt = APAMap_GInfo.NewCordSysOPt;
  LineYEndPt.y = LineYStrPt.y + 1000 * MATH_COS(OrgAng);
  LineYEndPt.x = LineYStrPt.x - 1000 * MATH_SIN(OrgAng);
  if (Bordpttype == 0) {
    LineXStrPt = APAMap_GInfo.SlotPar.Obj1Pt;
  } else {
    LineXStrPt = APAMap_GInfo.SlotPar.Obj2Pt;
  }
  TempDis = APAMap_GetSearchMaxInnerY(Bordpttype, bDataAtRightSide, LineXStrPt,
                                      ObjAng);
  TempDis -= 300;
  if (TempDis < 0) {
    TempDis = 300;
  }
  MaxInnerOffsetY = TempDis;

  LineXEndPt.y = LineXStrPt.y + 1000 * MATH_COS(ObjAng);
  LineXEndPt.x = LineXStrPt.x - 1000 * MATH_SIN(ObjAng);
  bSearch = TRUE;
  i = 0;
  j = 0;
  k = 0;
  OffsetX = -MaxInnerOffsetX;
  OffsetY = -MaxOutOffsetY;
  PreOffsetY = 0;
  PreOffsetX = 0;
  ODInSlotPtForOffsetX.x = NO_OBJ_DISTANCE;
  ODInSlotPtForOffsetX.y = NO_OBJ_DISTANCE;
  ODInSlotPtForOffsetY.x = NO_OBJ_DISTANCE;
  ODInSlotPtForOffsetY.y = NO_OBJ_DISTANCE;
  while (bSearch) {
    if (j == 0) {
      while (i < pODInfo->Square.ObjNum) {
        CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
        if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
            (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
            (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
            (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
            (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
            (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
            (CurObjComInfo.Label == Obj_Label_Stone_Piers)) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Square.ObjNum) {
        Data[0].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.x;
        Data[0].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.y;
        Data[1].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.x;
        Data[1].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.y;
        Data[2].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.x;
        Data[2].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.y;
        Data[3].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.x;
        Data[3].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.y;
        DataNum = 4;
        i++;
      } else {
        j++;
        i = 0;
      }
    }
#if 0
    if (j == 1) {
     
      if (i < pODInfo->Triangle.ObjNum) {
        Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.x;
        Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.y;
        Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.x;
        Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.y;
        Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.x;
        Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.y;
        DataNum = 3;
        CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 2) {
      if (i < pODInfo->CirCular.ObjNum) {
        TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.x;
        TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.y;
        TempPt2.x =
            -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        TempPt2.y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.x = -TempPt2.x;
        Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, OrgAng, TempPt1);
        DataNum = 4;
        CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 3) {
      if (i < pODInfo->Polygon.ObjNum) {
        for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .y;
        }
        DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
        CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
      } else {
        bSearch = FALSE;
      }
    }
#else
    if (j == 1) {
      while (i < pODInfo->Polygon.ObjNum) {
        CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
        if (CurObjComInfo.Label == Obj_Label_Curb) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Polygon.ObjNum) {
        for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .y;
        }
        DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
        i++;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 2) {
      bSearch = FALSE;
    }
#endif

    if (bSearch) {
      PreOffsetY = OffsetY;
      PreOffsetX = OffsetX;
      APAMAP_GetSlotBdPtOffsetByGivenObjPts(
          TargetXLoc, TargetYLoc, &LineXStrPt, &LineXEndPt, &LineYStrPt,
          &LineYEndPt, &Data[0], DataNum, MaxOutOffsetX, MaxInnerOffsetX,
          MaxOutOffsetY, MaxInnerOffsetY, &OffsetX, &OffsetY, &OffsetYRefPt,
          &OffsetXRefPt);
      if (PreOffsetX < OffsetX) {
        ODInSlotPtForOffsetX.x = (APA_DISTANCE_TYPE)OffsetXRefPt.x;
        ODInSlotPtForOffsetX.y = (APA_DISTANCE_TYPE)OffsetXRefPt.y;
      }
      if (PreOffsetY < OffsetY) {
        ODInSlotPtForOffsetY.x = (APA_DISTANCE_TYPE)OffsetYRefPt.x;
        ODInSlotPtForOffsetY.y = (APA_DISTANCE_TYPE)OffsetYRefPt.y;
      }
    }
    i++;
  };
#ifdef DEBUG_PRINT_SLOTOBJ
  char log_string[512];
  snprintf(log_string, sizeof(log_string),
           "==ODOffset(%u)(%f,%f),Max(%f),ODInSlotPtForOffsetX(%d,%d),"
           "ODInSlotPtForOffsetY(%d,%d))",
           Bordpttype, OffsetX, OffsetY, MaxInnerOffsetY,
           ODInSlotPtForOffsetX.x, ODInSlotPtForOffsetX.y,
           ODInSlotPtForOffsetY.x, ODInSlotPtForOffsetY.y);
  TLOG_INFO << log_string;
#endif
  if ((OffsetX > 50) || (OffsetY > 50)) {
    if (OffsetX < 0) {
      OffsetX = 0;
    }
    *pOffsetX = OffsetX;
    *pOffsetY = OffsetY;
  } else {
    *pOffsetX = 0;
    *pOffsetY = 0;
  }
  APAMap_GInfo.SlotPar.ODPt[Bordpttype] = ODInSlotPtForOffsetX;
  return;
}
void APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  APAMAP_ParkingOutGetSlotBdPtByODObjs(0, pOffsetX1, pOffsetY1);
  APAMAP_ParkingOutGetSlotBdPtByODObjs(1, pOffsetX2, pOffsetY2);

  return;
}
void APAMap_ParkingOutPickDispersedObstacles(ObstaclesInfo_INF* pObjInfo) {
  st_MapODDataType* pODInfo;
  BOOLEAN bSearch;
  APA_INDEX_TYPE i, j, k, m;
  APACoordinateDataCalFloatType Data[10];
  APACoordinateDataCalFloatType DataTemp[10];
  APA_ENUM_TYPE PtNum;
  // APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  // APACoordinateDataCalFloatType TempPt1, TempPt2;
  uint8_t_INF ObjNum;
  Obj_Information_t CurObjComInfo;
  APA_ENUM_TYPE ParkMode;
  APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
  // APA_DISTANCE_CAL_FLOAT_TYPE Offset;
  APACarCoordinateDataCalFloatType CarPos;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACoordinateDataCalFloatType pRectPtForLimiter[4];
  APALineParameterABCType pRectLineForLimiter[4];
  // float Confidence;
  uint8_t_INF ObjType;
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType pRectPt2[4];
  APALineParameterABCType pRectLine2[4];
  APACoordinateDataCalFloatType pRectPtForLimiter2[4];
  APALineParameterABCType pRectLineForLimiter2[4];
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType pRectPt3[4];
  APALineParameterABCType pRectLine3[4];
  APA_ENUM_TYPE Cnt;
  BOOLEAN slot_data_at_right_side;

#ifdef APA_MAP_DEBUG_INFO_LIMITER
  APA_ENUM_TYPE LimiterNum;
  LimiterNum = 0;
#endif
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CarPos = APAMap_GInfo.SlotPar.EndPos;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  pObjInfo->obstacle_num = 0;
  pObjInfo->timestamp_ms = APAMap_GInputData.ParkReqPar.timestamp_ms;
  APAMap_CalAndAddRskOBjObstacles(pObjInfo);
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return;
  }
  pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
  // Angle = APAMap_GInfo.NewCordSysAng;

  if (pObjInfo == NULL) {
    return;
  }
  i = 0;
  j = 0;
  ObjNum = pObjInfo->obstacle_num;
  bSearch = TRUE;
  FOffset = 150;
  BOffset = 150;
  LOffset = 200;
  ROffset = 200;
  // Offset = 300;
  APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos, pRectPt,
                        pRectLine);
  APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos, pRectPt,
                        pRectLine);
  FOffset =
      -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndFBumper -
                                     APAMap_ComCfg.LenBetweenFRAxis);
  BOffset =
      -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndRBumper -
                                     100);
  LOffset = 200;
  ROffset = 200;
  APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CarPos,
                        pRectPtForLimiter, pRectLineForLimiter);
  if (ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    FOffset = 10;
    BOffset = 10;
    LOffset = 10;
    ROffset = 10;
  } else {
    FOffset = 50;
    BOffset = 50;
    LOffset = 50;
    ROffset = 50;
  }
  APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos, pRectPt2,
                        pRectLine2);
  FOffset =
      -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndFBumper -
                                     APAMap_ComCfg.LenBetweenFRAxis);
  BOffset =
      -(APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg.LenBetweenRAxisAndRBumper -
                                     100);
  LOffset = 200;
  ROffset = 200;
  APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
                        pRectPtForLimiter2, pRectLineForLimiter2);
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side) {
      FOffset = 1500;
      BOffset = 1500;
      LOffset = -(APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
      ROffset = 10;
    } else {
      FOffset = 1500;
      BOffset = 1500;
      LOffset = 10;
      ROffset = -(APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
    }
    APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
                          pRectPt3, pRectLine3);
  }

  while (bSearch) {
    PtNum = 0;
    if (j == 0) {
      while (i < pODInfo->Square.ObjNum) {
        CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
        if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
            (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
            (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
            (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
            (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
            (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
            (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
            (CurObjComInfo.Label == Obj_Label_WheelChock)) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Square.ObjNum) {
        Data[0].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.x;
        Data[0].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.y;
        Data[1].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.x;
        Data[1].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.y;
        Data[2].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.x;
        Data[2].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.y;
        Data[3].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.x;
        Data[3].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.y;
        PtNum = 4;
        i++;
      } else {
        j++;
        i = 0;
      }
    }
#if 0
    if (j == 1) {
      if (i < pODInfo->Triangle.ObjNum) {
        Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.x;
        Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_1.y;
        Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.x;
        Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_2.y;
        Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.x;
        Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Triangle.Triangles[i]
                        .Points.Point_3.y;
        CurObjComInfo = pODInfo->Triangle.Triangles[i].ObjInfo;
        Data[3] = {0.0, 0.0}; // to remove build warning
        PtNum = 3;
        i++;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 2) {
      if (i < pODInfo->CirCular.ObjNum) {
        TempPt1.x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.x;
        TempPt1.y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i]
                        .CenterPoint.y;
        TempPt2.x =
            -(APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        TempPt2.y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->CirCular.Circulars[i].Radius;
        Data[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[1] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
        TempPt2.x = -TempPt2.x;
        Data[2] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);
        TempPt2.y = -TempPt2.y;
        Data[3] = AlgCom_PointPosWithAngAndCenterPt(TempPt2, Angle, TempPt1);

        PtNum = 4;
        CurObjComInfo = pODInfo->CirCular.Circulars[i].ObjInfo;
        i++;
      } else {
        j++;
        i = 0;
      }
    }

    if (j == 3) 
    {
#if 0
      if (i < pODInfo->Polygon.ObjNum) 

      {
        for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
          Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .x;
          Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                          .Points.Point[k]
                          .y;
        }
        PtNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
        CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
        i++;
      } else
#else
#endif
      {
        PtNum = 0;
        bSearch = FALSE;
      }
     
    }
#else
    if (j == 1) {
      while (i < pODInfo->Polygon.ObjNum) {
        CurObjComInfo = pODInfo->Polygon.Polygons[i].ObjInfo;
        if (CurObjComInfo.Label == Obj_Label_Curb) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Polygon.ObjNum) {
        Cnt = pODInfo->Polygon.Polygons[i].Points.PointNum;
        m = 0;
        for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
          DataTemp[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                  .Points.Point[k]
                  .x;
          DataTemp[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                  .Points.Point[k]
                  .y;
          if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
            if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &DataTemp[k], 1,
                                                        pRectPt3, pRectLine3)) {
              Cnt--;
              if (Cnt < 0) {
                Cnt = 0;
              }
              continue;
            }
          }
          Data[m].x = DataTemp[k].x;
          Data[m].y = DataTemp[k].y;
          m++;
        }
        PtNum = Cnt;
        i++;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 2) {
      PtNum = 0;
      bSearch = FALSE;
    }
#endif
    if (ObjNum >= OBSTACLE_MAX_NUM) {
      bSearch = FALSE;
    }
    if (bSearch) {
      ObjType = CurObjComInfo.Label + APA_MAP_PT_PROPERTY_OD_STR;
      for (k = 0; k < PtNum; k++) {
        pObjInfo->obstacle_list[ObjNum].polygon_point_list[k].x =
            Data[k].x * 0.001;
        pObjInfo->obstacle_list[ObjNum].polygon_point_list[k].y =
            Data[k].y * 0.001;
      }
      pObjInfo->obstacle_list[ObjNum].id = ObjNum;  // CurObjComInfo.ID;
      pObjInfo->obstacle_list[ObjNum].type = ObjType;
      if (ObjType != OD_OBJ_WheelChock) {
        if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
                                                    pRectPt2, pRectLine2)) {
          pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
        } else if (TRUE == APAMap_CheckIfObjWithinRectArea(
                               0x01, &Data[0], PtNum, pRectPt, pRectLine)) {
          pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
        } else if (ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
#if 0
           if(TRUE == APAMap_CheckIfObjWithinRectArea(0x01,&Data[0],PtNum,pRectPt,pRectLine))
           {
              if(TRUE == APAMap_CheckIfObjWithinRectArea(0x01,&Data[0],PtNum,pRectPtForLimiter,pRectLineForLimiter))
              {
                 pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
              }
           }
#endif
        }
      } else {
        if ((ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) &&
            (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
                                                     pRectPt2, pRectLine2))) {
          if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
            if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], PtNum,
                                                        pRectPtForLimiter2,
                                                        pRectLineForLimiter2)) {
              pObjInfo->obstacle_list[ObjNum].type = OBJ_IGNORE;
            }
          } else {
            if (FALSE == APAMap_CheckIfObjWithinRectArea(
                             0x01, &Data[0], PtNum, pRectPtForLimiter2,
                             pRectLineForLimiter2)) {
              s_parking_out_state.flags.od_wheel_chock = TRUE;
            }
          }
        }
      }
      pObjInfo->obstacle_list[ObjNum].confidence = CurObjComInfo.Confidence;
      pObjInfo->obstacle_list[ObjNum].polygon_point_num = PtNum;
      if ((CurObjComInfo.Speed.x == 0) && (CurObjComInfo.Speed.y == 0) &&
          (CurObjComInfo.Speed.z == 0) && (CurObjComInfo.Acceleration.x == 0) &&
          (CurObjComInfo.Acceleration.y == 0) &&
          (CurObjComInfo.Acceleration.z == 0)) {
        pObjInfo->obstacle_list[ObjNum].is_static = TRUE;
      } else {
        pObjInfo->obstacle_list[ObjNum].is_static = FALSE;
      }
      pObjInfo->obstacle_list[ObjNum].length = 0;
      pObjInfo->obstacle_list[ObjNum].width = 0;
      pObjInfo->obstacle_list[ObjNum].theta = 0;
      ObjNum++;
    }
  }
  pObjInfo->timestamp_ms = APAMap_GInputData.ParkReqPar.timestamp_ms;
  pObjInfo->obstacle_num = ObjNum;
  return;
}
BOOLEAN APAMap_ParkingOutUpDataMapInfoBySlotCorInfo() {
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  BOOLEAN bUpdataObjFlag;
  BOOLEAN slot_data_at_right_side;
  uint8_t_INF park_out_mode;

  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  bUpdataObjFlag = TRUE;
  s_parking_out_state.flags.cnt_add = FALSE;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 6) &&
      (APAMap_GInputData.ParkReqPar.APAstate >= 4) &&
      ((APAMap_GInputData.ParkReqPar.Request_cmd == 2) ||
       (APAMap_GInputData.ParkReqPar.Request_cmd == 7))) {
    s_parking_out_state.flags.after_new_anchor_point = TRUE;
  } else {
    s_parking_out_state.flags.after_new_anchor_point = FALSE;
  }
  if ((TRUE == s_parking_out_state.flags.after_new_anchor_point) &&
      ((MATH_FABS(APAMap_GInfo.SlotPar.SlotBordPt[0].x) - 0.0) <= 2000) &&
      (APAMap_GInputData.ParkReqPar.request_cnt != APAMap_GInfo.lastreqcnt)) {
    s_parking_out_state.flags.cnt_add = TRUE;
  }
  if (TRUE ==
      s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位，则不再更新Obj1、Obj2
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
      if (CurCarCoordinateX > 0) {
        bUpdataObjFlag = FALSE;
      }
    } else {
      if (CurCarCoordinateX > 1) {
        bUpdataObjFlag = FALSE;
      }
    }
  }
  if (TRUE == bUpdataObjFlag) {
    APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo();
  }
  APAMap_ParkingOutCalSlotSlotAlignInfo();
  return TRUE;
}

BOOLEAN APAMap_ParkingOutObliqueRowStairsInfo() {
  APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_DISTANCE_TYPE i;
  APA_ENUM_TYPE k;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType NSegment[127];
  APA_ENUM_TYPE NSegNum;
  APA_ENUM_TYPE DataNum;
  st_MapTopViewFSD* pTopViewInfo;
  APA_DISTANCE_TYPE TopViewPtNum;
  APA_ENUM_TYPE LocStyle;
  UCHAR CurID;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType MainLinYStrPt2, MainLinYEndPt2;
  APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;
  APACoordinateDataCalFloatType MainLinXStrPt3, MainLinXEndPt3;
  APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
  APA_DISTANCE_CAL_FLOAT_TYPE MinOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  uint8_t_INF park_out_mode;
  BOOLEAN bUpdataObliqueRowStairsFlag;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;

  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARKING_OUT) {
    // return FALSE;
  }
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    return FALSE;
  }
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  bUpdataObliqueRowStairsFlag = FALSE;
  if (TRUE == s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }
    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if (CurCarCoordinateX > -3) {
        bUpdataObliqueRowStairsFlag = TRUE;
      }
    } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
      if (CurCarCoordinateX > -0.5) {
        bUpdataObliqueRowStairsFlag = TRUE;
      }
    }
  }

  if (FALSE == bUpdataObliqueRowStairsFlag) {
    return FALSE;
  }
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return FALSE;
  }
  pTopViewInfo = &APAMap_GInputData.TotalMapInfo.mapData.FSDInfo.TopView;
  MaxOffsetX = 2000;
  MinOffsetX = 1000;
  MaxOffsetY = 1000;
  OrgObj2Pt = APAMap_GInfo.SlotPar.Obj2Pt;
  OrgObj1Pt = APAMap_GInfo.SlotPar.Obj1Pt;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);

  MainLinYStrPt2.x = MinOffsetX;
  MainLinYStrPt2.y = 0;
  MainLinYEndPt2.x = MainLinYStrPt2.x;
  MainLinYEndPt2.y = 1000;
  MainLinYStrPt2 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt2, Angle, Pto);
  MainLinYEndPt2 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt2, Angle, Pto);

  // obj2 borderline;
  MainLinXStrPt2 = OrgObj2Pt;
  MainLinXEndPt2 = MainLinXStrPt2;
  LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
  MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);

  // obj2 borderline2;
  MainLinXStrPt3.x = 0;
  MainLinXStrPt3.y = MaxOffsetY;
  MainLinXEndPt3.x = 1000;
  MainLinXEndPt3.y = MainLinXStrPt3.y;
  MainLinXStrPt3 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinXStrPt3, Angle, Pto);
  MainLinXEndPt3 =
      AlgCom_PointPosWithAngAndCenterPt(MainLinXEndPt3, Angle, Pto);

  NSegNum = 0;
  i = 0;
  TopViewPtNum = pTopViewInfo->PointNum;
  while (i < TopViewPtNum) {
    // get fsd data with same id;
    CurID = pTopViewInfo->InfoPoint[i].ID;
    Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.x;
    Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i].Point.y;
    for (k = 1; k < 100; k++) {
      if ((i + k) < pTopViewInfo->PointNum) {
        if (pTopViewInfo->InfoPoint[i + k].ID != CurID) {
          break;
        } else {
          Data[k].x =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
                  .Point.x;
          Data[k].y =
              (APA_DISTANCE_CAL_FLOAT_TYPE)pTopViewInfo->InfoPoint[i + k]
                  .Point.y;
        }
      } else {
        break;
      }
    }
    DataNum = k;
    i += DataNum;
    //----------------------------------
    // Get valid fsd data for fus obj2bordline;
    NSegNum = 0;
    for (k = 0; k < DataNum; k++) {
      TempPt = Data[k];
      LocStyle = AlgCom_GetPointLocationAccordGivenVector(
          &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
      if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
          ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt2, &MainLinYEndPt2, &TempPt, &fDis);
        if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
          if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
            LocStyle = AlgCom_GetPointLocationAccordGivenVector(
                &MainLinXStrPt3, &MainLinXEndPt3, &TempPt, &fDis);
            if (LocStyle != 0) {
              NSegment[NSegNum] = TempPt;
              NSegNum++;
            }
          }
        }
      }
    }
    if (NSegNum >= 3) {
      return TRUE;
    }
  }
  return FALSE;
}
void APAMap_ParkingOutMapScenarioModeCheck(
    BOOLEAN* pCarLeftSideExistSlot, BOOLEAN* pCarRightSideExistSlot,
    BOOLEAN* pCarForwardExistSlot, BOOLEAN* pCarBackwardExistSlot,
    BOOLEAN* pCarLeftSideExistODObj, BOOLEAN* pCarRightSideExistODObj,
    BOOLEAN* pCarForwardExistODObj, BOOLEAN* pCarBackwardExistODObj) {
  st_MapODDataType* vObjInfo;
  APA_ENUM_TYPE vObjNum;
  uint8_t_INF park_out_mode;
  APALineParameterABCType LeftLine;
  APALineParameterABCType RightLine;
  APALineParameterABCType TopLine;
  APALineParameterABCType BottomLine;
  APALineParameterABCType pRectanglRegionLine[4], pRectanglRegionLine1[4];
  APACoordinateDataCalFloatType pPt, tempPt, tempPt1, tempRectanglePt[4],
      tempRectanglePt1[4];
  BOOLEAN bTurnToOppositeScenario, bTurnToOppositeScenario1;
  APA_INDEX_TYPE cPtInAreaNum, cPtInAreaNum1, RectRegionExistODObj,
      RectRegionExistODObj1;
  plf_RefercLineInfo* pRefercLineInfo;
  APA_DISTANCE_TYPE RefercLinePtNum;
  APACoordinateDataCalFloatType pSlotPt, pSlotPt1;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType OrgPt;
  BOOLEAN slot_data_at_right_side;
  BOOLEAN CarLeftSideExistSlot;
  BOOLEAN CarRightSideExistSlot;
  BOOLEAN CarForwardExistSlot;
  BOOLEAN CarBackwardExistSlot;
  BOOLEAN CarLeftSideExistODObj;
  BOOLEAN CarRightSideExistODObj;
  BOOLEAN CarForwardExistODObj;
  BOOLEAN CarBackwardExistODObj;

  CarLeftSideExistSlot = *pCarLeftSideExistSlot;
  CarRightSideExistSlot = *pCarRightSideExistSlot;
  CarForwardExistSlot = *pCarForwardExistSlot;
  CarBackwardExistSlot = *pCarBackwardExistSlot;
  CarLeftSideExistODObj = *pCarLeftSideExistODObj;
  CarRightSideExistODObj = *pCarRightSideExistODObj;
  CarForwardExistODObj = *pCarForwardExistODObj;
  CarBackwardExistODObj = *pCarBackwardExistODObj;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  bTurnToOppositeScenario = FALSE;
  bTurnToOppositeScenario1 = FALSE;
  RectRegionExistODObj = 0;
  RectRegionExistODObj1 = 0;
  cPtInAreaNum = 0;
  cPtInAreaNum1 = 0;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;

  tempPt = APAMap_GInfo.SlotPar.SlotBordPt[0];
  tempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      tempPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
  tempPt1 = APAMap_GInfo.SlotPar.SlotBordPt[1];
  tempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      tempPt1, 0, OrgAng, OrgPt);  // APA转锚点坐标系下

  if (park_out_mode ==
      APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)  // perpendicularparkingoutmode
  {
    if (slot_data_at_right_side == TRUE)  // perp,HeadParkingOut,leftside
    {
      tempRectanglePt[0].x = tempPt.x + 500;
      tempRectanglePt[0].y = tempPt.y + 400;
      tempRectanglePt[1].x = tempPt.x - 1000;
      tempRectanglePt[1].y = tempPt.y + 400;
      tempRectanglePt[2].x = tempPt.x - 1000;
      tempRectanglePt[2].y = tempPt.y - 400;
      tempRectanglePt[3].x = tempPt.x + 500;
      tempRectanglePt[3].y = tempPt.y - 400;

      tempRectanglePt1[0].x = tempPt1.x + 500;
      tempRectanglePt1[0].y = tempPt1.y + 400;
      tempRectanglePt1[1].x = tempPt1.x - 1000;
      tempRectanglePt1[1].y = tempPt1.y + 400;
      tempRectanglePt1[2].x = tempPt1.x - 1000;
      tempRectanglePt1[2].y = tempPt1.y - 400;
      tempRectanglePt1[3].x = tempPt1.x + 500;
      tempRectanglePt1[3].y = tempPt1.y - 400;
    } else {
      tempRectanglePt[0].x = tempPt.x - 500;
      tempRectanglePt[0].y = tempPt.y - 400;
      tempRectanglePt[1].x = tempPt.x + 1000;
      tempRectanglePt[1].y = tempPt.y - 400;
      tempRectanglePt[2].x = tempPt.x + 1000;
      tempRectanglePt[2].y = tempPt.y + 400;
      tempRectanglePt[3].x = tempPt.x - 500;
      tempRectanglePt[3].y = tempPt.y + 400;

      tempRectanglePt1[0].x = tempPt1.x - 500;
      tempRectanglePt1[0].y = tempPt1.y - 400;
      tempRectanglePt1[1].x = tempPt1.x + 1000;
      tempRectanglePt1[1].y = tempPt1.y + 400;
      tempRectanglePt1[2].x = tempPt1.x + 1000;
      tempRectanglePt1[2].y = tempPt1.y + 400;
      tempRectanglePt1[3].x = tempPt1.x - 500;
      tempRectanglePt1[3].y = tempPt1.y + 400;
    }
  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
    if (slot_data_at_right_side == TRUE)  // perp rear parkingout turn left
    {
      tempRectanglePt[0].x = tempPt.x - 500;
      tempRectanglePt[0].y = tempPt.y - 400;
      tempRectanglePt[1].x = tempPt.x + 1000;
      tempRectanglePt[1].y = tempPt.y - 400;
      tempRectanglePt[2].x = tempPt.x + 1000;
      tempRectanglePt[2].y = tempPt.y + 400;
      tempRectanglePt[3].x = tempPt.x - 500;
      tempRectanglePt[3].y = tempPt.y + 400;

      tempRectanglePt1[0].x = tempPt1.x - 500;
      tempRectanglePt1[0].y = tempPt1.y - 400;
      tempRectanglePt1[1].x = tempPt1.x + 1000;
      tempRectanglePt1[1].y = tempPt1.y - 400;
      tempRectanglePt1[2].x = tempPt1.x + 1000;
      tempRectanglePt1[2].y = tempPt1.y + 400;
      tempRectanglePt1[3].x = tempPt1.x - 500;
      tempRectanglePt1[3].y = tempPt1.y + 400;
    } else {
      tempRectanglePt[0].x = tempPt.x - 500;
      tempRectanglePt[0].y = tempPt.y - 400;
      tempRectanglePt[1].x = tempPt.x + 1000;
      tempRectanglePt[1].y = tempPt.y - 400;
      tempRectanglePt[2].x = tempPt.x + 1000;
      tempRectanglePt[2].y = tempPt.y + 400;
      tempRectanglePt[3].x = tempPt.x - 500;
      tempRectanglePt[3].y = tempPt.y + 400;

      tempRectanglePt1[0].x = tempPt1.x - 500;
      tempRectanglePt1[0].y = tempPt1.y - 400;
      tempRectanglePt1[1].x = tempPt1.x + 1000;
      tempRectanglePt1[1].y = tempPt1.y + 400;
      tempRectanglePt1[2].x = tempPt1.x + 1000;
      tempRectanglePt1[2].y = tempPt1.y + 400;
      tempRectanglePt1[3].x = tempPt1.x - 500;
      tempRectanglePt1[3].y = tempPt1.y + 400;
    }

  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == TRUE)  // parall parkingout turn left
    {
      tempRectanglePt[0].x = tempPt.x - 500;
      tempRectanglePt[0].y = tempPt.y - 400;
      tempRectanglePt[1].x = tempPt.x + 1000;
      tempRectanglePt[1].y = tempPt.y - 400;
      tempRectanglePt[2].x = tempPt.x + 1000;
      tempRectanglePt[2].y = tempPt.y + 400;
      tempRectanglePt[3].x = tempPt.x - 500;
      tempRectanglePt[3].y = tempPt.y + 400;

      tempRectanglePt1[0].x = tempPt1.x - 500;
      tempRectanglePt1[0].y = tempPt1.y - 400;
      tempRectanglePt1[1].x = tempPt1.x + 1000;
      tempRectanglePt1[1].y = tempPt1.y - 400;
      tempRectanglePt1[2].x = tempPt1.x + 1000;
      tempRectanglePt1[2].y = tempPt1.y + 400;
      tempRectanglePt1[3].x = tempPt1.x - 500;
      tempRectanglePt1[3].y = tempPt1.y + 400;
    } else {
      tempRectanglePt[0].x = tempPt.x + 500;
      tempRectanglePt[0].y = tempPt.y - 400;
      tempRectanglePt[1].x = tempPt.x - 1000;
      tempRectanglePt[1].y = tempPt.y - 400;
      tempRectanglePt[2].x = tempPt.x - 1000;
      tempRectanglePt[2].y = tempPt.y + 400;
      tempRectanglePt[3].x = tempPt.x + 500;
      tempRectanglePt[3].y = tempPt.y + 400;

      tempRectanglePt1[0].x = tempPt1.x + 500;
      tempRectanglePt1[0].y = tempPt1.y - 400;
      tempRectanglePt1[1].x = tempPt1.x - 1000;
      tempRectanglePt1[1].y = tempPt1.y - 400;
      tempRectanglePt1[2].x = tempPt1.x - 1000;
      tempRectanglePt1[2].y = tempPt1.y + 400;
      tempRectanglePt1[3].x = tempPt1.x + 500;
      tempRectanglePt1[3].y = tempPt1.y + 400;
    }
  } else {
    return;
  }

  // get left rectangle 4 line
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt[0], tempRectanglePt[1],
                               &LeftLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt[1], tempRectanglePt[2],
                               &BottomLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt[2], tempRectanglePt[3],
                               &RightLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt[3], tempRectanglePt[1],
                               &TopLine);
  pRectanglRegionLine[0] = TopLine;
  pRectanglRegionLine[1] = BottomLine;
  pRectanglRegionLine[2] = LeftLine;
  pRectanglRegionLine[3] = RightLine;
  // get Right rectangle 4 line
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[0], tempRectanglePt1[1],
                               &LeftLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[1], tempRectanglePt1[2],
                               &BottomLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[2], tempRectanglePt1[3],
                               &RightLine);
  AlgCom_LineParABCbyTwoPoints(tempRectanglePt1[3], tempRectanglePt1[1],
                               &TopLine);
  pRectanglRegionLine1[0] = TopLine;
  pRectanglRegionLine1[1] = BottomLine;
  pRectanglRegionLine1[2] = LeftLine;
  pRectanglRegionLine1[3] = RightLine;

  vObjInfo = &APAMap_GInputData.TotalMapInfo.mapData
                  .ODInfo;  // getODobj,CheckRegionExistODObj
  vObjNum = (APA_ENUM_TYPE)vObjInfo->Square.ObjNum;
  if (vObjNum > 0) {
    RectRegionExistODObj = 0;
    RectRegionExistODObj1 = 0;
    for (uint16_t i = 0; i < vObjNum; i++) {
      if ((vObjInfo->Square.Quadrilaterals[i].ObjInfo.Label >=
           Obj_Label_Pedestrian) &&
          (vObjInfo->Square.Quadrilaterals[i].ObjInfo.Label <=
           Obj_Label_WarningTriangle)) {
        cPtInAreaNum = 0;
        cPtInAreaNum1 = 0;
        pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_1.x);
        pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_1.y);
        pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        bTurnToOppositeScenario =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
        if (bTurnToOppositeScenario == TRUE) {
          cPtInAreaNum++;
        }
        bTurnToOppositeScenario1 =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
        if (bTurnToOppositeScenario1 == TRUE) {
          cPtInAreaNum1++;
        }
        pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_2.x);
        pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_2.y);
        pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        bTurnToOppositeScenario =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
        if (bTurnToOppositeScenario == TRUE) {
          cPtInAreaNum++;
        }
        bTurnToOppositeScenario1 =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
        if (bTurnToOppositeScenario1 == TRUE) {
          cPtInAreaNum1++;
        }
        pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_3.x);
        pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_3.y);
        pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        bTurnToOppositeScenario =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
        if (bTurnToOppositeScenario == TRUE) {
          cPtInAreaNum++;
        }
        bTurnToOppositeScenario1 =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
        if (bTurnToOppositeScenario1 == TRUE) {
          cPtInAreaNum1++;
        }
        pPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_4.x);
        pPt.y = (APA_DISTANCE_CAL_FLOAT_TYPE)(vObjInfo->Square.Quadrilaterals[i]
                                                  .Points.Point_4.y);
        pPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下
        bTurnToOppositeScenario =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine);
        if (bTurnToOppositeScenario == TRUE) {
          cPtInAreaNum++;
        }
        bTurnToOppositeScenario1 =
            AlgCom_CheckIfGivenPtIntheRectRegion(&pPt, pRectanglRegionLine1);
        if (bTurnToOppositeScenario1 == TRUE) {
          cPtInAreaNum1++;
        }
        if (cPtInAreaNum > 1) {
          RectRegionExistODObj++;
        }
        if (cPtInAreaNum1 > 1) {
          RectRegionExistODObj1++;
        }
      }
      if (RectRegionExistODObj >= 3) {
        if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
          if (slot_data_at_right_side == FALSE) {
            CarLeftSideExistODObj = TRUE;
          } else {
            CarRightSideExistODObj = TRUE;
          }
        } else if (park_out_mode ==
                   APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
          if (slot_data_at_right_side == FALSE) {
            CarLeftSideExistODObj = TRUE;
          } else {
            CarRightSideExistODObj = TRUE;
          }
        } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
          if (slot_data_at_right_side == TRUE)  // parall parkingout turn left
          {
            CarForwardExistODObj = TRUE;
          } else {
            CarBackwardExistODObj = TRUE;
          }
        }
      } else {
        // CarLeftSideExistODObj = FALSE;
      }
      if (RectRegionExistODObj1 >= 3) {
        if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
          if (slot_data_at_right_side == FALSE) {
            CarRightSideExistODObj = TRUE;
          } else {
            CarLeftSideExistODObj = TRUE;
          }
        } else if (park_out_mode ==
                   APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
          if (slot_data_at_right_side == FALSE) {
            CarRightSideExistODObj = TRUE;
          } else {
            CarLeftSideExistODObj = TRUE;
          }
        } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
          if (slot_data_at_right_side == TRUE)  // parall parkingout turn left
          {
            CarBackwardExistODObj = TRUE;
          } else {
            CarForwardExistODObj = TRUE;
          }
        }
      } else {
        // CarRightSideExistODObj = FALSE;
      }
    }
  }

  pRefercLineInfo = &APAMap_GInputData.TotalMapInfo.mapData
                         .RefercLineInfo;  // getslotcloselinetwopoint
  RefercLinePtNum = pRefercLineInfo->RefercLineTotalNum;
  if (RefercLinePtNum > 0) {
    for (uint16_t i = 0; i < RefercLinePtNum; i++) {
      pSlotPt.x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fx;
      pSlotPt.y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt1.fy;
      pSlotPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pSlotPt, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
      pSlotPt1.x =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fx;
      pSlotPt1.y =
          (APA_DISTANCE_CAL_FLOAT_TYPE)pRefercLineInfo->stLeftRefercLineInfo
              .stRefercLineParam[i]
              .pt2.fy;
      pSlotPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pSlotPt1, 0, CurCarPos.CarAng, CurCarPos.Coordinate);
      if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
          (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
        if ((pSlotPt.x <= CurCarPos.Coordinate.x) &&
            (pSlotPt1.x <= CurCarPos.Coordinate.x) &&
            (pSlotPt.x > CurCarPos.Coordinate.x - 4500) &&
            (pSlotPt1.x > CurCarPos.Coordinate.x - 4500) &&
            (pSlotPt.y <= CurCarPos.Coordinate.y + 5000) &&
            (pSlotPt1.y <= CurCarPos.Coordinate.y + 5000) &&
            (pSlotPt.y > CurCarPos.Coordinate.y) &&
            (pSlotPt1.y > CurCarPos.Coordinate.y) &&
            (pSlotPt.x != pSlotPt1.x))  // checkthecarleftsideexistslot
        {
          CarLeftSideExistSlot = TRUE;
        } else {
          // CarLeftSideExistSlot = FALSE;
        }
        if ((pSlotPt.x >= CurCarPos.Coordinate.x) &&
            (pSlotPt1.x >= CurCarPos.Coordinate.x) &&
            (pSlotPt.x < CurCarPos.Coordinate.x + 4500) &&
            (pSlotPt1.x < CurCarPos.Coordinate.x + 4500) &&
            (pSlotPt.y <= CurCarPos.Coordinate.y + 5000) &&
            (pSlotPt1.y <= CurCarPos.Coordinate.y + 5000) &&
            (pSlotPt.y > CurCarPos.Coordinate.y) &&
            (pSlotPt1.y > CurCarPos.Coordinate.y) &&
            (pSlotPt.x != pSlotPt1.x))  // checkthecarRightsideexistslot
        {
          CarRightSideExistSlot = TRUE;
        } else {
          // CarRightSideExistSlot = FALSE;
        }
      } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
        if ((pSlotPt.y <= (APAMap_ComCfg.LengthOfCar + 9000.0)) &&
            (pSlotPt1.y <= (APAMap_ComCfg.LengthOfCar + 9000.0)) &&
            (pSlotPt.y > APAMap_ComCfg.LengthOfCar) &&
            (pSlotPt1.y > APAMap_ComCfg.LengthOfCar) && (pSlotPt.x > -2500.0) &&
            (pSlotPt1.x > -2500.0) && (pSlotPt.x <= 2500.0) &&
            (pSlotPt1.x <= 2500.0)) {
          CarForwardExistSlot = TRUE;
        } else {
        }
        if ((pSlotPt.y <= (-APAMap_ComCfg.LenBetweenRAxisAndRBumper)) &&
            (pSlotPt1.y <= (-APAMap_ComCfg.LenBetweenRAxisAndRBumper)) &&
            (pSlotPt.y > (-APAMap_ComCfg.LenBetweenRAxisAndRBumper - 9000.0)) &&
            (pSlotPt1.y >
             (-APAMap_ComCfg.LenBetweenRAxisAndRBumper - 9000.0)) &&
            (pSlotPt.x > -2500.0) && (pSlotPt1.x > -2500.0) &&
            (pSlotPt.x <= 2500.0) && (pSlotPt1.x <= 2500.0)) {
          CarBackwardExistSlot = TRUE;
        } else {
        }
      }
    }
  } else {
    CarLeftSideExistSlot = FALSE;
    CarRightSideExistSlot = FALSE;
    CarForwardExistSlot = FALSE;
    CarBackwardExistSlot = FALSE;
    CarLeftSideExistODObj = FALSE;
    CarRightSideExistODObj = FALSE;
    CarForwardExistODObj = FALSE;
    CarBackwardExistODObj = FALSE;
  }
  *pCarLeftSideExistSlot = CarLeftSideExistSlot;
  *pCarRightSideExistSlot = CarRightSideExistSlot;
  *pCarForwardExistSlot = CarForwardExistSlot;
  *pCarBackwardExistSlot = CarBackwardExistSlot;
  *pCarLeftSideExistODObj = CarLeftSideExistODObj;
  *pCarRightSideExistODObj = CarRightSideExistODObj;
  *pCarForwardExistODObj = CarForwardExistODObj;
  *pCarBackwardExistODObj = CarBackwardExistODObj;
  return;
}

void APAMap_ParkingOutElectrFenceMapBulid(
    APACoordinateDataCalFloatType* pgetVPLSlotData,
    APACoordinateDataCalFloatType* pgetObj1Pt,
    APACoordinateDataCalFloatType* pgetObj2Pt,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCarSideToObj1LineDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCarSideToObj2LineDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCurSlotTopLineAngle,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCurSlotCloseLineAngle,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj1MoveDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj2MoveDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCloseLineMoveDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pSlotOutsideDis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pSlotInnerDis) {
  uint8_t_INF park_out_mode;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType pPtcd, pPtb, pPta, pPth, pPtg, pPtgf, pPtfg,
      pPtf, pPte, pPtdc, tempPt, tempPt1;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng, vDltAngle;
  APACoordinateDataCalFloatType OrgPt;
  APACoordinateDataCalFloatType tempSlotPt[4];
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis1, CloseLineMoveDis2;
  APA_DISTANCE_CAL_FLOAT_TYPE ObjLabelAngledObj1SafeDis,
      ObjLabelAngledObj2SafeDis;
  APA_DISTANCE_CAL_FLOAT_TYPE ObjParallelObj1SafeDis;
  APA_DISTANCE_CAL_FLOAT_TYPE ObjParallelObj2SafeDis;
  BOOLEAN CarLeftSideExistSlot;
  BOOLEAN CarRightSideExistSlot;
  BOOLEAN CarForwardExistSlot;
  BOOLEAN CarBackwardExistSlot;
  BOOLEAN CarLeftSideExistODObj;
  BOOLEAN CarRightSideExistODObj;
  BOOLEAN CarForwardExistODObj;
  BOOLEAN CarBackwardExistODObj;
  BOOLEAN ParkingOutClockwise;
  BOOLEAN CurSlotIsAngle;
  APACoordinateDataCalFloatType getVPLSlotData[SlotPtNum];
  uint8_t_INF i;
  APACoordinateDataCalFloatType getObj1Pt;
  APACoordinateDataCalFloatType getObj2Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj1LineDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj2LineDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CurSlotTopLineAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE CurSlotCloseLineAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1MoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2MoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE SlotOutsideDis;
  APA_DISTANCE_CAL_FLOAT_TYPE SlotInnerDis;
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
  APA_DISTANCE_CAL_FLOAT_TYPE FOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE BOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE LOffset;
  APA_DISTANCE_CAL_FLOAT_TYPE ROffset;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACoordinateDataCalFloatType Data[100];
  APA_DISTANCE_TYPE ODNSegNum;
  st_MapODDataType* pODInfo;
  Obj_Information_t CurObjComInfo;
  APA_ENUM_TYPE DataNum;
  uint8_t_INF k;
#endif

  for (i = 0; i < SlotPtNum; i++) {
    getVPLSlotData[i] = pgetVPLSlotData[i];
  }
  getObj1Pt = *pgetObj1Pt;
  getObj2Pt = *pgetObj2Pt;
  CarSideToObj1LineDis = *pCarSideToObj1LineDis;
  CarSideToObj2LineDis = *pCarSideToObj2LineDis;
  CurSlotTopLineAngle = *pCurSlotTopLineAngle;
  CurSlotCloseLineAngle = *pCurSlotCloseLineAngle;
  Obj1MoveDis = *pObj1MoveDis;
  Obj2MoveDis = *pObj2MoveDis;
  CloseLineMoveDis = *pCloseLineMoveDis;
  SlotOutsideDis = *pSlotOutsideDis;
  SlotInnerDis = *pSlotInnerDis;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  OrgAng = APAMap_GInfo.NewCordSysAng;
  OrgPt = APAMap_GInfo.NewCordSysOPt;
  tempPt = getObj1Pt;  //(0,0)
  tempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      tempPt, 0, OrgAng, OrgPt);  // APA转锚点坐标系下//(0,0)
  tempPt1 = getObj2Pt;            //(0,-2730)
  tempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      tempPt1, 0, OrgAng, OrgPt);  // APA转锚点坐标系下//(0,-2730)

  CarLeftSideExistSlot = FALSE;
  CarRightSideExistSlot = FALSE;
  CarForwardExistSlot = FALSE;
  CarBackwardExistSlot = FALSE;
  CarLeftSideExistODObj = FALSE;
  CarRightSideExistODObj = FALSE;
  CarForwardExistODObj = FALSE;
  CarBackwardExistODObj = FALSE;
  ParkingOutClockwise = FALSE;
  CurSlotIsAngle = FALSE;
  CloseLineMoveDis1 = 0.0;
  CloseLineMoveDis2 = 0.0;
  ObjLabelAngledObj1SafeDis = 300.0;
  ObjLabelAngledObj2SafeDis = 300.0;
  ObjParallelObj1SafeDis = 1800;
  ObjParallelObj2SafeDis = 1500;  // 900;
#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
  pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
  i = 0;
  k = 0;
  ODNSegNum = 0;
  DataNum = 0;
  while (i < pODInfo->Square.ObjNum) {
    CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
    if ((CurObjComInfo.Label == Obj_Label_WarningPost) ||
        (CurObjComInfo.Label == Obj_Label_ConeBucket) ||
        (CurObjComInfo.Label == Obj_Label_SquareColumn) ||
        (CurObjComInfo.Label == Obj_Label_TwoWheelsVehicle) ||
        (CurObjComInfo.Label == Obj_Label_NoParkingSign) ||
        (CurObjComInfo.Label == Obj_Label_UPILLAR) ||
        (CurObjComInfo.Label == Obj_Label_Stone_Piers) ||
        (CurObjComInfo.Label == Obj_Label_WheelChock)) {
      Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_1.x;
      Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_1.y;
      Data[1].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_2.x;
      Data[1].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_2.y;
      Data[2].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_3.x;
      Data[2].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_3.y;
      Data[3].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_4.x;
      Data[3].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                      .Points.Point_4.y;
      DataNum = 4;
    } else if (CurObjComInfo.Label == Obj_Label_Curb) {
      for (k = 0; k < pODInfo->Polygon.Polygons[i].Points.PointNum; k++) {
        Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                        .Points.Point[k]
                        .x;
        Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Polygon.Polygons[i]
                        .Points.Point[k]
                        .y;
      }
      DataNum = pODInfo->Polygon.Polygons[i].Points.PointNum;
    } else {
      DataNum = 0;
    }
    if (DataNum != 0) {
      FOffset = 0;
      BOffset = 400;
      LOffset = 400;
      ROffset = 0;
      APAMap_GetCarRectArea(FOffset, BOffset, LOffset, ROffset, CurCarPos,
                            pRectPt, pRectLine);
      if (TRUE == APAMap_CheckIfObjWithinRectArea(0x01, &Data[0], DataNum,
                                                  pRectPt, pRectLine)) {
        ODNSegNum++;
        break;
      }
    }
    i++;
  }
  if (ODNSegNum != 0) {
    ObjParallelObj2SafeDis = 1800;
  }
#endif
  if (TRUE == s_parking_out_state.flags.od_wheel_chock) {
    ObjParallelObj2SafeDis = 2700;
  }
  for (i = 0; i < 4; i++) {
    tempSlotPt[i] = getVPLSlotData[i];
    tempSlotPt[i] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        tempSlotPt[i], 0, CurCarPos.CarAng, CurCarPos.Coordinate);
  }
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
    if (slot_data_at_right_side == TRUE) {
      if (tempSlotPt[3].y > tempSlotPt[0].y + 200) {
        ParkingOutClockwise = TRUE;
      } else if (tempSlotPt[0].y > tempSlotPt[3].y + 200) {
        ParkingOutClockwise = FALSE;
      }
    } else {
      if (tempSlotPt[0].y > tempSlotPt[3].y + 200) {
        ParkingOutClockwise = TRUE;
      } else if (tempSlotPt[3].y > tempSlotPt[0].y + 200) {
        ParkingOutClockwise = FALSE;
      }
    }
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    if (slot_data_at_right_side == TRUE) {
      if (tempSlotPt[1].y > tempSlotPt[2].y - 200) {
        ParkingOutClockwise = TRUE;
      } else if (tempSlotPt[1].y < tempSlotPt[2].y - 200) {
        ParkingOutClockwise = FALSE;
      }
    } else {
      if (tempSlotPt[1].y > tempSlotPt[2].y - 200) {
        ParkingOutClockwise = FALSE;
      } else if (tempSlotPt[1].y < tempSlotPt[2].y - 200) {
        ParkingOutClockwise = TRUE;
      }
    }
  } else {
    /* code */
  }

  vDltAngle = CurSlotTopLineAngle - CurSlotCloseLineAngle;
  if ((MATH_FABS(vDltAngle) > (100 * PI / 180.0)) ||
      (MATH_FABS(vDltAngle) < (80 * PI / 180.0))) {
    CurSlotIsAngle = TRUE;
  }

  APAMap_ParkingOutMapScenarioModeCheck(
      &CarLeftSideExistSlot, &CarRightSideExistSlot, &CarForwardExistSlot,
      &CarBackwardExistSlot, &CarLeftSideExistODObj, &CarRightSideExistODObj,
      &CarForwardExistODObj, &CarBackwardExistODObj);
  if ((park_out_mode ==
       APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)  // perpendicularparkingoutmode
      || (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
    if (slot_data_at_right_side == FALSE)  // perp,HeadParkingOut,leftside
    {
      pPtcd.x = tempPt.x;
      pPtcd.y = tempPt.y + 300;
      pPtdc.x = tempPt.x;
      pPtdc.y = tempPt.y + 300;

      pPtgf.x = tempPt1.x;
      pPtgf.y = tempPt1.y - 300;
      pPtfg.x = tempPt1.x;
      pPtfg.y = tempPt1.y - 300;

      Obj1MoveDis = 300.0;
      Obj2MoveDis = 300.0;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt.x - 500;
        pPtcd.y = tempPt.y + 500;
        pPtdc.x = tempPt.x - 500;
        pPtdc.y = tempPt.y + 500;

        Obj1MoveDis = 500.0;
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarLeftSideExistSlot == FALSE) &&
                 (CarLeftSideExistODObj == FALSE)) {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + 300;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = 0.0;
      } else if ((CarLeftSideExistSlot == TRUE) &&
                 (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt.x - 1000;
        pPtcd.y = tempPt.y + 1000;
        pPtdc.x = tempPt.x - 1000;
        pPtdc.y = tempPt.y + 1000;

        Obj1MoveDis = 1000.0;
        CloseLineMoveDis1 = -1000.0;
      } else {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + 300;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = 0.0;
      }
      if ((CarRightSideExistSlot == FALSE) &&
          (CarRightSideExistODObj == TRUE)) {
        pPtgf.x = tempPt1.x - 500;
        pPtgf.y = tempPt1.y - 500;
        pPtfg.x = tempPt1.x - 500;
        pPtfg.y = tempPt1.y - 500;

        Obj2MoveDis = 500.0;
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarRightSideExistSlot == FALSE) &&
                 (CarRightSideExistODObj == FALSE)) {
        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - 300;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = 0.0;
      } else if ((CarRightSideExistSlot == TRUE) &&
                 (CarRightSideExistODObj == TRUE)) {
        pPtgf.x = tempPt1.x - 1000;
        pPtgf.y = tempPt1.y - 1000;
        pPtfg.x = tempPt1.x - 1000;
        pPtfg.y = tempPt1.y - 1000;

        Obj2MoveDis = 1000.0;
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - 300;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = 0.0;
      }
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt.y + 2000;
      pPta.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
      pPta.y = tempPt.y + 2000;
      pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
      pPth.y = tempPt1.y - 2000;
      pPtg.x = pPtfg.x;
      pPtg.y = tempPt1.y - 2000;
      pPtf.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
      pPtf.y = pPtfg.y;
      pPte.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
      pPte.y = pPtdc.y;

      SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
      SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
    } else  // perp,HeadParkingOut,rightside
    {
      pPtcd.x = tempPt1.x;
      pPtcd.y = tempPt1.y - 300;
      pPtdc.x = tempPt1.x;
      pPtdc.y = tempPt1.y - 300;
      pPtfg.x = tempPt.x;
      pPtfg.y = tempPt.y + 300;
      pPtgf.x = tempPt.x;
      pPtgf.y = tempPt.y + 300;

      Obj1MoveDis = 300.0;
      Obj2MoveDis = 300.0;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x + 500;
        pPtcd.y = tempPt1.y - 500;
        pPtdc.x = tempPt1.x + 500;
        pPtdc.y = tempPt1.y - 500;

        Obj2MoveDis = 500.0;
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarLeftSideExistSlot == FALSE) &&
                 (CarLeftSideExistODObj == FALSE)) {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - 300;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = (0.0);
      } else if ((CarLeftSideExistSlot == TRUE) &&
                 (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x + 1000;
        pPtcd.y = tempPt1.y - 1000;
        pPtdc.x = tempPt1.x + 1000;
        pPtdc.y = tempPt1.y - 1000;

        Obj2MoveDis = 1000.0;
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - 300;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = (0.0);
      }
      if ((CarRightSideExistSlot == FALSE) &&
          (CarRightSideExistODObj == TRUE)) {
        pPtfg.x = tempPt.x + 500;
        pPtfg.y = tempPt.y + 500;
        pPtgf.x = tempPt.x + 500;
        pPtgf.y = tempPt.y + 500;

        Obj1MoveDis = 500.0;
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarRightSideExistSlot == FALSE) &&
                 (CarRightSideExistODObj == FALSE)) {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + 300;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = (0.0);
      } else if ((CarRightSideExistSlot == TRUE) &&
                 (CarRightSideExistODObj == TRUE)) {
        pPtfg.x = tempPt.x + 1000;
        pPtfg.y = tempPt.y + 1000;
        pPtgf.x = tempPt.x + 1000;
        pPtgf.y = tempPt.y + 1000;

        Obj1MoveDis = 1000.0;
        CloseLineMoveDis1 = (-1000.0);
      } else {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + 300;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = (0.0);
      }
      pPta.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 500;
      pPta.y = tempPt1.y - 2000;
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt1.y - 2000;
      pPte.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
      pPte.y = pPtdc.y;
      pPtf.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
      pPtf.y = pPtfg.y;
      pPtg.x = pPtgf.x;
      pPtg.y = tempPt.y + 2000;
      pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
      pPth.y = tempPt.y + 200;

      SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
      SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
    }
    if (CurSlotIsAngle == TRUE) {
      if (slot_data_at_right_side == FALSE)  // Perp AnlgSlot Leftside
      {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + ObjLabelAngledObj2SafeDis;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + ObjLabelAngledObj2SafeDis;

        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - ObjLabelAngledObj1SafeDis;

        Obj1MoveDis = ObjLabelAngledObj1SafeDis;
        Obj2MoveDis = ObjLabelAngledObj2SafeDis;
        CloseLineMoveDis1 = 0.0;
        CloseLineMoveDis2 = 0.0;
        if ((CarLeftSideExistSlot == FALSE) &&
            (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt.x - 500;
          pPtcd.y = tempPt.y + 500;
          pPtdc.x = tempPt.x - 500;
          pPtdc.y = tempPt.y + 500;

          Obj1MoveDis = 500.0;
          CloseLineMoveDis1 = (-500.0);
        } else if ((CarLeftSideExistSlot == FALSE) &&
                   (CarLeftSideExistODObj == FALSE)) {
          pPtcd.x = tempPt.x;
          pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtdc.x = tempPt.x;
          pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        } else if ((CarLeftSideExistSlot == TRUE) &&
                   (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt.x - 1000;
          pPtcd.y = tempPt.y + 1000;
          pPtdc.x = tempPt.x - 1000;
          pPtdc.y = tempPt.y + 1000;

          Obj1MoveDis = 1000.0;
          CloseLineMoveDis1 = (-1000.0);
        } else {
          pPtcd.x = tempPt.x;
          pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtdc.x = tempPt.x;
          pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        }
        if ((CarRightSideExistSlot == FALSE) &&
            (CarRightSideExistODObj == TRUE)) {
          pPtgf.x = tempPt1.x - 500;
          pPtgf.y = tempPt1.y - 500;
          pPtfg.x = tempPt1.x - 500;
          pPtfg.y = tempPt1.y - 500;

          Obj2MoveDis = 500.0;
          CloseLineMoveDis2 = (-50.0);
        } else if ((CarRightSideExistSlot == FALSE) &&
                   (CarRightSideExistODObj == FALSE)) {
          pPtgf.x = tempPt1.x;
          pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtfg.x = tempPt1.x;
          pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        } else if ((CarRightSideExistSlot == TRUE) &&
                   (CarRightSideExistODObj == TRUE)) {
          pPtgf.x = tempPt1.x - 1000;
          pPtgf.y = tempPt1.y - 1000;
          pPtfg.x = tempPt1.x - 1000;
          pPtfg.y = tempPt1.y - 1000;

          Obj2MoveDis = 1000.0;
          CloseLineMoveDis2 = (-1000.0);
        } else {
          pPtgf.x = tempPt1.x;
          pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtfg.x = tempPt1.x;
          pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        }
        if (ParkingOutClockwise == TRUE) {
          pPta.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
          pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
          pPth.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
          pPth.y = tempPt1.y - 2000;

          SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
        } else {
          pPta.x = tempPt.x + 7000;
          pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
          pPth.x = tempPt1.x + 7000;
          pPth.y = tempPt1.y - 2000;

          SlotOutsideDis = (7000.0);
        }
        pPtb.x = pPtdc.x;
        pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
        pPtg.x = pPtfg.x;
        pPtg.y = tempPt1.y - 2000;
        pPtf.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
        pPtf.y = pPtfg.y;
        pPte.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
        pPte.y = pPtdc.y;

        SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
      } else {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjLabelAngledObj2SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjLabelAngledObj2SafeDis;

        Obj1MoveDis = ObjLabelAngledObj1SafeDis;
        Obj2MoveDis = ObjLabelAngledObj2SafeDis;
        CloseLineMoveDis1 = 0.0;
        CloseLineMoveDis2 = 0.0;
        if ((CarLeftSideExistSlot == FALSE) &&
            (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt1.x + 500;
          pPtcd.y = tempPt1.y - 500;
          pPtdc.x = tempPt1.x + 500;
          pPtdc.y = tempPt1.y - 500;

          Obj2MoveDis = 500.0;
          CloseLineMoveDis2 = (-500.0);
        } else if ((CarLeftSideExistSlot == FALSE) &&
                   (CarLeftSideExistODObj == FALSE)) {
          pPtcd.x = tempPt1.x;
          pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtdc.x = tempPt1.x;
          pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        } else if ((CarLeftSideExistSlot == TRUE) &&
                   (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt1.x + 1000;
          pPtcd.y = tempPt1.y - 1000;
          pPtdc.x = tempPt1.x + 1000;
          pPtdc.y = tempPt1.y - 1000;

          Obj2MoveDis = 1000.0;
          CloseLineMoveDis2 = (-1000.0);
        } else {
          pPtcd.x = tempPt1.x;
          pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtdc.x = tempPt1.x;
          pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        }
        if ((CarRightSideExistSlot == FALSE) &&
            (CarRightSideExistODObj == TRUE)) {
          pPtfg.x = tempPt.x + 500;
          pPtfg.y = tempPt.y + 500;
          pPtgf.x = tempPt.x + 500;
          pPtgf.y = tempPt.y + 500;

          Obj1MoveDis = 500.0;
          CloseLineMoveDis1 = (-500.0);
        } else if ((CarRightSideExistSlot == FALSE) &&
                   (CarRightSideExistODObj == FALSE)) {
          pPtfg.x = tempPt.x;
          pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtgf.x = tempPt.x;
          pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        } else if ((CarRightSideExistSlot == TRUE) &&
                   (CarRightSideExistODObj == TRUE)) {
          pPtfg.x = tempPt.x + 1000;
          pPtfg.y = tempPt.y + 1000;
          pPtgf.x = tempPt.x + 1000;
          pPtgf.y = tempPt.y + 1000;

          Obj1MoveDis = 1000.0;
          CloseLineMoveDis1 = (-1000.0);
        } else {
          pPtfg.x = tempPt.x;
          pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtgf.x = tempPt.x;
          pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        }
        if (ParkingOutClockwise == TRUE) {
          pPta.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
          pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
          pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
          pPth.y = tempPt.y + 2000;

          SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
        } else {
          pPta.x = tempPt1.x - 7000;
          pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
          pPth.x = tempPt.x - 7000;
          pPth.y = tempPt.y + 2000;

          SlotOutsideDis = (7000.0);
        }
        pPtb.x = pPtdc.x;
        pPtb.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
        pPtg.x = pPtfg.x;
        pPtg.y = tempPt.y + 2000;
        pPtf.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
        pPtf.y = pPtfg.y;
        pPte.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
        pPte.y = pPtdc.y;

        SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
      }
    }
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT)) {
    if (slot_data_at_right_side == FALSE)  // perp,RearParkingOut,leftside
    {
      pPtcd.x = tempPt.x;
      pPtcd.y = tempPt.y + 300;
      pPtdc.x = tempPt.x;
      pPtdc.y = tempPt.y + 300;

      pPtgf.x = tempPt1.x;
      pPtgf.y = tempPt1.y - 300;
      pPtfg.x = tempPt1.x;
      pPtfg.y = tempPt1.y - 300;

      Obj1MoveDis = 300.0;
      Obj2MoveDis = 300.0;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarRightSideExistSlot == FALSE) &&
          (CarRightSideExistODObj == TRUE)) {
        pPtgf.x = tempPt1.x + 500;
        pPtgf.y = tempPt1.y - 500;
        pPtfg.x = tempPt1.x + 500;
        pPtfg.y = tempPt1.y - 500;

        Obj2MoveDis = 500.0;
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarRightSideExistSlot == FALSE) &&
                 (CarRightSideExistODObj == FALSE)) {
        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - 300;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = (0.0);
      } else if ((CarRightSideExistSlot == TRUE) &&
                 (CarRightSideExistODObj == TRUE)) {
        pPtgf.x = tempPt1.x + 1000;
        pPtgf.y = tempPt1.y - 1000;
        pPtfg.x = tempPt1.x + 1000;
        pPtfg.y = tempPt1.y - 1000;

        Obj2MoveDis = 1000.0;
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - 300;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - 300;

        Obj2MoveDis = 300.0;
        CloseLineMoveDis2 = (0.0);
      }
      if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt.x + 500;
        pPtcd.y = tempPt.y + 500;
        pPtdc.x = tempPt.x + 500;
        pPtdc.y = tempPt.y + 500;

        Obj1MoveDis = 500.0;
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarLeftSideExistSlot == FALSE) &&
                 (CarLeftSideExistODObj == FALSE)) {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + 300;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = (0.0);
      } else if ((CarLeftSideExistSlot == TRUE) &&
                 (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt.x + 1000;
        pPtcd.y = tempPt.y + 1000;
        pPtdc.x = tempPt.x + 1000;
        pPtdc.y = tempPt.y + 1000;

        Obj1MoveDis = 1000.0;
        CloseLineMoveDis1 = (-1000.0);
      } else {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + 300;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + 300;

        Obj1MoveDis = 300.0;
        CloseLineMoveDis1 = (0.0);
      }
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt.y + 2000;
      pPta.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
      pPta.y = tempPt.y + 2000;
      pPth.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 500;
      pPth.y = tempPt1.y - 2000;
      pPtg.x = pPtfg.x;
      pPtg.y = tempPt1.y - 2000;
      pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
      pPtf.y = pPtfg.y;
      pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
      pPte.y = pPtdc.y;

      SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
      SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
    } else  // perp,RearParkingOut,rightside
    {
      pPtcd.x = tempPt1.x;
      pPtcd.y = tempPt1.y - 300;
      pPtdc.x = tempPt1.x;
      pPtdc.y = tempPt1.y - 300;
      pPtfg.x = tempPt.x;
      pPtfg.y = tempPt.y + 300;
      pPtgf.x = tempPt.x;
      pPtgf.y = tempPt.y + 300;

      Obj1MoveDis = 300.0;
      Obj2MoveDis = 300.0;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarLeftSideExistSlot == FALSE) && (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x - 500;
        pPtcd.y = tempPt1.y - 500;
        pPtdc.x = tempPt1.x - 500;
        pPtdc.y = tempPt1.y - 500;

        Obj2MoveDis = (500.0);
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarLeftSideExistSlot == FALSE) &&
                 (CarLeftSideExistODObj == FALSE)) {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - 300;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - 300;

        Obj2MoveDis = (300.0);
        CloseLineMoveDis2 = (0.0);
      } else if ((CarLeftSideExistSlot == TRUE) &&
                 (CarLeftSideExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x - 1000;
        pPtcd.y = tempPt1.y - 1000;
        pPtdc.x = tempPt1.x - 1000;
        pPtdc.y = tempPt1.y - 1000;

        Obj2MoveDis = (1000.0);
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - 300;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - 300;

        Obj2MoveDis = (300.0);
        CloseLineMoveDis2 = (0.0);
      }
      if ((CarRightSideExistSlot == FALSE) &&
          (CarRightSideExistODObj == TRUE)) {
        pPtfg.x = tempPt.x - 500;
        pPtfg.y = tempPt.y + 500;
        pPtgf.x = tempPt.x - 500;
        pPtgf.y = tempPt.y + 500;

        Obj1MoveDis = (500.0);
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarRightSideExistSlot == FALSE) &&
                 (CarRightSideExistODObj == FALSE)) {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + 300;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + 300;

        Obj1MoveDis = (300.0);
        CloseLineMoveDis1 = (0.0);
      } else if ((CarRightSideExistSlot == TRUE) &&
                 (CarRightSideExistODObj == TRUE)) {
        pPtfg.x = tempPt.x - 1000;
        pPtfg.y = tempPt.y + 1000;
        pPtgf.x = tempPt.x - 1000;
        pPtgf.y = tempPt.y + 1000;

        Obj1MoveDis = (1000.0);
        CloseLineMoveDis1 = (-1000.0);
      } else {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + 300;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + 300;

        Obj1MoveDis = (300.0);
        CloseLineMoveDis1 = (0.0);
      }
      pPta.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 500;
      pPta.y = tempPt1.y - 2000;
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt1.y - 2000;
      pPte.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
      pPte.y = pPtdc.y;
      pPtf.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
      pPtf.y = pPtfg.y;
      pPtg.x = pPtgf.x;
      pPtg.y = tempPt.y + 2000;
      pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 500;
      pPth.y = tempPt.y + 200;

      SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 500);
      SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
    }
    if (CurSlotIsAngle == TRUE) {
      if (slot_data_at_right_side == FALSE)  // angleslot parkingout leftside
      {
        pPtcd.x = tempPt.x;
        pPtcd.y = tempPt.y + ObjLabelAngledObj2SafeDis;
        pPtdc.x = tempPt.x;
        pPtdc.y = tempPt.y + ObjLabelAngledObj2SafeDis;

        pPtgf.x = tempPt1.x;
        pPtgf.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtfg.x = tempPt1.x;
        pPtfg.y = tempPt1.y - ObjLabelAngledObj1SafeDis;

        Obj1MoveDis = ObjLabelAngledObj1SafeDis;
        Obj2MoveDis = ObjLabelAngledObj2SafeDis;
        CloseLineMoveDis1 = 0.0;
        CloseLineMoveDis2 = 0.0;
        if ((CarRightSideExistSlot == FALSE) &&
            (CarRightSideExistODObj == TRUE)) {
          pPtgf.x = tempPt1.x + 500;
          pPtgf.y = tempPt1.y - 500;
          pPtfg.x = tempPt1.x + 500;
          pPtfg.y = tempPt1.y - 500;

          Obj2MoveDis = 500.0;
          CloseLineMoveDis2 = (-500.0);
        } else if ((CarRightSideExistSlot == FALSE) &&
                   (CarRightSideExistODObj == FALSE)) {
          pPtgf.x = tempPt1.x;
          pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtfg.x = tempPt1.x;
          pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        } else if ((CarRightSideExistSlot == TRUE) &&
                   (CarRightSideExistODObj == TRUE)) {
          pPtgf.x = tempPt1.x + 1000;
          pPtgf.y = tempPt1.y - 1000;
          pPtfg.x = tempPt1.x + 1000;
          pPtfg.y = tempPt1.y - 1000;

          Obj2MoveDis = 1000.0;
          CloseLineMoveDis2 = (-1000.0);
        } else {
          pPtgf.x = tempPt1.x;
          pPtgf.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtfg.x = tempPt1.x;
          pPtfg.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        }
        if ((CarLeftSideExistSlot == FALSE) &&
            (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt.x + 500;
          pPtcd.y = tempPt.y + 500;
          pPtdc.x = tempPt.x + 500;
          pPtdc.y = tempPt.y + 500;

          Obj1MoveDis = 500.0;
          CloseLineMoveDis1 = (-500.0);
        } else if ((CarLeftSideExistSlot == FALSE) &&
                   (CarLeftSideExistODObj == FALSE)) {
          pPtcd.x = tempPt.x;
          pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtdc.x = tempPt.x;
          pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        } else if ((CarLeftSideExistSlot == TRUE) &&
                   (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt.x + 1000;
          pPtcd.y = tempPt.y + 1000;
          pPtdc.x = tempPt.x + 1000;
          pPtdc.y = tempPt.y + 1000;

          Obj1MoveDis = 1000.0;
          CloseLineMoveDis1 = (-1000.0);
        } else {
          pPtcd.x = tempPt.x;
          pPtcd.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtdc.x = tempPt.x;
          pPtdc.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        }
        if (ParkingOutClockwise == TRUE) {
          pPta.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
          pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
          pPth.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
          pPth.y = tempPt.y + 2000;

          SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
        } else {
          pPta.x = tempPt1.x + 7000;
          pPta.y = tempPt1.y - APAMap_ComCfg.LengthOfCar - 4000;
          pPth.x = tempPt.x + 7000;
          pPth.y = tempPt.y + 2000;

          SlotOutsideDis = (7000.0);
        }
        pPtb.x = pPtdc.x;
        pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
        pPtg.x = pPtfg.x;
        pPtg.y = tempPt1.y - 2000;
        pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
        pPtf.y = pPtfg.y;
        pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
        pPte.y = pPtdc.y;

        SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
      } else  // angleslot parkingout rightside
      {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjLabelAngledObj1SafeDis;
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjLabelAngledObj2SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjLabelAngledObj2SafeDis;

        Obj1MoveDis = ObjLabelAngledObj1SafeDis;
        Obj2MoveDis = ObjLabelAngledObj2SafeDis;
        CloseLineMoveDis1 = 0.0;
        CloseLineMoveDis2 = 0.0;
        if ((CarLeftSideExistSlot == FALSE) &&
            (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt1.x - 500;
          pPtcd.y = tempPt1.y - 500;
          pPtdc.x = tempPt1.x - 500;
          pPtdc.y = tempPt1.y - 500;

          Obj2MoveDis = 500.0;
          CloseLineMoveDis2 = (-500.0);
        } else if ((CarLeftSideExistSlot == FALSE) &&
                   (CarLeftSideExistODObj == FALSE)) {
          pPtcd.x = tempPt1.x;
          pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtdc.x = tempPt1.x;
          pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        } else if ((CarLeftSideExistSlot == TRUE) &&
                   (CarLeftSideExistODObj == TRUE)) {
          pPtcd.x = tempPt1.x - 1000;
          pPtcd.y = tempPt1.y - 1000;
          pPtdc.x = tempPt1.x - 1000;
          pPtdc.y = tempPt1.y - 1000;

          Obj2MoveDis = 1000.0;
          CloseLineMoveDis2 = (-1000.0);
        } else {
          pPtcd.x = tempPt1.x;
          pPtcd.y = tempPt1.y - ObjLabelAngledObj2SafeDis;
          pPtdc.x = tempPt1.x;
          pPtdc.y = tempPt1.y - ObjLabelAngledObj2SafeDis;

          Obj2MoveDis = ObjLabelAngledObj2SafeDis;
          CloseLineMoveDis2 = (0.0);
        }
        if ((CarRightSideExistSlot == FALSE) &&
            (CarRightSideExistODObj == TRUE)) {
          pPtfg.x = tempPt.x - 500;
          pPtfg.y = tempPt.y + 500;
          pPtgf.x = tempPt.x - 500;
          pPtgf.y = tempPt.y + 500;

          Obj1MoveDis = 500.0;
          CloseLineMoveDis1 = (-500.0);
        } else if ((CarRightSideExistSlot == FALSE) &&
                   (CarRightSideExistODObj == FALSE)) {
          pPtfg.x = tempPt.x;
          pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtgf.x = tempPt.x;
          pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        } else if ((CarRightSideExistSlot == TRUE) &&
                   (CarRightSideExistODObj == TRUE)) {
          pPtfg.x = tempPt.x - 1000;
          pPtfg.y = tempPt.y + 1000;
          pPtgf.x = tempPt.x - 1000;
          pPtgf.y = tempPt.y + 1000;

          Obj1MoveDis = 1000.0;
          CloseLineMoveDis1 = (-1000.0);
        } else {
          pPtfg.x = tempPt.x;
          pPtfg.y = tempPt.y + ObjLabelAngledObj1SafeDis;
          pPtgf.x = tempPt.x;
          pPtgf.y = tempPt.y + ObjLabelAngledObj1SafeDis;

          Obj1MoveDis = ObjLabelAngledObj1SafeDis;
          CloseLineMoveDis1 = (0.0);
        }
        if (ParkingOutClockwise == TRUE) {
          pPta.x = tempPt.x - APAMap_ComCfg.LengthOfCar - 1000;
          pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
          pPth.x = tempPt1.x - APAMap_ComCfg.LengthOfCar - 1000;
          pPth.y = tempPt1.y - 2000;

          SlotOutsideDis = (APAMap_ComCfg.LengthOfCar + 1000);
        } else {
          pPta.x = tempPt.x - 7000;
          pPta.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
          pPth.x = tempPt1.x - 7000;
          pPth.y = tempPt1.y - 2000;

          SlotOutsideDis = (7000.0);
        }
        pPtb.x = pPtdc.x;
        pPtb.y = tempPt.y + APAMap_ComCfg.LengthOfCar + 4000;
        pPtg.x = pPtfg.x;
        pPtg.y = tempPt1.y - 2000;
        pPtf.x = tempPt1.x + APAMap_ComCfg.LengthOfCar + 1000;
        pPtf.y = pPtfg.y;
        pPte.x = tempPt.x + APAMap_ComCfg.LengthOfCar + 1000;
        pPte.y = pPtdc.y;

        SlotInnerDis = (-APAMap_ComCfg.LengthOfCar - 1000);
      }
    }
  } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == TRUE)  // Parall,ParkingOut,leftside
    {
      pPtcd.x = tempPt1.x;
      pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
      pPtdc.x = tempPt1.x;
      pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
      pPtfg.x = tempPt.x;
      pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
      pPtgf.x = tempPt.x;
      pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

      Obj1MoveDis = ObjParallelObj1SafeDis;
      Obj2MoveDis = ObjParallelObj2SafeDis;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarForwardExistSlot == FALSE) && (CarForwardExistODObj == TRUE)) {
        pPtfg.x = tempPt.x + 500;
        pPtfg.y = tempPt.y + 500;
        pPtgf.x = tempPt.x + 500;
        pPtgf.y = tempPt.y + 500;

        Obj1MoveDis = 500.0;
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarForwardExistSlot == FALSE) &&
                 (CarForwardExistODObj == FALSE)) {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

        Obj1MoveDis = ObjParallelObj1SafeDis;
        CloseLineMoveDis1 = (0.0);
      } else if ((CarForwardExistSlot == TRUE) &&
                 (CarForwardExistODObj == TRUE)) {
        pPtfg.x = tempPt.x + 1000;
        pPtfg.y = tempPt.y + 1000;
        pPtgf.x = tempPt.x + 1000;
        pPtgf.y = tempPt.y + 1000;

        Obj1MoveDis = 1000.0;
        CloseLineMoveDis1 = (-1000.0);
      } else {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

        Obj1MoveDis = ObjParallelObj1SafeDis;
        CloseLineMoveDis1 = (0.0);
      }
      if ((CarBackwardExistSlot == FALSE) && (CarBackwardExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x + 500;
        pPtcd.y = tempPt1.y - 500;
        pPtdc.x = tempPt1.x + 500;
        pPtdc.y = tempPt1.y - 500;

        Obj2MoveDis = 500.0;
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarBackwardExistSlot == FALSE) &&
                 (CarBackwardExistODObj == FALSE)) {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;

        Obj2MoveDis = ObjParallelObj2SafeDis;
        CloseLineMoveDis2 = (0.0);
      } else if ((CarBackwardExistSlot == TRUE) &&
                 (CarBackwardExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x + 1000;
        pPtcd.y = tempPt1.y - 1000;
        pPtdc.x = tempPt1.x + 1000;
        pPtdc.y = tempPt1.y - 1000;

        Obj2MoveDis = 1000.0;
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;

        Obj2MoveDis = ObjParallelObj2SafeDis;
        CloseLineMoveDis2 = (0.0);
      }
      pPta.x = tempPt1.x - APAMap_ComCfg.WidthOfCar - 2500;
      pPta.y = tempPt1.y - 1000;
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt1.y - 1000;
      pPte.x = tempPt1.x + APAMap_ComCfg.WidthOfCar + kPerpendicularSlotMinExtraWidthMm;
      pPte.y = pPtdc.y;
      pPtf.x = tempPt.x + APAMap_ComCfg.WidthOfCar + kPerpendicularSlotMinExtraWidthMm;
      pPtf.y = pPtfg.y;
      pPtg.x = pPtgf.x;
      pPtg.y = tempPt.y + 4000;
      pPth.x = tempPt.x - APAMap_ComCfg.WidthOfCar - 2500;
      pPth.y = tempPt.y + 4000;

      SlotOutsideDis = (APAMap_ComCfg.WidthOfCar + 2500);
      SlotInnerDis = (-APAMap_ComCfg.WidthOfCar - 500);
    } else {
      pPtcd.x = tempPt1.x;
      pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
      pPtdc.x = tempPt1.x;
      pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;
      pPtfg.x = tempPt.x;
      pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
      pPtgf.x = tempPt.x;
      pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

      Obj1MoveDis = ObjParallelObj1SafeDis;
      Obj2MoveDis = ObjParallelObj2SafeDis;
      CloseLineMoveDis1 = 0.0;
      CloseLineMoveDis2 = 0.0;
      if ((CarForwardExistSlot == FALSE) && (CarForwardExistODObj == TRUE)) {
        pPtfg.x = tempPt.x - 500;
        pPtfg.y = tempPt.y + 500;
        pPtgf.x = tempPt.x - 500;
        pPtgf.y = tempPt.y + 500;

        Obj1MoveDis = 500.0;
        CloseLineMoveDis1 = (-500.0);
      } else if ((CarForwardExistSlot == FALSE) &&
                 (CarForwardExistODObj == FALSE)) {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

        Obj1MoveDis = ObjParallelObj1SafeDis;
        CloseLineMoveDis1 = (0.0);
      } else if ((CarForwardExistSlot == TRUE) &&
                 (CarForwardExistODObj == TRUE)) {
        pPtfg.x = tempPt.x - 1000;
        pPtfg.y = tempPt.y + 1000;
        pPtgf.x = tempPt.x - 1000;
        pPtgf.y = tempPt.y + 1000;

        Obj1MoveDis = 1000.0;
        CloseLineMoveDis1 = (-1000.0);
      } else {
        pPtfg.x = tempPt.x;
        pPtfg.y = tempPt.y + ObjParallelObj1SafeDis;
        pPtgf.x = tempPt.x;
        pPtgf.y = tempPt.y + ObjParallelObj1SafeDis;

        Obj1MoveDis = ObjParallelObj1SafeDis;
        CloseLineMoveDis1 = (0.0);
      }
      if ((CarBackwardExistSlot == FALSE) && (CarBackwardExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x - 500;
        pPtcd.y = tempPt1.y - 500;
        pPtdc.x = tempPt1.x - 500;
        pPtdc.y = tempPt1.y - 500;

        Obj2MoveDis = 500.0;
        CloseLineMoveDis2 = (-500.0);
      } else if ((CarBackwardExistSlot == FALSE) &&
                 (CarBackwardExistODObj == FALSE)) {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;

        Obj2MoveDis = ObjParallelObj2SafeDis;
        CloseLineMoveDis2 = (0.0);
      } else if ((CarBackwardExistSlot == TRUE) &&
                 (CarBackwardExistODObj == TRUE)) {
        pPtcd.x = tempPt1.x - 1000;
        pPtcd.y = tempPt1.y - 1000;
        pPtdc.x = tempPt1.x - 1000;
        pPtdc.y = tempPt1.y - 1000;

        Obj2MoveDis = 1000.0;
        CloseLineMoveDis2 = (-1000.0);
      } else {
        pPtcd.x = tempPt1.x;
        pPtcd.y = tempPt1.y - ObjParallelObj2SafeDis;
        pPtdc.x = tempPt1.x;
        pPtdc.y = tempPt1.y - ObjParallelObj2SafeDis;

        Obj2MoveDis = ObjParallelObj2SafeDis;
        CloseLineMoveDis2 = (0.0);
      }
      pPta.x = tempPt1.x + APAMap_ComCfg.WidthOfCar + 2500;
      pPta.y = tempPt1.y - 1000;
      pPtb.x = pPtdc.x;
      pPtb.y = tempPt1.y - 1000;
      pPte.x = tempPt1.x - APAMap_ComCfg.WidthOfCar - 500;
      pPte.y = pPtdc.y;
      pPtf.x = tempPt.x - APAMap_ComCfg.WidthOfCar - 500;
      pPtf.y = pPtfg.y;
      pPtg.x = pPtgf.x;
      pPtg.y = tempPt.y + 4000;
      pPth.x = tempPt.x + APAMap_ComCfg.WidthOfCar + 2500;
      pPth.y = tempPt.y + 4000;

      SlotOutsideDis = (APAMap_ComCfg.WidthOfCar + 2500);
      SlotInnerDis = (-APAMap_ComCfg.WidthOfCar - 500);
    }
  } else {
    return;
  }
  CloseLineMoveDis = (CloseLineMoveDis1 + CloseLineMoveDis2) / 2;
  Obj1MoveDis = Obj1MoveDis + CarSideToObj1LineDis;
  Obj2MoveDis = Obj2MoveDis + CarSideToObj2LineDis;
  if ((Obj1MoveDis < 400.0) && (CarSideToObj1LineDis <= 0)) {
    Obj1MoveDis = 400;
  }
  if ((Obj2MoveDis < 400.0) && (CarSideToObj2LineDis <= 0)) {
    Obj2MoveDis = 400;
  }
  APAMapEFOutputData.ElectronicFencePt[0].x = pPta.x;
  APAMapEFOutputData.ElectronicFencePt[0].y = pPta.y;
  APAMapEFOutputData.ElectronicFencePt[1].x = pPtb.x;
  APAMapEFOutputData.ElectronicFencePt[1].y = pPtb.y;
  APAMapEFOutputData.ElectronicFencePt[2].x = pPtcd.x;
  APAMapEFOutputData.ElectronicFencePt[2].y = pPtcd.y;
  APAMapEFOutputData.ElectronicFencePt[3].x = pPtdc.x;
  APAMapEFOutputData.ElectronicFencePt[3].y = pPtdc.y;
  APAMapEFOutputData.ElectronicFencePt[4].x = pPte.x;
  APAMapEFOutputData.ElectronicFencePt[4].y = pPte.y;
  APAMapEFOutputData.ElectronicFencePt[5].x = pPtf.x;
  APAMapEFOutputData.ElectronicFencePt[5].y = pPtf.y;
  APAMapEFOutputData.ElectronicFencePt[6].x = pPtfg.x;
  APAMapEFOutputData.ElectronicFencePt[6].y = pPtfg.y;
  APAMapEFOutputData.ElectronicFencePt[7].x = pPtgf.x;
  APAMapEFOutputData.ElectronicFencePt[7].y = pPtgf.y;
  APAMapEFOutputData.ElectronicFencePt[8].x = pPtg.x;
  APAMapEFOutputData.ElectronicFencePt[8].y = pPtg.y;
  APAMapEFOutputData.ElectronicFencePt[9].x = pPth.x;
  APAMapEFOutputData.ElectronicFencePt[9].y = pPth.y;
  APAMapEFOutputData.timeStamp_ms = APAMap_GInputData.CarLocInfo.timeStamp_ms;
  APAMapEFOutputData.CarPos.CarAng = CurCarPos.CarAng;
  APAMapEFOutputData.CarPos.Coordinate.x = CurCarPos.Coordinate.x;
  APAMapEFOutputData.CarPos.Coordinate.y = CurCarPos.Coordinate.y;
  for (i = 0; i < ElectrFencePtNum; i++) {
    APAMapEFOutputData.ElectronicFencePt[i] = AlgCom_PointPosWithAngAndCenterPt(
        APAMapEFOutputData.ElectronicFencePt[i], OrgAng, OrgPt);
  }
  for (i = 0; i < SlotPtNum; i++) {
    pgetVPLSlotData[i] = getVPLSlotData[i];
  }
  *pgetObj1Pt = getObj1Pt;
  *pgetObj2Pt = getObj2Pt;
  *pCarSideToObj1LineDis = CarSideToObj1LineDis;
  *pCarSideToObj2LineDis = CarSideToObj2LineDis;
  *pCurSlotTopLineAngle = CurSlotTopLineAngle;
  *pCurSlotCloseLineAngle = CurSlotCloseLineAngle;
  *pObj1MoveDis = Obj1MoveDis;
  *pObj2MoveDis = Obj2MoveDis;
  *pCloseLineMoveDis = CloseLineMoveDis;
  *pSlotOutsideDis = SlotOutsideDis;
  *pSlotInnerDis = SlotInnerDis;
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==ElectrFence==LfSideExistSlot(%d)==RtSideExistSlot(%d)\n"
             "==FdExistSlot(%d)==BdExistSlot(%d)==LfSideExistODObj(%d)=="
             "RtSideExistODObj(%d)==FdExistODObj(%d)==BdExistODObj(%d)\n"
             "==ParkingOutClockwise(%d)==CurSlotIsAngle(%d)==Obj1MoveDis(%f)=="
             "Obj2MoveDis(%f)==CloseLineMoveDis(%f)\n"
             "==SlotOutsideDis(%f)==SlotInnerDis(%f)",
             CarLeftSideExistSlot, CarRightSideExistSlot, CarForwardExistSlot,
             CarBackwardExistSlot, CarLeftSideExistODObj,
             CarRightSideExistODObj, CarForwardExistODObj,
             CarBackwardExistODObj, ParkingOutClockwise, CurSlotIsAngle,
             Obj1MoveDis, Obj2MoveDis, CloseLineMoveDis, SlotOutsideDis,
             SlotInnerDis);
    TLOG_INFO << log_string;
  }
}

BOOLEAN APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(
    APACoordinateDataCalFloatType* pVPLSlotPts,
    APACoordinateDataCalFloatType* pNewVPLSlotPts, APA_ENUM_TYPE* pOrgIndex) {
  uint8_t_INF park_out_mode;
  BOOLEAN slot_data_at_right_side;
  APA_ENUM_TYPE Obj2PtIndex;
  APA_ENUM_TYPE Obj1PtIndex;
  APA_ENUM_TYPE Obj2InnerIndex;
  APA_ENUM_TYPE Obj1InnerIndex;
  APALineParameterABCType CloseLine;
  APALineParameterABCType TopLine;
  APALineParameterABCType FarLine;
  APALineParameterABCType TempLine;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 0;     // Left_Top
      Obj1PtIndex = 1;     // Left_Bottom
      Obj2InnerIndex = 3;  // Right_Top
      Obj1InnerIndex = 2;  // Right_Bottom
    } else {
      Obj2PtIndex = 3;     // Right_Top
      Obj1PtIndex = 2;     // Right_Bottom
      Obj2InnerIndex = 0;  // Left_Top
      Obj1InnerIndex = 1;  // Left_Bottom
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1PtIndex],
                                 pVPLSlotPts[Obj2PtIndex], &CloseLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
        pVPLSlotPts[Obj2PtIndex], &CloseLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj2InnerIndex]);
    TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
        pVPLSlotPts[Obj1InnerIndex], &CloseLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 3;     // Right_Top
      Obj1PtIndex = 0;     // Left_Top
      Obj2InnerIndex = 2;  // Right_Bottom
      Obj1InnerIndex = 1;  // Left_Bottom
    } else {
      Obj2PtIndex = 0;     // Left_Top
      Obj1PtIndex = 3;     // Right_Top
      Obj2InnerIndex = 1;  // Left_Bottom
      Obj1InnerIndex = 2;  // Right_Bottom
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
                                 pVPLSlotPts[Obj2PtIndex], &TopLine);
    AlgCom_LineParABCByParallelLineAndPointOnLine(
        &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 1;     // Left_Bottom
      Obj1PtIndex = 2;     // Right_Bottom
      Obj2InnerIndex = 0;  // Left_Top
      Obj1InnerIndex = 3;  // Right_Top
    } else {
      Obj2PtIndex = 2;     // Right_Bottom
      Obj1PtIndex = 1;     // Left_Bottom
      Obj2InnerIndex = 3;  // Right_Top
      Obj1InnerIndex = 0;  // Left_Top
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
                                 pVPLSlotPts[Obj2PtIndex], &TopLine);
    AlgCom_LineParABCByParallelLineAndPointOnLine(
        &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else {
    // data errors;
    return FALSE;
  }
  pNewVPLSlotPts[0] = pVPLSlotPts[Obj2PtIndex];
  pNewVPLSlotPts[1] = pVPLSlotPts[Obj1PtIndex];
  pNewVPLSlotPts[2] = pVPLSlotPts[Obj1InnerIndex];
  pNewVPLSlotPts[3] = pVPLSlotPts[Obj2InnerIndex];
  pOrgIndex[0] = Obj2PtIndex;
  pOrgIndex[1] = Obj1PtIndex;
  pOrgIndex[2] = Obj1InnerIndex;
  pOrgIndex[3] = Obj2InnerIndex;
  return TRUE;
}
BOOLEAN APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot(
    APACoordinateDataCalFloatType* pObj2Pt,
    APACoordinateDataCalFloatType* pObj1Pt,
    APA_DISTANCE_CAL_FLOAT_TYPE* pSlotAng, APA_DISTANCE_CAL_FLOAT_TYPE ObjAng,
    APACoordinateDataCalFloatType* pVPLSlotPtsNearBy,
    BOOLEAN slot_data_at_right_side, APA_ENUM_TYPE* pFailCause) {
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType TempPt1;
  APACoordinateDataCalFloatType TempPt2;
  APA_DISTANCE_CAL_FLOAT_TYPE SlotAngNearBy;
  APA_DISTANCE_CAL_FLOAT_TYPE ObjAngNearBy;
  APALineParameterABCType TempLine;
  APALineParameterABCType TempLine1;
  APA_DISTANCE_CAL_FLOAT_TYPE DeltaAng;
  APA_DISTANCE_CAL_FLOAT_TYPE NewSlotAng;
  BOOLEAN bRerAngSlot;
  APACarCoordinateDataCalFloatType TempCarPos;
  *pFailCause = 0;
  AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[1], pVPLSlotPtsNearBy[0],
                        &SlotAngNearBy);
  DeltaAng = *pSlotAng - SlotAngNearBy;
  AlgCom_AngNormalized(&DeltaAng);
  if (MATH_FABS(DeltaAng) > 5.0 * PI / 180.0) {
    *pFailCause = 1;
    return FALSE;
  }
  AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[3], pVPLSlotPtsNearBy[0],
                        &ObjAngNearBy);
  DeltaAng = ObjAng - ObjAngNearBy;
  AlgCom_AngNormalized(&DeltaAng);
  if (MATH_FABS(DeltaAng) > 5.0 * PI / 180.0) {
    *pFailCause = 2;
    return FALSE;
  }
  TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      pVPLSlotPtsNearBy[0], 0, *pSlotAng, *pObj2Pt);
  TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      pVPLSlotPtsNearBy[1], 0, *pSlotAng, *pObj2Pt);
  TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      *pObj1Pt, 0, *pSlotAng, *pObj2Pt);
  if (TempPt2.y > 0) {
    if (MATH_FABS(TempPt1.y) > 600)  // 300
    {
      *pFailCause = 3;
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot===Test"
                 "==TempPt2(%.2f,%.2f)==TempPt1(%.2f,%.2f)==TempPt(%.2f,%.2f)=="
                 "MATH_FABS(TempPt2.y - TempPt.y))(%.2f)",
                 TempPt2.x, TempPt2.y, TempPt1.x, TempPt1.y, TempPt.x, TempPt.y,
                 MATH_FABS(TempPt2.y - TempPt.y));
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
    if (((TempPt1.x < 0) && (slot_data_at_right_side == TRUE)) ||
        ((TempPt1.x > 0) && (slot_data_at_right_side == FALSE))) {
      // 30~60;
      bRerAngSlot = FALSE;
      AlgCom_GetAngByTwoPts(*pObj1Pt, pVPLSlotPtsNearBy[1], &NewSlotAng);
    } else {
      // 120 ~150;
      bRerAngSlot = TRUE;
      AlgCom_GetAngByTwoPts(*pObj2Pt, pVPLSlotPtsNearBy[0], &NewSlotAng);
    }
    DeltaAng = NewSlotAng - *pSlotAng;
    AlgCom_AngNormalized(&DeltaAng);
    DeltaAng = MATH_FABS(DeltaAng);
    if (DeltaAng < 15.0 * PI / 180.0)  // 25
    {
      *pFailCause = 4;
      return FALSE;
    } else if (DeltaAng > 65.0 * PI / 180.0) {
      *pFailCause = 5;
      return FALSE;
    } else {
      if (bRerAngSlot == FALSE) {
        AlgCom_LineParABCbyTwoPoints(*pObj1Pt, pVPLSlotPtsNearBy[1], &TempLine);
        TempCarPos.CarAng = ObjAng;
        TempCarPos.Coordinate = *pObj2Pt;
        TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
        if (TRUE ==
            AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
          *pObj2Pt = TempPt1;
          *pSlotAng = NewSlotAng;
          return TRUE;
        } else {
          *pFailCause = 6;
          return FALSE;
        }
      } else {
        AlgCom_LineParABCbyTwoPoints(*pObj2Pt, pVPLSlotPtsNearBy[0], &TempLine);
        TempCarPos.CarAng = ObjAng;
        TempCarPos.Coordinate = *pObj1Pt;
        TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
        if (TRUE ==
            AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
          *pObj1Pt = TempPt1;
          *pSlotAng = NewSlotAng;
          return TRUE;
        } else {
          *pFailCause = 7;
          return FALSE;
        }
      }
    }
  } else {
    if (MATH_FABS(TempPt2.y - TempPt.y) > 600)  // 300
    {
      *pFailCause = 8;
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot===Test"
                 "==TempPt2(%.2f,%.2f)==TempPt1(%.2f,%.2f)==TempPt(%.2f,%.2f)=="
                 "MATH_FABS(TempPt2.y - TempPt.y))(%.2f)",
                 TempPt2.x, TempPt2.y, TempPt1.x, TempPt1.y, TempPt.x, TempPt.y,
                 MATH_FABS(TempPt2.y - TempPt.y));
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
    if (((TempPt2.x > 0) && (slot_data_at_right_side == TRUE)) ||
        ((TempPt2.x < 0) && (slot_data_at_right_side == FALSE))) {
      // 30~60;
      bRerAngSlot = FALSE;
      AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[1], *pObj1Pt, &NewSlotAng);
    } else {
      // 120 ~150;
      bRerAngSlot = TRUE;
      AlgCom_GetAngByTwoPts(pVPLSlotPtsNearBy[0], *pObj2Pt, &NewSlotAng);
    }
    DeltaAng = NewSlotAng - *pSlotAng;
    AlgCom_AngNormalized(&DeltaAng);
    DeltaAng = MATH_FABS(DeltaAng);
    if (DeltaAng < 15.0 * PI / 180.0)  // 25
    {
      *pFailCause = 9;
      return FALSE;
    } else if (DeltaAng > 65.0 * PI / 180.0) {
      *pFailCause = 10;
      return FALSE;
    } else {
      if (bRerAngSlot == FALSE) {
        AlgCom_LineParABCbyTwoPoints(*pObj1Pt, pVPLSlotPtsNearBy[1], &TempLine);
        TempCarPos.CarAng = ObjAng;
        TempCarPos.Coordinate = *pObj2Pt;
        TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
        if (TRUE ==
            AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
          *pObj2Pt = TempPt1;
          *pSlotAng = NewSlotAng;
          return TRUE;
        } else {
          *pFailCause = 11;
          return FALSE;
        }
      } else {
        AlgCom_LineParABCbyTwoPoints(*pObj2Pt, pVPLSlotPtsNearBy[0], &TempLine);
        TempCarPos.CarAng = ObjAng;
        TempCarPos.Coordinate = *pObj1Pt;
        TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
        if (TRUE ==
            AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt1)) {
          *pObj1Pt = TempPt1;
          *pSlotAng = NewSlotAng;
          return TRUE;
        } else {
          *pFailCause = 12;
          return FALSE;
        }
      }
    }
  }
  return FALSE;
}
BOOLEAN APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(
    APACoordinateDataCalFloatType* pVPLSlotPts,
    APACoordinateDataCalFloatType* pObj2Pt,
    APACoordinateDataCalFloatType* pObj1Pt,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Ang,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Ang,
    APA_DISTANCE_CAL_FLOAT_TYPE* pNewOrgAng,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Dis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Dis,
    APA_DISTANCE_CAL_FLOAT_TYPE* pCarOffsetX,
    APA_DISTANCE_CAL_FLOAT_TYPE* pMinSlotDpth,
    APA_DISTANCE_CAL_FLOAT_TYPE* pVPLSlotDpth) {
  APACarCoordinateDataCalFloatType CurCarPos;
  uint8_t_INF park_out_mode;
  BOOLEAN slot_data_at_right_side;
  APA_ENUM_TYPE Obj2PtIndex;
  APA_ENUM_TYPE Obj1PtIndex;
  APA_ENUM_TYPE Obj2InnerIndex;
  APA_ENUM_TYPE Obj1InnerIndex;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis2;
  APA_ENUM_TYPE LocStyle;
  APALineParameterABCType CloseLine;
  APALineParameterABCType TopLine;
  APALineParameterABCType FarLine;
  APALineParameterABCType TempLine;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, &pRectPt[0], &pRectLine[0]);
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 0;     // Left_Top
      Obj1PtIndex = 1;     // Left_Bottom
      Obj2InnerIndex = 3;  // Right_Top
      Obj1InnerIndex = 2;  // Right_Bottom
    } else {
      Obj2PtIndex = 3;     // Right_Top
      Obj1PtIndex = 2;     // Right_Bottom
      Obj2InnerIndex = 0;  // Left_Top
      Obj1InnerIndex = 1;  // Left_Bottom
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1PtIndex],
                                 pVPLSlotPts[Obj2PtIndex], &CloseLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
        pVPLSlotPts[Obj2PtIndex], &CloseLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj2InnerIndex]);
    TempLine = AlgCom_LineParABCByPerPendLineAndPointOnLine(
        pVPLSlotPts[Obj1InnerIndex], &CloseLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 3;     // Right_Top
      Obj1PtIndex = 0;     // Left_Top
      Obj2InnerIndex = 2;  // Right_Bottom
      Obj1InnerIndex = 1;  // Left_Bottom
    } else {
      Obj2PtIndex = 0;     // Left_Top
      Obj1PtIndex = 3;     // Right_Top
      Obj2InnerIndex = 1;  // Left_Bottom
      Obj1InnerIndex = 2;  // Right_Bottom
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
                                 pVPLSlotPts[Obj2PtIndex], &TopLine);
    AlgCom_LineParABCByParallelLineAndPointOnLine(
        &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND)) {
    if (slot_data_at_right_side == TRUE) {
      Obj2PtIndex = 1;     // Left_Bottom
      Obj1PtIndex = 2;     // Right_Bottom
      Obj2InnerIndex = 0;  // Left_Top
      Obj1InnerIndex = 3;  // Right_Top
    } else {
      Obj2PtIndex = 2;     // Right_Bottom
      Obj1PtIndex = 1;     // Left_Bottom
      Obj2InnerIndex = 3;  // Right_Top
      Obj1InnerIndex = 0;  // Left_Top
    }
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj2InnerIndex],
                                 pVPLSlotPts[Obj2PtIndex], &TopLine);
    AlgCom_LineParABCByParallelLineAndPointOnLine(
        &TempLine, pVPLSlotPts[Obj1PtIndex], &TopLine);
    AlgCom_LineParABCbyTwoPoints(pVPLSlotPts[Obj1InnerIndex],
                                 pVPLSlotPts[Obj2InnerIndex], &FarLine);
    AlgCom_CrossPointOfTwoLines(&TempLine, &FarLine,
                                &pVPLSlotPts[Obj1InnerIndex]);
  } else {
    // data errors;
    return FALSE;
  }
  *pObj2Pt = pVPLSlotPts[Obj2PtIndex];
  *pObj1Pt = pVPLSlotPts[Obj1PtIndex];
  AlgCom_GetAngByTwoPts(pVPLSlotPts[Obj1PtIndex], pVPLSlotPts[Obj2PtIndex],
                        pNewOrgAng);
  AlgCom_GetAngByTwoPts(pVPLSlotPts[Obj2InnerIndex], pVPLSlotPts[Obj2PtIndex],
                        pObj2Ang);
  *pObj1Ang = *pObj2Ang;
  // Cal Obj2Dis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj2InnerIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj2PtIndex], &TempDis);
  if (((LocStyle == 1) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 0) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over TopLine;
    TempDis = -TempDis;
  }
  TempDis2 = TempDis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj2InnerIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj2InnerIndex], &TempDis);
  if (((LocStyle == 1) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 0) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over TopLine;
    TempDis = -TempDis;
  }
  if (TempDis2 > TempDis) {
    TempDis2 = TempDis;
  }
  *pObj2Dis = TempDis2;

  // Cal Obj1Dis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1InnerIndex], &pVPLSlotPts[Obj1PtIndex],
      &pRectPt[Obj1PtIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over BottomLine;
    TempDis = -TempDis;
  }
  TempDis1 = TempDis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1InnerIndex], &pVPLSlotPts[Obj1PtIndex],
      &pRectPt[Obj1InnerIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over BottomLine;
    TempDis = -TempDis;
  }
  if (TempDis1 > TempDis) {
    TempDis1 = TempDis;
  }
  *pObj1Dis = TempDis1;
  // Cal CarOffsetX;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj2PtIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  TempDis1 = TempDis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj1PtIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  if (TempDis1 > TempDis) {
    TempDis1 = TempDis;
  }
  *pCarOffsetX = TempDis1;

  // Cal pMinSlotDpth;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj2InnerIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  TempDis2 = TempDis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pRectPt[Obj1InnerIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  if (TempDis2 < TempDis) {
    TempDis2 = TempDis;
  }
  *pMinSlotDpth = TempDis2;

  // Cal pVPLSlotDpth;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pVPLSlotPts[Obj2InnerIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  TempDis2 = TempDis;
  LocStyle = AlgCom_GetPointLocationAccordGivenVector(
      &pVPLSlotPts[Obj1PtIndex], &pVPLSlotPts[Obj2PtIndex],
      &pVPLSlotPts[Obj1InnerIndex], &TempDis);
  if (((LocStyle == 0) && (slot_data_at_right_side == TRUE)) ||
      ((LocStyle == 1) && (slot_data_at_right_side == FALSE))) {
    // CarCorPt Over CloseLine;
    TempDis = -TempDis;
  }
  if (TempDis2 < TempDis) {
    TempDis2 = TempDis;
  }
  *pVPLSlotDpth = TempDis2;
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==ParkOutGetSlotInfoFromLocVPLSlotByTotalMap===SlotPt:0(%.2f,%."
             "2f),1(%.2f,%.2f),"
             "2(%.2f,%.2f),3(%.2f,%.2f),Obj2Pt(%.2f,%.2f),Obj1Pt(%.2f,%.2f),"
             "ObjAng(%.2f),OrgAng(%.2f),Dis(%.2f,%.2f),CarOffsetX(%.2f),"
             "MinSlotDpth(%.2f),VPLSlotDpth(%.2f)",
             pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
             pVPLSlotPts[1].y, pVPLSlotPts[2].x, pVPLSlotPts[2].y,
             pVPLSlotPts[3].x, pVPLSlotPts[3].y, pObj2Pt->x, pObj2Pt->y,
             pObj1Pt->x, pObj1Pt->y, *pObj2Ang * 180.0 / PI,
             *pNewOrgAng * 180.0 / PI, *pObj2Dis, *pObj1Dis, *pCarOffsetX,
             *pMinSlotDpth, *pVPLSlotDpth);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
#if 1
BOOLEAN APAMap_ParkingOutBuildSlotByOneSideNearbySlot(
    APACoordinateDataCalFloatType* pCurSegData,
    APACoordinateDataCalFloatType* pFirstSegData,
    APACoordinateDataCalFloatType* pSecondSegData,
    APACoordinateDataCalFloatType FirstNearByCarPosSlot,
    APACoordinateDataCalFloatType SecondNearByCarPosSlot,
    uint8_t_INF Data1Index, uint8_t_INF Data2Index, uint8_t Label,
    uint8_t_INF slot_side) {
  /**
   * APA（自动泊车辅助）系统坐标数据计算相关变量声明
   * 使用浮点类型进行坐标数据计算
   */
  APACoordinateDataCalFloatType Data[4];  // 存储坐标数据的数组，包含4个元素
  uint8_t_INF i;                          //  循环计数器，8位无符号整数类型
  APACarCoordinateDataCalFloatType CurCarPos;  // 当前车辆位置坐标数据，浮点类型
  BOOLEAN result;
  APACoordinateDataCalFloatType FirstSegData[4];
  APACoordinateDataCalFloatType SecondSegData[4];

  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  for (i = 0; i < 4; i++) {  //  循环处理4个数据点
    Data[i] = pCurSegData[i];
    FirstSegData[i] = pFirstSegData[i];
    SecondSegData[i] = pSecondSegData[i];
  }
  result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
      &FirstSegData[0], FirstNearByCarPosSlot, CurCarPos.CarAng);
  if (TRUE == result) {
    result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
        &SecondSegData[0], SecondNearByCarPosSlot, CurCarPos.CarAng);
  }
  if (FALSE == result) {
    return FALSE;
  }

  if (Label == Obj_Label_Ladder_Slot) {  //  处理阶梯车位（Ladder Slot）的情况
    if (0 == slot_side)                   // left side slot
    {
      if (0 == Data1Index)  // FirstBottom  Data2Index == 2
      {
        Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
        Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
        Data[1] = FirstSegData[2];
        Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
        Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
        Data[3].x = 2 * Data[0].x - FirstSegData[0].x;
        Data[3].y = 2 * Data[0].y - FirstSegData[0].y;
      } else if (1 == Data1Index)  // FirstTop  Data2Index == 3
      {
        Data[0] = FirstSegData[3];
        Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
        Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
        Data[2].x = 2 * Data[1].x - FirstSegData[1].x;
        Data[2].y = 2 * Data[1].y - FirstSegData[1].y;
        Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
        Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
      } else if (2 == Data1Index)  // SecondBottom  Data2Index == 0
      {
        Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
        Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
        Data[1] = SecondSegData[2];
        Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
        Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
        Data[3].x = 2 * Data[0].x - SecondSegData[0].x;
        Data[3].y = 2 * Data[0].y - SecondSegData[0].y;
      } else if (3 == Data1Index)  // SecondTop  Data2Index == 1
      {
        Data[0] = SecondSegData[3];
        Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
        Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
        Data[2].x = 2 * Data[1].x - SecondSegData[1].x;
        Data[2].y = 2 * Data[1].y - SecondSegData[1].y;
        Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
        Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
      }
    } else  // right side slot
    {
      if (0 == Data1Index)  // FirstBottom  Data2Index == 2
      {
        Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
        Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
        Data[2] = FirstSegData[1];
        Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
        Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
        Data[0].x = 2 * Data[3].x - FirstSegData[3].x;
        Data[0].y = 2 * Data[3].y - FirstSegData[3].y;
      } else if (1 == Data1Index)  // FirstTop  Data2Index == 3
      {
        Data[3] = FirstSegData[0];
        Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
        Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
        Data[1].x = 2 * Data[2].x - FirstSegData[2].x;
        Data[1].y = 2 * Data[2].y - FirstSegData[2].y;
        Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
        Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
      } else if (2 == Data1Index)  // SecondBottom  Data2Index == 0
      {
        Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
        Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
        Data[2] = SecondSegData[1];
        Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
        Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
        Data[0].x = 2 * Data[3].x - SecondSegData[3].x;
        Data[0].y = 2 * Data[3].y - SecondSegData[3].y;
      } else if (3 == Data1Index)  // SecondTop  Data2Index == 1
      {
        Data[3] = SecondSegData[0];
        Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
        Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
        Data[1].x = 2 * Data[2].x - SecondSegData[2].x;
        Data[1].y = 2 * Data[2].y - SecondSegData[2].y;
        Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
        Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
      }
    }
  } else  // Obj_Label_Angled_Slot
  {
    if (0 == slot_side)  // left side slot
    {
      if ((0 == Data1Index)      // FirstBottom  Data2Index == 2
          || (1 == Data1Index))  // FirstTop  Data2Index == 3
      {
        Data[0] = FirstSegData[3];
        Data[1] = FirstSegData[2];
        Data[2].x = 2 * FirstSegData[2].x - SecondSegData[2].x;
        Data[2].y = 2 * FirstSegData[2].y - SecondSegData[2].y;
        Data[3].x = 2 * FirstSegData[3].x - SecondSegData[3].x;
        Data[3].y = 2 * FirstSegData[3].y - SecondSegData[3].y;
      } else if ((2 == Data1Index)      // SecondBottom  Data2Index == 0
                 || (3 == Data1Index))  // SecondTop  Data2Index == 1
      {
        Data[0] = SecondSegData[3];
        Data[1] = SecondSegData[2];
        Data[2].x = 2 * SecondSegData[2].x - FirstSegData[2].x;
        Data[2].y = 2 * SecondSegData[2].y - FirstSegData[2].y;
        Data[3].x = 2 * SecondSegData[3].x - FirstSegData[3].x;
        Data[3].y = 2 * SecondSegData[3].y - FirstSegData[3].y;
      }
    } else  // right side slot
    {
      if ((0 == Data1Index)      // FirstBottom  Data2Index == 2
          || (1 == Data1Index))  // FirstTop  Data2Index == 3
      {
        Data[0].x = 2 * FirstSegData[0].x - SecondSegData[0].x;
        Data[0].y = 2 * FirstSegData[0].y - SecondSegData[0].y;
        Data[1].x = 2 * FirstSegData[1].x - SecondSegData[1].x;
        Data[1].y = 2 * FirstSegData[1].y - SecondSegData[1].y;
        Data[2] = FirstSegData[1];
        Data[3] = FirstSegData[0];
      } else if ((2 == Data1Index)      // SecondBottom  Data2Index == 0
                 || (3 == Data1Index))  // SecondTop  Data2Index == 1
      {
        Data[0].x = 2 * SecondSegData[0].x - FirstSegData[0].x;
        Data[0].y = 2 * SecondSegData[0].y - FirstSegData[0].y;
        Data[1].x = 2 * SecondSegData[1].x - FirstSegData[1].x;
        Data[1].y = 2 * SecondSegData[1].y - FirstSegData[1].y;
        Data[2] = SecondSegData[1];
        Data[3] = SecondSegData[0];
      }
    }
  }

  for (i = 0; i < 4; i++) {
    pCurSegData[i] = Data[i];
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutBuildSlotByOneSideNearbySlot="
             "==SlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
             "Label(%d)==slot_side(%d)==Data1Index(%d)",
             Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x, Data[2].y,
             Data[3].x, Data[3].y, Label, slot_side, Data1Index);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
BOOLEAN APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
    uint8_t* pLabel, APACoordinateDataCalFloatType* pData,
    uint8_t_INF* pNearBySlotNumsByAngled, uint8_t_INF* pNearBySlotNumsByLadder,
    uint8_t_INF* pData1Index, APACoordinateDataCalFloatType* pNearByCarPosSlot1,
    APACoordinateDataCalFloatType* pData1, uint8_t_INF* pSlotSideIndex1) {
  APACoordinateDataCalFloatType NearByCarPosSlot[4];
  APACoordinateDataCalFloatType NearByCarPosSlot1;
  APACoordinateDataCalFloatType NearByCarPosSlot2;
  uint8_t_INF m;
  uint8_t_INF k;
  APACoordinateDataCalFloatType Data[4];
  APACoordinateDataCalFloatType Data1[4];
  APACoordinateDataCalFloatType Data2[4];
  uint8_t_INF Data1Index;
  uint8_t_INF Data2Index;
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  BOOLEAN bResult1;
  APACarCoordinateDataCalFloatType CurCarPos;
  uint8_t_INF NearBySlotNumsByLadder;
  uint8_t_INF NearBySlotNumsByAngled;
  uint8_t_INF park_out_mode;
  uint8_t_INF slot_side;
  uint8_t_INF SlotSideIndex1;
  uint8_t_INF SlotSideIndex2;
  BOOLEAN bSearch;
  uint8_t Label;

  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    return FALSE;
  }
  bResult1 = FALSE;
  Data1Index = *pData1Index;
  Data2Index = 0;
  NearByCarPosSlot1 = *pNearByCarPosSlot1;
  NearBySlotNumsByLadder = *pNearBySlotNumsByLadder;
  NearBySlotNumsByAngled = *pNearBySlotNumsByAngled;
  slot_side = 0;
  SlotSideIndex1 = *pSlotSideIndex1;
  SlotSideIndex2 = 0;
  Label = *pLabel;
  for (k = 0; k < 4; k++) {
    Data[k] = pData[k];
    Data1[k] = pData1[k];
  }
  CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  bSearch = TRUE;
  while (bSearch) {
    NearByCarPosSlot[0].x = -(CarWidth + 500);  // FirstBottom
    NearByCarPosSlot[0].y = -1500;
    NearByCarPosSlot[1].x = -(CarWidth + 500);  // FirstTop
    NearByCarPosSlot[1].y = 1500;
    NearByCarPosSlot[2].x = -(2 * CarWidth + 1000);  // SecondBottom
    NearByCarPosSlot[2].y = -3000;
    NearByCarPosSlot[3].x = -(2 * CarWidth + 1000);  // SecondTop
    NearByCarPosSlot[3].y = 3000;
    // slot_side 0:left , 1:right
    if (SlotSideIndex1 == 1) {
      slot_side = 1;
    }
    if (slot_side == 1) {
      for (k = 0; k < 4; k++) {
        NearByCarPosSlot[k].x = -NearByCarPosSlot[k].x;
      }
    }

    if ((Label == Obj_Label_Angled_Slot) || (Label == Obj_Label_Ladder_Slot)) {
      for (k = 0; k < 4; k++) {
        NearByCarPosSlot[k] = AlgCom_PointPosWithAngAndCenterPt(
            NearByCarPosSlot[k], CurCarPos.CarAng, CurCarPos.Coordinate);
        bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&NearByCarPosSlot[k],
                                                           &Data[0], 4);
        if (TRUE == bResult1) {
          char log_string[512];
          snprintf(
              log_string, sizeof(log_string),
              "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot==Valid"
              "==bResult1(%d)==NearBySlotNumsByAngled(%d)=="
              "NearBySlotNumsByLadder(%d)==slot_side(%d)"
              "==NearByCarPosSlot:(k(%d)(%.2f,%.2f))",
              bResult1, NearBySlotNumsByAngled, NearBySlotNumsByLadder,
              slot_side, k, NearByCarPosSlot[k].x, NearByCarPosSlot[k].y);
          TLOG_INFO << log_string;
        }
        if (TRUE == bResult1) {
          // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
          if (NearBySlotNumsByLadder == 1) {
            if ((Label == Obj_Label_Perpen_Slot) ||
                (Label == Obj_Label_Parall_Slot)) {
              Label = Obj_Label_Ladder_Slot;
            }
          }
          if ((Label == Obj_Label_Angled_Slot) ||
              (Label == Obj_Label_Ladder_Slot)) {
            char log_string[512];
            snprintf(log_string, sizeof(log_string),
                     "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot="
                     "=Valid==Label(%d)",
                     Label);
            TLOG_INFO << log_string;
          }

          if (Label == Obj_Label_Angled_Slot) {
            NearBySlotNumsByAngled++;
          } else if (Label == Obj_Label_Ladder_Slot) {
            NearBySlotNumsByLadder++;
          }
          if ((NearBySlotNumsByAngled == 1) || (NearBySlotNumsByLadder == 1)) {
            // first slot
            for (m = 0; m < 4; m++) {
              memcpy(&Data1[m], &Data[m], sizeof(Data1[m]));
            }
            Data1Index = k;
            memcpy(&NearByCarPosSlot1, &NearByCarPosSlot[k],
                   sizeof(NearByCarPosSlot1));
            SlotSideIndex1 = slot_side;
          } else if ((NearBySlotNumsByAngled >= 2)  // 单边两个斜列车位
                     || (NearBySlotNumsByLadder >= 2))  // 单边两个斜列阶梯车位
          {
            Data2Index = k;
            SlotSideIndex2 = slot_side;
            {
              char log_string[512];
              snprintf(
                  log_string, sizeof(log_string),
                  "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot=="
                  "==Data1Index(%d)==Data2Index(%d)==SlotSideIndex1(%d)=="
                  "SlotSideIndex2(%d)",
                  Data1Index, Data2Index, SlotSideIndex1, SlotSideIndex2);
              TLOG_INFO << log_string;
            }
            if ((SlotSideIndex1 == SlotSideIndex2) &&
                (((Data1Index == 0) && (Data2Index == 2)) ||
                 ((Data1Index == 1) && (Data2Index == 3)) ||
                 ((Data1Index == 2) && (Data2Index == 0)) ||
                 ((Data1Index == 3) && (Data2Index == 1)))) {
              for (m = 0; m < 4; m++) {
                memcpy(&Data2[m], &Data[m], sizeof(Data2[m]));
              }
              memcpy(&NearByCarPosSlot2, &NearByCarPosSlot[k],
                     sizeof(NearByCarPosSlot2));
            } else {
              bResult1 = FALSE;
              continue;
            }
            // build CurCarPosSlot
            bResult1 = APAMap_ParkingOutBuildSlotByOneSideNearbySlot(
                &Data[0], &Data1[0], &Data2[0], NearByCarPosSlot1,
                NearByCarPosSlot2, Data1Index, Data2Index, Label, slot_side);
          }
          break;
        }
      }
    }
    if (FALSE == bResult1) {
      slot_side++;
    } else {
      bSearch = FALSE;
    }
    if (slot_side >= 2) {
      slot_side = 0;
      bSearch = FALSE;
    }
  }

  if (TRUE == bResult1) {
    if ((NearBySlotNumsByAngled < 2) && (NearBySlotNumsByLadder < 2)) {
      bResult1 = FALSE;
    }
  }
  if (TRUE == bResult1) {
    if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
      Label = Obj_Label_Angled_Slot;
    }
  }
  *pData1Index = Data1Index;
  *pNearByCarPosSlot1 = NearByCarPosSlot1;
  *pNearBySlotNumsByAngled = NearBySlotNumsByAngled;
  *pNearBySlotNumsByLadder = NearBySlotNumsByLadder;
  *pSlotSideIndex1 = SlotSideIndex1;
  *pLabel = Label;
  for (k = 0; k < 4; k++) {
    pData[k] = Data[k];
    pData1[k] = Data1[k];
  }
  if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot=="
             "bResult1(%d)==slot_side(%d)==Label(%d)="
             "=FirstSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)="
             "=NearByCarPosSlot1(%.2f,%.2f)==Data1Index(%d)="
             "=SecondSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)"
             "==NearByCarPosSlot2(%.2f,%.2f)==Data2Index(%d)=",
             bResult1, slot_side, Label, Data1[0].x, Data1[0].y, Data1[1].x,
             Data1[1].y, Data1[2].x, Data1[2].y, Data1[3].x, Data1[3].y,
             NearByCarPosSlot1.x, NearByCarPosSlot1.y, Data1Index, Data2[0].x,
             Data2[0].y, Data2[1].x, Data2[1].y, Data2[2].x, Data2[2].y,
             Data2[3].x, Data2[3].y, NearByCarPosSlot2.x, NearByCarPosSlot2.y,
             Data2Index);
    TLOG_INFO << log_string;
  }
  return bResult1;
}
BOOLEAN APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
    uint8_t* pLabel, APACoordinateDataCalFloatType* pData,
    uint8_t_INF* pNearBySlotNumsByAngled, uint8_t_INF* pNearBySlotNumsByLadder,
    uint8_t_INF* pNearBySlotNumsByPerpen, uint8_t_INF* pData1Index,
    APACoordinateDataCalFloatType* pNearByCarPosSlot1,
    APACoordinateDataCalFloatType* pData1) {
  APACoordinateDataCalFloatType NearByCarPosSlot[4];
  APACoordinateDataCalFloatType NearByCarPosSlot1;
  APACoordinateDataCalFloatType NearByCarPosSlot2;
  uint8_t_INF m;
  uint8_t_INF k;
  APACoordinateDataCalFloatType Data[4];
  APACoordinateDataCalFloatType Data1[4];
  APACoordinateDataCalFloatType Data2[4];
  uint8_t_INF Data1Index;
  uint8_t_INF Data2Index;
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
  BOOLEAN result;
  BOOLEAN bResult1;
  APACarCoordinateDataCalFloatType CurCarPos;
  uint8_t_INF NearBySlotNumsByLadder;
  uint8_t_INF NearBySlotNumsByAngled;
  uint8_t_INF NearBySlotNumsByPerpen;
  uint8_t_INF park_out_mode;
  uint8_t Label;
  APA_DISTANCE_CAL_FLOAT_TYPE Dis1, Dis2;
  APA_DISTANCE_CAL_FLOAT_TYPE Dis;

  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    return FALSE;
  }
  bResult1 = FALSE;
  Data1Index = *pData1Index;
  Data2Index = 0;
  NearByCarPosSlot1 = *pNearByCarPosSlot1;
  NearBySlotNumsByLadder = *pNearBySlotNumsByLadder;
  NearBySlotNumsByAngled = *pNearBySlotNumsByAngled;
  NearBySlotNumsByPerpen = *pNearBySlotNumsByPerpen;
  Label = *pLabel;
  for (k = 0; k < 4; k++) {
    Data[k] = pData[k];
    Data1[k] = pData1[k];
  }
  CarWidth = APAMap_ComCfg.WidthOfCar;  // mm
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  NearByCarPosSlot[0].x = -(CarWidth + 500);  // BottomLeft
  NearByCarPosSlot[0].y = -2000;
  NearByCarPosSlot[1].x = (CarWidth + 500);  // BottomRight
  NearByCarPosSlot[1].y = -2000;
  NearByCarPosSlot[2].x = (CarWidth + 500);  // TopRight
  NearByCarPosSlot[2].y = 2000;
  NearByCarPosSlot[3].x = -(CarWidth + 500);  // TopLeft
  NearByCarPosSlot[3].y = 2000;
  if ((Label == Obj_Label_Angled_Slot) || (Label == Obj_Label_Ladder_Slot) ||
      (Label == Obj_Label_Perpen_Slot) || (Label == Obj_Label_Parall_Slot)) {
    for (k = 0; k < 4; k++) {
      NearByCarPosSlot[k] = AlgCom_PointPosWithAngAndCenterPt(
          NearByCarPosSlot[k], CurCarPos.CarAng, CurCarPos.Coordinate);
      bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&NearByCarPosSlot[k],
                                                         &Data[0], 4);
      if (TRUE == bResult1) {
        char log_string[512];
        snprintf(
            log_string, sizeof(log_string),
            "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot==Valid"
            "==bResult1(%d)==NearBySlotNumsByAngled(%d)=="
            "NearBySlotNumsByLadder(%d)==NearBySlotNumsByPerpen(%d)==k(%d)",
            bResult1, NearBySlotNumsByAngled, NearBySlotNumsByLadder,
            NearBySlotNumsByPerpen, k);
        TLOG_INFO << log_string;
      }
      if (TRUE == bResult1) {
        // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
        if (NearBySlotNumsByLadder == 1) {
          if ((Label == Obj_Label_Perpen_Slot) ||
              (Label == Obj_Label_Parall_Slot)) {
            Label = Obj_Label_Ladder_Slot;
          }
        }
        // 代表当前周围车位为垂直车位，但车位与当前车辆的距离达到了阶梯车位水平的偏差，把车位类型修正为阶梯车位
        if (Label == Obj_Label_Perpen_Slot) {
          result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
              &Data[0], NearByCarPosSlot[k], CurCarPos.CarAng);
          if (TRUE == result) {
            Dis1 =
                (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg
                                                  .LenBetweenRAxisAndRBumper +
                                              APAMap_ComCfg.HalfWidthOfCar +
                                              300);
            Dis2 =
                (APA_DISTANCE_CAL_FLOAT_TYPE)(APAMap_ComCfg
                                                  .LenBetweenRAxisAndFBumper +
                                              APAMap_ComCfg.HalfWidthOfCar);
            if (k == 0) {  // BottomLeft
              Dis = AlgCom_GetTwoPointDisFloat(Data[2], CurCarPos.Coordinate);
              if (Dis > Dis1) {
                Label = Obj_Label_Ladder_Slot;
              }
            } else if (k == 1) {  // BottomRight
              Dis = AlgCom_GetTwoPointDisFloat(Data[1], CurCarPos.Coordinate);
              if (Dis > Dis1) {
                Label = Obj_Label_Ladder_Slot;
              }
            } else if (k == 2) {  // TopRight
              Dis = AlgCom_GetTwoPointDisFloat(Data[0], CurCarPos.Coordinate);
              if (Dis > Dis2) {
                Label = Obj_Label_Ladder_Slot;
              }
            } else if (k == 3) {  // TopLeft
              Dis = AlgCom_GetTwoPointDisFloat(Data[3], CurCarPos.Coordinate);
              if (Dis > Dis2) {
                Label = Obj_Label_Ladder_Slot;
              }
            }
          }
        }
        if ((Label == Obj_Label_Angled_Slot) ||
            (Label == Obj_Label_Ladder_Slot)) {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot=="
                   "Valid==Label(%d)",
                   Label);
          TLOG_INFO << log_string;
        }

        if (Label == Obj_Label_Angled_Slot) {
          NearBySlotNumsByAngled++;
        } else if (Label == Obj_Label_Ladder_Slot) {
          NearBySlotNumsByLadder++;
        } else {
          NearBySlotNumsByPerpen++;
        }

        if ((NearBySlotNumsByAngled == 1) || (NearBySlotNumsByLadder == 1) ||
            (NearBySlotNumsByPerpen == 1)) {
          for (m = 0; m < 4; m++) {
            memcpy(&Data1[m], &Data[m], sizeof(Data1[m]));
          }
          Data1Index = k;
          memcpy(&NearByCarPosSlot1, &NearByCarPosSlot[k],
                 sizeof(NearByCarPosSlot1));
          bResult1 = FALSE;
        } else if ((NearBySlotNumsByAngled >= 2)  // 左右两边各一个斜列车位
                   ||
                   (NearBySlotNumsByLadder >= 2)  // 左右两边各一个斜列阶梯车位
                   || (NearBySlotNumsByPerpen >= 2))  // 左右两边各一个垂直车位
        {
          Data2Index = k;
          if ((((Data1Index == 0) || (Data1Index == 3)) && (Data2Index != 0) &&
               ((Data2Index != 3))) ||
              (((Data1Index == 1) || (Data1Index == 2)) && (Data2Index != 1) &&
               ((Data2Index != 2)))) {
            for (m = 0; m < 4; m++) {
              memcpy(&Data2[m], &Data[m], sizeof(Data2[m]));
            }
            memcpy(&NearByCarPosSlot2, &NearByCarPosSlot[k],
                   sizeof(NearByCarPosSlot2));
          } else {
            bResult1 = FALSE;
            continue;
          }
          // build CurCarPosSlot
          bResult1 = APAMap_ParkingOutBuildSlotByTwoNearbySlot(
              &Data[0], &Data1[0], &Data2[0], NearByCarPosSlot1,
              NearByCarPosSlot2, Data1Index, Data2Index, Label);
        }
        break;
      }
    }
  }
  if (TRUE == bResult1) {
    if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2)) {
      Label = Obj_Label_Angled_Slot;
    } else if (NearBySlotNumsByPerpen >= 2) {
      Label = Obj_Label_Perpen_Slot;
    }
  }

  *pData1Index = Data1Index;
  *pNearByCarPosSlot1 = NearByCarPosSlot1;
  *pNearBySlotNumsByAngled = NearBySlotNumsByAngled;
  *pNearBySlotNumsByLadder = NearBySlotNumsByLadder;
  *pNearBySlotNumsByPerpen = NearBySlotNumsByPerpen;
  *pLabel = Label;
  for (k = 0; k < 4; k++) {
    pData[k] = Data[k];
    pData1[k] = Data1[k];
  }
  if ((NearBySlotNumsByAngled >= 2) || (NearBySlotNumsByLadder >= 2) ||
      (NearBySlotNumsByPerpen >= 2)) {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot==bResult1(%"
             "d)==Label(%d)="
             "=FirstSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)="
             "=NearByCarPosSlot1(%.2f,%.2f)==Data1Index(%d)="
             "=SecondSlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)"
             "==NearByCarPosSlot2(%.2f,%.2f)==Data2Index(%d)=",
             bResult1, Label, Data1[0].x, Data1[0].y, Data1[1].x, Data1[1].y,
             Data1[2].x, Data1[2].y, Data1[3].x, Data1[3].y,
             NearByCarPosSlot1.x, NearByCarPosSlot1.y, Data1Index, Data2[0].x,
             Data2[0].y, Data2[1].x, Data2[1].y, Data2[2].x, Data2[2].y,
             Data2[3].x, Data2[3].y, NearByCarPosSlot2.x, NearByCarPosSlot2.y,
             Data2Index);
    TLOG_INFO << log_string;
  }
  return bResult1;
}
BOOLEAN APAMap_ParkingOutBuildSlotByTwoNearbySlot(
    APACoordinateDataCalFloatType* pCurSegData,
    APACoordinateDataCalFloatType* pFirstSegData,
    APACoordinateDataCalFloatType* pSecondSegData,
    APACoordinateDataCalFloatType FirstNearByCarPosSlot,
    APACoordinateDataCalFloatType SecondNearByCarPosSlot,
    uint8_t_INF Data1Index, uint8_t_INF Data2Index, uint8_t Label) {
  APACoordinateDataCalFloatType Data[4];
  APACoordinateDataCalFloatType NSegment[2];
  uint8_t_INF i;
  APACarCoordinateDataCalFloatType CurCarPos;
  BOOLEAN result;
  APACoordinateDataCalFloatType TempCarPos;
  APALineParameterKBType LaneLineKBType;
  APALineParameterKBType LaneLineKBType2;
  APALineParameterABCType TempLine1;
  APALineParameterABCType TempLine2;
  APACoordinateDataCalFloatType FirstSegData[4];
  APACoordinateDataCalFloatType SecondSegData[4];

  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  for (i = 0; i < 4; i++) {
    Data[i] = pCurSegData[i];
    FirstSegData[i] = pFirstSegData[i];
    SecondSegData[i] = pSecondSegData[i];
  }
  result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
      &FirstSegData[0], FirstNearByCarPosSlot, CurCarPos.CarAng);
  if (TRUE == result) {
    result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
        &SecondSegData[0], SecondNearByCarPosSlot, CurCarPos.CarAng);
  }
  if (FALSE == result) {
    return FALSE;
  }

  if (Label == Obj_Label_Angled_Slot) {
    if ((0 == Data1Index)  // Data2Index == 1,2
        || (3 == Data1Index)) {
      Data[0] = FirstSegData[3];
      Data[1] = FirstSegData[2];
      Data[2] = SecondSegData[1];
      Data[3] = SecondSegData[0];
    } else if ((1 == Data1Index)  // Data2Index == 0,3
               || (2 == Data1Index)) {
      Data[0] = SecondSegData[3];
      Data[1] = SecondSegData[2];
      Data[2] = FirstSegData[1];
      Data[3] = FirstSegData[0];
    } else {
    }
  } else  // Obj_Label_Ladder_Slot || Obj_Label_Perpen_Slot
  {
    if (0 == Data1Index)  // BottomLeft
    {
      Data[1] = FirstSegData[2];
      Data[3] = SecondSegData[0];

      /***
       * 3        2
       * ----------
       * |        |
       * | First  |
       * |        31--------2
       * |        |        |
       * ---------|        |
       * 0       1|  Cur   |3-------2
       *          |        |        |
       *          0-------1|        |
       *                   | Second |
       *                   |        |
       *                   0--------1
       */

      NSegment[0] = FirstSegData[0];
      NSegment[1] = SecondSegData[0];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = FirstSegData[2];
      NSegment[1] = FirstSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[0].x = FirstSegData[2].x;
        Data[0].y = FirstSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[0] = TempCarPos;
      }

      NSegment[0] = FirstSegData[2];
      NSegment[1] = SecondSegData[2];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = SecondSegData[0];
      NSegment[1] = SecondSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[2].x = SecondSegData[0].x;
        Data[2].y = SecondSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[2] = TempCarPos;
      }
    } else if (1 == Data1Index)  // BottomRight
    {
      Data[0] = SecondSegData[3];
      Data[2] = FirstSegData[1];

      NSegment[0] = FirstSegData[1];
      NSegment[1] = SecondSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = SecondSegData[2];
      NSegment[1] = SecondSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[1].x = SecondSegData[2].x;
        Data[1].y = SecondSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[1] = TempCarPos;
      }

      NSegment[0] = FirstSegData[3];
      NSegment[1] = SecondSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = FirstSegData[0];
      NSegment[1] = FirstSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[3].x = FirstSegData[0].x;
        Data[3].y = FirstSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[3] = TempCarPos;
      }
    } else if (2 == Data1Index)  // TopRight
    {
      Data[1] = SecondSegData[2];
      Data[3] = FirstSegData[0];

      NSegment[0] = FirstSegData[0];
      NSegment[1] = SecondSegData[0];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = SecondSegData[2];
      NSegment[1] = SecondSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[0].x = SecondSegData[2].x;
        Data[0].y = SecondSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[0] = TempCarPos;
      }

      NSegment[0] = FirstSegData[2];
      NSegment[1] = SecondSegData[2];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = FirstSegData[0];
      NSegment[1] = FirstSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[2].x = FirstSegData[0].x;
        Data[2].y = FirstSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[2] = TempCarPos;
      }
    } else if (3 == Data1Index)  // TopLeft
    {
      Data[0] = FirstSegData[3];
      Data[2] = SecondSegData[1];

      NSegment[0] = FirstSegData[1];
      NSegment[1] = SecondSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = FirstSegData[2];
      NSegment[1] = FirstSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[1].x = FirstSegData[2].x;
        Data[1].y = FirstSegData[2].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[1] = TempCarPos;
      }

      NSegment[0] = FirstSegData[3];
      NSegment[1] = SecondSegData[3];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType);
      TempLine1 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType);
      NSegment[0] = SecondSegData[0];
      NSegment[1] = SecondSegData[1];
      APAMap_ParkingOutLineParABCbyPoints(&NSegment[0], 2, &LaneLineKBType2);
      if ((MATH_FABS(LaneLineKBType2.K) < 0.01) ||
          (MATH_FABS(LaneLineKBType2.K) > 500)) {
        Data[3].x = SecondSegData[0].x;
        Data[3].y = SecondSegData[0].x * LaneLineKBType.K + LaneLineKBType.B;
      } else {
        TempLine2 = AlgCom_LineParABCByLineParKBType(&LaneLineKBType2);
        AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempCarPos);
        Data[3] = TempCarPos;
      }
    } else {
    }
  }

  for (i = 0; i < 4; i++) {
    pCurSegData[i] = Data[i];
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutBuildSlotByTwoNearbySlot="
             "==SlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)",
             Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x, Data[2].y,
             Data[3].x, Data[3].y);
    TLOG_INFO << log_string;
  }
  return TRUE;
}
BOOLEAN APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(
    APACoordinateDataCalFloatType* pObj2Pt,
    APACoordinateDataCalFloatType* pObj1Pt,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj2Ang,
    APA_DISTANCE_CAL_FLOAT_TYPE* pObj1Ang,
    APA_DISTANCE_CAL_FLOAT_TYPE* pNewOrgAng,
    APA_DISTANCE_CAL_FLOAT_TYPE* pMaxSubLane,
    APA_DISTANCE_CAL_FLOAT_TYPE* pMaxSlotInnerX) {
  st_MapODDataType* pODInfo;
  BOOLEAN result;  // 搜索结果
  BOOLEAN bSearch;
  uint8_t_INF i;
  uint8_t_INF j;
  uint8_t_INF k;
  APACoordinateDataCalFloatType Data[4];
  Obj_Information_t CurObjComInfo;
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType Obj2Pt;
  APACoordinateDataCalFloatType Obj1Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE NewOrgAng;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Dis;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1Dis;
  APA_DISTANCE_CAL_FLOAT_TYPE CarOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MinSlotDpth;
  APA_DISTANCE_CAL_FLOAT_TYPE VPLSlotDpth;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxObj2Dis;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxObj1Dis;
  APA_DISTANCE_CAL_FLOAT_TYPE CloseLineOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSubLaneX;
  APA_DISTANCE_CAL_FLOAT_TYPE MoveObj2Dis;
  APA_DISTANCE_CAL_FLOAT_TYPE MoveObj1Dis;
  APACarCoordinateDataCalFloatType TempCarPos;
  APALineParameterABCType TopLine, BotLine, CloseLine;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType DataNearBy[20][4]; /* 附近车位坐标数据数组，最多存储20个车位，每个车位4个坐标点
                                                    */
  uint8_t_INF DataNearByNum; /* 附近车位数量 */
  BOOLEAN bResult1;
  uint8_t_INF LocLabel;
  APACoordinateDataCalFloatType LocSlotData[4];
  APACoordinateDataCalFloatType pVPLSlotPts[4];
  APACoordinateDataCalFloatType pTempVPLSlotPts[4];
  APA_ENUM_TYPE OrgIndex[4];
  APA_ENUM_TYPE TempOrgIndex[4];
  APACoordinateDataCalFloatType TempPt;
  APA_ENUM_TYPE FailCause;
  APALineParameterABCType TempLine;
  APALineParameterABCType TempLine1;
  BOOLEAN bCheckIfLadderSlot;
  uint8_t_INF park_out_mode;
  uint8_t_INF NearByTwoSideSlotNumsByAngled;  /* 附近两侧斜列车位数量 */
  uint8_t_INF NearByTwoSideSlotNumsByLadder;  /* 车位两侧阶梯车位数量 */
  uint8_t_INF NearByTwoSideSlotNumsByLadder1; /* 车位两侧阶梯车位数量 */
  uint8_t_INF NearByTwoSideSlotNumsByPerpen;  /* 车位两侧垂直车位数量 */
  uint8_t_INF NearByOneSideSlotNumsByAngled;  /* 车位单侧斜列车位数量 */
  uint8_t_INF NearByOneSideSlotNumsByLadder;  /*邻近单侧邻近阶梯车位的数量 */
  uint8_t_INF NearByOneSideSlotNumsByLadder1; /* 单侧邻近阶梯车位的数量 */
  uint8_t_INF Data1Index;
  uint8_t_INF Data1IndexByOneSide;
  APACoordinateDataCalFloatType NearByCarPosSlot1;
  APACoordinateDataCalFloatType NearByCarPosSlot1ByOneSide;
  APACoordinateDataCalFloatType Data1[4];
  APACoordinateDataCalFloatType Data2[4];
  uint8_t_INF slot_side;
  APACoordinateDataCalFloatType getVPLSlotData[SlotPtNum];
  APACoordinateDataCalFloatType getObj1Pt;
  APACoordinateDataCalFloatType getObj2Pt;
  APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj1LineDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CarSideToObj2LineDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CurSlotTopLineAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE CurSlotCloseLineAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1MoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2MoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CloseLineMoveDis;
  APA_DISTANCE_CAL_FLOAT_TYPE SlotOutsideDis;
  APA_DISTANCE_CAL_FLOAT_TYPE SlotInnerDis;
  static BOOLEAN bObjLabelAngledFlag = FALSE;  // 斜列车位框标志位
  BOOLEAN bBuildAngledByNeaybyFlag;  // 可构造斜列或阶梯斜列标志位
  BOOLEAN bCurAngledFlag;  // 自身车位类型为斜列车位类型标志位
  BOOLEAN bBuildIfLadderSlotFlag;  // 阶梯车位类型建立斜列阶梯车位成功与否标志位
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
#if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
  APACoordinateDataCalFloatType SlotATemp,SlotBTemp,SlotCTemp,SlotDTemp;
  //APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal1;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal2;
  APA_DISTANCE_CAL_FLOAT_TYPE SafeDisCal3;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarWidth;
#endif
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return FALSE;
  }
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  pODInfo = &APAMap_GInputData.TotalMapInfo.mapData.ODInfo;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  NewOrgAng = 0;
  NearByTwoSideSlotNumsByAngled = 0;
  NearByTwoSideSlotNumsByLadder1 = 0;
  NearByTwoSideSlotNumsByPerpen = 0;
  NearByOneSideSlotNumsByAngled = 0;
  NearByOneSideSlotNumsByLadder1 = 0;
  Data1Index = 0;
  Data1IndexByOneSide = 0;
  NearByCarPosSlot1.x = 0;
  NearByCarPosSlot1.y = 0;
  NearByCarPosSlot1ByOneSide.x = 0;
  NearByCarPosSlot1ByOneSide.y = 0;
  DataNearByNum = 0;
  slot_side = 0;
  bResult1 = FALSE;
  result = FALSE;
  FailCause = 0;
  LocLabel = Obj_Label_Parall_Slot;
  bCheckIfLadderSlot = FALSE;
  bBuildIfLadderSlotFlag = FALSE;
  bBuildAngledByNeaybyFlag = FALSE;
  bCurAngledFlag = FALSE;
  CurObjComInfo = pODInfo->Square.Quadrilaterals[0].ObjInfo;
  for (k = 0; k < 4; k++) {
    LocSlotData[k].x = 0;
    LocSlotData[k].y = 0;
    Data1[k].x = 0;
    Data1[k].y = 0;
    Data2[k].x = 0;
    Data2[k].y = 0;
  }
  for (k = 0; k < SlotPtNum; k++) {
    getVPLSlotData[k].x = 0.0;
    getVPLSlotData[k].y = 0.0;
  }
  getObj1Pt.x = 0.0;
  getObj1Pt.y = 0.0;
  getObj2Pt.x = 0.0;
  getObj2Pt.x = 0.0;
  CarSideToObj1LineDis = 0.0;
  CarSideToObj2LineDis = 0.0;
  CurSlotTopLineAngle = 0.0;
  CurSlotCloseLineAngle = 0.0;
  Obj1MoveDis = 0.0;
  Obj2MoveDis = 0.0;
  CloseLineMoveDis = 0.0;
  SlotOutsideDis = 0.0;
  SlotInnerDis = 0.0;
#if 0  // def SUPPORT_PARKING_OUT_SYSTEM_TEST
  CarWidth = APAMap_ComCfg.WidthOfCar; //mm
  //SafeDisCal1 = APAMap_ComCfg.ObjInSlotMinSafeDis[0]; // 250mm, 0 paralIn;
  SafeDisCal2 = APAMap_ComCfg.ObjInSlotMinSafeDis[1]; // 400mm, 1 PerpIn;
  CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper; //mm, 800
  CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper; //mm, 3000
#if 0
  if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL){
    SafeDisCal3 = 0;
  } else {
    SafeDisCal3 = 200;
  }  
  SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 + SafeDisCal3;
  SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2 + SafeDisCal3);
  SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2 + SafeDisCal3);
  SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 + SafeDisCal3;
#else  // 仿真模拟斜列式车位
#if 0
  SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 + 1000;
  SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2);
  SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000);
  SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2;
#else
#if 0  // 仿真模拟车辆左边斜列车位
  SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotATemp.y = (CarLFCal + SafeDisCal2 + 1000) + 1000;
  SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
  SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) + 1000;
  SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotDTemp.y = (CarLFCal + SafeDisCal2) + 1000;
#else  // 仿真模拟阶梯斜列车位 left
#if 1
  SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotATemp.y = (CarLFCal + SafeDisCal2) + 1000;
  SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
  SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) + 1000;
  SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotDTemp.y = (CarLFCal + SafeDisCal2) + 1000;
#else  // right
  SlotATemp.x = ((CarWidth / 2) +  SafeDisCal2);
  SlotATemp.y = (CarLFCal + SafeDisCal2) - 1000;
  SlotBTemp.x = ((CarWidth / 2) +  SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
#endif
#endif
#endif
#endif
  SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL){
    //pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Perpen_Slot;
    //pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Angled_Slot;
    pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Ladder_Slot;
  } else {
    pODInfo->Square.Quadrilaterals[0].ObjInfo.Label = Obj_Label_Parall_Slot;
  }  
  pODInfo->Square.Quadrilaterals[0].Points.Point_1.x = SlotATemp.x;
  pODInfo->Square.Quadrilaterals[0].Points.Point_1.y = SlotATemp.y;
  pODInfo->Square.Quadrilaterals[0].Points.Point_2.x = SlotBTemp.x;
  pODInfo->Square.Quadrilaterals[0].Points.Point_2.y = SlotBTemp.y;
  pODInfo->Square.Quadrilaterals[0].Points.Point_3.x = SlotCTemp.x;
  pODInfo->Square.Quadrilaterals[0].Points.Point_3.y = SlotCTemp.y;
  pODInfo->Square.Quadrilaterals[0].Points.Point_4.x = SlotDTemp.x;
  pODInfo->Square.Quadrilaterals[0].Points.Point_4.y = SlotDTemp.y;
  pODInfo->Square.ObjNum = 1;
#if 1  // 第二个车位
#if 1  // 仿真当前垂直车位
  SlotATemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2;
  SlotBTemp.x = -((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2);
  SlotCTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2);
  SlotDTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2;
#else
#if 0  // 仿真模拟车辆右边斜列车位
  SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 + 1000 - 1000;
  SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) - 1000;
  SlotDTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
  SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
#else
#if 1  // 仿真模拟阶梯斜列车位 right
  SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
  SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
#else  // 仿真模拟阶梯斜列车位 left
#if 0
  SlotATemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
  SlotBTemp.x = -(3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotDTemp.x = -((CarWidth / 2) +  SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
#else
#if 0  // 仿真模拟阶梯斜列车位单边第二个 left
  SlotATemp.x = -(5 * (CarWidth / 2) + 4 * SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 + 2000;
  SlotBTemp.x = -(5 * (CarWidth / 2) + 4 * SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) + 2000;
  SlotCTemp.x = -(3 * (CarWidth / 2) +  3 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) + 2000;
  SlotDTemp.x = -(3 * (CarWidth / 2) +  3 * SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 + 2000;
#else  // 仿真模拟阶梯斜列车位单边第二个 right
  SlotATemp.x = (3 * (CarWidth / 2) + 3 * SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 - 2000;
  SlotBTemp.x = (3 * (CarWidth / 2) + 3 * SafeDisCal2 - 100);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 2000;
  SlotCTemp.x = (5 * (CarWidth / 2) +  4 * SafeDisCal2 - 100);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 2000;
  SlotDTemp.x = (5 * (CarWidth / 2) +  4 * SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 - 2000;
#endif
#endif
#endif
#endif
#endif
  SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Perpen_Slot;
  //pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Angled_Slot;
  //pODInfo->Square.Quadrilaterals[1].ObjInfo.Label = Obj_Label_Ladder_Slot;
  pODInfo->Square.Quadrilaterals[1].Points.Point_1.x = SlotATemp.x;
  pODInfo->Square.Quadrilaterals[1].Points.Point_1.y = SlotATemp.y;
  pODInfo->Square.Quadrilaterals[1].Points.Point_2.x = SlotBTemp.x;
  pODInfo->Square.Quadrilaterals[1].Points.Point_2.y = SlotBTemp.y;
  pODInfo->Square.Quadrilaterals[1].Points.Point_3.x = SlotCTemp.x;
  pODInfo->Square.Quadrilaterals[1].Points.Point_3.y = SlotCTemp.y;
  pODInfo->Square.Quadrilaterals[1].Points.Point_4.x = SlotDTemp.x;
  pODInfo->Square.Quadrilaterals[1].Points.Point_4.y = SlotDTemp.y;
  pODInfo->Square.ObjNum = 2;
#endif
#if 1  // 第三个车位
#if 0  // 仿真模拟阶梯斜列车位 right
  SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 - 1000;
  SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotDTemp.x = (3 * (CarWidth / 2) + 2 * SafeDisCal2);
  SlotDTemp.y = CarLFCal + SafeDisCal2 - 1000;
#else
#if 1  // 仿真模拟车辆右边斜列车位
  SlotATemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotATemp.y = CarLFCal + SafeDisCal2 + 1000 - 1000;
  SlotBTemp.x = ((CarWidth / 2) + SafeDisCal2);
  SlotBTemp.y = -(CarLRCal + SafeDisCal2) - 1000;
  SlotCTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
  SlotCTemp.y = -(CarLRCal + SafeDisCal2 + 1000) - 1000;
  SlotDTemp.x = (3 * (CarWidth / 2) +  2 * SafeDisCal2);
  SlotDTemp.y = (CarLFCal + SafeDisCal2) - 1000;
#endif
#endif
  SlotATemp = AlgCom_PointPosWithAngAndCenterPt(SlotATemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotBTemp = AlgCom_PointPosWithAngAndCenterPt(SlotBTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotCTemp = AlgCom_PointPosWithAngAndCenterPt(SlotCTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  SlotDTemp = AlgCom_PointPosWithAngAndCenterPt(SlotDTemp, CurCarPos.CarAng,
                                      CurCarPos.Coordinate);
  //pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Perpen_Slot;
  pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Angled_Slot;
  //pODInfo->Square.Quadrilaterals[2].ObjInfo.Label = Obj_Label_Ladder_Slot;
  pODInfo->Square.Quadrilaterals[2].Points.Point_1.x = SlotATemp.x;
  pODInfo->Square.Quadrilaterals[2].Points.Point_1.y = SlotATemp.y;
  pODInfo->Square.Quadrilaterals[2].Points.Point_2.x = SlotBTemp.x;
  pODInfo->Square.Quadrilaterals[2].Points.Point_2.y = SlotBTemp.y;
  pODInfo->Square.Quadrilaterals[2].Points.Point_3.x = SlotCTemp.x;
  pODInfo->Square.Quadrilaterals[2].Points.Point_3.y = SlotCTemp.y;
  pODInfo->Square.Quadrilaterals[2].Points.Point_4.x = SlotDTemp.x;
  pODInfo->Square.Quadrilaterals[2].Points.Point_4.y = SlotDTemp.y;
  pODInfo->Square.ObjNum = 3;
#endif
#endif
  // First
  // Search,先预遍历一遍，从前向后搜索，找出当前车辆周围是否存在斜列或阶梯斜列车位
  bSearch = TRUE;
  i = 0;
  j = 0;
  while (bSearch) {
    if (j == 0) {
      while (i < pODInfo->Square.ObjNum) {
        CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
        if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Angled_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Parall_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Ladder_Slot)) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Square.ObjNum) {
        Data[0].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.x;
        Data[0].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.y;
        Data[1].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.x;
        Data[1].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.y;
        Data[2].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.x;
        Data[2].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.y;
        Data[3].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.x;
        Data[3].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.y;
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==ParkOutSlotInfoFromTotalMap==First==SlotPt:"
                   "0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
                   "CurLabel(%d)",
                   Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x,
                   Data[2].y, Data[3].x, Data[3].y, CurObjComInfo.Label);
          TLOG_INFO << log_string;
        }
        i++;
      } else {
        j++;
        i = 0;
      }
    }
    if (j == 1) {
      bSearch = FALSE;
    }
    if (bSearch == TRUE) {
      LocLabel = CurObjComInfo.Label;
      bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
          &LocLabel, &Data[0], &NearByTwoSideSlotNumsByAngled,
          &NearByTwoSideSlotNumsByLadder1, &NearByTwoSideSlotNumsByPerpen,
          &Data1Index, &NearByCarPosSlot1, &Data1[0]);
      if (FALSE == bResult1) {
        bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
            &LocLabel, &Data[0], &NearByOneSideSlotNumsByAngled,
            &NearByOneSideSlotNumsByLadder1, &Data1IndexByOneSide,
            &NearByCarPosSlot1ByOneSide, &Data2[0], &slot_side);
      }
      if ((TRUE == bResult1) && (NearByTwoSideSlotNumsByPerpen < 2)) {
        bBuildAngledByNeaybyFlag = TRUE;
      }
      bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(&CurCarPos.Coordinate,
                                                         &Data[0], 4);
      if ((CurObjComInfo.Label == Obj_Label_Angled_Slot) &&
          (TRUE == bResult1)) {
        bCurAngledFlag = TRUE;
      }
    }
  }
  // Second Search,开始正式遍历，从前向后搜索，找出当前车位，没有则造车位
  NearByTwoSideSlotNumsByAngled = 0;
  NearByTwoSideSlotNumsByLadder = 0;
  NearByTwoSideSlotNumsByPerpen = 0;
  NearByOneSideSlotNumsByAngled = 0;
  NearByOneSideSlotNumsByLadder = 0;
  Data1Index = 0;
  Data1IndexByOneSide = 0;
  NearByCarPosSlot1.x = 0;
  NearByCarPosSlot1.y = 0;
  NearByCarPosSlot1ByOneSide.x = 0;
  NearByCarPosSlot1ByOneSide.y = 0;
  slot_side = 0;
  for (k = 0; k < 4; k++) {
    Data1[k].x = 0;
    Data1[k].y = 0;
    Data2[k].x = 0;
    Data2[k].y = 0;
  }
  bSearch = TRUE;
  bResult1 = FALSE;
  i = 0;
  j = 0;
  while (bSearch) {
    if (j == 0) {
      while (i < pODInfo->Square.ObjNum) {
        CurObjComInfo = pODInfo->Square.Quadrilaterals[i].ObjInfo;
        if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Angled_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Parall_Slot) ||
            (CurObjComInfo.Label == Obj_Label_Ladder_Slot)) {
          break;
        }
        i++;
      }
      if (i < pODInfo->Square.ObjNum) {  // 获取车位信息 Square.Quadrilaterals
        Data[0].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.x;
        Data[0].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_1.y;
        Data[1].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.x;
        Data[1].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_2.y;
        Data[2].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.x;
        Data[2].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_3.y;
        Data[3].x =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.x;
        Data[3].y =
            (APA_DISTANCE_CAL_FLOAT_TYPE)pODInfo->Square.Quadrilaterals[i]
                .Points.Point_4.y;
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==ParkOutSlotInfoFromTotalMap==Second==SlotPt:"
                   "0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f)=="
                   "CurLabel(%d)",
                   Data[0].x, Data[0].y, Data[1].x, Data[1].y, Data[2].x,
                   Data[2].y, Data[3].x, Data[3].y, CurObjComInfo.Label);
          TLOG_INFO << log_string;
        }
        i++;
      } else {  // 搜索结束
        j++;
        i = 0;
      }
    }
    if (j == 1) {
      bSearch = FALSE;
    }
    if (bSearch == TRUE) {
      // 判断是否满足条件：没有真实的融合OD斜列车位类型车位框存在。则允许由周围车位框构造当前车位框。
      if (FALSE == bCurAngledFlag) {
        bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot(
            &CurObjComInfo.Label, &Data[0], &NearByTwoSideSlotNumsByAngled,
            &NearByTwoSideSlotNumsByLadder, &NearByTwoSideSlotNumsByPerpen,
            &Data1Index, &NearByCarPosSlot1, &Data1[0]);
        if (FALSE == bResult1) {
          bResult1 = APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot(
              &CurObjComInfo.Label, &Data[0], &NearByOneSideSlotNumsByAngled,
              &NearByOneSideSlotNumsByLadder, &Data1IndexByOneSide,
              &NearByCarPosSlot1ByOneSide, &Data2[0], &slot_side);
        }
        if (TRUE == bResult1) {
          bCheckIfLadderSlot = FALSE;
          result = FALSE;
        }
      }
      if (FALSE == bResult1) {
        bResult1 = AlgCom_CheckIfGivenPtInthePolygonRegion(
            &CurCarPos.Coordinate, &Data[0], 4);
        // 代表当前车位误识别成垂直、水平，但附近车位识别为阶梯斜列的情况，把当前车位类型修改为阶梯斜列
        if (((CurObjComInfo.Label == Obj_Label_Perpen_Slot) ||
             (CurObjComInfo.Label == Obj_Label_Parall_Slot)) &&
            (TRUE == bResult1)) {
          if ((NearByTwoSideSlotNumsByLadder1 > 0) ||
              (NearByOneSideSlotNumsByLadder1 >
               0)) { /* * 判断附近一侧是否有可用车位 *
                        条件：检查NearByOneSideSlotNumsByLadder1变量是否大于0 *
                        如果大于0，表示存在可用车位，条件成立 */
            CurObjComInfo.Label = Obj_Label_Ladder_Slot;
          }
        }
        // 代表当前车位误识别成垂直，但根据周围车位可以构造出当前斜列车位类型的情况，把返回值置成FALSE，继续搜索附近斜列车位来构造斜列车位框
        if ((CurObjComInfo.Label == Obj_Label_Perpen_Slot) &&
            (TRUE == bResult1)) {
          if (TRUE == bBuildAngledByNeaybyFlag) {
            bResult1 = FALSE;
          }
        }
#ifdef SUPPORT_PARKING_OUT_DEBUG
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==Test==1111=======bResult(%d)==bResult1(%d)=="
                   "bBuildAngledByNeaybyFlag(%d)==Label(%d)",
                   result, bResult1, bBuildAngledByNeaybyFlag,
                   CurObjComInfo.Label);
          TLOG_INFO << log_string;
        }
#endif
      }
      if (bResult1 == TRUE) {  // 车辆位置在车位内
#ifdef SUPPORT_PARKING_OUT_DEBUG
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==Test==2222=======bResult(%d)==bResult1(%d)=="
                   "DataNearByNum(%d)==Label(%d)",
                   result, bResult1, DataNearByNum, CurObjComInfo.Label);
          TLOG_INFO << log_string;
        }
#endif
        if (result == FALSE) {
          result = TRUE;  // 为什么搜索结束后 这里要置 True?
          LocLabel = CurObjComInfo.Label;
          for (k = 0; k < 4; k++) {
            LocSlotData[k] = Data[k];
          }
          if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
            if (LocLabel == Obj_Label_Ladder_Slot) {
              bObjLabelAngledFlag = TRUE;
              s_parking_out_state.flags.obj_label_ladder = TRUE;
            } else if (LocLabel == Obj_Label_Angled_Slot) {
              bObjLabelAngledFlag = TRUE;
              s_parking_out_state.flags.obj_label_ladder = FALSE;
            } else {
              s_parking_out_state.flags.obj_label_ladder = FALSE;
              bObjLabelAngledFlag = FALSE;
            }
          }
#ifdef APA_MAP_PARKOUT_LADDER_SLOT
          if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
              (LocLabel == Obj_Label_Ladder_Slot)) {
            bCheckIfLadderSlot = TRUE;
          }
#endif
#ifdef SUPPORT_PARKING_OUT_DEBUG
          {
            char log_string[512];
            snprintf(log_string, sizeof(log_string),
                     "==Test==3333=======bResult(%d)==bResult1(%d)=="
                     "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
                     result, bResult1, DataNearByNum, bCheckIfLadderSlot);
            TLOG_INFO << log_string;
          }
#endif
        }
      } else {
        if (DataNearByNum >= 20) {
          break;
        }
        for (k = 0; k < 4; k++) {
          DataNearBy[DataNearByNum][k] = Data[k];
        }
        DataNearByNum++;
#ifdef SUPPORT_PARKING_OUT_DEBUG
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==Test==4444=======bResult(%d)==bResult1(%d)=="
                   "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
                   result, bResult1, DataNearByNum, bCheckIfLadderSlot);
          TLOG_INFO << log_string;
        }
#endif
      }
    }
    if (result == TRUE) {
#ifdef SUPPORT_PARKING_OUT_DEBUG
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==Test==5555=======bResult(%d)==bResult1(%d)==DataNearByNum(%"
                 "d)==bCheckIfLadderSlot(%d)",
                 result, bResult1, DataNearByNum, bCheckIfLadderSlot);
        TLOG_INFO << log_string;
      }
#endif
      if (bCheckIfLadderSlot == FALSE) {
#ifdef SUPPORT_PARKING_OUT_DEBUG
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==Test==6666=======bResult(%d)==bResult1(%d)=="
                   "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
                   result, bResult1, DataNearByNum, bCheckIfLadderSlot);
          TLOG_INFO << log_string;
        }
#endif
        break;
      } else {
#ifdef SUPPORT_PARKING_OUT_DEBUG
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==Test==7777=======bResult(%d)==bResult1(%d)=="
                   "DataNearByNum(%d)==bCheckIfLadderSlot(%d)",
                   result, bResult1, DataNearByNum, bCheckIfLadderSlot);
          TLOG_INFO << log_string;
        }
#endif
        bResult1 = FALSE;
        // continue search;
      }
    }
  }
  if (result == TRUE) {
    if ((bObjLabelAngledFlag == FALSE) &&
        ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
         (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_GO_STRAIGHT))) {
      result = FALSE;  // 车头、车尾直出就不需要继续搜索了
    }
    if (result == TRUE) {
      result = AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
          &LocSlotData[0], CurCarPos.Coordinate, CurCarPos.CarAng);
      if (result == FALSE) {
        FailCause = 3;
      }
    } else {
      FailCause = 2;
    }
  } else {
    FailCause = 1;
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==ParkOutGetLocSlotFromTotalMap==Result(%d)==Cause(%d)"
             "=LocSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),"
             "LocLabel(%d),LadderSlot(%d),SlotNumNearBy(%d)",
             result, FailCause, LocSlotData[0].x, LocSlotData[0].y,
             LocSlotData[1].x, LocSlotData[1].y, LocSlotData[2].x,
             LocSlotData[2].y, LocSlotData[3].x, LocSlotData[3].y, LocLabel,
             bCheckIfLadderSlot, DataNearByNum);
    TLOG_INFO << log_string;
  }
  if ((result == TRUE) && (bCheckIfLadderSlot == TRUE)) {
    if (DataNearByNum > 0) {
      APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(  //  根据泊车模式重新排序VPL车位点
          &LocSlotData[0], &pVPLSlotPts[0], &OrgIndex[0]);
      AlgCom_GetAngByTwoPts(pVPLSlotPts[1], pVPLSlotPts[0],
                            &NewOrgAng);  //  通过两点计算角度，获取新的原始角度
      AlgCom_GetAngByTwoPts(pVPLSlotPts[3], pVPLSlotPts[0], &Obj2Ang);
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==ParkOutLocVPLPts===pVPLSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),"
                 "2(%.2f,%.2f),3(%.2f,%.2f)==OrgIndex(%d,%d,%d,%d)"
                 "==CloseLineAng(%.2f)==Obj2Ang(%.2f)==NearBySlotNum(%d)",
                 pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
                 pVPLSlotPts[1].y, pVPLSlotPts[2].x, pVPLSlotPts[2].y,
                 pVPLSlotPts[3].x, pVPLSlotPts[3].y, OrgIndex[0], OrgIndex[1],
                 OrgIndex[2], OrgIndex[3], NewOrgAng * 180.0 / PI,
                 Obj2Ang * 180.0 / PI, DataNearByNum);
        TLOG_INFO << log_string;
      }
      for (i = 0; i < DataNearByNum; i++) {
        FailCause = 0;
        if (TRUE == AlgCom_LineParABCbyTwoPoints(DataNearBy[i][0],
                                                 DataNearBy[i][2], &TempLine)) {
          if (TRUE == AlgCom_LineParABCbyTwoPoints(
                          DataNearBy[i][1], DataNearBy[i][3], &TempLine1)) {
            if (1 ==
                AlgCom_CrossPointOfTwoLines(&TempLine, &TempLine1, &TempPt)) {
              if (TRUE == AlgCom_ReOrderRectPtsByGivenOrgPtAndAng(
                              &DataNearBy[i][0], TempPt, CurCarPos.CarAng)) {
                APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode(
                    &DataNearBy[i][0], &pTempVPLSlotPts[0], &TempOrgIndex[0]);
                if (TRUE == APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot(
                                &pVPLSlotPts[0], &pVPLSlotPts[1], &NewOrgAng,
                                Obj2Ang, &pTempVPLSlotPts[0],
                                slot_data_at_right_side, &FailCause)) {
                  for (k = 0; k < 4; k++) {
                    LocSlotData[OrgIndex[k]] = pVPLSlotPts[k];
                  }
                  bBuildIfLadderSlotFlag = TRUE;
                  char log_string[512];
                  snprintf(log_string, sizeof(log_string),
                           "==ParkOutCheckIfTargetSlotIsLadderSlotSuccess!"
                           "=NearBySlotPt:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%."
                           "2f),3(%.2f,%.2f))"
                           "=OrdVPLSlotPts:0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%."
                           "2f),3(%.2f,%.2f))"
                           "==NewObj2(%.2f,%.2f)==NewObj1(%.2f,%.2f)=="
                           "NewOrgAng(%.2f)",
                           DataNearBy[i][0].x, DataNearBy[i][0].y,
                           DataNearBy[i][1].x, DataNearBy[i][1].y,
                           DataNearBy[i][2].x, DataNearBy[i][2].y,
                           DataNearBy[i][3].x, DataNearBy[i][3].y,
                           pTempVPLSlotPts[0].x, pTempVPLSlotPts[0].y,
                           pTempVPLSlotPts[1].x, pTempVPLSlotPts[1].y,
                           pTempVPLSlotPts[2].x, pTempVPLSlotPts[2].y,
                           pTempVPLSlotPts[3].x, pTempVPLSlotPts[3].y,
                           pVPLSlotPts[0].x, pVPLSlotPts[0].y, pVPLSlotPts[1].x,
                           pVPLSlotPts[1].y, NewOrgAng);
                  TLOG_INFO << log_string;
                  break;
                }
              } else {
                FailCause = 0x40;
              }
            } else {
              FailCause = 0x30;
            }
          } else {
            FailCause = 0x20;
          }
        } else {
          FailCause = 0x10;
        }
        {
          char log_string[512];
          snprintf(log_string, sizeof(log_string),
                   "==ParkOutCheckIfTargetSlotIsLadderSlotFailCase(%d)==="
                   "NearBySlotPt:0(%.2f,%.2f),1(%.2f,%.2f)"
                   "2(%.2f,%.2f),3(%.2f,%.2f)",
                   FailCause, DataNearBy[i][0].x, DataNearBy[i][0].y,
                   DataNearBy[i][1].x, DataNearBy[i][1].y, DataNearBy[i][2].x,
                   DataNearBy[i][2].y, DataNearBy[i][3].x, DataNearBy[i][3].y);
          TLOG_INFO << log_string;
        }
      }
    }
    if (bBuildIfLadderSlotFlag == FALSE) {
      s_parking_out_state.flags.obj_label_ladder = FALSE;
      bObjLabelAngledFlag = FALSE;
    }
  }
  if (result == TRUE) {
    result = APAMap_ParkingOutGetSlotInfoFromVPLSlotPts(
        &LocSlotData[0], &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &NewOrgAng,
        &Obj2Dis, &Obj1Dis, &CarOffsetX, &MinSlotDpth, &VPLSlotDpth);
    if (result == TRUE) {
      for (i = 0; i < 4; i++) {
        getVPLSlotData[i] = LocSlotData[i];
      }
      getObj1Pt = Obj1Pt;
      getObj2Pt = Obj2Pt;
      CurSlotTopLineAngle = Obj2Ang;
      CurSlotCloseLineAngle = NewOrgAng;
      CarSideToObj1LineDis = Obj1Dis;
      CarSideToObj2LineDis = Obj2Dis;
#ifdef SUPPORT_ELECTRONIC_FENCE_MAP
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==FirstParkingOutElectrFenceMapBulid");
        TLOG_INFO << log_string;
      }
      APAMap_ParkingOutElectrFenceMapBulid(
          &getVPLSlotData[0], &getObj1Pt, &getObj2Pt, &CarSideToObj1LineDis,
          &CarSideToObj2LineDis, &CurSlotTopLineAngle, &CurSlotCloseLineAngle,
          &Obj1MoveDis, &Obj2MoveDis, &CloseLineMoveDis, &SlotOutsideDis,
          &SlotInnerDis);
      MaxObj2Dis = Obj2MoveDis;
      MaxObj1Dis = Obj1MoveDis;
      CloseLineOffsetX = CloseLineMoveDis;
      MaxSubLaneX = SlotOutsideDis;
// MinSlotDpth = SlotInnerDis;
#else
      MaxObj2Dis = 1000;  // 500;
      MaxObj1Dis = 1000;  // 500;
      CloseLineOffsetX = 0;
      if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
        MaxSubLaneX = 5000;
      } else {
        MaxSubLaneX = 7000;
      }
#endif
      MoveObj2Dis = MaxObj2Dis - Obj2Dis;  //>0 make slot big;
      MoveObj1Dis = MaxObj1Dis - Obj1Dis;  //>0 need makeslot big;
      TempCarPos.Coordinate = Obj2Pt;
      TempCarPos.CarAng = Obj2Ang;
      TopLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
      TempCarPos.Coordinate = Obj1Pt;
      TempCarPos.CarAng = Obj1Ang;
      BotLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
      TempCarPos.Coordinate = Obj2Pt;
      TempCarPos.CarAng = NewOrgAng;
      CloseLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
      // move obj2 and obj1;
      if (FALSE == slot_data_at_right_side) {
        MoveObj2Dis = -MoveObj2Dis;
        MoveObj1Dis = -MoveObj1Dis;
        CloseLineOffsetX = -CloseLineOffsetX;
      }
      TopLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
          &TopLine, Obj2Ang, MoveObj2Dis);
      BotLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
          &BotLine, Obj1Ang, -MoveObj1Dis);
      CloseLine = AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(
          &CloseLine, NewOrgAng, -CloseLineOffsetX);
      AlgCom_CrossPointOfTwoLines(&TopLine, &CloseLine, &Obj2Pt);
      AlgCom_CrossPointOfTwoLines(&BotLine, &CloseLine, &Obj1Pt);
      // construct par;
      *pObj2Pt = Obj2Pt;
      *pObj1Pt = Obj1Pt;
      *pObj2Ang = Obj2Ang;
      *pObj1Ang = Obj1Ang;
      *pNewOrgAng = NewOrgAng;
      *pMaxSubLane = MaxSubLaneX;
      *pMaxSlotInnerX = MinSlotDpth;
    }
  }
  // 针对水平泊出，如果车身角度和锚点坐标系的y轴方向夹角大于5度(0.0873)，则直接采用传统水平泊出构造虚拟车位框
  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (TRUE == result) &&
      (MATH_FABS(CurCarPos.CarAng - NewOrgAng) > M_PI / 36)) {
    result = FALSE;
  }
  if (FALSE == result) {
    s_parking_out_state.flags.carry_out_slot = FALSE;
  } else {
    s_parking_out_state.flags.carry_out_slot = TRUE;
  }
  // 针对垂直和斜列泊出，如果Obj2角度和锚点角度夹角基本为90度（误差正负10度），则判定为垂直车位类型
  TempAng = Obj2Ang;
  AlgCom_AngNormalized(&TempAng);
  TempAng1 = NewOrgAng;
  AlgCom_AngNormalized(&TempAng1);
  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (TRUE == s_parking_out_state.flags.carry_out_slot) &&
      (MATH_FABS(TempAng - TempAng1) > (M_PI / 2 - M_PI / 18)) &&
      (MATH_FABS(TempAng - TempAng1) < (M_PI / 2 + M_PI / 18))) {
    bObjLabelAngledFlag = FALSE;
  }
  // 针对垂直车位泊出，如果Obj2角度和锚点角度夹角大于93度（1.63）或小于87度（1.51），则直接采用传统垂直泊出构造虚拟车位框
  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (TRUE == s_parking_out_state.flags.carry_out_slot) && (FALSE == bObjLabelAngledFlag) &&
      ((MATH_FABS(TempAng - TempAng1) > 1.63) ||
       (MATH_FABS(TempAng - TempAng1) < 1.51))) {
    s_parking_out_state.flags.carry_out_slot = FALSE;
    result = FALSE;
  }
  if ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
      (TRUE == s_parking_out_state.flags.carry_out_slot) && (TRUE == bObjLabelAngledFlag)) {
    s_parking_out_state.flags.label_angled = TRUE;
  }
#ifdef SUPPORT_PARKING_OUT_DEBUG
  {
    char log_string[512];
    snprintf(
        log_string, sizeof(log_string),
        "==APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap=="
        "bResult(%d)==bResult1(%d)==bCarryOutSlot(%d)==bObjLabelAngledFlag(%d)="
        "=bLabelAngledFlag(%d)==bObjLabelLadderFlag(%d)",
        result, bResult1, s_parking_out_state.flags.carry_out_slot, bObjLabelAngledFlag, s_parking_out_state.flags.label_angled,
        s_parking_out_state.flags.obj_label_ladder);
    TLOG_INFO << log_string;
  }
#endif
  return result;
}
#endif

BOOLEAN APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo() {
  APA_ENUM_TYPE SlotType;
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis1;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType OrgPt;
  APA_DISTANCE_CAL_FLOAT_TYPE OrgAng;
  APACoordinateDataCalFloatType TempPt1, TempPt2, TempPt3;
  APA_DISTANCE_TYPE SlotLength;
  APA_DISTANCE_TYPE SlotDepth;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis2;
  APALineParameterABCType TempLine;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotPtX;
  APA_ENUM_TYPE park_side;
  BOOLEAN result;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSubLaneX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxSlotInnerX;
  BOOLEAN bSeizeEndCarPosFlag;  // fsd侵占终点位置标志位

  bSeizeEndCarPosFlag = FALSE;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  CurCarPos = APAMap_GInputData.CarLocInfo.CarPos;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  s_parking_out_state.eight_mode = APA_PARKPROC_EIGHT_PARKING_OUT_MODE_UNKNOWNMODE;
  // zqf-GetParkOutEightMode
  park_side = APAMap_GInputData.ParkReqPar.parkside;
  s_parking_out_state.eight_mode = AlgCom_GetParkOutEightMode(park_out_mode, park_side);
  // zqf-SetParkOutObjPt
  result = APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap(
      &Obj2Pt, &Obj1Pt, &Obj2Ang, &Obj1Ang, &OrgAng, &MaxSubLaneX,
      &MaxSlotInnerX);
  if (FALSE == result) {
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

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 1);
  } else {
    TempLine = AlgCom_LineParABCByCurrentCarPosition(&CurCarPos, 0);
  }
  fDis1 = AlgCom_GetPointToLineDis(Obj1Pt, TempLine);
  fDis2 = AlgCom_GetPointToLineDis(Obj2Pt, TempLine);
  SlotLength = (APA_DISTANCE_TYPE)(fDis1 + fDis2);
  if (((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - kSlotLengthFailMarginMm)) ||
      ((park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) &&
       (SlotLength < APAMap_ComCfg.APASlotPMinSmallSlotLen - kSlotLengthFailMarginMm))) {
    APAMAP_Setfailcause(100);
    return FALSE;
  }

  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    SlotType = 0;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  } else if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
             (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
    SlotType = 1;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  } else {
    SlotType = 1;
    SlotDepth = (APA_DISTANCE_TYPE)(MaxSlotInnerX + 300);
  }
  OrgPt = Obj2Pt;
  if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    if (slot_data_at_right_side == FALSE) {
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
    if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND)) {
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
  if (slot_data_at_right_side == FALSE) {
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
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
  } else {
    TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
  }
#else
  TempPt3 = APAMap_ParkingOutSetEndCarPosInOldCorSys(
      park_out_mode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
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
#if 1
void APAMap_ParkingOutBkDataBfSDGFusInit(void) {
  APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Invalid;
  APAMap_BkDataBfSDGFus.MapMainSlotBord.PtNum = 0;
  return;
}
void APAMap_ParkingOutBkSDGOutPutDataInit(void) {
  APAMap_BkSDGOutPutData.Obj2PtNum = 0;
  APAMap_BkSDGOutPutData.Obj1PtNum = 0;
  return;
}
void APAMap_ParkingOutSiftAndSeqSDGPts(
    APACarCoordinateDataCalFloatType* pCurCarPos,
    APACoordinateDataCalFloatType* pLeftSeg,
    APACoordinateDataCalFloatType* pRightSeg, uint8_INF* pLeftSegNum,
    uint8_INF* pRightSegNum) {
#ifdef APAMAP_PARKOUT_USE_SDG_OBJS
  APACoordinateDataCalFloatType NewPto;
  APA_DISTANCE_CAL_FLOAT_TYPE NewAngle;
  APACoordinateDataCalFloatType TempSeg[2];
  APACoordinateDataCalFloatType TempPt;

  APA_DISTANCE_TYPE i, j, k;
  BOOLEAN bValid;
  APA_ENUM_TYPE ObjLocIndex;
  APA_ENUM_TYPE CurLoc;
  APA_DISTANCE_CAL_FLOAT_TYPE MinY;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxY;
  APACoordinateDataCalFloatType* pTarSeg;
  uint8_INF* pTarSegNum;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACarCoordinateDataCalFloatType TempCarPos;
  APACoordinateDataCalFloatType NewSeg[2];
  APACoordinateDataCalFloatType AddSeg[2][2];
  BOOLEAN bAddSeg[2];
  APA_DISTANCE_CAL_FLOAT_TYPE VirSegX[2];
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  APA_ENUM_TYPE AddPtNum;
  st_MapUSS* pSDGInfo;
  *pLeftSegNum = 0;
  *pRightSegNum = 0;
  pSDGInfo = &APAMap_GInputData.TotalMapInfo.mapData.USSObjInfo;
#ifdef APAMAP_PARKOUT_PCDEMO_USE_DEFAULT_SDG_OBJS
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APACoordinateDataCalFloatType Pto;
  BOOLEAN slot_data_at_right_side;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  pSDGInfo->ObjNum = 6;
  pSDGInfo->Obj[0].Pt[0].x = 0;
  pSDGInfo->Obj[0].Pt[0].y = 0;
  pSDGInfo->Obj[0].Pt[1].x = 300;
  pSDGInfo->Obj[0].Pt[1].y = -300;
  pSDGInfo->Obj[1].Pt[0].x = 3500;
  pSDGInfo->Obj[1].Pt[0].y = -300;
  pSDGInfo->Obj[1].Pt[1].x = 2500;
  pSDGInfo->Obj[1].Pt[1].y = -400;
  pSDGInfo->Obj[2].Pt[0].x = 3500;
  pSDGInfo->Obj[2].Pt[0].y = -3000;
  pSDGInfo->Obj[2].Pt[1].x = 1500;
  pSDGInfo->Obj[2].Pt[1].y = -2800;
  pSDGInfo->Obj[3].Pt[0].x = 0;
  pSDGInfo->Obj[3].Pt[0].y = -3000;
  pSDGInfo->Obj[3].Pt[1].x = 500;
  pSDGInfo->Obj[3].Pt[1].y = -3200;
  pSDGInfo->Obj[4].Pt[0].x = 4000;
  pSDGInfo->Obj[4].Pt[0].y = -2900;
  pSDGInfo->Obj[4].Pt[1].x = 4500;
  pSDGInfo->Obj[4].Pt[1].y = -2700;
  pSDGInfo->Obj[5].Pt[0].x = 4500;
  pSDGInfo->Obj[5].Pt[0].y = 200;
  pSDGInfo->Obj[5].Pt[1].x = 4000;
  pSDGInfo->Obj[5].Pt[1].y = -400;
  if (slot_data_at_right_side == FALSE) {
    for (i = 0; i < pSDGInfo->ObjNum; i++) {
      pSDGInfo->Obj[i].Pt[0].x = -pSDGInfo->Obj[i].Pt[0].x;
      pSDGInfo->Obj[i].Pt[1].x = -pSDGInfo->Obj[i].Pt[1].x;
    }
  }
  for (i = 0; i < pSDGInfo->ObjNum; i++) {
    TempPt.x = pSDGInfo->Obj[i].Pt[0].x;
    TempPt.y = pSDGInfo->Obj[i].Pt[0].y;
    TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    pSDGInfo->Obj[i].Pt[0].x = TempPt.x;
    pSDGInfo->Obj[i].Pt[0].y = TempPt.y;
    TempPt.x = pSDGInfo->Obj[i].Pt[1].x;
    TempPt.y = pSDGInfo->Obj[i].Pt[1].y;
    TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    pSDGInfo->Obj[i].Pt[1].x = TempPt.x;
    pSDGInfo->Obj[i].Pt[1].y = TempPt.y;
  }
#else
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return;
  }
#endif
  if ((pSDGInfo->ObjNum <= 0) || (pSDGInfo->ObjNum > MAP_US_OBJ_EXTR_MAX_NUM)) {
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==NoSDGPts:(%d)",
             pSDGInfo->ObjNum);
    TLOG_INFO << log_string;
    return;
  }
  if (pSDGInfo->ObjNum > 0) {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==OrgSDGPts==(%d):0[(%d,%d),(%d,%d)],1[(%d,%d),(%d,%d)],2[(%d,%d)"
             ",(%d,%d)],"
             "3[(%d,%d),(%d,%d)],4[(%d,%d),(%d,%d)],5[(%d,%d),(%d,%d)],6[(%d,%"
             "d),(%d,%d)],7[(%d,%d),(%d,%d)],"
             "8[(%d,%d),(%d,%d)],9[(%d,%d),(%d,%d)],10[(%d,%d),(%d,%d)],11[(%d,"
             "%d),(%d,%d)],12[(%d,%d),(%d,%d)]",
             pSDGInfo->ObjNum, pSDGInfo->Obj[0].Pt[0].x,
             pSDGInfo->Obj[0].Pt[0].y, pSDGInfo->Obj[0].Pt[1].x,
             pSDGInfo->Obj[0].Pt[1].y, pSDGInfo->Obj[1].Pt[0].x,
             pSDGInfo->Obj[1].Pt[0].y, pSDGInfo->Obj[1].Pt[1].x,
             pSDGInfo->Obj[1].Pt[1].y, pSDGInfo->Obj[2].Pt[0].x,
             pSDGInfo->Obj[2].Pt[0].y, pSDGInfo->Obj[2].Pt[1].x,
             pSDGInfo->Obj[2].Pt[1].y, pSDGInfo->Obj[3].Pt[0].x,
             pSDGInfo->Obj[3].Pt[0].y, pSDGInfo->Obj[3].Pt[1].x,
             pSDGInfo->Obj[3].Pt[1].y, pSDGInfo->Obj[4].Pt[0].x,
             pSDGInfo->Obj[4].Pt[0].y, pSDGInfo->Obj[4].Pt[1].x,
             pSDGInfo->Obj[4].Pt[1].y, pSDGInfo->Obj[5].Pt[0].x,
             pSDGInfo->Obj[5].Pt[0].y, pSDGInfo->Obj[5].Pt[1].x,
             pSDGInfo->Obj[5].Pt[1].y, pSDGInfo->Obj[6].Pt[0].x,
             pSDGInfo->Obj[6].Pt[0].y, pSDGInfo->Obj[6].Pt[1].x,
             pSDGInfo->Obj[6].Pt[1].y, pSDGInfo->Obj[7].Pt[0].x,
             pSDGInfo->Obj[7].Pt[0].y, pSDGInfo->Obj[7].Pt[1].x,
             pSDGInfo->Obj[7].Pt[1].y, pSDGInfo->Obj[8].Pt[0].x,
             pSDGInfo->Obj[8].Pt[0].y, pSDGInfo->Obj[8].Pt[1].x,
             pSDGInfo->Obj[8].Pt[1].y, pSDGInfo->Obj[9].Pt[0].x,
             pSDGInfo->Obj[9].Pt[0].y, pSDGInfo->Obj[9].Pt[1].x,
             pSDGInfo->Obj[9].Pt[1].y, pSDGInfo->Obj[10].Pt[0].x,
             pSDGInfo->Obj[10].Pt[0].y, pSDGInfo->Obj[10].Pt[1].x,
             pSDGInfo->Obj[10].Pt[1].y, pSDGInfo->Obj[11].Pt[0].x,
             pSDGInfo->Obj[11].Pt[0].y, pSDGInfo->Obj[11].Pt[1].x,
             pSDGInfo->Obj[11].Pt[1].y, pSDGInfo->Obj[12].Pt[0].x,
             pSDGInfo->Obj[12].Pt[0].y, pSDGInfo->Obj[12].Pt[1].x,
             pSDGInfo->Obj[12].Pt[1].y);
    TLOG_INFO << log_string;
  }

  MaxY = APAMap_ComCfg.LenBetweenRAxisAndFBumper + 1500;
  MinY = -(APAMap_ComCfg.LenBetweenRAxisAndRBumper + 1000);
  NewPto = pCurCarPos->Coordinate;
  NewAngle = pCurCarPos->CarAng;
  TempCarPos.Coordinate.x = 0;
  TempCarPos.Coordinate.y = 0;
  TempCarPos.CarAng = 0;
  VirSegX[0] = -(APAMap_ComCfg.HalfWidthOfCar + 500);
  VirSegX[1] = (APAMap_ComCfg.HalfWidthOfCar + 500);
  APAMap_GetCarRectArea(100, 100, 100, 100, TempCarPos, &pRectPt[0],
                        &pRectLine[0]);
  for (i = 0; i < pSDGInfo->ObjNum; i++) {
    bValid = FALSE;
    ObjLocIndex = -1;
    for (j = 0; j < 2; j++) {
      TempSeg[j].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSDGInfo->Obj[i].Pt[j].x;
      TempSeg[j].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSDGInfo->Obj[i].Pt[j].y;
      TempSeg[j] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          TempSeg[j], 0, NewAngle, NewPto);
      if ((TempSeg[j].y > MinY) && (TempSeg[j].y < MaxY)) {
        bValid = TRUE;
      }
      if (TempSeg[j].x < 0) {
        CurLoc = 0;  // Left;
      } else {
        CurLoc = 1;  // Right;
      }
      if ((ObjLocIndex == -1) || (ObjLocIndex == CurLoc)) {
        ObjLocIndex = CurLoc;
      } else {
        if (TempSeg[j].y > 0) {
          ObjLocIndex = 2;  // Front;
        } else {
          ObjLocIndex = 3;  // Back;
        }
      }
    }
    if (bValid == TRUE) {
      if (TempSeg[1].y > TempSeg[0].y) {
        TempPt = TempSeg[0];
        TempSeg[0] = TempSeg[1];
        TempSeg[1] = TempPt;
      }
      if (ObjLocIndex == 0) {
        pTarSeg = &pLeftSeg[0];
        pTarSegNum = pLeftSegNum;
      } else if (ObjLocIndex == 1) {
        pTarSeg = &pRightSeg[0];
        pTarSegNum = pRightSegNum;
      } else {
        pTarSeg = NULL;
        pTarSegNum = NULL;
      }
      if (pTarSeg != NULL) {
        j = 0;
        while (j < *pTarSegNum) {
          if (TempSeg[0].y > pTarSeg[j].y) {
            break;
          }
          j += 2;
        }

        k = *pTarSegNum - 1;
        AddPtNum = 2;
        if ((*pTarSegNum + AddPtNum) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
          // buff is not big enough
          return;
        }
        while (k >= j) {
          pTarSeg[k + AddPtNum] = pTarSeg[k];
          k--;
        }
        pTarSeg[j] = TempSeg[0];
        pTarSeg[j + 1] = TempSeg[1];
        *pTarSegNum += AddPtNum;
      }
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==SiftAndSeqSDGPts:Org(%d,%d)",
             *pLeftSegNum, *pRightSegNum);
    TLOG_INFO << log_string;
  }
  for (j = 0; j < 2; j++) {
    if (j == 0) {
      pTarSeg = &pLeftSeg[0];
      pTarSegNum = pLeftSegNum;
    } else {
      pTarSeg = &pRightSeg[0];
      pTarSegNum = pRightSegNum;
    }
    i = 2;
    AddPtNum = 0;
    while (i < *pTarSegNum) {
      AddPtNum = 0;
      bAddSeg[0] = FALSE;
      if ((pTarSeg[i].y < pTarSeg[i - 1].y)) {
        // check FrontSeg;
        NewSeg[0] = pTarSeg[i - 1];
        NewSeg[1] = pTarSeg[i];
        if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NewSeg[0], 2,
                                                    pRectPt, pRectLine)) {
          bAddSeg[0] = TRUE;
          if (j == 0) {
            fDis = VirSegX[0];
            if (fDis > NewSeg[0].x) {
              fDis = NewSeg[0].x;
            }
            if (fDis > NewSeg[1].x) {
              fDis = NewSeg[1].x;
            }
          } else {
            fDis = VirSegX[1];
            if (fDis < NewSeg[0].x) {
              fDis = NewSeg[0].x;
            }
            if (fDis < NewSeg[1].x) {
              fDis = NewSeg[1].x;
            }
          }
          AddSeg[0][0].x = fDis;
          AddSeg[0][0].y = NewSeg[0].y;
          AddSeg[0][1].x = fDis;
          AddSeg[0][1].y = NewSeg[1].y;
        }
      }
      bAddSeg[1] = FALSE;
      if (((i + 2) < *pTarSegNum) && (pTarSeg[i + 1].y > pTarSeg[i + 2].y)) {
        // check BackSeg;
        NewSeg[0] = pTarSeg[i + 1];
        NewSeg[1] = pTarSeg[i + 2];
        if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NewSeg[0], 2,
                                                    pRectPt, pRectLine)) {
          bAddSeg[1] = TRUE;
          if (j == 0) {
            fDis = VirSegX[0];
            if (fDis > NewSeg[0].x) {
              fDis = NewSeg[0].x;
            }
            if (fDis > NewSeg[1].x) {
              fDis = NewSeg[1].x;
            }
          } else {
            fDis = VirSegX[1];
            if (fDis < NewSeg[0].x) {
              fDis = NewSeg[0].x;
            }
            if (fDis < NewSeg[1].x) {
              fDis = NewSeg[1].x;
            }
          }
          AddSeg[1][0].x = fDis;
          AddSeg[1][0].y = NewSeg[0].y;
          AddSeg[1][1].x = fDis;
          AddSeg[1][1].y = NewSeg[1].y;
        }
      }
      if (bAddSeg[0] == TRUE) {
        AddPtNum += 2;
      }
      if (bAddSeg[1] == TRUE) {
        AddPtNum += 2;
      }
      if ((*pTarSegNum + AddPtNum) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
        // buff is not big enough
        return;
      }
      k = *pTarSegNum - 1;
      while (k >= i + 2) {
        pTarSeg[k + AddPtNum] = pTarSeg[k];
        k--;
      }
      k = i + AddPtNum;
      if (bAddSeg[1] == TRUE) {
        pTarSeg[k] = AddSeg[1][0];
        pTarSeg[k + 1] = AddSeg[1][1];
      }
      if (bAddSeg[0] == TRUE) {
        pTarSeg[i + 2] = pTarSeg[i];
        pTarSeg[i + 3] = pTarSeg[i + 1];
        pTarSeg[i] = AddSeg[0][0];
        pTarSeg[i + 1] = AddSeg[0][1];
      }
      i += AddPtNum;
      i += 4;
      *pTarSegNum += AddPtNum;
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==SiftAndSeqSDGPts:Add(%d,%d)",
             *pLeftSegNum, *pRightSegNum);
    TLOG_INFO << log_string;
  }
#else
  *pLeftSegNum = 0;
  *pRightSegNum = 0;
#endif
  return;
}
BOOLEAN APAMap_ParkingOutGetSDGInfoByParkMode(
    APACarCoordinateDataCalFloatType* pCurCarPos,
    APACoordinateDataCalFloatType* pLeftSeg,
    APACoordinateDataCalFloatType* pRightSeg, uint8_INF u8LeftSegNum,
    uint8_INF u8RightSegNum, APACoordinateDataCalFloatType* pSDGObj2Info,
    APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE* pObj2PtNum,
    APA_ENUM_TYPE* pObj1PtNum) {
  APA_ENUM_TYPE i, k;
  uint8_t_INF park_out_mode;
  uint8_t_INF ParkMode;
  APACoordinateDataCalFloatType NewPto;
  APA_DISTANCE_CAL_FLOAT_TYPE NewAngle;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType* pObj2Src;
  APACoordinateDataCalFloatType* pObj1Src;
  APA_ENUM_TYPE Obj2Num;
  APA_ENUM_TYPE Obj1Num;
  APA_ENUM_TYPE Obj2StrIndex;
  APA_ENUM_TYPE Obj1StrIndex;
  APA_ENUM_TYPE Obj1SearchStep;
  APA_ENUM_TYPE Obj2SearchStep;
  APA_ENUM_TYPE LeftSegNum;
  APA_ENUM_TYPE RightSegNum;
  APACoordinateDataCalFloatType TempPt;
  *pObj2PtNum = 0;
  *pObj1PtNum = 0;
  ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  NewPto = pCurCarPos->Coordinate;
  NewAngle = pCurCarPos->CarAng;
  if (u8LeftSegNum > APA_MAP_BK_SDG_OUTPUT_MAX_NUM) {
    LeftSegNum = 0;
  } else {
    LeftSegNum = (APA_ENUM_TYPE)u8LeftSegNum;
  }
  if (u8RightSegNum > APA_MAP_BK_SDG_OUTPUT_MAX_NUM) {
    RightSegNum = 0;
  } else {
    RightSegNum = (APA_ENUM_TYPE)u8RightSegNum;
  }
  AlgCom_SmoothSegMent(TRUE, 1000, 10, pLeftSeg, &LeftSegNum);
  AlgCom_SmoothSegMent(FALSE, 1000, 10, pRightSeg, &RightSegNum);
  if (LeftSegNum > 0) {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==GetSDGInfoByParkMode==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%."
             "2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f),"
             "5(%.2f,%.2f),6(%.2f,%.2f),7(%.2f,%.2f),8(%.2f,%.2f),9(%.2f,%.2f),"
             "10(%.2f,%.2f),11(%.2f,%.2f),"
             "12(%.2f,%.2f),13(%.2f,%.2f),14(%.2f,%.2f),15(%.2f,%.2f),16(%.2f,%"
             ".2f),17(%.2f,%.2f),18(%.2f,%.2f),19(%.2f,%.2f)",
             LeftSegNum, pLeftSeg[0].x, pLeftSeg[0].y, pLeftSeg[1].x,
             pLeftSeg[1].y, pLeftSeg[2].x, pLeftSeg[2].y, pLeftSeg[3].x,
             pLeftSeg[3].y, pLeftSeg[4].x, pLeftSeg[4].y, pLeftSeg[5].x,
             pLeftSeg[5].y, pLeftSeg[6].x, pLeftSeg[6].y, pLeftSeg[7].x,
             pLeftSeg[7].y, pLeftSeg[8].x, pLeftSeg[8].y, pLeftSeg[9].x,
             pLeftSeg[9].y, pLeftSeg[10].x, pLeftSeg[10].y, pLeftSeg[11].x,
             pLeftSeg[11].y, pLeftSeg[12].x, pLeftSeg[12].y, pLeftSeg[13].x,
             pLeftSeg[13].y, pLeftSeg[14].x, pLeftSeg[14].y, pLeftSeg[15].x,
             pLeftSeg[15].y, pLeftSeg[16].x, pLeftSeg[16].y, pLeftSeg[17].x,
             pLeftSeg[17].y, pLeftSeg[18].x, pLeftSeg[18].y, pLeftSeg[19].x,
             pLeftSeg[19].y);
    TLOG_INFO << log_string;
  }
  if (RightSegNum > 0) {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==GetSDGInfoByParkMode==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%."
             "2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f),"
             "5(%.2f,%.2f),6(%.2f,%.2f),7(%.2f,%.2f),8(%.2f,%.2f),9(%.2f,%.2f),"
             "10(%.2f,%.2f),11(%.2f,%.2f),"
             "12(%.2f,%.2f),13(%.2f,%.2f),14(%.2f,%.2f),15(%.2f,%.2f),16(%.2f,%"
             ".2f),17(%.2f,%.2f),18(%.2f,%.2f),19(%.2f,%.2f)",
             RightSegNum, pRightSeg[0].x, pRightSeg[0].y, pRightSeg[1].x,
             pRightSeg[1].y, pRightSeg[2].x, pRightSeg[2].y, pRightSeg[3].x,
             pRightSeg[3].y, pRightSeg[4].x, pRightSeg[4].y, pRightSeg[5].x,
             pRightSeg[5].y, pRightSeg[6].x, pRightSeg[6].y, pRightSeg[7].x,
             pRightSeg[7].y, pRightSeg[8].x, pRightSeg[8].y, pRightSeg[9].x,
             pRightSeg[9].y, pRightSeg[10].x, pRightSeg[10].y, pRightSeg[11].x,
             pRightSeg[11].y, pRightSeg[12].x, pRightSeg[12].y, pRightSeg[13].x,
             pRightSeg[13].y, pRightSeg[14].x, pRightSeg[14].y, pRightSeg[15].x,
             pRightSeg[15].y, pRightSeg[16].x, pRightSeg[16].y, pRightSeg[17].x,
             pRightSeg[17].y, pRightSeg[18].x, pRightSeg[18].y, pRightSeg[19].x,
             pRightSeg[19].y);
    TLOG_INFO << log_string;
  }
  if ((LeftSegNum == 0) && (RightSegNum == 0)) {
    return FALSE;
  }
  if ((ParkMode != APA_PARKPROC_PARKING_MODE_PARKING_OUT) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
    return FALSE;
  }

  if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
      (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
    if (slot_data_at_right_side == TRUE) {
      pObj2Src = pRightSeg;
      Obj2Num = RightSegNum;
      pObj1Src = pLeftSeg;
      Obj1Num = LeftSegNum;
    } else {
      pObj2Src = pLeftSeg;
      Obj2Num = LeftSegNum;
      pObj1Src = pRightSeg;
      Obj1Num = RightSegNum;
    }
    Obj2StrIndex = Obj2Num - 1;
    Obj2SearchStep = -1;
    Obj1StrIndex = 0;
    Obj1SearchStep = 1;
  } else {
    if (slot_data_at_right_side == TRUE) {
      pObj2Src = pLeftSeg;
      Obj2Num = LeftSegNum;
      pObj1Src = pRightSeg;
      Obj1Num = RightSegNum;
    } else {
      pObj2Src = pRightSeg;
      Obj2Num = RightSegNum;
      pObj1Src = pLeftSeg;
      Obj1Num = LeftSegNum;
    }
    Obj2StrIndex = 0;
    Obj2SearchStep = 1;
    Obj1StrIndex = Obj1Num - 1;
    Obj1SearchStep = -1;
  }
  i = Obj2StrIndex;
  k = 0;
  while (k < Obj2Num) {
    TempPt = pObj2Src[i];
    pSDGObj2Info[k] =
        AlgCom_PointPosWithAngAndCenterPt(TempPt, NewAngle, NewPto);
    k++;
    i += Obj2SearchStep;
  }
  *pObj2PtNum = Obj2Num;

  i = Obj1StrIndex;
  k = 0;
  while (k < Obj1Num) {
    TempPt = pObj1Src[i];
    pSDGObj1Info[k] =
        AlgCom_PointPosWithAngAndCenterPt(TempPt, NewAngle, NewPto);
    k++;
    i += Obj1SearchStep;
  }
  *pObj1PtNum = Obj1Num;
  return TRUE;
}
void APAMap_ParkingOutGetBkSDGOutPutData(
    APACoordinateDataCalFloatType* pSDGObj2Info,
    APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE* pObj2PtNum,
    APA_ENUM_TYPE* pObj1PtNum) {
  APA_ENUM_TYPE i;
  *pObj2PtNum = 0;
  *pObj1PtNum = 0;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
    if ((APAMap_BkSDGOutPutData.Obj2PtNum > 0) &&
        (APAMap_BkSDGOutPutData.Obj2PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
      for (i = 0; i < APAMap_BkSDGOutPutData.Obj2PtNum; i++) {
        pSDGObj2Info[i] = APAMap_BkSDGOutPutData.SDGObj2Info[i];
      }
      *pObj2PtNum = APAMap_BkSDGOutPutData.Obj2PtNum;
    }
    if ((APAMap_BkSDGOutPutData.Obj1PtNum > 0) &&
        (APAMap_BkSDGOutPutData.Obj1PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
      for (i = 0; i < APAMap_BkSDGOutPutData.Obj1PtNum; i++) {
        pSDGObj1Info[i] = APAMap_BkSDGOutPutData.SDGObj1Info[i];
      }
      *pObj1PtNum = APAMap_BkSDGOutPutData.Obj1PtNum;
    }
  }
  return;
}
void APAMap_ParkingOutSaveBkSDGOutPutData(
    APACoordinateDataCalFloatType* pSDGObj2Info,
    APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE Obj2PtNum,
    APA_ENUM_TYPE Obj1PtNum) {
  APA_ENUM_TYPE i;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
    if ((Obj2PtNum > 0) && (Obj2PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
      for (i = 0; i < Obj2PtNum; i++) {
        APAMap_BkSDGOutPutData.SDGObj2Info[i] = pSDGObj2Info[i];
      }
      APAMap_BkSDGOutPutData.Obj2PtNum = Obj2PtNum;
    }
    if ((Obj1PtNum > 0) && (Obj1PtNum <= APA_MAP_BK_SDG_OUTPUT_MAX_NUM)) {
      for (i = 0; i < Obj1PtNum; i++) {
        APAMap_BkSDGOutPutData.SDGObj1Info[i] = pSDGObj1Info[i];
      }
      APAMap_BkSDGOutPutData.Obj1PtNum = Obj1PtNum;
    }
  }
  return;
}
void APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus(void) {
  BOOLEAN slot_data_at_right_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pSlotBordBk;
  APA_ENUM_TYPE i;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
    if (slot_data_at_right_side == FALSE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    }
    pSlotBordBk = &APAMap_BkDataBfSDGFus.MapMainSlotBord;
    if ((pSlotBordBk->PtNum > 0) &&
        (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pSlotBordBk->PtNum; i++) {
        pMapMainSlotBord->Property[i] = pSlotBordBk->Property[i];
        pMapMainSlotBord->Points[i] = pSlotBordBk->Points[i];
      }
      pMapMainSlotBord->PtNum = pSlotBordBk->PtNum;
    }
    APAMap_GInfo.SlotPar.Obj1PtIndex = APAMap_BkDataBfSDGFus.Obj1PtIndex;
    APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_BkDataBfSDGFus.SlotStrIndex;
    APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_BkDataBfSDGFus.SlotEndIndex;
    APAMap_GInfo.SlotPar.Obj2PtIndex = APAMap_BkDataBfSDGFus.Obj2PtIndex;
  }
  return;
}

void APAMap_ParkingOutDeleteMainSlotBord(void) {
  BOOLEAN slot_data_at_right_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  APA_ENUM_TYPE i, j;
  APACoordinateDataCalFloatType TempPt1;
  APACoordinateDataCalFloatType TempPt2;
  APACoordinateDataCalFloatType TempPt3;
  APACoordinateDataCalFloatType TempPt4;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APA_DISTANCE_TYPE PrePtNum;
  APA_DISTANCE_TYPE PtNumTemp;
  APACoordinateDataCalFloatType Pto;
  PrePtNum = 0;
  PtNumTemp = 0;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
    if (slot_data_at_right_side == FALSE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    }
    PrePtNum = pMapMainSlotBord->PtNum;
    if (pMapMainSlotBord->PtNum > 2) {
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pMapMainSlotBord->Points[PrePtNum - 1], 0, Angle, Pto);
      for (i = 2; i < (PrePtNum - APAMap_GInfo.SlotPar.SlotEndIndex); i++) {
        TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pMapMainSlotBord->Points[PrePtNum - i], 0, Angle, Pto);
        if (MATH_FABS(TempPt1.y - TempPt2.y) < 0.1) {
          pMapMainSlotBord->PtNum -= 1;
        }
      }
#if 1
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pMapMainSlotBord->Points[PrePtNum - 2], 0, Angle, Pto);
#endif

      TempPt3 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pMapMainSlotBord->Points[0], 0, Angle, Pto);
      for (i = 1; i < APAMap_GInfo.SlotPar.SlotStrIndex; i++) {
        TempPt4 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
            pMapMainSlotBord->Points[i], 0, Angle, Pto);
        if (MATH_FABS(TempPt3.y - TempPt4.y) < 0.1) {
          PtNumTemp++;
        }
      }
      if (PtNumTemp > 0) {
        for (j = 0; j < (pMapMainSlotBord->PtNum - PtNumTemp); j++) {
          pMapMainSlotBord->Property[j] =
              pMapMainSlotBord->Property[j + PtNumTemp];
          pMapMainSlotBord->Points[j] = pMapMainSlotBord->Points[j + PtNumTemp];
        }
        pMapMainSlotBord->PtNum -= PtNumTemp;
        APAMap_GInfo.SlotPar.Obj1PtIndex -= PtNumTemp;
        APAMap_GInfo.SlotPar.SlotStrIndex -= PtNumTemp;
        APAMap_GInfo.SlotPar.SlotEndIndex -= PtNumTemp;
        APAMap_GInfo.SlotPar.Obj2PtIndex -= PtNumTemp;
      }
#if 1
      TempPt4 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pMapMainSlotBord->Points[1], 0, Angle, Pto);
#endif
      {
        char log_string[512];
        snprintf(
            log_string, sizeof(log_string),
            "==DeleteMainSlotBord==PrePtNum(%d)==PtNum(%d)==TempPt1(%.2f,%.2f)="
            "=TempPt2(%.2f,%.2f)"
            "==TempPt3(%.2f,%.2f)==TempPt4(%.2f,%.2f)==SlotIndex(%d,%d,%d,%d)",
            PrePtNum, pMapMainSlotBord->PtNum, TempPt1.x, TempPt1.y, TempPt2.x,
            TempPt2.y, TempPt3.x, TempPt3.y, TempPt4.x, TempPt4.y,
            APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
            APAMap_GInfo.SlotPar.SlotEndIndex,
            APAMap_GInfo.SlotPar.Obj2PtIndex);
        TLOG_INFO << log_string;
      }
    }
  }
  return;
}

void APAMap_ParkingOutSaveBkDataBfSDGFus(void) {
  BOOLEAN slot_data_at_right_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pSlotBordBk;
  APA_ENUM_TYPE i;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Updata) {
    if (slot_data_at_right_side == FALSE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    }
    pSlotBordBk = &APAMap_BkDataBfSDGFus.MapMainSlotBord;
    if ((pMapMainSlotBord->PtNum > 0) &&
        (pMapMainSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pMapMainSlotBord->PtNum; i++) {
        pSlotBordBk->Property[i] = pMapMainSlotBord->Property[i];
        pSlotBordBk->Points[i] = pMapMainSlotBord->Points[i];
      }
      pSlotBordBk->PtNum = pMapMainSlotBord->PtNum;
    }
    APAMap_BkDataBfSDGFus.Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
    APAMap_BkDataBfSDGFus.SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
    APAMap_BkDataBfSDGFus.SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
    APAMap_BkDataBfSDGFus.Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
  }
  return;
}
void APAMap_ParkingOutUpDataMapBoundaryBySDGInfo(void) {
  APACoordinateDataCalFloatType pSDGObj2Info[APA_MAP_BK_SDG_OUTPUT_MAX_NUM];
  APACoordinateDataCalFloatType pSDGObj1Info[APA_MAP_BK_SDG_OUTPUT_MAX_NUM];
  APA_ENUM_TYPE Obj2PtNum;
  APA_ENUM_TYPE Obj1PtNum;
  if (TRUE == APAMap_ParkingOutCheckIfFusBoundarySDGInfo()) {
    if (TRUE == APAMap_ParkingOutGetSDGInfoPt(&pSDGObj2Info[0],
                                              &pSDGObj1Info[0], &Obj2PtNum,
                                              &Obj1PtNum)) {
      APAMap_ParkingOutSaveBkSDGOutPutData(&pSDGObj2Info[0], &pSDGObj1Info[0],
                                           Obj2PtNum, Obj1PtNum);
    } else {
      APAMap_ParkingOutGetBkSDGOutPutData(&pSDGObj2Info[0], &pSDGObj1Info[0],
                                          &Obj2PtNum, &Obj1PtNum);
    }
    if ((Obj2PtNum > 0) || (Obj1PtNum > 0)) {
      APAMap_ParkingOutSaveBkDataBfSDGFus();
      APAMap_ParkingOutFusBoundaryBySDGInfo(&pSDGObj2Info[0], &pSDGObj1Info[0],
                                            Obj2PtNum, Obj1PtNum);
      APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Updata;
      char log_string[512];
      snprintf(log_string, sizeof(log_string), "==SDGFusionBorderUpData!==");
      TLOG_INFO << log_string;
    }
  }
  char log_string[512];
  snprintf(log_string, sizeof(log_string),
           "==FusSDGStatus:(%d)==BkDataBfSDGFus:Index(%d,%d,%d,%d),BkBordPtNum("
           "%d)==BkSDGOutPutDataNum:(%d,%d)",
           APAMap_BkDataBfSDGFus.FusSDGStatus,
           APAMap_BkDataBfSDGFus.SlotStrIndex,
           APAMap_BkDataBfSDGFus.SlotEndIndex,
           APAMap_BkDataBfSDGFus.Obj1PtIndex, APAMap_BkDataBfSDGFus.Obj2PtIndex,
           APAMap_BkDataBfSDGFus.MapMainSlotBord.PtNum,
           APAMap_BkSDGOutPutData.Obj2PtNum, APAMap_BkSDGOutPutData.Obj1PtNum);
  TLOG_INFO << log_string;
  return;
}
BOOLEAN APAMap_ParkingOutCheckIfFusBoundarySDGInfo(void) {
  uint8_t_INF park_out_mode;
  uint8_t_INF ParkMode;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APACoordinateDataCalFloatType Pto;
  BOOLEAN slot_data_at_right_side;
  BOOLEAN result;
  BOOLEAN bResult1;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACoordinateDataCalFloatType TempPt2;
  APACoordinateDataCalFloatType TempPt1;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1Ang;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis1;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis2;
  APACarCoordinateDataCalFloatType TempCarPos;
  APALineParameterABCType TopLine;
  APALineParameterABCType BottomLine;
  BOOLEAN bCarPosValidForFusSDG;
  APA_ENUM_TYPE CarCorIndex[4];
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  ParkMode = APAMap_GInputData.ParkReqPar.parkmode;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  CurCarPos = APAMap_GInfo.CarPos;
  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, &pRectPt[0], &pRectLine[0]);
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.Obj2Pt;
  TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      TempCarPos.Coordinate, 0, Angle, Pto);
  Obj2Ang = APAMap_GInfo.SlotPar.Obj2Ang;
  Obj2Ang -= Angle;
  TempCarPos.CarAng = Obj2Ang;
  TopLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.Obj1Pt;
  TempCarPos.Coordinate = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
      TempCarPos.Coordinate, 0, Angle, Pto);
  Obj1Ang = APAMap_GInfo.SlotPar.Obj1Ang;
  Obj1Ang -= Angle;
  TempCarPos.CarAng = Obj1Ang;
  BottomLine = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  bCarPosValidForFusSDG = FALSE;
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Keep) {
    return FALSE;
  }
  if ((ParkMode == APA_PARKPROC_PARKING_MODE_PARKING_OUT) &&
      (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL)) {
    if ((park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) ||
        (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_GO_STRAIGHT)) {
      CarCorIndex[0] = 1;
      CarCorIndex[1] = 2;
      CarCorIndex[2] = 0;
      CarCorIndex[3] = 3;
    } else {
      CarCorIndex[0] = 0;
      CarCorIndex[1] = 3;
      CarCorIndex[2] = 1;
      CarCorIndex[3] = 2;
    }
    TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        pRectPt[CarCorIndex[0]], 0, Angle, Pto);

    TempDis1 = TopLine.A * TempPt1.x + TopLine.C - TempPt1.y;
    TempDis2 = BottomLine.A * TempPt1.x + BottomLine.C - TempPt1.y;
    if ((TempDis1 >= 0) && (TempDis2 <= 0)) {
      result = TRUE;
    } else {
      result = FALSE;
    }
    TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        pRectPt[CarCorIndex[1]], 0, Angle, Pto);

    TempDis1 = TopLine.A * TempPt2.x + TopLine.C - TempPt2.y;
    TempDis2 = BottomLine.A * TempPt2.x + BottomLine.C - TempPt2.y;
    if ((TempDis1 >= 0) && (TempDis2 <= 0)) {
      bResult1 = TRUE;
    } else {
      bResult1 = FALSE;
    }
    if (slot_data_at_right_side == FALSE) {
      TempPt2.x = -TempPt2.x;
      TempPt1.x = -TempPt1.x;
    }
    if (TempPt2.x < TempPt1.x) {
      TempDis = TempPt2.x;
    } else {
      TempDis = TempPt1.x;
    }
    if ((result == TRUE) && (bResult1 == TRUE) && (TempDis > 0)) {
      // car two innercor still in slot;
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pRectPt[CarCorIndex[2]], 0, Angle, Pto);
      TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pRectPt[CarCorIndex[3]], 0, Angle, Pto);
      if (slot_data_at_right_side == FALSE) {
        TempPt2.x = -TempPt2.x;
        TempPt1.x = -TempPt1.x;
      }
      if (TempPt2.x < TempPt1.x) {
        TempDis = TempPt2.x;
      } else {
        TempDis = TempPt1.x;
      }
      if (TempDis > -3500)  //-1500
      {
        // car two out corner still not far away from slot;
        bCarPosValidForFusSDG = TRUE;
      }
    }
    if (bCarPosValidForFusSDG == FALSE) {
      APAMap_BkDataBfSDGFus.FusSDGStatus = APAMap_FusSDGStatus_Keep;
    }
  } else {
    APAMap_ParkingOutBkDataBfSDGFusInit();
  }
  return bCarPosValidForFusSDG;
}
BOOLEAN
APAMap_ParkingOutGetSDGInfoPt(APACoordinateDataCalFloatType* pSDGObj2Info,
                              APACoordinateDataCalFloatType* pSDGObj1Info,
                              APA_ENUM_TYPE* pObj2PtNum,
                              APA_ENUM_TYPE* pObj1PtNum) {
#ifdef APAMAP_PARKOUT_USE_SDG_OBJS

#if 0
   APACoordinateDataCalFloatType Obj1SDGPt[5];
   APACoordinateDataCalFloatType Obj2SDGPt[5];
   APA_DISTANCE_CAL_FLOAT_TYPE Angle;
   APACoordinateDataCalFloatType Pto;
   BOOLEAN slot_data_at_right_side;
   APA_ENUM_TYPE i;
   Pto = APAMap_GInfo.NewCordSysOPt;
   Angle = APAMap_GInfo.NewCordSysAng;
   slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
   for(i = 0; i < 5;i++)
   {
     Obj2SDGPt[i].x = 0;
     Obj2SDGPt[i].y = 0;
     Obj1SDGPt[i].x = 0;
     Obj1SDGPt[i].y = 0;
   }
   *pObj2PtNum = 2;
   *pObj1PtNum = 2;
   Obj2SDGPt[0].x = 3500;
   Obj2SDGPt[0].y = -1800;//200;//-100;
   Obj2SDGPt[1].x = 0;
   Obj2SDGPt[1].y = 400;//-100;//200;
   

   Obj1SDGPt[0].x = 0;
   Obj1SDGPt[0].y = -4300;//-3900;//-4200;
   Obj1SDGPt[1].x = 3500;
   Obj1SDGPt[1].y = -5300;//-4200;//-3900;
   if(slot_data_at_right_side == FALSE)
   {
      for(i = 0; i < *pObj2PtNum;i++)
      {
         Obj2SDGPt[i].x = -Obj2SDGPt[i].x;
      }
      for(i = 0; i < *pObj1PtNum;i++)
      {
         Obj1SDGPt[i].x = -Obj1SDGPt[i].x;
      }
   }
   for(i = 0; i < *pObj2PtNum;i++)
   {
      pSDGObj2Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj2SDGPt[i],Angle,Pto);
   }
   for(i = 0; i < *pObj1PtNum;i++)
   {
      pSDGObj1Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj1SDGPt[i],Angle,Pto);
   }
   {
    char log_string[1024];
      snprintf(log_string, sizeof(log_string),
            "==SDGDebugData==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)"
            "==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)",
            *pObj2PtNum,
            pSDGObj2Info[0].x,pSDGObj2Info[0].y,
            pSDGObj2Info[1].x,pSDGObj2Info[1].y,
            pSDGObj2Info[2].x,pSDGObj2Info[2].y,
            pSDGObj2Info[3].x,pSDGObj2Info[3].y,
            pSDGObj2Info[4].x,pSDGObj2Info[4].y,
            *pObj1PtNum,
            pSDGObj1Info[0].x,pSDGObj1Info[0].y,
            pSDGObj1Info[1].x,pSDGObj1Info[1].y,
            pSDGObj1Info[2].x,pSDGObj1Info[2].y,
            pSDGObj1Info[3].x,pSDGObj1Info[3].y,
            pSDGObj1Info[4].x,pSDGObj1Info[4].y);
      TLOG_INFO << log_string;
    }
   return TRUE;
#endif

  APACoordinateDataCalFloatType LeftSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  APACoordinateDataCalFloatType RightSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  uint8_INF LeftSegNum;
  uint8_INF RightSegNum;
  APACarCoordinateDataCalFloatType CurCarPos;
  CurCarPos = APAMap_GInfo.CarPos;
  APAMap_ParkingOutSiftAndSeqSDGPts(&CurCarPos, &LeftSeg[0], &RightSeg[0],
                                    &LeftSegNum, &RightSegNum);
  if (TRUE == APAMap_ParkingOutGetSDGInfoByParkMode(
                  &CurCarPos, &LeftSeg[0], &RightSeg[0], LeftSegNum,
                  RightSegNum, pSDGObj2Info, pSDGObj1Info, pObj2PtNum,
                  pObj1PtNum)) {
    return TRUE;
  } else {
    return FALSE;
  }
#else
  return FALSE;
#endif
}
// APAMap_BkDataBfSDGFus
BOOLEAN APAMap_ParkingOutFusBoundaryBySDGInfo(
    APACoordinateDataCalFloatType* pSDGObj2Info,
    APACoordinateDataCalFloatType* pSDGObj1Info, APA_ENUM_TYPE Obj2PtNum,
    APA_ENUM_TYPE Obj1PtNum) {
  APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
  APACoordinateDataCalFloatType Obj2InnerPt, Obj1InnerPt;
  APACoordinateDataCalFloatType SDGObj2EndPt, SDGObj1StrPt;
  APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
  APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
  APA_ENUM_TYPE Index;
  APA_DISTANCE_TYPE i;
  APA_ENUM_TYPE k;
  tMap_BoundPt_t* pMapMainSlotBord;
  APA_DISTANCE_CAL_FLOAT_TYPE OuterBorderFusAng;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj2InnerBorderFusAng;
  APA_DISTANCE_CAL_FLOAT_TYPE Obj1InnerBorderFusAng;
  BOOLEAN slot_data_at_right_side;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APACoordinateDataCalFloatType Pto;
  APACarCoordinateDataCalFloatType TempCarPos;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType pData1[127];
  APACoordinateDataCalFloatType pData2[127];
  APACoordinateDataCalFloatType pData3[127];
  APACoordinateDataCalFloatType pData4[127];
  APACoordinateDataCalFloatType pData5[127];
  APA_ENUM_TYPE SDGDataPtNum;
  uint8_t_INF pPtStyle[127];
  uint8_t_INF NewProperty1[127];
  uint8_t_INF NewProperty2[127];
  uint8_t_INF NewProperty3[127];
  uint8_t_INF NewProperty4[127];
  uint8_t_INF NewProperty5[127];
  uint8_t_INF SDGSegProperty[127];
  uint16_t_INF DataNum;
  APA_ENUM_TYPE Data1Num;
  APA_ENUM_TYPE Data2Num;
  APA_ENUM_TYPE Data3Num;
  APA_ENUM_TYPE Data4Num;
  APA_ENUM_TYPE Data5Num;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetY;
  APALineParameterABCType TempLine1;
  APALineParameterABCType TempLine2;
  APACoordinateDataCalFloatType StrPt, EndPt;
  APACoordinateDataCalFloatType TempPt1;
  APALineParameterABCType BordLine[2];
  BOOLEAN bCallNewObjPt;
  APACoordinateDataCalFloatType NewObjPt;
  APACoordinateDataCalFloatType SDGObj2OuterInfo[100];
  APA_ENUM_TYPE SDGObj2OutPtNum;
  APACoordinateDataCalFloatType SDGObj2InnerInfo[100];
  APA_ENUM_TYPE SDGObj2InnerPtNum;
  APACoordinateDataCalFloatType SDGObj1OuterInfo[100];
  APA_ENUM_TYPE SDGObj1OutPtNum;
  APACoordinateDataCalFloatType SDGObj1InnerInfo[100];
  APA_ENUM_TYPE SDGObj1InnerPtNum;
  BOOLEAN bCrossBordLine;
  APACoordinateDataCalFloatType* pSDGData;
  APA_ENUM_TYPE CrossPtIndex;
  APACoordinateDataCalFloatType CrossPt;
  APACoordinateDataCalFloatType Obj2ValidPt[100];
  APACoordinateDataCalFloatType Obj1ValidPt[100];
  APA_ENUM_TYPE ValidObj2Num;
  APA_ENUM_TYPE ValidObj1Num;
  APA_DISTANCE_CAL_FLOAT_TYPE PrePtY;
  APACarCoordinateDataCalFloatType CurCarPos;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APA_ENUM_TYPE ValidObjNum;
  BOOLEAN bValidFlag;
  CurCarPos = APAMap_GInfo.CarPos;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if ((Obj2PtNum <= 0) && (Obj1PtNum <= 0)) {
    return FALSE;
  }
  MaxOffsetX = 1500;
  MaxOffsetY = 2000;
  if (slot_data_at_right_side == FALSE) {
    pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
  } else {
    pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
  }
  SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
  SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
  Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
  Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
  OrgObj2Pt = pMapMainSlotBord->Points[Obj2PtIndex];
  OrgObj1Pt = pMapMainSlotBord->Points[Obj1PtIndex];
  Obj2InnerPt = pMapMainSlotBord->Points[SlotEndIndex];
  Obj1InnerPt = pMapMainSlotBord->Points[SlotStrIndex];
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  if (slot_data_at_right_side) {
    OuterBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
    AlgCom_AngNormalized(&OuterBorderFusAng);
    Obj2InnerBorderFusAng = Angle;
    Obj1InnerBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI);
    AlgCom_AngNormalized(&Obj1InnerBorderFusAng);
  } else {
    OuterBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
    AlgCom_AngNormalized(&OuterBorderFusAng);
    Obj2InnerBorderFusAng = Angle;
    Obj1InnerBorderFusAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI);
    AlgCom_AngNormalized(&Obj1InnerBorderFusAng);
  }
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }

  TempPt.x = MaxOffsetX;
  TempPt.y = 0;
  TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
  TempCarPos.Coordinate = TempPt;
  TempCarPos.CarAng = Angle;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  i = Obj2PtNum - 1;
  bCrossBordLine = FALSE;
  CrossPtIndex = 0;

  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
  while (i > 0) {
    StrPt = pSDGObj2Info[i];
    EndPt = pSDGObj2Info[i - 1];
    if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
      if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt)) {
        if (TRUE ==
            AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
          bCrossBordLine = TRUE;
          CrossPtIndex = i;
          CrossPt = TempPt;
          break;
        }
      }
    }
    i--;
  }
  if (bCrossBordLine == FALSE) {
    CrossPt = pSDGObj2Info[Obj2PtNum - 1];
    TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CrossPt, 0,
                                                                  Angle, Pto);
    if (((TempPt1.x >= MaxOffsetX) && (slot_data_at_right_side == TRUE)) ||
        ((TempPt1.x <= MaxOffsetX) && (slot_data_at_right_side == FALSE))) {
      CrossPtIndex = Obj2PtNum;
    } else {
      CrossPtIndex = 0;
    }
  }
  ValidObjNum = 0;
  ValidObj2Num = 0;
  for (Index = 0; Index < CrossPtIndex; Index++) {
    Obj2ValidPt[ValidObj2Num] = pSDGObj2Info[Index];
    TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
        Obj2ValidPt[ValidObj2Num], 0, Angle, Pto);
    if (ValidObj2Num < (CrossPtIndex - 1)) {
      ValidObjNum = ValidObj2Num + 1;
      Obj2ValidPt[ValidObjNum] = pSDGObj2Info[CrossPtIndex - 1];
    } else {
      ValidObjNum = ValidObj2Num;
    }
    bValidFlag = FALSE;
    if (FALSE == APAMap_CheckIfObjWithinRectArea(
                     0x01, &Obj2ValidPt[0], ValidObjNum, pRectPt, pRectLine)) {
      if (TempPt1.y <= MaxOffsetY) {
        ValidObj2Num++;
        bValidFlag = TRUE;
      } else if (((slot_data_at_right_side == TRUE) && (TempPt1.x >= 0)) ||
                 ((slot_data_at_right_side == FALSE) && (TempPt1.x <= 0))) {
        ValidObj2Num++;
        bValidFlag = TRUE;
      }
    }
    if (FALSE == bValidFlag) {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==SDGFusionObj2OuterBorder==1==Obj2ValidPt[%d](%.2f,%.2f)"
               "==TempPt1(%.2f,%.2f)",
               ValidObj2Num, Obj2ValidPt[ValidObj2Num].x,
               Obj2ValidPt[ValidObj2Num].y, TempPt1.x, TempPt1.y);
      TLOG_INFO << log_string;
    }
  }

  if (bCrossBordLine == TRUE) {
    Obj2ValidPt[ValidObj2Num] = CrossPt;
    ValidObj2Num++;
  }
  i = 0;
  bCrossBordLine = FALSE;
  CrossPtIndex = 0;
  while (i < Obj1PtNum - 1) {
    StrPt = pSDGObj1Info[i];
    EndPt = pSDGObj1Info[i + 1];
    if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
      if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &TempPt)) {
        if (TRUE ==
            AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
          bCrossBordLine = TRUE;
          CrossPtIndex = i + 1;
          CrossPt = TempPt;
          break;
        }
      }
    }
    i++;
  }
  if (bCrossBordLine == FALSE) {
    CrossPt = pSDGObj1Info[Obj1PtNum - 1];
    TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(CrossPt, 0,
                                                                  Angle, Pto);
    if (((TempPt1.x >= MaxOffsetX) && (slot_data_at_right_side == TRUE)) ||
        ((TempPt1.x <= MaxOffsetX) && (slot_data_at_right_side == FALSE))) {
      CrossPtIndex = 0;
    } else {
      CrossPtIndex = Obj1PtNum;
    }
  }
  ValidObjNum = 0;
  ValidObj1Num = 0;
  if (bCrossBordLine == TRUE) {
    Obj1ValidPt[ValidObj1Num] = CrossPt;
    ValidObj1Num++;
  }
  for (Index = CrossPtIndex; Index < Obj1PtNum; Index++) {
    Obj1ValidPt[ValidObj1Num] = pSDGObj1Info[Index];
    if (ValidObj1Num < (Obj1PtNum - 1)) {
      ValidObjNum = ValidObj1Num + 1;
      Obj1ValidPt[ValidObjNum] = pSDGObj1Info[ValidObjNum];
    } else {
      ValidObjNum = ValidObj1Num;
    }
    if (FALSE == APAMap_CheckIfObjWithinRectArea(
                     0x01, &Obj1ValidPt[0], ValidObjNum, pRectPt, pRectLine)) {
      ValidObj1Num++;
    } else {
      char log_string[512];
      snprintf(log_string, sizeof(log_string),
               "==SDGFusionObj2OuterBorder==2==Obj1ValidPt[%d](%.2f,%.2f)",
               ValidObj1Num, Obj1ValidPt[ValidObj1Num].x,
               Obj1ValidPt[ValidObj1Num].y);
      TLOG_INFO << log_string;
    }
  }
  // obj1 borderline;
  Data1Num = 0;
  for (Index = 0; Index <= Obj1PtIndex; Index++) {
    pData1[Data1Num] = pMapMainSlotBord->Points[Index];
    NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
    Data1Num++;
    if (Data1Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==SDG Buffer Not enough==1==Data1Num:(%d)", Data1Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // obj2 borderline;
  Data2Num = 0;
  for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
    pData2[Data2Num] = pMapMainSlotBord->Points[Index];
    NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
    Data2Num++;
    if (Data2Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==SDG Buffer Not enough==2==Data2Num:(%d)", Data2Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // obj1Inner borderline;
  Data3Num = 0;
  for (Index = Obj1PtIndex; Index <= SlotStrIndex; Index++) {
    pData3[Data3Num] = pMapMainSlotBord->Points[Index];
    NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
    Data3Num++;
    if (Data3Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==SDG Buffer Not enough==3==Data3Num:(%d)", Data3Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // obj2Inner borderline;
  Data4Num = 0;
  for (Index = SlotEndIndex; Index <= Obj2PtIndex; Index++) {
    pData4[Data4Num] = pMapMainSlotBord->Points[Index];
    NewProperty4[Data4Num] = pMapMainSlotBord->Property[Index];
    Data4Num++;
    if (Data4Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==SDG Buffer Not enough==4==Data4Num:(%d)", Data4Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // SlotInner Borderline;
  Data5Num = 0;
  for (Index = SlotStrIndex + 1; Index < SlotEndIndex; Index++) {
    pData5[Data5Num] = pMapMainSlotBord->Points[Index];
    NewProperty5[Data5Num] = pMapMainSlotBord->Property[Index];
    Data5Num++;
    if (Data5Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==SDG Buffer Not enough==5==Data5Num:(%d)", Data5Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  for (k = 0; k < 100; k++) {
    SDGSegProperty[k] = 0;
  }
  AlgCom_LineParABCbyTwoPoints(OrgObj2Pt, Obj2InnerPt, &BordLine[1]);
  AlgCom_LineParABCbyTwoPoints(OrgObj1Pt, Obj1InnerPt, &BordLine[0]);

  TempCarPos.Coordinate = pMapMainSlotBord->Points[pMapMainSlotBord->PtNum - 1];
  TempCarPos.CarAng = OuterBorderFusAng;
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  TempCarPos.Coordinate = Obj2ValidPt[ValidObj2Num - 1];
  TempCarPos.CarAng = Angle;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SDGObj2EndPt);

  TempCarPos.Coordinate = pMapMainSlotBord->Points[0];
  TempCarPos.CarAng = OuterBorderFusAng;
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  TempCarPos.Coordinate = Obj1ValidPt[0];
  TempCarPos.CarAng = Angle;
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);
  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SDGObj1StrPt);
  bCallNewObjPt = FALSE;
  bCrossBordLine = FALSE;
  SDGDataPtNum = ValidObj2Num;
  pSDGData = &Obj2ValidPt[0];
  CrossPtIndex = 0;
  NewObjPt = OrgObj2Pt;
  for (i = SDGDataPtNum - 1; i >= 0; i--) {
    if (i == SDGDataPtNum - 1) {
      StrPt = SDGObj2EndPt;
      EndPt = pSDGData[i];
    } else {
      StrPt = pSDGData[i + 1];
      EndPt = pSDGData[i];
    }
    if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
      if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &BordLine[1], &TempPt)) {
        if (TRUE ==
            AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
          TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              TempPt, 0, Angle, OrgObj2Pt);
          if (((TempPt1.x < 0) && (slot_data_at_right_side == TRUE)) ||
              ((TempPt1.x > 0) && (slot_data_at_right_side == FALSE))) {
            bCallNewObjPt = TRUE;
            NewObjPt = TempPt;
          }
          bCrossBordLine = TRUE;
          CrossPtIndex = i;
          break;
        }
      }
    }
  }
  if (bCrossBordLine == FALSE) {
    SDGObj2OutPtNum = 0;
    for (i = 0; i < SDGDataPtNum; i++) {
      SDGObj2OuterInfo[SDGObj2OutPtNum] = pSDGData[i];
      SDGObj2OutPtNum++;
    }
    SDGObj2OuterInfo[SDGObj2OutPtNum] = SDGObj2EndPt;
    SDGObj2OutPtNum++;
    SDGObj2InnerPtNum = 0;
  } else {
    SDGObj2OutPtNum = 0;
    for (i = CrossPtIndex; i < SDGDataPtNum; i++) {
      SDGObj2OuterInfo[SDGObj2OutPtNum] = pSDGData[i];
      SDGObj2OutPtNum++;
    }
    SDGObj2OuterInfo[SDGObj2OutPtNum] = SDGObj2EndPt;
    SDGObj2OutPtNum++;

    SDGObj2InnerPtNum = 0;
    k = CrossPtIndex + 1;
    if (k > SDGDataPtNum - 1) {
      k = SDGDataPtNum - 1;
    }
    for (i = 0; i <= k; i++) {
      SDGObj2InnerInfo[SDGObj2InnerPtNum] = pSDGData[i];
      SDGObj2InnerPtNum++;
    }
  }
  i = 0;
  CrossPtIndex = 0;
  while (i < SDGObj2OutPtNum - 1) {
    TempPt = SDGObj2OuterInfo[i];
    TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(TempPt, 0,
                                                                 Angle, Pto);
    if (i == 0) {
      PrePtY = TempPt.y;
    } else if (TempPt.y < PrePtY) {
      CrossPtIndex = i;
    }
    i++;
  }
  k = 0;
  for (i = CrossPtIndex; i < SDGObj2OutPtNum; i++) {
    SDGObj2OuterInfo[k] = SDGObj2OuterInfo[i];
    k++;
  }
  SDGObj2OutPtNum = k;

  if (bCallNewObjPt == TRUE) {
    pData4[Data4Num - 1] = NewObjPt;
    i = 0;
    while (i < Data2Num) {
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData2[i], 0, Angle, NewObjPt);
      if (TempPt1.y >= 0) {
        if (TempPt1.y == 0) {
          if (((TempPt1.x < 0) && (slot_data_at_right_side == TRUE)) ||
              ((TempPt1.x > 0) && (slot_data_at_right_side == FALSE))) {
            break;
          }
        } else {
          break;
        }
      }
      i++;
    }
    pData2[0] = NewObjPt;
    Index = 1;
    if (i == 0) {
      k = Data2Num;
      while (k > 0) {
        pData2[k + 1] = pData2[k];
        NewProperty2[k + 1] = NewProperty2[k];
        Index++;
        k--;
      }
    } else {
      for (k = i; k < Data2Num; k++) {
        pData2[Index] = pData2[k];
        NewProperty2[Index] = NewProperty2[k];
        Index++;
      }
    }
    Data2Num = Index;
  }

  if (TRUE == APAMap_FusTwoLineSegments(
                  slot_data_at_right_side, OuterBorderFusAng, &pData2[0], Data2Num,
                  &NewProperty2[0], &SDGObj2OuterInfo[0], SDGObj2OutPtNum,
                  &SDGSegProperty[0], &pData2[0], &Data2Num, &pPtStyle[0])) {
    // updata obj2 bordline;
    for (k = 0; k < Data2Num; k++) {
      NewProperty2[k] = pPtStyle[k];
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj2OuterBorderSuccess==");
    TLOG_INFO << log_string;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj2OuterBorder==(%f)==Data2Num(%d)==InfoNum(%d)=="
             "Data1:0(%f,%f)1(%f,%f)2(%f,%f)"
             "==SDGObj2OuterInfo:0(%f,%f)1(%f,%f)2(%f,%f)",
             OuterBorderFusAng, Data2Num, SDGObj2OutPtNum, pData2[0].x,
             pData2[0].y, pData2[1].x, pData2[1].y, pData2[2].x, pData2[2].y,
             SDGObj2OuterInfo[0].x, SDGObj2OuterInfo[0].y,
             SDGObj2OuterInfo[1].x, SDGObj2OuterInfo[1].y,
             SDGObj2OuterInfo[2].x, SDGObj2OuterInfo[2].y);
    TLOG_INFO << log_string;
  }
  if (TRUE == APAMap_FusTwoLineSegments(
                  slot_data_at_right_side, Obj2InnerBorderFusAng, &pData4[0],
                  Data4Num, &NewProperty4[0], &SDGObj2InnerInfo[0],
                  SDGObj2InnerPtNum, &SDGSegProperty[0], &pData4[0], &Data4Num,
                  &pPtStyle[0])) {
    // updata obj2 Innerbordline;
    for (k = 0; k < Data4Num; k++) {
      NewProperty4[k] = pPtStyle[k];
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj2InnerBorderSuccess==");
    TLOG_INFO << log_string;
  }

  bCallNewObjPt = FALSE;
  bCrossBordLine = FALSE;
  SDGDataPtNum = ValidObj1Num;
  pSDGData = &Obj1ValidPt[0];
  CrossPtIndex = 0;
  NewObjPt = OrgObj1Pt;
  for (i = 0; i < SDGDataPtNum; i++) {
    if (i == 0) {
      StrPt = SDGObj1StrPt;
      EndPt = pSDGData[0];
    } else {
      StrPt = pSDGData[i - 1];
      EndPt = pSDGData[i];
    }
    if (TRUE == AlgCom_LineParABCbyTwoPoints(StrPt, EndPt, &TempLine1)) {
      if (1 == AlgCom_CrossPointOfTwoLines(&TempLine1, &BordLine[0], &TempPt)) {
        if (TRUE ==
            AlgCom_ChkIfPtOnLineIsWithinGivenLineSeg(&StrPt, &EndPt, &TempPt)) {
          TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              TempPt, 0, Angle, OrgObj1Pt);
          if (((TempPt1.x < 0) && (slot_data_at_right_side == TRUE)) ||
              ((TempPt1.x > 0) && (slot_data_at_right_side == FALSE))) {
            bCallNewObjPt = TRUE;
            NewObjPt = TempPt;
          }
          bCrossBordLine = TRUE;
          CrossPtIndex = i;
          break;
        }
      }
    }
  }
  if (bCrossBordLine == FALSE) {
    SDGObj1OutPtNum = 1;
    SDGObj1OuterInfo[0] = SDGObj1StrPt;
    for (i = 0; i < SDGDataPtNum; i++) {
      SDGObj1OuterInfo[SDGObj1OutPtNum] = pSDGData[i];
      SDGObj1OutPtNum++;
    }
    SDGObj1InnerPtNum = 0;
  } else {
    SDGObj1OutPtNum = 1;
    SDGObj1OuterInfo[0] = SDGObj1StrPt;
    for (i = 0; i <= CrossPtIndex; i++) {
      SDGObj1OuterInfo[SDGObj1OutPtNum] = pSDGData[i];
      SDGObj1OutPtNum++;
    }
    SDGObj1InnerPtNum = 0;
    k = CrossPtIndex - 1;
    if (k < 0) {
      k = 0;
    }
    for (i = k; i <= SDGDataPtNum - 1; i++) {
      SDGObj1InnerInfo[SDGObj1InnerPtNum] = pSDGData[i];
      SDGObj1InnerPtNum++;
    }
  }
  i = 0;
  CrossPtIndex = SDGObj1OutPtNum;
  while (i < SDGObj1OutPtNum - 1) {
    TempPt = SDGObj1OuterInfo[i];
    TempPt = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(TempPt, 0,
                                                                 Angle, Pto);
    if (i == 0) {
      PrePtY = TempPt.y;
    } else if (TempPt.y < PrePtY) {
      CrossPtIndex = i;
    }
    i++;
  }
  k = 0;
  for (i = 0; i < CrossPtIndex; i++) {
    SDGObj1OuterInfo[k] = SDGObj1OuterInfo[i];
    k++;
  }
  SDGObj1OutPtNum = k;
  if (bCallNewObjPt == TRUE) {
    pData3[0] = NewObjPt;
    i = Data1Num - 1;
    while (i >= 0) {
      TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          pData1[i], 0, Angle, NewObjPt);
      if (TempPt1.y >= 0) {
        if (TempPt1.y == 0) {
          if (((TempPt1.x > 0) && (slot_data_at_right_side == TRUE)) ||
              ((TempPt1.x < 0) && (slot_data_at_right_side == FALSE))) {
            break;
          }
        }
      } else {
        i++;
        break;
      }
      i--;
    }
    Index = 0;
    for (k = 0; k < i; k++) {
      pData1[Index] = pData1[k];
      NewProperty1[Index] = NewProperty1[k];
      Index++;
    }
    pData1[Index] = NewObjPt;
    Index++;
    Data1Num = Index;
  }

  if (TRUE == APAMap_FusTwoLineSegments(
                  slot_data_at_right_side, OuterBorderFusAng, &pData1[0], Data1Num,
                  &NewProperty1[0], &SDGObj1OuterInfo[0], SDGObj1OutPtNum,
                  &SDGSegProperty[0], &pData1[0], &Data1Num, &pPtStyle[0])) {
    // updata obj1 bordline;
    for (k = 0; k < Data1Num; k++) {
      NewProperty1[k] = pPtStyle[k];
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj1OuterBorderSuccess==");
    TLOG_INFO << log_string;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj1OuterBorder==(%f)==Data1Num(%d)==InfoNum(%d)=="
             "Data1:0(%f,%f)1(%f,%f)2(%f,%f)"
             "==SDGObj1OuterInfo:0(%f,%f)1(%f,%f)2(%f,%f)",
             OuterBorderFusAng, Data1Num, SDGObj1OutPtNum, pData1[0].x,
             pData1[0].y, pData1[1].x, pData1[1].y, pData1[2].x, pData1[2].y,
             SDGObj1OuterInfo[0].x, SDGObj1OuterInfo[0].y,
             SDGObj1OuterInfo[1].x, SDGObj1OuterInfo[1].y,
             SDGObj1OuterInfo[2].x, SDGObj1OuterInfo[2].y);
    TLOG_INFO << log_string;
  }

  // 3
  if (TRUE == APAMap_FusTwoLineSegments(
                  slot_data_at_right_side, Obj1InnerBorderFusAng, &pData3[0],
                  Data3Num, &NewProperty3[0], &SDGObj1InnerInfo[0],
                  SDGObj1InnerPtNum, &SDGSegProperty[0], &pData3[0], &Data3Num,
                  &pPtStyle[0])) {
    // updata obj1 Innerbordline;
    for (k = 0; k < Data3Num; k++) {
      NewProperty3[k] = pPtStyle[k];
    }
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==SDGFusionObj1InnerBorderSuccess==");
    TLOG_INFO << log_string;
  }
  DataNum = Data1Num + Data3Num + Data5Num + Data4Num + Data2Num - 2;
  if (DataNum <= BOUNDARY_PT_MAX_NUM) {
    Index = 0;
    for (k = 0; k < Data1Num; k++) {
      pMapMainSlotBord->Points[Index] = pData1[Index];
      pMapMainSlotBord->Property[Index] = NewProperty1[Index];
      Index++;
    }
    APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
    for (k = 1; k < Data3Num; k++) {
      pMapMainSlotBord->Points[Index] = pData3[k];
      pMapMainSlotBord->Property[Index] = NewProperty3[k];
      Index++;
    }
    APAMap_GInfo.SlotPar.SlotStrIndex = Index - 1;

    for (k = 0; k < Data5Num; k++) {
      pMapMainSlotBord->Points[Index] = pData5[k];
      pMapMainSlotBord->Property[Index] = NewProperty5[k];
      Index++;
    }
    APAMap_GInfo.SlotPar.SlotEndIndex = Index;
    for (k = 0; k < Data4Num - 1; k++) {
      pMapMainSlotBord->Points[Index] = pData4[k];
      pMapMainSlotBord->Property[Index] = NewProperty4[k];
      Index++;
    }
    APAMap_GInfo.SlotPar.Obj2PtIndex = Index;
    for (k = 0; k < Data2Num; k++) {
      pMapMainSlotBord->Points[Index] = pData2[k];
      pMapMainSlotBord->Property[Index] = NewProperty2[k];
      Index++;
    }
    pMapMainSlotBord->PtNum = DataNum;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FSDFusionSubSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  {
    char log_string[512];
    snprintf(
        log_string, sizeof(log_string),
        "==FusBordBySDG==SlotIndex(%d,%d,%d,%d)==OrgSDGPtNum(%d,%d)=="
        "ValidSDGPtNum(%d,%d)"
        "==SDGObj2OutPtNum(%d)==SDGObj2InnerPtNum(%d)==SDGObj1OutPtNum(%d)=="
        "SDGObj1InnerPtNum(%d)",
        APAMap_GInfo.SlotPar.Obj1PtIndex, APAMap_GInfo.SlotPar.SlotStrIndex,
        APAMap_GInfo.SlotPar.SlotEndIndex, APAMap_GInfo.SlotPar.Obj2PtIndex,
        Obj2PtNum, Obj1PtNum, ValidObj2Num, ValidObj1Num, SDGObj2OutPtNum,
        SDGObj2InnerPtNum, SDGObj1OutPtNum, SDGObj1InnerPtNum);
    TLOG_INFO << log_string;
  }
  return TRUE;
}

tAPAParkProcEightParkingOutModeType APAMap_ParkingOutGetEightParkOutMode() {
  return s_parking_out_state.eight_mode;
}

void APAMap_ParkingOutCarPosInvadeSlotBorderInfo(
    APACoordinateDataCalFloatType* pObj2Pt,
    APACoordinateDataCalFloatType* pObj1Pt, BOOLEAN bCarLeaveSlotFlag) {
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateY;
  APA_DISTANCE_CAL_FLOAT_TYPE Temp1, Temp2;
  uint8_t_INF park_out_mode;
  APACoordinateDataCalFloatType Obj2Pt, Obj1Pt;
  BOOLEAN bBloundarySeizeCarFlag;  // 边界侵占车辆标志位
  APA_DISTANCE_CAL_FLOAT_TYPE CarLRCal;
  APA_DISTANCE_CAL_FLOAT_TYPE CarLFCal;

  if (FALSE == s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后
  {
    return;
  }
  if (TRUE == bCarLeaveSlotFlag)  // 判断车离开车位框后
  {
    return;
  }
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if (park_out_mode != APA_PARKPROC_PARKING_OUT_MODE_PARALLEL) {
    return;
  }
  Obj2Pt = *pObj2Pt;
  Obj1Pt = *pObj1Pt;
  bBloundarySeizeCarFlag = FALSE;
  CurCarCoordinateY = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.y;
  CarLRCal = APAMap_ComCfg.LenBetweenRAxisAndRBumper;  // mm, 800
  CarLFCal = APAMap_ComCfg.LenBetweenRAxisAndFBumper;  // mm, 3000
  Temp1 = CurCarCoordinateY - CarLRCal - 100;
  Temp2 = CurCarCoordinateY + CarLFCal + 100;
  if (Temp1 < Obj1Pt.y) {
    bBloundarySeizeCarFlag = TRUE;
    Obj1Pt.y = Temp1 - 300;
  }
  if (Temp2 > Obj2Pt.y) {
    bBloundarySeizeCarFlag = TRUE;
    Obj2Pt.y = Temp2 + 300;
  }
  *pObj2Pt = Obj2Pt;
  *pObj1Pt = Obj1Pt;
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==APAMap_ParkingOutCarPosInvadeSlotBorderInfo=="
             "bBloundarySeizeCarFlag(%d)"
             "==Obj2Pt(%.2f,%.2f)==Obj1Pt(%.2f,%.2f)==Temp2(%.2f)==Temp1(%.2f)",
             bBloundarySeizeCarFlag, Obj2Pt.x, Obj2Pt.y, Obj1Pt.x, Obj1Pt.y,
             Temp2, Temp1);
    TLOG_INFO << log_string;
  }
  return;
}

void APAMap_ParkingOutSideSlotInfo(BOOLEAN* pbUpdataCalBoundaryFlag) {
  BOOLEAN bUpdataCalBoundaryFlag;
  APA_DISTANCE_CAL_FLOAT_TYPE CurCarCoordinateX;
  BOOLEAN slot_data_at_right_side;
  uint8_t_INF park_out_mode;
  bUpdataCalBoundaryFlag = *pbUpdataCalBoundaryFlag;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  CurCarCoordinateX = APAMap_GInputData.CarLocInfo.CarPos.Coordinate.x * 0.001;
  park_out_mode = APAMap_GInputData.ParkReqPar.parkoutmode;
  if (TRUE ==
      s_parking_out_state.flags.after_new_anchor_point)  // 判断在锚点转换之后，且车辆已开出车位，则不再初始化主边界
  {
    if (slot_data_at_right_side) {
      CurCarCoordinateX = -CurCarCoordinateX;
    }

    if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_HEAD_TURN_ROUND) {
      if ((CurCarCoordinateX > 1) ||
          (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
                     APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 4))) {
        bUpdataCalBoundaryFlag = FALSE;
      }
    } else if (park_out_mode == APA_PARKPROC_PARKING_OUT_MODE_REAR_TURN_ROUND) {
      if ((CurCarCoordinateX > 2) ||
          (MATH_FABS(APAMap_GInputData.CarLocInfo.CarPos.CarAng -
                     APAMap_GInfo.SlotPar.EndPos.CarAng) < (M_PI / 4))) {
        bUpdataCalBoundaryFlag = FALSE;
      }
    } else {
      if (CurCarCoordinateX > 0.3) {
        bUpdataCalBoundaryFlag = FALSE;
      }
    }
    if (TRUE == s_parking_out_state.flags.label_angled) {
      if (TRUE == s_parking_out_state.flags.label_angled_parking_out_slot) {
        bUpdataCalBoundaryFlag = FALSE;
      }
    }
    if (TRUE == s_parking_out_state.flags.cnt_add) {
      bUpdataCalBoundaryFlag = TRUE;
    }
  }
  *pbUpdataCalBoundaryFlag = bUpdataCalBoundaryFlag;
  return;
}

void APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus(void) {
  BOOLEAN slot_data_at_right_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  tMap_BoundPt_t* pSlotBordBk;
  APA_ENUM_TYPE i;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (APAMap_BkDataBfPDCFus.FusPDCStatus == APAMap_FusPDCStatus_Updata) {
    if (slot_data_at_right_side == FALSE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
    pSlotBordBk = &APAMap_BkDataBfPDCFus.MapMainSlotBord;
    if ((pSlotBordBk->PtNum > 0) &&
        (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pSlotBordBk->PtNum; i++) {
        pMapMainSlotBord->Property[i] = pSlotBordBk->Property[i];
        pMapMainSlotBord->Points[i] = pSlotBordBk->Points[i];
      }
      pMapMainSlotBord->PtNum = pSlotBordBk->PtNum;
    }
    APAMap_GInfo.SlotPar.Obj1PtIndex = APAMap_BkDataBfPDCFus.Obj1PtIndex;
    APAMap_GInfo.SlotPar.SlotStrIndex = APAMap_BkDataBfPDCFus.SlotStrIndex;
    APAMap_GInfo.SlotPar.SlotEndIndex = APAMap_BkDataBfPDCFus.SlotEndIndex;
    APAMap_GInfo.SlotPar.Obj2PtIndex = APAMap_BkDataBfPDCFus.Obj2PtIndex;
    pSlotBordBk = &APAMap_BkDataBfPDCFus.MapSubSlotBord;
    if ((pSlotBordBk->PtNum > 0) &&
        (pSlotBordBk->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pSlotBordBk->PtNum; i++) {
        pMapSubSlotBord->Property[i] = pSlotBordBk->Property[i];
        pMapSubSlotBord->Points[i] = pSlotBordBk->Points[i];
      }
      pMapSubSlotBord->PtNum = pSlotBordBk->PtNum;
    }
    APAMap_BkDataBfPDCFus.timestamp_ms =
        APAMap_GInputData.CarLocInfo.timeStamp_ms;
    APAMap_BkDataBfPDCFus.OrgSysAtGMap =
        APAMap_GInputData.CarLocInfo.OrgSysAtGMap;
  }
  return;
}

void APAMap_ParkingOutUpDataMapBoundaryByPDCInfo(void) {
  APACoordinateDataCalFloatType pMainSlotPDCInfo[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
  APACoordinateDataCalFloatType pSubSlotPDCInfo[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
  uint8_INF pMainSlotPtID[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
  uint8_INF pSubSlotPtID[APA_MAP_BK_PDC_OUTPUT_MAX_NUM];
  APA_ENUM_TYPE MainSidePtNum;
  APA_ENUM_TYPE SubSidePtNum;
  if (TRUE == APAMap_ParkingOutCheckIfFusBoundaryPDCInfo()) {
    if (TRUE == APAMap_ParkingOutGetPDCInfoPt(
                    &pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0], pMainSlotPtID,
                    pSubSlotPtID, &MainSidePtNum, &SubSidePtNum)) {
    } else {
      APAMap_GetBkPDCOutPutData(&pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0],
                                pMainSlotPtID, pSubSlotPtID, &MainSidePtNum,
                                &SubSidePtNum);
    }
    if ((MainSidePtNum > 0) || (SubSidePtNum > 0)) {
      APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Updata;
      APAMap_SaveBkPDCOutPutData(&pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0],
                                 pMainSlotPtID, pSubSlotPtID, MainSidePtNum,
                                 SubSidePtNum);
      APAMap_ParkingOutSaveBkDataBfPDCFus();
      APAMap_ParkingOutFusBoundaryByPDCInfo(
          &pMainSlotPDCInfo[0], &pSubSlotPDCInfo[0], pMainSlotPtID,
          pSubSlotPtID, MainSidePtNum, SubSidePtNum);
      char log_string[512];
      snprintf(log_string, sizeof(log_string), "==PDCFusionBorderUpData!==");
      TLOG_INFO << log_string;
    }
  }
  {
    char log_string[512];
    snprintf(
        log_string, sizeof(log_string),
        "==ParkingOutFusPDCStatus:(%d)==BkDataBfPDCFus:Index(%d,%d,%d,%d),"
        "BkBordPtNum(%d,%d)==BkPDCOutPutDataNum:(%d,%d)",
        APAMap_BkDataBfPDCFus.FusPDCStatus, APAMap_BkDataBfPDCFus.SlotStrIndex,
        APAMap_BkDataBfPDCFus.SlotEndIndex, APAMap_BkDataBfPDCFus.Obj1PtIndex,
        APAMap_BkDataBfPDCFus.Obj2PtIndex,
        APAMap_BkDataBfPDCFus.MapMainSlotBord.PtNum,
        APAMap_BkDataBfPDCFus.MapSubSlotBord.PtNum,
        APAMap_BkPDCOutPutData.PDCMainSidePtNum,
        APAMap_BkPDCOutPutData.PDCSubSidePtNum);
    TLOG_INFO << log_string;
  }
  return;
}

BOOLEAN APAMap_ParkingOutCheckIfFusBoundaryPDCInfo(void) {
  if (APAMap_BkDataBfSDGFus.FusSDGStatus == APAMap_FusSDGStatus_Keep) {
    APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Updata;
    return TRUE;
  } else {
    APAMap_BkDataBfPDCFus.FusPDCStatus = APAMap_FusPDCStatus_Invalid;
    return FALSE;
  }
}

BOOLEAN APAMap_ParkingOutGetPDCInfoPt(
    APACoordinateDataCalFloatType* pPDCMainSlotInfo,
    APACoordinateDataCalFloatType* pPDCSubSlotInfo, uint8_INF* pMainSlotPtID,
    uint8_INF* pSubSlotPtID, APA_ENUM_TYPE* pPDCMainSidePtNum,
    APA_ENUM_TYPE* pPDCSubSidePtNum) {
#if 1  // #ifdef APAMAP_USE_PDC_OBJS

#if 0
   APACoordinateDataCalFloatType Obj1PDCPt[5];
   APACoordinateDataCalFloatType Obj2PDCPt[5];
   APA_DISTANCE_CAL_FLOAT_TYPE Angle;
   APACoordinateDataCalFloatType Pto;
   BOOLEAN slot_data_at_right_side;
   APA_ENUM_TYPE i;
   Pto = APAMap_GInfo.NewCordSysOPt;
   Angle = APAMap_GInfo.NewCordSysAng;
   slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
   for(i = 0; i < 5;i++)
   {
     Obj2PDCPt[i].x = 0;
     Obj2PDCPt[i].y = 0;
     Obj1PDCPt[i].x = 0;
     Obj1PDCPt[i].y = 0;
   }
   *pObj2PtNum = 2;
   *pObj1PtNum = 2;
   Obj2PDCPt[0].x = 3500;
   Obj2PDCPt[0].y = 200;//-100;
   Obj2PDCPt[1].x = 0;
   Obj2PDCPt[1].y = -100;//200;
   

   Obj1PDCPt[0].x = 0;
   Obj1PDCPt[0].y = -3900;//-4200;
   Obj1PDCPt[1].x = 3500;
   Obj1PDCPt[1].y = -4200;//-3900;
   if(slot_data_at_right_side == FALSE)
   {
      for(i = 0; i < *pObj2PtNum;i++)
      {
         Obj2PDCPt[i].x = -Obj2PDCPt[i].x;
      }
      for(i = 0; i < *pObj1PtNum;i++)
      {
         Obj1PDCPt[i].x = -Obj1PDCPt[i].x;
      }
   }
   for(i = 0; i < *pObj2PtNum;i++)
   {
      pPDCObj2Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj2PDCPt[i],Angle,Pto);
   }
   for(i = 0; i < *pObj1PtNum;i++)
   {
      pPDCObj1Info[i] = AlgCom_PointPosWithAngAndCenterPt(Obj1PDCPt[i],Angle,Pto);
   }
   {
    char log_string[1024];
      snprintf(log_string, sizeof(log_string),
            "==PDCDebugData==Obj2(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)"
            "==Obj1(%d):0(%.2f,%.2f),1(%.2f,%.2f),2(%.2f,%.2f),3(%.2f,%.2f),4(%.2f,%.2f)",
            *pObj2PtNum,
            pPDCObj2Info[0].x,pPDCObj2Info[0].y,
            pPDCObj2Info[1].x,pPDCObj2Info[1].y,
            pPDCObj2Info[2].x,pPDCObj2Info[2].y,
            pPDCObj2Info[3].x,pPDCObj2Info[3].y,
            pPDCObj2Info[4].x,pPDCObj2Info[4].y,
            *pObj1PtNum,
            pPDCObj1Info[0].x,pPDCObj1Info[0].y,
            pPDCObj1Info[1].x,pPDCObj1Info[1].y,
            pPDCObj1Info[2].x,pPDCObj1Info[2].y,
            pPDCObj1Info[3].x,pPDCObj1Info[3].y,
            pPDCObj1Info[4].x,pPDCObj1Info[4].y);
      TLOG_INFO << log_string;
    }
   return TRUE;
#endif

  APACoordinateDataCalFloatType LeftSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  APACoordinateDataCalFloatType RightSeg[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  uint8_INF pLeftPtID[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  uint8_INF pRightPtID[MAP_US_OBJ_EXTR_MAX_NUM * 2];
  uint8_INF LeftSegNum;
  uint8_INF RightSegNum;
  // APACarCoordinateDataCalFloatType CurCarPos;
  // CurCarPos = APAMap_GInfo.CarPos;
  APAMap_ParkingOutSiftAndSeqPDCPts(&LeftSeg[0], &RightSeg[0], pLeftPtID,
                                    pRightPtID, &LeftSegNum, &RightSegNum);
  if (TRUE == APAMap_ParkingOutGetPDCInfoByParkSide(
                  &LeftSeg[0], &RightSeg[0], pLeftPtID, pRightPtID, LeftSegNum,
                  RightSegNum, pPDCMainSlotInfo, pPDCSubSlotInfo, pMainSlotPtID,
                  pSubSlotPtID, pPDCMainSidePtNum, pPDCSubSidePtNum)) {
    return TRUE;
  } else {
    return FALSE;
  }
#else
  return FALSE;
#endif
}

void APAMap_ParkingOutSiftAndSeqPDCPts(APACoordinateDataCalFloatType* pLeftSeg,
                                       APACoordinateDataCalFloatType* pRightSeg,
                                       uint8_INF* pLeftPtID,
                                       uint8_INF* pRightPtID,
                                       uint8_INF* pLeftSegNum,
                                       uint8_INF* pRightSegNum) {
#ifdef APAMAP_PARKOUT_FUS_PDC
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  APACoordinateDataCalFloatType TempSeg[2];
  APACoordinateDataCalFloatType TempPt;

  APA_DISTANCE_TYPE i, j, k;
  APA_ENUM_TYPE ObjLocIndex;
  APA_ENUM_TYPE CurLoc;
  APACoordinateDataCalFloatType* pTarSeg;
  uint8_INF* pTarSegNum;
  // APACoordinateDataCalFloatType pRectPt[4];
  // APALineParameterABCType pRectLine[4];
  // APACarCoordinateDataCalFloatType TempCarPos;
  // APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  st_MapUSS* pPDCInfo;
  uint8_INF PtID;
  uint8_INF* pTargetID;
  APA_DISTANCE_CAL_FLOAT_TYPE TempDis;
  APA_DISTANCE_CAL_FLOAT_TYPE CenterX;
  BOOLEAN slot_data_at_right_side;
  *pLeftSegNum = 0;
  *pRightSegNum = 0;
  pPDCInfo = &APAMap_GInputData.TotalMapInfo.mapData.USSObjInfo;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
#ifdef APAMAP_PARKOUT_PCDEMO_USE_DEFAULT_SDG_OBJS
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  pPDCInfo->ObjNum = 6;
  pPDCInfo->Obj[0].Pt[0].x = 0;
  pPDCInfo->Obj[0].Pt[0].y = 0;
  pPDCInfo->Obj[0].Pt[1].x = -300;
  pPDCInfo->Obj[0].Pt[1].y = 300;
  pPDCInfo->Obj[1].Pt[0].x = -300;
  pPDCInfo->Obj[1].Pt[0].y = 3500;
  pPDCInfo->Obj[1].Pt[1].x = -400;
  pPDCInfo->Obj[1].Pt[1].y = 2500;
  pPDCInfo->Obj[2].Pt[0].x = -6000;
  pPDCInfo->Obj[2].Pt[0].y = 3000;
  pPDCInfo->Obj[2].Pt[1].x = -5500;
  pPDCInfo->Obj[2].Pt[1].y = -1000;
  pPDCInfo->Obj[3].Pt[0].x = -5000;
  pPDCInfo->Obj[3].Pt[0].y = -2000;
  pPDCInfo->Obj[3].Pt[1].x = -6500;
  pPDCInfo->Obj[3].Pt[1].y = -3000;
  pPDCInfo->Obj[4].Pt[0].x = -400;
  pPDCInfo->Obj[4].Pt[0].y = -5000;
  pPDCInfo->Obj[4].Pt[1].x = -300;
  pPDCInfo->Obj[4].Pt[1].y = -4000;
  pPDCInfo->Obj[5].Pt[0].x = -300;
  pPDCInfo->Obj[5].Pt[0].y = -3300;
  pPDCInfo->Obj[5].Pt[1].x = 0;
  pPDCInfo->Obj[5].Pt[1].y = -3000;
  if (slot_data_at_right_side == FALSE) {
    for (i = 0; i < pPDCInfo->ObjNum; i++) {
      pPDCInfo->Obj[i].Pt[0].x = -pPDCInfo->Obj[i].Pt[0].x;
      pPDCInfo->Obj[i].Pt[1].x = -pPDCInfo->Obj[i].Pt[1].x;
    }
  }
  for (i = 0; i < pPDCInfo->ObjNum; i++) {
    TempPt.x = pPDCInfo->Obj[i].Pt[0].x;
    TempPt.y = pPDCInfo->Obj[i].Pt[0].y;
    TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    pPDCInfo->Obj[i].Pt[0].x = TempPt.x;
    pPDCInfo->Obj[i].Pt[0].y = TempPt.y;
    TempPt.x = pPDCInfo->Obj[i].Pt[1].x;
    TempPt.y = pPDCInfo->Obj[i].Pt[1].y;
    TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    pPDCInfo->Obj[i].Pt[1].x = TempPt.x;
    pPDCInfo->Obj[i].Pt[1].y = TempPt.y;
  }
#else
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return;
  }
#endif
  if (APAMap_GInputData.TotalMapInfo.mapData.TimeStamp == 0) {
    return;
  }
  if ((pPDCInfo->ObjNum <= 0) || (pPDCInfo->ObjNum > MAP_US_OBJ_EXTR_MAX_NUM)) {
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==NoPDCPts:(%d)",
             pPDCInfo->ObjNum);
    TLOG_INFO << log_string;
    return;
  }
  if (pPDCInfo->ObjNum > 0) {
    char log_string[1024];
    snprintf(log_string, sizeof(log_string),
             "==OrgPDCPts==(%d):0[(%d,%d),(%d,%d)],1[(%d,%d),(%d,%d)],2[(%d,%d)"
             ",(%d,%d)],"
             "3[(%d,%d),(%d,%d)],4[(%d,%d),(%d,%d)],5[(%d,%d),(%d,%d)],6[(%d,%"
             "d),(%d,%d)],7[(%d,%d),(%d,%d)],"
             "8[(%d,%d),(%d,%d)],9[(%d,%d),(%d,%d)],10[(%d,%d),(%d,%d)],11[(%d,"
             "%d),(%d,%d)],12[(%d,%d),(%d,%d)]",
             pPDCInfo->ObjNum, pPDCInfo->Obj[0].Pt[0].x,
             pPDCInfo->Obj[0].Pt[0].y, pPDCInfo->Obj[0].Pt[1].x,
             pPDCInfo->Obj[0].Pt[1].y, pPDCInfo->Obj[1].Pt[0].x,
             pPDCInfo->Obj[1].Pt[0].y, pPDCInfo->Obj[1].Pt[1].x,
             pPDCInfo->Obj[1].Pt[1].y, pPDCInfo->Obj[2].Pt[0].x,
             pPDCInfo->Obj[2].Pt[0].y, pPDCInfo->Obj[2].Pt[1].x,
             pPDCInfo->Obj[2].Pt[1].y, pPDCInfo->Obj[3].Pt[0].x,
             pPDCInfo->Obj[3].Pt[0].y, pPDCInfo->Obj[3].Pt[1].x,
             pPDCInfo->Obj[3].Pt[1].y, pPDCInfo->Obj[4].Pt[0].x,
             pPDCInfo->Obj[4].Pt[0].y, pPDCInfo->Obj[4].Pt[1].x,
             pPDCInfo->Obj[4].Pt[1].y, pPDCInfo->Obj[5].Pt[0].x,
             pPDCInfo->Obj[5].Pt[0].y, pPDCInfo->Obj[5].Pt[1].x,
             pPDCInfo->Obj[5].Pt[1].y, pPDCInfo->Obj[6].Pt[0].x,
             pPDCInfo->Obj[6].Pt[0].y, pPDCInfo->Obj[6].Pt[1].x,
             pPDCInfo->Obj[6].Pt[1].y, pPDCInfo->Obj[7].Pt[0].x,
             pPDCInfo->Obj[7].Pt[0].y, pPDCInfo->Obj[7].Pt[1].x,
             pPDCInfo->Obj[7].Pt[1].y, pPDCInfo->Obj[8].Pt[0].x,
             pPDCInfo->Obj[8].Pt[0].y, pPDCInfo->Obj[8].Pt[1].x,
             pPDCInfo->Obj[8].Pt[1].y, pPDCInfo->Obj[9].Pt[0].x,
             pPDCInfo->Obj[9].Pt[0].y, pPDCInfo->Obj[9].Pt[1].x,
             pPDCInfo->Obj[9].Pt[1].y, pPDCInfo->Obj[10].Pt[0].x,
             pPDCInfo->Obj[10].Pt[0].y, pPDCInfo->Obj[10].Pt[1].x,
             pPDCInfo->Obj[10].Pt[1].y, pPDCInfo->Obj[11].Pt[0].x,
             pPDCInfo->Obj[11].Pt[0].y, pPDCInfo->Obj[11].Pt[1].x,
             pPDCInfo->Obj[11].Pt[1].y, pPDCInfo->Obj[12].Pt[0].x,
             pPDCInfo->Obj[12].Pt[0].y, pPDCInfo->Obj[12].Pt[1].x,
             pPDCInfo->Obj[12].Pt[1].y);
    TLOG_INFO << log_string;
  }

  // TempCarPos.Coordinate.x = 0;
  // TempCarPos.Coordinate.y = 0;
  // TempCarPos.CarAng = 0;
  // APAMap_GetCarRectArea(100,100,100,100,TempCarPos,&pRectPt[0],&pRectLine[0]);
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (slot_data_at_right_side == TRUE) {
    CenterX = -3000;
  } else {
    CenterX = 3000;
  }
  for (i = 0; i < pPDCInfo->ObjNum; i++) {
    ObjLocIndex = -1;
    for (j = 0; j < 2; j++) {
      TempSeg[j].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pPDCInfo->Obj[i].Pt[j].x;
      TempSeg[j].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pPDCInfo->Obj[i].Pt[j].y;
      TempSeg[j] = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
          TempSeg[j], 0, Angle, Pto);
      if (TempSeg[j].x < CenterX) {
        CurLoc = 0;  // Left;
      } else {
        CurLoc = 1;  // Right;
      }
      if ((ObjLocIndex == -1) || (ObjLocIndex == CurLoc)) {
        ObjLocIndex = CurLoc;
      } else {
        if (TempSeg[j].y > 0) {
          ObjLocIndex = 2;  // Front;
        } else {
          ObjLocIndex = 3;  // Back;
        }
      }
    }
    {
      if (TempSeg[1].y > TempSeg[0].y) {
        TempPt = TempSeg[0];
        TempSeg[0] = TempSeg[1];
        TempSeg[1] = TempPt;
      }
      if (ObjLocIndex == 0) {
        pTarSeg = &pLeftSeg[0];
        pTarSegNum = pLeftSegNum;
      } else if (ObjLocIndex == 1) {
        pTarSeg = &pRightSeg[0];
        pTarSegNum = pRightSegNum;
      } else {
        pTarSeg = NULL;
        pTarSegNum = NULL;
      }
      if (pTarSeg != NULL) {
        j = 0;
        while (j < *pTarSegNum) {
          if (TempSeg[0].y > pTarSeg[j].y) {
            break;
          }
          j += 2;
        }

        k = *pTarSegNum - 1;
        if ((*pTarSegNum + 2) > MAP_US_OBJ_EXTR_MAX_NUM * 2) {
          // buff is not big enough
          return;
        }
        while (k >= j) {
          pTarSeg[k + 2] = pTarSeg[k];
          k--;
        }
        pTarSeg[j] = TempSeg[0];
        pTarSeg[j + 1] = TempSeg[1];
        *pTarSegNum += 2;
      }
    }
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string), "==SiftAndSeqPDCPts:Org(%d,%d)",
             *pLeftSegNum, *pRightSegNum);
    TLOG_INFO << log_string;
  }
  for (j = 0; j < 2; j++) {
    if (j == 0) {
      pTarSeg = &pLeftSeg[0];
      pTarSegNum = pLeftSegNum;
      pTargetID = pLeftPtID;
    } else {
      pTarSeg = &pRightSeg[0];
      pTarSegNum = pRightSegNum;
      pTargetID = pRightPtID;
    }
    PtID = 1;
    i = 0;
    while (i < *pTarSegNum) {
      if (i > 1) {
        TempDis = AlgCom_GetTwoPointDisFloat(pTarSeg[i - 1], pTarSeg[i]);
        if (TempDis > 500) {
          PtID++;
        }
      }
      pTargetID[i] = PtID;
      pTargetID[i + 1] = PtID;
      i += 2;
    };
  }
#else
  *pLeftSegNum = 0;
  *pRightSegNum = 0;
#endif
  return;
}

BOOLEAN APAMap_ParkingOutGetPDCInfoByParkSide(
    APACoordinateDataCalFloatType* pLeftSeg,
    APACoordinateDataCalFloatType* pRightSeg, uint8_INF* pLeftPtID,
    uint8_INF* pRightPtID, uint8_INF u8LeftSegNum, uint8_INF u8RightSegNum,
    APACoordinateDataCalFloatType* pMainSlotPDCInfo,
    APACoordinateDataCalFloatType* pSubSlotPDCInfo, uint8_INF* pMainSlotPtID,
    uint8_INF* pSubSlotPtID, APA_ENUM_TYPE* pMainSlotPDCPtNum,
    APA_ENUM_TYPE* pSubSlotPDCPtNum) {
  APA_ENUM_TYPE i, k;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType* pMainSlotSrc;
  APACoordinateDataCalFloatType* pSubSlotSrc;
  uint8_t_INF* pMainSlotPtIDSrc;
  uint8_t_INF* pSubSlotPtIDSrc;
  APA_ENUM_TYPE MainSidePtNum;
  APA_ENUM_TYPE SubSidePtNum;
  APA_ENUM_TYPE MainSideStrIndex;
  APA_ENUM_TYPE SubSideStrIndex;
  APA_ENUM_TYPE SubSideSearchStep;
  APA_ENUM_TYPE MainSideSearchStep;
  APA_ENUM_TYPE LeftSegNum;
  APA_ENUM_TYPE RightSegNum;
  APACoordinateDataCalFloatType TempPt;
  *pMainSlotPDCPtNum = 0;
  *pSubSlotPDCPtNum = 0;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  if (u8LeftSegNum > APA_MAP_BK_PDC_OUTPUT_MAX_NUM) {
    LeftSegNum = 0;
  } else {
    LeftSegNum = (APA_ENUM_TYPE)u8LeftSegNum;
  }
  if (u8RightSegNum > APA_MAP_BK_PDC_OUTPUT_MAX_NUM) {
    RightSegNum = 0;
  } else {
    RightSegNum = (APA_ENUM_TYPE)u8RightSegNum;
  }
  // AlgCom_SmoothSegMent(TRUE,1000,10,pLeftSeg,&LeftSegNum);
  // AlgCom_SmoothSegMent(FALSE,1000,10,pRightSeg,&RightSegNum);
  if (LeftSegNum > 0) {
    char log_string[1024];
    snprintf(
        log_string, sizeof(log_string),
        "==GetPDCInfoByParkMode==Left(%d):0(%.2f,%.2f,%d),1(%.2f,%.2f,%d),2(%."
        "2f,%.2f,%d),3(%.2f,%.2f,%d),4(%.2f,%.2f,%d),"
        "5(%.2f,%.2f,%d),6(%.2f,%.2f,%d),7(%.2f,%.2f,%d),8(%.2f,%.2f,%d),9(%."
        "2f,%.2f,%d),10(%.2f,%.2f,%d),11(%.2f,%.2f,%d),"
        "12(%.2f,%.2f,%d),13(%.2f,%.2f,%d),14(%.2f,%.2f,%d),15(%.2f,%.2f,%d),"
        "16(%.2f,%.2f,%d),17(%.2f,%.2f,%d),18(%.2f,%.2f,%d),19(%.2f,%.2f,%d)",
        LeftSegNum, pLeftSeg[0].x, pLeftSeg[0].y, pLeftPtID[0], pLeftSeg[1].x,
        pLeftSeg[1].y, pLeftPtID[1], pLeftSeg[2].x, pLeftSeg[2].y, pLeftPtID[2],
        pLeftSeg[3].x, pLeftSeg[3].y, pLeftPtID[3], pLeftSeg[4].x,
        pLeftSeg[4].y, pLeftPtID[4], pLeftSeg[5].x, pLeftSeg[5].y, pLeftPtID[5],
        pLeftSeg[6].x, pLeftSeg[6].y, pLeftPtID[6], pLeftSeg[7].x,
        pLeftSeg[7].y, pLeftPtID[7], pLeftSeg[8].x, pLeftSeg[8].y, pLeftPtID[8],
        pLeftSeg[9].x, pLeftSeg[9].y, pLeftPtID[9], pLeftSeg[10].x,
        pLeftSeg[10].y, pLeftPtID[10], pLeftSeg[11].x, pLeftSeg[11].y,
        pLeftPtID[11], pLeftSeg[12].x, pLeftSeg[12].y, pLeftPtID[12],
        pLeftSeg[13].x, pLeftSeg[13].y, pLeftPtID[13], pLeftSeg[14].x,
        pLeftSeg[14].y, pLeftPtID[14], pLeftSeg[15].x, pLeftSeg[15].y,
        pLeftPtID[15], pLeftSeg[16].x, pLeftSeg[16].y, pLeftPtID[16],
        pLeftSeg[17].x, pLeftSeg[17].y, pLeftPtID[17], pLeftSeg[18].x,
        pLeftSeg[18].y, pLeftPtID[18], pLeftSeg[19].x, pLeftSeg[19].y,
        pLeftPtID[19]);
    TLOG_INFO << log_string;
  }
  if (RightSegNum > 0) {
    char log_string[1024];
    snprintf(
        log_string, sizeof(log_string),
        "==GetPDCInfoByParkMode==Right(%d):0(%.2f,%.2f,%d),1(%.2f,%.2f,%d),2(%."
        "2f,%.2f,%d),3(%.2f,%.2f,%d),4(%.2f,%.2f,%d),"
        "5(%.2f,%.2f,%d),6(%.2f,%.2f,%d),7(%.2f,%.2f,%d),8(%.2f,%.2f,%d),9(%."
        "2f,%.2f,%d),10(%.2f,%.2f,%d),11(%.2f,%.2f,%d),"
        "12(%.2f,%.2f,%d),13(%.2f,%.2f,%d),14(%.2f,%.2f,%d),15(%.2f,%.2f,%d),"
        "16(%.2f,%.2f,%d),17(%.2f,%.2f,%d),18(%.2f,%.2f,%d),19(%.2f,%.2f,%d)",
        RightSegNum, pRightSeg[0].x, pRightSeg[0].y, pRightPtID[0],
        pRightSeg[1].x, pRightSeg[1].y, pRightPtID[1], pRightSeg[2].x,
        pRightSeg[2].y, pRightPtID[2], pRightSeg[3].x, pRightSeg[3].y,
        pRightPtID[3], pRightSeg[4].x, pRightSeg[4].y, pRightPtID[4],
        pRightSeg[5].x, pRightSeg[5].y, pRightPtID[5], pRightSeg[6].x,
        pRightSeg[6].y, pRightPtID[6], pRightSeg[7].x, pRightSeg[7].y,
        pRightPtID[7], pRightSeg[8].x, pRightSeg[8].y, pRightPtID[8],
        pRightSeg[9].x, pRightSeg[9].y, pRightPtID[9], pRightSeg[10].x,
        pRightSeg[10].y, pRightPtID[10], pRightSeg[11].x, pRightSeg[11].y,
        pRightPtID[11], pRightSeg[12].x, pRightSeg[12].y, pRightPtID[12],
        pRightSeg[13].x, pRightSeg[13].y, pRightPtID[13], pRightSeg[14].x,
        pRightSeg[14].y, pRightPtID[14], pRightSeg[15].x, pRightSeg[15].y,
        pRightPtID[15], pRightSeg[16].x, pRightSeg[16].y, pRightPtID[16],
        pRightSeg[17].x, pRightSeg[17].y, pRightPtID[17], pRightSeg[18].x,
        pRightSeg[18].y, pRightPtID[18], pRightSeg[19].x, pRightSeg[19].y,
        pRightPtID[19]);
    TLOG_INFO << log_string;
  }
  if ((LeftSegNum == 0) && (RightSegNum == 0)) {
    return FALSE;
  }
  if (slot_data_at_right_side == TRUE) {
    pMainSlotSrc = pRightSeg;
    MainSidePtNum = RightSegNum;
    pSubSlotSrc = pLeftSeg;
    SubSidePtNum = LeftSegNum;
    pMainSlotPtIDSrc = pRightPtID;
    pSubSlotPtIDSrc = pLeftPtID;
  } else {
    pMainSlotSrc = pLeftSeg;
    MainSidePtNum = LeftSegNum;
    pSubSlotSrc = pRightSeg;
    SubSidePtNum = RightSegNum;
    pMainSlotPtIDSrc = pLeftPtID;
    pSubSlotPtIDSrc = pRightPtID;
  }
  MainSideStrIndex = MainSidePtNum - 1;
  MainSideSearchStep = -1;
  SubSideStrIndex = SubSidePtNum - 1;
  SubSideSearchStep = -1;
  i = MainSideStrIndex;
  k = 0;
  while (k < MainSidePtNum) {
    pMainSlotPtID[k] = pMainSlotPtIDSrc[i];
    TempPt = pMainSlotSrc[i];
    pMainSlotPDCInfo[k] = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    k++;
    i += MainSideSearchStep;
  }
  *pMainSlotPDCPtNum = MainSidePtNum;

  i = SubSideStrIndex;
  k = 0;
  while (k < SubSidePtNum) {
    pSubSlotPtID[k] = pSubSlotPtIDSrc[i];
    TempPt = pSubSlotSrc[i];
    pSubSlotPDCInfo[k] = AlgCom_PointPosWithAngAndCenterPt(TempPt, Angle, Pto);
    k++;
    i += SubSideSearchStep;
  }
  *pSubSlotPDCPtNum = SubSidePtNum;
  return TRUE;
}

void APAMap_ParkingOutSaveBkDataBfPDCFus(void) {
  BOOLEAN slot_data_at_right_side;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  tMap_BoundPt_t* pSlotBordBk;
  APA_ENUM_TYPE i;
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
  if (APAMap_BkDataBfPDCFus.FusPDCStatus == APAMap_FusPDCStatus_Updata) {
    if (slot_data_at_right_side == FALSE) {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    } else {
      pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
      pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    }
    pSlotBordBk = &APAMap_BkDataBfPDCFus.MapMainSlotBord;
    if ((pMapMainSlotBord->PtNum > 0) &&
        (pMapMainSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pMapMainSlotBord->PtNum; i++) {
        pSlotBordBk->Property[i] = pMapMainSlotBord->Property[i];
        pSlotBordBk->Points[i] = pMapMainSlotBord->Points[i];
      }
      pSlotBordBk->PtNum = pMapMainSlotBord->PtNum;
    }
    APAMap_BkDataBfPDCFus.Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
    APAMap_BkDataBfPDCFus.SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
    APAMap_BkDataBfPDCFus.SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
    APAMap_BkDataBfPDCFus.Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
    pSlotBordBk = &APAMap_BkDataBfPDCFus.MapSubSlotBord;
    if ((pMapSubSlotBord->PtNum > 0) &&
        (pMapSubSlotBord->PtNum <= BOUNDARY_PT_MAX_NUM)) {
      for (i = 0; i < pMapSubSlotBord->PtNum; i++) {
        pSlotBordBk->Property[i] = pMapSubSlotBord->Property[i];
        pSlotBordBk->Points[i] = pMapSubSlotBord->Points[i];
      }
      pSlotBordBk->PtNum = pMapSubSlotBord->PtNum;
    }
  }
  return;
}

BOOLEAN APAMap_ParkingOutFusBoundaryByPDCInfo(
    APACoordinateDataCalFloatType* pMainSlotPDCInfo,
    APACoordinateDataCalFloatType* pSubSlotPDCInfo, uint8_INF* pMainSlotPtID,
    uint8_INF* pSubSlotPtID, APA_ENUM_TYPE MainSlotPDCPtNum,
    APA_ENUM_TYPE SubSlotPDCPtNum) {
  APACoordinateDataCalFloatType OrgObj2Pt, OrgObj1Pt;
  APACarCoordinateDataCalFloatType CurCarPos;
  APA_ENUM_TYPE SlotStrIndex, SlotEndIndex;
  APACoordinateDataCalFloatType Pto;
  APA_DISTANCE_CAL_FLOAT_TYPE Angle;

  APA_ENUM_TYPE Index;
  APA_ENUM_TYPE i, k;
  tMap_BoundPt_t* pMapMainSlotBord;
  tMap_BoundPt_t* pMapSubSlotBord;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng;
  APA_DISTANCE_CAL_FLOAT_TYPE TempAng1;
  BOOLEAN slot_data_at_right_side;
  APACoordinateDataCalFloatType TempPt;
  APACoordinateDataCalFloatType Data[127];
  APACoordinateDataCalFloatType pData1[127];
  APACoordinateDataCalFloatType pData2[127];
  APACoordinateDataCalFloatType pData3[127];
  APACoordinateDataCalFloatType pData4[127];
  APACoordinateDataCalFloatType NSegment[127];
  uint8_t_INF pPtStyle[127];
  uint8_t_INF NewProperty1[127];
  uint8_t_INF NewProperty2[127];
  uint8_t_INF NewProperty3[127];
  uint8_t_INF NewProperty4[127];
  uint8_t_INF NSegProperty[127];
  APA_ENUM_TYPE NSegNum;
  uint16_t_INF DataNum;
  APA_ENUM_TYPE Data1Num;
  APA_ENUM_TYPE Data2Num;
  APA_ENUM_TYPE Data3Num;
  APA_ENUM_TYPE Data4Num;
  APA_ENUM_TYPE LocStyle;
  APACoordinateDataCalFloatType MainLinYStrPt, MainLinYEndPt;
  APACoordinateDataCalFloatType MainLinXStrPt1, MainLinXEndPt1;
  APACoordinateDataCalFloatType MainLinXStrPt2, MainLinXEndPt2;

  APACoordinateDataCalFloatType SubLinYStrPt, SubLinYEndPt;
  APACoordinateDataCalFloatType SubLinYStrPt1, SubLinYEndPt1;
  // APA_DISTANCE_CAL_FLOAT_TYPE LineYAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE LineXAngle;
  APA_DISTANCE_CAL_FLOAT_TYPE MaxOffsetX;
  APA_DISTANCE_CAL_FLOAT_TYPE fDis;
  APACoordinateDataCalFloatType pRectPt[4];
  APALineParameterABCType pRectLine[4];
  APACoordinateDataCalFloatType SubLinXStrPt, SubLinXEndPt;
  APALineParameterABCType TempLine1;
  APALineParameterABCType TempLine2;
  APACarCoordinateDataCalFloatType TempCarPos;
  APACoordinateDataCalFloatType TempPt1, TempPt2;
  APA_ENUM_TYPE OffsetIndex2, OffsetIndex1;
  APA_ENUM_TYPE Obj2PtIndex, Obj1PtIndex;
  APACoordinateDataCalFloatType pDataBk[127];
  APA_ENUM_TYPE DataNumBk;
  BOOLEAN bCheckSubLane;
  BOOLEAN bFusvalid;
  uint8_t_INF CurID;
  if ((MainSlotPDCPtNum <= 0) && (SubSlotPDCPtNum <= 0)) {
    return FALSE;
  }
  slot_data_at_right_side = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;

  if (slot_data_at_right_side == FALSE) {
    pMapMainSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
    pMapSubSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
  } else {
    pMapMainSlotBord = &(APAMap_GInfo.OutLine.RightBoundary);
    pMapSubSlotBord = &(APAMap_GInfo.OutLine.LeftBoundary);
  }

  OrgObj2Pt = APAMap_GInfo.SlotPar.SlotBordPt[0];
  OrgObj1Pt = APAMap_GInfo.SlotPar.SlotBordPt[1];
  CurCarPos = APAMap_GInfo.CarPos;
  Pto = APAMap_GInfo.NewCordSysOPt;
  Angle = APAMap_GInfo.NewCordSysAng;
  TempAng = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&TempAng);
  TempAng1 = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle + PI / 2);
  AlgCom_AngNormalized(&TempAng1);
  if (slot_data_at_right_side == FALSE) {
    fDis = TempAng;
    TempAng = TempAng1;
    TempAng1 = fDis;
  }

  SlotEndIndex = APAMap_GInfo.SlotPar.SlotEndIndex;
  SlotStrIndex = APAMap_GInfo.SlotPar.SlotStrIndex;
  Obj2PtIndex = APAMap_GInfo.SlotPar.Obj2PtIndex;
  Obj1PtIndex = APAMap_GInfo.SlotPar.Obj1PtIndex;
  OffsetIndex1 = SlotStrIndex - Obj1PtIndex;
  OffsetIndex2 = Obj2PtIndex - SlotEndIndex;
  MaxOffsetX = 1500;
  if (slot_data_at_right_side == TRUE) {
    MaxOffsetX = -MaxOffsetX;
  }
  MainLinYStrPt.x = MaxOffsetX;
  MainLinYStrPt.y = 0;
  MainLinYEndPt.x = MainLinYStrPt.x;
  MainLinYEndPt.y = 1000;
  MainLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYStrPt, Angle, Pto);
  MainLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(MainLinYEndPt, Angle, Pto);
  // LineYAngle = Angle;

  // obj 1 border line
  MainLinXStrPt1 = OrgObj1Pt;
  MainLinXEndPt1 = OrgObj1Pt;
  LineXAngle = APAMap_GInfo.SlotPar.Obj1Ang;
  MainLinXEndPt1.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt1.y += 1000 * MATH_COS(LineXAngle);
  Data1Num = 0;
  for (Index = 0; Index <= Obj1PtIndex; Index++) {
    pData1[Data1Num] = pMapMainSlotBord->Points[Index];
    NewProperty1[Data1Num] = pMapMainSlotBord->Property[Index];
    Data1Num++;
    if (Data1Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==PDC Buffer Not enough==1==Data1Num:(%d)", Data1Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }

  // obj2 borderline;
  MainLinXStrPt2 = OrgObj2Pt;
  MainLinXEndPt2 = OrgObj2Pt;
  LineXAngle = APAMap_GInfo.SlotPar.Obj2Ang;
  MainLinXEndPt2.x -= 1000 * MATH_SIN(LineXAngle);
  MainLinXEndPt2.y += 1000 * MATH_COS(LineXAngle);
  Data2Num = 0;
  for (Index = Obj2PtIndex; Index < pMapMainSlotBord->PtNum; Index++) {
    pData2[Data2Num] = pMapMainSlotBord->Points[Index];
    NewProperty2[Data2Num] = pMapMainSlotBord->Property[Index];
    Data2Num++;
    if (Data2Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==PDC Buffer Not enough==2==Data2Num:(%d)", Data2Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // data pt in slot;
  Data3Num = 0;
  for (Index = Obj1PtIndex + 1; Index < Obj2PtIndex; Index++) {
    pData3[Data3Num] = pMapMainSlotBord->Points[Index];
    NewProperty3[Data3Num] = pMapMainSlotBord->Property[Index];
    Data3Num++;
    if (Data3Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==PDC Buffer Not enough==3==Data3Num:(%d)", Data3Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  // Fus  Subborder
  MaxOffsetX = 3000;
  if (APAMap_GInputData.ParkReqPar.parkmode ==
      APA_PARKPROC_PARKING_MODE_PARALLEL) {
    MaxOffsetX = 2500;
  } else {
    MaxOffsetX = 3000;
  }
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt.x = TempPt.x;
  SubLinYStrPt.y = 0;
  SubLinYEndPt.x = SubLinYStrPt.x;
  SubLinYEndPt.y = 1000;
  SubLinYStrPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt, Angle, Pto);
  SubLinYEndPt = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt, Angle, Pto);
  // Fus  Subborder
  MaxOffsetX = 3000;
  if (slot_data_at_right_side == TRUE) {
    TempPt.x = -MaxOffsetX;
  } else {
    TempPt.x = MaxOffsetX;
  }
  SubLinYStrPt1.x = TempPt.x;
  SubLinYStrPt1.y = 0;
  SubLinYEndPt1.x = SubLinYStrPt1.x;
  SubLinYEndPt1.y = 1000;
  SubLinYStrPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYStrPt1, Angle, Pto);
  SubLinYEndPt1 = AlgCom_PointPosWithAngAndCenterPt(SubLinYEndPt1, Angle, Pto);

  LineXAngle = (APA_DISTANCE_CAL_FLOAT_TYPE)(Angle - PI / 2);
  AlgCom_AngNormalized(&LineXAngle);
  TempCarPos.CarAng = LineXAngle;

  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine1 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.HalfWidthOfCar;
  TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
  if (TempLine1.C < TempLine2.C) {
    TempLine1.C = TempLine2.C;
  }

  TempPt.y =
      (APA_DISTANCE_CAL_FLOAT_TYPE)APAMap_ComCfg.LenBetweenRAxisAndFBumper;
  TempPt.x = (APA_DISTANCE_CAL_FLOAT_TYPE)-APAMap_ComCfg.HalfWidthOfCar;
  TempPt = AlgCom_PointPosWithAngAndCenterPt(TempPt, CurCarPos.CarAng,
                                             CurCarPos.Coordinate);
  AlgCom_LineParABCByParallelLineAndPointOnLine(&TempLine2, TempPt, &TempLine1);
  if (TempLine1.C < TempLine2.C) {
    TempLine1.C = TempLine2.C;
  }

  TempLine1.C = AlgCom_LineParCByParallelLineAndDisToLine(1000, &TempLine1);

  TempCarPos.CarAng = Angle;
  TempCarPos.Coordinate = APAMap_GInfo.SlotPar.SlotBordPt[0];
  TempLine2 = AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0);

  AlgCom_CrossPointOfTwoLines(&TempLine1, &TempLine2, &SubLinXStrPt);
  SubLinXEndPt = SubLinXStrPt;
  SubLinXEndPt.x -= 1000 * MATH_SIN(LineXAngle);
  SubLinXEndPt.y += 1000 * MATH_COS(LineXAngle);

  Data4Num = 0;
  for (Index = 0; Index < pMapSubSlotBord->PtNum; Index++) {
    pData4[Data4Num] = pMapSubSlotBord->Points[Index];
    NewProperty4[Data4Num] = pMapSubSlotBord->Property[Index];
    Data4Num++;
    if (Data4Num > 127) {
      {
        char log_string[512];
        snprintf(log_string, sizeof(log_string),
                 "==PDC Buffer Not enough==4==Data4Num:(%d)", Data3Num);
        TLOG_INFO << log_string;
      }
      return FALSE;
    }
  }
  DataNumBk = 0;
  bCheckSubLane = FALSE;
  if (((0 < APAMAP_GetBindAlleySlotCase()) &&
       (3 > APAMAP_GetBindAlleySlotCase())) ||
      (APAMap_GInfo.bAddFrontFSDPtToMapBord == TRUE)) {
    bCheckSubLane = TRUE;
  }
  NSegNum = 0;
  APAMap_GetCarRectArea(0, 0, 0, 0, CurCarPos, pRectPt, pRectLine);
  if (MainSlotPDCPtNum > 1) {
    // get fsd data with same id;
    i = 0;

    while (i < MainSlotPDCPtNum) {
      // get fsd data with same id;
      CurID = pMainSlotPtID[i];
      Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i].x;
      Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i].y;
      for (k = 1; k < 100; k++) {
        if ((i + k) < MainSlotPDCPtNum) {
          if (pMainSlotPtID[i + k] != CurID) {
            break;
          } else {
            Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i + k].x;
            Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pMainSlotPDCInfo[i + k].y;
          }
        } else {
          break;
        }
      }
      DataNum = k;
      i += DataNum;
      // Get valid fsd data for fus obj1bordline;
      NSegNum = 0;
      for (k = 0; k < DataNum; k++) {
        TempPt = Data[k];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
        if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &MainLinXStrPt1, &MainLinXEndPt1, &TempPt, &fDis);
          if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        }
      }
      for (k = 0; k < NSegNum; k++) {
        NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
      }
      APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                             Pto, Angle);
      AlgCom_SmoothSegMent(slot_data_at_right_side, 500, 10, &NSegment[0],
                           &NSegNum);
      if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                  pRectPt, pRectLine)) {
        NSegNum = 0;
      }
      if (TRUE == APAMap_FusTwoLineSegments(
                      slot_data_at_right_side, TempAng, &pData1[0], Data1Num,
                      &NewProperty1[0], &NSegment[0], NSegNum, &NSegProperty[0],
                      &pData1[0], &Data1Num, &pPtStyle[0])) {
        // updata obj1 bordline;

        for (k = 0; k < Data1Num; k++) {
          NewProperty1[k] = pPtStyle[k];
        }
        if (Data1Num > 2) {
          TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              pData1[0], 0, Angle, Pto);
          TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              pData1[1], 0, Angle, Pto);
          if ((TempPt1.y >= TempPt2.y) &&
              (((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
               ((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
            TempPt1.x = TempPt2.x;
            pData1[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
            for (k = 1; k < Data1Num - 1; k++) {
              NewProperty1[k] = NewProperty1[k + 1];
              pData1[k] = pData1[k + 1];
            }
            Data1Num--;
          }
        }
        char log_string[512];
        snprintf(log_string, sizeof(log_string), "==PDCFusionObj1Success==");
        TLOG_INFO << log_string;
      }
      //----------------------------------
      // Get valid fsd data for fus obj2bordline;
      NSegNum = 0;
      for (k = 0; k < DataNum; k++) {
        TempPt = Data[k];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &MainLinYStrPt, &MainLinYEndPt, &TempPt, &fDis);
        if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
            ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &MainLinXStrPt2, &MainLinXEndPt2, &TempPt, &fDis);
          if (((LocStyle != 0) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 1) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        }
      }
      for (k = 0; k < NSegNum; k++) {
        NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
      }
      if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                  pRectPt, pRectLine)) {
        NSegNum = 0;
      }
#ifdef SUPPORT_BLIND_ALLEY_SLOT
      if ((0 != APAMAP_GetBindAlleySlotCase()) ||
          (APAMap_GInfo.bAddFrontFSDPtToMapBord == TRUE)) {
        NSegNum = 0;
      }
#endif
      APAMap_ReOderSegmentPt(TRUE, slot_data_at_right_side, &NSegment[0], &NSegNum,
                             Pto, Angle);
      AlgCom_SmoothSegMent(slot_data_at_right_side, 500, 10, &NSegment[0],
                           &NSegNum);
      if (TRUE == APAMap_FusTwoLineSegments(
                      slot_data_at_right_side, TempAng, &pData2[0], Data2Num,
                      &NewProperty2[0], &NSegment[0], NSegNum, &NSegProperty[0],
                      &pData2[0], &Data2Num, &pPtStyle[0])) {
        // updata obj2 bordline;
        for (k = 0; k < Data2Num; k++) {
          NewProperty2[k] = pPtStyle[k];
        }
        char log_string[512];
        snprintf(log_string, sizeof(log_string), "==PDCFusionObj2Success==");
        TLOG_INFO << log_string;
      }
    }
  }
  if (SubSlotPDCPtNum > 1) {
    i = 0;
    while (i < SubSlotPDCPtNum) {
      // get fsd data with same id;
      CurID = pSubSlotPtID[i];
      Data[0].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i].x;
      Data[0].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i].y;
      for (k = 1; k < 100; k++) {
        if ((i + k) < SubSlotPDCPtNum) {
          if (pSubSlotPtID[i + k] != CurID) {
            break;
          } else {
            Data[k].x = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i + k].x;
            Data[k].y = (APA_DISTANCE_CAL_FLOAT_TYPE)pSubSlotPDCInfo[i + k].y;
          }
        } else {
          break;
        }
      }
      DataNum = k;
      i += DataNum;

      //----------------------------------
      // Get valid PDC data for fus Subbordline;
      NSegNum = 0;
      for (k = 0; k < DataNum; k++) {
        TempPt = Data[k];
        LocStyle = AlgCom_GetPointLocationAccordGivenVector(
            &SubLinXStrPt, &SubLinXEndPt, &TempPt, &fDis);
        if (LocStyle != 0) {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt, &SubLinYEndPt, &TempPt, &fDis);
          if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        } else {
          LocStyle = AlgCom_GetPointLocationAccordGivenVector(
              &SubLinYStrPt1, &SubLinYEndPt1, &TempPt, &fDis);
          if (((LocStyle != 1) && (slot_data_at_right_side == TRUE)) ||
              ((LocStyle != 0) && (slot_data_at_right_side == FALSE))) {
            NSegment[NSegNum] = TempPt;
            NSegNum++;
          }
        }
      }
      for (k = 0; k < NSegNum; k++) {
        NSegProperty[k] = APA_MAP_PT_PROPERTY_FSD_STR;
      }
      APAMap_ReOderSegmentPt(TRUE, !slot_data_at_right_side, &NSegment[0],
                             &NSegNum, Pto, Angle);
      AlgCom_SmoothSegMent(!slot_data_at_right_side, 500, 10, &NSegment[0],
                           &NSegNum);
      if (TRUE == APAMap_CheckIfObjWithinRectArea(0x00, &NSegment[0], NSegNum,
                                                  pRectPt, pRectLine)) {
        NSegNum = 0;
      }
      if ((bCheckSubLane == TRUE) && (NSegNum != 0)) {
        for (k = 0; k < Data4Num; k++) {
          pDataBk[k] = pData4[k];
        }
        DataNumBk = Data4Num;
      }
      bFusvalid = FALSE;
      if (TRUE == APAMap_FusTwoLineSegments(
                      !slot_data_at_right_side, TempAng1, &pData4[0], Data4Num,
                      &NewProperty4[0], &NSegment[0], NSegNum, &NSegProperty[0],
                      &pData4[0], &Data4Num, &pPtStyle[0])) {
        if (bCheckSubLane == TRUE) {
          if (TRUE == APAMap_CheckIfObjWithinRectArea(
                          0x00, &pData4[0], Data4Num, pRectPt, pRectLine)) {
            for (k = 0; k < DataNumBk; k++) {
              pData4[k] = pDataBk[k];
            }
            Data4Num = DataNumBk;
          } else {
            bFusvalid = TRUE;
          }
        } else {
          bFusvalid = TRUE;
        }
      }
      if (bFusvalid == TRUE) {
        // updata sublane;
        for (k = 0; k < Data4Num; k++) {
          NewProperty4[k] = pPtStyle[k];
        }
        if (Data4Num > 2) {
          TempPt1 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              pData4[0], 0, Angle, Pto);
          TempPt2 = AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(
              pData4[1], 0, Angle, Pto);
          if ((TempPt1.y >= TempPt2.y) &&
              (((TempPt1.x <= TempPt2.x) && (slot_data_at_right_side == TRUE)) ||
               ((TempPt1.x >= TempPt2.x) && (slot_data_at_right_side == FALSE)))) {
            TempPt1.x = TempPt2.x;
            pData4[0] = AlgCom_PointPosWithAngAndCenterPt(TempPt1, Angle, Pto);
            for (k = 1; k < Data4Num - 1; k++) {
              NewProperty4[k] = NewProperty4[k + 1];
              pData4[k] = pData4[k + 1];
            }
            Data4Num--;
          }
        }
        char log_string[512];
        snprintf(log_string, sizeof(log_string), "==PDCFusionSubLaneSuccess==");
        TLOG_INFO << log_string;
      }
    }
  }
  DataNum = Data1Num + Data3Num + Data2Num;
  if (DataNum <= BOUNDARY_PT_MAX_NUM) {
    for (Index = 0; Index < DataNum; Index++) {
      if (Index < Data1Num) {
        pMapMainSlotBord->Points[Index] = pData1[Index];
        pMapMainSlotBord->Property[Index] = NewProperty1[Index];
      } else if (Index < Data1Num + Data3Num) {
        pMapMainSlotBord->Points[Index] = pData3[Index - Data1Num];
        pMapMainSlotBord->Property[Index] = NewProperty3[Index - Data1Num];

      } else {
        pMapMainSlotBord->Points[Index] = pData2[Index - Data1Num - Data3Num];
        pMapMainSlotBord->Property[Index] =
            NewProperty2[Index - Data1Num - Data3Num];
      }
    }
    pMapMainSlotBord->PtNum = DataNum;
    APAMap_GInfo.SlotPar.Obj1PtIndex = Data1Num - 1;
    APAMap_GInfo.SlotPar.SlotStrIndex =
        APAMap_GInfo.SlotPar.Obj1PtIndex + OffsetIndex1;
    APAMap_GInfo.SlotPar.Obj2PtIndex = Data1Num + Data3Num;
    APAMap_GInfo.SlotPar.SlotEndIndex =
        APAMap_GInfo.SlotPar.Obj2PtIndex - OffsetIndex2;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==PDCFusionMainSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  if (Data4Num <= BOUNDARY_PT_MAX_NUM) {
    for (Index = 0; Index < Data4Num; Index++) {
      pMapSubSlotBord->Points[Index] = pData4[Index];
      pMapSubSlotBord->Property[Index] = NewProperty4[Index];
    }
    pMapSubSlotBord->PtNum = Data4Num;
  } else {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==PDCFusionSubSlotFail For Buffer Not enough==");
    TLOG_INFO << log_string;
  }
  {
    char log_string[512];
    snprintf(log_string, sizeof(log_string),
             "==FusBordByPDC==SlotIndex(%d,%d,%d,%d)==Offset(%d,%d))",
             APAMap_GInfo.SlotPar.Obj1PtIndex,
             APAMap_GInfo.SlotPar.SlotStrIndex,
             APAMap_GInfo.SlotPar.SlotEndIndex,
             APAMap_GInfo.SlotPar.Obj2PtIndex, OffsetIndex1, OffsetIndex2);
    TLOG_INFO << log_string;
  }
  return TRUE;
}

#endif
#endif
