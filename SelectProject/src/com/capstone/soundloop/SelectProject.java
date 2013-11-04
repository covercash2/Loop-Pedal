package com.capstone.soundloop;

import java.util.List;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class SelectProject extends ListActivity implements OnClickListener {

	public static final String TAG = "SelectProject";

	private ProjectsDataSource dataSource;
	private List<Project> values;
	private ArrayAdapter<Project> adapter;
	private Project this_project;
	
	private long id = 0;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_select_project);

		// Instantiate and open the database
		dataSource = new ProjectsDataSource(this);
		dataSource.open();

		// get values from the database
		values = dataSource.getAllProjects();

		// initialize 'this_project' if there are projects in the db
		this_project = new Project();
		if (!values.isEmpty())
			this_project = values.get(0);
		
		Log.i(TAG, this_project.toString());

		// Array adapter to show elements in the ListView
		adapter = new ArrayAdapter<Project>(this,
				android.R.layout.simple_list_item_1, values);
		setListAdapter(adapter);

	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {

		Bundle bundle = new Bundle();
		bundle.putLong("id", position);

//		Intent i = new Intent(this, LoopPedal.class);
//		i.putExtras(bundle);
//		startActivity(i);
		
		Intent i = new Intent("com.capstone.soundloop.PEDALACTIVITY");
		i.putExtras(bundle);
		startActivity(i);
		
	}

	@Override
	protected void onResume() {
		super.onResume();
		dataSource.open();
		values = dataSource.getAllProjects();
		adapter.notifyDataSetChanged();
	}

	@Override
	protected void onPause() {
		dataSource.close();
		super.onPause();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_select_project, menu);
		return true;
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		
		dataSource.open();
		
		if(data == null) 
			Log.wtf(TAG, "what the fuck");
		
		if(data != null) {
			Log.i(TAG, data.toString());
			id = data.getLongExtra("id", 0);
			this_project = dataSource.getProject(id);
			if(id != 0) {
				adapter.add(this_project);
			}
			this_project = adapter.getItem(0);
		}
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.add:
			// start intent to add projects
			Intent i = new Intent("com.capstone.soundloop.ADDPROJECT");
			startActivityForResult(i, 1);
			break;
		case R.id.delete:
			// delete first value for now
			if(!adapter.isEmpty()) {
				dataSource.deleteProject(this_project);
				adapter.remove(this_project);
				if(!adapter.isEmpty())
					this_project = adapter.getItem(0);
			}
			break;
		}
	}

}
