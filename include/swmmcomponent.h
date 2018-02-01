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
typedef struct Project Project;

class SWMMCOMPONENT_EXPORT SWMMComponent : public AbstractTimeModelComponent
{
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

    Project* project() const;

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

    void initializeNodeGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void initializeLinkGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void initializeSubCatchmentGeometries(QTextStream &streamReader, const QRegExp &delimiter);

    void createInputs() override;

    void initializeIdBasedNodeWSEInput();

    void initializeIdBasedNodeInflowInput();

    void initializeNodeWSEInput();

    void initializeNodePondedDepthInput();

    void initializeNodeInflowInput();

    void initializePondedAreaInput();

    void intializeSurfaceInflows();

    void createOutputs() override;

    void initializeIdBasedNodeWSEOutput();

    void initializeIdBasedLinkFlowOutput();

    void initializeNodeWSEOutput();

    void initializeNodeFloodingOutput();

    void initializeLinkFlowOutput();

    bool hasError(QString &message);

    void intializeFailureCleanUp() override;

    void disposeProject();

    void resetSurfaceInflows();

  private:

    std::map<QString, QSharedPointer<HCGeometry> > m_sharedNodesGeoms;
    std::map<QString, QSharedPointer<HCGeometry> > m_sharedLinkGeoms;

    std::map<int,double> m_surfaceInflow;
    SWMMComponentInfo *m_SWMMComponentInfo = nullptr;
    IdBasedArgumentString *m_inputFilesArgument = nullptr;
    IdBasedArgumentDouble *m_nodeAreas = nullptr;
    IdBasedArgumentDouble *m_nodePerimeters = nullptr;
    IdBasedArgumentDouble *m_nodeOrificeDischargeCoeffs = nullptr;
    Project* m_SWMMProject = nullptr;

    NodeSurfaceFlowOutput *m_nodeSurfaceFlowOutput = nullptr;
    LinkFlowOutput *m_linkFlowOutput = nullptr;

    PondedAreaInput *m_pondedAreaInput = nullptr;
    NodeWSEInput *m_nodeWSEInput = nullptr;
    NodePondedDepthInput *m_nodePondedDepth = nullptr;

    Dimension *m_idDimension = nullptr,
              *m_geometryDimension = nullptr,
              *m_timeDimension = nullptr;

    std::map<QString,QFileInfo> m_inputFiles;
    SDKTemporal::DateTime *m_startDateTime = nullptr,
    *m_endDateTime = nullptr,
    *m_currentDateTime = nullptr;

    QTextStream m_scratchFileTextStream;
    QFile m_scratchFileIO;
    int m_currentNPeriods = -1000;

    double m_timeStep = 0.001;

    int m_currentProgress;
};

Q_DECLARE_METATYPE(SWMMComponent*)

#endif // SWMMCOMPONENT_H
