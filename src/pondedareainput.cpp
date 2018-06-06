#include "stdafx.h"
#include "pondedareainput.h"
#include "headers.h"
#include "swmmcomponent.h"
#include "spatial/octree.h"
#include "spatial/point.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

PondedAreaInput::PondedAreaInput(const QString &id, Dimension *geometryDimesion,
                                 ValueDefinition *valueDefinition, SWMMComponent *modelComponent):
  GeometryInputDouble(id,IGeometry::Point,geometryDimesion,valueDefinition,modelComponent),
  m_SWMMComponent(modelComponent),
  m_retrieved(false),
  m_applied(false)
{

}

PondedAreaInput::~PondedAreaInput()
{

}

bool PondedAreaInput::canConsume(HydroCouple::IOutput *provider, QString &message) const
{
  HydroCouple::Spatial::IGeometryComponentDataItem* geometryComp;

  if((geometryComp = dynamic_cast<HydroCouple::Spatial::IGeometryComponentDataItem*>(provider)) &&
     geometryComp->valueDefinition()->type() == QVariant::Double)
  {
    switch (geometryComp->geometryType())
    {
      case IGeometry::Polygon:
      case IGeometry::PolygonZ:
      case IGeometry::PolygonM:
      case IGeometry::PolygonZM:
      case IGeometry::Triangle:
      case IGeometry::TriangleZ:
      case IGeometry::TriangleM:
      case IGeometry::TriangleZM:
        {
          return true;
        }
        break;
      default:
        {
          return false;
        }
        break;
    }
  }

  return false;
}

void PondedAreaInput::retrieveValuesFromProvider()
{
  if(!m_retrieved)
  {

    m_geometryMapping.clear();

    IGeometryComponentDataItem *geomDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider());

    if(geomDataItem)
    {

      Octree *octree = new Octree(Octree::Octree2D, Octree::AlongEnvelopes, 6, 50);

      for(int i = 0; i < geometryCount(); i++)
      {
        IGeometry* node = geometry(i);
        octree->addGeometry(node);
      }


      for(int i = 0; i < geomDataItem->geometryCount(); i++)
      {
        IGeometry *geometry = geomDataItem->geometry(i);

        std::vector<IGeometry*> colliding = octree->findCollidingGeometries(geometry);

        for(IGeometry* collidingG : colliding)
        {
          if(geometry->contains(collidingG))
          {
            m_geometryMapping[collidingG->index()] = i;
            break;
          }
        }
      }

      delete octree;
    }

    m_retrieved = true;
  }
}

void PondedAreaInput::applyData()
{
  if(!m_applied)
  {
    IGeometryComponentDataItem *geomDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider());

    if(geomDataItem)
    {
      for(int i = 0; i < geometryCount(); i++)
      {
        if(m_geometryMapping.find(i) != m_geometryMapping.end())
        {
          int g = m_geometryMapping[i];

          double value = 0;
          geomDataItem->getValue(g, &value);

          if(value != geomDataItem->valueDefinition()->missingValue().toDouble())
          {
            HCPoint *nodeGeom = dynamic_cast<HCPoint*>(geometry(i));
            TNode &node = m_SWMMComponent->project()->Node[nodeGeom->marker()];
            double cf = UCF(m_SWMMComponent->project(), LENGTH);
            node.pondedArea = value / (cf * cf);
          }
        }
      }
    }

    m_applied = true;
  }
}

