/*!
 * \file   conduitbankxsectareaoutput.cpp
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
#include "headers.h"
#include "conduitbankxsectareaoutput.h"
#include "swmmcomponent.h"
#include "temporal/timedata.h"
#include "spatial/linestring.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

ConduitBankXSectAreaOutput::ConduitBankXSectAreaOutput(const QString &id,
                                                       Dimension *timeDimension,
                                                       Dimension *geometryDimension,
                                                       ValueDefinition *valueDefinition,
                                                       SWMMComponent *modelComponent)
  : TimeGeometryOutputDouble(id, IGeometry::LineString,
                             timeDimension, geometryDimension,
                             valueDefinition, modelComponent),
    m_SWMMComponent(modelComponent)
{

}

ConduitBankXSectAreaOutput::~ConduitBankXSectAreaOutput()
{

}

void ConduitBankXSectAreaOutput::updateValues(IInput *querySpecifier)
{
  if(!m_SWMMComponent->workflow())
  {
    ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);
    QList<IOutput*>updateList;
//    QList<IOutput*>updateList({this});

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

void ConduitBankXSectAreaOutput::updateValues()
{
  moveDataToPrevTime();

  int currentTimeIndex = timeCount() - 1;
  m_times[currentTimeIndex]->setJulianDay(m_SWMMComponent->currentDateTime()->julianDay());
  resetTimeSpan();

//#ifdef USE_OPENMP
//#pragma omp parallel for
//#endif
  for(int i = 0 ; i < (int)m_geometries.size() ; i++)
  {
    QSharedPointer<HCGeometry> &linkGeom = m_geometries[i];
    TLink &link = m_SWMMComponent->project()->Link[linkGeom->marker()];

    double value = xsect_getAofY(m_SWMMComponent->project(), &link.xsect, link.newDepth);
    double convFactor = UCF(m_SWMMComponent->project(), LENGTH);
    value *= convFactor * convFactor;
    setValue(currentTimeIndex,i,&value);
  }

  refreshAdaptedOutputs();
}

ConduitBankXSectAreaOutput::Bank ConduitBankXSectAreaOutput::bank() const
{
  return m_bank;
}

void ConduitBankXSectAreaOutput::setBank(Bank bank)
{
  m_bank = bank;
}
