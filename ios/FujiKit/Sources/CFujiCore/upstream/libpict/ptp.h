/** \file */ 
// This file describes Picture Transfer Protocol (ISO 15740)
// And many vendor opcodes, property codes, and event codes.

#ifndef PTP_H
#define PTP_H

#include <stdint.h>

// PTP Packet container types
#define PTP_PACKET_TYPE_COMMAND 	0x1
#define PTP_PACKET_TYPE_DATA		0x2
#define PTP_PACKET_TYPE_RESPONSE	0x3
#define PTP_PACKET_TYPE_EVENT		0x4

// Standard USB-only packet
struct __attribute__((packed)) PtpBulkContainer {
	uint32_t length; // length of packet, in bytes
	uint16_t type; // See PACKET_TYPE_*
	uint16_t code; // See PTP_OC_*
	uint32_t transaction;

	// Parameters are only included in command packets,
	// It is typically considered a part of payload in data packets
	uint32_t params[5];

	// Payload data follows, if any
};

struct __attribute__((packed)) PtpEventContainer {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t transaction;

	uint32_t params[3];
};

struct __attribute__((packed)) PtpIpHeader {
	uint32_t length;
	uint32_t type;
	uint32_t params[3];
};

struct __attribute__((packed)) PtpIpRequestContainer {
	uint32_t length;
	uint32_t type;
	uint32_t data_phase;
	uint16_t code;
	uint32_t transaction;
	uint32_t params[5];	
};

struct __attribute__((packed)) PtpIpResponseContainer {
	uint32_t length;
	uint32_t type;
	uint16_t code;
	uint32_t transaction;
	uint32_t params[5];
};

struct __attribute__((packed)) PtpIpStartDataPacket {
	uint32_t length;
	uint32_t type;
	uint32_t transaction;
	uint64_t payload_length;
};

struct __attribute__((packed)) PtpIpEndDataPacket {
	uint32_t length;
	uint32_t type;
	uint32_t transaction;
};

struct __attribute__((packed)) PtpIpInitPacket {
	uint32_t length;
	uint32_t type;
	uint32_t guid1;
	uint32_t guid2;
	uint32_t guid3;
	uint32_t guid4;
	char device_name[8]; // Size ??
	uint16_t major_ver;
	uint16_t minor_ver;
};

// Standard PTP Operation Codes (OC)
#define PTP_OC_GetDeviceInfo		0x1001
#define PTP_OC_OpenSession			0x1002
#define PTP_OC_CloseSession			0x1003
#define PTP_OC_GetStorageIDs		0x1004
#define PTP_OC_GetStorageInfo		0x1005
#define PTP_OC_GetNumObjects		0x1006
#define PTP_OC_GetObjectHandles		0x1007
#define PTP_OC_GetObjectInfo		0x1008
#define PTP_OC_GetObject			0x1009
#define PTP_OC_GetThumb				0x100A
#define PTP_OC_DeleteObject			0x100B
#define PTP_OC_SendObjectInfo		0x100C
#define PTP_OC_SendObject			0x100D
#define PTP_OC_InitiateCapture		0x100E
#define PTP_OC_FormatStore			0x100F
#define PTP_OC_ResetDevice			0x1010
#define PTP_OC_SelfTest				0x1011
#define PTP_OC_SetObjectProtection	0x1012
#define PTP_OC_PowerDown			0x1013
#define PTP_OC_GetDevicePropDesc	0x1014
#define PTP_OC_GetDevicePropValue	0x1015
#define PTP_OC_SetDevicePropValue	0x1016
#define PTP_OC_ResetDevicePropValue	0x1017
#define PTP_OC_TerminateOpenCapture	0x1018
#define PTP_OC_MoveObject			0x1019
#define PTP_OC_CopyObject			0x101A
#define PTP_OC_GetPartialObject		0x101B
#define PTP_OC_InitiateOpenCapture	0x101C

// Media Transfer Protocol (MTP) / Microsoft codes
#define PTP_OC_MTP_GetObjectPropsSupported	0x9801
#define PTP_OC_MTP_GetObjectPropDesc		0x9802
#define PTP_OC_MTP_GetObjectPropValue		0x9803
#define PTP_OC_MTP_SetObjectPropValue		0x9804
#define PTP_OC_MTP_GetObjPropList			0x9805
#define PTP_OC_MTP_SetObjPropList			0x9806
#define PTP_OC_MTP_SendObjectPropList		0x9808
#define PTP_OC_MTP_GetObjectReferences		0x9810
#define PTP_OC_MTP_SetObjectReferences		0x9811
#define PTP_OC_MTP_UpdateDeviceFirmware		0x9812
#define PTP_OC_MTP_Skip						0x9820

