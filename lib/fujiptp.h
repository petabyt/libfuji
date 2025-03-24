/// @file
/// This file (tries to) describes all of Fuji's PTP vendor extensions
#ifndef FUJIPTP_H
#define FUJIPTP_H

#define FUJI_PROTOCOL_VERSION 0x8f53e4f2

/// @brief All of Fuji's protocols/transport features
enum FujiTransport {
	/// @brief 'PLAY BACK MENU' -> 'PC AUTO SAVE'
	FUJI_FEATURE_AUTOSAVE = 1,
	/// @brief 'WIRELESS TETHER SHOOTING FIXED'
	FUJI_FEATURE_WIRELESS_TETHER = 2,
	/// @brief 'WIRELESS COMMUNICATION' or 'WIRELESS TRANSFER'
	FUJI_FEATURE_WIRELESS_COMM = 3,
	/// @brief 'USB CARD READER'
	FUJI_FEATURE_USB_CARD_READER = 4,
	/// @brief 'USB TETHER SHOOTING FIXED/AUTO'
	FUJI_FEATURE_USB_TETHER_SHOOT = 5,
	/// @brief 'USB RAW CONV./BACKUP RESTORE'
	FUJI_FEATURE_RAW_CONV = 6,
	/// @brief Generic PTP/USB of unknown feature, assume MTP
	FUJI_FEATURE_USB = 7,
	/// @brief Recently added 'X WEBCAM' mode
	FUJI_FEATURE_WEBCAM = 8,
	/// @brief 'USB MOVIE SHOOTING FIXED/AUTO'
	FUJI_FEATURE_MOVIE_SHOOT = 9,
};

#define FUJI_CMD_IP_PORT 55740
#define FUJI_EVENT_IP_PORT 55741
#define FUJI_LIVEVIEW_IP_PORT 55742

// Fuji USB and IP extensions, available on almost all cameras
#define PTP_OC_FUJI_SendObjectInfo	0x900c // create file
#define PTP_OC_FUJI_SendObject2		0x900d // Appears to be the same as 901d
#define PTP_OC_FUJI_SendObject		0x901d // write to file


/// @defgroup Device Property Codes
/// @brief All the Fuji vendor codes defined in libfudge
/// @addtogroup DPC
/// @{

#define PTP_DPC_FUJI_EventsList		0xd212
#define PTP_DPC_FUJI_SelectedImgsMode	0xd220
#define PTP_DPC_FUJI_ObjectCount		0xd222
#define PTP_DPC_FUJI_CameraState		0xdf00
#define PTP_DPC_FUJI_ClientState		0xdf01
/// @info If 1, will heavily compress images into 400kb-800kb. Fuji uses this for downloading a quick preview.
#define PTP_DPC_FUJI_CompressSmall	0xD226
/// @info If 0 (default value), the compressed_size field of PtpObjectInfo will be set at 100kb.
/// If 1, it will set the correct file size.
/// Fuji sets this to 1 before GetObjectInfo and GetPartialObject(s) calls, and 0 after.
/// It has no noticeable performance impact on download speed there. But if this property is set in
/// the setup process, then it will make the gallery slower (I think slowing down GetThumb calls).
/// PC AutoSave will set this to 1 on startup, although this doesn't make a difference since it never calls GetThumb.
#define PTP_DPC_FUJI_EnableCorrectFileSize	0xD227 // Enable full image download

// Fuji Camera Connect has this version - 2.11 if parsed as bytes. Or 11.2
// XS10 on reported 0x02000A, camera connect set to 2000B
#define FUJI_CAM_CONNECT_REMOTE_VER 0x2000C

// Downloader opcodes, mostly unknown
#define PTP_OC_FUJI_Unknown1	0x9054
#define PTP_OC_FUJI_Unknown2	0x9055

#define PTP_DPC_FUJI_UnknownD21C	0xd21c
#define PTP_DPC_FUJI_Unknown_D224	0xd224
// Client sets this in PC Autosave, likely for the camera to update it's database of images the client hasn't downloaded?
// See `struct FujiD228` - structure appears similar to PTP_DPC_FUJI_BatteryInfo2
// Setting this doesn't appear to be necessary for downloading photos
#define PTP_DPC_FUJI_AutoSaveDatabaseStatus	0xD228
#define PTP_DPC_FUJI_Unknown15		0xD22B
#define PTP_DPC_FUJI_CompressionCutOff	0xD235
#define PTP_DPC_FUJI_StorageID		0xd244
#define PTP_DPC_FUJI_Unknown_D400	0xd400 // Possibly SelectedImgsMode2
#define PTP_DPC_FUJI_ObjectCount2	0xd401
#define PTP_DPC_FUJI_Unknown2		0xdc04
#define PTP_DPC_FUJI_Unknown1		0xd246 //
#define PTP_DPC_FUJI_Unknown7		0xd406
#define PTP_DPC_FUJI_Unknown8		0xd407
#define PTP_DPC_FUJI_Geolocation		0xd500 // "0000.000000,N00000.000000,E00000.00,M 000.0,K0000:00:0000:00:00.000"
#define PTP_DPC_FUJI_Unknown_D52F	0xd52f

