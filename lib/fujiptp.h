// Extra PTP & API defs for Fuifilm
#ifndef FUJIPTP_H
#define FUJIPTP_H

#define FUJI_CMD_IP_PORT 55740
#define FUJI_EVENT_IP_PORT 55741
#define FUJI_LIVEVIEW_IP_PORT 55742

// Fuji USB and IP extensions
#define PTP_OC_FUJI_SendObjectInfo	0x900c // create file
#define PTP_OC_FUJI_SendObject2		0x900d // Appears to be the same as 901d
#define PTP_OC_FUJI_SendObject		0x901d // write to file

// WiFi only codes
#define PTP_PC_FUJI_EventsList		0xd212
#define PTP_PC_FUJI_SelectedImgsMode	0xd220
#define PTP_PC_FUJI_ObjectCount		0xd222
#define PTP_PC_FUJI_CameraState		0xdf00 // formerly Unlocked 
#define PTP_PC_FUJI_FunctionMode	0xdf01 // formerly Mode - should be (CameraReportedState??)
#define PTP_PC_FUJI_CompressSmall	0xD226 // compress into 400-800kb
#define PTP_PC_FUJI_NoCompression	0xD227 // Enable full image download

// Fuji Camera Connect has this version
#define FUJI_CAM_CONNECT_REMOTE_VER 0x2000B

// Downloader opcodes, mostly unknown
#define PTP_OC_FUJI_Unknown1	0x9054
#define PTP_OC_FUJI_Unknown2	0x9055

// Property codes, others are in camlib ptp.h
#define PTP_PC_FUJI_Unknown4		0xD228
#define PTP_PC_FUJI_Unknown15		0xD22B
#define PTP_PC_FUJI_CompressionCutOff	0xD235
#define PTP_PC_FUJI_StorageID		0xd244
#define PTP_PC_FUJI_DriveMode		0xd246
#define PTP_PC_FUJI_Unknown_D400	0xd400 // probably version code
#define PTP_PC_FUJI_ObjectCount2	0xd401
#define PTP_PC_FUJI_Unknown2		0xdc04
#define PTP_PC_FUJI_Unknown1		0xd246
#define PTP_PC_FUJI_Unknown7		0xd406
#define PTP_PC_FUJI_Unknown8		0xd407
#define PTP_PC_FUJI_Unknown5		0xd500
#define PTP_PC_FUJI_Unknown_D52F	0xd52f // probably version code
#define PTP_PC_FUJI_ImageGetVersion	0xdf21
#define PTP_PC_FUJI_ImageExploreVersion	0xdf22
#define PTP_PC_FUJI_Unknown10		0xdf23 // another version prop?
#define PTP_PC_FUJI_RemoteVersion	0xdf24
#define PTP_PC_FUJI_RemoteImageExploreVersion	0xdf25
#define PTP_PC_FUJI_ImageGetLimitedVersion	0xdf26
#define PTP_PC_FUJI_Unknown13		0xdf27
#define PTP_PC_FUJI_Unknown_DF28	0xdf28
#define PTP_PC_FUJI_LocationGetterVersion	0xdf31
#define PTP_PC_FUJI_Unknown11		0xdf44
#define PTP_PC_FUJI_Unknown17		0xD621

// Function Modes
#define FUJI_VIEW_MULTIPLE	1
#define FUJI_VIEW_ALL_IMGS	2
#define FUJI_MODE_UNKNOWN2	3
#define FUJI_MODE_UNKNOWN1	4
#define FUJI_REMOTE_MODE	5
#define FUJI_MODE_UNKNOWN3	6
#define FUJI_CAMERA_ERR		7
#define FUJI_SELECTED_FRAME	8
#define FUJI_MODE_IMG_VIEW_IN_CAM	9
#define FUJI_GPS_ASSIST_V2	10
#define FUJI_MODE_REMOTE_IMG_VIEW	11
#define FUJI_MODE_SET_GPS	17
#define FUJI_LIMITED_IMG_TRANSMISSION	18
#define FUJI_MODE_TRANSFER_FIRMARE	19
#define FUJI_MODE_REMOTE_IMG_VIEW_BLE	20 // ????

