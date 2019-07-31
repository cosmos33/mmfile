package com.cosmos.mmfile.app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.mm.mmfile.MMFileHelper;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

MMFileHelper.write(
        "performance",          // 业务名称
        "log内容");                 // 内容

    }
}
