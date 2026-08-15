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
import javax.swing.plaf.basic.BasicToggleButtonUI;

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
    updateMonitorColors();
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
      // The native L&F (e.g. Windows) paints its own hover highlight on
      // top of a button's background regardless of setBackground(), so
      // switch to the basic UI delegate to get a flat fill that doesn't
      // shift color under the mouse.
      btn.setUI(new BasicToggleButtonUI());
      btn.setRolloverEnabled(false);
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
    // Selected monitors are shown in a vivid blue while this widget is
    // actually interactive ("Full screen on selected monitor(s)" mode),
    // and a paler blue otherwise, when the selection is just inherited
    // state rather than something the user can act on right now.
    Color activeColor = isEnabled() ? new Color(0, 122, 251)
                                    : new Color(164, 205, 246);
    Color unselectedColor = new Color(200, 200, 200);

    for (JToggleButton btn : monitorButtons.values()) {
      if (btn.isSelected()) {
        btn.setBackground(activeColor);
        btn.setForeground(isEnabled() ? Color.WHITE : Color.BLACK);
      } else {
        btn.setBackground(unselectedColor);
        btn.setForeground(Color.BLACK);
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
