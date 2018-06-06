#include "stdafx.h"
#include "nodesurfaceflowoutput.h"
#include "headers.h"
#include "swmmcomponent.h"
#include "temporal/timedata.h"
#include "spatial/point.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

NodeSurfaceFlowOutput::NodeSurfaceFlowOutput(const QString &id,
                                             Dimension *timeDimension,
                                             Dimension *geometryDimension,
                                             ValueDefinition *valueDefinition,
                                             SWMMComponent *modelComponent):
  TimeGeometryOutputDouble(id, IGeometry::Point,
                           timeDimension, geometryDimension,
                           valueDefinition, modelComponent),
  m_SWMMComponent(modelComponent)
{

}

NodeSurfaceFlowOutput::~NodeSurfaceFlowOutput()
{
}

void NodeSurfaceFlowOutput::updateValues(HydroCouple::IInput *querySpecifier)
{
  if(!m_SWMMComponent->workflow())
  {
    ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);
    QList<IOutput*>updateList;
    //QList<IOutput*>updateList({this});

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

void NodeSurfaceFlowOutput::updateValues()
{
  moveDataToPrevTime();

  int currentTimeIndex = timeCount() - 1;
  m_times[currentTimeIndex]->setJulianDay(m_SWMMComponent->currentDateTime()->julianDay());
  resetTimeSpan();

  double inflowTotal = 0.0;
  double outflowTotal = 0.0;


  for(size_t i = 0 ; i < m_geometries.size() ; i++)
  {
    QSharedPointer<HCGeometry> nodeGeom = m_geometries[i];
    TNode &node = m_SWMMComponent->project()->Node[nodeGeom->marker()];

    double value = 0;

    switch (node.type)
    {
      case NodeType::OUTFALL:
        {
          value = node.inflow;
          outflowTotal += value;
        }
        break;
      default:
        {
          if(node.newDepth < node.fullDepth)
          {
            if(m_SWMMComponent->m_surfaceInflow[nodeGeom->marker()])
              value = -m_SWMMComponent->m_surfaceInflow[nodeGeom->marker()];
          }
          else
          {
            value = node.overflowAndInflow;
          }
        }
        break;
    }

    value *= UCF(m_SWMMComponent->project(), FLOW);

    if(value > 0)
      outflowTotal += value;
    else
      inflowTotal -= value;

    setValue(currentTimeIndex,i,&value);
  }

  if(m_printTracker > 30)
  {
    printf("SWMM Inflow Total: %f\n", inflowTotal);
    printf("SWMM Outflow Total: %f\n", outflowTotal);
    m_printTracker = 0;
  }
  else
  {
    m_printTracker++;
  }


  refreshAdaptedOutputs();
}
