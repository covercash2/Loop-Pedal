package com.capstone.soundloop;

import java.util.ArrayList;

public class TrackPool {
	
	private ArrayList<Runnable> pool;

	public TrackPool() {
		// setup initial threads
		
		
	}
	
	// add a track (thread)
	public void add(Runnable thread) {
		
		pool.add(thread);
		
	}

}
