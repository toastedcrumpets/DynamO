/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define __CL_ENABLE_EXCEPTIONS

#include <magnet/arg_share.hpp>
#include <coil/coilMaster.hpp>
#include <magnet/CL/CLGL.hpp>
#include <coil/coilWindow.hpp>
#include <GL/freeglut_ext.h>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

volatile size_t CoilRegister::_counter(0);
magnet::thread::Mutex CoilRegister::_mutex;

CoilMaster::CoilMaster():
  _runFlag(false),
  _coilReadyFlag(false),
  _GTKit(magnet::ArgShare::getInstance().getArgc(), magnet::ArgShare::getInstance().getArgv())
{}

CoilMaster::~CoilMaster(){
}

void CoilMaster::bootRenderThread()
{
  _runFlag = true;
  _coilThread = magnet::thread::Thread
    (magnet::function::Task::makeTask(&CoilMaster::coilThreadEntryPoint, this));
  
  //Spinlock waiting for the boot thread to come up
  while (!_coilReadyFlag) { smallSleep(); }
}
 
void CoilMaster::CallBackDisplayFunc(){
   int windowID = glutGetWindow();   

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackDisplayFunc();
}

void CoilMaster::CallBackCloseWindow()
{
  int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

  CoilMaster::getInstance()._viewPorts[windowID]->deinit(false);

  //Shutdown all windows, we can't yet recover from a window close
  //CoilMaster::getInstance().shutdownCoil();
}

void CoilMaster::CallBackKeyboardFunc(unsigned char key, int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackKeyboardFunc(key, x, y);
}

void CoilMaster::CallBackKeyboardUpFunc(unsigned char key, int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackKeyboardUpFunc(key, x, y);
}

void CoilMaster::CallBackMotionFunc(int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackMotionFunc(x, y);
}

void CoilMaster::CallBackMouseFunc(int button, int state, int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackMouseFunc(button, state, x, y);
}

void CoilMaster::CallBackMouseWheelFunc(int button, int dir, int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

  std::cerr << "Mouse Wheel event!";

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackMouseWheelFunc(button, dir, x, y);
}

void CoilMaster::CallBackPassiveMotionFunc(int x, int y){
   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackPassiveMotionFunc(x, y);
}

void CoilMaster::CallBackReshapeFunc(int w, int h){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackReshapeFunc(w, h);
}

void CoilMaster::CallBackSpecialFunc(int key, int x, int y){

  int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

  CoilMaster::getInstance()._viewPorts[windowID]->CallBackSpecialFunc(key, x, y);
}   

void CoilMaster::CallBackSpecialUpFunc(int key, int x, int y){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackSpecialUpFunc(key, x, y);
}   

void CoilMaster::CallBackVisibilityFunc(int visible){

   int windowID = glutGetWindow();

#ifdef DYNAMO_DEBUG
  if (!CoilMaster::getInstance()._viewPorts.count(windowID))
    M_throw() << "Missing viewport!";
#endif

   CoilMaster::getInstance()._viewPorts[windowID]->CallBackVisibilityFunc(visible);
}

void CoilMaster::CallGlutCreateWindow(const char * setTitle, CoilWindow * coilWindow){

   // Open new window, record its windowID , 

   int windowID = glutCreateWindow(setTitle);

   coilWindow->SetWindowID(windowID);

   // Store the address of new window in global array 
   // so CoilMaster can send events to propoer callback functions.

   // Hand address of universal static callback functions to Glut.
   // This must be for each new window, even though the address are constant.
   glutDisplayFunc(CallBackDisplayFunc);
   
   //Idling is handled in coilMasters main loop
   glutIdleFunc(NULL);
   glutKeyboardFunc(CallBackKeyboardFunc);
   glutKeyboardUpFunc(CallBackKeyboardUpFunc);
   glutSpecialFunc(CallBackSpecialFunc);
   glutSpecialUpFunc(CallBackSpecialUpFunc);
   glutMouseFunc(CallBackMouseFunc);
   glutMouseWheelFunc(CallBackMouseWheelFunc);
   glutMotionFunc(CallBackMotionFunc);
   glutPassiveMotionFunc(CallBackPassiveMotionFunc);
   glutReshapeFunc(CallBackReshapeFunc); 
   glutVisibilityFunc(CallBackVisibilityFunc);
   glutCloseFunc(CallBackCloseWindow);
}

