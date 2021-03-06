package com.cnk.communication;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.cnk.communication.task.ExhibitDownloadTask;
import com.cnk.communication.task.ExperimentDataDownloadTask;
import com.cnk.communication.task.MapDownloadTask;
import com.cnk.communication.task.RaportUploadTask;
import com.cnk.communication.task.ServerTask;
import com.cnk.communication.task.Task;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class NetworkHandler {

    public interface SuccessAction {
        void perform(Task sender);
    }

    public interface FailureAction {
        void perform(Task sender, ServerTask.FailureReason reason);
    }

    private static final String LOG_TAG = "NetworkHandler";
    private static final long SECONDS_DELAY = 30;
    private static NetworkHandler instance;

    private ScheduledExecutorService scheduledExecutor;
    private boolean bgSyncStarted = false;
    private Context appContext;

    private NetworkHandler() {
        scheduledExecutor = Executors.newSingleThreadScheduledExecutor();
    }

    public static NetworkHandler getInstance() {
        if (instance == null) {
            instance = new NetworkHandler();
        }
        return instance;
    }

    public void setAppContext(Context appContext) {
        this.appContext = appContext;
    }

    public boolean isConnectedToWifi() {
        ConnectivityManager connManager =
                (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo mWifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

        return mWifi.isConnected();
    }

    public synchronized void downloadExperimentData(SuccessAction success, FailureAction failure) {
        Task task = new ExperimentDataDownloadTask(success, failure);
        scheduledExecutor.schedule(task::run, 0, TimeUnit.SECONDS);
    }

    public synchronized void downloadMap(SuccessAction success, FailureAction failure) {
        Task task = new MapDownloadTask(success, failure);
        scheduledExecutor.schedule(task::run, 0, TimeUnit.SECONDS);
    }

    public synchronized void startBgDataSync() {
        if (bgSyncStarted) {
            return;
        }
        bgSyncStarted = true;

        Task exhibitsTask = new ExhibitDownloadTask(null, null);
        Task raportsTask = new RaportUploadTask(null, null);
        scheduledExecutor.scheduleWithFixedDelay(exhibitsTask::run,
                                                 SECONDS_DELAY,
                                                 SECONDS_DELAY,
                                                 TimeUnit.SECONDS);
        scheduledExecutor.scheduleWithFixedDelay(raportsTask::run,
                                                 SECONDS_DELAY,
                                                 SECONDS_DELAY,
                                                 TimeUnit.SECONDS);
    }
}
