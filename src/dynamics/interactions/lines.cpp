/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2008  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "lines.hpp"
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iomanip>
#include "../../base/is_exception.hpp"
#include "../../extcode/xmlwriter.hpp"
#include "../../extcode/xmlParser.h"
#include "../../dynamics/interactions/intEvent.hpp"
#include "../liouvillean/liouvillean.hpp"
#include "../liouvillean/OrientationL.hpp"
#include "../units/units.hpp"
#include "../../base/is_simdata.hpp"
#include "../2particleEventData.hpp"
#include "../BC/BC.hpp"
#include "../ranges/1range.hpp"
#include "../../schedulers/scheduler.hpp"
#include "../NparticleEventData.hpp"

CILines::CILines(DYNAMO::SimData* tmp, Iflt nd, 
		 Iflt ne, C2Range* nR):
  CICapture(tmp, nR),
  length(nd), l2(nd*nd), e(ne) 
{}

CILines::CILines(const XMLNode& XML, DYNAMO::SimData* tmp):
  CICapture(tmp, NULL)
{
  operator<<(XML);
}

void 
CILines::initialise(size_t nID)
{
  if (dynamic_cast<const CLNOrientation*>(&(Sim->Dynamics.Liouvillean()))
      == NULL)
    D_throw() << "Interaction requires an orientation capable Liouvillean.";
  
  ID = nID; 
  
  CICapture::initCaptureMap();
}

void 
CILines::operator<<(const XMLNode& XML)
{ 
  if (strcmp(XML.getAttribute("Type"),"Lines"))
    D_throw() << "Attempting to load Lines from non Lines entry";
  
  range.set_ptr(C2Range::loadClass(XML,Sim));
  
  try 
    {
      length = Sim->Dynamics.units().unitLength() * 
	boost::lexical_cast<Iflt>(XML.getAttribute("Length"));
      
      l2 = length * length;
      
      e = boost::lexical_cast<Iflt>(XML.getAttribute("Elasticity"));
      
      intName = XML.getAttribute("Name");
      
      CICapture::loadCaptureMap(XML);   
    }
  catch (boost::bad_lexical_cast &)
    {
      D_throw() << "Failed a lexical cast in CILines";
    }
}

Iflt 
CILines::maxIntDist() const 
{ return length; }

Iflt 
CILines::hardCoreDiam() const 
{ return 0.0; }

void 
CILines::rescaleLengths(Iflt scale) 
{ 
  length += scale * length;
  
  l2 = length * length;
}

CInteraction* 
CILines::Clone() const 
{ return new CILines(*this); }

CIntEvent 
CILines::getEvent(const CParticle &p1,
		  const CParticle &p2) const 
{
#ifdef DYNAMO_DEBUG
  if (!Sim->Dynamics.Liouvillean().isUpToDate(p1))
    D_throw() << "Particle 1 is not up to date";
  
  if (!Sim->Dynamics.Liouvillean().isUpToDate(p2))
    D_throw() << "Particle 2 is not up to date";

  if (p1 == p2)
    D_throw() << "You shouldn't pass p1==p2 events to the interactions!";
#endif 
  
  CPDData colldat(*Sim, p1, p2);
  
  if (isCaptured(p1, p2)) 
    {
      //Run this to determine when the spheres no longer intersect
      Sim->Dynamics.Liouvillean().SphereSphereOutRoot(colldat, l2);

      //colldat.dt has the upper limit of the line collision time
      //Lower limit is right now
      //Test for a line collision
      //Upper limit can be HUGE_VAL!
      if (Sim->Dynamics.Liouvillean().getLineLineCollision
	  (colldat, length, p1, p2))
	return CIntEvent(p1, p2, colldat.dt, CORE, *this);
      
      return CIntEvent(p1, p2, colldat.dt, WELL_OUT, *this);
    }
  else if (Sim->Dynamics.Liouvillean().SphereSphereInRoot(colldat, l2)) 
    return CIntEvent(p1, p2, colldat.dt, WELL_IN, *this);
  
  return CIntEvent(p1, p2, HUGE_VAL, NONE, *this);
}

