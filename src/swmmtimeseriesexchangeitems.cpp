#include "stdafx.h"

#include <QDebug>

#include "swmmcomponent.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include "swmmtimeseriesexchangeitems.h"
#include "dataexchangecache.h"

using namespace HydroCouple;

SWMMNodeWSETimeSeriesOutput::SWMMNodeWSETimeSeriesOutput(TNode* node,
                                                         Dimension* dimension,
                                                         const std::list<SDKTemporal::DateTime *> &times,
                                                         ValueDefinition* valueDefinition,
                                                         SWMMComponent* component)
  : TimeSeriesOutputDouble(QString(node->ID) + "-N-WSE-Out", times, dimension, valueDefinition, component)
{
  double wse = node->invertElev + node->initDepth;
  m_component = component;
  m_node = node;
  setValuesT(0,1,&wse);
}

SWMMNodeWSETimeSeriesOutput::~SWMMNodeWSETimeSeriesOutput()
{

}

void SWMMNodeWSETimeSeriesOutput::updateValues(HydroCouple::IInput *querySpecifier)
{
  ITimeSeriesComponentDataItem* timeExchangeItem = dynamic_cast<ITimeSeriesComponentDataItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    QList<HydroCouple::Temporal::IDateTime*> inpTimes = timeExchangeItem->times();
    HydroCouple::Temporal::IDateTime *lastTime = inpTimes[inpTimes.length() -1];

    while (m_component->currentDateTime()->julianDay() < lastTime->julianDay() &&
           m_component->status() == IModelComponent::Updated)
    {
      m_component->update();
    }
  }
  else
  {
    while(m_component->status() != IModelComponent::Done &&
          m_component->status() != IModelComponent::Failed &&
          m_component->status() != IModelComponent::Finished)
    {
      m_component->update();
    }
  }
}

void SWMMNodeWSETimeSeriesOutput::updateValues()
{
  int timeDimLength = dimensionLength(std::vector<int>());
  std::vector<SDKTemporal::DateTime*> ctimes = timesInternal();
  SDKTemporal::DateTime *lastTime = ctimes[timeDimLength -1];
  double elev = m_node->invertElev + m_node->newDepth;

  if(m_component->currentDateTime()->julianDay() > lastTime->julianDay())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];

      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        ctimes[i]->setDateTime(ctimes[i+1]->dateTime());
      }
    }
    setValues(timeDimLength -1, 1, &elev);
    lastTime->setJulianDay(m_component->currentDateTime()->julianDay());
  }
  else if(m_component->currentDateTime()->julianDay() == lastTime->julianDay())
  {
    setValues(timeDimLength -1, 1, &elev);
  }

  QList<HydroCouple::IAdaptedOutput*> tadaptedOutputs = adaptedOutputs();

  for(HydroCouple::IAdaptedOutput* adaptedOutput :tadaptedOutputs)
  {
    adaptedOutput->refresh();
  }
}

//======================================================================================================================================================================

SWMMNodeWSETimeSeriesInput::SWMMNodeWSETimeSeriesInput(TNode* node,
                                                       Dimension *dimension,
                                                       const std::list<SDKTemporal::DateTime *> &times,
                                                       ValueDefinition* valueDefinition,
                                                       SWMMComponent *component)
  :TimeSeriesInputDouble(QString(node->ID) + "-N-WSE-Inp", times, dimension, valueDefinition, component)
{
  m_component = component;
  m_node = node;
}

SWMMNodeWSETimeSeriesInput::~SWMMNodeWSETimeSeriesInput()
{

}

bool SWMMNodeWSETimeSeriesInput::canConsume(HydroCouple::IOutput* provider, QString& message) const
{
  return true;
}

void SWMMNodeWSETimeSeriesInput::retrieveValuesFromProvider()
{
  HydroCouple::IOutput *provider = this->provider();

  int timeDimLength = dimensionLength(std::vector<int>());
  std::vector<SDKTemporal::DateTime*> ctimes = timesInternal();
  SDKTemporal::DateTime *lastTime = ctimes[timeDimLength -1];

  if(m_component->currentDateTime()->julianDay() > lastTime->julianDay())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];
      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        ctimes[i]->setDateTime(ctimes[i+1]->dateTime());
      }
    }

    lastTime->setJulianDay(m_component->currentDateTime()->julianDay());
  }

  provider->updateValues(this);


  double value = 0;

  ITimeSeriesComponentDataItem *tsoutput = dynamic_cast<ITimeSeriesComponentDataItem*>(provider);

  if(tsoutput)
  {
    tsoutput->getValues(tsoutput->dimensionLength(std::vector<int>()) - 1, 1 , &value);
  }

  int index = project_findObject(m_component->project(), NODE, m_node->ID);

  if(value && value > m_node->invertElev)
  {
    setValues(timeDimLength-1,1,&value);
    addNodeDepth(m_component->project(),index,value - m_node->invertElev);
  }
  else
  {
    setValues(timeDimLength-1,1,&m_node->invertElev);
    removeNodeLateralInflow(m_component->project(),index);
  }
}