#define PTP_OC_NIKON_Capture		0x90C0
#define PTP_OC_NIKON_AfCaptureSDRAM	0x90CB
#define PTP_OC_NIKON_StartLiveView	0x9201
#define PTP_OC_NIKON_EndLiveView	0x9202
#define PTP_OC_NIKON_GetEvent		0x90C7

// Non EOS (Canon point and shoot) operation codes
#define PTP_OC_CANON_ViewFinderOn		0x900B
#define PTP_OC_CANON_ViewFinderOff		0x900C
#define PTP_OC_CANON_InitCaptureInRAM	0x901A
#define PTP_OC_CANON_GetViewFinderImage	0x901D
#define PTP_OC_CANON_LockUI				0x9004
#define PTP_OC_CANON_UnlockUI			0x9005
#define PTP_OC_CANON_DoNothing			0x902F

// EOS specific
#define PTP_OC_EOS_GetStorageIDs		0x9101
#define PTP_OC_EOS_GetStorageInfo		0x9102
#define PTP_OC_EOS_GetObjectInfoEx		0x9109
#define PTP_OC_EOS_SetDevicePropValueEx	0x9110
#define PTP_OC_EOS_SetRemoteMode		0x9114
#define PTP_OC_EOS_SetEventMode			0x9115
#define PTP_OC_EOS_GetEvent				0x9116
#define PTP_OC_EOS_PCHDDCapacity		0x911A
#define PTP_OC_EOS_SetUILock			0x911B
#define PTP_OC_EOS_ResetUILock			0x911C
#define PTP_OC_EOS_KeepDeviceOn			0x911D
#define PTP_OC_EOS_UpdateFirmware		0x911F
#define PTP_OC_EOS_BulbStart			0x9125
#define PTP_OC_EOS_BulbEnd				0x9126
#define PTP_OC_EOS_GetDevicePropValue	0x9127
#define PTP_OC_EOS_RemoteReleaseOn		0x9128
#define PTP_OC_EOS_RemoteReleaseOff		0x9129
#define PTP_OC_EOS_DriveLens			0x9155
#define PTP_OC_EOS_InitiateViewfinder	0x9151
#define PTP_OC_EOS_TerminateViewfinder	0x9152
#define PTP_OC_EOS_GetViewFinderData	0x9153
#define PTP_OC_EOS_DoAutoFocus			0x9154
#define PTP_OC_EOS_AfCancel				0x9160
#define PTP_OC_EOS_SetDefaultSetting	0x91BE

#define PTP_OC_EOS_EnableEventProc		0x9050
#define PTP_OC_EOS_ExecuteEventProc		0x9052
#define PTP_OC_EOS_GetEventProcReturnData 0x9053
#define PTP_OC_EOS_IsEventProcRunning	0x9057

#define EOS_DESTINATION_CAM		0x2
#define EOS_DESTINATION_PC		0x4
#define EOS_DESTINATION_BOTH	0x6

// Return codes (RC)
#define PTP_RC_Undefined				0x2000
#define PTP_RC_OK						0x2001
#define PTP_RC_GeneralError				0x2002
#define PTP_RC_SessionNotOpen			0x2003
#define PTP_RC_InvalidTransactionID		0x2004
#define PTP_RC_OperationNotSupported	0x2005
#define PTP_RC_ParameterNotSupported	0x2006
#define PTP_RC_IncompleteTransfer		0x2007
#define PTP_RC_InvalidStorageId			0x2008
#define PTP_RC_InvalidObjectHandle		0x2009
#define PTP_RC_DevicePropNotSupported	0x200A
#define PTP_RC_InvalidObjectFormatCode	0x200B
#define PTP_RC_StoreFull				0x200C
#define PTP_RC_ObjectWriteProtected		0x200D
#define PTP_RC_StoreReadOnly			0x200E
#define PTP_RC_AccessDenied				0x200F
#define PTP_RC_NoThumbnailPresent		0x2010
#define PTP_RC_SelfTestFailed			0x2011
#define PTP_RC_PartialDeletion			0x2012
#define PTP_RC_StoreNotAvailable		0x2013
#define PTP_RC_SpecByFormatUnsupported	0x2014
#define PTP_RC_NoValidObjectInfo		0x2015
#define PTP_RC_InvalidCodeFormat		0x2016
#define PTP_RC_UnknownVendorCode		0x2017
#define PTP_RC_CaptureAlreadyTerminated	0x2018
#define PTP_RC_DeviceBusy				0x2019
#define PTP_RC_InvalidParentObject		0x201A
#define PTP_RC_InvalidDevicePropFormat	0x201B
#define PTP_RC_InvalidDevicePropValue	0x201C
#define PTP_RC_InvalidParameter			0x201D
#define PTP_RC_SessionAlreadyOpened		0x201E
#define PTP_RC_TransactionCanceled		0x201F
#define PTP_RC_SpecOfDestinationUnsupported	0x2020

