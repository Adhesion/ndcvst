//	MultiGUIEditor.cpp - Class to allow you to add multiple editors to the
//						 window passed to you by the host.
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

#include "MultiGUIEditor.h"

using namespace std;

#ifdef WIN32
//This is the instance of the application, set in the main source file.
//We use it to create the various child windows.
extern void* hInstance;
#endif

//----------------------------------------------------------------------------
MultiGUIEditor::MultiGUIEditor(AudioEffect *effect):
AEffEditor(effect)
{
	_rect.left = 0;
	_rect.top = 0;
	_rect.right = 0;
	_rect.bottom = 0;

#ifdef WIN32
	char tempstr[32];

	//Register window class.
	WNDCLASSEX winClass;

	//most of this stuff is copied from VSTGUI
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	winClass.lpfnWndProc = WndProc;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;
	winClass.hInstance = static_cast<HINSTANCE>(hInstance);
	winClass.hIcon = 0;
	winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winClass.lpszMenuName = 0;
	//sprintf(tempstr, "MultiGUIWindow%08x", static_cast<HINSTANCE>(hInstance));
	sprintf(tempstr, "MultiGUIWindow%08x", reinterpret_cast<int>(this));
	winClass.lpszClassName = tempstr;
	winClass.hIconSm = NULL;

	//Register the window class (this is unregistered in the
	//MultiGUIEditor destructor).
	RegisterClassEx(&winClass);
#endif
#ifdef MACX
	char tempstr[32];

	boundsX = 0;
	boundsY = 0;
	skipPosition = false;

	//Register our dummy HIView object.
	//Not sure that all of these are needed, but I don't want to risk breaking
	//anything.
	EventTypeSpec eventList[] = {{kEventClassControl, kEventControlBoundsChanged},
								 {kEventClassControl, kEventControlHitTest},
								 {kEventClassControl, kEventControlDraw},
								 {kEventClassMouse, kEventMouseWheelMoved},
								 {kEventClassControl, kEventControlClick},
								 {kEventClassControl, kEventControlTrack},
								 {kEventClassControl, kEventControlContextualMenuClick},
								 {kEventClassKeyboard, kEventRawKeyRepeat},
								 {kEventClassControl, kEventControlDragEnter},
								 {kEventClassControl, kEventControlDragWithin},
								 {kEventClassControl, kEventControlDragLeave},
								 {kEventClassControl, kEventControlDragReceive},
								 {kEventClassControl, kEventControlGetClickActivation},
								 {kEventClassControl, kEventControlGetOptimalBounds},
								 {kEventClassScrollable, kEventScrollableGetInfo},
								 {kEventClassScrollable, kEventScrollableScrollTo},
								 {kEventClassControl, kEventControlSetFocusPart},
								 {kEventClassControl, kEventControlGetFocusPart}};
	sprintf(tempstr, "com.multigui.%08x", reinterpret_cast<int>(this));
	classId = CFStringCreateWithCString(NULL, tempstr, 0);
	controlSpec.defType = kControlDefObjectClass;
	controlSpec.u.classRef = NULL;
	ToolboxObjectClassRef controlClass = NULL;

	OSStatus status = RegisterToolboxObjectClass(classId,
												 NULL,
												 GetEventTypeCount (eventList),
												 eventList,
												 macEventHandler,
												 this,
												 &controlClass);
	if (status == noErr)
		controlSpec.u.classRef = controlClass;
#endif
}

//----------------------------------------------------------------------------
MultiGUIEditor::~MultiGUIEditor()
{
	int i;
	char tempstr[32];
	vector<AEffEditor *>::iterator it;

	i = 0;

	for(it=editors.begin();it!=editors.end();it++)
	{
		delete *it; //Deletes the actual editors.

#ifdef WIN32
		//Unregisters the window class.
		//sprintf(tempstr, "MGUIWinClass%d", i);
		sprintf(tempstr, "MGUIWinClass%08x%d", reinterpret_cast<int>(this), i);
		UnregisterClass(tempstr, static_cast<HINSTANCE>(hInstance));
#endif

		i++;
	}
	editors.clear(); //Removes the entries from _editors (is this necessary?).

#ifdef MACX
	//Unregister our HIView object.
	UnregisterToolboxObjectClass((ToolboxObjectClassRef)controlSpec.u.classRef);
#endif

#ifdef WIN32
	//sprintf(tempstr, "MultiGUIWindow%08x", static_cast<HINSTANCE>(hInstance));
	sprintf(tempstr, "MultiGUIWindow%08x", reinterpret_cast<int>(this));
	//unregisters the window class
	UnregisterClass(tempstr, static_cast<HINSTANCE>(hInstance));
#endif
}