void
CILines::runEvent(const CParticle& p1, 
		  const CParticle& p2,
		  const CIntEvent& iEvent) const
{
  switch (iEvent.getType())
    {
    case CORE:
      {
	++Sim->lNColl;
	//We have a line interaction! Run it
	C2ParticleData retval(Sim->Dynamics.Liouvillean().runLineLineCollision
			      (iEvent, e, length));

	Sim->signalParticleUpdate(retval);
	
	Sim->ptrScheduler->fullUpdate(p1, p2);
	
	BOOST_FOREACH(smrtPlugPtr<COutputPlugin> & Ptr, 
		      Sim->outputPlugins)
	  Ptr->eventUpdate(iEvent, retval);

	break;
      }
    case WELL_IN:
      {
	addToCaptureMap(p1, p2);

	//Unfortunately we cannot be smart as this well event may have
	//been pushed into both particles update lists, therefore we
	//must do a full update
	Sim->ptrScheduler->fullUpdate(p1, p2);

	Sim->freestreamAcc += iEvent.getdt();
	break;
      }
    case WELL_OUT:
      {
	removeFromCaptureMap(p1, p2);

	//Unfortunately we cannot be smart as this well event may have
	//been pushed into both particles update lists, therefore we
	//must do a full update
	Sim->ptrScheduler->fullUpdate(p1, p2);

	Sim->freestreamAcc += iEvent.getdt();
	break;
      }
    default:
      D_throw() << "Unknown collision type";
    }
}
   
void 
CILines::outputXML(xmlw::XmlStream& XML) const
{
  XML << xmlw::attr("Type") << "Lines"
      << xmlw::attr("Length") << length / Sim->Dynamics.units().unitLength()
      << xmlw::attr("Elasticity") << e
      << xmlw::attr("Name") << intName
      << range;

  CICapture::outputCaptureMap(XML);
}

bool 
CILines::captureTest(const CParticle& p1, const CParticle& p2) const
{
  CVector<> rij = p1.getPosition() - p2.getPosition();
  Sim->Dynamics.BCs().setPBC(rij);
  
  return (rij | rij) <= l2;
}

void
CILines::checkOverlaps(const CParticle& part1, const CParticle& part2) const
{}

void 
CILines::write_povray_desc(const DYNAMO::RGB& rgb, const size_t& specID, 
			   std::ostream& os) const
{
  try {
    dynamic_cast<const CLNOrientation&>(Sim->Dynamics.Liouvillean());
  }
  catch(std::bad_cast)
    {
      D_throw() << "Liouvillean is not an orientation liouvillean!";
    }
  
  BOOST_FOREACH(const size_t& pid, *(Sim->Dynamics.getSpecies()[specID].getRange()))
    {
      const CParticle& part(Sim->vParticleList[pid]);

      const CLNOrientation::rotData& 
	rdat(static_cast<const CLNOrientation&>
	     (Sim->Dynamics.Liouvillean()).getRotData(part));

      CVector<> pos(part.getPosition());
      Sim->Dynamics.BCs().setPBC(pos);

      CVector<> point(pos - 0.5 * length * rdat.orientation);
      
      os << "cylinder {\n <" << point[0];
      for (size_t iDim(1); iDim < NDIM; ++iDim)
	os << "," << point[iDim];

      point = pos + 0.5 * length * rdat.orientation;

      os << ">, \n <" << point[0];
      for (size_t iDim(1); iDim < NDIM; ++iDim)
	os << "," << point[iDim];

      os << ">, " << length *0.01 
	 << "\n texture { pigment { color rgb<" << rgb.R << "," << rgb.G 
	 << "," << rgb.B << "> }}\nfinish { phong 0.9 phong_size 60 }\n}\n";
    }

}