// EOS or Canon?
#define PTP_RC_CANON_Unknown	0xA001
#define PTP_RC_CANON_NotReady	0xA102
#define PTP_RC_CANON_BatteryLow	0xA101

// MTP (Microsoft) extension
#define PTP_RC_UndefinedMTP				0xA800
#define PTP_RC_InvalidObjPropCode		0xA801
#define PTP_RC_InvalidObjPropCodeFormat	0xA802
#define PTP_RC_InvalidObjPropCodeValue	0xA803
#define PTP_RC_InvalidObjReference		0xA804
#define PTP_RC_InvalidDataset			0xA806
#define PTP_RC_GroupSpecUnsupported		0xA807
#define PTP_RC_DepthSpecUnsupported		0xA808
#define PTP_RC_ObjectTooLarge			0xA809
#define PTP_RC_ObjectPropUnsupported	0xA80A

// Event Codes (EC)
#define PTP_EC_Undefined			0x4000
#define PTP_EC_CancelTransaction	0x4001
#define PTP_EC_ObjectAdded			0x4002
#define PTP_EC_ObjectRemoved		0x4003
#define PTP_EC_StoreAdded			0x4004
#define PTP_EC_StoreRemoved			0x4005
#define PTP_EC_DevicePropChanged	0x4006
#define PTP_EC_ObjectInfoChanged	0x4007
#define PTP_EC_DeviceInfoChanged	0x4008
#define PTP_EC_RequestObjectTransfer 0x4009
#define PTP_EC_StoreFull			0x400A
#define PTP_EC_DeviceReset			0x400B
#define PTP_EC_StorageInfoChanged	0x400C
#define PTP_EC_CaptureComplete		0x400D
#define PTP_EC_UnreportedStatus		0x400E

#define PTP_EC_Canon_RequestObjectTransfer	0xC009

/* EOS events */
#define PTP_EC_EOS_RequestGetEvent			0xC101
#define PTP_EC_EOS_ObjectAddedEx			0xC181
#define PTP_EC_EOS_ObjectRemoved			0xC182
#define PTP_EC_EOS_RequestGetObjectInfoEx	0xC183
#define PTP_EC_EOS_StorageStatusChanged		0xC184
#define PTP_EC_EOS_StorageInfoChanged		0xC185
#define PTP_EC_EOS_RequestObjectTransfer	0xc186
#define PTP_EC_EOS_ObjectInfoChangedEx		0xC187
#define PTP_EC_EOS_ObjectContentChanged		0xC188
#define PTP_EC_EOS_PropValueChanged			0xC189
#define PTP_EC_EOS_AvailListChanged			0xC18A
#define PTP_EC_EOS_CameraStatusChanged		0xC18B
#define PTP_EC_EOS_WillSoonShutdown			0xC18D
#define PTP_EC_EOS_ShutdownTimerUpdated		0xC18E
#define PTP_EC_EOS_RequestCancelTransfer	0xC18F
#define PTP_EC_EOS_RequestObjectTransferDT	0xC190
#define PTP_EC_EOS_RequestCancelTransferDT	0xC191
#define PTP_EC_EOS_StoreAdded				0xC192
#define PTP_EC_EOS_StoreRemoved				0xC193
#define PTP_EC_EOS_BulbExposureTime			0xC194
#define PTP_EC_EOS_RecordingTime			0xC195
#define PTP_EC_EOS_RequestObjectTransferTS	0xC1A2
#define PTP_EC_EOS_AfResult					0xC1A3
#define PTP_EC_EOS_InfoCheckComplete		0xC1A4

#define PTP_EC_Nikon_ObjectAddedInSDRAM			0xC101
#define PTP_EC_Nikon_CaptureCompleteRecInSdram	0xC102

