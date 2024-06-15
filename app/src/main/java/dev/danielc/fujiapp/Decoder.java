package dev.danielc.fujiapp;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.util.Log;

import java.nio.ByteBuffer;

// https://developer.android.com/reference/android/media/MediaCodec

public class Decoder {
    MediaCodecInfo getCodecInfo(String mime) {
        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        MediaCodecInfo infos[] = list.getCodecInfos();

        for (int i = 0; i < infos.length; i++) {
            String[] type = infos[i].getSupportedTypes();
            for (int j = 0; j < type.length; j++) {
                Log.d("decoder", type[j]);
                if (type[j].equalsIgnoreCase(mime)) {
                    return infos[i];
                }
            }
        }

        return null;
    }

    MediaCodec decoder;

    Decoder() {
        //MediaCodecInfo info = getCodecInfo("video/mjpeg");
        try {
            //decoder = MediaCodec.createByCodecName(info.getName());
            decoder = MediaCodec.createDecoderByType("video/mjpeg");

            int format = MediaCodecInfo.CodecCapabilities.COLOR_Format32bitARGB8888;

            MediaFormat mf = MediaFormat.createVideoFormat("video/mjpeg", 720, 480);
            decoder.configure(mf, null, null, 0);
            decoder.start();
        } catch (Exception e) {
            Log.e("decoder", e.toString());
        }
    }

    void inputStream(byte[] data) {
        int index = decoder.dequeueInputBuffer(10000);
        ByteBuffer buffer = decoder.getInputBuffer(index);
        buffer.clear();
        buffer.put(data);
        decoder.queueInputBuffer(index, 0, data.length, 10000, 0);
        Log.d("decoder", "queue");
    }
}
