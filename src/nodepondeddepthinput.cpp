#include "stdafx.h"
#include "headers.h"
#include "swmmcomponent.h"
#include "nodepondeddepthinput.h"
#include "spatial/octree.h"
#include "spatial/point.h"
#include "dataexchangecache.h"
#include "temporal/timedata.h"

using namespace HydroCouple::Spatial;
using namespace HydroCouple::SpatioTemporal;

NodePondedDepthInput::NodePondedDepthInput(const QString &id,
                                           Dimension *timeDimension,
                                           Dimension *geometryDimension,
                                           ValueDefinition *valueDefinition,
                                           SWMMComponent *modelComponent)
  :TimeGeometryInputDouble(id,
                           IGeometry::Point,
                           timeDimension,
                           geometryDimension,
                           valueDefinition,
                           modelComponent),
    m_SWMMComponent(modelComponent)
{

}

bool NodePondedDepthInput::setProvider(HydroCouple::IOutput *provider)
{
  if(AbstractInput::setProvider(provider))
  {
    ITimeTINComponentDataItem *timeTINComponentDataItem = nullptr;
    m_geometryMapping.clear();

    if((timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider)))
    {

      ITIN* tin = timeTINComponentDataItem->TIN();
      Octree *octree = new Octree(Octree::Octree2D, Octree::AlongEnvelopes, 10, 100);

      for(int i = 0; i < geometryCount(); i++)
      {
        IGeometry* node = geometry(i);
        octree->addGeometry(node);
      }

      for(int i = 0; i < tin->patchCount(); i++)
      {
        ITriangle *triangle = tin->triangle(i);
        std::vector<IGeometry*> colliding = octree->findCollidingGeometries(triangle);

        for(IGeometry* collidingG : colliding)
        {
          if(triangle->contains(collidingG))
          {
            m_geometryMapping[collidingG->index()] = i;
            break;
          }
        }
      }

      delete octree;

      return true;
    }
  }

  return false;
}

bool NodePondedDepthInput::canConsume(HydroCouple::IOutput *provider, QString &message) const
{
  ITimeTINComponentDataItem *timeTINComponentDataItem = nullptr;

  if((timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider)) &&
     timeTINComponentDataItem->meshDataType() == HydroCouple::Spatial::Centroid)
  {
    return true;
  }

  message = "Provider must be an ITimeTINComponentDataItem with a centroid mesh data type";

  return false;
}

void NodePondedDepthInput::retrieveValuesFromProvider()
{
  moveDataToPrevTime();
  int currentTimeIndex = m_times.size() - 1;
  m_times[currentTimeIndex]->setJulianDay(m_SWMMComponent->currentDateTime()->julianDay());

  provider()->updateValues(this);

  ITimeTINComponentDataItem *timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider());
  //  ITIN  *tin = timeTINComponentDataItem->TIN();
  int currentProviderTime = timeTINComponentDataItem->timeCount() -1;

  //  double missingValue = valueDefinition()->missingValue().toDouble();

  for(std::pair<int,int> geoMap : m_geometryMapping)
  {
    double d = 0;
    timeTINComponentDataItem->getValue(currentProviderTime, geoMap.second,0,0,&d);
    setValue(currentTimeIndex, geoMap.first, &d);
  }
}

void NodePondedDepthInput::applyData()
{
  int currentTimeIndex = m_times.size() - 1;


  for(std::pair<int,int> geoMap : m_geometryMapping)
  {
    HCPoint *nodeGeom = dynamic_cast<HCPoint*>(geometry(geoMap.first));
    double pondedDepth = 0;
    getValue(currentTimeIndex, geoMap.first, &pondedDepth);
    pondedDepth /= UCF(m_SWMMComponent->project(), LENGTH);

    TNode &node = m_SWMMComponent->project()->Node[nodeGeom->marker()];

    //check if surface has any water at all.
    if(pondedDepth >  0.05)
    {
      //retrieve flow from surface using weir
      if(node.type != NodeType::OUTFALL)
      {
        if(node.newDepth < node.fullDepth && node.type != NodeType::OUTFALL)
        {
          double a_in_w = node.area / node.perimeter;
          double flow = 0.0;

          if(pondedDepth <=  a_in_w)
          {
            double coeff = 3.22 + 0.44 * pondedDepth / node.fullDepth;
            flow = coeff * (node.perimeter - 0.003) * pow(pondedDepth + 0.003, 3.0 /2.0);
          }
          else
          {
            flow = node.orificeDischargeCoeff * node.area * sqrt(2.0 * 32.2 * pondedDepth);
          }

          double maxflowPossible = (pondedDepth * node.pondedArea) / m_SWMMComponent->m_timeStep;
          flow = std::min(maxflowPossible, flow);

          m_SWMMComponent->m_nodeSurfaceInflow[nodeGeom->marker()] = flow;
          addNodeLateralInflow(m_SWMMComponent->m_SWMMProject, nodeGeom->marker(), flow);
        }
        //otherwise
        else if(node.newDepth > node.fullDepth)
        {
          double depth = pondedDepth + node.fullDepth;
          addNodeDepth(m_SWMMComponent->m_SWMMProject, nodeGeom->marker(), depth);
        }
      }
      else if(pondedDepth > 0.25)
      {
        double depth = pondedDepth;
        addNodeDepth(m_SWMMComponent->m_SWMMProject, nodeGeom->marker(), depth);
      }
    }
  }
}
