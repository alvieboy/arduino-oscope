/*
 * Copyright (c) 2009 Alvaro Lopes <alvieboy@alvie.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

package com.alvie.arduino.oscope;

import javax.swing.*;        
import com.alvie.arduino.oscope.*;
import java.util.*;
import java.lang.*;
import java.io.*;
import gnu.io.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.event.*;


public class ArduinoOscopeImpl implements Protocol.ScopeDisplayer, ChangeListener, ActionListener {

    Protocol proto;

    JMenu serialMenu;

    SerialMenuListener serialMenuListener;
    DualChannelListener dualChannelListener;
    InvertTriggerListener invertTriggerListener;
    SingleShotListener singleShotListener;

    String current_serial_port;

    ScopeDisplay scope ;
    JSlider triggerSlider;
    JSlider holdoffSlider;
    JComboBox prescaleCombo,vrefCombo;
    JCheckBox dualChannelCheckBox,invertTriggerCheckBox;
    JButton freezeUnfreezeButton;
    JFrame frame;
    JPanel panel;
    JDialog waitTriggerDialog;

    ArduinoOscopeImpl()
    {
    }

    public void displayData(int [] buf, int size)
    {
        scope.setData(buf);
    }

    public void triggerDone()
    {
        if (null!=waitTriggerDialog) {
            waitTriggerDialog.dispose();
            waitTriggerDialog=null;
        }
    }

    public void gotParameters(int triggerLevel,int holdoffSamples,
                              int adcref, int prescale, int numSamples,
                              int flags)
    {
        boolean isDual = false;//(flags & Protocol.FLAG_DUAL_CHANNEL) != 0;
        boolean isInvertTrigger = (flags & Protocol.FLAG_INVERT_TRIGGER) != 0;

        triggerSlider.setValue(triggerLevel);
        holdoffSlider.setValue(holdoffSamples);
        prescaleCombo.setSelectedIndex(7-prescale);
        if(scope.setNumSamples(numSamples)) {
            frame.pack();
        }
        scope.setDual( isDual );

        dualChannelCheckBox.setSelected(isDual);
        invertTriggerCheckBox.setSelected(isInvertTrigger);

        switch(adcref) {
        case 0:
        case 1:
            vrefCombo.setSelectedIndex(adcref);
            break;
        case 3:
        default:
            vrefCombo.setSelectedIndex(2);
        }

    }

    protected void populateSerialMenu() {
        JMenuItem rbMenuItem;

        serialMenu.removeAll();
        boolean empty = true;

        try
        {
            Vector ports = proto.getPortList();
            Iterator i = ports.iterator();
            while (i.hasNext()) {
                String curr_port = (String) i.next();
                boolean active = (current_serial_port==null ? false: curr_port.equals(current_serial_port));
                rbMenuItem = new JCheckBoxMenuItem(curr_port, active);
                rbMenuItem.addActionListener(serialMenuListener);
                serialMenu.add(rbMenuItem);
                empty = false;

            }
            if (!empty) {
                //System.out.println("enabling the serialMenu");
                serialMenu.setEnabled(true);
            }

        }

        catch (Exception exception)
        {
            System.out.println("error retrieving port list");
            exception.printStackTrace();
        }

        if (serialMenu.getItemCount() == 0) {
            serialMenu.setEnabled(false);
        }
    }


    public void createAndShowGUI() {
        //Create and set up the window.
        proto = new Protocol();

        serialMenuListener = new SerialMenuListener();

        frame = new JFrame("Arduino Oscope");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        JMenuBar bar = new JMenuBar();

        serialMenu = new JMenu("Serial Port");
        bar.add(serialMenu);

        frame.setJMenuBar(bar);
        populateSerialMenu();

        scope = new ScopeDisplay();

        panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.PAGE_AXIS));

        JPanel hpanel = new JPanel();



        JPanel triggerPanel = new JPanel();
        triggerPanel.setLayout(new BoxLayout(triggerPanel, BoxLayout.PAGE_AXIS));

        triggerSlider = new JSlider(JSlider.HORIZONTAL, 0, 255, 0);

        JLabel triggerLabel = new JLabel("Trigger Level", JLabel.CENTER);
        triggerLabel.setAlignmentX(Component.CENTER_ALIGNMENT);

        triggerSlider.setMajorTickSpacing(16);
        triggerSlider.setMinorTickSpacing(1);
        triggerSlider.setPaintTicks(true);
        triggerSlider.addChangeListener(this);

        triggerPanel.add(triggerLabel);
        triggerPanel.add(triggerSlider);
        hpanel.add(triggerPanel);


        JPanel holdoffPanel = new JPanel();
        holdoffPanel.setLayout(new BoxLayout(holdoffPanel, BoxLayout.PAGE_AXIS));

        holdoffSlider = new JSlider(JSlider.HORIZONTAL, 0, 255, 0);

        JLabel holdoffLabel = new JLabel("Holdoff samples", JLabel.CENTER);
        holdoffLabel.setAlignmentX(Component.CENTER_ALIGNMENT);

        holdoffSlider.setMajorTickSpacing(16);
        holdoffSlider.setMinorTickSpacing(1);
        holdoffSlider.setPaintTicks(true);
        holdoffSlider.setPaintLabels(false);
        holdoffSlider.addChangeListener(this);
        holdoffPanel.add(holdoffLabel);
        holdoffPanel.add(holdoffSlider);
        hpanel.add(holdoffPanel);


        JPanel prescalePanel = new JPanel();
        prescalePanel.setLayout(new BoxLayout(prescalePanel, BoxLayout.PAGE_AXIS));
        String[] prescaleStrings = { "128","64","32","16","8","4","2" };
        JLabel prescaleLabel = new JLabel("Prescaler",JLabel.CENTER);
        prescaleLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
        prescaleCombo = new JComboBox(prescaleStrings);
        prescaleCombo.addActionListener(this);
        prescalePanel.add(prescaleLabel);
        prescalePanel.add(prescaleCombo);
        hpanel.add(prescalePanel);

        JPanel vrefPanel = new JPanel();
        vrefPanel.setLayout(new BoxLayout(vrefPanel, BoxLayout.PAGE_AXIS));
        String[] vrefStrings = { "AREF","AVcc","Internal 1.1V" };
        JLabel vrefLabel = new JLabel("VRef",JLabel.CENTER);
        vrefLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
        vrefCombo = new JComboBox(vrefStrings);
        vrefCombo.addActionListener(this);
        vrefPanel.add(vrefLabel);
        vrefPanel.add(vrefCombo);
        hpanel.add(vrefPanel);


        dualChannelCheckBox = new JCheckBox("Dual Channel", false);
        hpanel.add(dualChannelCheckBox);
        dualChannelListener = new DualChannelListener();
        dualChannelCheckBox.addActionListener(dualChannelListener);

        invertTriggerCheckBox = new JCheckBox("Invert trigger", false);
        hpanel.add(invertTriggerCheckBox);
        invertTriggerListener = new InvertTriggerListener();
        invertTriggerCheckBox.addActionListener(invertTriggerListener);

        panel.add(scope);
        panel.add(hpanel);

        JPanel buttonPanel = new JPanel();
        JButton singleShot = new JButton("Single shot");
        buttonPanel.add(singleShot);
        singleShotListener = new SingleShotListener();
        singleShot.addActionListener(singleShotListener);

        freezeUnfreezeButton = new JButton("Freeze");
        buttonPanel.add(freezeUnfreezeButton);
        // Not ready yet. panel.add(buttonPanel);

        frame.getContentPane().add(panel);

        //Display the window.
        frame.pack();
        frame.setVisible(true);

        proto.setDisplayer(this);
    }

    public void stateChanged(ChangeEvent e) {
        JSlider source = (JSlider)e.getSource();
        int value = (int)source.getValue();

        if (source==triggerSlider) {
            System.out.println("New trigger " + value);
            proto.setTriggerLevel(value);
            scope.setTriggerLevel(value);
        } else if (source==holdoffSlider) {
            proto.setHoldoff(value);
        }
    }

    public double log2(double d) {
        return Math.log(d)/Math.log(2.0);
    }

    class DualChannelListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JCheckBox cb = (JCheckBox)e.getSource();
            proto.setDualChannel(cb.isSelected());
        }
    }

    class InvertTriggerListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JCheckBox cb = (JCheckBox)e.getSource();
            proto.setTriggerInvert(cb.isSelected());
        }
    }

    class SingleShotListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JOptionPane p = new JOptionPane("Waiting for trigger...\n"+
                                            "Press OK to cancel",
                                            JOptionPane.INFORMATION_MESSAGE
                                            );

            waitTriggerDialog = p.createDialog(frame,"Trigger");
            waitTriggerDialog.pack();
            proto.setOneShot(true);
            waitTriggerDialog.setVisible(true);
            proto.setOneShot(false);
            waitTriggerDialog = null;
        }
    }

    public void actionPerformed(ActionEvent e) {
        JComboBox cb = (JComboBox)e.getSource();
        String name = (String)cb.getSelectedItem();
        if (cb==prescaleCombo) {
            int base = (int)log2(Integer.parseInt(name));
            System.out.println("New prescaler " + base + " (" + name + ")");
            proto.setPrescaler(base);
        } else if (cb==vrefCombo) {
            int base;
            if (name.equals("AREF")) {
                base=0;
            } else if (name.equals("AVcc")) {
                base=1;
            } else {
                base = 3;
            }
            proto.setVref(base);
        }
    }

    class SerialMenuListener implements ActionListener {
        //public SerialMenuListener() { }

        public void actionPerformed(ActionEvent e) {
            if(serialMenu == null) {
                System.out.println("serialMenu is null");
                return;
            }
            int count = serialMenu.getItemCount();
            for (int i = 0; i < count; i++) {
                ((JCheckBoxMenuItem)serialMenu.getItem(i)).setState(false);
            }
            JCheckBoxMenuItem item = (JCheckBoxMenuItem)e.getSource();
            item.setState(true);
            current_serial_port = item.getText();
            try {
                proto.changeSerialPort(current_serial_port);
            } catch (Protocol.SerialPortOpenException ex) {
                current_serial_port=null;
            } 
        }
    }

}
