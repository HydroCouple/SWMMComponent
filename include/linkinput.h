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

#ifndef LINKINPUT_H
#define LINKINPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometrymultiinput.h"

#include <unordered_map>

class SWMMComponent;

class SWMMCOMPONENT_EXPORT LinkInput : public TimeGeometryMultiInputDouble
{
    Q_OBJECT

  public:

    enum LinkVariable
    {
      Roughness,
      LateralInflow,
      SeepageLossRate,
      EvaporationLossRate
    };

    LinkInput(const QString &id,
              Dimension *timeDimension,
              Dimension *geometryDimension,
              ValueDefinition *valueDefinition,
              LinkVariable linkInputVariable,
              SWMMComponent *modelComponent);

    virtual ~LinkInput();

    bool addProvider(HydroCouple::IOutput *provider) override;

    bool removeProvider(HydroCouple::IOutput *provider) override;

    bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:

    LinkVariable m_linkVariable;
    SWMMComponent *m_SWMMComponent;
    std::unordered_map<HydroCouple::IOutput*, std::unordered_map<int,int>> m_geometryMapping;
    std::unordered_map<int, double> m_sumInflow;

};

#endif // LINKINPUT_H
