#include "stdafx.h"
#include "component/swmmtimeseriesexchangeitems.h"
#include "component/swmmcomponent.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include <QDebug>

using namespace SDKTemporal;
//
//

SWMMNodeWSETimeSeriesOutput::SWMMNodeWSETimeSeriesOutput(TNode* node,
                                                         Dimension* dimension,
                                                         const QList<SDKTemporal::Time*>& times,
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

void SWMMNodeWSETimeSeriesOutput::update(HydroCouple::IInput *querySpecifier)
{
  ITimeExchangeItem* timeExchangeItem = dynamic_cast<ITimeExchangeItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    QList<HydroCouple::Temporal::ITime*> inpTimes = timeExchangeItem->times();
    HydroCouple::Temporal::ITime *lastTime = inpTimes[inpTimes.length() -1];

    while (m_component->currentDateTime()->dateTime() < lastTime->dateTime() &&
           m_component->status() == HydroCouple::Updated)
    {
      m_component->update();
    }
  }
  else
  {
    while (m_component->status() != HydroCouple::Done &&
           m_component->status() != HydroCouple::Failed &&
           m_component->status() != HydroCouple::Finished)
    {
      m_component->update();
    }
  }
}

void SWMMNodeWSETimeSeriesOutput::retrieveDataFromModel()
{
  int timeDimLength = dimensionLength(nullptr,0);
  QList<Time*> ctimes = timesInternal();
  Time *lastTime = ctimes[timeDimLength -1];
  double elev = m_node->invertElev + m_node->newDepth;

  if(m_component->currentDateTime()->dateTime() > lastTime->dateTime())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];

      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        ctimes[i]->setDateTime(ctimes[i+1]->qDateTime());
      }
    }
    setValues(timeDimLength -1, 1, &elev);
    lastTime->setDateTime(m_component->currentDateTime()->qDateTime());
  }
  else if(m_component->currentDateTime()->dateTime() == lastTime->dateTime())
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
                                                       const QList<SDKTemporal::Time*>& times,
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

void SWMMNodeWSETimeSeriesInput::retrieveOuputItemData()
{
  HydroCouple::IOutput *provider = this->provider();

  int timeDimLength = dimensionLength(nullptr,0);
  QList<Time*> ctimes = timesInternal();
  Time *lastTime = ctimes[timeDimLength -1];

  if(m_component->currentDateTime()->dateTime() > lastTime->dateTime())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];
      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        ctimes[i]->setDateTime(ctimes[i+1]->qDateTime());
      }
    }

    lastTime->setDateTime(m_component->currentDateTime()->qDateTime());
  }

  provider->update(this);


  double value = 0;

  ITimeExchangeItem *tsoutput = dynamic_cast<ITimeExchangeItem*>(provider);

  if(tsoutput)
  {
    tsoutput->getValues(tsoutput->dimensionLength(nullptr,0) - 1, 1 , &value);
  }

  int index = project_findObject(m_component->SWMMProject(), NODE, m_node->ID);

  if(value && value > m_node->invertElev)
  {
    setValues(timeDimLength-1,1,&value);
    qDebug() << "depth" << value - m_node->invertElev;
    addNodeDepth(m_component->SWMMProject(),index,value - m_node->invertElev);
  }
  else
  {
    setValues(timeDimLength-1,1,&m_node->invertElev);
    removeNodeLateralInflow(m_component->SWMMProject(),index);
  }
}

//======================================================================================================================================================================

SWMMNodeLatInflowTimeSeriesInput::SWMMNodeLatInflowTimeSeriesInput(TNode* node,
                                                                   Dimension *dimension,
                                                                   const QList<SDKTemporal::Time*>& times,
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

void SWMMNodeLatInflowTimeSeriesInput::retrieveOuputItemData()
{
  QList<HydroCouple::IOutput*> inproviders = providers();
  int timeDimLength = dimensionLength(nullptr,0);
  QList<Time*> ctimes = timesInternal();
  Time *lastTime = ctimes[timeDimLength -1];

  if(m_component->currentDateTime()->dateTime() > lastTime->dateTime())
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

    lastTime->setDateTime(m_component->currentDateTime()->dateTime());
  }

  for(HydroCouple::IOutput* output : inproviders)
  {
    output->update(this);
  }

  double value = 0;

  for(HydroCouple::IOutput* toutput : inproviders)
  {
    ITimeExchangeItem* tsoutput = dynamic_cast<ITimeExchangeItem*>(toutput);

    if(tsoutput)
    {
      double ovalue = 0;
      int ind[1]={toutput->dimensionLength(nullptr,0) - 1};
      int str[1] = {1};
      toutput->getValues(ind, str, &ovalue);
      value = value + ovalue;
    }
  }

  int index = project_findObject(m_component->SWMMProject(), NODE, m_node->ID);

  if(value)
  {
    setValues(timeDimLength-1,1,&value);
    addNodeLateralInflow(m_component->SWMMProject(),index,value);
  }
  else
  {
    removeNodeLateralInflow(m_component->SWMMProject(),index);
  }
}

//======================================================================================================================================================================

SWMMLinkDischargeTimeSeriesOutput::SWMMLinkDischargeTimeSeriesOutput(TLink* link,
                                                                     Dimension *dimension,
                                                                     const QList<SDKTemporal::Time*>& times,
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

void SWMMLinkDischargeTimeSeriesOutput::update(HydroCouple::IInput *querySpecifier)
{
  ITimeExchangeItem* timeExchangeItem = dynamic_cast<ITimeExchangeItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    QList<HydroCouple::Temporal::ITime*> inptimes = timeExchangeItem->times();
    HydroCouple::Temporal::ITime *lastTime = inptimes[inptimes.length() -1];

    while (m_component->currentDateTime()->dateTime() < lastTime->dateTime() &&
           m_component->status() == HydroCouple::Updated)
    {
      m_component->update();
    }
  }
  else //run to completion
  {
    while (m_component->status() != HydroCouple::Done &&
           m_component->status() != HydroCouple::Failed &&
           m_component->status() != HydroCouple::Finished)
    {
      m_component->update();
    }
  }

  //otherwise run to end;
}

void SWMMLinkDischargeTimeSeriesOutput::retrieveDataFromModel()
{
  int timeDimLength = dimensionLength(nullptr,0);
  QList<HydroCouple::Temporal::ITime*> times = this->times();
  HydroCouple::Temporal::ITime *lastTime = times[timeDimLength -1];

  if(m_component->currentDateTime()->dateTime() > lastTime->dateTime())
  {
    if(timeDimLength > 1)
    {
      double values[timeDimLength -1];

      getValues(1,timeDimLength - 1,values);
      setValues(0,timeDimLength - 1,values);

      for(int i = 0 ; i < timeDimLength -1; i++)
      {
        times[i]->setDateTime(times[i+1]->dateTime());
      }
    }

    double flow = m_link->newFlow;

    if(flow)
    {
      flow *=1.0;
    }

    setValues(timeDimLength -1, 1, &flow);
    lastTime->setDateTime(m_component->currentDateTime()->dateTime());
  }
  else if(m_component->currentDateTime()->dateTime() == lastTime->dateTime())
  {
    setValues(timeDimLength -1, 1, &m_link->newFlow);
  }
}
