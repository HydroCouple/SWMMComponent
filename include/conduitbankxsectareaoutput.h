/*!
 * \file   conduitbankxsectareaoutput.h
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

#ifndef CONDUITBANKXSECTAREAOUTPUT_H
#define CONDUITBANKXSECTAREAOUTPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryoutput.h"

class SWMMComponent;

class SWMMCOMPONENT_EXPORT ConduitBankXSectAreaOutput: public TimeGeometryOutputDouble
{
    Q_OBJECT

  public:

    enum Bank
    {
      Left,
      Right
    };

    ConduitBankXSectAreaOutput(const QString &id,
                               Dimension *timeDimension,
                               Dimension *geometryDimension,
                               ValueDefinition *valueDefinition,
                               SWMMComponent *modelComponent);

    virtual ~ConduitBankXSectAreaOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

    Bank bank() const;

    void setBank(Bank bank);

  private:
    Bank m_bank;
    double m_bankFraction;
    SWMMComponent *m_SWMMComponent;
};

#endif // CONDUITBANKXSECTAREAOUTPUT_H
