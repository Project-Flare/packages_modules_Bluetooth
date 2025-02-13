/*
 * Copyright (C) 2015 Samsung System LSI
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.bluetooth.map;

import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothProtoEnums;
import android.util.Log;
import android.util.Xml;

import com.android.bluetooth.BluetoothStatsLog;
import com.android.bluetooth.Utils;
import com.android.bluetooth.content_profiles.ContentProfileErrorReportUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.io.InputStream;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

// Next tag value for ContentProfileErrorReportUtils.report(): 3
public class BluetoothMapConvoListing {
    private boolean mHasUnread = false;
    private static final String TAG = "BluetoothMapConvoListing";
    private static final String XML_TAG = "MAP-convo-listing";

    private List<BluetoothMapConvoListingElement> mList;

    public BluetoothMapConvoListing() {
        mList = new ArrayList<BluetoothMapConvoListingElement>();
    }

    public void add(BluetoothMapConvoListingElement element) {
        mList.add(element);
        /* update info regarding whether the list contains unread conversations */
        if (element.getReadBool()) {
            mHasUnread = true;
        }
    }

    /**
     * Used to fetch the number of BluetoothMapConvoListingElement elements in the list.
     *
     * @return the number of elements in the list.
     */
    public int getCount() {
        if (mList != null) {
            return mList.size();
        }
        return 0;
    }

    /**
     * does the list contain any unread messages
     *
     * @return true if unread messages have been added to the list, else false
     */
    public boolean hasUnread() {
        return mHasUnread;
    }

    /**
     * returns the entire list as a list
     *
     * @return list
     */
    public List<BluetoothMapConvoListingElement> getList() {
        return mList;
    }

    /**
     * Encode the list of BluetoothMapMessageListingElement(s) into a UTF-8 formatted XML-string in
     * a trimmed byte array
     *
     * @return a reference to the encoded byte array.
     */
    public byte[] encode() {
        StringWriter sw = new StringWriter();
        XmlSerializer xmlConvoElement = Xml.newSerializer();
        try {
            xmlConvoElement.setOutput(sw);
            xmlConvoElement.startDocument("UTF-8", true);
            xmlConvoElement.setFeature(
                    "http://xmlpull.org/v1/doc/features.html#indent-output", true);
            xmlConvoElement.startTag(null, XML_TAG);
            xmlConvoElement.attribute(null, "version", "1.0");
            // Do the XML encoding of list
            for (BluetoothMapConvoListingElement element : mList) {
                element.encode(xmlConvoElement); // Append the list element
            }
            xmlConvoElement.endTag(null, XML_TAG);
            xmlConvoElement.endDocument();
        } catch (IllegalArgumentException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.MAP,
                    BluetoothProtoEnums.BLUETOOTH_MAP_CONVO_LISTING,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    0);
            Log.w(TAG, e);
        } catch (IllegalStateException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.MAP,
                    BluetoothProtoEnums.BLUETOOTH_MAP_CONVO_LISTING,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    1);
            Log.w(TAG, e);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.MAP,
                    BluetoothProtoEnums.BLUETOOTH_MAP_CONVO_LISTING,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    2);
            Log.w(TAG, e);
        }
        return sw.toString().getBytes(StandardCharsets.UTF_8);
    }

    public void sort() {
        Collections.sort(mList);
    }

    public void segment(int count, int offset) {
        count = Math.min(count, mList.size() - offset);
        if (count > 0) {
            mList = mList.subList(offset, offset + count);
            if (mList == null) {
                mList = new ArrayList<BluetoothMapConvoListingElement>(); // Return an empty list
            }
        } else {
            if (offset > mList.size()) {
                mList = new ArrayList<BluetoothMapConvoListingElement>();
                Log.d(TAG, "offset greater than list size. Returning empty list");
            } else {
                mList = mList.subList(offset, mList.size());
            }
        }
    }

    public void appendFromXml(InputStream xmlDocument)
            throws XmlPullParserException, IOException, ParseException {
        try {
            XmlPullParser parser = Xml.newPullParser();
            int type;
            parser.setInput(xmlDocument, "UTF-8");

            // First find the folder-listing
            while ((type = parser.next()) != XmlPullParser.END_TAG
                    && type != XmlPullParser.END_DOCUMENT) {
                // Skip until we get a start tag
                if (parser.getEventType() != XmlPullParser.START_TAG) {
                    continue;
                }
                // Skip until we get a folder-listing tag
                String name = parser.getName();
                if (!name.equalsIgnoreCase(XML_TAG)) {
                    Log.w(TAG, "Unknown XML tag: " + name);
                    Utils.skipCurrentTag(parser);
                }
                readConversations(parser);
            }
        } finally {
            xmlDocument.close();
        }
    }

    /**
     * Parses folder elements, and add to mSubFolders.
     *
     * @param parser the Xml Parser currently pointing to an folder-listing tag.
     */
    private void readConversations(XmlPullParser parser)
            throws XmlPullParserException, IOException, ParseException {
        int type;
        Log.d(TAG, "readConversations");
        while ((type = parser.next()) != XmlPullParser.END_TAG
                && type != XmlPullParser.END_DOCUMENT) {
            // Skip until we get a start tag
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }
            // Skip until we get a folder-listing tag
            String name = parser.getName();
            if (!name.trim()
                    .equalsIgnoreCase(BluetoothMapConvoListingElement.XML_TAG_CONVERSATION)) {
                Log.w(TAG, "Unknown XML tag: " + name);
                Utils.skipCurrentTag(parser);
                continue;
            }
            // Add a single conversation
            add(BluetoothMapConvoListingElement.createFromXml(parser));
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof BluetoothMapConvoListing other)) {
            return false;
        }
        if (mHasUnread != other.mHasUnread) {
            return false;
        }
        if (!Objects.equals(mList, other.mList)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mHasUnread, mList);
    }
}
