/**
 * AudioEngine.java
 * 	A java wrapper for a set of JNI functions for audio 
 * recording and playback.
 */
package com.capstone.soundloop;

public class AudioEngine {
	
	public static final long MAX_TRACK_LENGTH = 30000; // in milliseconds

	public static void startEngine() {
		try {
			createEngine();
		} catch (Exception e) {
			System.err.println("error creating the engine");
			e.printStackTrace();
		}
		try {
			createPlayer();
		} catch (Exception e) {
			System.err.println("error creating the player");
			e.printStackTrace();
		}
		try {
			createRecorder();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public String toString() {
		return "AudioEngine";
	}

	/** Native methods implemented in jni folder */

	public static native void createEngine();

	public static native void createPlayer();

	public static native void createRecorder();

	public static native void record();

	public static native void stopRecording();

	public static native void play();

	public static native void stopPlaying();

	public static native void stop();

	public static native void reset();

	public static native void shutdown();

	static {
		System.loadLibrary("soundloop");
	}

}
