package com.steven.avgraphics.module.av;


public class FFCodec {

    public static native int _decode(String srcFile, String yuvDst, String pcmDst);

}
