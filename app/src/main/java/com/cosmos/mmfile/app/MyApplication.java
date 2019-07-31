package com.cosmos.mmfile.app;

import android.app.Application;
import android.os.Environment;

import com.mm.mmfile.FileUploadConfig;
import com.mm.mmfile.IMMFileUploader;
import com.mm.mmfile.MMFileHelper;
import com.mm.mmfile.Strategy;
import com.mm.mmfile.core.FileWriteConfig;
import com.mm.mmfile.core.MMLogInfo;

import java.io.File;
import java.util.Collections;

/**
 * Created by chenwangwang on 2019-07-31.
 */
public class MyApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();

        MMFileHelper.install(
                new Strategy.Builder()
                        .businesses("performance", "errorLog")
                        .fileUploadConfig(new FileUploadConfig.Builder()
                                .uploader(
                                        new IMMFileUploader() {
                                            @Override
                                            public boolean upload(File file) {
                                                // TODO 实现提交逻辑
                                                return true;
                                            }
                                        })
                                .uploadClockTimeSeconds(5)
                                .build())
                        .fileWriteConfig(
                                new FileWriteConfig.Builder()
                                        .cacheDir(Environment.getExternalStorageDirectory().getAbsolutePath())          // 缓存目录
                                        .logDir("/sdcard/xxxAppHome/mmfile")                                            // 日志存储目录，自行调整
                                        .filePrefix(Utils.getCurrentProcessSuffix(this) + "_mmfile")             // 文件前缀(加上进程前缀和策略名称)
                                        .commonInfo(new MMLogInfo(Collections.singletonList("common"), "{\"key\":\"body\"}"))       // 公共信息
                                        .build())
                        .build()
        );
    }
}
