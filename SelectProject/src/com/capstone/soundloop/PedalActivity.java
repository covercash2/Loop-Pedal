package com.capstone.soundloop;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class PedalActivity extends Activity {

	public static final String TAG = "PedalActivity";
	public static final String TIMER_FORMAT = ":%02d / : %02d";

	private Context cntxt;

	private Toast toast;

	// UI objects
	private Button button1;
	private Button button2;
	private TextView timerView;

	private Thread timerThread;
	private Timer timer;
	
	private Object semaphore;

	// states of the recorder
	public enum State {
		PLAYING, RECORDING, STOPPED, CLEARED
	}

	private State state;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.pedal_layout);
		cntxt = this;

		button1 = (Button) findViewById(R.id.bPedal1);
		button2 = (Button) findViewById(R.id.bPedal2);
		timerView = (TextView) findViewById(R.id.tvPedal);

		state = State.CLEARED;

		button1.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				bPedalOneAction();
			}

		});

		button2.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				bPedalTwoAction();
			}

		});

		timer = new Timer();
		timerThread = new Thread(timer);
		timerThread.start();
	}

	@Override
	protected void onPause() {
		super.onPause();
		AudioEngine.shutdown();
	}

	@Override
	protected void onResume() {
		super.onResume();
		AudioEngine.startEngine();
	}

	public void bPedalOneAction() {

		Log.i(TAG, "Pedal 1 pushed");

		synchronized (timer) {

			switch (state) {

			case PLAYING:
				// change state to RECORDING
				// timer: start timer from beginning and record maxtime
				// i.e. restart
				record();
				break;
			case RECORDING:
				// change state to PLAYING
				// timer: start timer from beginning
				// i.e. reset
				play();
			case STOPPED:
				// change state to PLAYING
				// timer: start playing from current position
				// i.e. start
				play();
				break;
			case CLEARED:
				// change state to RECORDING
				// timer: start timer from beginning
				// i.e. start
				record();
				break;

			default:
				break;
			}

			semaphore.notify();

		}

	}

	public void bPedalTwoAction() {

		Log.i(TAG, "Pedal 2 pushed");

		synchronized (timer) {

			switch (state) {

			case PLAYING:
				// change state to STOPPED
				stop();
				break;
			case RECORDING:
				// change state to STOPPED
				stop();
				break;
			case STOPPED:
				// change state to CLEARED
				clear();
				break;
			case CLEARED:
				// do nothing
				break;

			default:
				break;
			}

			semaphore.notify();

		}

		 semaphore.notifyAll();
	}

	public State getState() {
		return state;
	}

	public void setState(State state) {
		this.state = state;
	}

	public void record() {
		AudioEngine.record();
		setState(State.RECORDING);
	}

	public void play() {
		AudioEngine.play();
		setState(State.PLAYING);

	}

	public void stop() {
		AudioEngine.stop();
		setState(State.STOPPED);
	}

	public void clear() {
		AudioEngine.reset();
		setState(State.CLEARED);
	}

	public void updateTime(long t) {

	}

	private class Timer implements Runnable {

		private long starttime = 0;
		private long curtime = 0;
		private long maxtime = AudioEngine.MAX_TRACK_LENGTH;

		@Override
		public void run() {

			Log.i(TAG, "Timer started");

			starttime = System.currentTimeMillis();

			synchronized (this) {

				while (true) {

					switch (state) {
					case CLEARED:
						curtime = System.currentTimeMillis() - starttime;
						try {
							Log.i(TAG, "CLEARED");
							wait();
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						break;
					case PLAYING:
						// increment the current time
						Log.i(TAG, "PLAYING");
						increment();
						break;
					case RECORDING:
						// increment the current time
						Log.i(TAG, "RECORDING");
						increment();
						break;
					case STOPPED:
						// do not increment timer
						try {
							Log.i(TAG, "STOPPED");
							wait(1000);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						break;
					default:
						curtime = 0;
						try {
							wait(1000);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						break;
					}

					timerView.post(new Runnable() {
						@Override
						public void run() {
							timerView.setText(String.format(TIMER_FORMAT,
									curtime / 1000, maxtime / 1000));
						}
					});

					Log.i(TAG, String.format(
							"(starttime, curtime, maxtime) = (%d, %d, %d)",
							starttime, curtime, maxtime));

				}

			}
		}

		public void reset() {
			if (curtime != 0) {
				curtime = 0;
			}
		}

		private void increment() {
			try {
				Thread.sleep(1000);
				curtime += System.currentTimeMillis() - starttime;
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

	}

}