// Object Formats (OF)
#define PTP_OF_Undefined		0x3000
#define PTP_OF_Association		0x3001 // Aka Folder
#define PTP_OF_Script			0x3002
#define PTP_OF_Executable		0x3003
#define PTP_OF_Text				0x3004
#define PTP_OF_HTML				0x3005
#define PTP_OF_DPOF				0x3006
#define PTP_OF_AIFF				0x3007
#define PTP_OF_WAV				0x3008
#define PTP_OF_MP3				0x3009
#define PTP_OF_AVI				0x300A
#define PTP_OF_MPEG				0x300B
#define PTP_OF_ASF				0x300C
#define PTP_OF_MOV				0x300D
#define PTP_OF_JPEG				0x3801
#define PTP_OF_TIFF_EP			0x3802
#define PTP_OF_FlashPix			0x3803
#define PTP_OF_BMP				0x3804
#define PTP_OF_CIFF				0x3805
#define PTP_OF_Reserved2		0x3806
#define PTP_OF_GIF				0x3807
#define PTP_OF_JFIF				0x3808
#define PTP_OF_PCD				0x3809
#define PTP_OF_PICT				0x380A
#define PTP_OF_PNG				0x380B
#define PTP_OF_Reserved1		0x380C
#define PTP_OF_TIFF				0x380D
#define PTP_OF_TIFF_IT			0x380E
#define PTP_OF_JP2				0x380F
#define PTP_OF_JPX				0x3810
#define PTP_OF_Firmware			0xB802
#define PTP_OF_WIF				0xB881
#define PTP_OF_Audio			0xB900
#define PTP_OF_WMA				0xb901
#define PTP_OF_OGG				0xb902
#define PTP_OF_AAC				0xb903
#define PTP_OF_Audible			0xb904
#define PTP_OF_FLAC				0xb906
#define PTP_OF_SamsungPlaylist	0xb909
#define PTP_OF_Video			0xb980
#define PTP_OF_WMV				0xb981
#define PTP_OF_MP4				0xb982
#define PTP_OF_3GP				0xb984
#define PTP_OF_MP2				0xb983

