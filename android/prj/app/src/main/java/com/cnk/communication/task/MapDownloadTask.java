package com.cnk.communication.task;

import android.util.Log;

import com.cnk.communication.MapImagesRequest;
import com.cnk.communication.MapImagesResponse;
import com.cnk.communication.Server;
import com.cnk.data.DataHandler;
import com.cnk.notificators.Notificator;

import org.apache.thrift.TException;

import java.io.IOException;
import java.util.Map;

public class MapDownloadTask extends ServerTask {

    private static final String LOG_TAG = "MapDownloadTask";

    public MapDownloadTask(Notificator notificator) {
        super(notificator);
    }

    public void performInSession(Server.Client client) throws TException {
        Log.i(LOG_TAG, "Downloading map");
        MapImagesRequest request = new MapImagesRequest();
        if (DataHandler.getInstance().getMapVersion() != null) {
            request.setAcquiredVersion(DataHandler.getInstance().getMapVersion());
        }
        MapImagesResponse response = client.getMapImages(request);
        updateDataHandler(response);
        Log.i(LOG_TAG, "Map downloaded");
    }

    private void updateDataHandler(MapImagesResponse response) {
        DataHandler.getInstance().setMapVersion(response.version);
        for (Map.Entry<Integer, String> entry : response.levelImageUrls.entrySet()) {
            try {
                DataHandler.getInstance().setMapFloor(entry.getKey(), entry.getValue());
            } catch (IOException e) {
                Log.e(LOG_TAG, "cant set floor");
                e.printStackTrace();
            }
        }
    }
}
