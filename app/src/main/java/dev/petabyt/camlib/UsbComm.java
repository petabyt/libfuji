// Basic libusb-like driver intented for PTP devices
// Copyright Daniel Cook - Apache License
package dev.petabyt.camlib;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

public class UsbComm {
    public UsbDevice dev = null;
    UsbInterface interf = null;

    public int vendorID = 0;
    public String deviceName;

    UsbEndpoint endpointOut = null;
    UsbEndpoint endpointIn = null;

    int timeout = 1000;
    public int endpoints = 0;

    UsbDeviceConnection connection;
    UsbManager inMan;

    // Once this is enabled (during connection), all comm will be disabled
    public boolean killSwitch = true;

    public void getEndpoints() throws Exception {
        endpointOut = null;
        endpointIn = null;
        endpoints = interf.getEndpointCount();
        for (int e = 0; e < interf.getEndpointCount(); e++) {
            UsbEndpoint ep = interf.getEndpoint(e);
            if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                if (ep.getDirection() == UsbConstants.USB_DIR_OUT) {
                    endpointOut = ep;
                    //max = ep.getMaxPacketSize();
                } else {
                    endpointIn = ep;
                }
            }
        }

        if (endpointOut == null || endpointIn == null) {
            throw new Exception("Could not find a USB endpoint.");
        }
    }

    public void getUsbDevices(UsbManager man) throws Exception {
        inMan = man;
        for (UsbDevice d : man.getDeviceList().values()) {
            if (d.getDeviceProtocol() == 0) {
                vendorID = d.getVendorId();
                deviceName = d.getDeviceName();
                dev = d;
                return;
            }
        }

        throw new Exception("Could not find a USB device.");
    }

    // Loop till we hit the first PTP device
    public void getInterface() throws Exception {
        for (int i = 0; i < dev.getInterfaceCount(); i++) {
            interf = dev.getInterface(i);
            if (interf.getInterfaceClass() == 6) {
                return;
            }
        }

        throw new Exception("Could not find a USB interface.");
    }

    public void openConnection() throws Exception {
        connection = inMan.openDevice(dev);
        if (connection == null) {
            throw new Exception("Could not connect to the USB device.");
        }

        killSwitch = false;
    }

    public int write(byte[] data, int length) throws Exception {
        if (killSwitch) return -1;
        try {
            return connection.bulkTransfer(endpointOut, data, length, timeout);
        } catch (Error e) {
            return -5;
        }
    }

    public int read(byte[] data, int length) throws Exception {
        if (killSwitch) return -1;
        try {
            return connection.bulkTransfer(endpointIn, data, length, timeout);
        } catch (Error e) {
            return -5;
        }
    }

    public void closeConnection() {
        connection.releaseInterface(interf);
        connection.close();
        endpointOut = null;
        endpointIn = null;
        dev = null;
        interf = null;
        connection = null;
    }

    public int reqMessage(int type, int id, byte data[]) {
        return connection.controlTransfer(type, id, 0, 0, data, data.length, timeout);
    }

    public int clear() {
        byte[] data = new byte[33];
        return connection.controlTransfer(0xa0, 0x67, 0, 0, data, data.length, timeout);
    }

    public int reset() {
        return reqMessage(0x20, 0x66, new byte[0]);
    }
}
