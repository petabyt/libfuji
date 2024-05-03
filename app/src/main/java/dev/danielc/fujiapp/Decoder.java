package dev.danielc.fujiapp;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;

public class Decoder {
    MediaCodecInfo getCodecInfo(String mime) {
        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        MediaCodecInfo infos[] = list.getCodecInfos();

        for (int i = 0; i < infos.length; i++) {
            String[] type = infos[i].getSupportedTypes();
            for (int j = 0; j < type.length; j++) {
                if (type[j].equalsIgnoreCase(mime)) {
                    return infos[i];
                }
            }
        }

        return null;
    }

    Decoder() {
        MediaCodecInfo info = getCodecInfo("video/mjpeg");
        try {
            MediaCodec decoder = MediaCodec.createByCodecName(info.getName());
        } catch (Exception e) {

        }
    }
}
