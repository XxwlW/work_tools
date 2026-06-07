void QWSOutputBar::LogReplayOnPaint(QPainter *pPainter)
{
	pPainter->setBackground(Qt::transparent);
	QRect rect = this->rect();
	QRect ClientRect = this->rect();
	QRect showRect;
	showRect.setCoords(0, 0, ClientRect.right(), ClientRect.bottom());

	QPainter MemDC;
	QImage img(ClientRect.width(), ClientRect.height(), QImage::Format_ARGB32);
	MemDC.begin(&img);
	MemDC.fillRect(img.rect(), QColor(192, 192, 192));

	APACarParameterInfoType *pCarPar = &APACarPar;

	//20191009 bqp
	if (bLoadFinish == TRUE)
	{
		pCarPar->APAState = LogData.LogDataBuffer[iBufferTemp].APAStatus;
		APACarPar.APARightSlotDataIndex = 0;//hzc 20210426
		APACarPar.APALeftSlotDataIndex = 1;//hzc 20210426
	}
	else
	{
	}

	//APASlotInfoDataType *pSlot = &pCarPar->Slot;
	APASlotOutlineCoordinateDataType *pMainSlot = &pCarPar->Slot[APACarPar.APARightSlotDataIndex].SlotOutline.Lane; //modified by DSH 2012.2.15
	APASlotOutlineCoordinateDataType *pMainSlotDetectedByRearSideSns = &pCarPar->Slot[APACarPar.APARightSlotDataIndex].SlotOutlineRearSideSnsDetected.Lane;
	APASlotOutlineCoordinateDataType *pSubSlot = &pCarPar->Slot[APACarPar.APALeftSlotDataIndex].SlotOutline.Lane;  //modified by DSH 2012.2.15
	APASlotOutlineCoordinateDataType *pSubSlotDetectedByRearSideSns = &pCarPar->Slot[APACarPar.APALeftSlotDataIndex].SlotOutlineRearSideSnsDetected.Lane;
#if 0
	APASlotOutlineCoordinateDataType *pRightCurb = &pCarPar->Curb[0].CurbBorderline.Curb; //Right
	APASlotOutlineCoordinateDataType *pLeftCurb = &pCarPar->Curb[1].CurbBorderline.Curb;  //Left
#endif
	if (pCarPar->APAState >= 4) {
		m_Scale = (double)ClientRect.height() * m_ScaleByRMouse * m_ScaleByReduce / 30000.0;
	}
	else {
		m_Scale = (double)ClientRect.height() * m_ScaleByRMouse * m_ScaleByReduce / 45000.0;
	}
	QPoint CenterPt(m_ClientCenterPoint.x(), m_ClientCenterPoint.y());
	//	QPoint CenterPtAPATest(m_ClientCenterPoint.x + 20000,m_ClientCenterPoint.y );

	int i;
	int nMax, nMin;
	nMax = 0;
	nMin = 0;
	/*pMainSlot->ObjPtCnt = APASlotDataWrIndex;
	for (i = 0; i < pMainSlot->ObjPtCnt; i++) {
		if (pMainSlot->CarCenterPoint[i].x > nMax)
			nMax = pMainSlot->CarCenterPoint[i].x;
		else if (pMainSlot->CarCenterPoint[i].x < nMin)
			nMin = pMainSlot->CarCenterPoint[i].x;
	}
	pSubSlot->ObjPtCnt = APASlotDataWrIndex;
	for (i = 0; i < pSubSlot->ObjPtCnt; i++) {
		if (pSubSlot->CarCenterPoint[i].x > nMax)
			nMax = pSubSlot->CarCenterPoint[i].x;
		else if (pSubSlot->CarCenterPoint[i].x < nMin)
			nMin = pSubSlot->CarCenterPoint[i].x;
	}*/
	//QPoint CenterPt((nMax - nMin) / 2 + 5500, 1000);
	if (pCarPar->APARunningState >= APA_RUNNING_STATE_PARKING_START_EPS_CONTROL_CONNECTING) {
		if (pCarPar->APACarCenterPt.Coordinate.y > 0) {
			//CenterPt.ry() = 10000;
		}
		else {
			CenterPt.ry() = 4000;
		}
	}
	else {
		nMax = (int)(pCarPar->APACarCenterPt.Coordinate.y / 10000);
		CenterPt.ry() += nMax * 10000;
	}
	//QPoint CenterPt(5000, 10000);
	// Car modle
	QLineInfo HLine(Qt::SolidLine, 1, QColor(0, 0, 255));
	QLineInfo VLine(Qt::SolidLine, 1, QColor(0, 0, 255));

	double SDGDemoCarScale = m_Scale;
#if 0
	RightSlot.setPointNum((pMainSlot->ObjPtCnt) * 2, false);
	CarCenter.setPointNum((pMainSlot->ObjPtCnt) * 2, false);
	int j;
	j = 0;
	for (i = 0; i < pMainSlot->ObjPtCnt; i++) {
		QPoint DstPt(pMainSlot->Point[i].Coordinate.x, pMainSlot->Point[i].Coordinate.y);

		CarCenter.pPointArr[j + 1] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
		if (j > 0) {
			CarCenter.pPointArr[j].x = CarCenter.pPointArr[j - 1].x;
			CarCenter.pPointArr[j].y = CarCenter.pPointArr[j + 1].y;
		}
		else {
			CarCenter.pPointArr[j].x = CarCenter.pPointArr[j + 1].x;
			CarCenter.pPointArr[j].y = CarCenter.pPointArr[j + 1].y;
		}

		APASlotCoordinateDataType MainSlotPt;
		APACoordinateDataType Pt1; // Just for VC demo debug
		UCHAR SnsIndex;
		SnsIndex = APACal.APASlotRAPASnsIndex;
		if (APACal.CarParkAtLeftOrRightSide == APA_CAR_PARK_AT_LEFT_SIDE) {
			SnsIndex = APACal.APASlotLAPASnsIndex;
		}
		APA_DISTANCE_TYPE SlotDepth = pMainSlot->Point[i].DisFromCarToObj;
		if (SlotDepth == NO_OBJ_DISTANCE) {
			SlotDepth = 4000;
		}
		MainSlotPt = APACalSlotRelativeToCarCoordinate(&pCarPar->APACal, SlotDepth, SnsIndex); // Just for VC demo

		Pt1.x = MainSlotPt.Coordinate.x;
		Pt1.y = MainSlotPt.Coordinate.y;
		Pt1 = APATrajCalPointPosWithAngAndCenterPt(Pt1,
			pMainSlot->CarAng[i],
			pMainSlot->Point[i].Coordinate);
		DstPt.x = Pt1.x;
		DstPt.y = Pt1.y;
		RightSlot.pPointArr[j + 1] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
		if (j > 0) {
			RightSlot.pPointArr[j].x = RightSlot.pPointArr[j - 1].x;
			RightSlot.pPointArr[j].y = RightSlot.pPointArr[j + 1].y;
		}
		else {
			RightSlot.pPointArr[j].x = RightSlot.pPointArr[j + 1].x;
			RightSlot.pPointArr[j].y = RightSlot.pPointArr[j + 1].y;
		}
		j += 2;
	}

	LeftSlot.setPointNum((pSubSlot->ObjPtCnt) * 2);
	j = 0;
	for (i = 0; i < pSubSlot->ObjPtCnt; i++) {
		QPoint DstPt(pSubSlot->Point[i].Coordinate.x, pSubSlot->Point[i].Coordinate.y);

		LeftSlot.pPointArr[j + 1] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
		if (j > 0) {
			LeftSlot.pPointArr[j].x = LeftSlot.pPointArr[j - 1].x;
			LeftSlot.pPointArr[j].y = LeftSlot.pPointArr[j + 1].y;
		}
		else {
			LeftSlot.pPointArr[j].x = LeftSlot.pPointArr[j + 1].x;
			LeftSlot.pPointArr[j].y = LeftSlot.pPointArr[j + 1].y;
		}
		j += 2;
	}
#else


	//if(SDGState == SDG_OPERATION_MODE)
	{
		//CenterPt.x = (int)((double)(rect.Width()) / m_Scale / 2.0 - 2000);
		//CenterPt.y = (int)(((double)rect.Height()) / m_Scale /2.0 - 10000);
		LeftSlot.setPointNum(SDGObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtCnt, false);
		for (i = 0; i < LeftSlot.getPointNum(); i++) {
			QPoint DstPt(SDGObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i].Pt.y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			/*if(SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE){
				if(i > 0){
					if(SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE){
						LeftSlot.pPointArr[i].x = LeftSlot.pPointArr[i - 1].x;
					} else {
						LeftSlot.pPointArr[i].x = LeftSlot.pPointArr[i - 1].x - 1000;
					}
					LeftSlot.pPointArr[i].y = DstPt.y;
				} else {
					LeftSlot.pPointArr[i] = DstPt;
					LeftSlot.pPointArr[i].x -= 1000;
				}
			} else*/ {
				LeftSlot.pPointArr[i] = DstPt;
			}
		}
		LeftSlot2.setPointNum(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtCnt, false);
		for (i = 0; i < LeftSlot2.getPointNum(); i++) {
			QPoint DstPt(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			/*if(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE){
				if(i > 0){
					if(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE){
						LeftSlot2.pPointArr[i].x = LeftSlot2.pPointArr[i - 1].x;
					} else {
						LeftSlot2.pPointArr[i].x = LeftSlot2.pPointArr[i - 1].x - 1000;
					}
					LeftSlot2.pPointArr[i].y = DstPt.y;
				} else {
					LeftSlot2.pPointArr[i] = DstPt;
					LeftSlot2.pPointArr[i].x -= 1000;
				}
			} else*/ {
				LeftSlot2.pPointArr[i] = DstPt;
			}
		}
		RightSlot.setPointNum(SDGObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtCnt, false);
		for (i = 0; i < RightSlot.getPointNum(); i++) {
			QPoint DstPt(SDGObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i].Pt.y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			/*if(SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE){
				if(i > 0){
					if(SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE){
						RightSlot.pPointArr[i].x = RightSlot.pPointArr[i - 1].x;
					} else {
						RightSlot.pPointArr[i].x = RightSlot.pPointArr[i - 1].x - 1000;
					}
					RightSlot.pPointArr[i].y = DstPt.y;
				} else {
					RightSlot.pPointArr[i] = DstPt;
					RightSlot.pPointArr[i].x -= 1000;
				}
			} else*/ {
				RightSlot.pPointArr[i] = DstPt;
			}
		}
		RightSlot2.setPointNum(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtCnt, false);
		for (i = 0; i < RightSlot2.getPointNum(); i++) {
			QPoint DstPt(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			/*if(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE){
				if(i > 0){
					if(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE){
						RightSlot2.pPointArr[i].x = RightSlot2.pPointArr[i - 1].x;
					} else {
						RightSlot2.pPointArr[i].x = RightSlot2.pPointArr[i - 1].x - 1000;
					}
					RightSlot2.pPointArr[i].y = DstPt.y;
				} else {
					RightSlot2.pPointArr[i] = DstPt;
					RightSlot2.pPointArr[i].x -= 1000;
				}
			} else */ {
				RightSlot2.pPointArr[i] = DstPt;
			}
		}
#if 0  
		//Curb start
		LeftCurb.setPointNum(pLeftCurb->ObjPtCnt, false);
		for (i = 0; i < LeftCurb.getPointNum(); i++) {
			QPoint DstPt(pLeftCurb->ObjPt[i].x,
				pLeftCurb->ObjPt[i].y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			LeftCurb.pPointArr[i] = DstPt;
		}

		RightCurb.setPointNum(pRightCurb->ObjPtCnt, false);
		for (i = 0; i < RightCurb.getPointNum(); i++) {
			QPoint DstPt(pRightCurb->ObjPt[i].x,
				pRightCurb->ObjPt[i].y);

			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			RightCurb.pPointArr[i] = DstPt;
		}
		//Curb end
#endif
		//draw FSD map grid start LLP 20210514 画在最底层，位置移到On Paint的前面
		tAutoDrvFSDMapBldMapDebugInfoType vMapDebugInfo;
		if (bDrawFSDGRID_APA == TRUE)
		{
			if (LogData.LogDataBuffer[iBufferTemp].IsASPCoordinate == FALSE)
			{
				pLogDataParse->AutoDrvFSDMapBld_GetMapInfoForDebugOutput_replay(&vMapDebugInfo);
				pLogDataParse->SetFSDMapGridPntLine_replay(CenterPt, &vMapDebugInfo);
				pLogDataParse->m_GridPntLine.MyDrawPointLine(&MemDC, m_Scale, &vMapDebugInfo);
				if (bDrawFSD1DGrid)
				{
					pLogDataParse->m_1DGridPntLine.MyDrawPointLine(&MemDC, m_Scale, &vMapDebugInfo);
				}
				if (bDrawFSD1DObj)
				{
					float FSD_1D_Obj_ColorR = 180;//紫色
					float FSD_1D_Obj_ColorG = 60;
					float FSD_1D_Obj_ColorB = 180;
					DrawFSD1DObj(&MemDC,
						(QPoint)CenterPt,
						(float)FSD_1D_Obj_ColorR,
						(float)FSD_1D_Obj_ColorG,
						(float)FSD_1D_Obj_ColorB);
				}

			}
		}

#if 1
		RightSlot.bDrawLinePoint = TRUE;
		RightSlot.DrawPointLine(&MemDC, m_Scale);
		LeftSlot.bDrawLinePoint = TRUE;
		LeftSlot.DrawPointLine(&MemDC, m_Scale);
		RightSlot2.bDrawLinePoint = TRUE;
		RightSlot2.DrawPointLine(&MemDC, m_Scale);
		LeftSlot2.bDrawLinePoint = TRUE;
		LeftSlot2.DrawPointLine(&MemDC, m_Scale);
#endif
		RightCurb.bDrawLinePoint = TRUE;
		RightCurb.DrawPointLine(&MemDC, m_Scale);
		LeftCurb.bDrawLinePoint = TRUE;
		LeftCurb.DrawPointLine(&MemDC, m_Scale);


#if 0
		// draw the SDG car2 屏蔽右边未用的小车
		if (bSDGDemoObjInfoLocked == FALSE) {
			bSDGDemoObjInfoLocked = TRUE;
			QPoint SDGDebugCarCenterPoint = CenterPt;
			SDGDebugCarCenterPoint.x += 10000;
			SDGLeftObjF.setPointNum(SDGDemoObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtCnt);
			for (i = 0; i < SDGLeftObjF.getPointNum(); i++) {
				QPoint DstPt(SDGDemoObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGDemoObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i].Pt.y);

				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, SDGDebugCarCenterPoint);
				if (SDGDemoObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE) {
					if (i > 0) {
						if (SDGDemoObjInfo.SnsObjPtBuf[SDG_FLS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE) {
							SDGLeftObjF.pPointArr[i].x = SDGLeftObjF.pPointArr[i - 1].x;
						}
						else {
							SDGLeftObjF.pPointArr[i].x = SDGLeftObjF.pPointArr[i - 1].x - 1000;
						}
						SDGLeftObjF.pPointArr[i].y = DstPt.y;
					}
					else {
						SDGLeftObjF.pPointArr[i] = DstPt;
						SDGLeftObjF.pPointArr[i].x -= 1000;
					}
				}
				else {
					SDGLeftObjF.pPointArr[i] = DstPt;
				}
			}
			SDGLeftObjR.setPointNum(SDGDemoObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtCnt);
			for (i = 0; i < SDGLeftObjR.getPointNum(); i++) {
				QPoint DstPt(SDGDemoObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGDemoObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.y);

				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, SDGDebugCarCenterPoint);
				if (SDGDemoObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE) {
					if (i > 0) {
						if (SDGDemoObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE) {
							SDGLeftObjR.pPointArr[i].x = SDGLeftObjR.pPointArr[i - 1].x;
						}
						else {
							SDGLeftObjR.pPointArr[i].x = SDGLeftObjR.pPointArr[i - 1].x - 1000;
						}
						SDGLeftObjR.pPointArr[i].y = DstPt.y;
					}
					else {
						SDGLeftObjR.pPointArr[i] = DstPt;
						SDGLeftObjR.pPointArr[i].x -= 1000;
					}
				}
				else {
					SDGLeftObjR.pPointArr[i] = DstPt;
				}
			}
			SDGRightObjF.setPointNum(SDGDemoObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtCnt);
			for (i = 0; i < SDGRightObjF.getPointNum(); i++) {
				QPoint DstPt(SDGDemoObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGDemoObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i].Pt.y);

				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, SDGDebugCarCenterPoint);
				if (SDGDemoObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE) {
					if (i > 0) {
						if (SDGDemoObjInfo.SnsObjPtBuf[SDG_FRS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE) {
							SDGRightObjF.pPointArr[i].x = SDGRightObjF.pPointArr[i - 1].x;
						}
						else {
							SDGRightObjF.pPointArr[i].x = SDGRightObjF.pPointArr[i - 1].x + 1000;
						}
						SDGRightObjF.pPointArr[i].y = DstPt.y;
					}
					else {
						SDGRightObjF.pPointArr[i] = DstPt;
						SDGRightObjF.pPointArr[i].x += 1000;
					}
				}
				else {
					SDGRightObjF.pPointArr[i] = DstPt;
				}
			}
			SDGRightObjR.setPointNum(SDGDemoObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtCnt);
			for (i = 0; i < SDGRightObjR.getPointNum(); i++) {
				QPoint DstPt(SDGDemoObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.x, SDGDemoObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.y);

				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, SDGDebugCarCenterPoint);
				if (SDGDemoObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].ObjDis == NO_OBJ_DISTANCE) {
					if (i > 0) {
						if (SDGDemoObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i - 1].ObjDis == NO_OBJ_DISTANCE) {
							SDGRightObjR.pPointArr[i].x = SDGRightObjR.pPointArr[i - 1].x;
						}
						else {
							SDGRightObjR.pPointArr[i].x = SDGRightObjR.pPointArr[i - 1].x + 1000;
						}
						SDGRightObjR.pPointArr[i].y = DstPt.y;
					}
					else {
						SDGRightObjR.pPointArr[i] = DstPt;
						SDGRightObjR.pPointArr[i].x += 1000;
					}
				}
				else {
					SDGRightObjR.pPointArr[i] = DstPt;
				}
			}
			int Y = 0 - APACal.LenBetweenRAxisAndRBumper;
			int X = 0 - APACal.HalfWidthOfCar;
			VLine.StartPt = QPoint((int)0, (int)Y);
			VLine.EndPt = QPoint((int)0, (int)(Y + APACal.LengthOfCar));
			HLine.StartPt = QPoint(X, (int)0);
			HLine.EndPt = QPoint((int)X + APACal.WidthOfCar, (int)0);

			SDGCar.setPointNum(5);
			SDGCar.pPointArr[0].x = (int)(HLine.StartPt.x);
			SDGCar.pPointArr[0].y = (int)(VLine.StartPt.y);
			SDGCar.pPointArr[1].x = (int)(HLine.EndPt.x);
			SDGCar.pPointArr[1].y = (int)(VLine.StartPt.y);
			SDGCar.pPointArr[2].x = (int)(HLine.EndPt.x);
			SDGCar.pPointArr[2].y = (int)(VLine.EndPt.y);
			SDGCar.pPointArr[3].x = (int)(HLine.StartPt.x);
			SDGCar.pPointArr[3].y = (int)(VLine.EndPt.y);

			SDGCar.pPointArr[4] = SDGCar.pPointArr[0];

			HLine.StartPt = CalRealWorldToScreenCoordinateTransition(HLine.StartPt, 0, SDGDebugCarCenterPoint);
			HLine.EndPt = CalRealWorldToScreenCoordinateTransition(HLine.EndPt, 0, SDGDebugCarCenterPoint);
			VLine.StartPt = CalRealWorldToScreenCoordinateTransition(VLine.StartPt, 0, SDGDebugCarCenterPoint);
			VLine.EndPt = CalRealWorldToScreenCoordinateTransition(VLine.EndPt, 0, SDGDebugCarCenterPoint);

			for (i = 0; i < 5; i++) {
				SDGCar.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(SDGCar.pPointArr[i], 0, SDGDebugCarCenterPoint);
			}
			VLine.DrawLine(pDC, SDGDemoCarScale);
			HLine.DrawLine(pDC, SDGDemoCarScale);
			bSDGDemoObjInfoLocked = FALSE;
		}

