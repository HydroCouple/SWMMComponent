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
  ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    double queryTime = timeExchangeItem->time(timeExchangeItem->timeCount() - 1)->modifiedJulianDay();

    while (m_SWMMComponent->currentDateTime()->modifiedJulianDay() < queryTime &&
           m_SWMMComponent->status() == IModelComponent::Updated)
    {
      m_SWMMComponent->update(QList<IOutput*>({this}));
    }
  }
  else
  {
    if(m_SWMMComponent->status() == IModelComponent::Updated)
    {
      m_SWMMComponent->update(QList<IOutput*>({this}));
    }
  }

  refreshAdaptedOutputs();
}

void NodeSurfaceFlowOutput::updateValues()
{
  moveDataToPrevTime();

  int currentTimeIndex = timeCount() - 1;
  m_times[currentTimeIndex]->setModifiedJulianDay(m_SWMMComponent->currentDateTime()->modifiedJulianDay());
  resetTimeSpan();

  double inflowTotal = 0.0;
  double outflowTotal = 0.0;


  for(size_t i = 0 ; i < m_geometries.size() ; i++)
  {
    QSharedPointer<HCGeometry> nodeGeom = m_geometries[i];
    char *nodeId = const_cast<char*>(nodeGeom->id().toStdString().c_str());
    int nodeIndex = project_findObject(m_SWMMComponent->project(),NODE, nodeId);
    TNode &node = m_SWMMComponent->project()->Node[nodeIndex];

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
            if(m_SWMMComponent->m_surfaceInflow[nodeIndex])
              value = -m_SWMMComponent->m_surfaceInflow[nodeIndex];
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