// MTP specific Object Properties (copied from libgphoto2)
#define PTP_OPC_StorageID				0xDC01
#define PTP_OPC_ObjectFormat				0xDC02
#define PTP_OPC_ProtectionStatus			0xDC03
#define PTP_OPC_ObjectSize				0xDC04
#define PTP_OPC_AssociationType				0xDC05
#define PTP_OPC_AssociationDesc				0xDC06
#define PTP_OPC_ObjectFileName				0xDC07
#define PTP_OPC_DateCreated				0xDC08
#define PTP_OPC_DateModified				0xDC09
#define PTP_OPC_Keywords				0xDC0A
#define PTP_OPC_ParentObject				0xDC0B
#define PTP_OPC_AllowedFolderContents			0xDC0C
#define PTP_OPC_Hidden					0xDC0D
#define PTP_OPC_SystemObject				0xDC0E
#define PTP_OPC_PersistantUniqueObjectIdentifier	0xDC41
#define PTP_OPC_SyncID					0xDC42
#define PTP_OPC_PropertyBag				0xDC43
#define PTP_OPC_Name					0xDC44
#define PTP_OPC_CreatedBy				0xDC45
#define PTP_OPC_Artist					0xDC46
#define PTP_OPC_DateAuthored				0xDC47
#define PTP_OPC_Description				0xDC48
#define PTP_OPC_URLReference				0xDC49
#define PTP_OPC_LanguageLocale				0xDC4A
#define PTP_OPC_CopyrightInformation			0xDC4B
#define PTP_OPC_Source					0xDC4C
#define PTP_OPC_OriginLocation				0xDC4D
#define PTP_OPC_DateAdded				0xDC4E
#define PTP_OPC_NonConsumable				0xDC4F
#define PTP_OPC_CorruptOrUnplayable			0xDC50
#define PTP_OPC_ProducerSerialNumber			0xDC51
#define PTP_OPC_RepresentativeSampleFormat		0xDC81
#define PTP_OPC_RepresentativeSampleSize		0xDC82
#define PTP_OPC_RepresentativeSampleHeight		0xDC83
#define PTP_OPC_RepresentativeSampleWidth		0xDC84
#define PTP_OPC_RepresentativeSampleDuration		0xDC85
#define PTP_OPC_RepresentativeSampleData		0xDC86
#define PTP_OPC_Width					0xDC87
#define PTP_OPC_Height					0xDC88
#define PTP_OPC_Duration				0xDC89
#define PTP_OPC_Rating					0xDC8A
#define PTP_OPC_Track					0xDC8B
#define PTP_OPC_Genre					0xDC8C
#define PTP_OPC_Credits					0xDC8D
#define PTP_OPC_Lyrics					0xDC8E
#define PTP_OPC_SubscriptionContentID			0xDC8F
#define PTP_OPC_ProducedBy				0xDC90
#define PTP_OPC_UseCount				0xDC91
#define PTP_OPC_SkipCount				0xDC92
#define PTP_OPC_LastAccessed				0xDC93
#define PTP_OPC_ParentalRating				0xDC94
#define PTP_OPC_MetaGenre				0xDC95
#define PTP_OPC_Composer				0xDC96
#define PTP_OPC_EffectiveRating				0xDC97
#define PTP_OPC_Subtitle				0xDC98
#define PTP_OPC_OriginalReleaseDate			0xDC99
#define PTP_OPC_AlbumName				0xDC9A
#define PTP_OPC_AlbumArtist				0xDC9B
#define PTP_OPC_Mood					0xDC9C
#define PTP_OPC_DRMStatus				0xDC9D
#define PTP_OPC_SubDescription				0xDC9E
#define PTP_OPC_IsCropped				0xDCD1
#define PTP_OPC_IsColorCorrected			0xDCD2
#define PTP_OPC_ImageBitDepth				0xDCD3
#define PTP_OPC_Fnumber					0xDCD4
#define PTP_OPC_ExposureTime				0xDCD5
#define PTP_OPC_ExposureIndex				0xDCD6
#define PTP_OPC_DisplayName				0xDCE0
#define PTP_OPC_BodyText				0xDCE1
#define PTP_OPC_Subject					0xDCE2
#define PTP_OPC_Priority				0xDCE3
#define PTP_OPC_GivenName				0xDD00
#define PTP_OPC_MiddleNames				0xDD01
#define PTP_OPC_FamilyName				0xDD02
#define PTP_OPC_Prefix					0xDD03
#define PTP_OPC_Suffix					0xDD04
#define PTP_OPC_PhoneticGivenName			0xDD05
#define PTP_OPC_PhoneticFamilyName			0xDD06
#define PTP_OPC_EmailPrimary				0xDD07
#define PTP_OPC_EmailPersonal1				0xDD08
#define PTP_OPC_EmailPersonal2				0xDD09
#define PTP_OPC_EmailBusiness1				0xDD0A
#define PTP_OPC_EmailBusiness2				0xDD0B
#define PTP_OPC_EmailOthers				0xDD0C
#define PTP_OPC_PhoneNumberPrimary			0xDD0D
#define PTP_OPC_PhoneNumberPersonal			0xDD0E
#define PTP_OPC_PhoneNumberPersonal2			0xDD0F
#define PTP_OPC_PhoneNumberBusiness			0xDD10
#define PTP_OPC_PhoneNumberBusiness2			0xDD11
#define PTP_OPC_PhoneNumberMobile			0xDD12
#define PTP_OPC_PhoneNumberMobile2			0xDD13
#define PTP_OPC_FaxNumberPrimary			0xDD14
#define PTP_OPC_FaxNumberPersonal			0xDD15
#define PTP_OPC_FaxNumberBusiness			0xDD16
#define PTP_OPC_PagerNumber				0xDD17
#define PTP_OPC_PhoneNumberOthers			0xDD18
#define PTP_OPC_PrimaryWebAddress			0xDD19
#define PTP_OPC_PersonalWebAddress			0xDD1A
#define PTP_OPC_BusinessWebAddress			0xDD1B
#define PTP_OPC_InstantMessengerAddress			0xDD1C
#define PTP_OPC_InstantMessengerAddress2		0xDD1D
#define PTP_OPC_InstantMessengerAddress3		0xDD1E
#define PTP_OPC_PostalAddressPersonalFull		0xDD1F
#define PTP_OPC_PostalAddressPersonalFullLine1		0xDD20
#define PTP_OPC_PostalAddressPersonalFullLine2		0xDD21
#define PTP_OPC_PostalAddressPersonalFullCity		0xDD22
#define PTP_OPC_PostalAddressPersonalFullRegion		0xDD23
#define PTP_OPC_PostalAddressPersonalFullPostalCode	0xDD24
#define PTP_OPC_PostalAddressPersonalFullCountry	0xDD25
#define PTP_OPC_PostalAddressBusinessFull		0xDD26
#define PTP_OPC_PostalAddressBusinessLine1		0xDD27
#define PTP_OPC_PostalAddressBusinessLine2		0xDD28
#define PTP_OPC_PostalAddressBusinessCity		0xDD29
#define PTP_OPC_PostalAddressBusinessRegion		0xDD2A
#define PTP_OPC_PostalAddressBusinessPostalCode		0xDD2B
#define PTP_OPC_PostalAddressBusinessCountry		0xDD2C
#define PTP_OPC_PostalAddressOtherFull			0xDD2D
#define PTP_OPC_PostalAddressOtherLine1			0xDD2E
#define PTP_OPC_PostalAddressOtherLine2			0xDD2F
#define PTP_OPC_PostalAddressOtherCity			0xDD30
#define PTP_OPC_PostalAddressOtherRegion		0xDD31
#define PTP_OPC_PostalAddressOtherPostalCode		0xDD32
#define PTP_OPC_PostalAddressOtherCountry		0xDD33
#define PTP_OPC_OrganizationName			0xDD34
#define PTP_OPC_PhoneticOrganizationName		0xDD35
#define PTP_OPC_Role					0xDD36
#define PTP_OPC_Birthdate				0xDD37
#define PTP_OPC_MessageTo				0xDD40
#define PTP_OPC_MessageCC				0xDD41
#define PTP_OPC_MessageBCC				0xDD42
#define PTP_OPC_MessageRead				0xDD43
#define PTP_OPC_MessageReceivedTime			0xDD44
#define PTP_OPC_MessageSender				0xDD45
#define PTP_OPC_ActivityBeginTime			0xDD50
#define PTP_OPC_ActivityEndTime				0xDD51
#define PTP_OPC_ActivityLocation			0xDD52
#define PTP_OPC_ActivityRequiredAttendees		0xDD54
#define PTP_OPC_ActivityOptionalAttendees		0xDD55
#define PTP_OPC_ActivityResources			0xDD56
#define PTP_OPC_ActivityAccepted			0xDD57
#define PTP_OPC_Owner					0xDD5D
#define PTP_OPC_Editor					0xDD5E
#define PTP_OPC_Webmaster				0xDD5F
#define PTP_OPC_URLSource				0xDD60
#define PTP_OPC_URLDestination				0xDD61
#define PTP_OPC_TimeBookmark				0xDD62
#define PTP_OPC_ObjectBookmark				0xDD63
#define PTP_OPC_ByteBookmark				0xDD64
#define PTP_OPC_LastBuildDate				0xDD70
#define PTP_OPC_TimetoLive				0xDD71
#define PTP_OPC_MediaGUID				0xDD72
#define PTP_OPC_TotalBitRate				0xDE91
#define PTP_OPC_BitRateType				0xDE92
#define PTP_OPC_SampleRate				0xDE93
#define PTP_OPC_NumberOfChannels			0xDE94
#define PTP_OPC_AudioBitDepth				0xDE95
#define PTP_OPC_ScanDepth				0xDE97
#define PTP_OPC_AudioWAVECodec				0xDE99
#define PTP_OPC_AudioBitRate				0xDE9A
#define PTP_OPC_VideoFourCCCodec			0xDE9B
#define PTP_OPC_VideoBitRate				0xDE9C
#define PTP_OPC_FramesPerThousandSeconds		0xDE9D
#define PTP_OPC_KeyFrameDistance			0xDE9E
#define PTP_OPC_BufferSize				0xDE9F
#define PTP_OPC_EncodingQuality				0xDEA0
#define PTP_OPC_EncodingProfile				0xDEA1
#define PTP_OPC_BuyFlag					0xD901

