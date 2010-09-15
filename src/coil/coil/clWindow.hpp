/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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
#pragma once

#include <vector>

#include <coil/coilMaster.hpp>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.h>
#include <CL/cl.hpp>

#include <coil/Maths/Maths.h>
#include <coil/extcode/vector2.hpp>
#include <coil/extcode/static_assert.hpp>

#include <magnet/GL/light.hpp>
#include <magnet/GL/CLBuffer.hpp>
#include <magnet/GL/shadowShader.hpp>
#include <magnet/GL/blur.hpp>
#include <magnet/GL/downsample.hpp>
#include <magnet/GL/shadowFBO.hpp>
#include <magnet/GL/viewPort.hpp>
#include <magnet/GL/FBO.hpp>

#include <coil/RenderObj/RenderObj.hpp>


class CLGLWindow : public CoilWindow
{
public:
  CLGLWindow(int setWidth, int setHeight,
	     int setInitPositionX, int setInitPositionY,
	     std::string title,
	     cl::Platform& plat,
	     bool hostTransfers = false);

  ~CLGLWindow();
  
  virtual void CallBackDisplayFunc(void); 
  virtual void CallBackIdleFunc(void);
  void CallBackReshapeFunc(int w, int h);    

  cl::Platform& getCLPlatform() { return _clplatform; }
  cl::Context& getCLContext() { return  _clcontext; }
  cl::Device& getCLDevice() { return  _cldevice; }
  cl::CommandQueue& getCLCommandQueue() { return  _clcmdq; }

  const std::string& getWindowTitle() const { return windowTitle; }
  void setWindowtitle(const std::string& newtitle);
  
  void displayFPS(bool enable = true);

  void addRenderObj(RenderObj* nObj) { RenderObjects.push_back(nObj); }

  bool HostTransferModeAllowed() { return _hostTransfers; }
  
  template<class T>  
  T& addRenderObj() {STATIC_ASSERT(false,'Check Arg Types'); }

  template<class T, class T1> 
  T& addRenderObj(T1) { STATIC_ASSERT(false,'Check Arg Types');}

  template<class T, class T1, class T2> 
  T& addRenderObj(T1, T2) { STATIC_ASSERT(false,'Check Arg Types'); }

  template<class T, class T1, class T2, class T3> 
  T& addRenderObj(T1, T2, T3) { STATIC_ASSERT(false,'Check Arg Types'); }

  template<class T, class T1, class T2, class T3, class T4> 
  T& addRenderObj(T1, T2, T3, T4) { STATIC_ASSERT(false,'Check Arg Types'); }

  template<class T, class T1, class T2, class T3, class T4, class T5> 
  T& addRenderObj(T1, T2, T3, T4, T5) { STATIC_ASSERT(false,'Check Arg Types'); }

  template<class T, class T1, class T2, class T3, class T4, class T5, class T6> 
  T& addRenderObj(T1, T2, T3, T4, T5, T6) { STATIC_ASSERT(false,'Check Arg Types'); }

  inline cl::CommandQueue& getCommandQueue() { return _clcmdq; } 

  inline const int& getLastFrameTime() const { return _lastFrameTime; }
protected:
  magnet::GL::shadowShader _shadowShader;
  magnet::GL::downsampleFilter _downsampleFilter;
  magnet::GL::shadowFBO _shadowFBO;


  cl::Platform _clplatform;
  cl::Context _clcontext;
  cl::Device _cldevice;
  cl::CommandQueue _clcmdq;

  size_t _height, _width;
  int _windowX, _windowY;

  std::vector<RenderObj*> RenderObjects;

  void CallBackSpecialUpFunc(int key, int x, int y);
  void CallBackSpecialFunc(int key, int x, int y);
  void CallBackKeyboardFunc(unsigned char key, int x, int y);
  void CallBackKeyboardUpFunc(unsigned char key, int x, int y);
  void CallBackMouseWheelFunc(int button, int dir, int x, int y);
  void CallBackMouseFunc(int button, int state, int x, int y);
  void CallBackMotionFunc(int x, int y);

private:
  void CameraSetup();

  void initOpenGL();
  void initOpenCL();

  void drawAxis();

  void drawScene();

  enum KeyStateType
    {
      DEFAULT = 0,
      LEFTMOUSE = 1,
      RIGHTMOUSE = 2,
      MIDDLEMOUSE = 4
    };

  int keyState;
  
  std::string windowTitle;
  bool FPSmode;
  size_t frameCounter;


  int _currFrameTime;
  int _lastFrameTime;
  int _FPStime; 

  magnet::GL::viewPort _viewPortInfo;
    
  bool keyStates[256];

  float _mouseSensitivity; 
  float _moveSensitivity;
 
  int _oldMouseX, _oldMouseY;
  int _specialKeys;

  bool _hostTransfers;

  bool _shadows;

  magnet::GL::lightInfo _light0;
};
