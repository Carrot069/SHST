package bio.shenhua;

import android.content.Intent;
import android.os.Build;
import android.util.Log;
import android.app.IntentService;

import java.io.*;
import java.util.ArrayList;
import android.net.Uri;
import androidx.annotation.RequiresApi;
import androidx.core.content.FileProvider;

public class ShareFileService extends IntentService implements ZipFileNotifier
{
    private static final String TAG = "ShareFileService";

    private static native void shareFinished();
    private static native void shareZipFinished();
    private static native void shareZipProgress(int done, int total);
    private static native void shareZipStart();

    public ShareFileService() {
        super("ShareFileService");
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    @Override
    protected void onHandleIntent(Intent intent) {
        boolean isZip = new String(intent.getByteArrayExtra("isZip")).equals("1");
        boolean isNoJavaShare = new String(intent.getByteArrayExtra("isNoJavaShare")).equals("1");
        //String fileName = intent.getStringExtra("fileName");
        String fileName = new String(intent.getByteArrayExtra("fileName"));
        Log.d(TAG, "onHandleIntent, fileName:" + fileName);

        ArrayList<Uri> imageUris = new ArrayList<Uri>();
        String[] files = fileName.split("\\n");
        for (String s: files) {
            Log.d(TAG, s);
            File f = new File(s);
            Uri fileUri = FileProvider.getUriForFile(this, "bio.shenhua.fileprovider", f);
            //Log.d(TAG, fileUri.toString());
            imageUris.add(fileUri);
        }

        if (imageUris.size() == 0)
            return;

        Intent sendIntent = new Intent();

        // 只有一个文件, 直接分享
        if (imageUris.size() == 1) {
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.putExtra(Intent.EXTRA_STREAM, imageUris.get(0));
            sendIntent.setType("image/*");
        }
        // 有多个文件且需要压缩
        else if (isZip) {
            //String zipFileName = intent.getStringExtra("zipFileName");
            String zipFileName = new String(intent.getByteArrayExtra("zipFileName"));
            try {
                ZipFileUtil.makeZipFile(files, zipFileName, this);
                if (isNoJavaShare) {
                    return;
                }
                Uri zipUri = FileProvider.getUriForFile(this, "bio.shenhua.fileprovider", new File(zipFileName));
                sendIntent.setAction(Intent.ACTION_SEND);
                sendIntent.putExtra(Intent.EXTRA_STREAM, zipUri);
                sendIntent.setType("application/*");
            } catch (IOException e) {
                e.printStackTrace();
            }

        }
        // 有多个文件且不需要压缩
        else {
            sendIntent.setAction(Intent.ACTION_SEND_MULTIPLE);
            sendIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, imageUris);
            sendIntent.setType("image/*");
        }

        sendIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        sendIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        Intent startIntent = Intent.createChooser(sendIntent, "分享图片");
        startIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        shareFinished();

        startActivity(startIntent);
    }

    @Override
    public void zipStart() {
        shareZipStart();
    }

    @Override
    public void zipProgress(int done, int total) {
        shareZipProgress(done, total);
    }

    @Override
    public void zipFinished() {
        shareZipFinished();
    }
}
