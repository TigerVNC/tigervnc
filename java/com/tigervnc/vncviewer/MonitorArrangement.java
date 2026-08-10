/* Copyright (C) 2026 TigerVNC Team
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;

public final class MonitorArrangement extends JPanel {

  private final Map<Integer, JToggleButton> monitorButtons = new LinkedHashMap<Integer, JToggleButton>();
  private final Map<Integer, Rectangle> monitorBounds = new LinkedHashMap<Integer, Rectangle>();
  private ActionListener selectionListener;

  public MonitorArrangement() {
    setLayout(null);
    setBackground(new Color(240, 240, 240));
    setBorder(BorderFactory.createLineBorder(new Color(200, 200, 200)));
    refreshMonitors();

    addComponentListener(new ComponentAdapter() {
      @Override
      public void componentResized(ComponentEvent e) {
        layoutMonitors();
      }
    });
  }

  @Override
  public void doLayout() {
    super.doLayout();
    layoutMonitors();
  }

  @Override
  public void setEnabled(boolean enabled) {
    super.setEnabled(enabled);
    for (JToggleButton btn : monitorButtons.values()) {
      btn.setEnabled(enabled);
    }
  }

  public void setSelectionListener(ActionListener listener) {
    this.selectionListener = listener;
  }

  public Set<Integer> getSelectedMonitors() {
    Set<Integer> selected = new TreeSet<Integer>();
    if (monitorButtons.size() == 1) {
      selected.addAll(monitorButtons.keySet());
      return selected;
    }
    for (Map.Entry<Integer, JToggleButton> entry : monitorButtons.entrySet()) {
      if (entry.getValue().isSelected()) {
        selected.add(entry.getKey());
      }
    }
    return selected;
  }

  public void setSelectedMonitors(Set<Integer> indices) {
    if (indices == null) indices = new TreeSet<Integer>();
    for (Map.Entry<Integer, JToggleButton> entry : monitorButtons.entrySet()) {
      boolean sel = indices.contains(entry.getKey()) || monitorButtons.size() == 1;
      entry.getValue().setSelected(sel);
    }
    updateMonitorColors();
    repaint();
  }

  public void refreshMonitors() {
    removeAll();
    monitorButtons.clear();
    monitorBounds.clear();

    GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
    GraphicsDevice[] devices = ge.getScreenDevices();

    for (int i = 0; i < devices.length; i++) {
      GraphicsConfiguration gc = devices[i].getDefaultConfiguration();
      Rectangle b = gc.getBounds();

      // Check for mirrored screen duplicates
      boolean duplicate = false;
      for (Rectangle existing : monitorBounds.values()) {
        if (existing.equals(b)) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;

      final int index = i;
      final JToggleButton btn = new JToggleButton(String.valueOf(i + 1));
      btn.setFocusable(false);
      btn.setToolTipText(String.format("Monitor %d: %dx%d at +%d,+%d",
                         i + 1, b.width, b.height, b.x, b.y));

      btn.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
          if (monitorButtons.size() == 1) {
            btn.setSelected(true);
          }
          updateMonitorColors();
          if (selectionListener != null) {
            selectionListener.actionPerformed(new ActionEvent(MonitorArrangement.this,
                                               ActionEvent.ACTION_PERFORMED,
                                               "monitorSelectionChanged"));
          }
        }
      });

      monitorButtons.put(index, btn);
      monitorBounds.put(index, b);
      add(btn);
    }

    if (monitorButtons.size() == 1) {
      for (JToggleButton btn : monitorButtons.values()) {
        btn.setSelected(true);
      }
    }

    layoutMonitors();
    updateMonitorColors();
    revalidate();
    repaint();
  }

  private void updateMonitorColors() {
    Set<Integer> selected = getSelectedMonitors();
    Rectangle boundingRect = null;

    if (!selected.isEmpty()) {
      for (int idx : selected) {
        Rectangle b = monitorBounds.get(idx);
        if (b != null) {
          if (boundingRect == null) boundingRect = new Rectangle(b);
          else boundingRect = boundingRect.union(b);
        }
      }
    }

    Color activeColor = new Color(51, 153, 255);
    Color requiredColor = new Color(180, 215, 255);
    Color defaultColor = UIManager.getColor("Button.background");

    for (Map.Entry<Integer, JToggleButton> entry : monitorButtons.entrySet()) {
      int idx = entry.getKey();
      JToggleButton btn = entry.getValue();

      if (btn.isSelected()) {
        btn.setBackground(activeColor);
        btn.setForeground(Color.WHITE);
      } else {
        boolean isRequired = false;
        if (boundingRect != null) {
          Rectangle mb = monitorBounds.get(idx);
          if (mb != null && boundingRect.contains(mb)) {
            isRequired = true;
          }
        }
        if (isRequired) {
          btn.setBackground(requiredColor);
          btn.setForeground(Color.BLACK);
        } else {
          btn.setBackground(defaultColor);
          btn.setForeground(Color.BLACK);
        }
      }
    }
  }

  private void layoutMonitors() {
    if (monitorBounds.isEmpty()) return;

    int totalMinX = Integer.MAX_VALUE, totalMinY = Integer.MAX_VALUE;
    int totalMaxX = Integer.MIN_VALUE, totalMaxY = Integer.MIN_VALUE;

    for (Rectangle b : monitorBounds.values()) {
      totalMinX = Math.min(totalMinX, b.x);
      totalMinY = Math.min(totalMinY, b.y);
      totalMaxX = Math.max(totalMaxX, b.x + b.width);
      totalMaxY = Math.max(totalMaxY, b.y + b.height);
    }

    int totalW = totalMaxX - totalMinX;
    int totalH = totalMaxY - totalMinY;

    int availableW = getWidth() - 20;
    int availableH = getHeight() - 20;
    if (availableW <= 0 || availableH <= 0) return;

    double scaleX = (double) availableW / totalW;
    double scaleY = (double) availableH / totalH;
    double scale = Math.min(scaleX, scaleY);

    int scaledTotalW = (int) (totalW * scale);
    int scaledTotalH = (int) (totalH * scale);

    int offsetX = (getWidth() - scaledTotalW) / 2;
    int offsetY = (getHeight() - scaledTotalH) / 2;

    for (Map.Entry<Integer, Rectangle> entry : monitorBounds.entrySet()) {
      int idx = entry.getKey();
      Rectangle b = entry.getValue();
      JToggleButton btn = monitorButtons.get(idx);

      int bx = offsetX + (int) ((b.x - totalMinX) * scale);
      int by = offsetY + (int) ((b.y - totalMinY) * scale);
      int bw = Math.max(25, (int) (b.width * scale));
      int bh = Math.max(20, (int) (b.height * scale));

      btn.setBounds(bx, by, bw, bh);
    }
  }

  @Override
  public Dimension getPreferredSize() {
    return new Dimension(320, 140);
  }

  @Override
  public Dimension getMinimumSize() {
    return new Dimension(320, 140);
  }
}
