#ifndef FUJIPTP_H
#define FUJIPTP_H

// Extra PTP defs for Fuifilm

// Downloader functions
#define PTP_OC_FUJI_Unknown1	0x9054
#define PTP_OC_FUJI_Unknown2	0x9055

#define PTP_PC_FUJI_Unknown4		0xD228
#define PTP_PC_FUJI_Unknown15		0xD22B
#define PTP_PC_FUJI_CompressionCutOff	0xD235
#define PTP_PC_FUJI_StorageID		0xd244
#define PTP_PC_FUJI_DriveMode		0xd246
#define PTP_PC_FUJI_ObjectCount2	0xd401
#define PTP_PC_FUJI_Unknown2		0xdc04
#define PTP_PC_FUJI_Unknown1		0xd246
#define PTP_PC_FUJI_Unknown7		0xd406
#define PTP_PC_FUJI_Unknown8		0xd407
#define PTP_PC_FUJI_Unknown5		0xd500
#define PTP_PC_FUJI_Unknown6		0xd52f
#define PTP_PC_FUJI_ImageGetVersion	0xdf21
#define PTP_PC_FUJI_ImageExploreVersion	0xdf22
#define PTP_PC_FUJI_Unknown10		0xdf23 // another version prop?
#define PTP_PC_FUJI_RemoteVersion	0xdf24
#define PTP_PC_FUJI_RemoteImageExploreVersion	0xdf25
#define PTP_PC_FUJI_ImageGetLimitedVersion	0xdf26
#define PTP_PC_FUJI_Unknown13		0xdf27
#define PTP_PC_FUJI_Unknown16		0xdf28
#define PTP_PC_FUJI_Unknown14		0xdf31
#define PTP_PC_FUJI_Unknown11		0xdf44

#define PTP_PC_FUJI_Unknown17		0xD621

// Function Modes
#define FUJI_VIEW_MULTIPLE	1
#define FUJI_VIEW_ALL_IMGS	2
#define FUJI_MODE_UNKNOWN1	4
#define FUJI_REMOTE_MODE	5
#define FUJI_MODE_UNKNOWN2	6
#define FUJI_CAMERA_ERR		7
#define FUJI_SELECTED_FRAME	8
#define FUJI_MODE_IMG_VIEW_IN_CAM	9
#define FUJI_GPS_ASSIST_V2	10
#define FUJI_MODE_REMOTE_IMG_VIEW	11
#define FUJI_MODE_SET_GPS	17
#define FUJI_LIMITED_IMG_TRANSMISSION	18
#define FUJI_MODE_TRANSFER_FIRMARE	19
#define FUJI_LIVEVIEW	20 // ?????

// Camera states
#define FUJI_WAIT_FOR_ACCESS	0
#define FUJI_MULTIPLE_TRANSFER	1
#define FUJI_FULL_ACCESS	2
#define FUJI_REMOTE_ACCESS	6

// Stuff from libgphoto2 ptp.h - a lot of USB stuff
#define PTP_EC_FUJI_PreviewAvailable		0xC001
#define PTP_EC_FUJI_ObjectAdded			0xC004