// Multple vendors appear to use these
#define PTP_OF_CANON_CRW	0xb101
#define PTP_OF_RAW			0xb103
#define PTP_OF_CANON_MOV	0xb104

// Association types
#define PTP_AT_Folder	0x1
#define PTP_AT_Album	0x1

// ISO Standard Property codes
#define PTP_DPC_BatteryLevel		0x5001
#define PTP_DPC_FunctionalMode	0x5002
#define PTP_DPC_ImageSize		0x5003
#define PTP_DPC_CompressionSetting	0x5004
#define PTP_DPC_WhiteBalance		0x5005
#define PTP_DPC_RGBGain			0x5006
#define PTP_DPC_FNumber			0x5007
#define PTP_DPC_FocalLength		0x5008
#define PTP_DPC_FocalDistance	0x5009
#define PTP_DPC_FocusMode		0x500A
#define PTP_DPC_ExposureMeteringMode	0x500B
#define PTP_DPC_FlashMode		0x500C
#define PTP_DPC_ExposureTime		0x500D
#define PTP_DPC_ExposureProgramMode	0x500E
#define PTP_DPC_ExposureIndex		0x500F
#define PTP_DPC_ExposureBiasCompensation	0x5010
#define PTP_DPC_DateTime			0x5011
#define PTP_DPC_CaptureDelay		0x5012
#define PTP_DPC_StillCaptureMode	0x5013
#define PTP_DPC_Contrast		0x5014
#define PTP_DPC_Sharpness		0x5015
#define PTP_DPC_DigitalZoom		0x5016
#define PTP_DPC_EffectMode		0x5017
#define PTP_DPC_BurstNumber		0x5018
#define PTP_DPC_BurstInterval		0x5019
#define PTP_DPC_TimelapseNumber		0x501A
#define PTP_DPC_TimelapseInterval	0x501B
#define PTP_DPC_FocusMeteringMode	0x501C
#define PTP_DPC_UploadURL		0x501D
#define PTP_DPC_Artist			0x501E
#define PTP_DPC_CopyrightInfo		0x501F

