package com.alvie.arduino.oscope;

import javax.swing.*;        
import java.awt.*;
import java.awt.geom.GeneralPath;
import java.lang.Math.*;

public class ScopeDisplay extends JComponent {

    private int[] data;

    int triggerLevel;
    int numSamples;
    boolean isDual;

    public ScopeDisplay() {
        numSamples = 692;
        resizeToNumSamples();
    }

    public void setDual(boolean value)
    {
        isDual = value;
        repaint();
    }

    void resizeToNumSamples()
    {
        Dimension d = new Dimension(numSamples,255);
        setMinimumSize(d);
        setMaximumSize(d);
        setPreferredSize(d);
    }

    public void setNumSamples(int value)
    {
        if (numSamples!=value) {
            numSamples = value;
            resizeToNumSamples();
        }
    }

    public void setTriggerLevel(int level)
    {
        triggerLevel=level;
        repaint();
    }

    public void setData(int[] in_data)
    {
        if (null!=in_data && null!=data) {
            if (in_data.length == data.length)
                System.arraycopy(in_data,0,data,0,data.length);
            else
                data = in_data.clone();
        } else
            data = in_data.clone();
        repaint();
    }

    private void drawBackground(Graphics2D g2d, Dimension d)
    {
        GeneralPath p = new GeneralPath(GeneralPath.WIND_NON_ZERO);
        
        p.moveTo(0,0);
        p.lineTo(d.width,0);
        p.lineTo(d.width,d.height);
        p.lineTo(0,d.height);
        p.lineTo(0,0);
        p.closePath();
        g2d.setColor(Color.black);
        g2d.fill(p);

    }
    public void paintWaveform(Graphics2D g2d, Dimension d)
    {
        if (null==data)
            return;

        GeneralPath p = new GeneralPath(GeneralPath.WIND_NON_ZERO);
        int i;

        
        if (isDual) {
            g2d.setColor(Color.yellow);
            p.moveTo(0,0);

            for (i=0; i<data.length;i+=2) {
                p.lineTo(i, d.height - data[i]);
            }

            g2d.draw(p);

            p.reset();

            p.moveTo(1,0);

            for (i=1; i<data.length;i+=2) {
                p.lineTo(i, d.height - data[i]);
            }
            g2d.setColor(Color.green);
            
            g2d.draw(p);

        } else {
            g2d.setColor(Color.green);
            p.moveTo(0,0);

            for (i=0; i<data.length;i++) {
                p.lineTo(i, d.height - data[i]);
            }
            g2d.draw(p);
        }

    }

    void paintTrigger(Graphics2D g2d, Dimension d)
    {
        GeneralPath p = new GeneralPath(GeneralPath.WIND_NON_ZERO);
        g2d.setColor(Color.blue);
        p.moveTo(0,d.height-triggerLevel);
        p.lineTo(d.width, d.height-triggerLevel);
        g2d.draw(p);
    }

    public void paint(Graphics g) {
        super.paintComponent(g);
        Dimension d = getSize();

        Graphics2D g2d = (Graphics2D)g;
        drawBackground(g2d,d);

        BasicStroke stroke = new BasicStroke(2.0f,BasicStroke.CAP_ROUND,BasicStroke.JOIN_ROUND);
        g2d.setStroke(stroke);

        RenderingHints rh = new RenderingHints(
		RenderingHints.KEY_ANTIALIASING,
		RenderingHints.VALUE_ANTIALIAS_ON);

        g2d.setRenderingHints(rh);
        paintTrigger(g2d,d);
        paintWaveform(g2d,d);
    }
};
