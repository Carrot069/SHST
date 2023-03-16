package bio.shenhua;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.app.Service;
import android.os.IBinder;

import android.content.Context;
import android.net.*;
import android.net.wifi.WifiNetworkSpecifier;

public class WiFiService extends Service
{
    private static native void sendToQt(String message);
    private static final String TAG = "WiFiService";

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Creating Service");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Destroying Service");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        int ret = super.onStartCommand(intent, flags, startId);
        String wifiName = new String(intent.getByteArrayExtra("wifiName"));
        String wifiPassword = new String(intent.getByteArrayExtra("wifiPassword"));

        WifiNetworkSpecifier.Builder builder = new WifiNetworkSpecifier.Builder();
        builder.setSsid(wifiName);
        builder.setWpa2Passphrase(wifiPassword);

        WifiNetworkSpecifier wifiNetworkSpecifier = builder.build();
        NetworkRequest.Builder networkRequestBuilder = new NetworkRequest.Builder();
        networkRequestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        networkRequestBuilder.setNetworkSpecifier(wifiNetworkSpecifier);
        NetworkRequest networkRequest = networkRequestBuilder.build();
        final ConnectivityManager cm = (ConnectivityManager)
                getBaseContext().getApplicationContext()
                        .getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm != null) {
            cm.requestNetwork(networkRequest, new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    super.onAvailable(network);
                    cm.bindProcessToNetwork(network);
                }
            });
        }

        return ret;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
