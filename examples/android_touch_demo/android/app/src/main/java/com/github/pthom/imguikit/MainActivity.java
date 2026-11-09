// The activity of imgui_kit's Android touch demo: a NativeActivity, plus what native code cannot do by itself
// (Dear ImGui's example_android_opengl3 does the same): the on-screen keyboard, the typed characters, and, for the
// kit, the keyboard's height. The C++ side calls these methods through JNI (main.cpp).
package com.github.pthom.imguikit;

import android.app.NativeActivity;
import android.content.Context;
import android.graphics.Insets;
import android.os.Build;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.InputMethodManager;
import java.util.concurrent.LinkedBlockingQueue;

public class MainActivity extends NativeActivity
{
    private final LinkedBlockingQueue<Integer> unicodeCharacterQueue = new LinkedBlockingQueue<>();
    private volatile int keyboardHeight = 0;

    @Override
    protected void onStart()
    {
        super.onStart();
        // The keyboard's height, in pixels, from the window's insets (API 30): the kit keeps the caret above it
        if (Build.VERSION.SDK_INT >= 30)
            getWindow().getDecorView().setOnApplyWindowInsetsListener((View view, WindowInsets insets) -> {
                Insets ime = insets.getInsets(WindowInsets.Type.ime());
                keyboardHeight = ime.bottom;
                return view.onApplyWindowInsets(insets);
            });
    }

    public void showSoftInput()
    {
        InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        manager.showSoftInput(getWindow().getDecorView(), 0);
    }

    public void hideSoftInput()
    {
        InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        manager.hideSoftInputFromWindow(getWindow().getDecorView().getWindowToken(), 0);
    }

    public int keyboardHeight()
    {
        return keyboardHeight;
    }

    // The native KeyEvent has no getUnicodeChar(): the characters are queued here, and polled by the native code
    @Override
    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (event.getAction() == KeyEvent.ACTION_DOWN)
            unicodeCharacterQueue.offer(event.getUnicodeChar(event.getMetaState()));
        return super.dispatchKeyEvent(event);
    }

    public int pollUnicodeChar()
    {
        Integer c = unicodeCharacterQueue.poll();
        return c == null ? 0 : c;
    }
}
