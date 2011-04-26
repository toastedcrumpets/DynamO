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

#ifdef DYNAMO_visualizer
# include "renderobjs/spheres.hpp"
# include "../liouvillean/CompressionL.hpp"
#endif

#include "include.hpp"
#include "../ranges/1range.hpp"
#include "../ranges/1RAll.hpp"
#include "../../simulation/particle.hpp"
#include "../../base/is_simdata.hpp"
#include "../units/units.hpp"
#include <boost/foreach.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <cstring>


void 
SpPoint::operator<<(const magnet::xml::Node& XML)
{
  range.set_ptr(CRange::getClass(XML,Sim));
  
  try {
    mass = XML.getAttribute("Mass").as<double>() * Sim->dynamics.units().unitMass();
    spName = XML.getAttribute("Name");
    intName = XML.getAttribute("IntName");
  } 
  catch (boost::bad_lexical_cast &)
    {
      M_throw() << "Failed a lexical cast in SpPoint";
    }
}

void 
SpPoint::outputXML(xml::XmlStream& XML) const
{
  XML << xml::attr("Mass") 
      << mass / Sim->dynamics.units().unitMass()
      << xml::attr("Name") << spName
      << xml::attr("IntName") << intName
      << xml::attr("Type") << "Point"
      << range;
}

void
SpPoint::initialise()
{ 
  if (IntPtr == NULL)
    M_throw() << "SpPoint missing a matching interaction";
}


#ifdef DYNAMO_visualizer

magnet::thread::RefPtr<RenderObj>& 
SpPoint::getCoilRenderObj() const
{
  if (!_renderObj.isValid())
    {
      _renderObj = new SphereParticleRenderer(range->size(), "Species: " + spName,
					      magnet::function::MakeDelegate(this, &SpPoint::updateColorObj));
      _coil = new CoilRegister;
    }

  return _renderObj;
}

void
SpPoint::updateRenderData(magnet::CL::CLGLState& CLState) const
{
  if (!_renderObj.isValid())
    M_throw() << "Updating before the render object has been fetched";
  
  std::vector<cl_float4>& particleData 
    = _renderObj.as<SphereParticleRenderer>()._particleData;

  ///////////////////////POSITION DATA UPDATE
  //Check if the system is compressing and adjust the radius scaling factor
  float factor = 1;
  if (Sim->dynamics.liouvilleanTypeTest<LCompression>())
    factor = (1 + static_cast<const LCompression&>(Sim->dynamics.getLiouvillean()).getGrowthRate() * Sim->dSysTime);
 
  double diam = getIntPtr()->maxIntDist() * factor;
  
  size_t sphID(0);
  BOOST_FOREACH(unsigned long ID, *range)
    {
      Vector pos = Sim->particleList[ID].getPosition();
      
      Sim->dynamics.BCs().applyBC(pos);
      
      for (size_t i(0); i < NDIM; ++i)
	particleData[sphID].s[i] = pos[i];
      
      particleData[sphID++].w = diam * 0.5;
    }

  if (_renderObj.as<SphereParticleRenderer>().getRecolorOnUpdate())
    updateColorObj(CLState);
  
  _coil->getInstance().getTaskQueue().queueTask(magnet::function::Task::makeTask(&SphereParticleRenderer::sendRenderData,
										 &(_renderObj.as<SphereParticleRenderer>()), CLState));
}

void 
SpPoint::updateColorObj(magnet::CL::CLGLState& CLState) const
{
  std::vector<cl_uchar4>& particleColorData 
    = _renderObj.as<SphereParticleRenderer>()._particleColorData;

  bool colorIfSleeping = _renderObj.as<SphereParticleRenderer>().getColorIfStatic();
  cl_uchar4 sleepcolor;
  for (size_t cc(0); cc < 4; ++cc)
    sleepcolor.s[cc] = _renderObj.as<SphereParticleRenderer>().getColorStatic().s[cc];

  switch (_renderObj.as<SphereParticleRenderer>().getDrawMode())
    {
    case SphereParticleRenderer::SINGLE_COLOR:
      {
	cl_uchar4 color;
	for (size_t cc(0); cc < 4; ++cc)
	  color.s[cc] = _renderObj.as<SphereParticleRenderer>().getColorFixed().s[cc];

	size_t sphID(0);
	BOOST_FOREACH(unsigned long ID, *range)
	  {
	    if (colorIfSleeping && !Sim->particleList[ID].testState(Particle::DYNAMIC))
	      for (size_t cc(0); cc < 4; ++cc)
		particleColorData[sphID].s[cc] = sleepcolor.s[cc];
	    else
	      for (size_t cc(0); cc < 4; ++cc)
		particleColorData[sphID].s[cc] = color.s[cc];

	    ++sphID;
	  }
	break;
      }
    case SphereParticleRenderer::COLOR_BY_ID:
      {
	size_t np = range->size();

	size_t sphID(0);
	BOOST_FOREACH(unsigned long ID, *range)
	  {
	    if (colorIfSleeping && !Sim->particleList[ID].testState(Particle::DYNAMIC))
	      for (size_t cc(0); cc < 4; ++cc)
		particleColorData[sphID].s[cc] = sleepcolor.s[cc];
	    else
	      _renderObj.as<SphereParticleRenderer>().map(particleColorData[sphID], ((float)(sphID)) / np);

	    ++sphID;
	  }
	break;
      }
    case SphereParticleRenderer::COLOR_BY_SPEED:
      {
	double scaleV = _renderObj.as<SphereParticleRenderer>().getScaleV() 
	  * Sim->dynamics.units().unitVelocity();
	size_t sphID(0);
	BOOST_FOREACH(unsigned long ID, *range)
	  {
	    if (colorIfSleeping && !Sim->particleList[ID].testState(Particle::DYNAMIC))
	      for (size_t cc(0); cc < 4; ++cc)
		particleColorData[sphID].s[cc] = sleepcolor.s[cc];
	    else
	      {
		Vector vel = Sim->particleList[ID].getVelocity();
		_renderObj.as<SphereParticleRenderer>()
		  .map(particleColorData[sphID], magnet::clamp(vel.nrm() / scaleV, 0.0, 1.0));
	      }
	    ++sphID;
	  }
	break;
      }
    default:
      M_throw() << "Not Implemented";
    }

  _coil->getInstance().getTaskQueue().queueTask(magnet::function::Task::makeTask(&SphereParticleRenderer::sendColorData, 
										 &(_renderObj.as<SphereParticleRenderer>()), CLState));
}
#endif