#endif
		// draw the SDG car2

			//} else {

				//SDGCar.setPointNum(0);

		RightSlot.setPointNum(pMainSlot->ObjPtCnt);
		CarCenterR.setPointNum(pMainSlot->ObjPtCnt);
		for (i = 0; i < pMainSlot->ObjPtCnt; i++) {
			QPoint DstPt;
			APACoordinateDataCalFloatType Pt1;
			APACoordinateDataCalFloatType CarCenterPt;
			APACoordinateDataType CarCenterPtTemp;
			APA_ANGLE_CAL_FLOAT_TYPE CarAng;

			/*      CarCenterPt.x = pMainSlot->CarCenterPoint[i].x;
				  CarCenterPt.y = pMainSlot->CarCenterPoint[i].y;
				  CarAng = pMainSlot->CarAng[i];

				  Pt1 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(pMainSlot->DisFromCarToObj[i], APACal.APASlotRAPASnsIndex);

				  Pt1 = APATrajCalPointPosWithAngAndCenterPt(Pt1,
						  CarAng,
						  CarCenterPt);
				  DstPt.x = Pt1.x;
				  DstPt.y = Pt1.y;
		  */

#if 0
			CarCenter.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			APASlotCoordinateDataType MainSlotPt;
			APACoordinateDataType Pt1; // Just for VC demo debug
			UCHAR SnsIndex;
			SnsIndex = APACal.APASlotRAPASnsIndex;
			if (APACal.CarParkAtLeftOrRightSide == APA_CAR_PARK_AT_LEFT_SIDE) {
				SnsIndex = APACal.APASlotLAPASnsIndex;
			}
			APA_DISTANCE_TYPE SlotDepth = pMainSlot->Point[i].DisFromCarToObj;
			if (SlotDepth == NO_OBJ_DISTANCE) {
				SlotDepth = 4000;
			}
			MainSlotPt = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(&pCarPar->APACal, SlotDepth, SnsIndex); // Just for VC demo

			Pt1.x = MainSlotPt.Coordinate.x;
			Pt1.y = MainSlotPt.Coordinate.y;
			Pt1 = APATrajCalPointPosWithAngAndCenterPt(Pt1,
				pMainSlot->CarAng[i],
				pMainSlot->Point[i].Coordinate);
			DstPt.x = Pt1.x;
			DstPt.y = Pt1.y;
			RightSlot.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
#endif

			DstPt = QPoint(pMainSlot->ObjPt[i].x, pMainSlot->ObjPt[i].y);
			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			if (pMainSlot->DisFromCarToObj[i] == NO_OBJ_DISTANCE) {

				APA_DISTANCE_TYPE SnsDis = 5000; // 5m.
				APA_INDEX_TYPE SnsIndex;

				//if(APACarPar.APACarParkAtLeftOrRightSide == FALSE){
				SnsIndex = APA_FRS_SNS_INDEX;
				//} else {
				//	SnsIndex = APA_FLS_SNS_INDEX;
				//}
				APACoordinateDataCalFloatType Pt2;

				Pt2 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(
					SnsDis, SnsIndex);

				CarCenterPt.x = pMainSlot->CarCenterPoint[i].x;
				CarCenterPt.y = pMainSlot->CarCenterPoint[i].y;

				Pt2 = APATrajCalPointPosWithAngAndCenterPt(Pt2,
					pMainSlot->CarAng[i],
					CarCenterPt);

				DstPt = QPoint(Pt2.x, Pt2.y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			else {
				DstPt = QPoint(pMainSlot->ObjPt[i].x, pMainSlot->ObjPt[i].y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			RightSlot.pPointArr[i] = DstPt;

			DstPt = QPoint(pMainSlot->CarCenterPoint[i].x, pMainSlot->CarCenterPoint[i].y);
			CarCenterR.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);

		}
		//
		RightSlot2.setPointNum(pMainSlotDetectedByRearSideSns->ObjPtCnt);
		//CarCenterR.setPointNum(pMainSlotDetectedByRearSideSns->ObjPtCnt);
		for (i = 0; i < pMainSlotDetectedByRearSideSns->ObjPtCnt; i++) {
			QPoint DstPt;
			APACoordinateDataCalFloatType Pt1;
			APACoordinateDataCalFloatType CarCenterPt;
			APACoordinateDataType CarCenterPtTemp;
			APA_ANGLE_CAL_FLOAT_TYPE CarAngRSns;

			DstPt = QPoint(pMainSlotDetectedByRearSideSns->ObjPt[i].x, pMainSlotDetectedByRearSideSns->ObjPt[i].y);
			DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			if (pMainSlotDetectedByRearSideSns->DisFromCarToObj[i] == NO_OBJ_DISTANCE) {

				APA_DISTANCE_TYPE SnsDis = 5000; // 5m.
				APA_INDEX_TYPE SnsIndex;

				//if(APACarPar.APACarParkAtLeftOrRightSide == FALSE){
				SnsIndex = APA_RRS_SNS_INDEX;
				//} else {
				//	SnsIndex = APA_FLS_SNS_INDEX;
				//}
				APACoordinateDataCalFloatType Pt2;

				Pt2 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(
					SnsDis, SnsIndex);

				CarCenterPt.x = pMainSlotDetectedByRearSideSns->CarCenterPoint[i].x;
				CarCenterPt.y = pMainSlotDetectedByRearSideSns->CarCenterPoint[i].y;

				Pt2 = APATrajCalPointPosWithAngAndCenterPt(Pt2,
					pMainSlotDetectedByRearSideSns->CarAng[i],
					CarCenterPt);

				DstPt = QPoint(Pt2.x, Pt2.y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			else {
				DstPt = QPoint(pMainSlotDetectedByRearSideSns->ObjPt[i].x, pMainSlotDetectedByRearSideSns->ObjPt[i].y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			RightSlot2.pPointArr[i] = DstPt;

			//DstPt = QPoint(pMainSlotDetectedByRearSideSns->CarCenterPoint[i].x, pMainSlotDetectedByRearSideSns->CarCenterPoint[i].y);
			//CarCenterR.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);

		}

		LeftSlot.setPointNum(pSubSlot->ObjPtCnt);
		//	CarCenterL.setPointNum(pSubSlot->ObjPtCnt);
		for (i = 0; i < pSubSlot->ObjPtCnt; i++) {
			QPoint DstPt;
			APACoordinateDataCalFloatType Pt1;
			APACoordinateDataCalFloatType CarCenterPt;
			APACoordinateDataType CarCenterPtTemp;
			APA_ANGLE_CAL_FLOAT_TYPE CarAng;

			/*CarCenterPt.x = pSubSlot->CarCenterPoint[i].x;
			CarCenterPt.y = pSubSlot->CarCenterPoint[i].y;
			CarAng = pSubSlot->CarAng[i];

			Pt1 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(pSubSlot->DisFromCarToObj[i], APACal.APASlotLAPASnsIndex);

			Pt1 = APATrajCalPointPosWithAngAndCenterPt (Pt1,
					CarAng,
					CarCenterPt);

			DstPt.x = Pt1.x;
			DstPt.y = Pt1.y;
	*/
			if (pSubSlot->DisFromCarToObj[i] == NO_OBJ_DISTANCE) {

				APA_DISTANCE_TYPE SnsDis = 5000; // 5m.
				APA_INDEX_TYPE SnsIndex;

				//if(APACarPar.APACarParkAtLeftOrRightSide == FALSE){
				//	SnsIndex = APA_FRS_SNS_INDEX;
				//} else {
				SnsIndex = APA_FLS_SNS_INDEX;
				//}
				APACoordinateDataCalFloatType Pt2;

				Pt2 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(
					SnsDis, SnsIndex);

				CarCenterPt.x = pSubSlot->CarCenterPoint[i].x;
				CarCenterPt.y = pSubSlot->CarCenterPoint[i].y;

				Pt2 = APATrajCalPointPosWithAngAndCenterPt(Pt2,
					pSubSlot->CarAng[i],
					CarCenterPt);

				DstPt = QPoint(Pt2.x, Pt2.y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			else {
				DstPt = QPoint(pSubSlot->ObjPt[i].x, pSubSlot->ObjPt[i].y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			LeftSlot.pPointArr[i] = DstPt;
			///		DstPt = QPoint(pSubSlot->CarCenterPoint[i].x, pSubSlot->CarCenterPoint[i].y);
			//		CarCenterL.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);

		}

		LeftSlot2.setPointNum(pSubSlotDetectedByRearSideSns->ObjPtCnt);
		for (i = 0; i < pSubSlotDetectedByRearSideSns->ObjPtCnt; i++) {
			QPoint DstPt;
			APACoordinateDataCalFloatType Pt1;
			APACoordinateDataCalFloatType CarCenterPt;
			APACoordinateDataType CarCenterPtTemp;
			APA_ANGLE_CAL_FLOAT_TYPE CarAngRSns;

			if (pSubSlotDetectedByRearSideSns->DisFromCarToObj[i] == NO_OBJ_DISTANCE) {

				APA_DISTANCE_TYPE SnsDis = 5000; // 5m.
				APA_INDEX_TYPE SnsIndex;

				SnsIndex = APA_RLS_SNS_INDEX;

				APACoordinateDataCalFloatType Pt2;

				Pt2 = APASlotProcCalSlotRelativeToCarCoordinateBySlotDisToCar(
					SnsDis, SnsIndex);

				CarCenterPt.x = pSubSlotDetectedByRearSideSns->CarCenterPoint[i].x;
				CarCenterPt.y = pSubSlotDetectedByRearSideSns->CarCenterPoint[i].y;

				Pt2 = APATrajCalPointPosWithAngAndCenterPt(Pt2,
					pSubSlotDetectedByRearSideSns->CarAng[i],
					CarCenterPt);

				DstPt = QPoint(Pt2.x, Pt2.y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			else {
				DstPt = QPoint(pSubSlotDetectedByRearSideSns->ObjPt[i].x, pSubSlotDetectedByRearSideSns->ObjPt[i].y);
				DstPt = CalRealWorldToScreenCoordinateTransition(DstPt, 0, CenterPt);
			}
			LeftSlot2.pPointArr[i] = DstPt;
		}

	}
#endif
	pCarPar->APACarCenterPt.Coordinate.x = LogData.LogDataBuffer[iBufferTemp].gCar_Current_Coordinate_X;
	pCarPar->APACarCenterPt.Coordinate.y = LogData.LogDataBuffer[iBufferTemp].gCar_Current_Coordinate_Y;
	pCarPar->APACarCenterPt.CarAng = LogData.LogDataBuffer[iBufferTemp].gCar_Angle;

	double X, Y;
	double X2, Y2;

	//建立坐标系
	X = pCarPar->APACarCenterPt.Coordinate.x;
	Y = pCarPar->APACarCenterPt.Coordinate.y;
	X += (double)APACal.LenBetweenRAxisAndRBumper * sin(pCarPar->APACarCenterPt.CarAng);
	Y -= (double)APACal.LenBetweenRAxisAndRBumper * cos(pCarPar->APACarCenterPt.CarAng);
	VLine.StartPt = QPoint((int)X, (int)Y);
	X2 = (double)APACal.WidthOfCar / 2.0 * cos(pCarPar->APACarCenterPt.CarAng);
	Y2 = (double)APACal.WidthOfCar / 2.0 * sin(pCarPar->APACarCenterPt.CarAng);

	HLine.StartPt.rx() = (int)(pCarPar->APACarCenterPt.Coordinate.x - X2);
	HLine.StartPt.ry() = (int)(pCarPar->APACarCenterPt.Coordinate.y - Y2);
	HLine.EndPt.rx() = (int)(pCarPar->APACarCenterPt.Coordinate.x + X2);
	HLine.EndPt.ry() = (int)(pCarPar->APACarCenterPt.Coordinate.y + Y2);
	HLine.StartPt = CalRealWorldToScreenCoordinateTransition(HLine.StartPt, 0, CenterPt);
	HLine.EndPt = CalRealWorldToScreenCoordinateTransition(HLine.EndPt, 0, CenterPt);
	Car.setPointNum(5);
	Car.pPointArr[0].rx() = (int)(X - X2);
	Car.pPointArr[0].ry() = (int)(Y - Y2);
	Car.pPointArr[1].rx() = (int)(X + X2);
	Car.pPointArr[1].ry() = (int)(Y + Y2);

	X = (double)APACal.LengthOfCar * sin(pCarPar->APACarCenterPt.CarAng);
	Y = (double)APACal.LengthOfCar * cos(pCarPar->APACarCenterPt.CarAng);
	VLine.EndPt.rx() = (int)(VLine.StartPt.x() - X);
	VLine.EndPt.ry() = (int)(VLine.StartPt.y() + Y);
	VLine.StartPt = CalRealWorldToScreenCoordinateTransition(VLine.StartPt, 0, CenterPt);
	VLine.EndPt = CalRealWorldToScreenCoordinateTransition(VLine.EndPt, 0, CenterPt);
	Car.pPointArr[2].rx() = Car.pPointArr[1].x() - (int)(X);
	Car.pPointArr[2].ry() = Car.pPointArr[1].y() + (int)(Y);

	Car.pPointArr[3].rx() = Car.pPointArr[2].x() - (int)(2 * X2);
	Car.pPointArr[3].ry() = Car.pPointArr[2].y() - (int)(2 * Y2);
	Car.pPointArr[4] = Car.pPointArr[0];
	for (i = 0; i < 5; i++) {
		Car.pPointArr[i] = CalRealWorldToScreenCoordinateTransition(Car.pPointArr[i], 0, CenterPt);
	}

	// Coordinate system.
	QLineInfo XLine(Qt::SolidLine, 1, QColor(0, 0, 0));
	QLineInfo YLine(Qt::SolidLine, 1, QColor(0, 0, 0)); //系统坐标

	QPoint PointTemp(-5000, 0);
	XLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
	PointTemp.rx() = 5000;
	XLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
	PointTemp.rx() = 0;
	PointTemp.ry() = 20000;
	YLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
	PointTemp.ry() = -20000;
	YLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

	MemDC.setBackground(Qt::transparent);
	Car.DrawPointLine(&MemDC, m_Scale);
	//SDGCar.DrawPointLine (&MemDC, SDGDemoCarScale);屏蔽右边未用的小车
	XLine.DrawLine(&MemDC, m_Scale);
	YLine.DrawLine(&MemDC, m_Scale);
	VLine.DrawLine(&MemDC, m_Scale);
	HLine.DrawLine(&MemDC, m_Scale);
	CarCenterR.DrawPointLine(&MemDC, m_Scale);

#if 1
	RightSlot.bDrawLinePoint = TRUE;
	RightSlot.DrawPointLine(&MemDC, m_Scale);
	LeftSlot.bDrawLinePoint = TRUE;
	LeftSlot.DrawPointLine(&MemDC, m_Scale);
#endif

	//RightSlot2.bDrawLinePoint = TRUE;
	//RightSlot2.DrawPointLine (pDC, m_Scale);
	//LeftSlot2.bDrawLinePoint = TRUE;
	//LeftSlot2.DrawPointLine (pDC, m_Scale);

	SDGLeftObjF.DrawPointLine(&MemDC, SDGDemoCarScale);
	SDGLeftObjR.DrawPointLine(&MemDC, SDGDemoCarScale);
	SDGRightObjF.DrawPointLine(&MemDC, SDGDemoCarScale);
	SDGRightObjR.DrawPointLine(&MemDC, SDGDemoCarScale);

	// The fit border line
	// Add 2012 08 13 Just for debug start
	if ((APACarPar.APACarParkingMode == APA_PARKPROC_PARKING_MODE_PERPENDICULAR)
		&& (m_bDrawBorderLineInSlot == TRUE)) {
		int i;
		QLineInfo LineRLS(Qt::SolidLine, 1, QColor(0, 0, 0));
		QLineInfo LinePtRLS(Qt::SolidLine, 5, QColor(255, 0, 0));
		for (i = 0; i < (SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtCnt - 1); i++) {
			//if(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i].Pt.y - SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);
			} */
			LineRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			//if(SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i + 1].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i + 1].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RLS_SNS_INDEX].ObjPtBuf[i + 1].Pt.y - SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);

			}*/
			LineRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LineRLS.DrawLine(&MemDC, m_Scale);
		}
		QLineInfo LineRL(Qt::SolidLine, 1, QColor(255, 255, 0));
		QLineInfo LinePtRL(Qt::SolidLine, 5, QColor(255, 0, 0));
		for (i = 0; i < (SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtCnt - 1); i++) {
			//if(SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].Pt.y;
			LinePtRL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRL.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i].Pt.y - SDGCal.SDGDtObjMinDis;
				LinePtRL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRL.DrawLine(pDC, m_Scale);
			} */
			LineRL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			//if(SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i + 1].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i + 1].Pt.y;
			LinePtRL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRL.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RL_SNS_INDEX].ObjPtBuf[i + 1].Pt.y - SDGCal.SDGDtObjMinDis;
				LinePtRL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRL.DrawLine(pDC, m_Scale);

			}*/
			LineRL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LineRL.DrawLine(&MemDC, m_Scale);
		}

		for (i = 0; i < (SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtCnt - 1); i++) {
			//if(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i].Pt.y + SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);
			}*/
			LineRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			//if(SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i + 1].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i + 1].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RRS_SNS_INDEX].ObjPtBuf[i + 1].Pt.y + SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);
			}*/
			LineRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LineRLS.DrawLine(&MemDC, m_Scale);

		}
		for (i = 0; i < (SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtCnt - 1); i++) {
			//if(SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i].Pt.y + SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);
			}*/
			LineRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			//if(SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i + 1].ObjDis != NO_OBJ_DISTANCE){
			PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
			PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i + 1].Pt.y;
			LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LinePtRLS.DrawLine(&MemDC, m_Scale);
			/*} else {
				PointTemp.rx() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i + 1].Pt.x;
				PointTemp.ry() = SDGObjInfo.SnsObjPtBuf[SDG_RR_SNS_INDEX].ObjPtBuf[i + 1].Pt.y + SDGCal.SDGDtObjMinDis;
				LinePtRLS.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
				LinePtRLS.DrawLine(pDC, m_Scale);
			}*/
			LineRLS.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LineRLS.DrawLine(&MemDC, m_Scale);

		}

	}
	// Add 2012 08 13 Just for debug end

	// Car turning radius and turning circle.
	if (pCarPar->APACurrentCarTurningRadius < APACal.APASupportMaxCarTurningRadius) {
		QLineInfo Line(Qt::SolidLine, 1, QColor(0, 255, 0));
		QArcInfo ArcR1(Qt::SolidLine, 1, QColor(0, 255, 0));
		double SteeringAng = pCarPar->APASteeringWheelAngle;
		if (SteeringAng < 0) {
			// Turning right.
			X2 = pCarPar->APACarCenterPt.Coordinate.x + pCarPar->APACurrentCarTurningRadius * MATH_COS(pCarPar->APACarCenterPt.CarAng);
			Y2 = pCarPar->APACarCenterPt.Coordinate.y + pCarPar->APACurrentCarTurningRadius * MATH_SIN(pCarPar->APACarCenterPt.CarAng);
		}
		else {
			// Turning left.
			X2 = pCarPar->APACarCenterPt.Coordinate.x - pCarPar->APACurrentCarTurningRadius * MATH_COS(pCarPar->APACarCenterPt.CarAng);
			Y2 = pCarPar->APACarCenterPt.Coordinate.y - pCarPar->APACurrentCarTurningRadius * MATH_SIN(pCarPar->APACarCenterPt.CarAng);
		}

		PointTemp = QPoint((int)X2, (int)Y2);
		Line.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		ArcR1.CenterPt = Line.StartPt;
		PointTemp = QPoint((int)pCarPar->APACarCenterPt.Coordinate.x, (int)pCarPar->APACarCenterPt.Coordinate.y);
		Line.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		Line.DrawLine(&MemDC, m_Scale);

		ArcR1.StartAngle = 0;
		ArcR1.SweepAngle = 2 * PI;
		ArcR1.Radius = pCarPar->APACurrentCarTurningRadius;
		ArcR1.CalStartPtAndEndPt();
		ArcR1.DrawArc(&MemDC, m_Scale);
	}

	// Car trajectory
	//if(pCarPar->APARunningState >= APA_RUNNING_STATE_SLOT_FOUND)
	{
		int i, j;
		BOOLEAN bFlag;

		// car end position
		QLineInfo Line(Qt::SolidLine, 1, QColor(0, 255, 255));
		PointTemp.rx() = pCarPar->TrajCalCarEndPos.Coordinate.x;
		PointTemp.ry() = 0;
		Line.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		PointTemp.ry() = -pCarPar->Slot[APACarPar.APARightSlotDataIndex].SlotPar[0].SlotLength;
		Line.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 3)//3:Enable 只在开启APA泊车功能后划线
		{
			Line.DrawLine(&MemDC, m_Scale);
		}

		// slot border line
		bFlag = FALSE;

		if (APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].EndPtIndex != 0) {
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LinePar.K = 0.617981851;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LinePar.B = 194.263931;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LineAng = 0.553536654;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].StartPtIndex = 21;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].EndPtIndex = 6;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LineLen = 1581;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].StartPt.x = 3417;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].StartPt.y = 2292;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].EndPt.x = 2072;
			// APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].EndPt.y = 1460;
			// In slot slot, Slot border line calcualted by RRS sns detected obj info.
			bFlag = TRUE;
			QLineInfo BorderLine(Qt::SolidLine, 3, QColor(0, 125, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LinePar.B;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			double dbTemp = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[0].LinePar.K * 4800.0
				+ PointTemp.y();
			PointTemp.ry() = dbTemp;

			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 4)//4:Active 只在泊车阶段显示
			{
				// BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}
		if (APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[1].EndPtIndex != 0) {
			// In slot slot, Slot border line calcualted by RR sns detected obj info.
			bFlag = TRUE;
			QLineInfo BorderLine(Qt::SolidLine, 3, QColor(125, 0, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[1].LinePar.B;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			double dbTemp = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[1].LinePar.K * 4800.0
				+ PointTemp.y();
			PointTemp.ry() = dbTemp;

			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 4)//4:Active 只在泊车阶段显示
			{
				// BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}
		if (bFlag == FALSE) {
			QLineInfo BorderLine(Qt::SolidLine, 1, QColor(0, 0, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APACarPar.TrajCalObj2Pos.y;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 3)//3:Enable
			{
				BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}//初始化时不划线

		bFlag = FALSE;

		if (APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].EndPtIndex != 0) {
			
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].EndPtIndex = 6;
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].StartPtIndex = 15;
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LineLen = 1129;
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LineAng = 0.510852933;
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LinePar.K = 0.560479045;
			APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LinePar.B = -3412.06812;
			// In slot slot, Slot border line calcualted by RLS sns detected obj info.
			bFlag = TRUE;
			QLineInfo BorderLine(Qt::SolidLine, 3, QColor(125, 125, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LinePar.B;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			double dbTemp = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[2].LinePar.K * 4800.0
				+ PointTemp.y();
			PointTemp.ry() = dbTemp;

			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 4)//4:Active 只在泊车阶段显示
			{
				BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}
		if (APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[3].EndPtIndex != 0) {
			// In slot slot, Slot border line calcualted by RL sns detected obj info.
			bFlag = TRUE;
			QLineInfo BorderLine(Qt::SolidLine, 3, QColor(125, 255, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[3].LinePar.B;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			double dbTemp = APAParkProcTargetParkingSlotInfo.BorderLineInfo.LineBuf[3].LinePar.K * 4800.0
				+ PointTemp.y();
			PointTemp.ry() = dbTemp;

			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 4)//4:Active 只在泊车阶段显示
			{
				// BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}
		if (bFlag == FALSE) {
			QLineInfo BorderLine(Qt::SolidLine, 1, QColor(0, 0, 125));
			PointTemp.rx() = 0;
			PointTemp.ry() = APACarPar.TrajCalObj1Pos.y;

			BorderLine.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

			PointTemp.rx() = 4800.0;
			BorderLine.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			if (pCarPar->APAState >= 3)//3:Enable
			{
				BorderLine.DrawLine(&MemDC, m_Scale);
			}
		}//初始化时不划线
		PointTemp.rx() = 0;
		Line.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

		if (pCarPar->Slot[APACarPar.APARightSlotDataIndex].SlotPar[0].SlotDepth != NO_OBJ_DISTANCE) {
			PointTemp.rx() = pCarPar->Slot[APACarPar.APARightSlotDataIndex].SlotPar[0].SlotDepth;
		}
		else {
			PointTemp.rx() = 4800; //4m 4800改为0，初始时不划青色X方向线
		}
		Line.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 3)//3:Enable
		{
			Line.DrawLine(&MemDC, m_Scale);
		}

		PointTemp.ry() = 0;
		Line.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 3)//3:Enable
		{
			Line.DrawLine(&MemDC, m_Scale);
		}

		// end pos line;
		QLineInfo Line1(Qt::SolidLine, 2, QColor(0, 255, 0));

		PointTemp.rx() = 0;
		PointTemp.ry() = pCarPar->TrajCalCarEndPosLine.C;

		Line1.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

		PointTemp.rx() = pCarPar->TrajCalCarEndPos.Coordinate.x;
		double dbTemp = pCarPar->TrajCalCarEndPosLine.A * pCarPar->TrajCalCarEndPos.Coordinate.x + pCarPar->TrajCalCarEndPosLine.C;
		PointTemp.ry() = dbTemp;

		Line1.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
		{
			Line1.DrawLine(&MemDC, m_Scale);
		}

		// end pos line   hzc 20190311 新增一条end position line
		QLineInfo Line_US(Qt::SolidLine, 2, QColor(255, 0, 0));

		PointTemp.rx() = 0;
		PointTemp.ry() = LogData.LogDataBuffer[iBufferTemp].m_APASlotCrtEndPositionLineC_US;

		Line_US.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

		PointTemp.rx() = pCarPar->TrajCalCarEndPos.Coordinate.x;
		double dbTemp_US = LogData.LogDataBuffer[iBufferTemp].m_APASlotCrtEndPositionLineK_US * pCarPar->TrajCalCarEndPos.Coordinate.x + LogData.LogDataBuffer[iBufferTemp].m_APASlotCrtEndPositionLineC_US;
		PointTemp.ry() = dbTemp_US;

		Line_US.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
		{
			Line_US.DrawLine(&MemDC, m_Scale);
		}

		// end pos line   hzc 20190618 新增一条end position line
		QLineInfo Line_VPL(Qt::SolidLine, 2, QColor(255, 163, 70));

		PointTemp.rx() = 0;
		PointTemp.ry() = LogData.LogDataBuffer[iBufferTemp].m_APAVplTrajCalCarEndPositionLineC;

		Line_VPL.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

		PointTemp.rx() = pCarPar->TrajCalCarEndPos.Coordinate.x;
		double dbTemp_VPL = LogData.LogDataBuffer[iBufferTemp].m_APAVplTrajCalCarEndPositionLineK * pCarPar->TrajCalCarEndPos.Coordinate.x + LogData.LogDataBuffer[iBufferTemp].m_APAVplTrajCalCarEndPositionLineC;
		PointTemp.ry() = dbTemp_VPL;

		Line_VPL.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
		{
			Line_VPL.DrawLine(&MemDC, m_Scale);
		}

		// end pos point;
		QLineInfo Line2(Qt::SolidLine, 8, QColor(0, 255, 0));
		PointTemp.rx() = pCarPar->TrajCalCarEndPos.Coordinate.x;
		PointTemp.ry() = pCarPar->TrajCalCarEndPos.Coordinate.y;

		Line2.StartPt = Line2.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
		{
			Line2.DrawLine(&MemDC, m_Scale);
		}

		// Draw the points of Obj1 and Obj2
		if (pCarPar->APAState >= 3)//3:Enable只在开始进入APA泊车功能时划点
		{
			QLineInfo Line3(Qt::SolidLine, 8, QColor(255, 0, 0));
			PointTemp.rx() = APACarPar.TrajCalObj1Pos.x;
			PointTemp.ry() = APACarPar.TrajCalObj1Pos.y;
			Line3.StartPt = Line3.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			Line3.DrawLine(&MemDC, m_Scale);

			QLineInfo Line4(Qt::SolidLine, 8, QColor(0, 0, 255));
			PointTemp.rx() = APACarPar.TrajCalObj2Pos.x;
			PointTemp.ry() = APACarPar.TrajCalObj2Pos.y;
			Line4.StartPt = Line4.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			Line4.DrawLine(&MemDC, m_Scale);
		}

		// trajectory
		if ((bLoadFinish == TRUE)
			&& ((bDisplayTrajectory == TRUE)
				|| (bDisplayTrajectory2 == TRUE)))//hzc 20190404
		{
			pCarPar->TrajCalCarParkingStepsNum = 15;
			if (bDisplayTrajectory2 == TRUE)
			{
				m_OriginalCarPos = pCarPar->APACarCenterPt;
			}
		}
		else
		{
			//pCarPar->TrajCalCarParkingStepsNum = 0;
		}

		for (j = 0; j < (pCarPar->TrajCalCarParkingStepsNum); j++) {
			if (j < pCarPar->TrajCalCarParkingStepsNum) {
				i = j;
			}
			else {
				{
					break;
				}
			}
			if ((pCarPar->TrajCalCarParkingStepDataArray[i].CarRearAxisCenterTurningRadius) == NO_OBJ_DISTANCE)
			{
				bFlag = TRUE;
				if (i > 0) {
					PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i - 1].CarPos.Coordinate.x;
					PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i - 1].CarPos.Coordinate.y;
				}
				else if (pCarPar->TrajCalCarParkingCurrentStepIndex == 0) {
					if (bDisplayTrajectory2 == TRUE)
					{
						PointTemp.rx() = m_OriginalCarPos.Coordinate.x;
						PointTemp.ry() = m_OriginalCarPos.Coordinate.y;
					}
					else
					{
						PointTemp.rx() = pCarPar->APACarCenterPt.Coordinate.x;
						PointTemp.ry() = pCarPar->APACarCenterPt.Coordinate.y;
					}
				}
				else {
					bFlag = FALSE;
					/*
					PointTemp.rx() = pMainSlot->CarCenterPoint[pMainSlot->ObjPtCnt - 1].x;
					PointTemp.ry() = pMainSlot->CarCenterPoint[pMainSlot->ObjPtCnt - 1].y;*/
				}
				if (bFlag) {
					Line.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.x;
					PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.y;
					Line.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
					{
						Line.DrawLine(&MemDC, m_Scale);//画直线
					}
				}
#if 0
				if (pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningRadiusTemp1 != NO_OBJ_DISTANCE) {
					double K, C1, C2, D, X, Y;
					if (abs(Line.EndPt.x - Line.StartPt.x) > 100) {
						K = (Line.EndPt.y - Line.StartPt.y) / (Line.EndPt.x - Line.StartPt.x);
						C1 = PointTemp.y - K * PointTemp.x;

						D = pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningRadiusTemp1;
						D = D * sqrt(K * K + 1);
						C2 = D - C1;

						if (pCarPar->CarParkAtLeftOrRightSide == APA_CAR_PARK_AT_LEFT_SIDE) {
							if (C2 > C1) {
								C2 = -D - C1;
							}
						}
						else {
							if (C2 < C1) {
								C2 = -D - C1;
							}
						}

						X = pMainSlot->CarCenterPoint[pMainSlot->ObjPtCnt - 1].x;
						Y = K * X + C2;
						QLineInfo Line2(Qt::SolidLine, 1, QColor(255, 0, 255));
						Line2.StartPt.x = (int)X;
						Line2.StartPt.y = (int)Y;
						Line2.StartPt = CalRealWorldToScreenCoordinateTransition(Line2.StartPt, 0, CenterPt);
						X = PointTemp.x;
						Y = K * X + C2;
						Line2.EndPt.x = (int)X;
						Line2.EndPt.y = (int)Y;
						Line2.EndPt = CalRealWorldToScreenCoordinateTransition(Line2.EndPt, 0, CenterPt);
						Line2.DrawLine(pDC, m_Scale);
					}
					else {
						// Vertical
						if (pCarPar->CarParkAtLeftOrRightSide == APA_CAR_PARK_AT_LEFT_SIDE) {
							PointTemp.x += pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningRadiusTemp1;
						}
						else {
							PointTemp.x -= pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningRadiusTemp1;
						}
						QLineInfo Line2(Qt::SolidLine, 1, QColor(255, 0, 255));
						Line2.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
						PointTemp.ry() = pMainSlot->CarCenterPoint[pMainSlot->ObjPtCnt - 1].y;
						Line2.EndPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
						Line2.DrawLine(pDC, m_Scale);
					}
				}
#endif
			}
			else {
				if (bDisplayTrajectory2 == TRUE)
				{
					/*add by hzc 20181222 start*/
					float y0, y1, y2, x0, x1, x2, y3, x3, Carx, Cary, ArcR1_EndAngle;

					QArcInfo ArcR1(Qt::SolidLine, 2, QColor(255, 0, 255));
					QLineInfo LineKeyPoint1(Qt::SolidLine, 4, QColor(0, 0, 255));
					QLineInfo LineKeyPoint2(Qt::SolidLine, 4, QColor(0, 0, 255));

					/*圆心*/
					PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningCenterPt.x;
					PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningCenterPt.y;
					PointTemp = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					ArcR1.CenterPt = PointTemp;
					ArcR1.Radius = pCarPar->TrajCalCarParkingStepDataArray[i].CarRearAxisCenterTurningRadius;

					y0 = ArcR1.CenterPt.y();
					x0 = ArcR1.CenterPt.x();
					//y1 = y0;//0度	使用tan2(y,x)函数不需要使用y1,x1这两个变量
					//x1 = PointTemp.x + ArcR1.Radius;

					if (i > 0)
					{
						PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i - 1].CarPos.Coordinate.x;
						PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i - 1].CarPos.Coordinate.y;

					}
					else if (i == 0)//只有一个关键点时将车的坐标作为前一个点
					{
						PointTemp.rx() = m_OriginalCarPos.Coordinate.x;
						PointTemp.ry() = m_OriginalCarPos.Coordinate.y;
					}
					else
					{
						return;//不应该进
					}
					LineKeyPoint1.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					y2 = LineKeyPoint1.StartPt.y();//point[i-1]
					x2 = LineKeyPoint1.StartPt.x();

					/*EndPoint为 i 的点*/
					PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.x;
					PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.y;
					LineKeyPoint2.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);

					y3 = LineKeyPoint2.StartPt.y();//point[i]
					x3 = LineKeyPoint2.StartPt.x();

					ArcR1.StartAngle = -atan2((float)(y2 - y0), (float)(x2 - x0));//StartAngle to 0
					ArcR1_EndAngle = -atan2((float)(y3 - y0), (float)(x3 - x0));//EndAngle to 0
					ArcR1.SweepAngle = -((float)ArcR1.StartAngle - (float)ArcR1_EndAngle);
					if (fabs((float)ArcR1.SweepAngle) > (float)PI)
					{
						ArcR1.SweepAngle = (float)2 * PI - fabs((float)ArcR1.SweepAngle);
					}

					PointTemp.rx() = pCarPar->APACarCenterPt.Coordinate.x;
					PointTemp.ry() = pCarPar->APACarCenterPt.Coordinate.y;

					LineKeyPoint1.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					Cary = LineKeyPoint1.StartPt.y();//car point
					Carx = LineKeyPoint1.StartPt.x();

					ArcR1.CalStartPtAndEndPt();
					ArcR1.DrawArc(&MemDC, m_Scale);
					/*add by hzc 20181222 end*/
				}
				else
				{
					QArcInfo ArcR1(Qt::SolidLine, 1, QColor(255, 0, 255));

					PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningCenterPt.x;
					PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i].CarTurningCenterPt.y;
					PointTemp = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					ArcR1.CenterPt = PointTemp;
					ArcR1.StartAngle = 0;
					ArcR1.SweepAngle = 2 * PI;
					ArcR1.Radius = pCarPar->TrajCalCarParkingStepDataArray[i].CarRearAxisCenterTurningRadius;
					ArcR1.CalStartPtAndEndPt();
					if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
					{
						ArcR1.DrawArc(&MemDC, m_Scale);//画圆
					}
				}

			}
			QLineInfo LineKeyPoint(Qt::SolidLine, 6, QColor(0, 0, 255));
			PointTemp.rx() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.x;
			PointTemp.ry() = pCarPar->TrajCalCarParkingStepDataArray[i].CarPos.Coordinate.y;
			LineKeyPoint.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
			LineKeyPoint.EndPt = LineKeyPoint.StartPt;
			if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
			{
				LineKeyPoint.DrawLine(&MemDC, m_Scale);//画关键点
			}

		}
	}