#define PTP_DPC_SupportedStreams	0x5020
#define PTP_DPC_EnabledStreams		0x5021
#define PTP_DPC_VideoFormat		0x5022
#define PTP_DPC_VideoResolution		0x5023
#define PTP_DPC_VideoQuality		0x5024
#define PTP_DPC_VideoFrameRate		0x5025
#define PTP_DPC_VideoContrast		0x5026
#define PTP_DPC_VideoBrightness		0x5027
#define PTP_DPC_AudioFormat		0x5028
#define PTP_DPC_AudioBitrate		0x5029
#define PTP_DPC_AudioSamplingRate	0x502A
#define PTP_DPC_AudioBitPerSample	0x502B
#define PTP_DPC_AudioVolume		0x502C

// Canon (Not EOS) Property Codes
#define PTP_DPC_CANON_BeepCode		0xD001
#define PTP_DPC_CANON_ViewFinderMode	0xD003
#define PTP_DPC_CANON_ImageQuality	0xD006
#define PTP_DPC_CANON_ImageSize		0xD008
#define PTP_DPC_CANON_FlashMode		0xD00a
#define PTP_DPC_CANON_TvAvSetting	0xD00c
#define PTP_DPC_CANON_MeteringMode	0xd010
#define PTP_DPC_CANON_MacroMode		0xd011
#define PTP_DPC_CANON_FocusingPoint	0xd012
#define PTP_DPC_CANON_WhiteBalance	0xd013
#define PTP_DPC_CANON_AFMode			0xD015
#define PTP_DPC_CANON_Contrast		0xD017
#define PTP_DPC_CANON_ISOSpeed		0xd01c
#define PTP_DPC_CANON_Aperture		0xd01c
#define PTP_DPC_CANON_ShutterSpeed	0xd01e
#define PTP_DPC_CANON_ExpComp		0xd01f
#define PTP_DPC_CANON_Zoom			0xd02a
#define PTP_DPC_CANON_SizeQuality	0xd02c
#define PTP_DPC_CANON_FlashMemory	0xd031
#define PTP_DPC_CANON_CameraModel	0xd032
#define PTP_DPC_CANON_CameraOwner	0xd033
#define PTP_DPC_CANON_UnixTime		0xd032
#define PTP_DPC_CANON_ViewFinderOut	0xD036
#define PTP_DPC_CANON_RealImageWidth	0xD039
#define PTP_DPC_CANON_PhotoEffect	0xD040
#define PTP_DPC_CANON_AssistLight	0xD041

// EOS Device Property Codes
#define PTP_DPC_EOS_Aperture				0xD101
#define PTP_DPC_EOS_ShutterSpeed			0xD102
#define PTP_DPC_EOS_ISOSpeed				0xD103
#define PTP_DPC_EOS_ExpCompensation		0xD104
#define PTP_DPC_EOS_AutoExposureMode		0xD105
#define PTP_DPC_EOS_DriveMode			0xD106
#define PTP_DPC_EOS_MeteringMode			0xD107 
#define PTP_DPC_EOS_FocusMode			0xD108
#define PTP_DPC_EOS_WhiteBalance			0xD109
#define PTP_DPC_EOS_ColorTemperature		0xD10A
#define PTP_DPC_EOS_WhiteBalanceAdjustA	0xD10B
#define PTP_DPC_EOS_WhiteBalanceAdjustB	0xD10C
#define PTP_DPC_EOS_WhiteBalanceXA		0xD10D
#define PTP_DPC_EOS_WhiteBalanceXB		0xD10E
#define PTP_DPC_EOS_ColorSpace			0xD10F
#define PTP_DPC_EOS_PictureStyle			0xD110
#define PTP_DPC_EOS_BatteryPower			0xD111
#define PTP_DPC_EOS_BatterySelect		0xD112
#define PTP_DPC_EOS_CameraTime			0xD113
#define PTP_DPC_EOS_Owner				0xD115
#define PTP_DPC_EOS_ModelID				0xD116
#define PTP_DPC_EOS_PTPExtensionVersion	0xD119
#define PTP_DPC_EOS_DPOFVersion			0xD11A
#define PTP_DPC_EOS_AvailableShots		0xD11B
#define PTP_DPC_EOS_CaptureDestination	0xD11C
#define PTP_DPC_EOS_CurrentFolder		0xD11F
#define PTP_DPC_EOS_ImageFormat			0xD120
#define PTP_DPC_EOS_ImageFormatCF		0xD121
#define PTP_DPC_EOS_ImageFormatSD		0xD122
#define PTP_DPC_EOS_ImageFormatExtHD		0xD123
#define PTP_DPC_EOS_AEModeDial			0xD138
#define PTP_DPC_EOS_ShutterCounter 		0xD1AC
#define PTP_DPC_EOS_VF_Output			0xD1B0
#define PTP_DPC_EOS_EVFMode				0xD1B1
#define PTP_DPC_EOS_DOFPreview			0xD1B2
#define PTP_DPC_EOS_VFSharp				0xD1B3
#define PTP_DPC_EOS_EVFWBMode			0xD1B4
#define PTP_DPC_EOS_FocusInfoEx			0xD1D3