// Xapp properties
#define PTP_DPC_FUJI_Unknown18		0xD620
#define PTP_DPC_FUJI_Unknown17		0xD621

// Most of 0xdfxx appear to be version/revision properties
/// @brief Prop checked before doing image related things
#define PTP_DPC_FUJI_ImageGetVersion	0xdf21
/// @brief Property that differentiates behavior for GetObjectInfo and GetObject
#define PTP_DPC_FUJI_GetObjectVersion	0xdf22
#define PTP_DPC_FUJI_AutoSaveVersion	0xdf23
#define PTP_DPC_FUJI_RemoteVersion	0xdf24
#define PTP_DPC_FUJI_RemoteGetObjectVersion	0xdf25 // same as GetObjectVersion, but for cams that support remote mode
// 0xdf26 and 0xdf27 appear to be unused
#define PTP_DPC_FUJI_Unknown_DF28	0xdf28 // xapp property, x-s10 sets to 1
#define PTP_DPC_FUJI_GeoTagVersion	0xdf31
#define PTP_DPC_FUJI_Unknown11		0xdf44

enum ClientStates {
	// Set if camera state is FUJI_MULTIPLE_TRANSFER,
	FUJI_VIEW_MULTIPLE = 1,
	// Set to view all images and have normal PTP functionality
	FUJI_VIEW_ALL_IMGS = 2,
	// Old remote - maybe from 2015-2017
	FUJI_OLD_REMOTE = 3,
	FUJI_MODE_UNKNOWN1 = 4,
	// Set to enter full remote mode
	FUJI_REMOTE_MODE = 5,
	FUJI_MODE_UNKNOWN3 = 6,
	// Set to tell camera that client has an error
	FUJI_CAMERA_ERR = 7,
	// Set to a similar (but completely different) version of FUJI_MULTIPLE_TRANSFER.
	// Client requests this, and user selects images on camera.
	FUJI_MULTIPLE_TRANSFER_REQ = 8,
	FUJI_MODE_IMG_VIEW_IN_CAM = 9,
	FUJI_GPS_ASSIST_V2 = 10,
	// Set to quiet down the liveview/remote functionality (still running I think) and start the image gallery
	// This is only for remote cameras. Better name would be IMG_VIEW_EXTENDED (?)
	FUJI_MODE_REMOTE_IMG_VIEW = 11,
	// Set GPS on cam, haven't tested
	FUJI_MODE_SET_GPS = 17,
	// All seem to be new or bluetooth only functionality
	FUJI_LIMITED_IMG_TRANSMISSION = 18,
	FUJI_MODE_TRANSFER_FIRMWARE = 19,
	FUJI_MODE_REMOTE_IMG_VIEW_XAPP = 20
};

// Modes for SelectedImgsMode
#define FUJI_SELECT_MULTIPLE_MODE_1 1

/// @brief Possible values of PTP_DPC_FUJI_CameraState
enum FujiStates {
	// We need to wait and poll camera for access
	FUJI_WAIT_FOR_ACCESS = 0,
	// Camera has indicated it has single or multiple photos to transfer. Go into loop to accept them.
	FUJI_MULTIPLE_TRANSFER = 1,
	// We have full access to the camera (non-remote), we can run the photo gallery, download photos,
	// and do normal PTP stuff.
	FUJI_FULL_ACCESS = 2,
	// PC save software (over WiFi -> UPnP)
	FUJI_PC_AUTO_SAVE = 3,
	// We have all features of FUJI_FULL_ACCESS and remote mode.
	FUJI_REMOTE_ACCESS = 6,
};

/// @brief Possible values for PTP_DPC_FUJI_USBMode
enum FujiUSBModes {
	FUJI_USB_MODE_TETHER = 5,
	FUJI_USB_MODE_RAWCONV = 6,
	FUJI_USB_MODE_WEBCAM = 8,
};