// Camera states
#define FUJI_WAIT_FOR_ACCESS	0
#define FUJI_MULTIPLE_TRANSFER	1
#define FUJI_FULL_ACCESS	2
#define FUJI_REMOTE_ACCESS	6

// temporary stuff from libgphoto2 ptp.h - most are innacurate
#define PTP_EC_FUJI_PreviewAvailable		0xC001
#define PTP_EC_FUJI_ObjectAdded			0xC004

#define PTP_PC_FUJI_FilmSimulation			0xD001
#define PTP_PC_FUJI_FilmSimulationTune			0xD002
#define PTP_PC_FUJI_DRangeMode				0xD007
#define PTP_PC_FUJI_ColorMode				0xD008
#define PTP_PC_FUJI_ColorSpace				0xD00A
#define PTP_PC_FUJI_WhitebalanceTune1			0xD00B
#define PTP_PC_FUJI_WhitebalanceTune2			0xD00C
#define PTP_PC_FUJI_ColorTemperature			0xD017
#define PTP_PC_FUJI_Quality				0xD018
#define PTP_PC_FUJI_RecMode				0xD019 /* LiveViewColorMode? */
#define PTP_PC_FUJI_LiveViewBrightness			0xD01A
#define PTP_PC_FUJI_ThroughImageZoom			0xD01B
#define PTP_PC_FUJI_NoiseReduction			0xD01C
#define PTP_PC_FUJI_MacroMode				0xD01D
#define PTP_PC_FUJI_LiveViewStyle			0xD01E
#define PTP_PC_FUJI_FaceDetectionMode			0xD020
#define PTP_PC_FUJI_RedEyeCorrectionMode		0xD021
#define PTP_PC_FUJI_RawCompression			0xD022
#define PTP_PC_FUJI_GrainEffect			0xD023
#define PTP_PC_FUJI_SetEyeAFMode			0xD024
#define PTP_PC_FUJI_FocusPoints			0xD025
#define PTP_PC_FUJI_MFAssistMode			0xD026
#define PTP_PC_FUJI_InterlockAEAFArea			0xD027
#define PTP_PC_FUJI_CommandDialMode			0xD028
#define PTP_PC_FUJI_Shadowing				0xD029
/* d02a - d02c also appear in setafmode */
#define PTP_PC_FUJI_ExposureIndex			0xD02A
#define PTP_PC_FUJI_MovieISO				0xD02B
#define PTP_PC_FUJI_WideDynamicRange			0xD02E
#define PTP_PC_FUJI_TNumber				0xD02F
#define PTP_PC_FUJI_Comment				0xD100
#define PTP_PC_FUJI_SerialMode				0xD101
#define PTP_PC_FUJI_ExposureDelay			0xD102
#define PTP_PC_FUJI_PreviewTime			0xD103
#define PTP_PC_FUJI_BlackImageTone			0xD104
#define PTP_PC_FUJI_Illumination			0xD105
#define PTP_PC_FUJI_FrameGuideMode			0xD106
#define PTP_PC_FUJI_ViewfinderWarning			0xD107
#define PTP_PC_FUJI_AutoImageRotation			0xD108
#define PTP_PC_FUJI_DetectImageRotation		0xD109
#define PTP_PC_FUJI_ShutterPriorityMode1		0xD10A
#define PTP_PC_FUJI_ShutterPriorityMode2		0xD10B
#define PTP_PC_FUJI_AFIlluminator			0xD112
#define PTP_PC_FUJI_Beep				0xD113
#define PTP_PC_FUJI_AELock				0xD114
#define PTP_PC_FUJI_ISOAutoSetting1			0xD115
#define PTP_PC_FUJI_ISOAutoSetting2			0xD116
#define PTP_PC_FUJI_ISOAutoSetting3			0xD117
#define PTP_PC_FUJI_ExposureStep			0xD118
#define PTP_PC_FUJI_CompensationStep			0xD119
#define PTP_PC_FUJI_ExposureSimpleSet			0xD11A
#define PTP_PC_FUJI_CenterPhotometryRange		0xD11B
#define PTP_PC_FUJI_PhotometryLevel1			0xD11C
#define PTP_PC_FUJI_PhotometryLevel2			0xD11D
#define PTP_PC_FUJI_PhotometryLevel3			0xD11E
#define PTP_PC_FUJI_FlashTuneSpeed			0xD11F
#define PTP_PC_FUJI_FlashShutterLimit			0xD120
#define PTP_PC_FUJI_BuiltinFlashMode			0xD121
#define PTP_PC_FUJI_FlashManualMode			0xD122
#define PTP_PC_FUJI_FlashRepeatingMode1		0xD123
#define PTP_PC_FUJI_FlashRepeatingMode2		0xD124
#define PTP_PC_FUJI_FlashRepeatingMode3		0xD125
#define PTP_PC_FUJI_FlashCommanderMode1		0xD126
#define PTP_PC_FUJI_FlashCommanderMode2		0xD127
#define PTP_PC_FUJI_FlashCommanderMode3		0xD128
#define PTP_PC_FUJI_FlashCommanderMode4		0xD129
#define PTP_PC_FUJI_FlashCommanderMode5		0xD12A
#define PTP_PC_FUJI_FlashCommanderMode6		0xD12B
#define PTP_PC_FUJI_FlashCommanderMode7		0xD12C
#define PTP_PC_FUJI_ModelingFlash			0xD12D
#define PTP_PC_FUJI_BKT				0xD12E
#define PTP_PC_FUJI_BKTChange				0xD12F
#define PTP_PC_FUJI_BKTOrder				0xD130
#define PTP_PC_FUJI_BKTSelection			0xD131
#define PTP_PC_FUJI_AEAFLockButton			0xD132
#define PTP_PC_FUJI_CenterButton			0xD133
#define PTP_PC_FUJI_MultiSelectorButton		0xD134
#define PTP_PC_FUJI_FunctionLock			0xD136
#define PTP_PC_FUJI_Password				0xD145
#define PTP_PC_FUJI_ChangePassword			0xD146	/* ? */
#define PTP_PC_FUJI_CommandDialSetting1		0xD147
#define PTP_PC_FUJI_CommandDialSetting2		0xD148
#define PTP_PC_FUJI_CommandDialSetting3		0xD149
#define PTP_PC_FUJI_CommandDialSetting4		0xD14A
#define PTP_PC_FUJI_ButtonsAndDials			0xD14B
#define PTP_PC_FUJI_NonCPULensData			0xD14C
#define PTP_PC_FUJI_MBD200Batteries			0xD14E
#define PTP_PC_FUJI_AFOnForMBD200Batteries		0xD14F
#define PTP_PC_FUJI_FirmwareVersion			0xD153
#define PTP_PC_FUJI_ShotCount				0xD154
#define PTP_PC_FUJI_ShutterExchangeCount		0xD155
#define PTP_PC_FUJI_WorldClock				0xD157
#define PTP_PC_FUJI_TimeDifference1			0xD158
#define PTP_PC_FUJI_TimeDifference2			0xD159
#define PTP_PC_FUJI_Language				0xD15A
#define PTP_PC_FUJI_FrameNumberSequence		0xD15B
#define PTP_PC_FUJI_VideoMode				0xD15C
#define PTP_PC_FUJI_SetUSBMode				0xD15D
#define PTP_PC_FUJI_CommentWriteSetting		0xD161
#define PTP_PC_FUJI_BCRAppendDelimiter			0xD162
#define PTP_PC_FUJI_CommentEx				0xD167
#define PTP_PC_FUJI_VideoOutOnOff			0xD168
#define PTP_PC_FUJI_CropMode				0xD16F
#define PTP_PC_FUJI_LensZoomPos			0xD170
#define PTP_PC_FUJI_FocusPosition			0xD171
#define PTP_PC_FUJI_LiveViewImageQuality		0xD173
#define PTP_PC_FUJI_LiveViewImageSize			0xD174
#define PTP_PC_FUJI_LiveViewCondition			0xD175
#define PTP_PC_FUJI_StandbyMode			0xD176
#define PTP_PC_FUJI_LiveViewExposure			0xD177
#define PTP_PC_FUJI_LiveViewWhiteBalance		0xD178 /* same values as 0x5005 */
#define PTP_PC_FUJI_LiveViewWhiteBalanceGain		0xD179
#define PTP_PC_FUJI_LiveViewTuning			0xD17A
#define PTP_PC_FUJI_FocusMeteringMode			0xD17C
#define PTP_PC_FUJI_FocusLength			0xD17D
#define PTP_PC_FUJI_CropAreaFrameInfo			0xD17E
#define PTP_PC_FUJI_ResetSetting			0xD17F /* also clean sensor? */
#define PTP_PC_FUJI_IOPCode				0xD184
#define PTP_PC_FUJI_TetherRawConditionCode		0xD186
#define PTP_PC_FUJI_TetherRawCompatibilityCode		0xD187
#define PTP_PC_FUJI_LightTune				0xD200
#define PTP_PC_FUJI_ReleaseMode			0xD201
#define PTP_PC_FUJI_BKTFrame1				0xD202
#define PTP_PC_FUJI_BKTFrame2				0xD203
#define PTP_PC_FUJI_BKTStep				0xD204
#define PTP_PC_FUJI_ProgramShift			0xD205
#define PTP_PC_FUJI_FocusAreas				0xD206
#define PTP_PC_FUJI_PriorityMode			0xD207 /* from setprioritymode */

