--  cp wireshark_dissector.lua ~/.local/lib/wireshark/plugins/fuji.lua

local ptp = Proto("fujiptp", "Fujifilm PTP over TCP")

local ptp_codes = {
    [0x1001] = "PTP_OC_GetDeviceInfo",
    [0x1002] = "PTP_OC_OpenSession",
    [0x1003] = "PTP_OC_CloseSession",
    [0x1004] = "PTP_OC_GetStorageIDs",
    [0x1005] = "PTP_OC_GetStorageInfo",
    [0x1006] = "PTP_OC_GetNumObjects",
    [0x1007] = "PTP_OC_GetObjectHandles",
    [0x1008] = "PTP_OC_GetObjectInfo",
    [0x1009] = "PTP_OC_GetObject",
    [0x100A] = "PTP_OC_GetThumb",
    [0x100B] = "PTP_OC_DeleteObject",
    [0x100C] = "PTP_OC_SendObjectInfo",
    [0x100D] = "PTP_OC_SendObject",
    [0x100E] = "PTP_OC_InitiateCapture",
    [0x100F] = "PTP_OC_FormatStore",
    [0x1010] = "PTP_OC_ResetDevice",
    [0x1011] = "PTP_OC_SelfTest",
    [0x1012] = "PTP_OC_SetObjectProtection",
    [0x1013] = "PTP_OC_PowerDown",
    [0x1014] = "PTP_OC_GetDevicePropDesc",
    [0x1015] = "PTP_OC_GetDevicePropValue",
    [0x1016] = "PTP_OC_SetDevicePropValue",
    [0x1017] = "PTP_OC_ResetDevicePropValue",
    [0x1018] = "PTP_OC_TerminateOpenCapture",
    [0x1019] = "PTP_OC_MoveObject",
    [0x101A] = "PTP_OC_CopyObject",
    [0x101B] = "PTP_OC_GetPartialObject",
    [0x101C] = "PTP_OC_InitiateOpenCapture",

    [0x900C] = "PTP_OC_FUJI_SendObjectInfo",
    [0x900D] = "PTP_OC_FUJI_SendObject2",
    [0x901D] = "PTP_OC_FUJI_SendObject",
    [0x9020] = "PTP_OC_FUJI_InitiateMovieCapture",
    [0x9021] = "PTP_OC_FUJI_TerminateMovieCapture",
    [0x9022] = "PTP_OC_FUJI_GetCapturePreview",
    [0x9023] = "PTP_OC_FUJI_StepZoom",
    [0x9024] = "PTP_OC_FUJI_StartZoom",
    [0x9025] = "PTP_OC_FUJI_StopZoom",
    [0x9026] = "PTP_OC_FUJI_LockS1Lock",
    [0x9027] = "PTP_OC_FUJI_UnlockS1Lock",
    [0x902B] = "PTP_OC_FUJI_GetDeviceInfo",
    [0x902C] = "PTP_OC_FUJI_StepShutterSpeed",
    [0x902D] = "PTP_OC_FUJI_StepFNumber",
    [0x902E] = "PTP_OC_FUJI_StepExposureBias",
    [0x9030] = "PTP_OC_FUJI_CancelInitiateCapture",
    [0x9040] = "PTP_OC_FUJI_FmSendObjectInfo",
    [0x9041] = "PTP_OC_FUJI_FmSendObject",
    [0x9042] = "PTP_OC_FUJI_FmSendPartialObject",
    [0x9050] = "PTP_OC_FUJI_GetFilterProps",
    [0x9053] = "PTP_OC_FUJI_GetImageImportDates",
    [0x9054] = "PTP_OC_FUJI_GetExtensionObjectInfo",
    [0x9055] = "PTP_OC_FUJI_GetExtensionThumb",
    [0x9056] = "PTP_OC_FUJI_GetExtensionPartialObject",
    [0x9060] = "PTP_OC_Unknown_9060",
}