void SWMMNodeWSETimeSeriesInput::applyData()
{

}

//======================================================================================================================================================================

SWMMNodeLatInflowTimeSeriesInput::SWMMNodeLatInflowTimeSeriesInput(TNode* node,
                                                                   Dimension *dimension,
                                                                   const std::list<SDKTemporal::DateTime *> &times,
                                                                   ValueDefinition *valueDefinition,
                                                                   SWMMComponent *component)
  :TimeSeriesMultiInputDouble(QString(node->ID)+"-N-Lat-Inf", times, dimension, valueDefinition, component)
{
  m_component = component;
  m_node = node;
}

SWMMNodeLatInflowTimeSeriesInput::~SWMMNodeLatInflowTimeSeriesInput()
{

}

bool SWMMNodeLatInflowTimeSeriesInput::canConsume(HydroCouple::IOutput* provider, QString& message) const
{
  return true;
}

void SWMMNodeLatInflowTimeSeriesInput::retrieveValuesFromProvider()
{
  QList<HydroCouple::IOutput*> inproviders = providers();
  int timeDimLength = dimensionLength(std::vector<int>());
  std::vector<SDKTemporal::DateTime*> ctimes = timesInternal();
  SDKTemporal::DateTime *lastTime = ctimes[timeDimLength -1];

  if(m_component->currentDateTime()->julianDay() > lastTime->julianDay())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];
      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        ctimes[i]->setDateTime(ctimes[i+1]->dateTime());
      }
    }

    lastTime->setJulianDay(m_component->currentDateTime()->julianDay());
  }

  for(HydroCouple::IOutput* output : inproviders)
  {
    output->updateValues(this);
  }

  double value = 0;

  for(HydroCouple::IOutput* toutput : inproviders)
  {
    ITimeSeriesComponentDataItem* tsoutput = dynamic_cast<ITimeSeriesComponentDataItem*>(toutput);

    if(tsoutput)
    {
      double ovalue = 0;
      int timeIndex ={toutput->dimensionLength(std::vector<int>()) - 1};
      tsoutput->getValues(timeIndex, 1, &ovalue);
      value = value + ovalue;
    }
  }

  int index = project_findObject(m_component->project(), NODE, m_node->ID);

  if(value)
  {
    setValues(timeDimLength-1,1,&value);
    addNodeLateralInflow(m_component->project(),index,value);
  }
  else
  {
    removeNodeLateralInflow(m_component->project(),index);
  }
}

void SWMMNodeLatInflowTimeSeriesInput::applyData()
{

}

//======================================================================================================================================================================

SWMMLinkDischargeTimeSeriesOutput::SWMMLinkDischargeTimeSeriesOutput(TLink* link,
                                                                     Dimension *dimension,
                                                                     const std::list<SDKTemporal::DateTime *> &times,
                                                                     ValueDefinition *valueDefinition,
                                                                     SWMMComponent *component)
  :TimeSeriesOutputDouble(QString(link->ID) + "-L-F", times, dimension, valueDefinition, component)
{
  m_component = component;
  m_link = link;
}

SWMMLinkDischargeTimeSeriesOutput::~SWMMLinkDischargeTimeSeriesOutput()
{

}

void SWMMLinkDischargeTimeSeriesOutput::updateValues(HydroCouple::IInput *querySpecifier)
{
  ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    QList<HydroCouple::Temporal::IDateTime*> inptimes = timeExchangeItem->times();
    HydroCouple::Temporal::IDateTime *lastTime = inptimes[inptimes.length() -1];

    while (m_component->currentDateTime()->julianDay() < lastTime->julianDay() &&
           m_component->status() == IModelComponent::Updated)
    {
      m_component->update();
    }
  }
  else //run to completion
  {
    while (m_component->status() != IModelComponent::Done &&
           m_component->status() != IModelComponent::Failed &&
           m_component->status() != IModelComponent::Finished)
    {
      m_component->update();
    }
  }

  //otherwise run to end;
}

void SWMMLinkDischargeTimeSeriesOutput::updateValues()
{
  int timeDimLength = dimensionLength(std::vector<int>());
  std::vector<SDKTemporal::DateTime*> times = this->timesInternal();
  SDKTemporal::DateTime *lastTime = times[timeDimLength -1];

  if(m_component->currentDateTime()->julianDay() > lastTime->julianDay())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];

      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        times[i]->setJulianDay(times[i+1]->julianDay());
      }
    }

    double flow = m_link->newFlow;

    if(flow)
    {
      flow *=1.0;
    }

    setValues(timeDimLength -1, 1, &flow);
    lastTime->setJulianDay(m_component->currentDateTime()->julianDay());
  }
  else if(m_component->currentDateTime()->julianDay() == lastTime->julianDay())
  {
    setValues(timeDimLength -1, 1, &m_link->newFlow);
  }
}