#ifdef DEBUG_APA_PERPENDICULAR_SLOT_LENGTH_CALCULATE

	if (m_PerpendicularSlotRelateTextDisplay == TRUE)
	{
		OnDrawPerpendicularSlotRelatedLine(&MemDC, CenterPt, 0);
		OnDrawPerpendicularSlotRelatedLine(&MemDC, CenterPt, 1);
	}
	if (m_SlotPtSlopDisplay == TRUE)
	{
		OnDrawSlotPtSlopeValue(&MemDC, CenterPt);
	}

#endif

	word SlotDataSelectIndex;

	QPointLineInfo  SlotDataAnalystSlotNearestObjetData;
	QPointLineInfo  SlotDataAnalystSlotPtData[2];


	if (0)//hzc 20190321 txt形式
	{
		word SlotDataIndex;
		SlotDataIndex = 0x0001;

		if (m_FlagFoundSelectSlotData == TRUE) {
			//	if(SlotDataSelectIndex & SlotDataIndex){
			int PTX;
			int PTY;
			if (m_EndPositionPrevPt.x() != m_EndPositionPt.x()) {
				PTX = (m_StartPositionPt.x() - m_EndPositionPt.x()) / m_Scale;
				m_CarCenterPt[m_CurrentSelectOpenFileIndex].rx() -= PTX;
				m_EndPositionPrevPt.rx() = m_EndPositionPt.x();
				m_StartPositionPt.rx() = m_EndPositionPt.x();
			}

			if (m_EndPositionPrevPt.y() != m_EndPositionPt.y()) {
				PTY = (m_StartPositionPt.y() - m_EndPositionPt.y()) / m_Scale;
				m_CarCenterPt[m_CurrentSelectOpenFileIndex].ry() -= PTY;
				m_EndPositionPrevPt.ry() = m_EndPositionPt.y();
				m_StartPositionPt.ry() = m_EndPositionPt.y();
			}
			m_FlagSelectSlotDataIsRedraw = TRUE;

			//	}
		}
		for (char mm = 0; mm < 10; mm++) {
			if (SlotDataAnalystCurrentSelectIndex & SlotDataIndex) {
				OnDrawSlotObjectDisPtData(&MemDC, m_CarCenterPt[mm], mm, SlotDataAnalystLineColour);
				OnDrawSlotObjectDisDataNearestDisDataDotLine(&MemDC, m_CarCenterPt[mm], mm, SlotDataAnalystLineColour, &SlotDataAnalystSlotNearestObjetData);
				if (SlotDataDisplay == TRUE) {
					OnDrawSlotPtDataLine(&MemDC, m_CarCenterPt[mm], mm, &SlotDataAnalystSlotPtData[0]);
				}
				if (SlotStdWvDataDisplay == TRUE) {
					OnDrawStandardEchoWvWidthSlotPtDataLine(&MemDC, m_CarCenterPt[mm], mm, &SlotDataAnalystSlotPtData[0]);
				}
				if (SlotMaxWvDataDisplay == TRUE) {
					OnDrawMaxEchoWvWidthSlotPtDataLine(&MemDC, m_CarCenterPt[mm], mm, &SlotDataAnalystSlotPtData[0]);
				}
			}
			SlotDataIndex = SlotDataIndex << 1;
		}
		OnDrawSlotProcessBufPtDataLine(&MemDC, CenterPt);

		/*

		word SlotStdWvDataIndex;
		SlotStdWvDataIndex = 0x0001;
		for(char nn = 0; nn < 10; nn ++){
			if(SlotDataAnalystCurrentSelectIndex & SlotStdWvDataIndex){
				//OnDrawSlotObjectDisPtData(&MemDC, m_CarCenterPt[nn], nn, SlotDataAnalystLineColour);
				OnDrawSlotObjectDisDataNearestDisDataDotLine(&MemDC,  m_CarCenterPt[nn], nn, SlotDataAnalystLineColour,&SlotDataAnalystSlotNearestObjetData);
				if(SlotStdWvDataDisplay == TRUE){
					OnDrawStandardEchoWvWidthSlotPtDataLine(&MemDC,  m_CarCenterPt[nn], nn, &SlotDataAnalystSlotPtData[0]);
				}
			}
			SlotStdWvDataIndex = SlotStdWvDataIndex<<1;
		}
		OnDrawSlotProcessBufPtDataLine(&MemDC,CenterPt);

		word SlotMaxWvDataIndex;
		SlotMaxWvDataIndex = 0x0001;
		for(char nn = 0; nn < 10; nn ++){
			if(SlotDataAnalystCurrentSelectIndex & SlotMaxWvDataIndex){
				//OnDrawSlotObjectDisPtData(&MemDC, m_CarCenterPt[nn], nn, SlotDataAnalystLineColour);
				OnDrawSlotObjectDisDataNearestDisDataDotLine(&MemDC,  m_CarCenterPt[nn], nn, SlotDataAnalystLineColour,&SlotDataAnalystSlotNearestObjetData);
				if(SlotMaxWvDataDisplay == TRUE){
					OnDrawMaxEchoWvWidthSlotPtDataLine(&MemDC,  m_CarCenterPt[nn], nn, &SlotDataAnalystSlotPtData[0]);
				}
			}
			SlotMaxWvDataIndex = SlotMaxWvDataIndex<<1;
		}
		OnDrawSlotProcessBufPtDataLine(&MemDC,CenterPt);
		*/

		QRect Rect1;
		Rect1.setTop(m_CurMousePoint.y());
		Rect1.setBottom(m_CurMousePoint.y() + (300 / m_ScaleByRMouse / m_ScaleByReduce));
		Rect1.setLeft(m_CurMousePoint.x());
		Rect1.setRight(m_CurMousePoint.x() + (130 / m_ScaleByRMouse / m_ScaleByReduce));

		if (Rect1.right() >= ClientRect.right()) {
			Rect1.setLeft(ClientRect.right() - (130 / m_ScaleByRMouse / m_ScaleByReduce));
		}

		if (Rect1.bottom() >= ClientRect.bottom()) {
			Rect1.setTop(ClientRect.bottom() - (200 / m_ScaleByRMouse / m_ScaleByReduce));
			Rect1.setBottom(ClientRect.bottom() - (20 / m_ScaleByRMouse / m_ScaleByReduce));
		}

		QString SlotParameter;

		if (SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotNum > 0) {
			//	uchar SlotNumIndex;

			//	SlotNumIndex =  SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotNum - 1;
			/*	SlotParameter.Format("SlotLength: %d\nSlotDepth: %d\n",
					SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[SlotNumIndex].SlotLengthTotal,
					SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[SlotNumIndex].SlotDepth);
					//((double)(m_CurMousePoint.x)/ m_Scale / 1000),
					//((double)(m_CurMousePoint.y)/ m_Scale / 1000)); // 改成slot参数即可， 需要判定的是当前选中的是哪个数据
			*/
			QString StringTemp;
			int SlotIndex = 0;
			SlotParameter = "";
			StringTemp = QString("Slot 0:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[0].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[0].SlotDepth);
			SlotParameter += StringTemp;
			SlotParameter += "\n";

			StringTemp = QString("Slot 1:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[1].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[1].SlotDepth);
			SlotParameter += StringTemp;
			SlotParameter += "\n";

			StringTemp = QString("Slot 2:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[2].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][0].SlotPar[2].SlotDepth);
			SlotParameter += StringTemp;
			//SlotParameter += "\n";

#ifdef DEBUG_APA_PERPENDICULAR_SLOT_LENGTH_CALCULATE
			extern APA_DISTANCE_TYPE SlotCompDis[2][2];
			for (char k = 0; k < 2; k++)
			{
				SlotParameter += "\n";
				StringTemp.Format("PerPendicular Slot ComDis: %d\n", SlotCompDis[0][k]);
				SlotParameter += StringTemp;
			}
#endif


		}
		else if (SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotNum > 0) {
			/*
			uchar SlotNumIndex;
			SlotNumIndex =  SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotNum - 1;
			SlotParameter.Format("SlotLength: %d\n,SlotDepth: %d\n",
				SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[SlotNumIndex].SlotLengthTotal,
				SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[SlotNumIndex].SlotDepth);
			SlotParameter.Format("%.3f, %.3f",((double)(m_CurMousePoint.x)/ m_Scale / 1000),((double)(m_CurMousePoint.y)/ m_Scale / 1000)); // 改成slot参数即可， 需要判定的是当前选中的是哪个数据
		*/
			QString StringTemp;
			int SlotIndex = 0;
			SlotParameter = "";
			StringTemp = QString("Slot 0:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[0].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[0].SlotDepth);
			SlotParameter += StringTemp;
			SlotParameter += "\n";

			StringTemp = QString("Slot 1:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[1].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[1].SlotDepth);
			SlotParameter += StringTemp;
			SlotParameter += "\n";

			StringTemp = QString("Slot 2:\nSlotLength: %1\nSlotDepth: %2\n")
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[2].SlotLengthTotal)
				.arg(SlotParameterData[m_CurrentSelectOpenFileIndex][1].SlotPar[2].SlotDepth);
			SlotParameter += StringTemp;

#ifdef DEBUG_APA_PERPENDICULAR_SLOT_LENGTH_CALCULATE
			extern APA_DISTANCE_TYPE SlotCompDis[2][2];
			for (char k = 0; k < 2; k++)
			{
				SlotParameter += "\n";
				StringTemp.Format("PerPendicular Slot ComDis: %d\n", SlotCompDis[0][k]);
				SlotParameter += StringTemp;
			}

#endif
		}
		MemDC.drawText(Rect1, Qt::AlignLeft, SlotParameter);
#if 0  //because delete windows.h
		if (m_FlagSelectCursor == TRUE) {
			if (m_CurrentSelectXorYLine == 1) {
				m_XCursorLinePos[m_XCurLineIndex] = (int)((double)m_CurMousePoint.x() / m_Scale);
				::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			}
			else if (m_CurrentSelectXorYLine == 2) {
				m_YCursorLinePos[m_YCurLineIndex] = (int)((double)m_CurMousePoint.y() / m_Scale);
				::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
			}
		}
#endif
		for (char index = 0; index < 10; index++) {
			{
				if (m_XCursorLinePos[index] > (1.0 / m_Scale)) {
					if (m_ScalePrev != 0) {
						if (m_ClientCenterPointPrev.x() != CenterPt.x()) {
							int DeltaX;
							DeltaX = CenterPt.x() - m_ClientCenterPointPrev.x();//(int)((double)(CenterPt.x - m_ClientCenterPointPrev.x));//* m_Scale);
							m_XCursorLinePos[index] += DeltaX;
						}
						/*else if(m_ScalePrev != m_Scale){
							int DeltaX;
							DeltaX = m_XCursorLinePos[index] - (int)((double)CenterPt.x * m_ScalePrev);
							m_XCursorLinePos[index] = (int)((double)CenterPt.x * m_Scale + ((double)DeltaX) / m_ScalePrev * m_Scale);
						}*/
					}
					QLineInfo YLine(Qt::SolidLine, 1, QColor(255, 0, 0));
					QPoint PointTemp((m_XCursorLinePos[index]), 0);
					YLine.StartPt = PointTemp; // CalRealWorldToScreenCoordinateTransition(PointTemp, 0,CenterPt);
					PointTemp.ry() = 10000 / m_Scale;//PointTemp;//
					YLine.EndPt = PointTemp;// CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					YLine.DrawLine(&MemDC, m_Scale);
				}
				if (m_YCursorLinePos[index] > (1.0 / m_Scale))
				{
					if (m_ClientCenterPointPrev.y() != CenterPt.y()) {
						int DeltaY;
						DeltaY = CenterPt.y() - m_ClientCenterPointPrev.y();//(int)((double)(CenterPt.y - m_ClientCenterPointPrev.y) * m_Scale);
						m_YCursorLinePos[index] += DeltaY;
					}
					/*else if(m_ScalePrev != 0){
						if(m_ScalePrev != m_Scale){
							int DeltaY;
							DeltaY = m_YCursorLinePos[index] - (int)((double)CenterPt.y * m_ScalePrev);
							m_YCursorLinePos[index] = (int)((double)CenterPt.y * m_Scale + ((double)DeltaY) / m_ScalePrev * m_Scale);
						}
					}*/
					QLineInfo XLine(Qt::SolidLine, 1, QColor(255, 0, 0));
					QPoint PointTemp(0, m_YCursorLinePos[index]);
					XLine.StartPt = PointTemp; //CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					PointTemp.rx() = 200000 / m_Scale;
					XLine.EndPt = PointTemp;//CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
					XLine.DrawLine(&MemDC, m_Scale);
				}
			}
		}
		m_ScalePrev = m_Scale;
		m_ClientCenterPointPrev.rx() = CenterPt.x();
		m_ClientCenterPointPrev.ry() = CenterPt.y();

	}

	if (bLoadFinish == TRUE)//hzc 20190321
	{
		OnDrawSlotProcessBufPtDataLineByLogFile(&MemDC, CenterPt);//彩色点
		
		QRect Rect1;
		Rect1.setTop(m_CurMousePoint.y());
		Rect1.setBottom(m_CurMousePoint.y() + (300 / m_ScaleByRMouse / m_ScaleByReduce));
		Rect1.setLeft(m_CurMousePoint.x() + (20 / m_ScaleByRMouse / m_ScaleByReduce));
		Rect1.setRight(m_CurMousePoint.x() + (150 / m_ScaleByRMouse / m_ScaleByReduce));

		if (Rect1.right() >= ClientRect.right()) {
			Rect1.setLeft(ClientRect.right() - (130 / m_ScaleByRMouse / m_ScaleByReduce));
		}

		if (Rect1.bottom() >= ClientRect.bottom()) {
			Rect1.setTop(ClientRect.bottom() - (200 / m_ScaleByRMouse / m_ScaleByReduce));
			Rect1.setBottom(ClientRect.bottom() - (20 / m_ScaleByRMouse / m_ScaleByReduce));
		}

		QString Info, InfoTemp;

		/*DrawPoint*/
		uchar PointSize = 8;
		COLORREF ColorRGB, ColorRGB0, ColorRGB1, ColorRGB2;
		uchar RorF, j, k;
#if 0
		if (uDisplayAEBData[Cfar] == TRUE)
		{
			ColorRGB0 = CfarTriObj1nColor;
			ColorRGB1 = CfarTriObj2nColor;
			ColorRGB2 = CfarTriObj3nColor;
			RorF = LogRorF[iBufferTemp][Cfar];
			//DrawAEBCFarNonTriObjPoints(&MemDC,CenterPt,PointSize,ColorRGB,RorF);
			DrawAEBCFarTriObjPoints(&MemDC, CenterPt, PointSize, ColorRGB0, ColorRGB1, ColorRGB2, RorF);

			for (j = 0; j < 6; j++)
			{
				for (k = 0; k < 3; k++)
				{
					if ((ShowCfarInfoTextFlag[j] == TRUE)
						&& (ShowCfarObjInfoTextFlag[k] == TRUE))
					{
						switch (j)
						{
						case R_RM_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("R_RM_TRIANGLE_H_OBJ_INDEX[0]\r\n");
							break;
						case RM_LM_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("RM_LM_TRIANGLE_H_OBJ_INDEX[1]\r\n");
							break;
						case RM_R_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("RM_R_TRIANGLE_H_OBJ_INDEX[2]\r\n");
							break;
						case LM_RM_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("LM_RM_TRIANGLE_H_OBJ_INDEX[3]\r\n");
							break;
						case LM_L_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("LM_L_TRIANGLE_H_OBJ_INDEX[4]\r\n");
							break;
						case L_LM_TRIANGLE_H_OBJ_INDEX:
							InfoTemp = QString("L_LM_TRIANGLE_H_OBJ_INDEX[5]\r\n");
							break;
						default:
							break;
						}
						Info += InfoTemp;
						InfoTemp = QString("TriObjInfo[%1].DisInfo.x = %2\r\n").arg(k).arg(LogAutoDrvData.CFARTemp[iBufferTemp][RorF].TriObjArry[j].TriObjInfo[k].DisInfo.x);
						Info += InfoTemp;
						InfoTemp = QString("TriObjInfo[%1].DisInfo.y = %2\r\n").arg(k).arg(LogAutoDrvData.CFARTemp[iBufferTemp][RorF].TriObjArry[j].TriObjInfo[k].DisInfo.y);
						Info += InfoTemp;
						InfoTemp = QString("TriObjInfo[%1].cCfarCfdLvl = %2\r\n").arg(k).arg(LogAutoDrvData.CFARTemp[iBufferTemp][RorF].TriObjArry[j].TriObjInfo[k].cCfarCfdLvl);
						Info += InfoTemp;
						InfoTemp = QString("TriObjInfo[%1].wTimeStamp = %2\r\n").arg(k).arg(LogAutoDrvData.CFARTemp[iBufferTemp][RorF].TriObjArry[j].TriObjInfo[k].wTimeStamp);
						Info += InfoTemp;
						InfoTemp = QString("\r\n");
						Info += InfoTemp;
						switch (k)
						{
						case 0:
							MemDC.setPen(ColorRGB0);
							break;
						case 1:
							MemDC.setPen(ColorRGB1);
							break;
						case 2:
							MemDC.setPen(ColorRGB2);
							break;
						default:
							break;
						}
						MemDC.drawText(Rect1, Qt::AlignLeft, Info);
					}
				}
			}
		}
		if (uDisplayAEBData[TrackObj] == TRUE)
		{
			ColorRGB = TraceTriObjnColor;
			RorF = LogRorF[iBufferTemp][TrackObj];
			DrawAEBTraceTriObjPoints(&MemDC, CenterPt, PointSize, ColorRGB, RorF);
			//DrawAEBTraceNonTriObjPoints(&MemDC,CenterPt,PointSize,ColorRGB,RorF);
			for (j = 0; j < 20; j++)
			{
				if (ShowTraceInfoTextFlag[j] == TRUE)
				{
					InfoTemp = QString("Index = %1\r\n").arg(uTraceDisplayBuffer[j]);
					Info += InfoTemp;
					InfoTemp = QString("DisInfo.x = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].DisInfo.x);
					Info += InfoTemp;
					InfoTemp = QString("DisInfo.y = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].DisInfo.y);
					Info += InfoTemp;
					InfoTemp = QString("cArrayIndex = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].cArrayIndex);
					Info += InfoTemp;
					InfoTemp = QString("cTrckCfdLvl = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].cTrckCfdLvl);
					Info += InfoTemp;
					InfoTemp = QString("cUpdateTime = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].cUpdateTime);
					Info += InfoTemp;
					InfoTemp = QString("wTimeStamp = %1\r\n").arg(LogAutoDrvData.TRACKOBJTemp[iBufferTemp][RorF].TriDataBuff[0].ObjInfo[j].wTimeStamp);
					Info += InfoTemp;
					InfoTemp = QString("\r\n");
					Info += InfoTemp;
					MemDC.setPen(ColorRGB);
					MemDC.drawText(Rect1, Qt::AlignLeft, Info);
				}
			}
		}
		if (uDisplayAEBData[LaebRskObjRecFus] == TRUE)
		{
			ColorRGB = LaebRskObjRecFusTrinColor;
			RorF = LogRorF[iBufferTemp][LaebRskObjRecFus];
			DrawAEBLaebRskObjRecFusPoints(&MemDC, CenterPt, PointSize, ColorRGB, RorF);
			if (ShowLaebRskObjRecFusInfoTextFlag[0] == TRUE)
			{
				InfoTemp = QString("LAEBRskObjRecFusUsRskObj:\r\n");
				Info += InfoTemp;
				InfoTemp = QString("RorF = %1\r\n").arg(RorF);
				Info += InfoTemp;
				InfoTemp = QString("ConfidenceLevel = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObj[iBufferTemp][RorF].ConfidenceLevel);
				Info += InfoTemp;
				InfoTemp = QString("DisToCar = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObj[iBufferTemp][RorF].DisToCar);
				Info += InfoTemp;
				/*InfoTemp.Format("LogFuseType = %1\r\n").arg(LogData.LAEBRskObjRecFusUsRskObj[iBufferTemp][RorF].LogFuseType);
				Info += InfoTemp;*/
				InfoTemp = QString("index = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObj[iBufferTemp][RorF].index);
				Info += InfoTemp;

				MemDC.setPen(ColorRGB);
				MemDC.drawText(Rect1, Qt::AlignLeft, Info);
			}
			if (ShowLaebRskObjRecFusInfoTextFlag[1] == TRUE)
			{
				InfoTemp = QString("LAEBRskObjRecFusUsRskObjNonTri:\r\n");
				Info += InfoTemp;
				InfoTemp = QString("RorF = %1\r\n").arg(RorF);
				Info += InfoTemp;
				InfoTemp = QString("ConfidenceLevel = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObjNonTri[iBufferTemp][RorF].ConfidenceLevel);
				Info += InfoTemp;
				InfoTemp = QString("DisToCar = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObjNonTri[iBufferTemp][RorF].DisToCar);
				Info += InfoTemp;
				/*InfoTemp= QString("LogFuseType = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObjNonTri[iBufferTemp][RorF].LogFuseType);
				Info += InfoTemp;*/
				InfoTemp = QString("index = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusUsRskObjNonTri[iBufferTemp][RorF].index);
				Info += InfoTemp;

				MemDC.setPen(ColorRGB);
				MemDC.drawText(Rect1, Qt::AlignLeft, Info);
			}
			if (ShowLaebRskObjRecFusInfoTextFlag[2] == TRUE)
			{
				InfoTemp = QString("LAEBRskObjRecFusVisRskObj:\r\n");
				Info += InfoTemp;
				InfoTemp = QString("RorF = %1\r\n").arg(RorF);
				Info += InfoTemp;
				InfoTemp = QString("ConfidenceLevel = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusVisRskObj[iBufferTemp][RorF].ConfidenceLevel);
				Info += InfoTemp;
				InfoTemp = QString("DisToCar = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusVisRskObj[iBufferTemp][RorF].DisToCar);
				Info += InfoTemp;
				/*	InfoTemp.Format("LogFuseType = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusVisRskObj[iBufferTemp][RorF].LogFuseType);
					Info += InfoTemp;*/
				InfoTemp = QString("index = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusVisRskObj[iBufferTemp][RorF].index);
				Info += InfoTemp;

				MemDC.setPen(ColorRGB);
				MemDC.drawText(Rect1, Qt::AlignLeft, Info);
			}
			if (ShowLaebRskObjRecFusInfoTextFlag[3] == TRUE)
			{
				InfoTemp = QString("LAEBRskObjRecFusRskObj:\r\n");
				Info += InfoTemp;
				InfoTemp = QString("RorF = %1\r\n").arg(RorF);
				Info += InfoTemp;
				InfoTemp = QString("ConfidenceLevel = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusRskObj[iBufferTemp][RorF].ConfidenceLevel);
				Info += InfoTemp;
				InfoTemp = QString("DisToCar = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusRskObj[iBufferTemp][RorF].DisToCar);
				Info += InfoTemp;
				/*InfoTemp= QString("LogFuseType = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusRskObj[iBufferTemp][RorF].LogFuseType);
				Info += InfoTemp;*/
				InfoTemp = QString("index = %1\r\n").arg(LogAutoDrvData.LAEBRskObjRecFusRskObj[iBufferTemp][RorF].index);
				Info += InfoTemp;

				MemDC.setPen(ColorRGB);
				MemDC.drawText(Rect1, Qt::AlignLeft, Info);
			}
		}
		if (uDisplayAEBData[LaebObjFuse_VisTrackObj] == TRUE)
		{
			//raw
			ColorRGB = LadbObjFuseVisTraceObjnColor;
			DrawAEBLaebObjFuseVisTraceObjRawPoints(&MemDC, CenterPt, PointSize, ColorRGB);
			RorF = LogRorF[iBufferTemp][LaebObjFuse_VisTrackObj];
			for (i = 0; i < 10; i++)
			{
				if (ShowLaebObjFuseVisTrackObjInfoTextFlag[i] == TRUE)
				{
					InfoTemp = QString("LaebObjFuse_VisTrackObjRaw[%1]:\r\n").arg(i);
					Info += InfoTemp;
					InfoTemp = QString("RorF = %1\r\n").arg(RorF);
					Info += InfoTemp;
					InfoTemp = QString("cType = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].cType);
					Info += InfoTemp;
					InfoTemp = QString("cConfidenceLevel = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].cConfidenceLevel);
					Info += InfoTemp;
					InfoTemp = QString("ObjCntrPtAtWorld.x = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].ObjCntrPtAtWorld.x);
					Info += InfoTemp;
					InfoTemp = QString("ObjCntrPtAtWorld.y = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].ObjCntrPtAtWorld.y);
					Info += InfoTemp;
					InfoTemp = QString("RxObjCntrPt.x = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].RxObjCntrPt.x);
					Info += InfoTemp;
					InfoTemp = QString("RxObjCntrPt.y = %1\r\n").arg(LogAutoDrvData.VisRawDataTemp[iBufferTemp].ObjBuf[i].RxObjCntrPt.y);
					Info += InfoTemp;

					MemDC.setPen(ColorRGB);
					MemDC.drawText(Rect1, Qt::AlignLeft, Info);
				}
			}
			//track
			ColorRGB = LadbObjFuseVisTraceObjnColor2;
			DrawAEBLaebObjFuseVisTraceObjTrackPoints(&MemDC, CenterPt, PointSize, ColorRGB, RorF);
			for (i = 0; i < 5; i++)
			{
				if (ShowLaebObjFuseVisTrackObjTrackInfoTextFlag[i] == TRUE)
				{
					InfoTemp = QString("LaebObjFuse_VisTrackObjTrack[%1]:\r\n").arg(i);
					Info += InfoTemp;
					InfoTemp = QString("RorF = %1\r\n").arg(RorF);
					Info += InfoTemp;
					InfoTemp = QString("Obj[i].Pos.x = %1\r\n").arg(LogAutoDrvData.LAEBObjFuse_VisTrackObj[iBufferTemp][RorF].Obj[i].Pos.x);
					Info += InfoTemp;
					InfoTemp = QString("Obj[i].Pos.y = %1\r\n").arg(LogAutoDrvData.LAEBObjFuse_VisTrackObj[iBufferTemp][RorF].Obj[i].Pos.y);
					Info += InfoTemp;
					InfoTemp = QString("Obj[i].ConfidenceLevel = %1\r\n").arg(LogAutoDrvData.LAEBObjFuse_VisTrackObj[iBufferTemp][RorF].Obj[i].ConfidenceLevel);
					Info += InfoTemp;
					InfoTemp = QString("Obj[i].Type = %1\r\n").arg(LogAutoDrvData.LAEBObjFuse_VisTrackObj[iBufferTemp][RorF].Obj[i].Type);
					Info += InfoTemp;
					InfoTemp = QString("Obj[i].TimeStamp = %1\r\n").arg(LogAutoDrvData.LAEBObjFuse_VisTrackObj[iBufferTemp][RorF].Obj[i].TimeStamp);
					Info += InfoTemp;

					MemDC.setPen(ColorRGB);
					MemDC.drawText(Rect1, Qt::AlignLeft, Info);
				}
			}
		}
#endif
	}
	//add by hzc 20190104 Draw slot
	if (pCarPar->APAState < 4)//Active只在找车位阶段显示
	{
		/**********************************************************VPL start*/
		float VPL_ColorR = 255;//橙色
		float VPL_ColorG = 163;
		float VPL_ColorB = 70;
		DrawFusionSlot_VPL(
			&MemDC,
			(QPoint)PointTemp,
			(QPoint)CenterPt,
			(float)VPL_ColorR,
			(float)VPL_ColorG,
			(float)VPL_ColorB
		);
		/**********************************************************VPL end*/
		/**********************************************************USVPL start*/
		float USVPL_ColorR = 0;//绿色
		float USVPL_ColorG = 128;
		float USVPL_ColorB = 0;
		DrawFusionSlot_USVPL(
			&MemDC,
			(QPoint)PointTemp,
			(QPoint)CenterPt,
			(float)USVPL_ColorR,
			(float)USVPL_ColorG,
			(float)USVPL_ColorB
		);
		/**********************************************************USVPL end*/
		/**********************************************************OBJVPL start*/
		float OBJVPL_ColorR = 0;//黑色
		float OBJVPL_ColorG = 0;
		float OBJVPL_ColorB = 0;
		DrawFusionObj(
			&MemDC,
			(QPoint)PointTemp,
			(QPoint)CenterPt,
			(float)OBJVPL_ColorR,
			(float)OBJVPL_ColorG,
			(float)OBJVPL_ColorB
		);
		/**********************************************************OBJVPL end*/
	}
	if (bDisplaySensorData == TRUE) //菜单栏中的SNS按钮按下时才显示sensor data points
	{
		QLineInfo TestPoint01(Qt::SolidLine, 6, QColor(255, 128, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint01_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint01_Y * 10;//厘米为单位
		TestPoint01.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint01.EndPt = TestPoint01.StartPt;
		TestPoint01.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint02(Qt::SolidLine, 6, QColor(0, 255, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint02_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint02_Y * 10;//厘米为单位
		TestPoint02.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint02.EndPt = TestPoint02.StartPt;
		TestPoint02.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint03(Qt::SolidLine, 6, QColor(128, 255, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint03_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint03_Y * 10;//厘米为单位
		TestPoint03.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint03.EndPt = TestPoint03.StartPt;
		TestPoint03.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint04(Qt::SolidLine, 6, QColor(0, 128, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint04_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint04_Y * 10;//厘米为单位
		TestPoint04.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint04.EndPt = TestPoint04.StartPt;
		TestPoint04.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint05(Qt::SolidLine, 6, QColor(255, 0, 0));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint05_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint05_Y * 10;//厘米为单位
		TestPoint05.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint05.EndPt = TestPoint05.StartPt;
		TestPoint05.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint06(Qt::SolidLine, 6, QColor(255, 0, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint06_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint06_Y * 10;//厘米为单位
		TestPoint06.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint06.EndPt = TestPoint06.StartPt;
		TestPoint06.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint07(Qt::SolidLine, 6, QColor(0, 128, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint07_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint07_Y * 10;//厘米为单位
		TestPoint07.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint07.EndPt = TestPoint07.StartPt;
		TestPoint07.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint08(Qt::SolidLine, 6, QColor(0, 64, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint08_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint08_Y * 10;//厘米为单位
		TestPoint08.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint08.EndPt = TestPoint08.StartPt;
		TestPoint08.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint09(Qt::SolidLine, 6, QColor(0, 0, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint09_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint09_Y * 10;//厘米为单位
		TestPoint09.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint09.EndPt = TestPoint09.StartPt;
		TestPoint09.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint10(Qt::SolidLine, 6, QColor(128, 0, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint10_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint10_Y * 10;//厘米为单位
		TestPoint10.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint10.EndPt = TestPoint10.StartPt;
		TestPoint10.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint11(Qt::SolidLine, 6, QColor(128, 64, 0));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint11_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint11_Y * 10;//厘米为单位
		TestPoint11.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint11.EndPt = TestPoint11.StartPt;
		TestPoint11.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint12(Qt::SolidLine, 6, QColor(128, 128, 0));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint12_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint12_Y * 10;//厘米为单位
		TestPoint12.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint12.EndPt = TestPoint12.StartPt;
		TestPoint12.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint13(Qt::SolidLine, 8, QColor(255, 128, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint13_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint13_Y * 10;//厘米为单位
		TestPoint13.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint13.EndPt = TestPoint13.StartPt;
		TestPoint13.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint14(Qt::SolidLine, 8, QColor(0, 255, 128));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint14_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint14_Y * 10;//厘米为单位
		TestPoint14.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint14.EndPt = TestPoint14.StartPt;
		TestPoint14.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint15(Qt::SolidLine, 8, QColor(128, 255, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint15_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint15_Y * 10;//厘米为单位
		TestPoint15.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint15.EndPt = TestPoint15.StartPt;
		TestPoint15.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint16(Qt::SolidLine, 8, QColor(0, 128, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint16_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint16_Y * 10;//厘米为单位
		TestPoint16.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint16.EndPt = TestPoint16.StartPt;
		TestPoint16.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint17(Qt::SolidLine, 8, QColor(255, 0, 0));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint17_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint17_Y * 10;//厘米为单位
		TestPoint17.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint17.EndPt = TestPoint17.StartPt;
		TestPoint17.DrawLine(&MemDC, m_Scale);

		QLineInfo TestPoint18(Qt::SolidLine, 8, QColor(255, 0, 255));
		PointTemp.rx() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint18_X * 10;//厘米为单位
		PointTemp.ry() = (INT16)LogData.LogDataBuffer[iBufferTemp].TestPoint18_Y * 10;//厘米为单位
		TestPoint18.StartPt = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
		TestPoint18.EndPt = TestPoint18.StartPt;
		TestPoint18.DrawLine(&MemDC, m_Scale);
	}

	//hzc 20190618 OD
	if (pCarPar->APAState >= 4)//4:Active只在泊车阶段显示
	{
		DrawAPAVPLSoltCorrectSlotLine(&MemDC,
			(QPoint)PointTemp,
			(QPoint)CenterPt);
	}

	//LLP 20230309
	if (pCarPar->APAState == 4)//4:Active 只在active阶段显示
	{
		DrawOriginalSlot_SPI((QPoint)CenterPt);
		DrawLineList(&MemDC, &m_SPIOriginalSlotRectLineList, m_Scale);
	}

	//hzc 201900625  LLP 20210621 AB点OD检测需要去掉APAState判定
	DrawObjInfo(&MemDC, (QPoint)CenterPt);

	//llp 20231207
	DrawFusionPublishObjRect((QPoint)CenterPt);
	DrawLineList(&MemDC, &m_FusionPublishObjRectLineList, m_Scale);
	DrawLineList(&MemDC, &m_FusionPublishObjRectLineList2, m_Scale);

	DrawFSDObjPt(&MemDC, (QPoint)CenterPt);

	//LLP 20220114
	if (bDrawSPIOverlaySlot == TRUE)
	{
		DrawOverlaySlot_SPI((QPoint)CenterPt);
		DrawLineList(&MemDC, &m_SPIOverlaySlotRectLineList, m_Scale);
	}
	DrawStartTurnningSWA_KeyPt(&MemDC, (QPoint)CenterPt);

	//LLP 20210830
	DrawLimitter(&MemDC, (QPoint)CenterPt);

	//CLY 20221104
	DrawUSSideCorObjPt(&MemDC, pCarPar, CenterPt);

	float USVPL_ColorR = 0;//蓝色
	float USVPL_ColorG = 0;
	float USVPL_ColorB = 200;
	DrawVsPillar(&MemDC, pCarPar, (QPoint)CenterPt, (float)USVPL_ColorR, (float)USVPL_ColorG, (float)USVPL_ColorB);

	//LLP 20210923
	if (pCarPar->APAState == 3)//3:Enable 只在找车位阶段显示
	{
		DrawParkingableSlotPt(&MemDC, (QPoint)CenterPt);
	}

	//LLP 20211214
	if (bDrawSlotBoundary)
	{
		DrawAPASearchSlotBoundary(&MemDC, (QPoint)CenterPt);
	}

	/*********************************US Slot start**************************/
	float US_Left_ColorR = 170;//咖啡色
	float US_Left_ColorG = 110;
	float US_Left_ColorB = 80;
	if (pCarPar->APAState == 3)//3:Enable 只在找车位阶段显示
	{
		DrawFusionLeftSlot_US(
			&MemDC,
			(QPoint)CenterPt,
			(float)US_Left_ColorR,
			(float)US_Left_ColorG,
			(float)US_Left_ColorB
		);
	}
	float US_Right_ColorR = 10;//宝蓝色
	float US_Right_ColorG = 140;
	float US_Right_ColorB = 200;
	if (pCarPar->APAState == 3)//3:Enable 只在找车位阶段显示
	{
		DrawFusionRightSlot_US(
			&MemDC,
			(QPoint)CenterPt,
			(float)US_Right_ColorR,
			(float)US_Right_ColorG,
			(float)US_Right_ColorB
		);
	}
	/*********************************US Slot end**************************/
	/*********************************FSD Obj start**************************/
	float FSD_Left_Obj_ColorR = 180;//紫色
	float FSD_Left_Obj_ColorG = 60;
	float FSD_Left_Obj_ColorB = 180;
	DrawFSDLeftObjRect(
		&MemDC,
		(QPoint)CenterPt,
		(float)FSD_Left_Obj_ColorR,
		(float)FSD_Left_Obj_ColorG,
		(float)FSD_Left_Obj_ColorB
	);

	float FSD_Right_Obj_ColorR = 170;//绿色
	float FSD_Right_Obj_ColorG = 220;
	float FSD_Right_Obj_ColorB = 10;
	DrawFSDRightObjRect(
		&MemDC,
		(QPoint)CenterPt,
		(float)FSD_Right_Obj_ColorR,
		(float)FSD_Right_Obj_ColorG,
		(float)FSD_Right_Obj_ColorB
	);
	/*********************************FSD Obj end**************************/
	if (bDrawSlotPtInAPACoord == TRUE)
	{
		DrawSlotPtInAPACoord(&MemDC, (QPoint)CenterPt);
	}
	DrawDisplaySlot((QPoint)CenterPt);

	QString strTxt;
	strTxt = QString("StepIndex:    %1\nDis to Key Pt:    %2\nKey Pt Position:\nx = %3 mm    y = %4 mm")
		.arg((int)LogData.LogDataBuffer[iBufferTemp].StepIndex)
		.arg((int)LogData.LogDataBuffer[iBufferTemp].StartTurnKeyPtDis)
		.arg((int)LogData.LogDataBuffer[iBufferTemp].StartTurnKeyPt_X)
		.arg((int)LogData.LogDataBuffer[iBufferTemp].StartTurnKeyPt_Y);

	QTextInfo Text(strTxt, Qt::SolidLine, 1, QColor(0, 0, 255));
	Text.str = strTxt;
	PointTemp.rx() = 5000;
	PointTemp.ry() = -5000;
	PointTemp = CalRealWorldToScreenCoordinateTransition(PointTemp, 0, CenterPt);
	Text.TxtRect.setCoords(PointTemp.x(), PointTemp.y(), PointTemp.x() + 16000, PointTemp.y() + 9000);
	Text.DrawTextInfo(&MemDC, m_Scale);

	//绘图完成后的清理
	SlotDataAnalystSlotNearestObjetData.DeleteAllData();
	SlotDataAnalystSlotPtData[0].DeleteAllData();
	SlotDataAnalystSlotPtData[1].DeleteAllData();

	MemDC.end();
	pPainter->drawImage(rect, img);
}
