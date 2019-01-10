/*!
 * \file   NodePondedDepthInput.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version   1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "linkinput.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "swmmcomponent.h"
#include "headers.h"
#include "dataexchangecache.h"
#include "temporal/timedata.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::SpatioTemporal;

using namespace std;

LinkInput::LinkInput(const QString &id,
                     Dimension *timeDimension,
                     Dimension *geometryDimension,
                     ValueDefinition *valueDefinition,
                     LinkVariable linkInputVariable,
                     SWMMComponent *modelComponent)
  : TimeGeometryMultiInputDouble(id,
                                 IGeometry::LineString,
                                 timeDimension, geometryDimension,
                                 valueDefinition, modelComponent),
    m_linkVariable(linkInputVariable),
    m_SWMMComponent(modelComponent)
{

}

LinkInput::~LinkInput()
{

}

bool LinkInput::addProvider(IOutput *provider)
{
  if(AbstractMultiInput::addProvider(provider))
  {
    ITimeGeometryComponentDataItem *timeGeometryDataItem = dynamic_cast<ITimeGeometryComponentDataItem*>(provider);

    std::unordered_map<int, int> geometryMapping;

    if(timeGeometryDataItem->geometryCount())
    {
      std::vector<bool> mapped(timeGeometryDataItem->geometryCount(), false);

      for(int i = 0; i < geometryCount() ; i++)
      {
        HCLineString *lineString = dynamic_cast<HCLineString*>(getGeometry(i));

        if(lineString->pointCount())
        {
          HCPoint *p1 = lineString->pointInternal(0);
          HCPoint *p2 = lineString->pointInternal(lineString->pointCount() - 1);

          for(int j = 0; j < timeGeometryDataItem->geometryCount() ; j++)
          {
            if(!mapped[j])
            {
              ILineString *lineStringProvider = dynamic_cast<ILineString*>(timeGeometryDataItem->geometry(j));

              IPoint *pp1 = lineStringProvider->point(0);
              IPoint *pp2 = lineStringProvider->point(lineStringProvider->pointCount() - 1);


              if(hypot(p1->x() - pp1->x() , p1->y() - pp1->y()) < 1e-3 && hypot(p2->x() - pp2->x() , p2->y() - pp2->y()) < 1e-3)
              {
                geometryMapping[i] = j;
                mapped[j] = true;
                break;
              }
              else if(hypot(p1->x() - pp2->x() , p1->y() - pp2->y()) < 1e-3 && hypot(p2->x() - pp1->x() , p2->y() - pp1->y()) < 1e-3)
              {
                geometryMapping[i] = j;
                mapped[j] = true;
                break;
              }
            }
          }
        }
      }
    }

    m_geometryMapping[provider] = geometryMapping;

    return true;
  }

  return false;
}

bool LinkInput::removeProvider(HydroCouple::IOutput *provider)
{
  if(AbstractMultiInput::removeProvider(provider))
  {
    m_geometryMapping.erase(provider);
    return true;
  }

  return false;
}

bool LinkInput::canConsume(IOutput *provider, QString &message) const
{
  ITimeGeometryComponentDataItem *timeGeometryDataItem = nullptr;
  IGeometryComponentDataItem *geometryDataItem = nullptr;

  if((timeGeometryDataItem = dynamic_cast<ITimeGeometryComponentDataItem*>(provider)) &&
     (timeGeometryDataItem->geometryType() == IGeometry::LineString ||
      timeGeometryDataItem->geometryType() == IGeometry::LineStringZ ||
      timeGeometryDataItem->geometryType() == IGeometry::LineStringZM) &&
     (provider->valueDefinition()->type() == QVariant::Double ||
      provider->valueDefinition()->type() == QVariant::Int))
  {
    return true;
  }
  else if((geometryDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider)) &&
          (geometryDataItem->geometryType() == IGeometry::LineString ||
           geometryDataItem->geometryType() == IGeometry::LineStringZ ||
           geometryDataItem->geometryType() == IGeometry::LineStringZM) &&
          (provider->valueDefinition()->type() == QVariant::Double ||
           provider->valueDefinition()->type() == QVariant::Int))
  {
    return true;
  }

  message = "Provider must be a LineString";

  return false;
}

void LinkInput::retrieveValuesFromProvider()
{
  moveDataToPrevTime();
  m_times[m_times.size() - 1]->setJulianDay(m_SWMMComponent->currentDateTime()->julianDay());
  resetTimeSpan();

  for(IOutput *provider : m_providers)
  {
    provider->updateValues(this);
  }
}

void LinkInput::applyData()
{
  double currentTime = m_SWMMComponent->currentDateTime()->julianDay();

  for(int i = 0; i < geometryCount(); i++)
  {
    m_sumInflow[i] = 0.0;
  }

  for(IOutput *provider : m_providers)
  {
    std::unordered_map<int,int> &geometryMapping = m_geometryMapping[provider];

    ITimeGeometryComponentDataItem *timeGeometryDataItem = nullptr;
    IGeometryComponentDataItem *geometryDataItem = nullptr;

    if((timeGeometryDataItem = dynamic_cast<ITimeGeometryComponentDataItem*>(provider)))
    {
      int currentTimeIndex = timeGeometryDataItem->timeCount() - 1;
      int previousTimeIndex = std::max(0, timeGeometryDataItem->timeCount() - 2);

      double providerCurrentTime = timeGeometryDataItem->time(currentTimeIndex)->julianDay();
      double providerPreviousTime = timeGeometryDataItem->time(previousTimeIndex)->julianDay();

      if(currentTime >=  providerPreviousTime && currentTime <= providerCurrentTime)
      {
        double factor = 0.0;

        if(providerCurrentTime > providerPreviousTime)
        {
          double denom = providerCurrentTime - providerPreviousTime;
          double numer = currentTime - providerPreviousTime;
          factor = numer / denom;
        }

        switch (m_linkVariable)
        {
          case Roughness:
            {
              for(auto it : geometryMapping)
              {
                double value1 = 0;
                double value2 = 0;

                timeGeometryDataItem->getValue(currentTimeIndex,it.second, &value1);
                timeGeometryDataItem->getValue(previousTimeIndex,it.second, &value2);

                TLink &link = m_SWMMComponent->project()->Link[m_geometries[it.first]->marker()];

                if(link.type == CONDUIT)
                {
                  TConduit &conduit = m_SWMMComponent->project()->Conduit[link.subIndex];
                  conduit.roughness = value2 + factor *(value1 - value2);
                }
              }
            }
            break;
          case LateralInflow:
          case SeepageLossRate:
          case EvaporationLossRate:
            {
              for(auto it : geometryMapping)
              {
                double value1 = 0;
                double value2 = 0;

                timeGeometryDataItem->getValue(currentTimeIndex,it.second, &value1);
                timeGeometryDataItem->getValue(previousTimeIndex,it.second, &value2);

                m_sumInflow[m_geometries[it.first]->marker()] += value2 + factor *(value1 - value2);
              }
            }
            break;
        }
      }
      else
      {
        switch (m_linkVariable)
        {
          case Roughness:
            {
              for(auto it : geometryMapping)
              {
                double value = 0;
                timeGeometryDataItem->getValue(currentTimeIndex,it.second, &value);

                TLink &link = m_SWMMComponent->project()->Link[m_geometries[it.first]->marker()];

                if(link.type == CONDUIT)
                {
                  TConduit &conduit = m_SWMMComponent->project()->Conduit[link.subIndex];
                  conduit.roughness = value;
                }
              }
            }
            break;
          case LateralInflow:
          case SeepageLossRate:
          case EvaporationLossRate:
            {
              for(auto it : geometryMapping)
              {
                double value = 0;
                timeGeometryDataItem->getValue(currentTimeIndex,it.second, &value);
                m_sumInflow[m_geometries[it.first]->marker()] += value;
              }
            }
            break;
        }
      }
    }
    else if((geometryDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider)))
    {
      switch (m_linkVariable)
      {
        case Roughness:
          {
            for(auto it : geometryMapping)
            {
              double value = 0;
              geometryDataItem->getValue(it.second, &value);

              TLink &link = m_SWMMComponent->project()->Link[m_geometries[it.first]->marker()];

              if(link.type == CONDUIT)
              {
                TConduit &conduit = m_SWMMComponent->project()->Conduit[link.subIndex];
                conduit.roughness = value;
              }

            }
          }
          break;
        case LateralInflow:
        case SeepageLossRate:
        case EvaporationLossRate:
          {
            for(auto it : geometryMapping)
            {
              double value = 0;
              geometryDataItem->getValue(it.second, &value);
              m_sumInflow[m_geometries[it.first]->marker()] += value;
            }
          }
          break;
      }
    }
  }

  switch (m_linkVariable)
  {
    case LateralInflow:
      {
        for(const auto &it : m_sumInflow)
        {
          double value = it.second / UCF(m_SWMMComponent->project(), FLOW);

          if(value)
          {
            TLink &link = m_SWMMComponent->project()->Link[it.first];

            if(link.newFlow >= 0.0)
            {
              addNodeLateralInflow(m_SWMMComponent->project(), link.node1, value);
            }
            else
            {
              addNodeLateralInflow(m_SWMMComponent->project(), link.node2, value);
            }
          }
        }
      }
      break;
    case SeepageLossRate:
      {
        for(const auto &it : m_sumInflow)
        {
          TLink &link = m_SWMMComponent->project()->Link[it.first];

          if(link.type == CONDUIT)
          {
            TConduit &conduit = m_SWMMComponent->project()->Conduit[link.subIndex];
            conduit.seepLossRate = it.second /  UCF(m_SWMMComponent->project(), FLOW);
          }
        }
      }
      break;
    case EvaporationLossRate:
      {
        for(const auto &it : m_sumInflow)
        {
          TLink &link = m_SWMMComponent->project()->Link[it.first];

          if(link.type == CONDUIT)
          {
            TConduit &conduit = m_SWMMComponent->project()->Conduit[link.subIndex];
            conduit.evapLossRate = it.second /  UCF(m_SWMMComponent->project(), FLOW);
          }
        }
      }
      break;
  }
}
