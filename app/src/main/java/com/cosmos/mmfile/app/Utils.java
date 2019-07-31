package com.cosmos.mmfile.app;

import android.app.ActivityManager;
import android.content.Context;
import android.text.TextUtils;

import java.io.Closeable;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;

/**
 * Created by chenwangwang on 2019-07-31.
 */
public class Utils {

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

}
