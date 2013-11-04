package com.capstone.soundloop;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.NavUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

public class AddProject extends Activity {

	public static final String TAG = "AddProject";

	private ProjectsDataSource db;
	private Project project;
	private Context cntxt;
	private Intent intent;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_add_project);

		cntxt = this;

		db = new ProjectsDataSource(this);
		db.open();
		
		intent = getIntent();
		
		this.setResult(1, getIntent());
		// Edit Text 'etName': Text field for entering the name of the project
		((EditText) findViewById(R.id.etName))
				.setOnEditorActionListener(new OnEditorActionListener() {

					@Override
					public boolean onEditorAction(TextView v, int actionId,
							KeyEvent event) {

						try {
							project = submitProject();
							if (project != null) {
								intent.putExtra("id", project.getId());
								setResult(1, intent);
								finish();
							}

						} catch (Exception e) {
							Log.wtf(TAG, "failed to add to database");
							e.printStackTrace();
						}

						return false;
					}

				});
		// Submit button
		((Button) findViewById(R.id.bSubmit))
				.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						project = submitProject();
						if (project != null) {
							intent.putExtra("id", project.getId());
							setResult(1, intent);
							finish();
						}
					}
				});

		// Show the Up button in the action bar.
		getActionBar().setDisplayHomeAsUpEnabled(true);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_add_project, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:
			// This ID represents the Home or Up button. In the case of this
			// activity, the Up button is shown. Use NavUtils to allow users
			// to navigate up one level in the application structure. For
			// more details, see the Navigation pattern on Android Design:
			//
			// http://developer.android.com/design/patterns/navigation.html#up-vs-back
			//
			NavUtils.navigateUpFromSameTask(this);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	/**
	 * Submit a project based on what is in the text field
	 * 
	 * @return returns the project that is submitted or null if there is an
	 *         error.
	 */
	public Project submitProject() {

		try {
			String name = ((EditText) findViewById(R.id.etName)).getText()
					.toString();
			Log.i(TAG, name);
			return db.createProject(name);
		} catch (Exception e) {
			e.printStackTrace();
			System.err.println("no value added");
			return null;
		}
	}

}