#define PTP_DPC_FUJI_FilmSimulation			0xD001
#define PTP_DPC_FUJI_FilmSimulationTune			0xD002
#define PTP_DPC_FUJI_DRangeMode				0xD007
#define PTP_DPC_FUJI_ColorMode				0xD008
#define PTP_DPC_FUJI_ColorSpace				0xD00A
#define PTP_DPC_FUJI_WhitebalanceTune1			0xD00B
#define PTP_DPC_FUJI_WhitebalanceTune2			0xD00C
#define PTP_DPC_FUJI_ColorTemperature			0xD017
#define PTP_DPC_FUJI_Quality				0xD018
#define PTP_DPC_FUJI_RecMode				0xD019 /* LiveViewColorMode? */
#define PTP_DPC_FUJI_LiveViewBrightness			0xD01A
#define PTP_DPC_FUJI_ThroughImageZoom			0xD01B
#define PTP_DPC_FUJI_NoiseReduction			0xD01C
#define PTP_DPC_FUJI_MacroMode				0xD01D
#define PTP_DPC_FUJI_LiveViewStyle			0xD01E
#define PTP_DPC_FUJI_FaceDetectionMode			0xD020
#define PTP_DPC_FUJI_RedEyeCorrectionMode		0xD021
#define PTP_DPC_FUJI_RawCompression			0xD022
#define PTP_DPC_FUJI_GrainEffect			0xD023
#define PTP_DPC_FUJI_SetEyeAFMode			0xD024
#define PTP_DPC_FUJI_FocusPoints			0xD025
#define PTP_DPC_FUJI_MFAssistMode			0xD026
#define PTP_DPC_FUJI_InterlockAEAFArea			0xD027
#define PTP_DPC_FUJI_CommandDialMode			0xD028
#define PTP_DPC_FUJI_Shadowing				0xD029
/* d02a - d02c also appear in setafmode */
#define PTP_DPC_FUJI_ExposureIndex			0xD02A
#define PTP_DPC_FUJI_MovieISO				0xD02B
#define PTP_DPC_FUJI_WideDynamicRange			0xD02E
#define PTP_DPC_FUJI_TNumber				0xD02F
#define PTP_DPC_FUJI_Comment				0xD100
#define PTP_DPC_FUJI_SerialMode				0xD101
#define PTP_DPC_FUJI_ExposureDelay			0xD102
#define PTP_DPC_FUJI_PreviewTime			0xD103
#define PTP_DPC_FUJI_BlackImageTone			0xD104
#define PTP_DPC_FUJI_Illumination			0xD105
#define PTP_DPC_FUJI_FrameGuideMode			0xD106
#define PTP_DPC_FUJI_ViewfinderWarning			0xD107
#define PTP_DPC_FUJI_AutoImageRotation			0xD108
#define PTP_DPC_FUJI_DetectImageRotation		0xD109
#define PTP_DPC_FUJI_ShutterPriorityMode1		0xD10A
#define PTP_DPC_FUJI_ShutterPriorityMode2		0xD10B
#define PTP_DPC_FUJI_AFIlluminator			0xD112
#define PTP_DPC_FUJI_Beep				0xD113
#define PTP_DPC_FUJI_AELock				0xD114
#define PTP_DPC_FUJI_ISOAutoSetting1			0xD115
#define PTP_DPC_FUJI_ISOAutoSetting2			0xD116
#define PTP_DPC_FUJI_ISOAutoSetting3			0xD117
#define PTP_DPC_FUJI_ExposureStep			0xD118
#define PTP_DPC_FUJI_CompensationStep			0xD119
#define PTP_DPC_FUJI_ExposureSimpleSet			0xD11A
#define PTP_DPC_FUJI_CenterPhotometryRange		0xD11B
#define PTP_DPC_FUJI_PhotometryLevel1			0xD11C
#define PTP_DPC_FUJI_PhotometryLevel2			0xD11D
#define PTP_DPC_FUJI_PhotometryLevel3			0xD11E
#define PTP_DPC_FUJI_FlashTuneSpeed			0xD11F
#define PTP_DPC_FUJI_FlashShutterLimit			0xD120
#define PTP_DPC_FUJI_BuiltinFlashMode			0xD121
#define PTP_DPC_FUJI_FlashManualMode			0xD122
#define PTP_DPC_FUJI_FlashRepeatingMode1		0xD123
#define PTP_DPC_FUJI_FlashRepeatingMode2		0xD124
#define PTP_DPC_FUJI_FlashRepeatingMode3		0xD125
#define PTP_DPC_FUJI_FlashCommanderMode1		0xD126
#define PTP_DPC_FUJI_FlashCommanderMode2		0xD127
#define PTP_DPC_FUJI_FlashCommanderMode3		0xD128
#define PTP_DPC_FUJI_FlashCommanderMode4		0xD129
#define PTP_DPC_FUJI_FlashCommanderMode5		0xD12A
#define PTP_DPC_FUJI_FlashCommanderMode6		0xD12B
#define PTP_DPC_FUJI_FlashCommanderMode7		0xD12C
#define PTP_DPC_FUJI_ModelingFlash			0xD12D
#define PTP_DPC_FUJI_BKT				0xD12E
#define PTP_DPC_FUJI_BKTChange				0xD12F
#define PTP_DPC_FUJI_BKTOrder				0xD130
#define PTP_DPC_FUJI_BKTSelection			0xD131
#define PTP_DPC_FUJI_AEAFLockButton			0xD132
#define PTP_DPC_FUJI_CenterButton			0xD133
#define PTP_DPC_FUJI_MultiSelectorButton		0xD134
#define PTP_DPC_FUJI_FunctionLock			0xD136
#define PTP_DPC_FUJI_Password				0xD145
#define PTP_DPC_FUJI_ChangePassword			0xD146	/* ? */
#define PTP_DPC_FUJI_CommandDialSetting1		0xD147
#define PTP_DPC_FUJI_CommandDialSetting2		0xD148
#define PTP_DPC_FUJI_CommandDialSetting3		0xD149
#define PTP_DPC_FUJI_CommandDialSetting4		0xD14A
#define PTP_DPC_FUJI_ButtonsAndDials			0xD14B
#define PTP_DPC_FUJI_NonCPULensData			0xD14C
#define PTP_DPC_FUJI_MBD200Batteries			0xD14E
#define PTP_DPC_FUJI_AFOnForMBD200Batteries		0xD14F
#define PTP_DPC_FUJI_FirmwareVersion			0xD153
#define PTP_DPC_FUJI_ShotCount				0xD154
#define PTP_DPC_FUJI_ShutterExchangeCount		0xD155
#define PTP_DPC_FUJI_WorldClock				0xD157
#define PTP_DPC_FUJI_TimeDifference1			0xD158
#define PTP_DPC_FUJI_TimeDifference2			0xD159
#define PTP_DPC_FUJI_Language				0xD15A
#define PTP_DPC_FUJI_FrameNumberSequence		0xD15B
#define PTP_DPC_FUJI_VideoMode				0xD15C
#define PTP_DPC_FUJI_SetUSBMode				0xD15D
#define PTP_DPC_FUJI_CommentWriteSetting		0xD161
#define PTP_DPC_FUJI_BCRAppendDelimiter			0xD162
#define PTP_DPC_FUJI_CommentEx				0xD167
#define PTP_DPC_FUJI_VideoOutOnOff			0xD168
#define PTP_DPC_FUJI_CropMode				0xD16F
#define PTP_DPC_FUJI_LensZoomPos			0xD170
#define PTP_DPC_FUJI_FocusPosition			0xD171
#define PTP_DPC_FUJI_LiveViewImageQuality		0xD173
#define PTP_DPC_FUJI_LiveViewImageSize			0xD174
#define PTP_DPC_FUJI_LiveViewCondition			0xD175
#define PTP_DPC_FUJI_StandbyMode			0xD176
#define PTP_DPC_FUJI_LiveViewExposure			0xD177
#define PTP_DPC_FUJI_LiveViewWhiteBalance		0xD178 /* same values as 0x5005 */
#define PTP_DPC_FUJI_LiveViewWhiteBalanceGain		0xD179
#define PTP_DPC_FUJI_LiveViewTuning			0xD17A
#define PTP_DPC_FUJI_FocusMeteringMode			0xD17C
#define PTP_DPC_FUJI_FocusLength			0xD17D
#define PTP_DPC_FUJI_CropAreaFrameInfo			0xD17E
#define PTP_DPC_FUJI_ResetSetting			0xD17F /* also clean sensor? */
#define PTP_DPC_FUJI_IOPCode				0xD184
#define PTP_DPC_FUJI_TetherRawConditionCode		0xD186
#define PTP_DPC_FUJI_TetherRawCompatibilityCode		0xD187
#define PTP_DPC_FUJI_LightTune				0xD200
#define PTP_DPC_FUJI_ReleaseMode			0xD201
#define PTP_DPC_FUJI_BKTFrame1				0xD202
#define PTP_DPC_FUJI_BKTFrame2				0xD203
#define PTP_DPC_FUJI_BKTStep				0xD204
#define PTP_DPC_FUJI_ProgramShift			0xD205
#define PTP_DPC_FUJI_FocusAreas				0xD206
#define PTP_DPC_FUJI_PriorityMode			0xD207 /* from setprioritymode */

#endif
