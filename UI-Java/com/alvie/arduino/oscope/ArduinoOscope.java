package com.alvie.arduino.oscope;

/*
 * HelloWorldSwing.java requires no other files. 
 */
import javax.swing.*;        

public class ArduinoOscope {
    /**
     * Create the GUI and show it.  For thread safety,
     * this method should be invoked from the
     * event-dispatching thread.
     */
    private static void createAndShowGUI() {
        //Create and set up the window.
        JFrame frame = new JFrame("HelloWorldSwing");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);


        JLabel label = new JLabel("Label1");
        JLabel label2 = new JLabel("Label2");
        JLabel label3 = new JLabel("Label3");
        JButton button1 = new JButton("Butao");

        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.PAGE_AXIS));

        panel.add(label);
        panel.add(label2);
        panel.add(label3);
        panel.add(button1);

        frame.getContentPane().add(panel);

        //Display the window.
        frame.pack();
        frame.setVisible(true);
    }

    public static void main(String[] args) {
        //Schedule a job for the event-dispatching thread:
        //creating and showing this application's GUI.
        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                createAndShowGUI();
            }
        });
    }
}
