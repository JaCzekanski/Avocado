package info.czekanski.avocado;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

public class StartingActivity extends Activity {
    private List<String> dataDirs = Arrays.asList("bios", "memory", "iso", "exe");

    static final int REQUEST_PERMISSION = 1;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_PERMISSION);
            return;
        }

        prepareEnviroment();
    }

    private void prepareEnviroment() {
        Path sdcard = Environment.getExternalStorageDirectory().toPath();
        Path avocadoData = sdcard.resolve("avocado").resolve("data");

        for (String dir : dataDirs) {
            avocadoData.resolve(dir).toFile().mkdirs();
        }

        runAvocado();
    }

    private void runAvocado() {
        startActivity(new Intent(this, MainActivity.class));
        finish();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == REQUEST_PERMISSION) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                prepareEnviroment();
            } else {
                Toast.makeText(this, "Storage permission is required", Toast.LENGTH_SHORT).show();
                runAvocado();
            }
        }
    }
}