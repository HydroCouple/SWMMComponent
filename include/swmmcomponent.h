/*!
 * \file swmmcomponent.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef SWMMCOMPONENT_H
#define SWMMCOMPONENT_H

#include "swmmcomponent_global.h"
#include "temporal/abstracttimemodelcomponent.h"

#include <QFileInfo>
#include <QTextStream>
#include <map>

namespace SDKTemporal
{
  class DateTime;
}

class SWMMComponentInfo;
class SWMMInputObjectItem;
class SWMMOutputObjectItem;
class IdBasedArgumentString;
class Argument1DString;
class HCGeometry;
class PondedAreaInput;
class NodeSurfaceFlowOutput;
class Dimension;
class NodeWSEInput;
class NodePondedDepthInput;
class IdBasedArgumentDouble;
class LinkFlowOutput;
class LinkDepthOutput;
class ConduitXSectAreaOutput;
class ConduitTopWidthOutput;
class ConduitBankXSectAreaOutput;

typedef struct Project Project;

class SWMMCOMPONENT_EXPORT SWMMComponent : public AbstractTimeModelComponent,
    public virtual HydroCouple::ICloneableModelComponent
{

    Q_INTERFACES(HydroCouple::ICloneableModelComponent)

    friend class NodeWSEInput;
    friend class NodePondedDepthInput;
    friend class NodeSurfaceFlowOutput;

    Q_OBJECT

  public:

    SWMMComponent(const QString &id, SWMMComponentInfo* parent = nullptr);

    SWMMComponent(const QString &id, const QString &caption, SWMMComponentInfo* parent = nullptr);

    virtual ~SWMMComponent();

    QList<QString> validate() override;

    void prepare() override;

    void update(const QList<HydroCouple::IOutput*> &requiredOutputs = QList<HydroCouple::IOutput*>()) override;

    void finish() override;

    HydroCouple::ICloneableModelComponent* parent() const override;

    HydroCouple::ICloneableModelComponent* clone() override;

    QList<HydroCouple::ICloneableModelComponent*> clones() const override;

    Project* project() const;

  protected:

    bool removeClone(SWMMComponent *component);

  private:

    void createDimensions();

    void createArguments() override;

    void createInputFileArguments();

    void createNodeAreasArguments();

    void createNodePerimetersArguments();

    void createNodeOrificeDischargeCoeffArguments();

    bool initializeArguments(QString &message) override;

    bool initializeInputFilesArguments(QString &message);

    bool initializeNodeAreasArguments(QString &message);

    bool initializeNodePerimetersArguments(QString &message);

    bool initializeNodeOrificeDischargeCoeffArguments(QString &message);

    void readNodeGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void readLinkGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void readSubCatchmentGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void createInputs() override;

    void createIdBasedNodeWSEInput();

    void createIdBasedNodeInflowInput();

    void createNodeWSEInput();

    void createNodePondedDepthInput();

    void createNodeInflowInput();

    void createPondedAreaInput();

    void createSurfaceInflows();

    void createOutputs() override;

    void createIdBasedNodeWSEOutput();

    void createIdBasedLinkFlowOutput();

    void createNodeWSEOutput();

    void createNodeFloodingOutput();

    void createLinkFlowOutput();

    void createLinkDepthOutput();

    void createConduitXSectAreaOutput();

    void createConduitTopWidthOutput();

    void createConduitBankXSectAreaOutput();

    bool hasError(QString &message);

    void initializeFailureCleanUp() override;

    void disposeProject();

    void resetSurfaceInflows();

  private:

    std::map<QString, QSharedPointer<HCGeometry> > m_sharedNodesGeoms;
    std::map<QString, QSharedPointer<HCGeometry> > m_sharedLinkGeoms;

    std::vector<double> m_surfaceInflow;
    SWMMComponentInfo *m_SWMMComponentInfo;
    IdBasedArgumentString *m_inputFilesArgument;
    IdBasedArgumentDouble *m_nodeAreas;
    IdBasedArgumentDouble *m_nodePerimeters;
    IdBasedArgumentDouble *m_nodeOrificeDischargeCoeffs;
    Project* m_SWMMProject;

    NodeSurfaceFlowOutput *m_nodeSurfaceFlowOutput;
    LinkFlowOutput *m_linkFlowOutput;
    LinkDepthOutput *m_linkDepthOutput;
    ConduitXSectAreaOutput *m_conduitXSectAreaOutput;
    ConduitTopWidthOutput *m_conduitTopWidthOutput;
    ConduitBankXSectAreaOutput *m_conduitBankXSectAreaOutput;

    PondedAreaInput *m_pondedAreaInput;
    NodeWSEInput *m_nodeWSEInput;
    NodePondedDepthInput *m_nodePondedDepth;

    Dimension *m_idDimension,
    *m_geometryDimension,
    *m_timeDimension;

    std::map<QString,QFileInfo> m_inputFiles;

    //    char *m_inputFile,
    //    *m_reportFile,
    //    *m_outputFile;

    double m_timeStep = 0.001;

    int m_currentProgress;

    SWMMComponent *m_parent;
    QList<HydroCouple::ICloneableModelComponent*> m_clones;
};

Q_DECLARE_METATYPE(SWMMComponent*)

#endif // SWMMCOMPONENT_H
