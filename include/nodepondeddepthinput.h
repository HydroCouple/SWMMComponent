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

#ifndef NODEPONDEDDEPTHINPUT_H
#define NODEPONDEDDEPTHINPUT_H


#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryinput.h"
#include <unordered_map>

class SWMMComponent;


class SWMMCOMPONENT_EXPORT NodePondedDepthInput: public TimeGeometryInputDouble
{
    Q_OBJECT

  public:
    NodePondedDepthInput(const QString &id,
                         Dimension *timeDimension,
                         Dimension *geometryDimension,
                         ValueDefinition *valueDefinition,
                         SWMMComponent *modelComponent);

    bool setProvider(HydroCouple::IOutput *provider) override;

    bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:

    SWMMComponent *m_SWMMComponent;
    std::unordered_map<int,int> m_geometryMapping;
    int m_printTracker;

};


#endif // NODEPONDEDDEPTHINPUT_H
