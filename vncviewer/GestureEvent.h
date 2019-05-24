/* Copyright 2019 Aaron Sowry for Cendio AB
 * Copyright 2020 Samuel Mannehed for Cendio AB
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
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __GESTUREEVENT_H__
#define __GESTUREEVENT_H__

enum GestureEventGesture {
  GestureOneTap,
  GestureTwoTap,
  GestureThreeTap,
  GestureDrag,
  GestureLongPress,
  GestureTwoDrag,
  GesturePinch,
};

enum GestureEventType {
  GestureBegin,
  GestureUpdate,
  GestureEnd,
};

// magnitude is used by two gestures:
//  GestureTwoDrag: distance moved since GestureBegin
//  GesturePinch: distance between fingers
struct GestureEvent {
  double eventX;
  double eventY;
  double magnitudeX;
  double magnitudeY;
  GestureEventGesture gesture;
  GestureEventType type;
};

#endif // __GESTUREEVENT_H__
