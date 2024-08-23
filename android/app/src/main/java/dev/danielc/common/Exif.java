package dev.danielc.common;

public class Exif {
    /// @brief returns byte[] to JPEG thumb or NULL
    public static native byte[] getThumbnail(String filepath);
    public static native String getInformationJSON(String path);
}
