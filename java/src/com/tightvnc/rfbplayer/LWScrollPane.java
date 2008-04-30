
package com.tightvnc.rfbplayer;

import java.awt.*;
import java.awt.event.*;

class LWScrollPane extends Container implements AdjustmentListener {

  /** position info */
  private Point scrollPosition = new Point(0, 0);

  /** panel to hold component to scroll */
  private Panel innerPanel = new Panel() {

    public void update(Graphics g) {
      if (needClear) {
        super.update(g);
        needClear = false;
      } else
        this.paint(g);
    }

  };

  /** component to display */
  private Component containedComp;

  /** layout info */
  private GridBagLayout gb;
  private GridBagConstraints gbc;

  /** scroll bars */
  private Scrollbar xScroller;
  private Scrollbar yScroller;

  /** flags indicating which scollbars are visible */
  private boolean showingXScroll = false;
  private boolean showingYScroll = false;

  /** flag indicating when innerpanel needs to repaint background */
  private boolean needClear = false;

  /** dimensions for our preferred size */
  private int width = 0;
  private int height = 0;

  /** c'tor for a new scroll pane */
  public LWScrollPane() {
    // create scroll bars
    xScroller = new Scrollbar(Scrollbar.HORIZONTAL) {

      public boolean isFocusable() {
        return false;
      }

    };
    yScroller = new Scrollbar(Scrollbar.VERTICAL) {

      public boolean isFocusable() {
        return false;
      }

    };
    xScroller.addAdjustmentListener(this);
    yScroller.addAdjustmentListener(this);

    // layout info 
    gb = new GridBagLayout();
    gbc = new GridBagConstraints();
    setLayout(gb);
    setBackground(Color.white);

    // added inner panel
    //innerPanel.setBackground(Color.blue);
    gbc.fill = GridBagConstraints.BOTH;
    gbc.gridx = 0;
    gbc.gridy = 0;
    gbc.weightx = 100;
    gbc.weighty = 100;
    gb.setConstraints(innerPanel, gbc);
    add(innerPanel);
    innerPanel.setLayout(null);
  }
  /*
  public void update(Graphics g) {
  paint(g);
  }
   */
  /*
  public void paint(Graphics g) {
  super.paint(g);
  }
   */

  /**
   * Provided to allow the containing frame to resize.
   * OS X JVM 1.3 would not allow a frame to be made
   * smaller without overriding getMinimumSize.
   */
  public Dimension getMinimumSize() {
    return new Dimension(0, 0);
  }

  public Dimension getPreferredSize() {
    return new Dimension(width, height);
  }

  public void setSize(int width, int height) {
    this.width = width;
    this.height = height;
    super.setSize(width, height);
  }

  public void setSize(Dimension d) {
    setSize(d.width, d.height);
  }

  /** 
   * Force component to clear itself before repainting. 
   * Primarily useful if the contained component shrinks
   * without the scroll pane reducing in size.
   */
  public void clearAndRepaint() {
    needClear = true;
    innerPanel.repaint();
  }

  /** Add the component to be scrolled by scroll pane */
  void addComp(Component comp) {
    containedComp = comp;
    innerPanel.add(containedComp);
  }

  /** 
   * Set the point of the component to display in the
   * upper left corner of the viewport.
   */
  void setScrollPosition(int x, int y) {
    Dimension vps = getViewportSize();
    Dimension ccs = containedComp.getPreferredSize();

    // skip entirely if component smaller than viewer
    if (ccs.width <= vps.width && ccs.height <= vps.height)
      return;

    // don't scroll too far left or up 
    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;

    // don't scroll too far right or down 
    if (ccs.width <= vps.width)
      x = 0;
    else if (x > (ccs.width - vps.width))
      x = ccs.width - vps.width;
    if (ccs.height <= vps.height)
      y = 0;
    else if (y > (ccs.height - vps.height))
      y = ccs.height - vps.height;

    scrollPosition = new Point(x, y);
    containedComp.setLocation(-scrollPosition.x, -scrollPosition.y);
    xScroller.setValue(scrollPosition.x);
    yScroller.setValue(scrollPosition.y);
  }

  /** Returns the point at the upper left corner of viewport */
  Point getScrollPosition() {
    return new Point(scrollPosition);
  }

  /** Return the dimensions of the viewport */
  public Dimension getViewportSize() {
    int vpW, vpH;
    Dimension size = getSize();
    vpW = size.width;
    vpH = size.height;
    if (showingYScroll)
      vpW -= yScroller.getSize().width;
    if (showingXScroll)
      vpH -= xScroller.getSize().height;

    return new Dimension(vpW, vpH);
  }

  /** 
   * Ensure that the scroll pane is properly arranged after
   * a component is added, the pane is resized, etc.
   */
  public void doLayout() {
    /** Add scroll bars as necessary */
    boolean needX = false, needY = false;
    Dimension innerSize = containedComp.getPreferredSize();
    Dimension scrollDimension = getSize();

    if (innerSize.width > scrollDimension.width)
      needX = true;
    if (innerSize.height > scrollDimension.height)
      needY = true;

    showingXScroll = false;
    showingYScroll = false;
    remove(yScroller);
    remove(xScroller);

    if (needY) {
      gbc.gridy = 0;
      gbc.gridx = 1;
      gbc.weightx = 0;
      gb.setConstraints(yScroller, gbc);
      add(yScroller);
      showingYScroll = true;
    }

    if (needX) {
      gbc.gridy = 1;
      gbc.gridx = 0;
      gbc.weightx = 100;
      gbc.weighty = 0;
      gb.setConstraints(xScroller, gbc);
      add(xScroller);
      showingXScroll = true;
    }

    /* set scroll bar values */
    int vpW, vpH;
    vpW = scrollDimension.width;
    vpH = scrollDimension.height;
    if (showingYScroll)
      vpW -= yScroller.getSize().width;
    if (showingXScroll)
      vpH -= xScroller.getSize().height;
    yScroller.setValues(0, vpH, 0, innerSize.height);
    xScroller.setValues(0, vpW, 0, innerSize.width);

    containedComp.setLocation(0, 0);
    super.doLayout();
  }

  /**
   * Adjustment listener method for receiving callbacks
   * from scroll actions. 
   *	
   * @param e the AdjustmentEvent
   * @return void
   */
  public void adjustmentValueChanged(AdjustmentEvent e) {
    Point p = containedComp.getLocation();
    if (e.getAdjustable() == xScroller) {
      p.x = -e.getValue();
      scrollPosition.x = e.getValue();
    } else {
      p.y = -e.getValue();
      scrollPosition.y = e.getValue();
    }
    containedComp.setLocation(p);
  }

}
