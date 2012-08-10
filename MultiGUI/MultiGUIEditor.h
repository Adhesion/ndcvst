//	MultiGUIEditor.h - Class to allow you to add multiple editors to the
//					   window passed to you by the host.
//	--------------------------------------------------------------------------
//	Copyright (c) 2005-2006 Niall Moody
//	
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//	--------------------------------------------------------------------------

#ifndef MULTIGUIEDITOR_H_
#define MULTIGUIEDITOR_H_

#include "AEffEditor.h"

#include <vector>

#ifdef WIN32
#include <windows.h>
#endif
#ifdef MACX
#include <Carbon/Carbon.h>
#endif

///	A more sensible way to store a rect, imo.
typedef struct fourints
{
	int x;
	int y;
	int width;
	int height;
} NiallsRect;

///	An editor class which is used to add multiple types of editors to the window provided by the host.
/*!
	The main use of this is to allow for OpenGL editor panels alongside
	standard VSTGUI/2D ones.
	You probably want to subclass this class to use as your main editor class
	(i.e. the one you construct from your plugin's constructor).
 */
class MultiGUIEditor : public AEffEditor
{
  public:
	///	Constructor.
	MultiGUIEditor(AudioEffect *effect);
	///	Destructor.
	virtual ~MultiGUIEditor();

	///	_rect is calculated from the editor panels held in editors.
	virtual bool getRect(ERect **rect) {*rect = &_rect; return true;};
	///	Called when the editor window is first displayed.
	virtual bool open(void *ptr);
	///	Called when the editor window is closed.
	virtual void close();
	///	Overridden to make sure child editors get idle() calls.
	virtual void idle();

	///	So we can call this from the plugin, and pass it on to the appropriate editor panels.
	/*!
		You have to implement this in your MultiGUIEditor subclass, because
		AEffEditor doesn't have a setParameter() method...
	 */
	virtual void setParameter(long index, float value) = 0;
	///	Intended to allow for resizeable interfaces (although I don't know if this is enough...).
	virtual void setSize(ERect rect) {_rect = rect;};

	///	Use this to add a new editor panel to the main plugin window.
	/*!
		_rect is updated for every editor panel added.
	 */
	void addEditor(AEffEditor *editor, NiallsRect rect);

#ifdef WIN32
	///	Message loop - we don't do anything with this, but it has to be here...
	static LONG WINAPI WndProc(HWND hwnd,
							   UINT message,
							   WPARAM wParam,
							   LPARAM lParam);
#endif
#ifdef MACX
	///	Message loop - we use this to intercept kEventControlBoundsChanged messages.
	static pascal OSStatus macEventHandler(EventHandlerCallRef handler,
										   EventRef event,
										   void *userData);

	///	Used to update our context's bounds if the host changes them.
	void updateBounds(int x, int y);
#endif
  private:
	///	Dynamic array of all the current editor panels.
	std::vector<AEffEditor *> editors;
	///	Dynamic array of the positions & sizes for each editor panel.
	std::vector<NiallsRect> rects;
	///	Dynamic array to hold the systemWindow pointers for each editor
	/*!	
		(since AEffEditor doesn't actually provide a getSystemWindow()
		method...)
	 */
	std::vector<void *> systemWindows;

	///	The rectangle holding the actual size of the plugin window (i.e. the size of each editor panel added up).
	ERect _rect;

#ifdef WIN32
	HWND tempHWnd;
#endif

#ifdef MACX
	///	Window group of all our child editors.
	WindowGroupRef mainGroup;

	///	Reference to our event handler.
	EventHandlerRef eventHandlerRef;
	///	Our context's actual bounds (x).
	int boundsX;
	///	Our context's actual bounds (y).
	int boundsY;
	///	The control class reference for our dummy HIView.
	ControlDefSpec controlSpec;
	///	Id for our dummy HI class.
	CFStringRef classId;
	///	Our dummy HIView (necessary for VST 2.4).
	ControlRef controlRef;

	///	Used to indicate that we called HIViewSetFrame, not the host.
	/*!
		Because Bidule doesn't send us kEventControlBoundsChanged, we need
		to make sure we don't try and move the windows when we call
		HIViewSetFrame().
	 */
	bool skipPosition;
#endif
};

#endif