local ptp_dpc_fuji = {
    [0xd212] = "PTP_DPC_FUJI_EventsList_D212",
    [0xd220] = "PTP_DPC_FUJI_SelectedImgsMode_D220",
    [0xd222] = "PTP_DPC_FUJI_ObjectCount_D2222",
    [0xdf00] = "PTP_DPC_FUJI_CameraState_DF00",
    [0xdf01] = "PTP_DPC_FUJI_ClientState_DF01",
    [0xd226] = "PTP_DPC_FUJI_CompressSmall_D226",
    [0xd227] = "PTP_DPC_FUJI_EnableCorrectFileSize_D227",
    [0xd21c] = "PTP_DPC_FUJI_UnknownD21C",
    [0xd224] = "PTP_DPC_FUJI_Unknown_D224",
    [0xd228] = "PTP_DPC_FUJI_AutoSaveDatabaseStatus",
    [0xd22b] = "PTP_DPC_FUJI_Unknown_D22B",
    [0xd22e] = "PTP_DPC_FUJI_Unknown_D22E",
    [0xd235] = "PTP_DPC_FUJI_CompressionCutOff",
    [0xd244] = "PTP_DPC_FUJI_StorageID_D244",
    [0xd400] = "PTP_DPC_FUJI_Unknown_D400",
    [0xd401] = "PTP_DPC_FUJI_ObjectCount2",
    [0xdc04] = "PTP_DPC_FUJI_Unknown2",
    [0xd246] = "PTP_DPC_FUJI_Unknown1",
    [0xd406] = "PTP_DPC_FUJI_Unknown7",
    [0xd407] = "PTP_DPC_FUJI_Unknown8",
    [0xd500] = "PTP_DPC_FUJI_Geolocation",
    [0xd52f] = "PTP_DPC_FUJI_Unknown_D52F",
    [0xd620] = "PTP_DPC_FUJI_ImageImportObjectCount",
    [0xd621] = "PTP_DPC_FUJI_ImageImportObjectHandles",
    [0xdf21] = "PTP_DPC_FUJI_ImageGetVersion_DF21",
    [0xdf22] = "PTP_DPC_FUJI_GetObjectVersion_DF22",
    [0xdf23] = "PTP_DPC_FUJI_AutoSaveVersion_DF23",
    [0xdf24] = "PTP_DPC_FUJI_RemoteVersion_DF24",
    [0xdf25] = "PTP_DPC_FUJI_RemoteGetObjectVersion_DF25",
    [0xdf28] = "PTP_DPC_FUJI_RemotePhotoViewExVersion_DF28",
    [0xdf2a] = "PTP_DPC_FUJI_Unknown_DF2A",
    [0xdf31] = "PTP_DPC_FUJI_GeoTagVersion",
    [0xdf44] = "PTP_DPC_FUJI_Unknown_DF44",
}

local f_len   = ProtoField.uint32("ptp_tcp.length",      "Length",      base.DEC)
local f_type  = ProtoField.uint16("ptp_tcp.type",        "Type",        base.HEX)
local f_code  = ProtoField.uint16("ptp_tcp.code",        "Code",        base.HEX, ptp_codes)
local f_trans = ProtoField.uint32("ptp_tcp.transaction", "Transaction", base.HEX)
local f_param = ProtoField.uint32("ptp_tcp.param",       "Param",       base.HEX)
local f_dpc = ProtoField.uint16("ptp_tcp.dpc",         "DevicePropCode", base.HEX, ptp_dpc_fuji)
local f_data  = ProtoField.bytes ("ptp_tcp.data",        "Payload")

ptp.fields = { f_len, f_type, f_code, f_trans, f_param, f_data, f_dpc }

function ptp.dissector(buf, pinfo, tree)
    if buf:len() < 12 then return end

    pinfo.cols.protocol = "PTP/TCP"

	local code = buf(6, 2):le_uint()
	local type = buf(4, 2):le_uint()
	local offset = 12

	if type == 1 then
		local subtree = tree:add(ptp, buf(), "PTP Command Phase")
		subtree:add_le(f_len,   buf(0, 4))
		subtree:add_le(f_type,  buf(4, 2))
		subtree:add_le(f_code,  buf(6, 2))
		subtree:add_le(f_trans, buf(8, 4))

		if code == 0x1015 or code == 0x1016 then
		    subtree:add_le(f_dpc, buf(offset, 4))
		    offset = offset + 4
		else
		    if buf:len() >= 32 then
		        for i = 0, 4 do
		            subtree:add_le(f_param, buf(offset, 4)):append_text(" [" .. i .. "]")
		            offset = offset + 4
		        end
		    end
		end
	elseif type == 2 then
		local subtree = tree:add(ptp, buf(), "PTP Data Phase")
		subtree:add_le(f_len,   buf(0, 4))
		subtree:add_le(f_type,  buf(4, 2))
		subtree:add_le(f_code,  buf(6, 2))
		subtree:add_le(f_trans, buf(8, 4))

	    if buf:len() > offset then
	        subtree:add(f_data, buf(offset, buf:len() - offset))
	    end
	elseif type == 3 then
		local subtree = tree:add(ptp, buf(), "PTP Response Phase")
		subtree:add_le(f_len,   buf(0, 4))
		subtree:add_le(f_type,  buf(4, 2))
		subtree:add_le(f_code,  buf(6, 2))
		subtree:add_le(f_trans, buf(8, 4))
	end
end

DissectorTable.get("tcp.port"):add(55740, ptp)