//----------------------------------------------------------------------------
bool MultiGUIEditor::open(void *ptr)
{
	vector<AEffEditor *>::iterator it_ed;
	vector<NiallsRect>::iterator it_rect;

	systemWindow = ptr;

#ifdef WIN32
	int i;
	char tempstr[32];
	HWND tempHWnd2;
	HWND parentHWnd = static_cast<HWND>(ptr);

	//create the main window...  (This seems to be necessary for Tracktion...)
	//sprintf(tempstr, "MultiGUIWindow%08x", static_cast<HINSTANCE>(hInstance));
	sprintf(tempstr, "MultiGUIWindow%08x", reinterpret_cast<int>(this));
	tempHWnd = CreateWindowEx(0,				   //extended window style
							  tempstr,	   //pointer to registered class name
							  "MultiGUIEditor",	   //pointer to window name
							  WS_CHILD|
							  WS_VISIBLE,	   //window style
							  0,				   //horizontal position of window
							  0,				   //vertical position of window
							  (_rect.right-_rect.left),//window width
							  (_rect.bottom-_rect.top),//window height
							  parentHWnd,		   //handle to parent or owner window
							  NULL,				   //handle to menu, or child-window identifier
							  (HINSTANCE)hInstance,//handle to application instance
							  NULL);			   //pointer to window-creation data

	i = 0;
	it_ed = editors.begin();

	//Loop through the editor panels, and create the actual windows.
	for(it_rect=rects.begin();it_rect!=rects.end();++it_rect)
	{
		sprintf(tempstr, "MGUIWinClass%08x%d", reinterpret_cast<int>(this), i);
		tempHWnd2 = CreateWindowEx(0,				   //extended window style
								  tempstr,			   //pointer to registered class name
								  "MultiGUIEditor",	   //pointer to window name
								  WS_CHILD|WS_VISIBLE, //window style
								  it_rect->x,          //horizontal position of window
								  it_rect->y,          //vertical position of window
								  it_rect->width,      //window width
								  it_rect->height,     //window height
								  tempHWnd,		   //handle to parent or owner window
								  NULL,				   //handle to menu, or child-window identifier
								  (HINSTANCE)hInstance,//handle to application instance
								  NULL);			   //pointer to window-creation data

		//Now open the individual editor panels, with the newly-created hWnd.
		(*it_ed)->open(static_cast<void *>(tempHWnd2));

		++it_ed;
		++i;
	}
#elif MACX
	Rect temprect, temprect2;
	WindowGroupRef parentGroup;
	WindowRef tempwin;
	WindowRef window = static_cast<WindowRef>(systemWindow);
	WindowAttributes attr;
	HIRect bounds;

	temprect.left = _rect.left;
	temprect.right = _rect.right;
	temprect.top = _rect.top;
	temprect.bottom = _rect.bottom;

	parentGroup = GetWindowGroup(window);
	CreateWindowGroup(kWindowGroupAttrSelectAsLayer|
					  kWindowGroupAttrMoveTogether|
					  kWindowGroupAttrLayerTogether|
					  kWindowGroupAttrSharedActivation|
					  kWindowGroupAttrHideOnCollapse,
					  &mainGroup);
	SetWindowGroupParent(mainGroup, parentGroup);
	SetWindowGroup(window, mainGroup);

	//So we know where to move the child windows to.
	GetWindowBounds(window, kWindowContentRgn, &temprect2);

	it_ed = editors.begin();
	//Loop through the editor panels, and create the actual windows.
	for(it_rect=rects.begin();it_rect!=rects.end();it_rect++)
	{
		temprect.left = temprect2.left + it_rect->x;
		temprect.top = temprect2.top + it_rect->y;
		temprect.right = temprect2.left + (it_rect->x + it_rect->width);
		temprect.bottom = temprect2.top + (it_rect->y + it_rect->height);
		CreateNewWindow(kOverlayWindowClass,
						kWindowStandardHandlerAttribute|
						kWindowCompositingAttribute|
						kWindowNoShadowAttribute/*|
						kWindowNoActivatesAttribute|	//Not sure if these lines are necessary...
						kWindowDoesNotCycleAttribute*/,
						&temprect,
						&tempwin);
		//Not sure if the following line is actually necessary...
		//SetWindowActivationScope(tempwin, kWindowActivationScopeAll);

		ShowWindow(tempwin);
		SetWindowGroup(tempwin, mainGroup);

		systemWindows.push_back(static_cast<void *>(tempwin));
		(*it_ed)->open(static_cast<void *>(tempwin));
		++it_ed;
	}

	//If the host's provided us with a composited window, add an HIView to
	//the window, since this is what the latest vstsdk requires.
	GetWindowAttributes((WindowRef)window, &attr);
	if(attr & kWindowCompositingAttribute)
	{
		HIViewRef contentView;
		if(HIViewFindByID(HIViewGetRoot((WindowRef)window),
						  kHIViewWindowContentID,
						  &contentView) == noErr)
		{
			Rect r = {0,
					  0,
					  (_rect.right-_rect.left),
					  (_rect.bottom-_rect.top)};

			//Create our control/HIView
			CreateCustomControl(NULL, &r, &controlSpec, NULL, &controlRef);

			//Are these necessary?
			SetControlDragTrackingEnabled(controlRef, true);
			SetAutomaticControlDragTrackingEnabledForWindow(window, true);

			//Search for parent HIView.
			if(HIViewFindByID(HIViewGetRoot(window),
							  kHIViewWindowContentID,
							  &contentView) == noErr)
			{
				//Add our HIView to the parent.
				HIViewAddSubview (contentView, controlRef);

				//Set our HIView's size.
				//Is all of this necessary?
				skipPosition = true;
				HIViewGetFrame(controlRef, &bounds);
				HIViewConvertPoint(&(bounds.origin), contentView, controlRef);
				bounds.size.width = _rect.right-_rect.left;
				bounds.size.height = _rect.bottom-_rect.top;
				HIViewSetFrame(controlRef, &bounds);
			}
		}
	}
#endif

	return true;
}

