package info.czekanski.avocado;

import android.Manifest;
import android.content.pm.PackageManager;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL2",
                "avocado"
        };
    }

    @Override
    protected String getMainFunction() {
        return "main";
    }

    boolean hasExternalStoragePermissions() {
        return checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }
}
