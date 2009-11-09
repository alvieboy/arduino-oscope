package com.alvie.arduino.oscope;


import javax.swing.*;        

public class ArduinoOscope {
    static ArduinoOscopeImpl impl;
    public static void main(String[] args) {
        impl = new ArduinoOscopeImpl();
        javax.swing.SwingUtilities.invokeLater(new Runnable() { public void run() { impl.createAndShowGUI(); } } );
    }
};

