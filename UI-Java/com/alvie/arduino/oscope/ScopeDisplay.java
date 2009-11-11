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
import java.awt.*;
import java.awt.geom.GeneralPath;
import java.lang.Math.*;

public class ScopeDisplay extends JComponent {

    private int[] data;

    int triggerLevel;
    int numSamples;
    boolean isDual;

    public ScopeDisplay() {
        numSamples = 962;
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
        invalidate();
    }

    public boolean setNumSamples(int value)
    {
        if (numSamples!=value) {
            numSamples = value;
            resizeToNumSamples();
            return true;
        }
        return false;
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