void CoilMaster::CallGlutDestroyWindow(CoilWindow * coilWindow, bool andGlutDestroy)
{
  int windowID = coilWindow->GetWindowID();
  if (andGlutDestroy) glutDestroyWindow(windowID);
  CoilMaster::getInstance()._viewPorts.erase(windowID);
}

void CoilMaster::smallSleep()
{
  timespec sleeptime;
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 100000000;
  nanosleep(&sleeptime, NULL);
}

void 
CoilMaster::waitForShutdown()
{
  if (_coilThread.validTask()) _coilThread.join();
}

void CoilMaster::coilThreadEntryPoint()
{
  glutInit(&magnet::ArgShare::getInstance().getArgc(), magnet::ArgShare::getInstance().getArgv());
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

  try {
      //Register the idle function
    //Glib::signal_idle().connect(sigc::mem_fun(this, &CoilMaster::GTKIdleFunc));
    Glib::signal_timeout().connect(sigc::mem_fun(this, &CoilMaster::glutIdleTimeout), 30, Glib::PRIORITY_DEFAULT_IDLE);
    Glib::signal_timeout().connect(sigc::mem_fun(this, &CoilMaster::taskTimeout), 50, Glib::PRIORITY_LOW);
    
    
    _coilReadyFlag = true;
    _GTKit.run();
  } catch (std::exception& except)
    {
      std::cerr << "\nRender thread caught an exception\n"
		<< except.what()
		<< "\n As we're in a thread we can only exit(1)!";
      std::exit(1);
    } catch (...)
    {
      std::cerr << "\nRender thread caught an unknown exception!\n"
		<< "\n As we're in a thread we can only exit(1)!";
      std::exit(1);
    }
}

bool 
CoilMaster::taskTimeout()
{
  try {
    _coilQueue.drainQueue();
    
    if (!CoilMaster::getInstance().isRunning()) 
      {
	//The task queue should not get any more tasks as
	//CoilMaster::getInstance().isRunning() is false
	
	//! \todo{There is a race condition here if a window is added as coil is shutting down}
	{
	  magnet::thread::ScopedLock lock(_coilLock);
	  
	  _viewPorts.clear();
	}
	
	Gtk::Main::quit();
	
	//Run glutMainLoopEvent to let destroyed windows close
	glutMainLoopEvent();
      }
  } catch (cl::Error err)
    {
      std::cerr << "\nCoil caught an exception while performing its tasks\n"
		<< "An OpenCL error occured," << err.what()
		<< "\nError num of " << err.err()
		<< "\n As we're in a thread we can only exit(1)!";
      std::exit(1);
    } catch (std::exception& except)
    {
      std::cerr << "\nCoil caught an exception while performing its tasks\n"
		<< except.what();
      std::exit(1);
    } 

  return true;
}

bool 
CoilMaster::glutIdleTimeout()
{
  try {
    //Fire off a tick to glut
    glutMainLoopEvent();
    
    //for (std::map<int,magnet::thread::RefPtr<CoilWindow> >::iterator iPtr 
    //	   = CoilMaster::getInstance()._viewPorts.begin();
    //	 iPtr != CoilMaster::getInstance()._viewPorts.end(); ++iPtr)
    //  iPtr->second->CallBackIdleFunc();

  } catch (cl::Error err)
    {
      M_throw() << "An OpenCL error occured," << err.what()
		<< "\nError num of " << err.err() << "\n";
    }
  
  return true;
}

//Somewhere for the vtable to live
CoilWindow::CoilWindow():
  _readyFlag(false)
{}
