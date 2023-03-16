package bio.shenhua;

import android.os.Build;
import android.util.Log;
import androidx.annotation.RequiresApi;

import java.io.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class ZipFileUtil {
    private static final String TAG = "ZipFileUtil";

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    public static void makeZipFile(String[] srcFiles, String zipFileName, ZipFileNotifier notifier) throws IOException {
        Log.i(TAG, "makeZipFile");
        FileOutputStream zipFileOutputStream = new FileOutputStream(new File(zipFileName));
        try (ZipOutputStream zip2 = new ZipOutputStream(zipFileOutputStream)) {
            notifier.zipStart();
            try {
                zip2.setLevel(0);
                int done = 0;
                for (String fileName : srcFiles) {
                    Log.i(TAG, fileName);
                    File f = new File(fileName);
                    zip2.putNextEntry(new ZipEntry(f.getName()));
                    zip2.write(getFileDataAsBytes(f));
                    zip2.closeEntry();
                    done++;
                    notifier.zipProgress(done, srcFiles.length);
                }
            } finally {
                notifier.zipFinished();
            }
        }
        Log.i(TAG, "makeZipFile, end");
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    private static byte[] getFileDataAsBytes(File f) throws IOException {
        byte[] data;
        try (InputStream input = new FileInputStream(f); ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            int n;
            while ((n = input.read()) != -1) {
                out.write(n);
            }
            data = out.toByteArray();
        }
        return data;
    }
}