//----------------------------------------------------------------------------
void MultiGUIEditor::close()
{
	vector<AEffEditor *>::iterator it_ed;
	vector<void *>::iterator it_syswin;

	for(it_ed=editors.begin();it_ed!=editors.end();++it_ed)
	{
		//close the individual editor panels
		(*it_ed)->close();
	}

#ifdef WIN32
	for(it_syswin=systemWindows.begin();it_syswin!=systemWindows.end();++it_syswin)
	{
		//destroy the individual windows
		DestroyWindow(static_cast<HWND>(*it_syswin));
		*it_syswin = 0;
	}
#elif MACX
	for(it_syswin=systemWindows.begin();it_syswin!=systemWindows.end();++it_syswin)
	{
		DisposeWindow(static_cast<WindowRef>(*it_syswin));
	}
	systemWindows.clear();
	ReleaseWindowGroup(mainGroup);

	HIViewRemoveFromSuperview(controlRef);

	if(controlRef)
		DisposeControl(controlRef);
#endif
}

//----------------------------------------------------------------------------
void MultiGUIEditor::idle()
{
	vector<AEffEditor *>::iterator it_ed;

	for(it_ed=editors.begin();it_ed!=editors.end();++it_ed)
	{
		(*it_ed)->idle();
	}
}

/* (AEffEditor doesn't have a setParameter method, so you'll have to set this
	up yourself, in your MultiGUIEditor subclass...)
//----------------------------------------------------------------------------
void MultiGUIEditor::setParameter(long index, float value)
{
	vector<AEffEditor *>::iterator it_ed;

	for(it_ed=editors.begin();it_ed!=editors.end();++it_ed)
	{
		(*it_ed)->setParameter(index, value);
	}
}*/

#ifdef MACX
//----------------------------------------------------------------------------
void MultiGUIEditor::updateBounds(int x, int y)
{
	int tempHeight;
	Rect tempRect2;
	HIRect tempRect;
	Rect temprect, temprect2;
	vector<NiallsRect>::iterator it_rect;
	vector<void *>::iterator it_syswin;
	WindowRef window = static_cast<WindowRef>(systemWindow);

	if(skipPosition)
	{
		skipPosition = false;
		return;
	}

	//Get the new bounds of our dummy HIView.
	HIViewGetFrame(controlRef, &tempRect);

	//Invert tempRect's y-axis (honestly, who designed this API!).
	GetWindowBounds(window, kWindowContentRgn, &tempRect2);
	tempHeight = (tempRect2.bottom-tempRect2.top);
	tempRect.origin.y = tempHeight - tempRect.origin.y - tempRect.size.height;

	boundsX = (int)tempRect.origin.x;
	boundsY = (int)tempRect.origin.y;

	//So we know where to move the child windows to.
	GetWindowBounds(window, kWindowContentRgn, &temprect2);

	it_syswin = systemWindows.begin();
	//Make sure the parent window doesn't move with our windows.
	ChangeWindowGroupAttributes(mainGroup, 0, kWindowGroupAttrMoveTogether);
	//Loop through the editor panels, and create the actual windows.
	for(it_rect=rects.begin();it_rect!=rects.end();++it_rect)
	{
		temprect.left = temprect2.left + it_rect->x + boundsX;
		temprect.top = temprect2.top + it_rect->y + boundsY;
		temprect.right = temprect2.left + (it_rect->x + it_rect->width) + boundsX;
		temprect.bottom = temprect2.top + (it_rect->y + it_rect->height) + boundsY;

		MoveWindowStructure(static_cast<WindowRef>(*it_syswin),
										   temprect.left,
										   temprect.top);
		++it_syswin;
	}
	//Make sure all the windows move together again.
	ChangeWindowGroupAttributes(mainGroup, kWindowGroupAttrMoveTogether, 0);
}
#endif