// ECs and PCs stuff from libgphoto2 ptp.h - most appear to be inaccurate or misplaced
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
#define PTP_DPC_FUJI_UnknownD040			0xd040
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
/// @brief value is 5 when in tether shoot mode, 6 in raw conv mode
/// 8 for x webcam mode
/// TODO: What value is MTP?
#define PTP_DPC_FUJI_USBMode				0xd16e
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
// 0xd180
#define PTP_DPC_FUJI_StartRawConversion			0xD183
#define PTP_DPC_FUJI_IOPCode				0xD184
#define PTP_DPC_FUJI_RawConvProfile			0xD185
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
#define PTP_DPC_FUJI_AFStatus				0xD209
#define PTP_DPC_FUJI_DeviceName				0xD20B
#define PTP_DPC_FUJI_MediaRecord			0xD20C /* from capmediarecord */
#define PTP_DPC_FUJI_MediaCapacity			0xD20D
#define PTP_DPC_FUJI_FreeSDRAMImages			0xD20E /* free images in SDRAM */
#define PTP_DPC_FUJI_MediaStatus			0xD211
#define PTP_DPC_FUJI_CurrentState			0xD212
#define PTP_DPC_FUJI_AELock2				0xD213
#define PTP_DPC_FUJI_Copyright				0xD215
#define PTP_DPC_FUJI_Copyright2				0xD216
#define PTP_DPC_FUJI_Aperture				0xD218
#define PTP_DPC_FUJI_ShutterSpeed			0xD219
#define PTP_DPC_FUJI_DeviceError			0xD21B
#define PTP_DPC_FUJI_SensitivityFineTune1		0xD222 /* ???? */
#define PTP_DPC_FUJI_SensitivityFineTune2		0xD223
#define PTP_DPC_FUJI_CaptureRemaining			0xD229	/* Movie AF Mode? */
#define PTP_DPC_FUJI_MovieRemainingTime			0xD22A	/* Movie Focus Area? */
#define PTP_DPC_FUJI_ForceMode				0xD230
#define PTP_DPC_FUJI_ShutterSpeed2			0xD240 /* Movie Aperture */
#define PTP_DPC_FUJI_ImageAspectRatio			0xD241
#define PTP_DPC_FUJI_BatteryLevel			0xD242 /* Movie Sensitivity???? */
#define PTP_DPC_FUJI_TotalShotCount			0xD310
#define PTP_DPC_FUJI_HighLightTone			0xD320
#define PTP_DPC_FUJI_ShadowTone				0xD321
#define PTP_DPC_FUJI_LongExposureNR			0xD322
#define PTP_DPC_FUJI_FullTimeManualFocus		0xD323
#define PTP_DPC_FUJI_ISODialHn1				0xD332
#define PTP_DPC_FUJI_ISODialHn2				0xD333
#define PTP_DPC_FUJI_ViewMode1				0xD33F
#define PTP_DPC_FUJI_ViewMode2				0xD340
#define PTP_DPC_FUJI_DispInfoMode			0xD343
#define PTP_DPC_FUJI_LensISSwitch			0xD346
#define PTP_DPC_FUJI_FocusPoint				0xD347
#define PTP_DPC_FUJI_InstantAFMode			0xD34A
#define PTP_DPC_FUJI_PreAFMode				0xD34B
#define PTP_DPC_FUJI_CustomSetting			0xD34C
#define PTP_DPC_FUJI_LMOMode				0xD34D
#define PTP_DPC_FUJI_LockButtonMode			0xD34E
#define PTP_DPC_FUJI_AFLockMode				0xD34F
#define PTP_DPC_FUJI_MicJackMode			0xD350
#define PTP_DPC_FUJI_ISMode				0xD351
#define PTP_DPC_FUJI_DateTimeDispFormat			0xD352
#define PTP_DPC_FUJI_AeAfLockKeyAssign			0xD353
#define PTP_DPC_FUJI_CrossKeyAssign			0xD354
#define PTP_DPC_FUJI_SilentMode				0xD355
#define PTP_DPC_FUJI_PBSound				0xD356
#define PTP_DPC_FUJI_EVFDispAutoRotate			0xD358
#define PTP_DPC_FUJI_ExposurePreview			0xD359
#define PTP_DPC_FUJI_DispBrightness1			0xD35A
#define PTP_DPC_FUJI_DispBrightness2			0xD35B
#define PTP_DPC_FUJI_DispChroma1			0xD35C
#define PTP_DPC_FUJI_DispChroma2			0xD35D
#define PTP_DPC_FUJI_FocusCheckMode			0xD35E
#define PTP_DPC_FUJI_FocusScaleUnit			0xD35F
#define PTP_DPC_FUJI_SetFunctionButton			0xD361
#define PTP_DPC_FUJI_SensorCleanTiming			0xD363
#define PTP_DPC_FUJI_CustomAutoPowerOff			0xD364
#define PTP_DPC_FUJI_FileNamePrefix1			0xD365
#define PTP_DPC_FUJI_FileNamePrefix2			0xD366
#define PTP_DPC_FUJI_BatteryInfo1			0xD36A
#define PTP_DPC_FUJI_BatteryInfo2			0xD36B
#define PTP_DPC_FUJI_LensNameAndSerial			0xD36D
#define PTP_DPC_FUJI_CustomDispInfo			0xD36E
#define PTP_DPC_FUJI_FunctionLockCategory1		0xD36F
#define PTP_DPC_FUJI_FunctionLockCategory2		0xD370
#define PTP_DPC_FUJI_CustomPreviewTime			0xD371
#define PTP_DPC_FUJI_FocusArea1				0xD372
#define PTP_DPC_FUJI_FocusArea2				0xD373
#define PTP_DPC_FUJI_FocusArea3				0xD374
#define PTP_DPC_FUJI_FrameGuideGridInfo1		0xD375
#define PTP_DPC_FUJI_FrameGuideGridInfo2		0xD376
#define PTP_DPC_FUJI_FrameGuideGridInfo3		0xD377
#define PTP_DPC_FUJI_FrameGuideGridInfo4		0xD378
#define PTP_DPC_FUJI_LensUnknownData			0xD38A
#define PTP_DPC_FUJI_LensZoomPosCaps			0xD38C
#define PTP_DPC_FUJI_LensFNumberList			0xD38D
#define PTP_DPC_FUJI_LensFocalLengthList		0xD38E
#define PTP_DPC_FUJI_FocusLimiter			0xD390
#define PTP_DPC_FUJI_FocusArea4				0xD395

