#include "stdafx.h"
#include "headers.h"
#include "linkflowoutput.h"
#include "swmmcomponent.h"
#include "temporal/timedata.h"
#include "spatial/linestring.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

LinkFlowOutput::LinkFlowOutput(const QString &id,
                                             Dimension *timeDimension,
                                             Dimension *geometryDimension,
                                             ValueDefinition *valueDefinition,
                                             SWMMComponent *modelComponent):
  TimeGeometryOutputDouble(id, IGeometry::LineString,
                           timeDimension, geometryDimension,
                           valueDefinition, modelComponent),
  m_SWMMComponent(modelComponent)
{

}

LinkFlowOutput::~LinkFlowOutput()
{
}

void LinkFlowOutput::updateValues(HydroCouple::IInput *querySpecifier)
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

void LinkFlowOutput::updateValues()
{
  moveDataToPrevTime();

  int currentTimeIndex = timeCount() - 1;
  m_times[currentTimeIndex]->setModifiedJulianDay(m_SWMMComponent->currentDateTime()->modifiedJulianDay());
  resetTimeSpan();

  for(size_t i = 0 ; i < m_geometries.size() ; i++)
  {
    QSharedPointer<HCGeometry> linkGeom = m_geometries[i];
    char *linkId = const_cast<char*>(linkGeom->id().toStdString().c_str());
    int linkIndex = project_findObject(m_SWMMComponent->project(),LINK, linkId);
    TLink &link = m_SWMMComponent->project()->Link[linkIndex];

    double value = link.newFlow * UCF(m_SWMMComponent->project(), FLOW);
    setValue(currentTimeIndex,i,&value);
  }

  refreshAdaptedOutputs();
}
