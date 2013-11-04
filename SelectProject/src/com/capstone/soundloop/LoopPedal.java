package com.capstone.soundloop;

import java.util.List;
import java.util.Timer;
import java.util.concurrent.BlockingQueue;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.database.sqlite.SQLiteException;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.format.Time;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Chronometer;
import android.widget.Chronometer.OnChronometerTickListener;
import android.widget.TextView;

public class LoopPedal extends Activity {
	
	public static final String TAG = "LoopPedal";

	private List<Project> projects;
	private Project project;
	private ProjectsDataSource dataSource;
	private boolean recording = false;
	private boolean playing = false;
	private boolean stopped = false;

	private TrackPool pool;
	private final int init_poolSize = 2;
	private final int max_poolSize = 10;
	private final long thread_time = 10000L;
	private BlockingQueue<Runnable> workQueue;

	private Timer timer;
	private Time time;
	private Chronometer chronometer;
	private int record_time = 0; /* in seconds */

	private String format = "";

	private BrutalListener listener;

	/** Required onCreate method called when Activity is started */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.loop_pedal);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

		// setup thread pool



		// setup a timer
		chronometer = (Chronometer) findViewById(R.id.cmCounter);
		time = new Time();
		time.format(format);

		listener = new BrutalListener();

//		chronometer.setOnChronometerTickListener(listener);
//		chronometer.setFormat("%S / 00:00");

		System.out.println(chronometer.toString());

		// initialize audio system
//		try { // create endine
//			createEngine();
//		} catch (Exception e) {
//			System.err.println("Error in createEngine");
//		}
//		try { // create player
//			createPlayer();
//		} catch (Exception e) {
//			System.err.println("Error in createPlayer");
//		}
//		try { // create recorder
//			createRecorder();
//		} catch (Exception e) {
//			System.err.println("Error in createRecorder");
//		}

		// open database
		try {
			dataSource = new ProjectsDataSource(this);
			dataSource.open();
		} catch (SQLiteException e) {
			System.err.println("Error opening database");
		}

		// get info from last activity
		long id = 0;
		try {
			Bundle bundle = getIntent().getExtras();
			id = (int) bundle.getLong("id");
		} catch (Exception e) {
			System.err.println("Error loading bundle");
			e.printStackTrace();
		}

		Log.i(TAG, "id = " + id);
		
		// get project from database
		try {
			project = dataSource.getProjectAt(id);
			dataSource.close();
		} catch (SQLiteException e) {
			System.err.println("Error loading project from database");
			e.printStackTrace();
		}

		// create a label for the pedal
		TextView name = (TextView) findViewById(R.id.project_name);
		// name.setText(project.getName() + "  " + project.getId());
		name.setText("LoopPedal");

		// the record/playback button
//		((Button) findViewById(R.id.bRecord))
//				.setOnClickListener(new OnClickListener() {
//
//					@Override
//					public void onClick(View v) {
//						if (recording || stopped) {
//							// stop recording and playback
//							JNI_play();
//						} else {
//							// record
//							JNI_record();
//						}
//					}
//
//				});

		// the stop/reset button
//		((Button) findViewById(R.id.bStop))
//				.setOnClickListener(new OnClickListener() {
//
//					@Override
//					public void onClick(View v) {
//						if (stopped) {
//							// reset
//							JNI_reset();
//						} else {
//							// stop
//							JNI_stop();
//						}
//					}
//
//				});

	}

	/** Called when the activity is about to be destroyed. */
//	@Override
//	protected void onPause() {
//		stop();
//		super.onPause();
//	}

	/** Called when the activity is about to be destroyed. */
//	@Override
//	protected void onDestroy() {
//		// shutdown();
//		super.onDestroy();
//	}

	public void updateText(String text, int id) {
		((TextView) findViewById(id)).setText(text);
	}

	// update the right portion of the counter
	public void updateTime() {
		listener.reset();
		time.set(record_time * 1000);
		format = "%s / " + time.format("%M:%S");
		chronometer.setFormat(format);
		chronometer.setBase(SystemClock.elapsedRealtime());
	}

	private class BrutalListener implements OnChronometerTickListener {
		private int ticks = 0;

		@Override
		public void onChronometerTick(Chronometer chr) {
			ticks++;
			if (!recording && (ticks >= record_time)) {
				reset();
			}
		}

		public int getTicks() {
			// compensate for extra ticks
			return ticks;
		}

		public void reset() {
			ticks = 0;
		}

	}

	/*----------------WRAPPER FUNCTIONS----------------*/
//	public void JNI_record() {
//		record();
//		chronometer.stop();
//		listener.reset();
//		updateTime();
//		chronometer.setBase(SystemClock.elapsedRealtime());
//		chronometer.start();
//		recording = true;
//		playing = false;
//		stopped = false;
//		record_time = 0;
//
//		updateText("Play", R.id.bRecord);
//		updateText("Stop", R.id.bStop);
//	}

//	public void JNI_play() {
//		play();
//		record_time = listener.getTicks();
//		updateTime();
//		chronometer.stop();
//		listener.reset();
//		chronometer.setBase(SystemClock.elapsedRealtime());
//		chronometer.start();
//		recording = false;
//		playing = true;
//		stopped = false;
//
//		updateText("Record", R.id.bRecord);
//		updateText("Stop", R.id.bStop);
//	}

//	public void JNI_stop() {
//		if (!stopped) {
//			stop();
//			stopped = true;
//			chronometer.stop();
//
//			updateText("Play", R.id.bRecord);
//			updateText("Reset", R.id.bStop);
//		}
//	}

//	public void JNI_reset() {
//		reset();
//		record_time = 0;
//		chronometer.stop();
//		listener.reset();
//		recording = false;
//		playing = false;
//		stopped = false;
//		updateTime();
//		chronometer.setBase(SystemClock.elapsedRealtime());
//
//		updateText("Record", R.id.bRecord);
//		updateText("Stop", R.id.bStop);
//	}

	/** Native methods implemented in jni folder */

//	public static native void createEngine();
//
//	public static native void createPlayer();
//
//	public static native void createRecorder();
//
//	public static native void record();
//
//	public static native void play();
//
//	public static native void stop();
//
//	public static native void reset();
//
//	static {
//		System.loadLibrary("soundloop");
//	}

}
