## 简介
本仓库代码为一个接入MMFile的Demo工程

## 客户端接入

### gradle配置
目前MMFile已经支持jCenter

```
dependencies {
    implementation "com.cosmos.baseutil:mmfile:3.0.0"
}
```

**NDK中有多个架构的SO库，有必要在gradle中配置自身应用需要的SO库，减少包大小占用**

```
android {
    defaultConfig {
        ndk {
            // 设置支持的SO库架构
            abiFilters 'armeabi' // 'armeabi-v7a', 'arm64-v8a'
        }
    }
}
```

### 初始化

```
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
```

获取进程前缀的方法建议使用如下方法：

```
/**
 * 获取当前进程的后缀
 *
 * @return main主进程，否则返回子进程
 */
public static String getCurrentProcessSuffix(Context context) {
    String processName = getProcessNameInternal(context);
    if (TextUtils.equals(processName, context.getPackageName())) {
        return "main";
    } else if (processName != null && processName.contains(":")) {
        int index = processName.indexOf(":");
        if (index > 0) {
            return processName.substring(processName.indexOf(":") + 1);
        }
    }
    return "";
}

private static String getProcessNameInternal(final Context context) {
    int myPid = android.os.Process.myPid();

    if (context == null || myPid <= 0) {
        return "";
    }

    ActivityManager.RunningAppProcessInfo myProcess = null;
    ActivityManager activityManager =
            (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);

    if (activityManager != null) {
        List<ActivityManager.RunningAppProcessInfo> appProcessList = activityManager
                .getRunningAppProcesses();

        if (appProcessList != null) {
            try {
                for (ActivityManager.RunningAppProcessInfo process : appProcessList) {
                    if (process.pid == myPid) {
                        myProcess = process;
                        break;
                    }
                }
            } catch (Exception e) {
            }

            if (myProcess != null) {
                return myProcess.processName;
            }
        }
    }

    byte[] b = new byte[128];
    FileInputStream in = null;
    try {
        in = new FileInputStream("/proc/" + myPid + "/cmdline");
        int len = in.read(b);
        if (len > 0) {
            for (int i = 0; i < len; i++) { // lots of '0' in tail , remove them
                if ((((int) b[i]) & 0xFF) > 128 || b[i] <= 0) {
                    len = i;
                    break;
                }
            }
            return new String(b, 0, len);
        }

    } catch (Exception e) {
    } finally {
        closeQuietly(in);
    }

    return "";
}

public static void closeQuietly(Closeable closeable) {
    closeAllQuietly(closeable);
}

public static void closeAllQuietly(Closeable... closeable) {
    if (closeable == null) {
        return;
    }
    for (Closeable c : closeable) {
        if (c != null) {
            try {
                c.close();
            } catch (IOException ioe) {
                // ignore
            }
        }
    }
}
```

### 写入内容

```
MMFileHelper.write(
        "performance",          // 业务名称
        "log内容");                 // 内容
```

### 可能碰到的情况

- **情况一：**
如果在添加“abiFilter”之后Android Studio出现以下提示：

`NDK integration is deprecated in the current plugin.  Consider trying the new experimental plugin.`

**解决：**在项目根目录的gradle.properties文件中添加：

`android.useDeprecatedNdk=true`

- **情况二：**
如果在集成NDK后，编译时出现如下错误信息：

`More than one file was found with OS independent path 'lib/armeabi-v7a/libc++_shared.so'`

**解决**
则对NDK的依赖方式改成如下格式：

```
implementation ('com.cosmos.baseutil:mmfile:3.0.0') {
    exclude group: 'com.cosmos.baseutil', module: 'cpp_shared'
}
```

## 文件解析
文件解析支持`java`、`python`、`c++`三种语言

### Java解析
通过maven引入解析库
```
<dependency>
    <groupId>com.cosmos.baseutil</groupId>
    <artifactId>mmfile-decoder</artifactId>
    <version>1.0.0</version>
</dependency>
```

解析每一条数据

```
MMFileDecompress mmFileDecompress = new MMFileDecompress(new MMTraverseResolver() {

    @Override
    public void onDecompressError(ErrorType errorType) {
        System.out.println("onDecompressError: " + errorType.toString());
    }

    @Override
    public void onMemoryExtension(long newLength) {
        System.out.println("onMemoryExtension");
    }

    @Override
    public void fling(MMLogInfo info) {
        // TODO 处理每一条数据
    }
});

mmFileDecompress.decode("/Users/momo/Downloads/mmap/log/xlog_20181129.xlog");
```

### python解析
[python解析脚本](./decode_mm_file.py)

#### 解析单个mmfile文件

```
python decode_mm_file.py ./xxx.xlog
```

#### 解析一个目录下的所有mmfile文件

```
python decode_mm_file.py ./mmfile/
```

### C++解析

使用类似Java，见：[MMFileDecompress.h](./compression/MMFileDecompress.h)