// Magic Lantern ptpview old opcodes
#define PTP_OC_ML_LiveBmpRam 0x9996
#define PTP_OC_ML_Live360x240 0x9997

#define PTP_OC_MagicLantern	0x9998
#define PTP_OC_CHDK			0x9999

// Storage Types
#define PTP_ST_Undefined	0x0
#define PTP_ST_FixedROM		0x1
#define PTP_ST_RemovableROM	0x2
#define PTP_ST_FixedRAM		0x3
#define PTP_ST_RemovableRAM	0x4

// Filesystem Type
#define PTP_FT_Undefined	0x0
#define PTP_FT_GenericFlat	0x1
#define PTP_FT_GenericHei	0x2
#define PTP_FT_DCF			0x3

// Access Capability
#define PTP_AC_ReadWrite	0x0
#define PTP_AC_Read			0x1
#define PTP_AC_ReadDelete	0x2

// Device type codes
#define PTP_TC_UNDEF	0x0
#define PTP_TC_INT8		0x1
#define PTP_TC_UINT8	0x2
#define PTP_TC_INT16	0x3
#define PTP_TC_UINT16	0x4
#define PTP_TC_INT32	0x5
#define PTP_TC_UINT32	0x6
#define PTP_TC_INT64	0x7
#define PTP_TC_UINT64	0x8
#define PTP_TC_INT128	0x9
#define PTP_TC_UINT128	0xA
#define PTP_TC_UINT8ARRAY	0x4002
#define PTP_TC_UINT16ARRAY	0x4004
#define PTP_TC_UINT32ARRAY	0x4006
#define PTP_TC_UINT64ARRAY	0x4008
#define PTP_TC_STRING		0xFFFF

#define PTP_NoForm 0x0
#define PTP_RangeForm	0x1
#define PTP_EnumerationForm	0x2
#define PTP_DateTimeForm	0x3
#define PTP_FixedLengthArrayForm	0x4
#define PTP_RegularExpressionForm	0x5
#define PTP_ByteArrayForm	0x6
#define PTP_LongStringForm	0xFF

#define PTP_WB_Undefined 0x0
#define PTP_WB_Manual 0x1
#define PTP_WB_Automatic 0x2
#define PTP_WB_Daylight 0x4
#define PTP_WB_Florescent 0x5
#define PTP_WB_Tungsten 0x5
#define PTP_WB_Flash 0x7

// Used for socket initialization
#define PTPIP_INIT_COMMAND_REQ	0x1
#define PTPIP_INIT_COMMAND_ACK	0x2
#define PTPIP_INIT_EVENT_REQ	0x3
#define PTPIP_INIT_EVENT_ACK	0x4

// Packet type extensions for PTP/IP
#define PTPIP_INIT_FAIL			0x5
#define PTPIP_COMMAND_REQUEST	0x6
#define PTPIP_COMMAND_RESPONSE	0x7
#define PTPIP_EVENT				0x8
#define PTPIP_DATA_PACKET_START	0x9
#define PTPIP_DATA_PACKET		0xA
#define PTPIP_CANCEL_TRANSACTION	0xB
#define PTPIP_DATA_PACKET_END	0xC
#define PTPIP_PING				0xD
#define PTPIP_PONG				0xE

#define USB_VENDOR_CANON 0x4A9

// ISO number for PTP/IP
#define PTP_IP_PORT 15740

// Standard interface Class ID for PTP.
// See https://en.wikipedia.org/wiki/USB#Device_classes
#define PTP_CLASS_ID 6

// bRequest codes
#define MTP_REQ_CANCEL              0x64
#define MTP_REQ_GET_EXT_EVENT_DATA  0x65
#define USB_REQ_RESET			0x66
#define USB_REQ_STATUS			0x67
#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE	0x01
#define USB_REQ_SET_FEATURE		0x03

// Data phase host to device, device to host
#define USB_DP_HTD				0x0
#define USB_DP_DTH				0x80

#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#ifndef USB_TYPE_CLASS
#define USB_TYPE_CLASS 0x20
#endif

#endif
