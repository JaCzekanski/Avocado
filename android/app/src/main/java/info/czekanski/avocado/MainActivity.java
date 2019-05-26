package info.czekanski.avocado;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL2",
                "avocado"
        };
    }

    @Override
    protected String getMainFunction() {
        return "main";
    }
}