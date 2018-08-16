package com.steven.avgraphics.module.av;


public interface OnDecodeListener {

    void onImageDecoded(byte[] image);

    void onSampleDecoded(byte[] sample);

    void onDecodeEnded(boolean vsucceed, boolean asucceed);

}
