package com.steven.avgraphics.ui;

import android.os.Bundle;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.HWCodec;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HWCodecActivity extends BaseActivity {

    private Button mBtnRecord;
    private Button mBtnDecode;
    private Button mBtnTranscode;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_hwcodec);
        init();
    }

    private void init() {
        findView();
        setListener();
    }

    private void findView() {
        mBtnRecord = findViewById(R.id.hwcodec_btn_record);
        mBtnDecode = findViewById(R.id.hwcodec_btn_decode);
        mBtnTranscode = findViewById(R.id.hwcodec_btn_transcode);
    }

    private void setListener() {
        mBtnRecord.setOnClickListener(v -> startActivity(HWRecordActivity.class));
        mBtnDecode.setOnClickListener(v -> decode());
        mBtnTranscode.setOnClickListener(v -> transcode());
    }

    private void decode() {
        if (!new File(Utils.getHWRecordOutput()).exists()) {
            ToastHelper.show("无视频，请先进行录制");
            return;
        }
        ExecutorService executorService = Executors.newCachedThreadPool();
        executorService.execute(() ->
                HWCodec.decodeVideo(Utils.getHWRecordOutput(), Utils.getHWDecodeYuvOutput()));
        executorService.execute(() ->
                HWCodec.decodeAudio(Utils.getHWRecordOutput(), Utils.getHWDecodePcmOutput()));
    }

    private void transcode() {
        if (!new File(Utils.getHWRecordOutput()).exists()) {
            ToastHelper.show("无视频，请先进行录制");
            return;
        }
        Executors.newSingleThreadExecutor().execute(() ->
                HWCodec.transcode(Utils.getHWRecordOutput(), Utils.getHWTranscodeOutput()));
    }

}
