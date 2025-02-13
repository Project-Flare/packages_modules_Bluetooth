/*
 * Copyright (C) 2021 The Android Open Source Project
 *
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

package com.android.bluetooth.hfpclient;

import static android.bluetooth.BluetoothUtils.writeStringToParcel;

import android.bluetooth.BluetoothDevice;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.SystemClock;

import java.util.UUID;

/**
 * This class represents a single call, its state and properties.
 *
 * <p>HfpClientCall is Parcelable so it can be passed through the TelecomManager Connection Service
 * framework.
 */
public final class HfpClientCall implements Parcelable {

    /* Call state */
    /** Call is active. */
    public static final int CALL_STATE_ACTIVE = 0;

    /** Call is in held state. */
    public static final int CALL_STATE_HELD = 1;

    /** Outgoing call that is being dialed right now. */
    public static final int CALL_STATE_DIALING = 2;

    /** Outgoing call that remote party has already been alerted about. */
    public static final int CALL_STATE_ALERTING = 3;

    /** Incoming call that can be accepted or rejected. */
    public static final int CALL_STATE_INCOMING = 4;

    /** Waiting call state when there is already an active call. */
    public static final int CALL_STATE_WAITING = 5;

    /**
     * Call that has been held by response and hold (see Bluetooth specification for further
     * references).
     */
    public static final int CALL_STATE_HELD_BY_RESPONSE_AND_HOLD = 6;

    /** Call that has been already terminated and should not be referenced as a valid call. */
    public static final int CALL_STATE_TERMINATED = 7;

    private final BluetoothDevice mDevice;
    private final int mId;
    private int mState;
    private String mNumber;
    private boolean mMultiParty;
    private final boolean mOutgoing;
    private final UUID mUUID;
    private final long mCreationElapsedMilli;
    private final boolean mInBandRing;

    /** Creates HfpClientCall instance. */
    public HfpClientCall(
            BluetoothDevice device,
            int id,
            int state,
            String number,
            boolean multiParty,
            boolean outgoing,
            boolean inBandRing) {
        this(device, id, UUID.randomUUID(), state, number, multiParty, outgoing, inBandRing);
    }

    private HfpClientCall(
            BluetoothDevice device,
            int id,
            UUID uuid,
            int state,
            String number,
            boolean multiParty,
            boolean outgoing,
            boolean inBandRing) {
        mDevice = device;
        mId = id;
        mUUID = uuid;
        mState = state;
        mNumber = number != null ? number : "";
        mMultiParty = multiParty;
        mOutgoing = outgoing;
        mInBandRing = inBandRing;
        mCreationElapsedMilli = SystemClock.elapsedRealtime();
    }

    /**
     * Sets call's state.
     *
     * <p>Note: This is an internal function and shouldn't be exposed
     *
     * @param state new call state.
     */
    public void setState(int state) {
        mState = state;
    }

    /**
     * Sets call's number.
     *
     * <p>Note: This is an internal function and shouldn't be exposed
     *
     * @param number String representing phone number.
     */
    public void setNumber(String number) {
        mNumber = number;
    }

    /**
     * Sets this call as multi party call.
     *
     * <p>Note: This is an internal function and shouldn't be exposed
     *
     * @param multiParty if <code>true</code> sets this call as a part of multi party conference.
     */
    public void setMultiParty(boolean multiParty) {
        mMultiParty = multiParty;
    }

    /**
     * Gets call's device.
     *
     * @return call device.
     */
    public BluetoothDevice getDevice() {
        return mDevice;
    }

    /**
     * Gets call's Id.
     *
     * @return call id.
     */
    public int getId() {
        return mId;
    }

    /**
     * Gets call's UUID.
     *
     * @return call uuid
     */
    public UUID getUUID() {
        return mUUID;
    }

    /**
     * Gets call's current state.
     *
     * @return state of this particular phone call.
     */
    public int getState() {
        return mState;
    }

    /**
     * Gets call's number.
     *
     * @return string representing phone number.
     */
    public String getNumber() {
        return mNumber;
    }

    /**
     * Gets call's creation time in millis since epoch.
     *
     * @return long representing the creation time.
     */
    public long getCreationElapsedMilli() {
        return mCreationElapsedMilli;
    }

    /**
     * Checks if call is an active call in a conference mode (aka multi party).
     *
     * @return <code>true</code> if call is a multi party call, <code>false</code> otherwise.
     */
    public boolean isMultiParty() {
        return mMultiParty;
    }

    /**
     * Checks if this call is an outgoing call.
     *
     * @return <code>true</code> if its outgoing call, <code>false</code> otherwise.
     */
    public boolean isOutgoing() {
        return mOutgoing;
    }

    /**
     * Checks if the ringtone will be generated by the connected phone
     *
     * @return <code>true</code> if in band ring is enabled, <code>false</code> otherwise.
     */
    public boolean isInBandRing() {
        return mInBandRing;
    }

    /**
     * Generate a log string for this call
     *
     * @return log string
     */
    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder("HfpClientCall{mDevice: ");
        builder.append(mDevice);
        builder.append(", mId: ");
        builder.append(mId);
        builder.append(", mUUID: ");
        builder.append(mUUID);
        builder.append(", mState: ");
        switch (mState) {
            case CALL_STATE_ACTIVE:
                builder.append("ACTIVE");
                break;
            case CALL_STATE_HELD:
                builder.append("HELD");
                break;
            case CALL_STATE_DIALING:
                builder.append("DIALING");
                break;
            case CALL_STATE_ALERTING:
                builder.append("ALERTING");
                break;
            case CALL_STATE_INCOMING:
                builder.append("INCOMING");
                break;
            case CALL_STATE_WAITING:
                builder.append("WAITING");
                break;
            case CALL_STATE_HELD_BY_RESPONSE_AND_HOLD:
                builder.append("HELD_BY_RESPONSE_AND_HOLD");
                break;
            case CALL_STATE_TERMINATED:
                builder.append("TERMINATED");
                break;
            default:
                builder.append(mState);
                break;
        }
        builder.append(", mNumber: ");
        builder.append(mNumber);
        builder.append(", mMultiParty: ");
        builder.append(mMultiParty);
        builder.append(", mOutgoing: ");
        builder.append(mOutgoing);
        builder.append(", mInBandRing: ");
        builder.append(mInBandRing);
        builder.append("}");
        return builder.toString();
    }

    /** {@link Parcelable.Creator} interface implementation. */
    public static final @android.annotation.NonNull Parcelable.Creator<HfpClientCall> CREATOR =
            new Parcelable.Creator<HfpClientCall>() {
                @Override
                public HfpClientCall createFromParcel(Parcel in) {
                    return new HfpClientCall(
                            BluetoothDevice.CREATOR.createFromParcel(in),
                            in.readInt(),
                            UUID.fromString(in.readString()),
                            in.readInt(),
                            in.readString(),
                            in.readInt() == 1,
                            in.readInt() == 1,
                            in.readInt() == 1);
                }

                @Override
                public HfpClientCall[] newArray(int size) {
                    return new HfpClientCall[size];
                }
            };

    @Override
    public void writeToParcel(Parcel out, int flags) {
        mDevice.writeToParcel(out, flags);
        out.writeInt(mId);
        writeStringToParcel(out, mUUID.toString());
        out.writeInt(mState);
        writeStringToParcel(out, mNumber);
        out.writeInt(mMultiParty ? 1 : 0);
        out.writeInt(mOutgoing ? 1 : 0);
        out.writeInt(mInBandRing ? 1 : 0);
    }

    @Override
    public int describeContents() {
        return 0;
    }
}
