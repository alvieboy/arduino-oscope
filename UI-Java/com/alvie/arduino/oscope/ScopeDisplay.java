package com.alvie.arduino.oscope;

import javax.swing.*;        
import java.awt.*;
import java.awt.geom.GeneralPath;
import java.lang.Math.*;

public class ScopeDisplay extends JComponent {

    public ScopeDisplay() {
        setMinimumSize(new Dimension(962,255));
        setMaximumSize(new Dimension(962,255));
        setPreferredSize(new Dimension(962,255));
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
    public final void test(Graphics2D g2d, Dimension d)
    {
        GeneralPath p = new GeneralPath(GeneralPath.WIND_NON_ZERO);
        int i;

        BasicStroke stroke = new BasicStroke(2.0f,BasicStroke.CAP_ROUND,BasicStroke.JOIN_ROUND);
        g2d.setStroke(stroke);

        g2d.setColor(Color.green);
        p.moveTo(0,0);
        for (i=0; i<d.width;i++) {
            double v = d.height/2 * java.lang.Math.sin( (double)i/4 );
            p.lineTo(i,((double)d.height/2)-v);
        }
        g2d.draw(p);
    }

    public void paint(Graphics g) {
        super.paintComponent(g);
        Dimension d = getSize();

        Graphics2D g2d = (Graphics2D)g;
        drawBackground(g2d,d);

        RenderingHints rh = new RenderingHints(
		RenderingHints.KEY_ANTIALIASING,
		RenderingHints.VALUE_ANTIALIAS_ON);

        g2d.setRenderingHints(rh);
        test(g2d,d);
        System.out.println("Paint");
    }
};