//----------------------------------------------------------------------------
void MultiGUIEditor::addEditor(AEffEditor *editor, NiallsRect rect)
{
	vector<NiallsRect>::iterator it_rect;
	int width = 0;
	int height = 0;

	if(editor)
	{
		editors.push_back(editor);
		rects.push_back(rect);

#ifdef WIN32
		char tempstr[32];
		WNDCLASSEX winClass;

		//most of this stuff is copied from VSTGUI
		winClass.cbSize = sizeof(WNDCLASSEX);
		winClass.style = 0;
		winClass.lpfnWndProc = WndProc;
		winClass.cbClsExtra = 0;
		winClass.cbWndExtra = 0;
		winClass.hInstance = static_cast<HINSTANCE>(hInstance);
		winClass.hIcon = 0;
		winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		winClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		winClass.lpszMenuName = 0;
		//sprintf(tempstr, "MGUIWinClass%d", (editors.size()-1));
		sprintf(tempstr, "MGUIWinClass%08x%d", reinterpret_cast<int>(this), (editors.size()-1));
		winClass.lpszClassName = tempstr;
		winClass.hIconSm = NULL;

		//Register the window class (this is unregistered in the
		//MultiGUIEditor destructor).
		RegisterClassEx(&winClass);
#endif
#ifdef MACX
		
#endif

		//First set the size of the window to be the accumulated size of the
		//individual editor panels.

		//Set the width & height of the main window.
		for(it_rect=rects.begin();it_rect!=rects.end();++it_rect)
		{
			if((it_rect->x+it_rect->width) > width)
				width = (it_rect->x+it_rect->width);
			if((it_rect->y+it_rect->height) > height)
				height = (it_rect->y+it_rect->height);
		}

		_rect.right = width;
		_rect.bottom = height;
	}
}

//----------------------------------------------------------------------------
#ifdef WIN32
LONG WINAPI MultiGUIEditor::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//We shouldn't have to do anything here - it's up to the editor panels
	//to do stuff...
	return DefWindowProc(hwnd, message, wParam, lParam);
}
#endif

//----------------------------------------------------------------------------
#ifdef MACX
pascal OSStatus MultiGUIEditor::macEventHandler(EventHandlerCallRef handler,
											 EventRef event,
											 void *userData)
{
	OSStatus result = eventNotHandledErr;
	MultiGUIEditor *ed = static_cast<MultiGUIEditor *>(userData);
	UInt32 eventClass = GetEventClass(event);
	UInt32 eventType = GetEventKind(event);

	if(ed)
	{
		if((eventClass == kEventClassControl)||(eventClass == kEventClassWindow))
		{
			switch(eventType)
			{
				case kEventWindowBoundsChanged:
				case kEventControlBoundsChanged:
					Rect tempRect;
					OSStatus err;
					err = GetEventParameter(event,
									  kEventParamCurrentBounds, 
									  typeQDRectangle,
									  NULL,
									  sizeof(Rect),
									  NULL,
									  &tempRect);
					if(err == noErr)
						ed->updateBounds(tempRect.left, tempRect.top);
					else
						ed->updateBounds(0, 0);
					result = noErr;
					break;
				case kEventControlHitTest:
					{
						ControlPartCode part = 127;
						SetEventParameter(event, kEventParamControlPart, typeControlPartCode, sizeof(part), &part);
					}
					result = noErr;
					break;
				case kEventControlTrack:
					result = noErr;
					break;
				case kEventControlHiliteChanged:
					result = noErr;
					break;
				case kEventControlDraw:
					result = noErr;
					break;
				default:
					CallNextEventHandler(handler, event);
			}
		}
	}

	return result;
}

#endif
