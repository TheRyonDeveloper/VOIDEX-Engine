package com.voidex.engine;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity {

    private GLSurfaceView gameView;

    static {
        System.loadLibrary("voidex");
    }

    static native void nativeInit();

    static native void nativeResize(int width, int height);

    static native void nativeRender();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        gameView = new GLSurfaceView(this);

        gameView.setEGLContextClientVersion(3);

        gameView.setPreserveEGLContextOnPause(true);

        gameView.setRenderer(new GameRenderer());

        gameView.setRenderMode(
            GLSurfaceView.RENDERMODE_CONTINUOUSLY
        );

        setContentView(gameView);
    }

    private static class GameRenderer
            implements GLSurfaceView.Renderer {

        @Override
        public void onSurfaceCreated(
                GL10 gl,
                EGLConfig config) {

            nativeInit();
        }

        @Override
        public void onSurfaceChanged(
                GL10 gl,
                int width,
                int height) {

            nativeResize(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {

            nativeRender();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (gameView != null) {
            gameView.onPause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (gameView != null) {
            gameView.onResume();
        }
    }
  }
