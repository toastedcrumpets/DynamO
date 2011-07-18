/*  dynamo:- Event driven molecular dynamics simulator 
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

#include <coil/RenderObj/console.hpp>
#include <magnet/exception.hpp>
#include <magnet/clamp.hpp>
#include <coil/glprimatives/arrow.hpp>

#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/freeglut.h>

extern const unsigned char _binary_coilfont_ttf_start[];
extern const unsigned char _binary_coilfont_ttf_end[];

namespace coil {
  void 
  Console::initOpenGL() 
  {
    _glutLastTime = glutGet(GLUT_ELAPSED_TIME);

    resize(_viewPort->getWidth(), _viewPort->getHeight());

    _axis.init();
    _grid.init(10,10);
  }

  void 
  Console::resize(size_t width, size_t height)
  {
    //_consoleLayout->SetLineLength(width);
  }

  void 
  Console::interfaceRender()
  {
    //Only draw if the console has something in it or if it's visible
    if (_consoleEntries.empty() || !_visible) return;

    //Disable anything that might affect the rastering 
    glDisable(GL_DEPTH_TEST);

    using namespace magnet::GL;
    Context& context = Context::getContext();
    //Draw the console in orthograpic projection
    context.setViewMatrix(GLMatrix::identity());
    context.setProjectionMatrix(GLMatrix::identity());
    context.cleanupAttributeArrays();

//    if (_showConsole->get_active())
//      {
//	float lineHeight = _consoleFont->FaceSize() / (0.5f * _viewPort->getHeight());
//	float consoleHeight = 1.0f - lineHeight;
//	
//	//Calculate how long since the last redraw
//	int tdelta = glutGet(GLUT_ELAPSED_TIME) - _glutLastTime;
//	_glutLastTime = glutGet(GLUT_ELAPSED_TIME);
//	
//	glColor3f(_consoleTextColor[0], _consoleTextColor[1], 
//		  _consoleTextColor[2]);
//	
//	glRasterPos3f(-1.0, consoleHeight, 0);
//	_consoleLayout->Render(_consoleEntries.front().second.c_str());
//	consoleHeight -= lineHeight;
//	
//	for (std::list<consoleEntry>::iterator iPtr = ++_consoleEntries.begin();
//	     iPtr != _consoleEntries.end();)
//	  {
//	    //Fade the color based on it's time in the queue
//	    glColor4f(_consoleTextColor[0], _consoleTextColor[1], 
//		      _consoleTextColor[2], 1.0f - iPtr->first / 1000.0f);
//	    glRasterPos3f(-1, consoleHeight, 0);
//	    _consoleLayout->Render(iPtr->second.c_str());
//	    iPtr->first += tdelta;
//	    consoleHeight -= lineHeight;
//	    
//	    std::list<consoleEntry>::iterator prev = iPtr++;
//	    //If this element is invisible, erase it
//	    if (prev->first > 1000) _consoleEntries.erase(prev);
//	  }
//      }

    if (_showAxis->get_active())
      {
	/////////////////RENDER THE AXIS//////////////////////////////////////////////

	const GLdouble nearPlane = 0.1,
	  axisScale = 0.09;
    
	//The axis is in a little 100x100 pixel area in the lower left
	GLint viewportDim[4];
	glGetIntegerv(GL_VIEWPORT, viewportDim);
	glViewport(0,0,100,100);
    
	context.setProjectionMatrix(GLMatrix::identity());
	context.setViewMatrix(GLMatrix::identity());
	context.color(0.5f,0.5f,0.5f,0.8f);

	glBegin(GL_QUADS);
	glVertex3f(-1,-1, 0);
	glVertex3f( 1,-1, 0);
	glVertex3f( 1, 1, 0);
	glVertex3f(-1, 1, 0);
	glEnd();
    
	context.setProjectionMatrix
	  (GLMatrix::perspective(45, 1, nearPlane, 1000));

	context.setViewMatrix
	  (GLMatrix::translate(0, 0, -(nearPlane + axisScale))
	   * GLMatrix::rotate(_viewPort->getTilt(), Vector(1, 0, 0))
	   * GLMatrix::rotate(_viewPort->getPan(), Vector(0, 1, 0))
	   * GLMatrix::scale(axisScale, axisScale, axisScale)
	   );
    
	glLineWidth(2.0f);
	_axis.glRender();
    
	//Do the axis labels
	//glColor3f(1,1,1);
	//glRasterPos3f( 0.5,-0.5,-0.5);
	//_consoleFont->Render("X");
	//glRasterPos3f(-0.5, 0.5,-0.5);
	//_consoleFont->Render("Y");
	//glRasterPos3f(-0.5,-0.5, 0.5);
	//_consoleFont->Render("Z");
	//glViewport(viewportDim[0], viewportDim[1], viewportDim[2], viewportDim[3]);
      }    

    //Restore GL state
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix ();
  }

  void Console::glRender()
  {
    if (_showGrid->get_active())
      {
	using namespace magnet::GL;
	Context& context = Context::getContext();

	GLMatrix old_model_view = context.getViewMatrix();
	GLMatrix model_view 
	  = old_model_view 
	  * GLMatrix::translate(_viewPort->getViewPlanePosition())
	  * GLMatrix::rotate(-_viewPort->getPan(), Vector(0, 1, 0))
	  * GLMatrix::rotate(-_viewPort->getTilt(), Vector(1, 0, 0));

	context.color(1,1,1,1);
	//Back face
	context.setViewMatrix(model_view 
			      * GLMatrix::scale(_viewPort->getScreenPlaneWidth(), 
						_viewPort->getScreenPlaneHeight(), 
						1)
			      * GLMatrix::translate(0,0,-1));
	_grid.glRender();

	//Sides
	context.setViewMatrix(model_view 
			      * GLMatrix::scale(_viewPort->getScreenPlaneWidth(), 
						_viewPort->getScreenPlaneHeight(), 
						1)
			      * GLMatrix::rotate(90, Vector(0,1,0))
			      * GLMatrix::translate(0.5,0,-0.5));
	_grid.glRender();
	//right
	context.setViewMatrix(context.getViewMatrix() * 
			      GLMatrix::translate(0,0,1));
	_grid.glRender();


	//Top and bottom
	context.setViewMatrix(model_view
			      * GLMatrix::scale(_viewPort->getScreenPlaneWidth(), 
						_viewPort->getScreenPlaneHeight(),
						1)
			      * GLMatrix::rotate(90, Vector(1,0,0))
			      * GLMatrix::translate(0, -0.5, -0.5));
	_grid.glRender();
	//bottom
	context.setViewMatrix(context.getViewMatrix() * 
			      GLMatrix::translate(0,0,1));
	_grid.glRender();
	context.setViewMatrix(old_model_view);
      }
  }


  void
  Console::initGTK()
  {
    _optList.reset(new Gtk::VBox);//The Vbox of options   

    {
      _showGrid.reset(new Gtk::CheckButton("Show viewing grid"));
      _showGrid->set_active(false);
      _optList->add(*_showGrid); _showGrid->show();
    }

    {
      _showConsole.reset(new Gtk::CheckButton("Show console"));
      _showConsole->set_active(false);
      _showConsole->set_sensitive(false);
      _optList->add(*_showConsole); _showConsole->show();
    }

    {
      _showAxis.reset(new Gtk::CheckButton("Show axis"));
      _showAxis->set_active(true);
      _optList->add(*_showAxis); _showAxis->show();
    }

    _optList->show();
    guiUpdate();
  }

  void
  Console::showControls(Gtk::ScrolledWindow* win)
  {
    win->remove();
    _optList->unparent();
    win->add(*_optList);
    win->show();
  }

  void 
  Console::guiUpdate()
  {}

}