#define PTP_PC_FUJI_CaptureRemaining			0xD229	/* Movie AF Mode? */
#define PTP_PC_FUJI_MovieRemainingTime			0xD22A	/* Movie Focus Area? */
#define PTP_PC_FUJI_DeviceError			0xD21B
#define PTP_PC_FUJI_BatteryLevel			0xD242 /* Movie Sensitivity???? */
#define PTP_PC_FUJI_ImageAspectRatio			0xD241

#define PTP_PC_FUJI_AFStatus				0xD209
#define PTP_PC_FUJI_DeviceName				0xD20B
#define PTP_PC_FUJI_MediaRecord			0xD20C /* from capmediarecord */
#define PTP_PC_FUJI_MediaCapacity			0xD20D
#define PTP_PC_FUJI_FreeSDRAMImages			0xD20E /* free images in SDRAM */
#define PTP_PC_FUJI_MediaStatus			0xD211
#define PTP_PC_FUJI_CurrentState			0xD212
#define PTP_PC_FUJI_AELock2				0xD213
#define PTP_PC_FUJI_Copyright				0xD215
#define PTP_PC_FUJI_Copyright2				0xD216
#define PTP_PC_FUJI_Aperture				0xD218
#define PTP_PC_FUJI_ShutterSpeed			0xD219
#define PTP_PC_FUJI_DeviceError			0xD21B
#define PTP_PC_FUJI_SensitivityFineTune1		0xD222
#define PTP_PC_FUJI_SensitivityFineTune2		0xD223
#define PTP_PC_FUJI_CaptureRemaining			0xD229	/* Movie AF Mode? */
#define PTP_PC_FUJI_MovieRemainingTime			0xD22A	/* Movie Focus Area? */
#define PTP_PC_FUJI_ForceMode				0xD230
#define PTP_PC_FUJI_ShutterSpeed2			0xD240 /* Movie Aperture */
#define PTP_PC_FUJI_ImageAspectRatio			0xD241
#define PTP_PC_FUJI_BatteryLevel			0xD242 /* Movie Sensitivity???? */
#define PTP_PC_FUJI_TotalShotCount			0xD310
#define PTP_PC_FUJI_HighLightTone			0xD320
#define PTP_PC_FUJI_ShadowTone				0xD321
#define PTP_PC_FUJI_LongExposureNR			0xD322
#define PTP_PC_FUJI_FullTimeManualFocus		0xD323
#define PTP_PC_FUJI_ISODialHn1				0xD332
#define PTP_PC_FUJI_ISODialHn2				0xD333
#define PTP_PC_FUJI_ViewMode1				0xD33F
#define PTP_PC_FUJI_ViewMode2				0xD340
#define PTP_PC_FUJI_DispInfoMode			0xD343
#define PTP_PC_FUJI_LensISSwitch			0xD346
#define PTP_PC_FUJI_FocusPoint				0xD347
#define PTP_PC_FUJI_InstantAFMode			0xD34A
#define PTP_PC_FUJI_PreAFMode				0xD34B
#define PTP_PC_FUJI_CustomSetting			0xD34C
#define PTP_PC_FUJI_LMOMode				0xD34D
#define PTP_PC_FUJI_LockButtonMode			0xD34E
#define PTP_PC_FUJI_AFLockMode				0xD34F
#define PTP_PC_FUJI_MicJackMode			0xD350
#define PTP_PC_FUJI_ISMode				0xD351
#define PTP_PC_FUJI_DateTimeDispFormat			0xD352
#define PTP_PC_FUJI_AeAfLockKeyAssign			0xD353
#define PTP_PC_FUJI_CrossKeyAssign			0xD354
#define PTP_PC_FUJI_SilentMode				0xD355
#define PTP_PC_FUJI_PBSound				0xD356
#define PTP_PC_FUJI_EVFDispAutoRotate			0xD358
#define PTP_PC_FUJI_ExposurePreview			0xD359
#define PTP_PC_FUJI_DispBrightness1			0xD35A
#define PTP_PC_FUJI_DispBrightness2			0xD35B
#define PTP_PC_FUJI_DispChroma1			0xD35C
#define PTP_PC_FUJI_DispChroma2			0xD35D
#define PTP_PC_FUJI_FocusCheckMode			0xD35E
#define PTP_PC_FUJI_FocusScaleUnit			0xD35F
#define PTP_PC_FUJI_SetFunctionButton			0xD361
#define PTP_PC_FUJI_SensorCleanTiming			0xD363
#define PTP_PC_FUJI_CustomAutoPowerOff			0xD364
#define PTP_PC_FUJI_FileNamePrefix1			0xD365
#define PTP_PC_FUJI_FileNamePrefix2			0xD366
#define PTP_PC_FUJI_BatteryInfo1			0xD36A
#define PTP_PC_FUJI_BatteryInfo2			0xD36B
#define PTP_PC_FUJI_LensNameAndSerial			0xD36D
#define PTP_PC_FUJI_CustomDispInfo			0xD36E
#define PTP_PC_FUJI_FunctionLockCategory1		0xD36F
#define PTP_PC_FUJI_FunctionLockCategory2		0xD370
#define PTP_PC_FUJI_CustomPreviewTime			0xD371
#define PTP_PC_FUJI_FocusArea1				0xD372
#define PTP_PC_FUJI_FocusArea2				0xD373
#define PTP_PC_FUJI_FocusArea3				0xD374
#define PTP_PC_FUJI_FrameGuideGridInfo1		0xD375
#define PTP_PC_FUJI_FrameGuideGridInfo2		0xD376
#define PTP_PC_FUJI_FrameGuideGridInfo3		0xD377
#define PTP_PC_FUJI_FrameGuideGridInfo4		0xD378
#define PTP_PC_FUJI_LensUnknownData			0xD38A
#define PTP_PC_FUJI_LensZoomPosCaps			0xD38C
#define PTP_PC_FUJI_LensFNumberList			0xD38D
#define PTP_PC_FUJI_LensFocalLengthList		0xD38E
#define PTP_PC_FUJI_FocusLimiter			0xD390
#define PTP_PC_FUJI_FocusArea4				0xD395

#endif
