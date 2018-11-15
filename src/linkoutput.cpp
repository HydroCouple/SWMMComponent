#include "stdafx.h"
#include "linkoutput.h"
#include "swmmcomponent.h"
#include "headers.h"
#include "temporal/timedata.h"
#include "spatial/linestring.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

LinkOutput::LinkOutput(const QString &id,
                       Dimension *timeDimension,
                       Dimension *geometryDimension,
                       ValueDefinition *valueDefinition,
                       LinkVariable linkOutputVariable,
                       SWMMComponent *modelComponent)
  : TimeGeometryOutputDouble(id, IGeometry::LineString, timeDimension, geometryDimension,
                             valueDefinition, modelComponent),
    m_linkVariable(linkOutputVariable),
    m_SWMMComponent(modelComponent)
{

}


LinkOutput::~LinkOutput()
{

}

void LinkOutput::updateValues(IInput *querySpecifier)
{
  if(!m_SWMMComponent->workflow())
  {
    ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);
    QList<IOutput*>updateList;

    if(timeExchangeItem)
    {
      double queryTime = timeExchangeItem->time(timeExchangeItem->timeCount() - 1)->julianDay();

      while (m_SWMMComponent->currentDateTime()->julianDay() < queryTime &&
             m_SWMMComponent->status() == IModelComponent::Updated)
      {
        m_SWMMComponent->update(updateList);
      }
    }
    else
    {
      if(m_SWMMComponent->status() == IModelComponent::Updated)
      {
        m_SWMMComponent->update(updateList);
      }
    }
  }

  refreshAdaptedOutputs();
}

void LinkOutput::updateValues()
{
  moveDataToPrevTime();

  int currentTimeIndex = timeCount() - 1;
  m_times[currentTimeIndex]->setJulianDay(m_SWMMComponent->currentDateTime()->julianDay());
  resetTimeSpan();


  switch (m_linkVariable)
  {
    case LinkVariable::Depth:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

          double value = link.newDepth * UCF(m_SWMMComponent->project(), LENGTH);
          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
    case LinkVariable::WaterSurfaceElevation:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];
          TNode &up = m_SWMMComponent->project()->Node[link.node1];
          TNode &down = m_SWMMComponent->project()->Node[link.node2];
          double linkElev = 0.5 * (up.invertElev + link.offset1 + down.invertElev + link.offset2);
          double value = (linkElev + link.newDepth) * UCF(m_SWMMComponent->project(), LENGTH);
          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
    case LinkVariable::XsectionArea:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

          double value = m_SWMMComponent->project()->Conduit[link.subIndex].a1;
          double convFactor = UCF(m_SWMMComponent->project(), LENGTH);
          value *= convFactor * convFactor;
          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
    case LinkVariable::TopWidth:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

          double value = xsect_getWofY(m_SWMMComponent->project(), &link.xsect, link.newDepth) * UCF(m_SWMMComponent->project(), LENGTH);
          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
    case LinkVariable::Perimeter:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

          double value = xsect_getAofY(m_SWMMComponent->project(), &link.xsect, link.newDepth) * UCF(m_SWMMComponent->project(), LENGTH) /
                         xsect_getRofY(m_SWMMComponent->project(), &link.xsect, link.newDepth);

          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
    case LinkVariable::Flow:
      {
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for(int i = 0 ; i < (int)m_geometries.size() ; i++)
        {
          QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
          TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

          double value = link.newFlow * UCF(m_SWMMComponent->project(), FLOW);
          setValue(currentTimeIndex,i,&value);
        }
      }
      break;
  }


  refreshAdaptedOutputs();
}