// WiFi opcodes, mostly from libgphoto2
#define PTP_OC_FUJI_InitiateMovieCapture		0x9020
#define PTP_OC_FUJI_TerminateMovieCapture		0x9021
#define PTP_OC_FUJI_GetCapturePreview			0x9022
#define PTP_OC_FUJI_StepZoom 0x9023
#define PTP_OC_FUJI_StartZoom 0x9024
#define PTP_OC_FUJI_StopZoom 0x9025
#define PTP_OC_FUJI_LockS1Lock			0x9026
#define PTP_OC_FUJI_UnlockS1Lock			0x9027
#define PTP_OC_FUJI_GetDeviceInfo			0x902B
#define PTP_OC_FUJI_StepShutterSpeed			0x902C
#define PTP_OC_FUJI_StepFNumber				0x902D
#define PTP_OC_FUJI_StepExposureBias		0x902E
#define PTP_OC_FUJI_CancelInitiateCapture		0x9030
#define PTP_OC_FUJI_FmSendObjectInfo			0x9040
#define PTP_OC_FUJI_FmSendObject			0x9041
#define PTP_OC_FUJI_FmSendPartialObject			0x9042

/** @}*/

#define PTP_OF_FUJI_FFF1 0xFFF1

#pragma pack(push, 1)

struct __attribute__((packed)) FujiInitPacket {
	uint32_t length;
	uint32_t type;
	uint32_t version;
	uint32_t guid1;
	uint32_t guid2;
	uint32_t guid3;
	uint32_t guid4;
	char device_name[54]; // unicode string
};

_Static_assert(sizeof(struct FujiInitPacket) == 82, "fail");

// Response to struct FujiInitPacket
struct __attribute__((packed)) PtpFujiInitResp {
	uint32_t x1;
	uint32_t x2;
	uint32_t x3;
	uint32_t x4;
	char cam_name[54];
};

_Static_assert(sizeof(struct PtpFujiInitResp) == 70, "fail");

// Appears to be an array for events
struct __attribute__((packed)) PtpFujiEvents {
	uint16_t length;
	struct PtpFujiEventsEntry {
		uint16_t code;
		uint32_t value;
	}events[];
};

// Looks very similar to standard ISO ObjectInfo, but random bits moved around.
// variable data starts at same location.
struct __attribute__((packed)) PtpFujiObjectInfo {
	uint32_t storage_id;
	uint16_t obj_format;
	uint16_t protection;
	uint32_t fuji_max_partial_size;
	uint8_t fuji_unknown2;
	uint32_t compressed_size;
	uint16_t fuji_unknown3;
	uint16_t fuji_unknown4;
	uint8_t fuji_unknown5;
	uint32_t img_width;
	uint32_t img_height;
	uint32_t img_bit_depth;
	uint32_t parent_obj;
	uint16_t assoc_type;
	uint32_t assoc_desc;
	uint32_t sequence_num;

#define PTP_FUJI_OBJ_INFO_VAR_START 52

	char filename[64];
	char date_created[32];
	char settings[32];
	char meta[32];
};

_Static_assert(sizeof(struct PtpFujiObjectInfo) == 208, "fail");

struct __attribute__((packed)) PtpFujiObjectInfoTag {
	uint32_t file_sizes[22];
};

struct __attribute__((packed)) FujiD228 {
	// @note Max length is 64.
	uint8_t length;
	// @note Last item must be 0x0
	// @note I think entries are object IDs
	uint16_t data[];
};

#pragma pack(pop)

#endif
