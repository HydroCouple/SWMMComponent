#include "stdafx.h"
#include "headers.h"
#include "swmmcomponent.h"
#include "nodewseinput.h"
#include "spatial/octree.h"
#include "spatial/point.h"
#include "dataexchangecache.h"
#include "temporal/timedata.h"

using namespace HydroCouple::Spatial;
using namespace HydroCouple::SpatioTemporal;

NodeWSEInput::NodeWSEInput(const QString &id,
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

bool NodeWSEInput::setProvider(HydroCouple::IOutput *provider)
{
  if(AbstractInput::setProvider(provider))
  {
    ITimeTINComponentDataItem *timeTINComponentDataItem = nullptr;

    m_geometryMapping.clear();

    if((timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider)))
    {

      ITIN* tin = timeTINComponentDataItem->TIN();
      Octree *octree = new Octree(Octree::Octree2D, Octree::AlongEnvelopes, 10, 50);
      std::unordered_map<IGeometry*,int> triangleIndexes;

      for(int i = 0; i < tin->patchCount(); i++)
      {
        ITriangle *triangle = tin->triangle(i);
        triangleIndexes[triangle] = i;
        octree->addGeometry(triangle);
      }

      for(int i = 0; i < geometryCount(); i++)
      {
        IGeometry* node = geometry(i);
        std::vector<IGeometry*> colliding = octree->findCollidingGeometries(node);

        if(colliding.size())
        {
          for(IGeometry* collidingG : colliding)
          {
            collidingG->contains(node);
            m_geometryMapping[i] = triangleIndexes[collidingG];
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

bool NodeWSEInput::canConsume(HydroCouple::IOutput *provider, QString &message) const
{
  ITimeTINComponentDataItem *timeTINComponentDataItem = nullptr;

  if((timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider)) &&
     timeTINComponentDataItem->meshDataType() == HydroCouple::Spatial::Node)
  {
    return true;
  }

  message = "Provider must be an ITimeTINComponentDataItem with a Node mesh data type";

  return false;
}

void NodeWSEInput::retrieveValuesFromProvider()
{
  moveDataToPrevTime();
  int currentTimeIndex = m_times.size() - 1;
  m_times[currentTimeIndex]->setModifiedJulianDay(m_SWMMComponent->currentDateTime()->modifiedJulianDay());

  provider()->updateValues(this);

  ITimeTINComponentDataItem *timeTINComponentDataItem = dynamic_cast<ITimeTINComponentDataItem*>(provider());
  ITIN  *tin = timeTINComponentDataItem->TIN();
  int currentProviderTime = timeTINComponentDataItem->timeCount() -1;

  //  double missingValue = valueDefinition()->missingValue().toDouble();

  for(std::pair<int,int> geoMap : m_geometryMapping)
  {
    ITriangle *triangle = tin->triangle(geoMap.second);
    IPoint *node = dynamic_cast<IPoint*>(geometry(geoMap.first));

    double z1 = 0, z2 = 0, z3 = 0;
    timeTINComponentDataItem->getValue(currentProviderTime, geoMap.second,0,0,&z1);
    timeTINComponentDataItem->getValue(currentProviderTime, geoMap.second,1,0,&z2);
    timeTINComponentDataItem->getValue(currentProviderTime, geoMap.second,2,0,&z3);


    Vect v1(*triangle->edge()->orig());
    Vect v2(*triangle->edge()->dest());
    Vect v3(*triangle->edge()->leftNext()->dest());

    v1.v[2] = z1; v2.v[2] = z2; v3.v[2] = z3;

    Vect v11 = v3 - v2;
    Vect v22 = v1 - v2;

    Vect n =  Vect::crossProduct(v11,v22);
    n.normalize();
    double d = -n.x() * v1.x() - n.y() * v1.y() - n.z() * v1.z();
    double z = (n.x() * node->x() + n.y() * node->y() + d) / -n.z();
    setValue(currentTimeIndex, geoMap.first, &z);
  }
}

void NodeWSEInput::applyData()
{
  int currentTimeIndex = m_times.size() - 1;
  double totalInflow = 0.0;

  for(std::pair<int,int> geoMap : m_geometryMapping)
  {
    IPoint *nodeGeom = dynamic_cast<IPoint*>(geometry(geoMap.first));
    double z = 0;
    getValue(currentTimeIndex, geoMap.first, &z);
    z /= UCF(m_SWMMComponent->project(), LENGTH);

    char *nodeId = const_cast<char*>(nodeGeom->id().toStdString().c_str());
    int nodeIndex = project_findObject(m_SWMMComponent->project(),NODE, nodeId);

    TNode &node = m_SWMMComponent->project()->Node[nodeIndex];
    double dp = (z - node.invertElev);
    double pondedDepth = dp - node.fullDepth;

    //check if surface has any water at all.
    if(pondedDepth >  0.0328084)
    {
      //retrieve flow from surface using weir
      if(node.newDepth < node.fullDepth && node.type != NodeType::OUTFALL)
      {
        double a_in_w = node.area / node.perimeter;
        double flow = 0.0;

        if(pondedDepth <=  a_in_w)
        {
          double coeff = 3.22 + 0.44 * pondedDepth / node.fullDepth;
          flow = coeff * (node.perimeter) * pow(pondedDepth + 0.003 , 3.0 /2.0);
        }
        else
        {
          flow = node.orificeDischargeCoeff * node.area * sqrt(2.0 * 32.2 * pondedDepth);
        }

        double maxflowPossible = (pondedDepth * node.pondedArea) / m_SWMMComponent->m_timeStep;
        flow = std::min(maxflowPossible, flow);

        m_SWMMComponent->m_surfaceInflow[nodeIndex] = flow;
        addNodeLateralInflow(m_SWMMComponent->m_SWMMProject, nodeIndex, flow);
        totalInflow += flow;

      }
      //otherwise
      else if(node.newDepth > node.fullDepth)
      {
        double depth = pondedDepth + node.fullDepth;
        addNodeDepth(m_SWMMComponent->m_SWMMProject, nodeIndex, depth);
      }
    }

    if(totalInflow)
      printf("SWMM Inflow Total: %f\n", totalInflow);
  }
}
