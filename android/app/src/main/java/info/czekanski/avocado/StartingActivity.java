package info.czekanski.avocado;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class StartingActivity extends Activity {
    private static final String TAG = StartingActivity.class.getSimpleName();
    static final int REQUEST_PERMISSION = 1;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_PERMISSION);
        } else {
            prepareEnviroment();
        }
    }

    private void copyFileOrDir(String path) {
        String[] assets = new String[]{};
        try {
            assets = getAssets().list(path);
            if (assets.length == 0) {
                copyFile(path);
            } else {
                String fullPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/avocado/" + path;
                File dir = new File(fullPath);
                if (!dir.exists()) {
                    dir.mkdir();
                }
                for (int i = 0; i < assets.length; ++i) {
                    copyFileOrDir(path + "/" + assets[i]);
                }
            }
        } catch (IOException ex) {
            Log.e(TAG, "I/O Exception", ex);
        }
    }

    private void copyFile(String filename) {
        String newFileName = Environment.getExternalStorageDirectory().getAbsolutePath() + "/avocado/" + filename;
        try (InputStream in = getAssets().open(filename);
             OutputStream out = new FileOutputStream(newFileName)) {
            byte[] buffer = new byte[16 * 1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            out.flush();
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    private void prepareEnviroment() {
        copyFileOrDir("");
        startActivity(new Intent(this, MainActivity.class));
        finish();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == REQUEST_PERMISSION && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            prepareEnviroment();
        } else {
            Toast.makeText(this, "Storage permission is required", Toast.LENGTH_SHORT).show();
            finish();
        }
    }
}