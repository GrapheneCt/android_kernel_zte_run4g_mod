package com.discretix.drmactions;

import android.content.Context;
import android.drm.DrmManagerClient;
import android.util.Log;

import com.discretix.drmactions.auxillary.Utilities;
import com.discretix.drmassist.DrmActionSync;

/**
 * Remove rights for a current file
 */
public class RemoveExpiredRights extends DrmActionSync {

	/*
	 * (non-Javadoc)
	 * @see com.discretix.drmassist.DrmActionBase#getName()
	 */
	public String getName() {
		return "Remove Expired Rights";
	}

	/*
	 * (non-Javadoc)
	 * @see com.discretix.drmassist.DrmActionBase#isSupported(java.lang.String, java.lang.String)
	 */
	public boolean isSupported(String extension, String filePath) {
		return Utilities.isPlayreadyDrm(extension);
	}
	
	/*
	 * (non-Javadoc)
	 * @see com.discretix.drmassist.DrmActionSync#perform(android.content.Context, java.lang.String, java.lang.String)
	 */
	protected String perform(Context context, String filePath, String extension) {
		
		DrmManagerClient drmClient = new DrmManagerClient(context);
		
		int status = drmClient.removeRights("playready://AllExpiredLicenses.playready");
		Log.d("RemoveRights", "DrmManagerClient::removeRights(Expired) returned " + Utilities.drmClientStatus2Str(status));
		
		return (DrmManagerClient.ERROR_NONE == status)? "SUCCESS" : Utilities.drmClientStatus2Str(status);
	}

}
